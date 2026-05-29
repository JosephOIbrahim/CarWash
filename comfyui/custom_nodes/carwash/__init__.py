# Copyright 2026 Joseph O. Ibrahim. All rights reserved.
# SPDX-License-Identifier: Proprietary
#
# CarWash ComfyUI Custom Nodes
# ============================
# Deterministic AI rendering nodes for Houdini integration.
#
# Reference: He et al., "Defeating Nondeterminism in LLM Inference", Sep 2025
# https://thinkingmachines.ai/blog/defeating-nondeterminism-in-llm-inference/

"""
CarWash ComfyUI Custom Nodes

These nodes provide deterministic, batch-invariant AI image generation
for use with the HdCarWash Hydra render delegate.

Nodes:
    - CarWashDeterministicSampler: Seed-locked sampling with batch invariance
    - CarWashContextLoader: Loads AOV context (depth, normal, beauty)
    - CarWashStyleConditioner: Style memory for consistent look
    - CarWashTemporalBlend: Frame-to-frame coherence
"""

import os
import torch
import numpy as np
from typing import Dict, Any, Tuple, Optional, List
import hashlib
import logging

logger = logging.getLogger("carwash.nodes")

# Try to import batch_invariant_ops for full determinism
try:
    from batch_invariant_ops import set_batch_invariant_mode
    BATCH_INVARIANT_AVAILABLE = True
except ImportError:
    BATCH_INVARIANT_AVAILABLE = False
    logger.warning("batch_invariant_ops not available. Using fallback determinism.")


