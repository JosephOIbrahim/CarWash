#!/usr/bin/env python
"""HdCarWash Automation Suite v0.1.

Automates testing and debugging of the HdCarWash render delegate. Reads the
plugin debug log and reports diagnostic information.

Usage:
    python carwash_automation.py [diagnose|logs|clear]

Configuration is layered (env vars beat config file beat defaults). See
``automation/synapse/config.py`` and ``automation/synapse/paths.py``.
"""

from __future__ import annotations

import argparse
import logging
import re
import sys
from datetime import datetime
from pathlib import Path
from typing import NamedTuple

sys.path.insert(0, str(Path(__file__).resolve().parent))

from synapse.paths import carwash_dll_path, debug_log_path

log = logging.getLogger("carwash.automation")


class LogStats(NamedTuple):
    frames: int
    total_rprims: int
    total_meshes: int
    aov_count: int
    framebuffer_sizes: list[tuple[int, int]]


class HdCarWashAutomation:
    """Automation controller for HdCarWash testing."""

    def __init__(
        self,
        debug_log: Path | None = None,
        dll_path: Path | None = None,
    ) -> None:
        self.debug_log = debug_log or debug_log_path()
        self.dll_path = dll_path or carwash_dll_path()

    def check_prerequisites(self) -> bool:
        """Check that the plugin is installed and the log directory is writable."""
        print("\n=== Checking Prerequisites ===")

        if self.dll_path.exists():
            print(f"[OK] HdCarWash DLL found at: {self.dll_path}")
            print(f"     Size: {self.dll_path.stat().st_size / 1024:.1f} KB")
        else:
            print(f"[ERROR] HdCarWash DLL not found at: {self.dll_path}")
            return False

        debug_dir = self.debug_log.parent
        debug_dir.mkdir(parents=True, exist_ok=True)
        print(f"[OK] Debug log directory ready: {debug_dir}")
        return True

    def read_debug_log(self) -> LogStats | None:
        """Read and analyse the debug log."""
        print("\n=== Reading Debug Log ===")

        if not self.debug_log.exists():
            print(f"[WARNING] Debug log not found: {self.debug_log}")
            print("          Run a render with CarWash renderer to generate log")
            return None

        with self.debug_log.open(encoding="utf-8", errors="replace") as f:
            content = f.read()

        lines = content.strip().split("\n")
        print(f"[INFO] Debug log has {len(lines)} lines")

        frames = 0
        total_rprims = 0
        total_meshes = 0
        aov_count = 0
        framebuffer_sizes: list[tuple[int, int]] = []

        for line in lines:
            if "Phase 1 Execute Frame" in line:
                frames += 1
            if (match := re.search(r"Rprim paths count:\s*(\d+)", line)) is not None:
                total_rprims += int(match.group(1))
            if (match := re.search(r"Meshes rasterized:\s*(\d+)", line)) is not None:
                total_meshes += int(match.group(1))
            if (match := re.search(r"AOV bindings count:\s*(\d+)", line)) is not None:
                aov_count = int(match.group(1))
            if (match := re.search(r"Framebuffer:\s*(\d+)x(\d+)", line)) is not None:
                framebuffer_sizes.append((int(match.group(1)), int(match.group(2))))

        print(f"[INFO] Frames rendered: {frames}")
        print(f"[INFO] Total Rprims seen: {total_rprims}")
        print(f"[INFO] Total meshes rasterized: {total_meshes}")
        print(f"[INFO] AOV bindings per frame: {aov_count}")

        for w, h in set(framebuffer_sizes):
            print(f"[INFO] Framebuffer size: {w}x{h}")

        if total_rprims == 0:
            print("\n[ANALYSIS] ISSUE IDENTIFIED:")
            print("           Rprim paths count is consistently 0")
            print("\n           Possible causes:")
            print("           1. sopimport creates USD prims but not 'Mesh' type")
            print("           2. Hydra not calling CreateRprim for geometry")
            print("           3. Render collection filtering out geometry")
            print("           4. Scene delegate not syncing mesh data")

        return LogStats(
            frames=frames,
            total_rprims=total_rprims,
            total_meshes=total_meshes,
            aov_count=aov_count,
            framebuffer_sizes=framebuffer_sizes,
        )

    def clear_debug_log(self) -> None:
        """Delete the debug log."""
        if self.debug_log.exists():
            self.debug_log.unlink()
            print(f"[OK] Debug log cleared: {self.debug_log}")
        else:
            print("[INFO] No debug log to clear")

    def run_full_diagnostic(self) -> bool:
        """Run the complete diagnostic sequence."""
        print("=" * 70)
        print("HdCarWash Automation Suite v0.1")
        print("=" * 70)
        print(f"Timestamp: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")

        if not self.check_prerequisites():
            print("\n[ABORT] Prerequisites check failed")
            return False

        stats = self.read_debug_log()

        print("\n" + "=" * 70)
        print("DIAGNOSTIC SUMMARY")
        print("=" * 70)

        if stats is not None:
            if stats.total_rprims > 0:
                print("[SUCCESS] HdCarWash is receiving mesh data")
                if stats.total_meshes > 0:
                    print("[SUCCESS] Meshes are being rasterized")
                else:
                    print("[WARNING] Rprims exist but meshes not rasterized")
            else:
                print("[ISSUE] HdCarWash is NOT receiving mesh data")
                self.show_fix_recommendations()

        return True

    def show_fix_recommendations(self) -> None:
        """Print recommendations for the 0-rprim issue."""
        print("\n" + "=" * 70)
        print("RECOMMENDED FIXES")
        print("=" * 70)
        print(
            "\nThe issue is that Houdini's Hydra scene delegate isn't registering"
            "\nmesh prims with HdCarWash. This is a common issue with USD stages."
            "\n\nSOLUTION 1: Check USD Prim Types"
            "\n--------------------------------"
            "\nIn Houdini Scene Graph Tree, verify the prim has type='Mesh'."
            "\nIf it shows as 'Xform' or 'Scope', the geometry isn't properly"
            "\nconverted to USD mesh format."
            "\n\nSOLUTION 2: Use Configure Primitive LOP"
            "\n----------------------------------------"
            "\nAdd a 'configureprimitive' LOP after sopimport to force type:"
            "\n    configure_prim = stage.createNode('configureprimitive')"
            "\n    configure_prim.parm('primpath').set('/test_geo/sphere1')"
            "\n    configure_prim.parm('createprimtype').set('Mesh')"
            "\n\nSOLUTION 3: Debug CreateRprim"
            "\n-----------------------------"
            "\nAdd logging to renderDelegate.cpp CreateRprim() to see what"
            "\ntypes Hydra is requesting."
            "\n\nSOLUTION 4: Check GetSupportedRprimTypes"
            "\n----------------------------------------"
            "\nVerify HdCarWash reports 'mesh' in GetSupportedRprimTypes()."
            "\n\nSOLUTION 5: Inspect with usdview"
            "\n--------------------------------"
            "\nExport the USD stage and open with usdview to verify structure."
        )


def main() -> int:
    parser = argparse.ArgumentParser(description="HdCarWash Automation Suite")
    parser.add_argument(
        "command",
        nargs="?",
        default="diagnose",
        choices=["diagnose", "logs", "clear"],
        help="Command to run",
    )
    args = parser.parse_args()

    logging.basicConfig(level=logging.INFO, format="%(levelname)s %(name)s: %(message)s")

    automation = HdCarWashAutomation()

    if args.command == "logs":
        automation.read_debug_log()
    elif args.command == "clear":
        automation.clear_debug_log()
    else:
        automation.run_full_diagnostic()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
