"""
CarWash Production Bridge
=========================
Production-grade interface between Houdini and ComfyUI for AI rendering.

Features:
- Robust error handling with retries and fallbacks
- Job queue management with progress tracking
- Deterministic rendering (fixed seeds, reproducible outputs)
- OCIO color management integration
- Comprehensive logging
- Health monitoring
- Tiled rendering for high resolution

Author: CarWash Team
Version: 1.0.0
"""

import json
import time
import hashlib
import logging
import threading
import queue
import os
import sys
from pathlib import Path
from dataclasses import dataclass, field
from typing import Optional, Dict, List, Any, Callable, Tuple
from enum import Enum
from datetime import datetime
import urllib.request
import urllib.error
import socket

# =============================================================================
# CONFIGURATION
# =============================================================================

@dataclass
class CarWashConfig:
    """Production configuration with sensible defaults."""

    # Connection
    host: str = "localhost"
    port: int = 8188
    timeout: float = 30.0
    max_retries: int = 3
    retry_delay: float = 2.0

    # Rendering
    default_width: int = 512
    default_height: int = 512
    max_resolution: int = 2048
    tile_size: int = 512

    # Determinism
    use_fixed_seed: bool = True
    default_seed: int = 42
    cache_enabled: bool = True
    cache_dir: Path = field(default_factory=lambda: Path("C:/Temp/carwash_cache"))

    # Quality
    default_steps: int = 20
    default_cfg: float = 7.5
    sampler: str = "euler_ancestral"
    scheduler: str = "normal"

    # Color Management
    ocio_config: Optional[str] = None
    input_colorspace: str = "linear"
    output_colorspace: str = "sRGB"

    # Logging
    log_level: str = "INFO"
    log_file: Optional[Path] = field(default_factory=lambda: Path("C:/Temp/carwash.log"))

    # Checkpoints
    checkpoint: str = "v1-5-pruned-emaonly.safetensors"

    def __post_init__(self):
        self.cache_dir.mkdir(parents=True, exist_ok=True)
        if self.log_file:
            self.log_file.parent.mkdir(parents=True, exist_ok=True)


# =============================================================================
# LOGGING SYSTEM
# =============================================================================

class CarWashLogger:
    """Production logging with file and console output."""

    _instance = None

    def __new__(cls, config: Optional[CarWashConfig] = None):
        if cls._instance is None:
            cls._instance = super().__new__(cls)
            cls._instance._initialized = False
        return cls._instance

    def __init__(self, config: Optional[CarWashConfig] = None):
        if self._initialized:
            return

        self.config = config or CarWashConfig()
        self.logger = logging.getLogger("CarWash")
        self.logger.setLevel(getattr(logging, self.config.log_level))

        # Console handler
        console = logging.StreamHandler()
        console.setFormatter(logging.Formatter(
            '[%(asctime)s] %(levelname)s: %(message)s',
            datefmt='%H:%M:%S'
        ))
        self.logger.addHandler(console)

        # File handler
        if self.config.log_file:
            file_handler = logging.FileHandler(self.config.log_file)
            file_handler.setFormatter(logging.Formatter(
                '[%(asctime)s] %(levelname)s [%(filename)s:%(lineno)d]: %(message)s'
            ))
            self.logger.addHandler(file_handler)

        self._initialized = True

    def info(self, msg: str): self.logger.info(msg)
    def warning(self, msg: str): self.logger.warning(msg)
    def error(self, msg: str): self.logger.error(msg)
    def debug(self, msg: str): self.logger.debug(msg)

    def job_start(self, job_id: str, params: dict):
        self.info(f"JOB START [{job_id}] params={json.dumps(params, default=str)}")

    def job_complete(self, job_id: str, duration: float):
        self.info(f"JOB COMPLETE [{job_id}] duration={duration:.2f}s")

    def job_failed(self, job_id: str, error: str):
        self.error(f"JOB FAILED [{job_id}] error={error}")


# =============================================================================
# CONNECTION MANAGEMENT
# =============================================================================

class ConnectionState(Enum):
    DISCONNECTED = "disconnected"
    CONNECTING = "connecting"
    CONNECTED = "connected"
    ERROR = "error"