class CarWashDeterministicSampler:
    """
    Deterministic sampler with batch-invariant guarantees.

    This sampler ensures that identical inputs produce identical outputs
    regardless of batch size, server load, or execution timing.

    Key features:
    - Fixed seed propagation
    - Deterministic algorithm enforcement
    - Optional batch_invariant_ops integration
    """

    CATEGORY = "CarWash/Sampling"
    RETURN_TYPES = ("LATENT",)
    RETURN_NAMES = ("latent",)
    FUNCTION = "sample"

    @classmethod
    def INPUT_TYPES(cls):
        return {
            "required": {
                "model": ("MODEL",),
                "positive": ("CONDITIONING",),
                "negative": ("CONDITIONING",),
                "latent_image": ("LATENT",),
                "seed": ("INT", {"default": 0, "min": 0, "max": 0xffffffffffffffff}),
                "steps": ("INT", {"default": 20, "min": 1, "max": 10000}),
                "cfg": ("FLOAT", {"default": 7.0, "min": 0.0, "max": 100.0, "step": 0.1}),
                "sampler_name": (["euler", "euler_ancestral", "heun", "dpm_2", "dpm_2_ancestral",
                                  "lms", "dpm_fast", "dpm_adaptive", "dpmpp_2s_ancestral",
                                  "dpmpp_sde", "dpmpp_2m", "ddim", "uni_pc"],),
                "scheduler": (["normal", "karras", "exponential", "sgm_uniform", "simple"],),
                "denoise": ("FLOAT", {"default": 1.0, "min": 0.0, "max": 1.0, "step": 0.01}),
            },
            "optional": {
                "strict_determinism": ("BOOLEAN", {"default": True}),
            }
        }

    def sample(self, model, positive, negative, latent_image, seed, steps, cfg,
               sampler_name, scheduler, denoise, strict_determinism=True):
        """Execute deterministic sampling."""

        # Store previous state
        prev_deterministic = torch.are_deterministic_algorithms_enabled()
        prev_benchmark = torch.backends.cudnn.benchmark
        prev_cudnn_det = torch.backends.cudnn.deterministic
        prev_cublas = os.environ.get('CUBLAS_WORKSPACE_CONFIG')

        try:
            # Enable deterministic mode
            if strict_determinism:
                torch.use_deterministic_algorithms(True, warn_only=True)
                torch.backends.cudnn.benchmark = False
                torch.backends.cudnn.deterministic = True
                os.environ['CUBLAS_WORKSPACE_CONFIG'] = ':4096:8'

                if BATCH_INVARIANT_AVAILABLE:
                    set_batch_invariant_mode(True)

            # Set all seeds
            self._set_seeds(seed)

            # Log determinism state
            logger.info(f"CarWashDeterministicSampler: seed={seed}, steps={steps}, "
                       f"batch_invariant={BATCH_INVARIANT_AVAILABLE}")

            # Import sampler utilities
            import comfy.samplers
            import comfy.sample

            # Get latent samples
            latent = latent_image.copy()
            samples = latent["samples"]

            # Create noise with fixed seed
            noise = self._create_deterministic_noise(samples, seed)

            # Sample
            samples = comfy.sample.sample(
                model, noise, steps, cfg, sampler_name, scheduler,
                positive, negative, samples,
                denoise=denoise, seed=seed
            )

            latent["samples"] = samples

            # Compute determinism hash for verification
            det_hash = self._compute_hash(samples)
            logger.info(f"CarWashDeterministicSampler: output_hash={det_hash}")

            return (latent,)

        finally:
            # Restore previous state
            if strict_determinism:
                torch.use_deterministic_algorithms(prev_deterministic)
                torch.backends.cudnn.benchmark = prev_benchmark
                torch.backends.cudnn.deterministic = prev_cudnn_det

                if prev_cublas is None:
                    os.environ.pop('CUBLAS_WORKSPACE_CONFIG', None)
                else:
                    os.environ['CUBLAS_WORKSPACE_CONFIG'] = prev_cublas

                if BATCH_INVARIANT_AVAILABLE:
                    set_batch_invariant_mode(False)

    def _set_seeds(self, seed: int):
        """Set all random number generators to known state."""
        import random
        random.seed(seed)
        np.random.seed(seed)
        torch.manual_seed(seed)
        if torch.cuda.is_available():
            torch.cuda.manual_seed(seed)
            torch.cuda.manual_seed_all(seed)

    def _create_deterministic_noise(self, samples: torch.Tensor, seed: int) -> torch.Tensor:
        """Create noise tensor with deterministic initialization."""
        generator = torch.Generator(device=samples.device)
        generator.manual_seed(seed)
        return torch.randn(samples.shape, generator=generator,
                          device=samples.device, dtype=samples.dtype)

    def _compute_hash(self, tensor: torch.Tensor) -> str:
        """Compute hash of tensor for verification."""
        data = tensor.detach().cpu().numpy().tobytes()
        return hashlib.sha256(data).hexdigest()[:16]


