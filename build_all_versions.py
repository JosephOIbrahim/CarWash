"""
HdCarWash Multi-Version Builder
Builds and deploys for all installed Houdini versions.

The user-pref deploy directory is derived per discovered install using the
`houdini<major>.<minor>` convention (e.g. `houdini21.0`, `houdini22.0`), so the
same script builds for 21.0 today and 22.0 when it ships.

Usage:
    python build_all_versions.py [--build-only] [--deploy-only] [--major 21]
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


def houdini_user_dir(version: str) -> Path:
    """Houdini user-pref dir follows the `houdini<major>.<minor>` convention."""
    parts = version.split(".")
    major, minor = parts[0], parts[1] if len(parts) > 1 else "0"
    return Path(os.path.expandvars(r"%USERPROFILE%")) / f"houdini{major}.{minor}"


def deploy_target_dir(version: str) -> Path:
    """Per-version deploy directory under the derived user-pref dir."""
    return houdini_user_dir(version) / "dso" / "usd" / "hdCarWash"


# Find all Houdini installations (any major), optionally filtered by --major.
def find_houdini_versions(major_filter=None):
    versions = []
    if HOUDINI_BASE.exists():
        for item in HOUDINI_BASE.iterdir():
            if not item.is_dir() or not item.name.startswith("Houdini "):
                continue
            version = item.name.replace("Houdini ", "")
            # Skip non-version siblings like "Houdini Server".
            parts = version.split(".")
            if not (len(parts) == 3 and all(p.isdigit() for p in parts)):
                continue
            if major_filter is not None and not version.startswith(f"{major_filter}."):
                continue
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
    """Deploy each version's DLL into that version's user-pref directory."""
    print(f"\n{'='*60}")
    print("Deploying")
    print(f"{'='*60}")

    if check_houdini_running():
        print("[ERROR] Houdini is running! Close it first.")
        return False

    pluginfo_src = PROJECT_ROOT / "plugin" / "plugInfo.json"

    for version, dll_path in dll_paths.items():
        if not (dll_path and dll_path.exists()):
            print(f"[SKIP] No DLL for Houdini {version}")
            continue

        target_dir = deploy_target_dir(version)
        lib_dir = target_dir / "lib"
        lib_dir.mkdir(parents=True, exist_ok=True)

        # Version-specific copy + the main DLL name the package loads.
        versioned_name = f"hdCarWash_{version.replace('.', '_')}.dll"
        shutil.copy2(dll_path, lib_dir / versioned_name)
        shutil.copy2(dll_path, lib_dir / "hdCarWash.dll")
        print(f"[SUCCESS] {version}: copied {versioned_name} + hdCarWash.dll -> {lib_dir}")

        # plugInfo.json sits at the plugin ROOT (matches deploy_hdcarwash.py layout).
        if pluginfo_src.exists():
            shutil.copy2(pluginfo_src, target_dir / "plugInfo.json")
        else:
            print(f"[WARNING] {version}: plugin/plugInfo.json missing")

    print(f"\n[SUCCESS] Deployed per-version user-pref directories")
    return True

def main():
    print("="*60)
    print("HdCarWash Multi-Version Builder")
    print("="*60)

    build_only = "--build-only" in sys.argv
    deploy_only = "--deploy-only" in sys.argv
    major_filter = None
    if "--major" in sys.argv:
        idx = sys.argv.index("--major")
        if idx + 1 < len(sys.argv):
            major_filter = sys.argv[idx + 1]

    versions = find_houdini_versions(major_filter)
    print(f"\nFound {len(versions)} Houdini installations"
          + (f" (major {major_filter})" if major_filter else "")
          + ":")
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