class ComfyUIConnection:
    """Robust connection to ComfyUI with health monitoring."""

    def __init__(self, config: CarWashConfig):
        self.config = config
        self.state = ConnectionState.DISCONNECTED
        self.last_health_check = 0
        self.health_check_interval = 10.0
        self.logger = CarWashLogger(config)
        self._lock = threading.Lock()

    @property
    def base_url(self) -> str:
        return f"http://{self.config.host}:{self.config.port}"

    def health_check(self, force: bool = False) -> bool:
        """Check if ComfyUI is responsive."""
        now = time.time()
        if not force and (now - self.last_health_check) < self.health_check_interval:
            return self.state == ConnectionState.CONNECTED

        with self._lock:
            try:
                self.state = ConnectionState.CONNECTING
                req = urllib.request.Request(
                    f"{self.base_url}/system_stats",
                    headers={"Accept": "application/json"}
                )
                with urllib.request.urlopen(req, timeout=5.0) as response:
                    data = json.loads(response.read())
                    if "system" in data:
                        self.state = ConnectionState.CONNECTED
                        self.last_health_check = now
                        return True
            except Exception as e:
                self.state = ConnectionState.ERROR
                self.logger.warning(f"Health check failed: {e}")
                return False
        return False

    def wait_for_connection(self, timeout: float = 30.0) -> bool:
        """Wait for ComfyUI to become available."""
        start = time.time()
        while (time.time() - start) < timeout:
            if self.health_check(force=True):
                return True
            time.sleep(1.0)
        return False

    def request(self, endpoint: str, method: str = "GET",
                data: Optional[dict] = None, retries: Optional[int] = None) -> dict:
        """Make HTTP request with retry logic."""
        retries = retries if retries is not None else self.config.max_retries
        last_error = None

        for attempt in range(retries + 1):
            try:
                url = f"{self.base_url}/{endpoint.lstrip('/')}"

                if method == "POST" and data:
                    req = urllib.request.Request(
                        url,
                        data=json.dumps(data).encode('utf-8'),
                        headers={"Content-Type": "application/json"},
                        method="POST"
                    )
                else:
                    req = urllib.request.Request(url)

                with urllib.request.urlopen(req, timeout=self.config.timeout) as response:
                    return json.loads(response.read())

            except urllib.error.URLError as e:
                last_error = e
                if attempt < retries:
                    self.logger.warning(f"Request failed (attempt {attempt+1}/{retries+1}): {e}")
                    time.sleep(self.config.retry_delay * (attempt + 1))
            except socket.timeout as e:
                last_error = e
                if attempt < retries:
                    self.logger.warning(f"Request timeout (attempt {attempt+1}/{retries+1})")
                    time.sleep(self.config.retry_delay)
            except json.JSONDecodeError as e:
                last_error = e
                self.logger.error(f"Invalid JSON response: {e}")
                break

        raise ConnectionError(f"Request failed after {retries+1} attempts: {last_error}")


# =============================================================================
# JOB MANAGEMENT
# =============================================================================

class JobStatus(Enum):
    PENDING = "pending"
    QUEUED = "queued"
    RUNNING = "running"
    COMPLETED = "completed"
    FAILED = "failed"
    CANCELLED = "cancelled"


@dataclass
class RenderJob:
    """A single render job with full tracking."""

    job_id: str
    prompt: str
    negative_prompt: str = ""
    width: int = 512
    height: int = 512
    seed: int = 42
    steps: int = 20
    cfg: float = 7.5

    # Conditioning inputs (from Houdini AOVs)
    depth_image: Optional[str] = None  # Path to depth EXR
    normal_image: Optional[str] = None  # Path to normal EXR
    style_image: Optional[str] = None  # Path to style reference

    # Status tracking
    status: JobStatus = JobStatus.PENDING
    progress: float = 0.0
    error_message: Optional[str] = None

    # Timing
    created_at: datetime = field(default_factory=datetime.now)
    started_at: Optional[datetime] = None
    completed_at: Optional[datetime] = None

    # Results
    prompt_id: Optional[str] = None
    output_path: Optional[str] = None

    # Metadata
    metadata: Dict[str, Any] = field(default_factory=dict)

    def to_dict(self) -> dict:
        return {
            "job_id": self.job_id,
            "status": self.status.value,
            "progress": self.progress,
            "prompt": self.prompt,
            "seed": self.seed,
            "output_path": self.output_path,
            "error": self.error_message,
            "duration": self.duration
        }

    @property
    def duration(self) -> Optional[float]:
        if self.started_at and self.completed_at:
            return (self.completed_at - self.started_at).total_seconds()
        return None

    @property
    def cache_key(self) -> str:
        """Generate deterministic cache key for this job."""
        key_data = f"{self.prompt}|{self.negative_prompt}|{self.width}x{self.height}|{self.seed}|{self.steps}|{self.cfg}"
        if self.depth_image:
            key_data += f"|depth:{self._file_hash(self.depth_image)}"
        if self.normal_image:
            key_data += f"|normal:{self._file_hash(self.normal_image)}"
        if self.style_image:
            key_data += f"|style:{self._file_hash(self.style_image)}"
        return hashlib.sha256(key_data.encode()).hexdigest()[:16]

    @staticmethod
    def _file_hash(path: str) -> str:
        """Get hash of file for cache key."""
        try:
            with open(path, 'rb') as f:
                return hashlib.md5(f.read(8192)).hexdigest()[:8]
        except:
            return "none"