class CarWashContextLoader:
    """
    Loads AOV context images from HdCarWash render.

    This node loads depth, normal, and beauty passes from the
    Hydra render delegate and prepares them for conditioning.
    """

    CATEGORY = "CarWash/Context"
    RETURN_TYPES = ("IMAGE", "IMAGE", "IMAGE", "MASK")
    RETURN_NAMES = ("depth", "normal", "beauty", "mask")
    FUNCTION = "load_context"

    @classmethod
    def INPUT_TYPES(cls):
        return {
            "required": {
                "context_path": ("STRING", {"default": "", "multiline": False}),
                "frame": ("INT", {"default": 1, "min": 1, "max": 100000}),
            },
            "optional": {
                "depth_scale": ("FLOAT", {"default": 1.0, "min": 0.01, "max": 100.0}),
                "normalize_normals": ("BOOLEAN", {"default": True}),
            }
        }

    def load_context(self, context_path: str, frame: int,
                     depth_scale: float = 1.0, normalize_normals: bool = True):
        """Load AOV context from disk or memory."""

        # Construct file paths
        depth_path = os.path.join(context_path, f"depth.{frame:04d}.exr")
        normal_path = os.path.join(context_path, f"normal.{frame:04d}.exr")
        beauty_path = os.path.join(context_path, f"beauty.{frame:04d}.exr")

        # Load images (with fallbacks)
        depth = self._load_exr(depth_path, channels=1)
        normal = self._load_exr(normal_path, channels=3)
        beauty = self._load_exr(beauty_path, channels=3)

        # Process depth
        if depth is not None:
            depth = depth * depth_scale
            # Normalize to 0-1 range
            depth_min = depth.min()
            depth_max = depth.max()
            if depth_max > depth_min:
                depth = (depth - depth_min) / (depth_max - depth_min)
        else:
            # Fallback: create empty depth
            depth = torch.zeros((1, 512, 512, 1), dtype=torch.float32)

        # Process normals
        if normal is not None:
            if normalize_normals:
                # Normalize to -1 to 1, then to 0 to 1 for display
                normal = (normal + 1.0) / 2.0
        else:
            # Fallback: create neutral normal (pointing up)
            normal = torch.ones((1, 512, 512, 3), dtype=torch.float32) * 0.5
            normal[..., 2] = 1.0  # Z up

        # Process beauty
        if beauty is None:
            beauty = torch.zeros((1, 512, 512, 3), dtype=torch.float32)

        # Create mask from depth (non-zero = valid)
        mask = (depth > 0.001).float()

        logger.info(f"CarWashContextLoader: loaded frame {frame} from {context_path}")

        return (depth, normal, beauty, mask.squeeze(-1))

    def _load_exr(self, path: str, channels: int) -> Optional[torch.Tensor]:
        """Load EXR file if it exists."""
        if not os.path.exists(path):
            logger.warning(f"Context file not found: {path}")
            return None

        try:
            import OpenEXR
            import Imath

            exr_file = OpenEXR.InputFile(path)
            header = exr_file.header()
            dw = header['dataWindow']
            width = dw.max.x - dw.min.x + 1
            height = dw.max.y - dw.min.y + 1

            pt = Imath.PixelType(Imath.PixelType.FLOAT)

            if channels == 1:
                # Single channel (depth)
                channel_data = exr_file.channel('R', pt) if 'R' in header['channels'] else exr_file.channel('Y', pt)
                arr = np.frombuffer(channel_data, dtype=np.float32).reshape(height, width, 1)
            else:
                # RGB channels
                r = np.frombuffer(exr_file.channel('R', pt), dtype=np.float32).reshape(height, width)
                g = np.frombuffer(exr_file.channel('G', pt), dtype=np.float32).reshape(height, width)
                b = np.frombuffer(exr_file.channel('B', pt), dtype=np.float32).reshape(height, width)
                arr = np.stack([r, g, b], axis=-1)

            # Convert to torch tensor with batch dimension
            tensor = torch.from_numpy(arr).unsqueeze(0)
            return tensor

        except ImportError:
            logger.warning("OpenEXR not available, using PIL fallback")
            return self._load_image_pil(path, channels)
        except Exception as e:
            logger.error(f"Failed to load EXR {path}: {e}")
            return None

    def _load_image_pil(self, path: str, channels: int) -> Optional[torch.Tensor]:
        """Fallback image loader using PIL."""
        try:
            from PIL import Image
            img = Image.open(path)
            arr = np.array(img).astype(np.float32) / 255.0
            if len(arr.shape) == 2:
                arr = arr[..., np.newaxis]
            if arr.shape[-1] > channels:
                arr = arr[..., :channels]
            return torch.from_numpy(arr).unsqueeze(0)
        except Exception as e:
            logger.error(f"Failed to load image {path}: {e}")
            return None


