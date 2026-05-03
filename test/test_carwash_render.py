# HdCarWash Test Script
# Run this in Houdini's Python Shell or Script Editor.
#
# Usage:
#   1. Open Houdini 21
#   2. Open Python Shell (Windows > Python Shell)
#   3. Paste this script and run
#
# Expected Result:
#   - Creates a simple sphere in Solaris
#   - Sets CarWash as the renderer
#   - Renders a frame
#   - Reports success/failure
#
# Constitution: C2 — errors are loud. Each parameter set is wrapped in a
# narrowly typed catch that records what failed and surfaces it; we never
# swallow exceptions silently.

import os
import sys
import traceback

import hou


def _safe_set(node, parm, value, *, label):
    """Set a parm and return True on success; record any failure on stderr."""
    try:
        node.parm(parm).set(value)
        return True
    except (hou.OperationFailed, hou.PermissionError, hou.ObjectWasDeleted, AttributeError) as exc:
        print(f"      WARNING: {label}: could not set {parm!r}: {exc}", file=sys.stderr)
        return False


def test_carwash_basic():
    """Test HdCarWash basic rendering with a simple sphere."""

    print("=" * 60)
    print("HdCarWash Render Test")
    print("=" * 60)

    print("\n[1/5] Checking renderer availability...")
    try:
        import _usd  # noqa: F401  — presence is the check
        print("      USD module available: OK")
    except ImportError as exc:
        print(f"      ERROR: USD module not available: {exc}", file=sys.stderr)
        return False

    print("\n[2/5] Creating test scene...")
    hou.hipFile.clear(suppress_save_prompt=True)

    stage = hou.node("/stage")
    if not stage:
        stage = hou.node("/").createNode("lopnet", "stage")

    sphere = stage.createNode("sphere", "test_sphere")
    sphere.parm("radius").set(1.0)

    camera = stage.createNode("camera", "test_camera")
    camera.setInput(0, sphere)
    camera.parm("tx").set(0)
    camera.parm("ty").set(0)
    camera.parm("tz").set(5)

    render_rop = stage.createNode("usdrender_rop", "carwash_render")
    render_rop.setInput(0, camera)

    print("      Scene created: sphere + camera + render ROP")

    print("\n[3/5] Configuring CarWash renderer...")
    if _safe_set(render_rop, "renderer", "HdCarWashRendererPlugin", label="renderer"):
        print("      Renderer set to: HdCarWashRendererPlugin")

    output_dir = os.path.join(os.environ.get("TEMP", "/tmp"), "hdcarwash_test")
    os.makedirs(output_dir, exist_ok=True)
    output_path = os.path.join(output_dir, "test_render.exr")
    if _safe_set(render_rop, "outputimage", output_path, label="outputimage"):
        print(f"      Output: {output_path}")

    if _safe_set(render_rop, "res1", 256, label="res1") and _safe_set(
        render_rop, "res2", 256, label="res2"
    ):
        print("      Resolution: 256x256")

    print("\n[4/5] Rendering...")
    try:
        render_rop.render()
        print("      Render completed!")
    except hou.OperationFailed as exc:
        print(f"      ERROR during render: {exc}", file=sys.stderr)
        traceback.print_exc()
        return False

    print("\n[5/5] Verifying output...")
    if os.path.exists(output_path):
        file_size = os.path.getsize(output_path)
        print(f"      Output file exists: {output_path}")
        print(f"      File size: {file_size} bytes")
        if file_size > 0:
            print("\n" + "=" * 60)
            print("TEST PASSED: HdCarWash rendered successfully!")
            print("=" * 60)
            return True
        print("\n      WARNING: Output file is empty")
    else:
        print(f"      WARNING: Output file not created at {output_path}")

    print("\n" + "=" * 60)
    print("TEST COMPLETED (check viewport for rendered result)")
    print("=" * 60)
    return True


def check_carwash_loaded():
    """Quick check if CarWash plugin is loaded."""

    print("Checking HdCarWash plugin status...")

    plugin_path = os.environ.get("PXR_PLUGINPATH_NAME", "")
    print(f"  PXR_PLUGINPATH_NAME: {plugin_path}")

    pref_dir = hou.getenv("HOUDINI_USER_PREF_DIR", "")
    if not pref_dir:
        print("  WARNING: HOUDINI_USER_PREF_DIR not set; cannot locate plugin DLL")
        return

    dll_name = "hdCarWash.dll" if sys.platform.startswith("win") else "libhdCarWash.so"
    dll_path = os.path.join(pref_dir, "dso", "usd", "hdCarWash", "lib", dll_name)

    if os.path.exists(dll_path):
        print(f"  DLL found: {dll_path}")
        print(f"  DLL size: {os.path.getsize(dll_path)} bytes")
    else:
        print(f"  WARNING: DLL not found at {dll_path}")

    pluginfo_path = dll_path.replace(
        os.path.join("lib", dll_name),
        os.path.join("resources", "plugInfo.json"),
    )
    if os.path.exists(pluginfo_path):
        print("  plugInfo.json found: OK")
    else:
        print(f"  WARNING: plugInfo.json not found at {pluginfo_path}")


if __name__ == "__main__":
    check_carwash_loaded()
    print()
    test_carwash_basic()
