"""
HdCarWash Multi-Version Builder
Builds and deploys for all installed Houdini 21.x versions.

Usage:
    python build_all_versions.py [--build-only] [--deploy-only]
"""

import os
import sys
import subprocess
import shutil
from pathlib import Path

# Configuration
PROJECT_ROOT = Path(__file__).parent
BUILD_ROOT = PROJECT_ROOT / "build_versions"
HOUDINI_BASE = Path(r"C:\Program Files\Side Effects Software")
TARGET_DIR = Path(os.path.expandvars(r"%USERPROFILE%\houdini21.0\dso\usd\hdCarWash"))

# Find all Houdini 21.x installations
def find_houdini_versions():
    versions = []
    if HOUDINI_BASE.exists():
        for item in HOUDINI_BASE.iterdir():
            if item.is_dir() and item.name.startswith("Houdini 21.0."):
                version = item.name.replace("Houdini ", "")
                versions.append((version, item))
    return sorted(versions, key=lambda x: x[0])

def check_houdini_running():
    """Check if any Houdini process is running."""
    try:
        result = subprocess.run(
            ["tasklist", "/FI", "IMAGENAME eq houdini.exe"],
            capture_output=True, text=True, shell=True
        )
        return "houdini.exe" in result.stdout.lower()
    except:
        return False

def build_version(version, hfs_path):
    """Build for a specific Houdini version."""
    print(f"\n{'='*60}")
    print(f"Building for Houdini {version}")
    print(f"{'='*60}")

    build_dir = BUILD_ROOT / version
    build_dir.mkdir(parents=True, exist_ok=True)

    # Configure
    cmake_prefix = hfs_path / "toolkit" / "cmake"
    env = os.environ.copy()
    env["HFS"] = str(hfs_path)

    config_cmd = [
        "cmake",
        str(PROJECT_ROOT),
        "-G", "Visual Studio 17 2022",
        "-A", "x64",
        f"-DCMAKE_PREFIX_PATH={cmake_prefix}"
    ]

    print(f"Configuring...")
    result = subprocess.run(config_cmd, cwd=build_dir, env=env, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"[ERROR] Configure failed: {result.stderr}")
        return None

    # Build
    build_cmd = ["cmake", "--build", ".", "--config", "Release"]
    print(f"Building...")
    result = subprocess.run(build_cmd, cwd=build_dir, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"[ERROR] Build failed: {result.stderr}")
        return None

    dll_path = build_dir / "plugin" / "hdCarWash" / "Release" / "hdCarWash.dll"
    if dll_path.exists():
        print(f"[SUCCESS] Built: {dll_path}")
        return dll_path
    else:
        print(f"[ERROR] DLL not found at {dll_path}")
        return None

def deploy_all(dll_paths):
    """Deploy all version DLLs."""
    print(f"\n{'='*60}")
    print("Deploying")
    print(f"{'='*60}")

    if check_houdini_running():
        print("[ERROR] Houdini is running! Close it first.")
        return False

    # Create target directories
    lib_dir = TARGET_DIR / "lib"
    res_dir = TARGET_DIR / "resources"
    lib_dir.mkdir(parents=True, exist_ok=True)
    res_dir.mkdir(parents=True, exist_ok=True)

    # Copy version-specific DLLs
    for version, dll_path in dll_paths.items():
        if dll_path and dll_path.exists():
            # Copy as version-specific name
            versioned_name = f"hdCarWash_{version.replace('.', '_')}.dll"
            dest = lib_dir / versioned_name
            shutil.copy2(dll_path, dest)
            print(f"[SUCCESS] Copied: {versioned_name}")

            # Also copy as main DLL (use latest version)
            main_dll = lib_dir / "hdCarWash.dll"
            shutil.copy2(dll_path, main_dll)

    # Copy plugInfo.json
    pluginfo_src = PROJECT_ROOT / "plugin" / "plugInfo.json"
    if pluginfo_src.exists():
        shutil.copy2(pluginfo_src, res_dir / "plugInfo.json")
        print("[SUCCESS] Copied: plugInfo.json")
    else:
        # Create simplified plugInfo.json
        pluginfo = '''{
    "Plugins": [
        {
            "Name": "hdCarWash",
            "Type": "library",
            "Root": "..",
            "LibraryPath": "lib/hdCarWash.dll",
            "ResourcePath": "resources",
            "Info": {
                "Types": {
                    "HdCarWashRendererPlugin": {
                        "bases": ["HdRendererPlugin"],
                        "displayName": "CarWash Renderer"
                    }
                }
            }
        }
    ]
}'''
        (res_dir / "plugInfo.json").write_text(pluginfo)
        print("[SUCCESS] Created: plugInfo.json")

    print(f"\n[SUCCESS] Deployed to: {TARGET_DIR}")
    return True

def main():
    print("="*60)
    print("HdCarWash Multi-Version Builder")
    print("="*60)

    build_only = "--build-only" in sys.argv
    deploy_only = "--deploy-only" in sys.argv

    versions = find_houdini_versions()
    print(f"\nFound {len(versions)} Houdini 21.x installations:")
    for v, p in versions:
        print(f"  - {v}: {p}")

    dll_paths = {}

    if not deploy_only:
        # Build for each version
        for version, hfs_path in versions:
            dll = build_version(version, hfs_path)
            dll_paths[version] = dll
    else:
        # Find existing builds
        for version, _ in versions:
            dll = BUILD_ROOT / version / "plugin" / "hdCarWash" / "Release" / "hdCarWash.dll"
            if dll.exists():
                dll_paths[version] = dll

    if not build_only:
        # Deploy
        deploy_all(dll_paths)

    print("\n" + "="*60)
    print("Done!")
    print("="*60)

if __name__ == "__main__":
    main()