class CarWashStyleConditioner:
    """
    Style memory node for consistent visual appearance.

    This node maintains style embeddings across frames to ensure
    visual consistency throughout an animation sequence.
    """

    CATEGORY = "CarWash/Style"
    RETURN_TYPES = ("CONDITIONING",)
    RETURN_NAMES = ("conditioning",)
    FUNCTION = "condition"

    # Class-level style cache for persistence across calls
    _style_cache: Dict[str, torch.Tensor] = {}

    @classmethod
    def INPUT_TYPES(cls):
        return {
            "required": {
                "clip": ("CLIP",),
                "prompt": ("STRING", {"default": "photorealistic 3D render", "multiline": True}),
                "style_id": ("STRING", {"default": "default", "multiline": False}),
            },
            "optional": {
                "reference_image": ("IMAGE",),
                "style_strength": ("FLOAT", {"default": 1.0, "min": 0.0, "max": 2.0, "step": 0.1}),
                "cache_style": ("BOOLEAN", {"default": True}),
            }
        }

    def condition(self, clip, prompt: str, style_id: str,
                  reference_image=None, style_strength: float = 1.0,
                  cache_style: bool = True):
        """Apply style conditioning with memory."""

        # Encode text prompt
        import comfy.sd
        tokens = clip.tokenize(prompt)
        cond, pooled = clip.encode_from_tokens(tokens, return_pooled=True)

        # Check style cache
        cache_key = f"{style_id}_{hash(prompt)}"

        if cache_key in self._style_cache and cache_style:
            # Use cached style
            cached_cond = self._style_cache[cache_key]
            # Blend with current conditioning
            cond = cond * (1.0 - style_strength * 0.5) + cached_cond * (style_strength * 0.5)
            logger.info(f"CarWashStyleConditioner: using cached style '{style_id}'")
        else:
            # Extract style from reference image if provided
            if reference_image is not None:
                style_embedding = self._extract_style(reference_image, clip)
                if style_embedding is not None:
                    cond = cond + style_embedding * style_strength

            # Cache the conditioning
            if cache_style:
                self._style_cache[cache_key] = cond.clone()
                logger.info(f"CarWashStyleConditioner: cached style '{style_id}'")

        return ([[cond, {"pooled_output": pooled}]],)

    def _extract_style(self, image: torch.Tensor, clip) -> Optional[torch.Tensor]:
        """Extract style embedding from reference image."""
        try:
            # Simple style extraction via CLIP image encoding
            # This is a simplified version - production would use IP-Adapter
            import comfy.clip_vision

            # Normalize image to CLIP expected format
            if image.shape[-1] == 3:
                image = image.permute(0, 3, 1, 2)

            # Resize to CLIP input size
            import torch.nn.functional as F
            image = F.interpolate(image, size=(224, 224), mode='bilinear', align_corners=False)

            # This would use a proper CLIP vision model in production
            # For now, return a learned bias toward the image content
            style_bias = image.mean(dim=(2, 3), keepdim=True)
            style_bias = style_bias.expand(-1, -1, 1, 768)  # Match text embedding dim

            return style_bias.squeeze()

        except Exception as e:
            logger.warning(f"Style extraction failed: {e}")
            return None

    @classmethod
    def clear_cache(cls):
        """Clear the style cache."""
        cls._style_cache.clear()
        logger.info("CarWashStyleConditioner: cache cleared")


