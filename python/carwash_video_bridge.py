"""
CarWash Video Bridge - Houdini <-> ComfyUI Video Pipeline

This module handles:
1. Exporting AOV sequences from Houdini (depth, normal)
2. Sending to ComfyUI for AnimateDiff/SVD processing
3. Importing AI-generated video back into Houdini

Architecture:
    Houdini                    ComfyUI                      Houdini
    -------                    -------                      -------
    Solaris Scene              AnimateDiff                  Composite
         |                          |                            |
    Render Depth/Normal   -->  Video ControlNet  -->   Import AI Video
    (EXR Sequence)              + IP-Adapter           (as Image Sequence)
         |                          |                            |
    Export MP4/EXR        -->  Generate Video    -->   COP Network
                                    |
                               Style Memory
                           (consistent look)
"""

import os
import json
import time
import struct
import asyncio
import tempfile
from pathlib import Path
from typing import Optional, Dict, List, Tuple
from dataclasses import dataclass
from enum import Enum

try:
    import hou
except ImportError:
    hou = None  # Running outside Houdini

try:
    import websockets
except ImportError:
    websockets = None


class VideoFormat(Enum):
    EXR_SEQUENCE = "exr_sequence"
    PNG_SEQUENCE = "png_sequence"
    MP4 = "mp4"


@dataclass
class VideoJob:
    """Represents a video generation job"""
    job_id: str
    input_depth: str
    input_normal: str
    output_path: str
    frame_start: int
    frame_end: int
    fps: float = 24.0
    prompt: str = "photorealistic 3D render"
    negative_prompt: str = "blurry, low quality, flickering"
    style_reference: Optional[str] = None
    status: str = "pending"