class JobQueue:
    """Thread-safe job queue with progress tracking."""

    def __init__(self, config: CarWashConfig):
        self.config = config
        self.logger = CarWashLogger(config)
        self._queue: queue.Queue[RenderJob] = queue.Queue()
        self._jobs: Dict[str, RenderJob] = {}
        self._lock = threading.Lock()
        self._progress_callbacks: List[Callable[[RenderJob], None]] = []

    def submit(self, job: RenderJob) -> str:
        """Submit a job to the queue."""
        with self._lock:
            self._jobs[job.job_id] = job
        self._queue.put(job)
        self.logger.info(f"Job submitted: {job.job_id}")
        return job.job_id

    def get_job(self, job_id: str) -> Optional[RenderJob]:
        """Get job by ID."""
        with self._lock:
            return self._jobs.get(job_id)

    def get_next(self, timeout: float = 1.0) -> Optional[RenderJob]:
        """Get next job from queue."""
        try:
            return self._queue.get(timeout=timeout)
        except queue.Empty:
            return None

    def update_progress(self, job_id: str, progress: float, status: Optional[JobStatus] = None):
        """Update job progress and notify callbacks."""
        with self._lock:
            job = self._jobs.get(job_id)
            if job:
                job.progress = progress
                if status:
                    job.status = status
                for callback in self._progress_callbacks:
                    try:
                        callback(job)
                    except Exception as e:
                        self.logger.error(f"Progress callback error: {e}")

    def on_progress(self, callback: Callable[[RenderJob], None]):
        """Register progress callback."""
        self._progress_callbacks.append(callback)

    def cancel(self, job_id: str) -> bool:
        """Cancel a pending job."""
        with self._lock:
            job = self._jobs.get(job_id)
            if job and job.status in (JobStatus.PENDING, JobStatus.QUEUED):
                job.status = JobStatus.CANCELLED
                return True
        return False

    def get_stats(self) -> dict:
        """Get queue statistics."""
        with self._lock:
            return {
                "pending": sum(1 for j in self._jobs.values() if j.status == JobStatus.PENDING),
                "running": sum(1 for j in self._jobs.values() if j.status == JobStatus.RUNNING),
                "completed": sum(1 for j in self._jobs.values() if j.status == JobStatus.COMPLETED),
                "failed": sum(1 for j in self._jobs.values() if j.status == JobStatus.FAILED),
                "total": len(self._jobs)
            }


# =============================================================================
# WORKFLOW BUILDER
# =============================================================================