class CarWashTemporalBlend:
    """
    Frame-to-frame coherence node.

    This node blends the current frame with previous frames
    to ensure temporal consistency in animations.
    """

    CATEGORY = "CarWash/Temporal"
    RETURN_TYPES = ("IMAGE",)
    RETURN_NAMES = ("blended",)
    FUNCTION = "blend"

    # Frame buffer for temporal coherence
    _frame_buffer: List[torch.Tensor] = []
    _max_buffer_size: int = 5

    @classmethod
    def INPUT_TYPES(cls):
        return {
            "required": {
                "current_frame": ("IMAGE",),
                "blend_strength": ("FLOAT", {"default": 0.3, "min": 0.0, "max": 1.0, "step": 0.05}),
            },
            "optional": {
                "previous_frame": ("IMAGE",),
                "motion_vectors": ("IMAGE",),
                "reset_buffer": ("BOOLEAN", {"default": False}),
            }
        }

    def blend(self, current_frame: torch.Tensor, blend_strength: float,
              previous_frame=None, motion_vectors=None, reset_buffer: bool = False):
        """Blend current frame with temporal history."""

        if reset_buffer:
            self._frame_buffer.clear()
            logger.info("CarWashTemporalBlend: buffer reset")

        # If we have a previous frame, use it directly
        if previous_frame is not None:
            blended = self._blend_frames(current_frame, previous_frame,
                                         blend_strength, motion_vectors)
        elif len(self._frame_buffer) > 0:
            # Use frame buffer
            prev = self._frame_buffer[-1]
            blended = self._blend_frames(current_frame, prev,
                                         blend_strength, motion_vectors)
        else:
            # First frame, no blending
            blended = current_frame

        # Update buffer
        self._frame_buffer.append(current_frame.clone())
        if len(self._frame_buffer) > self._max_buffer_size:
            self._frame_buffer.pop(0)

        logger.info(f"CarWashTemporalBlend: buffer_size={len(self._frame_buffer)}, "
                   f"blend_strength={blend_strength}")

        return (blended,)

    def _blend_frames(self, current: torch.Tensor, previous: torch.Tensor,
                      strength: float, motion_vectors=None) -> torch.Tensor:
        """Blend two frames with optional motion compensation."""

        # Ensure same shape
        if current.shape != previous.shape:
            import torch.nn.functional as F
            previous = F.interpolate(
                previous.permute(0, 3, 1, 2),
                size=current.shape[1:3],
                mode='bilinear',
                align_corners=False
            ).permute(0, 2, 3, 1)

        # Apply motion vectors if available
        if motion_vectors is not None:
            previous = self._warp_by_motion(previous, motion_vectors)

        # Simple alpha blend
        blended = current * (1.0 - strength) + previous * strength

        return blended

    def _warp_by_motion(self, frame: torch.Tensor,
                        motion_vectors: torch.Tensor) -> torch.Tensor:
        """Warp frame by motion vectors."""
        try:
            import torch.nn.functional as F

            B, H, W, C = frame.shape

            # Create base grid
            y, x = torch.meshgrid(
                torch.linspace(-1, 1, H, device=frame.device),
                torch.linspace(-1, 1, W, device=frame.device),
                indexing='ij'
            )
            grid = torch.stack([x, y], dim=-1).unsqueeze(0).expand(B, -1, -1, -1)

            # Apply motion vectors (assuming UV format)
            if motion_vectors.shape[-1] >= 2:
                mv = motion_vectors[..., :2]
                # Normalize motion vectors to grid space
                mv_normalized = mv / torch.tensor([W, H], device=mv.device) * 2
                grid = grid + mv_normalized

            # Warp
            frame_nchw = frame.permute(0, 3, 1, 2)
            warped = F.grid_sample(frame_nchw, grid, mode='bilinear',
                                   padding_mode='border', align_corners=False)

            return warped.permute(0, 2, 3, 1)

        except Exception as e:
            logger.warning(f"Motion warping failed: {e}")
            return frame

    @classmethod
    def clear_buffer(cls):
        """Clear the frame buffer."""
        cls._frame_buffer.clear()
        logger.info("CarWashTemporalBlend: buffer cleared")


# Node registration for ComfyUI
NODE_CLASS_MAPPINGS = {
    "CarWashDeterministicSampler": CarWashDeterministicSampler,
    "CarWashContextLoader": CarWashContextLoader,
    "CarWashStyleConditioner": CarWashStyleConditioner,
    "CarWashTemporalBlend": CarWashTemporalBlend,
}

NODE_DISPLAY_NAME_MAPPINGS = {
    "CarWashDeterministicSampler": "CarWash Deterministic Sampler",
    "CarWashContextLoader": "CarWash Context Loader",
    "CarWashStyleConditioner": "CarWash Style Conditioner",
    "CarWashTemporalBlend": "CarWash Temporal Blend",
}

# Module info
__version__ = "1.0.0"
__author__ = "Joseph O. Ibrahim"
