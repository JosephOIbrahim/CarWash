#!/usr/bin/env python
"""
HdCarWash Automation Suite v0.1
===============================

Automates testing and debugging of the HdCarWash render delegate.
Analyzes debug logs and provides diagnostic information.

Usage:
  python carwash_automation.py [command]

Commands:
  diagnose    - Run full diagnostic suite
  logs        - Read and analyze debug logs
  clear       - Clear debug log
"""

import os
import re
import sys
from pathlib import Path
from datetime import datetime

# Configuration
DEBUG_LOG_PATH = r"C:\Temp\hdcarwash_debug.txt"
CARWASH_DLL_PATH = os.path.expanduser(r"~\houdini21.0\dso\usd\hdCarWash\lib\hdCarWash.dll")


class HdCarWashAutomation:
    """Automation controller for HdCarWash testing."""

    def check_prerequisites(self):
        """Check if CarWash plugin is installed."""
        print("\n=== Checking Prerequisites ===")

        # Check DLL exists
        if os.path.exists(CARWASH_DLL_PATH):
            print(f"[OK] HdCarWash DLL found at: {CARWASH_DLL_PATH}")
            dll_size = os.path.getsize(CARWASH_DLL_PATH)
            print(f"     Size: {dll_size / 1024:.1f} KB")
        else:
            print(f"[ERROR] HdCarWash DLL not found at: {CARWASH_DLL_PATH}")
            return False

        # Check debug log directory
        debug_dir = os.path.dirname(DEBUG_LOG_PATH)
        if os.path.exists(debug_dir):
            print(f"[OK] Debug log directory exists: {debug_dir}")
        else:
            os.makedirs(debug_dir, exist_ok=True)
            print(f"[OK] Created debug log directory: {debug_dir}")

        return True

    def read_debug_log(self):
        """Read and analyze the debug log."""
        print("\n=== Reading Debug Log ===")

        if not os.path.exists(DEBUG_LOG_PATH):
            print(f"[WARNING] Debug log not found: {DEBUG_LOG_PATH}")
            print("          Run a render with CarWash renderer to generate log")
            return None

        with open(DEBUG_LOG_PATH, 'r') as f:
            content = f.read()

        lines = content.strip().split('\n')
        print(f"[INFO] Debug log has {len(lines)} lines")

        # Parse key metrics
        frames = 0
        total_rprims = 0
        total_meshes = 0
        aov_count = 0
        framebuffer_sizes = []

        for line in lines:
            if "Phase 1 Execute Frame" in line:
                frames += 1
            if "Rprim paths count:" in line:
                match = re.search(r'count:\s*(\d+)', line)
                if match:
                    count = int(match.group(1))
                    total_rprims += count
            if "Meshes rasterized:" in line:
                match = re.search(r'rasterized:\s*(\d+)', line)
                if match:
                    count = int(match.group(1))
                    total_meshes += count
            if "AOV bindings count:" in line:
                match = re.search(r'count:\s*(\d+)', line)
                if match:
                    aov_count = int(match.group(1))
            if "Framebuffer:" in line:
                match = re.search(r'Framebuffer:\s*(\d+)x(\d+)', line)
                if match:
                    framebuffer_sizes.append((int(match.group(1)), int(match.group(2))))

        print(f"[INFO] Frames rendered: {frames}")
        print(f"[INFO] Total Rprims seen: {total_rprims}")
        print(f"[INFO] Total meshes rasterized: {total_meshes}")
        print(f"[INFO] AOV bindings per frame: {aov_count}")

        if framebuffer_sizes:
            sizes = set(framebuffer_sizes)
            for w, h in sizes:
                print(f"[INFO] Framebuffer size: {w}x{h}")

        if total_rprims == 0:
            print("\n[ANALYSIS] ISSUE IDENTIFIED:")
            print("           Rprim paths count is consistently 0")
            print("\n           Possible causes:")
            print("           1. sopimport creates USD prims but not 'Mesh' type")
            print("           2. Hydra not calling CreateRprim for geometry")
            print("           3. Render collection filtering out geometry")
            print("           4. Scene delegate not syncing mesh data")
            print("\n           Diagnostic steps:")
            print("           1. Open Scene Graph Tree in Houdini")
            print("           2. Look for prim type - should be 'Mesh'")
            print("           3. Check if geometry visible with other renderers")
            print("           4. Add more logging to CreateRprim in renderDelegate.cpp")

        return {
            "frames": frames,
            "total_rprims": total_rprims,
            "total_meshes": total_meshes,
            "aov_count": aov_count,
            "framebuffer_sizes": framebuffer_sizes
        }

    def clear_debug_log(self):
        """Clear the debug log for fresh testing."""
        if os.path.exists(DEBUG_LOG_PATH):
            os.remove(DEBUG_LOG_PATH)
            print(f"[OK] Debug log cleared: {DEBUG_LOG_PATH}")
        else:
            print("[INFO] No debug log to clear")

    def run_full_diagnostic(self):
        """Run the complete diagnostic sequence."""
        print("=" * 70)
        print("HdCarWash Automation Suite v0.1")
        print("=" * 70)
        print(f"Timestamp: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")

        # Check prerequisites
        if not self.check_prerequisites():
            print("\n[ABORT] Prerequisites check failed")
            return False

        # Read and analyze log
        log_analysis = self.read_debug_log()

        # Summary
        print("\n" + "=" * 70)
        print("DIAGNOSTIC SUMMARY")
        print("=" * 70)

        if log_analysis:
            if log_analysis['total_rprims'] > 0:
                print("[SUCCESS] HdCarWash is receiving mesh data")
                if log_analysis['total_meshes'] > 0:
                    print("[SUCCESS] Meshes are being rasterized")
                else:
                    print("[WARNING] Rprims exist but meshes not rasterized")
            else:
                print("[ISSUE] HdCarWash is NOT receiving mesh data")
                self.show_fix_recommendations()

        return True

    def show_fix_recommendations(self):
        """Show recommendations for fixing the 0 rprim issue."""
        print("\n" + "=" * 70)
        print("RECOMMENDED FIXES")
        print("=" * 70)
        print("""
The issue is that Houdini's Hydra scene delegate isn't registering
mesh prims with HdCarWash. This is a common issue with USD stages.

SOLUTION 1: Check USD Prim Types
--------------------------------
In Houdini Scene Graph Tree, verify the prim has type="Mesh".
If it shows as "Xform" or "Scope", the geometry isn't properly
converted to USD mesh format.

SOLUTION 2: Use Configure Primitive LOP
----------------------------------------
Add a "configureprimitive" LOP after sopimport to force type:

    configure_prim = stage.createNode("configureprimitive")
    configure_prim.parm("primpath").set("/test_geo/sphere1")
    configure_prim.parm("createprimtype").set("Mesh")

SOLUTION 3: Debug CreateRprim
-----------------------------
Add logging to renderDelegate.cpp CreateRprim() to see what
types Hydra is requesting:

    TF_DEBUG_MSG(HD_CARWASH, "CreateRprim type: %s path: %s\\n",
                 typeId.GetText(), rprimId.GetText());

SOLUTION 4: Check GetSupportedRprimTypes
----------------------------------------
Verify HdCarWash reports "mesh" in GetSupportedRprimTypes().
Currently it does - see renderDelegate.cpp line 26.

SOLUTION 5: Inspect with usdview
--------------------------------
Export the USD stage and open with usdview to verify structure:

    hou.node("/stage").displayNode().stage().Export("test.usda")
""")


def main():
    import argparse

    parser = argparse.ArgumentParser(description="HdCarWash Automation Suite")
    parser.add_argument('command', nargs='?', default='diagnose',
                        choices=['diagnose', 'logs', 'clear'],
                        help='Command to run')
    args = parser.parse_args()

    automation = HdCarWashAutomation()

    if args.command == 'logs':
        automation.read_debug_log()
    elif args.command == 'clear':
        automation.clear_debug_log()
    else:  # 'diagnose'
        automation.run_full_diagnostic()


if __name__ == "__main__":
    main()