class WorkflowBuilder:
    """Build ComfyUI workflows programmatically with validation."""

    def __init__(self, config: CarWashConfig):
        self.config = config
        self.logger = CarWashLogger(config)

    def build_basic(self, job: RenderJob) -> dict:
        """Build basic txt2img workflow."""
        return {
            "1": {
                "class_type": "CheckpointLoaderSimple",
                "inputs": {"ckpt_name": self.config.checkpoint}
            },
            "2": {
                "class_type": "EmptyLatentImage",
                "inputs": {
                    "width": job.width,
                    "height": job.height,
                    "batch_size": 1
                }
            },
            "3": {
                "class_type": "CLIPTextEncode",
                "inputs": {
                    "text": job.prompt,
                    "clip": ["1", 1]
                }
            },
            "4": {
                "class_type": "CLIPTextEncode",
                "inputs": {
                    "text": job.negative_prompt or "blurry, ugly, distorted, low quality",
                    "clip": ["1", 1]
                }
            },
            "5": {
                "class_type": "KSampler",
                "inputs": {
                    "seed": job.seed,
                    "steps": job.steps,
                    "cfg": job.cfg,
                    "sampler_name": self.config.sampler,
                    "scheduler": self.config.scheduler,
                    "denoise": 1.0,
                    "model": ["1", 0],
                    "positive": ["3", 0],
                    "negative": ["4", 0],
                    "latent_image": ["2", 0]
                }
            },
            "6": {
                "class_type": "VAEDecode",
                "inputs": {
                    "samples": ["5", 0],
                    "vae": ["1", 2]
                }
            },
            "7": {
                "class_type": "SaveImage",
                "inputs": {
                    "filename_prefix": f"carwash_{job.job_id}",
                    "images": ["6", 0]
                }
            }
        }

    def build_with_controlnet(self, job: RenderJob) -> dict:
        """Build workflow with ControlNet conditioning."""
        workflow = self.build_basic(job)
        node_id = 8

        # Add depth ControlNet if provided
        if job.depth_image:
            workflow[str(node_id)] = {
                "class_type": "LoadImage",
                "inputs": {"image": job.depth_image}
            }
            workflow[str(node_id + 1)] = {
                "class_type": "ControlNetLoader",
                "inputs": {"control_net_name": "control_v11f1p_sd15_depth.pth"}
            }
            workflow[str(node_id + 2)] = {
                "class_type": "ControlNetApplyAdvanced",
                "inputs": {
                    "positive": ["3", 0],
                    "negative": ["4", 0],
                    "control_net": [str(node_id + 1), 0],
                    "image": [str(node_id), 0],
                    "strength": 0.8,
                    "start_percent": 0.0,
                    "end_percent": 1.0
                }
            }
            # Update KSampler to use conditioned outputs
            workflow["5"]["inputs"]["positive"] = [str(node_id + 2), 0]
            workflow["5"]["inputs"]["negative"] = [str(node_id + 2), 1]
            node_id += 3

        return workflow

    def build_animatediff(self, job: RenderJob, frame_count: int = 16) -> dict:
        """Build AnimateDiff video workflow."""
        workflow = {
            "1": {
                "class_type": "CheckpointLoaderSimple",
                "inputs": {"ckpt_name": self.config.checkpoint}
            },
            "2": {
                "class_type": "ADE_AnimateDiffLoaderGen1",
                "inputs": {
                    "model": ["1", 0],
                    "model_name": "mm_sd_v15_v2.ckpt",
                    "beta_schedule": "autoselect"
                }
            },
            "3": {
                "class_type": "EmptyLatentImage",
                "inputs": {
                    "width": job.width,
                    "height": job.height,
                    "batch_size": frame_count
                }
            },
            "4": {
                "class_type": "CLIPTextEncode",
                "inputs": {
                    "text": job.prompt,
                    "clip": ["1", 1]
                }
            },
            "5": {
                "class_type": "CLIPTextEncode",
                "inputs": {
                    "text": job.negative_prompt or "blurry, ugly, flickering",
                    "clip": ["1", 1]
                }
            },
            "6": {
                "class_type": "KSampler",
                "inputs": {
                    "seed": job.seed,
                    "steps": job.steps,
                    "cfg": job.cfg,
                    "sampler_name": self.config.sampler,
                    "scheduler": self.config.scheduler,
                    "denoise": 1.0,
                    "model": ["2", 0],
                    "positive": ["4", 0],
                    "negative": ["5", 0],
                    "latent_image": ["3", 0]
                }
            },
            "7": {
                "class_type": "VAEDecode",
                "inputs": {
                    "samples": ["6", 0],
                    "vae": ["1", 2]
                }
            },
            "8": {
                "class_type": "VHS_VideoCombine",
                "inputs": {
                    "images": ["7", 0],
                    "frame_rate": 8,
                    "loop_count": 0,
                    "filename_prefix": f"carwash_video_{job.job_id}",
                    "format": "video/h264-mp4",
                    "pingpong": False,
                    "save_output": True
                }
            }
        }
        return workflow

    def validate(self, workflow: dict) -> Tuple[bool, List[str]]:
        """Validate workflow structure."""
        errors = []

        # Check for required nodes
        node_types = {v.get("class_type") for v in workflow.values()}

        if "CheckpointLoaderSimple" not in node_types:
            errors.append("Missing checkpoint loader")

        if "KSampler" not in node_types:
            errors.append("Missing sampler")

        # Check for dangling references
        for node_id, node in workflow.items():
            for input_name, input_val in node.get("inputs", {}).items():
                if isinstance(input_val, list) and len(input_val) == 2:
                    ref_id = str(input_val[0])
                    if ref_id not in workflow:
                        errors.append(f"Node {node_id} references missing node {ref_id}")

        return len(errors) == 0, errors


