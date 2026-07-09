"""
CarWash Color Management
========================
OCIO integration for production color pipelines.

Handles:
- Linear to sRGB conversion for AI models (SD expects sRGB)
- sRGB to linear conversion for compositing
- ACES support
- EXR bit-depth handling

Author: CarWash Team
Version: 1.0.0
"""

import os
import numpy as np
from pathlib import Path
from typing import Optional, Tuple, Union
from dataclasses import dataclass
from enum import Enum
import struct
import array

# Try to import OpenColorIO
try:
    import PyOpenColorIO as ocio
    OCIO_AVAILABLE = True
except ImportError:
    OCIO_AVAILABLE = False


class ColorSpace(Enum):
    """Standard color spaces."""
    LINEAR = "linear"
    SRGB = "sRGB"
    ACES_CG = "ACEScg"
    ACES_2065 = "ACES2065-1"
    RAW = "Raw"


@dataclass
class ColorConfig:
    """Color management configuration."""
    ocio_config_path: Optional[str] = None
    input_colorspace: ColorSpace = ColorSpace.LINEAR
    output_colorspace: ColorSpace = ColorSpace.SRGB
    ai_working_space: ColorSpace = ColorSpace.SRGB  # SD models trained on sRGB
    bit_depth: int = 8  # 8 or 16 for AI model input
    clamp_negatives: bool = True
    apply_gamma: bool = True


class ColorManager:
    """OCIO-based color management for CarWash pipeline."""

    def __init__(self, config: Optional[ColorConfig] = None):
        self.config = config or ColorConfig()
        self._ocio_config = None
        self._processors = {}

        if OCIO_AVAILABLE:
            self._init_ocio()

    def _init_ocio(self):
        """Initialize OCIO configuration."""
        config_path = self.config.ocio_config_path

        if not config_path:
            # Try common OCIO config locations. Resolve Houdini's bundled
            # config from $HFS (or hou.getenv) rather than a hardcoded
            # install path, so this works across Houdini 21/22+.
            hfs = os.environ.get("HFS")
            if not hfs:
                try:
                    import hou  # available when run inside Houdini
                    hfs = hou.getenv("HFS")
                except Exception:
                    hfs = None
            houdini_ocio = (
                f"{hfs}/houdini/ocio/configs/aces_1.0.3/config.ocio" if hfs else None
            )
            potential_paths = [
                os.environ.get("OCIO"),
                houdini_ocio,
                "C:/ACES/aces_1.2/config.ocio",
            ]
            for path in potential_paths:
                if path and Path(path).exists():
                    config_path = path
                    break

        if config_path and Path(config_path).exists():
            try:
                self._ocio_config = ocio.Config.CreateFromFile(config_path)
                print(f"[CarWash Color] Loaded OCIO: {config_path}")
            except Exception as e:
                print(f"[CarWash Color] OCIO load failed: {e}")
                self._ocio_config = ocio.Config.CreateRaw()
        else:
            # Use built-in sRGB config
            self._ocio_config = ocio.Config.CreateRaw()
            print("[CarWash Color] Using fallback sRGB transforms")

    def _get_processor(self, src: str, dst: str) -> Optional[object]:
        """Get cached OCIO processor for transform."""
        key = f"{src}>{dst}"
        if key not in self._processors:
            if self._ocio_config:
                try:
                    self._processors[key] = self._ocio_config.getProcessor(src, dst)
                except:
                    self._processors[key] = None
            else:
                self._processors[key] = None
        return self._processors[key]

    def linear_to_srgb(self, data: np.ndarray) -> np.ndarray:
        """Convert linear RGB to sRGB."""
        if OCIO_AVAILABLE and self._ocio_config:
            proc = self._get_processor("scene_linear", "sRGB")
            if proc:
                cpu_proc = proc.getDefaultCPUProcessor()
                result = data.copy().astype(np.float32)
                if result.ndim == 3:
                    # Apply per-pixel
                    h, w, c = result.shape
                    flat = result.reshape(-1, c)
                    for i in range(len(flat)):
                        flat[i] = cpu_proc.applyRGB(flat[i][:3].tolist() + [0])[:3]
                    return flat.reshape(h, w, c)
                return result

        # Fallback: manual sRGB gamma
        return self._apply_srgb_gamma(data)

    def srgb_to_linear(self, data: np.ndarray) -> np.ndarray:
        """Convert sRGB to linear RGB."""
        if OCIO_AVAILABLE and self._ocio_config:
            proc = self._get_processor("sRGB", "scene_linear")
            if proc:
                cpu_proc = proc.getDefaultCPUProcessor()
                result = data.copy().astype(np.float32)
                if result.ndim == 3:
                    h, w, c = result.shape
                    flat = result.reshape(-1, c)
                    for i in range(len(flat)):
                        flat[i] = cpu_proc.applyRGB(flat[i][:3].tolist() + [0])[:3]
                    return flat.reshape(h, w, c)
                return result

        # Fallback: inverse sRGB gamma
        return self._remove_srgb_gamma(data)

    def _apply_srgb_gamma(self, linear: np.ndarray) -> np.ndarray:
        """Apply sRGB gamma curve (manual fallback)."""
        linear = np.clip(linear, 0, None) if self.config.clamp_negatives else linear
        srgb = np.where(
            linear <= 0.0031308,
            linear * 12.92,
            1.055 * np.power(np.maximum(linear, 0), 1/2.4) - 0.055
        )
        return np.clip(srgb, 0, 1)

    def _remove_srgb_gamma(self, srgb: np.ndarray) -> np.ndarray:
        """Remove sRGB gamma curve (manual fallback)."""
        linear = np.where(
            srgb <= 0.04045,
            srgb / 12.92,
            np.power((srgb + 0.055) / 1.055, 2.4)
        )
        return linear

    def prepare_for_ai(self, image: np.ndarray, source_colorspace: ColorSpace = ColorSpace.LINEAR) -> np.ndarray:
        """
        Prepare image for AI model input.

        Args:
            image: Input image (float32, any range)
            source_colorspace: Source color space

        Returns:
            Image ready for AI (uint8/16, sRGB, 0-255 range)
        """
        # Ensure float32
        img = image.astype(np.float32)

        # Convert to sRGB if needed
        if source_colorspace == ColorSpace.LINEAR:
            img = self.linear_to_srgb(img)
        elif source_colorspace == ColorSpace.ACES_CG:
            # ACEScg → sRGB (would need proper transform)
            img = self.linear_to_srgb(img * 0.6)  # Rough approximation

        # Clamp and scale
        img = np.clip(img, 0, 1)

        if self.config.bit_depth == 16:
            return (img * 65535).astype(np.uint16)
        else:
            return (img * 255).astype(np.uint8)

    def prepare_from_ai(self, image: np.ndarray, target_colorspace: ColorSpace = ColorSpace.LINEAR) -> np.ndarray:
        """
        Convert AI output back to linear/production color space.

        Args:
            image: AI output (uint8/16, sRGB)
            target_colorspace: Target color space

        Returns:
            Linear float32 image for compositing
        """
        # Normalize to 0-1
        if image.dtype == np.uint16:
            img = image.astype(np.float32) / 65535.0
        else:
            img = image.astype(np.float32) / 255.0

        # Convert from sRGB
        if target_colorspace == ColorSpace.LINEAR:
            img = self.srgb_to_linear(img)
        elif target_colorspace == ColorSpace.ACES_CG:
            img = self.srgb_to_linear(img) / 0.6  # Rough inverse

        return img