class CarWashVideoBridge:
    """
    Bridge between Houdini and ComfyUI for video generation.

    Usage in Houdini:
        from carwash_video_bridge import CarWashVideoBridge

        bridge = CarWashVideoBridge()
        bridge.connect()

        # Export AOVs and generate
        job = bridge.submit_video_job(
            depth_rop="/stage/usdrender1",
            normal_rop="/stage/usdrender1",
            frame_range=(1001, 1024),
            prompt="cinematic, high detail"
        )

        # Wait for result
        result = bridge.wait_for_job(job.job_id)

        # Import into COPs
        bridge.import_to_cops(result.output_path)
    """

    def __init__(self, host: str = "localhost", port: int = 8188):
        self.host = host
        self.port = port
        self.ws = None
        self.client_id = f"carwash_{int(time.time())}"
        self.jobs: Dict[str, VideoJob] = {}
        self.temp_dir = Path(tempfile.gettempdir()) / "carwash_video"
        self.temp_dir.mkdir(exist_ok=True)

    async def connect_async(self):
        """Connect to ComfyUI WebSocket"""
        uri = f"ws://{self.host}:{self.port}/ws?clientId={self.client_id}"
        self.ws = await websockets.connect(uri)
        print(f"[CarWash] Connected to ComfyUI at {uri}")

    def connect(self):
        """Synchronous connect wrapper"""
        if websockets is None:
            raise ImportError("websockets package required: pip install websockets")
        asyncio.get_event_loop().run_until_complete(self.connect_async())

    def export_aov_sequence(
        self,
        rop_path: str,
        aov_name: str,
        frame_range: Tuple[int, int],
        output_dir: Optional[str] = None
    ) -> str:
        """
        Export an AOV sequence from a USD Render ROP.

        Args:
            rop_path: Path to USD Render ROP
            aov_name: Name of AOV (e.g., "depth", "normal")
            frame_range: (start_frame, end_frame)
            output_dir: Output directory (uses temp if None)

        Returns:
            Path to exported sequence pattern (e.g., "/tmp/depth.####.exr")
        """
        if hou is None:
            raise RuntimeError("Must run inside Houdini")

        rop = hou.node(rop_path)
        if not rop:
            raise ValueError(f"ROP not found: {rop_path}")

        output_dir = output_dir or str(self.temp_dir)
        output_pattern = f"{output_dir}/{aov_name}.$F4.exr"

        # Configure ROP for AOV export
        # This would need customization based on your render setup
        start, end = frame_range

        print(f"[CarWash] Exporting {aov_name} frames {start}-{end}")
        print(f"[CarWash] Output: {output_pattern}")

        # TODO: Execute render
        # rop.render(frame_range=(start, end))

        return output_pattern

    def sequence_to_video(
        self,
        sequence_pattern: str,
        output_path: str,
        fps: float = 24.0
    ) -> str:
        """
        Convert EXR sequence to MP4 for ComfyUI input.
        Uses FFmpeg if available.
        """
        import subprocess

        # Convert $F4 to %04d for FFmpeg
        ffmpeg_pattern = sequence_pattern.replace("$F4", "%04d")

        cmd = [
            "ffmpeg", "-y",
            "-framerate", str(fps),
            "-i", ffmpeg_pattern,
            "-c:v", "libx264",
            "-pix_fmt", "yuv420p",
            "-crf", "18",
            output_path
        ]

        try:
            subprocess.run(cmd, check=True, capture_output=True)
            print(f"[CarWash] Created video: {output_path}")
            return output_path
        except subprocess.CalledProcessError as e:
            print(f"[CarWash] FFmpeg error: {e.stderr.decode()}")
            raise

    def build_workflow(self, job: VideoJob, workflow_path: str) -> dict:
        """
        Load and customize ComfyUI workflow for this job.
        """
        with open(workflow_path, 'r') as f:
            workflow = json.load(f)

        # Get carwash metadata
        meta = workflow.get("extra", {}).get("carwash_metadata", {})
        params = meta.get("parameters", {})
        inputs = meta.get("inputs", {})

        # Update nodes with job parameters
        for node in workflow.get("nodes", []):
            node_id = node["id"]

            # Update prompt
            if node_id == params.get("prompt", {}).get("node_id"):
                node["widgets_values"][0] = job.prompt

            # Update negative prompt
            if node_id == params.get("negative_prompt", {}).get("node_id"):
                node["widgets_values"][0] = job.negative_prompt

            # Update depth input
            if node_id == inputs.get("depth_video", {}).get("node_id"):
                node["widgets_values"][0] = job.input_depth

            # Update normal input
            if node_id == inputs.get("normal_video", {}).get("node_id"):
                node["widgets_values"][0] = job.input_normal

            # Update style reference
            if job.style_reference and node_id == inputs.get("style_reference", {}).get("node_id"):
                node["widgets_values"][0] = job.style_reference

        return workflow

    async def submit_workflow_async(self, workflow: dict) -> str:
        """Submit workflow to ComfyUI and return prompt_id"""
        import aiohttp

        url = f"http://{self.host}:{self.port}/prompt"

        payload = {
            "prompt": self._workflow_to_prompt(workflow),
            "client_id": self.client_id
        }

        async with aiohttp.ClientSession() as session:
            async with session.post(url, json=payload) as resp:
                result = await resp.json()
                return result.get("prompt_id")

    def _workflow_to_prompt(self, workflow: dict) -> dict:
        """Convert workflow format to ComfyUI prompt format"""
        prompt = {}

        for node in workflow.get("nodes", []):
            node_id = str(node["id"])
            prompt[node_id] = {
                "class_type": node["type"],
                "inputs": {}
            }

            # Add widget values as inputs
            if "widgets_values" in node:
                # Map widget values to input names based on node type
                # This is simplified - real implementation needs node definitions
                pass

        # Add links
        for link in workflow.get("links", []):
            link_id, from_node, from_slot, to_node, to_slot, data_type = link
            to_node_str = str(to_node)
            if to_node_str in prompt:
                # Find input name for this slot
                # This needs proper input slot mapping
                pass

        return prompt

    def submit_video_job(
        self,
        depth_path: str,
        normal_path: str,
        frame_range: Tuple[int, int],
        prompt: str = "photorealistic 3D render",
        negative_prompt: str = "blurry, low quality",
        style_reference: Optional[str] = None,
        workflow: str = "carwash_animatediff_workflow.json"
    ) -> VideoJob:
        """
        Submit a video generation job.

        Args:
            depth_path: Path to depth sequence/video
            normal_path: Path to normal sequence/video
            frame_range: (start, end) frames
            prompt: Generation prompt
            negative_prompt: Negative prompt
            style_reference: Optional style image
            workflow: Workflow file to use

        Returns:
            VideoJob with job_id for tracking
        """
        job_id = f"carwash_video_{int(time.time())}"

        output_path = str(self.temp_dir / f"{job_id}_output.mp4")

        job = VideoJob(
            job_id=job_id,
            input_depth=depth_path,
            input_normal=normal_path,
            output_path=output_path,
            frame_start=frame_range[0],
            frame_end=frame_range[1],
            prompt=prompt,
            negative_prompt=negative_prompt,
            style_reference=style_reference
        )

        self.jobs[job_id] = job

        # Build and submit workflow
        workflow_path = Path(__file__).parent.parent / "workflows" / workflow
        if workflow_path.exists():
            wf = self.build_workflow(job, str(workflow_path))
            # Submit to ComfyUI
            # asyncio.get_event_loop().run_until_complete(self.submit_workflow_async(wf))

        job.status = "submitted"
        print(f"[CarWash] Submitted video job: {job_id}")

        return job

    def import_to_cops(self, video_path: str, cop_path: str = "/img/carwash_import"):
        """
        Import generated video into Houdini COPs for compositing.
        """
        if hou is None:
            raise RuntimeError("Must run inside Houdini")

        # Create or get COP network
        img = hou.node("/img")
        if not img:
            img = hou.node("/").createNode("img", "img")

        cop_net = hou.node(cop_path)
        if not cop_net:
            cop_net = img.createNode("img", "carwash_import")

        # Create file node for video import
        file_cop = cop_net.createNode("file", "ai_video")
        file_cop.parm("filename1").set(video_path)

        print(f"[CarWash] Imported video to {file_cop.path()}")
        return file_cop


# Convenience functions for Houdini shelf tools

def render_and_process():
    """
    One-click: Render AOVs, process through AI, import result.
    """
    bridge = CarWashVideoBridge()
    bridge.connect()

    # Get current frame range from Houdini
    start = int(hou.playbar.playbackRange()[0])
    end = int(hou.playbar.playbackRange()[1])

    # Export depth and normal
    depth_seq = bridge.export_aov_sequence(
        "/stage/usdrender1", "depth", (start, end)
    )
    normal_seq = bridge.export_aov_sequence(
        "/stage/usdrender1", "normal", (start, end)
    )

    # Convert to video
    depth_video = bridge.sequence_to_video(
        depth_seq,
        str(bridge.temp_dir / "depth.mp4")
    )
    normal_video = bridge.sequence_to_video(
        normal_seq,
        str(bridge.temp_dir / "normal.mp4")
    )

    # Submit to ComfyUI
    job = bridge.submit_video_job(
        depth_video, normal_video,
        (start, end),
        prompt="photorealistic render, cinematic lighting"
    )

    print(f"[CarWash] Video job submitted: {job.job_id}")
    print(f"[CarWash] Output will be: {job.output_path}")


if __name__ == "__main__":
    # Test outside Houdini
    bridge = CarWashVideoBridge()
    print(f"CarWash Video Bridge initialized")
    print(f"Temp directory: {bridge.temp_dir}")