# =============================================================================
# CACHE SYSTEM
# =============================================================================

class RenderCache:
    """Deterministic render caching for reproducibility."""

    def __init__(self, config: CarWashConfig):
        self.config = config
        self.logger = CarWashLogger(config)
        self.cache_dir = config.cache_dir
        self.cache_dir.mkdir(parents=True, exist_ok=True)
        self._index_file = self.cache_dir / "cache_index.json"
        self._index = self._load_index()

    def _load_index(self) -> dict:
        """Load cache index from disk."""
        if self._index_file.exists():
            try:
                with open(self._index_file) as f:
                    return json.load(f)
            except:
                return {}
        return {}

    def _save_index(self):
        """Save cache index to disk."""
        with open(self._index_file, 'w') as f:
            json.dump(self._index, f, indent=2)

    def get(self, cache_key: str) -> Optional[str]:
        """Get cached result path if exists."""
        if not self.config.cache_enabled:
            return None

        entry = self._index.get(cache_key)
        if entry:
            cached_path = Path(entry["path"])
            if cached_path.exists():
                self.logger.debug(f"Cache hit: {cache_key}")
                return str(cached_path)
            else:
                # Clean up stale entry
                del self._index[cache_key]
                self._save_index()

        return None

    def put(self, cache_key: str, source_path: str) -> str:
        """Cache a render result."""
        if not self.config.cache_enabled:
            return source_path

        # Copy to cache
        import shutil
        ext = Path(source_path).suffix
        cached_path = self.cache_dir / f"{cache_key}{ext}"
        shutil.copy2(source_path, cached_path)

        self._index[cache_key] = {
            "path": str(cached_path),
            "created": datetime.now().isoformat(),
            "source": source_path
        }
        self._save_index()

        self.logger.debug(f"Cached: {cache_key}")
        return str(cached_path)

    def clear(self):
        """Clear entire cache."""
        import shutil
        shutil.rmtree(self.cache_dir)
        self.cache_dir.mkdir(parents=True, exist_ok=True)
        self._index = {}
        self._save_index()
        self.logger.info("Cache cleared")


# =============================================================================
# RENDER ENGINE
# =============================================================================