class EXRHandler:
    """Minimal EXR reading/writing without OpenEXR dependency."""

    @staticmethod
    def read_exr_simple(path: str) -> Tuple[np.ndarray, dict]:
        """
        Read EXR file (simplified - single part, float32 RGBA).

        Returns:
            (image_data, metadata)
        """
        # This is a simplified reader - production would use OpenEXR
        # For now, try using imageio or fallback to placeholder
        try:
            import imageio
            img = imageio.imread(path)
            return img.astype(np.float32), {"width": img.shape[1], "height": img.shape[0]}
        except:
            pass

        # Fallback: create placeholder
        print(f"[CarWash Color] EXR reading requires imageio or OpenEXR: {path}")
        return np.zeros((512, 512, 4), dtype=np.float32), {"width": 512, "height": 512}

    @staticmethod
    def write_exr_simple(path: str, data: np.ndarray, metadata: Optional[dict] = None):
        """Write EXR file (simplified)."""
        try:
            import imageio
            imageio.imwrite(path, data.astype(np.float32))
            return True
        except:
            print(f"[CarWash Color] EXR writing requires imageio or OpenEXR: {path}")
            return False


class DepthNormalizer:
    """Normalize depth/normal AOVs for ControlNet input."""

    def __init__(self, color_manager: Optional[ColorManager] = None):
        self.color_manager = color_manager or ColorManager()

    def normalize_depth(self, depth: np.ndarray, near: float = 0.1, far: float = 100.0) -> np.ndarray:
        """
        Normalize depth map for ControlNet depth model.

        Args:
            depth: Raw depth values (Z-buffer or linear depth)
            near: Near plane
            far: Far plane

        Returns:
            Normalized depth (uint8, 0=near, 255=far)
        """
        # Handle different depth conventions
        if depth.min() < 0:
            # Might be NDC space
            depth = np.abs(depth)

        # Clip to near/far range
        depth_clipped = np.clip(depth, near, far)

        # Normalize to 0-1
        depth_norm = (depth_clipped - near) / (far - near)

        # Invert so closer = brighter (ControlNet convention)
        depth_norm = 1.0 - depth_norm

        # Convert to 8-bit
        return (depth_norm * 255).astype(np.uint8)

    def normalize_normals(self, normals: np.ndarray) -> np.ndarray:
        """
        Normalize world-space normals for ControlNet normal model.

        Args:
            normals: World-space normals (-1 to 1 range, RGB = XYZ)

        Returns:
            Normalized normals (uint8, 0-255 range)
        """
        # Ensure proper range
        normals = np.clip(normals, -1, 1)

        # Convert from -1,1 to 0,1
        normals_norm = (normals + 1) / 2

        # Flip Y if needed (OpenGL vs DirectX convention)
        # ControlNet expects Y-up in standard form

        # Convert to 8-bit
        return (normals_norm * 255).astype(np.uint8)

    def prepare_controlnet_depth(self, exr_path: str, output_path: str,
                                  near: float = 0.1, far: float = 100.0) -> bool:
        """
        Prepare depth EXR for ControlNet consumption.

        Args:
            exr_path: Input depth EXR from Houdini
            output_path: Output PNG for ControlNet
            near: Near plane
            far: Far plane

        Returns:
            Success status
        """
        try:
            # Read EXR
            depth_data, _ = EXRHandler.read_exr_simple(exr_path)

            # Extract depth channel (might be in R channel or separate)
            if depth_data.ndim == 3:
                depth = depth_data[:, :, 0]  # Assume R channel
            else:
                depth = depth_data

            # Normalize
            depth_norm = self.normalize_depth(depth, near, far)

            # Save as PNG
            from PIL import Image
            img = Image.fromarray(depth_norm, mode='L')
            img.save(output_path)

            return True
        except Exception as e:
            print(f"[CarWash Color] Depth preparation failed: {e}")
            return False

    def prepare_controlnet_normal(self, exr_path: str, output_path: str) -> bool:
        """
        Prepare normal EXR for ControlNet consumption.

        Args:
            exr_path: Input normal EXR from Houdini
            output_path: Output PNG for ControlNet

        Returns:
            Success status
        """
        try:
            # Read EXR
            normal_data, _ = EXRHandler.read_exr_simple(exr_path)

            # Extract RGB (XYZ normals)
            if normal_data.shape[-1] >= 3:
                normals = normal_data[:, :, :3]
            else:
                normals = normal_data

            # Normalize
            normals_norm = self.normalize_normals(normals)

            # Save as PNG
            from PIL import Image
            img = Image.fromarray(normals_norm, mode='RGB')
            img.save(output_path)

            return True
        except Exception as e:
            print(f"[CarWash Color] Normal preparation failed: {e}")
            return False