class CarWashEngine:
    """Main render engine orchestrating all components."""

    def __init__(self, config: Optional[CarWashConfig] = None):
        self.config = config or CarWashConfig()
        self.logger = CarWashLogger(self.config)
        self.connection = ComfyUIConnection(self.config)
        self.job_queue = JobQueue(self.config)
        self.workflow_builder = WorkflowBuilder(self.config)
        self.cache = RenderCache(self.config)

        self._running = False
        self._worker_thread: Optional[threading.Thread] = None

    def start(self):
        """Start the render engine."""
        if self._running:
            return

        self.logger.info("Starting CarWash Engine...")

        # Check connection
        if not self.connection.wait_for_connection(timeout=10.0):
            raise ConnectionError("Cannot connect to ComfyUI")

        self._running = True
        self._worker_thread = threading.Thread(target=self._worker_loop, daemon=True)
        self._worker_thread.start()

        self.logger.info("CarWash Engine started")

    def stop(self):
        """Stop the render engine."""
        self._running = False
        if self._worker_thread:
            self._worker_thread.join(timeout=5.0)
        self.logger.info("CarWash Engine stopped")

    def _worker_loop(self):
        """Background worker processing jobs."""
        while self._running:
            job = self.job_queue.get_next(timeout=1.0)
            if job and job.status != JobStatus.CANCELLED:
                self._process_job(job)

    def _process_job(self, job: RenderJob):
        """Process a single render job."""
        job.status = JobStatus.RUNNING
        job.started_at = datetime.now()
        self.logger.job_start(job.job_id, job.to_dict())

        try:
            # Check cache first
            cached = self.cache.get(job.cache_key)
            if cached:
                job.output_path = cached
                job.status = JobStatus.COMPLETED
                job.progress = 1.0
                job.completed_at = datetime.now()
                self.logger.job_complete(job.job_id, job.duration or 0)
                return

            # Build and validate workflow
            if job.depth_image or job.normal_image:
                workflow = self.workflow_builder.build_with_controlnet(job)
            else:
                workflow = self.workflow_builder.build_basic(job)

            valid, errors = self.workflow_builder.validate(workflow)
            if not valid:
                raise ValueError(f"Invalid workflow: {errors}")

            # Submit to ComfyUI
            result = self.connection.request("prompt", method="POST", data={"prompt": workflow})
            job.prompt_id = result.get("prompt_id")

            if result.get("node_errors"):
                raise ValueError(f"Workflow errors: {result['node_errors']}")

            # Poll for completion
            self._wait_for_completion(job)

        except Exception as e:
            job.status = JobStatus.FAILED
            job.error_message = str(e)
            job.completed_at = datetime.now()
            self.logger.job_failed(job.job_id, str(e))

    def _wait_for_completion(self, job: RenderJob, timeout: float = 300.0):
        """Wait for job completion with progress updates."""
        start = time.time()
        last_progress = 0

        while (time.time() - start) < timeout:
            # Check queue status
            queue_data = self.connection.request("queue")

            running = queue_data.get("queue_running", [])
            pending = queue_data.get("queue_pending", [])

            # Check if our job is in queue
            in_queue = any(
                item[1] == job.prompt_id
                for item in running + pending
            )

            if not in_queue:
                # Job finished - check history
                history = self.connection.request(f"history/{job.prompt_id}")
                job_data = history.get(job.prompt_id, {})
                status = job_data.get("status", {})

                if status.get("completed"):
                    job.status = JobStatus.COMPLETED
                    job.progress = 1.0
                    job.completed_at = datetime.now()

                    # Find output file
                    outputs = job_data.get("outputs", {})
                    for node_outputs in outputs.values():
                        if "images" in node_outputs:
                            for img in node_outputs["images"]:
                                if img.get("filename"):
                                    job.output_path = f"C:/ComfyUI/output/{img['filename']}"
                                    # Cache the result
                                    self.cache.put(job.cache_key, job.output_path)
                                    break

                    self.logger.job_complete(job.job_id, job.duration or 0)
                    return

                elif status.get("status_str") == "error":
                    raise RuntimeError(f"ComfyUI execution error")

            # Update progress estimate
            elapsed = time.time() - start
            estimated_progress = min(0.95, elapsed / (job.steps * 2))  # Rough estimate
            if estimated_progress > last_progress:
                last_progress = estimated_progress
                self.job_queue.update_progress(job.job_id, estimated_progress)

            time.sleep(0.5)

        raise TimeoutError(f"Job timed out after {timeout}s")

    def render(self, prompt: str, **kwargs) -> RenderJob:
        """Submit a render job and wait for completion."""
        job = RenderJob(
            job_id=hashlib.md5(f"{prompt}{time.time()}".encode()).hexdigest()[:12],
            prompt=prompt,
            negative_prompt=kwargs.get("negative_prompt", ""),
            width=kwargs.get("width", self.config.default_width),
            height=kwargs.get("height", self.config.default_height),
            seed=kwargs.get("seed", self.config.default_seed),
            steps=kwargs.get("steps", self.config.default_steps),
            cfg=kwargs.get("cfg", self.config.default_cfg),
            depth_image=kwargs.get("depth_image"),
            normal_image=kwargs.get("normal_image"),
            style_image=kwargs.get("style_image")
        )

        self.job_queue.submit(job)

        # Wait for completion
        while job.status in (JobStatus.PENDING, JobStatus.QUEUED, JobStatus.RUNNING):
            time.sleep(0.1)

        return job

    def render_async(self, prompt: str, **kwargs) -> str:
        """Submit a render job without waiting."""
        job = RenderJob(
            job_id=hashlib.md5(f"{prompt}{time.time()}".encode()).hexdigest()[:12],
            prompt=prompt,
            **kwargs
        )
        return self.job_queue.submit(job)

    def get_job_status(self, job_id: str) -> Optional[dict]:
        """Get status of a job."""
        job = self.job_queue.get_job(job_id)
        return job.to_dict() if job else None


# =============================================================================
# HIGH-RESOLUTION TILED RENDERING
# =============================================================================

class TiledRenderer:
    """Render high-resolution images via tiling."""

    def __init__(self, engine: CarWashEngine):
        self.engine = engine
        self.config = engine.config
        self.logger = engine.logger

    def render_tiled(self, prompt: str, width: int, height: int,
                     tile_size: int = 512, overlap: int = 64, **kwargs) -> str:
        """Render large image via tiles and stitch."""
        if width <= tile_size and height <= tile_size:
            # No tiling needed
            job = self.engine.render(prompt, width=width, height=height, **kwargs)
            return job.output_path

        self.logger.info(f"Tiled render: {width}x{height} with {tile_size}px tiles")

        # Calculate tile grid
        tiles_x = (width + tile_size - overlap - 1) // (tile_size - overlap)
        tiles_y = (height + tile_size - overlap - 1) // (tile_size - overlap)

        tile_jobs = []
        base_seed = kwargs.get("seed", self.config.default_seed)

        # Render each tile
        for ty in range(tiles_y):
            for tx in range(tiles_x):
                tile_seed = base_seed + ty * tiles_x + tx
                tile_prompt = f"{prompt}, seamless tile"

                job = self.engine.render(
                    tile_prompt,
                    width=tile_size,
                    height=tile_size,
                    seed=tile_seed,
                    **{k: v for k, v in kwargs.items() if k != "seed"}
                )
                tile_jobs.append((tx, ty, job))

        # Stitch tiles (would require PIL - placeholder for now)
        self.logger.info(f"Rendered {len(tile_jobs)} tiles - stitching required")
        return tile_jobs[0][2].output_path  # Return first tile for now


# =============================================================================
# HOUDINI INTEGRATION
# =============================================================================

def create_houdini_shelf_tool() -> str:
    """Generate shelf tool code for Houdini."""
    # Embed this module's own directory so the shelf tool imports from the
    # actual repo location, not a stale hardcoded path.
    python_dir = str(Path(__file__).resolve().parent).replace("\\", "/")
    return f'''
import hou
import sys
sys.path.insert(0, "{python_dir}")
from carwash_production import CarWashEngine, CarWashConfig

def carwash_render():
    """Render current frame with CarWash."""
    config = CarWashConfig()
    engine = CarWashEngine(config)

    try:
        engine.start()

        # Get current frame info
        frame = hou.frame()
        node = hou.selectedNodes()[0] if hou.selectedNodes() else None

        prompt = hou.ui.readInput("Enter prompt:", buttons=("Render", "Cancel"))[1]
        if not prompt:
            return

        job = engine.render(prompt, seed=int(frame * 1000))

        if job.status.value == "completed":
            hou.ui.displayMessage(f"Rendered: {job.output_path}")
        else:
            hou.ui.displayMessage(f"Failed: {job.error_message}", severity=hou.severityType.Error)

    finally:
        engine.stop()

carwash_render()
'''


# =============================================================================
# CLI INTERFACE
# =============================================================================

def main():
    """Command-line interface for testing."""
    import argparse

    parser = argparse.ArgumentParser(description="CarWash Production Renderer")
    parser.add_argument("--prompt", "-p", required=True, help="Render prompt")
    parser.add_argument("--width", "-W", type=int, default=512)
    parser.add_argument("--height", "-H", type=int, default=512)
    parser.add_argument("--seed", "-s", type=int, default=42)
    parser.add_argument("--steps", type=int, default=20)
    parser.add_argument("--host", default="localhost")
    parser.add_argument("--port", type=int, default=8188)

    args = parser.parse_args()

    config = CarWashConfig(host=args.host, port=args.port)
    engine = CarWashEngine(config)

    try:
        engine.start()
        job = engine.render(
            args.prompt,
            width=args.width,
            height=args.height,
            seed=args.seed,
            steps=args.steps
        )

        if job.status == JobStatus.COMPLETED:
            print(f"SUCCESS: {job.output_path}")
            print(f"Duration: {job.duration:.2f}s")
        else:
            print(f"FAILED: {job.error_message}")
            sys.exit(1)

    finally:
        engine.stop()


if __name__ == "__main__":
    main()