# =============================================================================
# CONVENIENCE FUNCTIONS
# =============================================================================

def setup_production_color(ocio_config: Optional[str] = None) -> ColorManager:
    """Set up color management for production use."""
    config = ColorConfig(ocio_config_path=ocio_config)
    return ColorManager(config)


def prepare_aovs_for_controlnet(depth_exr: str, normal_exr: str,
                                  output_dir: str) -> Tuple[Optional[str], Optional[str]]:
    """
    Prepare Houdini AOVs for ControlNet consumption.

    Args:
        depth_exr: Path to depth EXR
        normal_exr: Path to normal EXR
        output_dir: Output directory for PNGs

    Returns:
        (depth_png_path, normal_png_path) or (None, None) on failure
    """
    output_dir = Path(output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)

    normalizer = DepthNormalizer()

    depth_out = None
    normal_out = None

    if depth_exr and Path(depth_exr).exists():
        depth_out = str(output_dir / "depth_controlnet.png")
        if not normalizer.prepare_controlnet_depth(depth_exr, depth_out):
            depth_out = None

    if normal_exr and Path(normal_exr).exists():
        normal_out = str(output_dir / "normal_controlnet.png")
        if not normalizer.prepare_controlnet_normal(normal_exr, normal_out):
            normal_out = None

    return depth_out, normal_out


# =============================================================================
# TESTS
# =============================================================================

def _test_color_transforms():
    """Test color space conversions."""
    cm = ColorManager()

    # Test linear to sRGB roundtrip
    linear = np.array([[[0.0, 0.5, 1.0]]], dtype=np.float32)
    srgb = cm.linear_to_srgb(linear)
    back = cm.srgb_to_linear(srgb)

    print(f"Linear: {linear[0,0]}")
    print(f"sRGB:   {srgb[0,0]}")
    print(f"Back:   {back[0,0]}")
    print(f"Error:  {np.abs(linear - back).max():.6f}")

    assert np.allclose(linear, back, atol=1e-4), "Roundtrip failed"
    print("[PASS] Color transform roundtrip")


if __name__ == "__main__":
    _test_color_transforms()
