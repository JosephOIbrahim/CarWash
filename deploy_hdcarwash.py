#!/usr/bin/env python3
"""
HdCarWash Automated Deployment System
======================================
Builds and deploys the HdCarWash Hydra delegate to Houdini.

The Houdini install and user-pref directory are derived from the build
target, not hardcoded — so the same script deploys to 21.0 today and 22.0
when it ships. The user-pref dir follows Houdini's `houdini<major>.<minor>`
convention (e.g. `houdini21.0`, `houdini22.0`).

Usage:
    python deploy_hdcarwash.py                                  # Full build and deploy (auto-detect)
    python deploy_hdcarwash.py --houdini-version 21.0.729      # Pin a specific install
    python deploy_hdcarwash.py --skip-build                    # Deploy only (skip cmake build)
    python deploy_hdcarwash.py --verbose                       # Verbose output
"""

import argparse
import datetime
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Optional


HOUDINI_INSTALL_BASE = Path(r"C:\Program Files\Side Effects Software")


def _is_version_install(dir_name: str) -> bool:
    """True for version-named installs like 'Houdini 21.0.729', False for
    non-version siblings like 'Houdini Server' or 'Houdini License Server'."""
    if not dir_name.startswith("Houdini "):
        return False
    rest = dir_name[len("Houdini "):]
    parts = rest.split(".")
    return len(parts) == 3 and all(p.isdigit() for p in parts)


def find_houdini_install(version: Optional[str] = None) -> Path:
    """Locate the Houdini install directory.

    If `version` is given (e.g. "21.0.729"), use that exact install. Otherwise
    auto-detect the newest installed Houdini under the Side Effects directory.
    """
    if version:
        path = HOUDINI_INSTALL_BASE / f"Houdini {version}"
        if not path.exists():
            raise FileNotFoundError(
                f"Houdini {version} not found at {path}. "
                f"Pass --houdini-version with an installed version."
            )
        return path

    if not HOUDINI_INSTALL_BASE.exists():
        raise FileNotFoundError(
            f"No Houdini install found under {HOUDINI_INSTALL_BASE}. "
            f"Pass --houdini-version explicitly."
        )

    installs = sorted(
        (p for p in HOUDINI_INSTALL_BASE.iterdir()
         if p.is_dir() and _is_version_install(p.name)),
        key=lambda p: p.name,
        reverse=True,  # newest first
    )
    if not installs:
        raise FileNotFoundError(
            f"No Houdini installs found under {HOUDINI_INSTALL_BASE}."
        )
    return installs[0]


def houdini_user_dir(version: str) -> Path:
    """Houdini user-pref dir follows the `houdini<major>.<minor>` convention."""
    parts = version.split(".")
    major, minor = parts[0], parts[1] if len(parts) > 1 else "0"
    return Path.home() / f"houdini{major}.{minor}"


class Config:
    """Centralized deployment configuration."""

    def __init__(self, version: Optional[str] = None):
        """Resolve install + user-pref paths from the Houdini version.

        `version` is the full version string (e.g. "21.0.729"). When None,
        auto-detect the newest installed Houdini.
        """
        self.install_dir = find_houdini_install(version)
        # Derive the version string from the install dir name ("Houdini 21.0.729").
        self.version = self.install_dir.name.replace("Houdini ", "")

        # Source paths (relative to project root)
        self.PROJECT_ROOT = Path(__file__).parent.resolve()
        self.BUILD_DIR = self.PROJECT_ROOT / "build"
        self.PLUGIN_DIR = self.PROJECT_ROOT / "plugin"
        self.SCHEMA_DIR = self.PROJECT_ROOT / "schema"

        # Source files
        self.DLL_SOURCE = self.BUILD_DIR / "plugin" / "hdCarWash" / "Release" / "hdCarWash.dll"
        self.PLUGINFO_SOURCE = self.PLUGIN_DIR / "plugInfo.json"

        # Target paths (Houdini user-pref directory, version-derived)
        self.HOUDINI_USER_DIR = houdini_user_dir(self.version)
        self.INSTALL_ROOT = self.HOUDINI_USER_DIR / "dso" / "usd" / "hdCarWash"
        self.INSTALL_LIB = self.INSTALL_ROOT / "lib"
        self.INSTALL_RESOURCES = self.INSTALL_ROOT / "resources"
        self.INSTALL_SCHEMA = self.INSTALL_RESOURCES / "schema"
        self.PACKAGES_DIR = self.HOUDINI_USER_DIR / "packages"

        # Target files
        self.DLL_TARGET = self.INSTALL_LIB / "hdCarWash.dll"
        # plugInfo.json must sit at the plugin ROOT (not resources/): its
        # LibraryPath "lib/hdCarWash.dll" is resolved relative to the plugInfo's own
        # directory. With it in resources/, USD looked for the DLL at
        # resources/lib/hdCarWash.dll (which doesn't exist) and the delegate failed
        # to allocate ("unable to read library plugin"). Root + lib/ matches the
        # working repo layout.
        self.PLUGINFO_TARGET = self.INSTALL_ROOT / "plugInfo.json"
        self.PACKAGE_FILE = self.PACKAGES_DIR / "hdCarWash.json"

        # Debug log
        self.DEBUG_LOG = Path("C:/Temp/hdcarwash_debug.txt")


class Logger:
    """Simple logger with optional verbose mode."""

    def __init__(self, verbose: bool = False, log_file: Optional[Path] = None):
        self.verbose = verbose
        self.log_file = log_file
        self._file_handle = None

        if log_file:
            log_file.parent.mkdir(parents=True, exist_ok=True)
            self._file_handle = open(log_file, 'a')

    def __del__(self):
        if self._file_handle:
            self._file_handle.close()

    def _write(self, msg: str):
        print(msg)
        if self._file_handle:
            self._file_handle.write(f"{datetime.datetime.now().isoformat()} - {msg}\n")
            self._file_handle.flush()

    def info(self, msg: str):
        self._write(f"[INFO] {msg}")

    def success(self, msg: str):
        self._write(f"[SUCCESS] {msg}")

    def warning(self, msg: str):
        self._write(f"[WARNING] {msg}")

    def error(self, msg: str):
        self._write(f"[ERROR] {msg}")

    def debug(self, msg: str):
        if self.verbose:
            self._write(f"[DEBUG] {msg}")

    def step(self, msg: str):
        self._write(f"\n{'='*50}")
        self._write(f"  {msg}")
        self._write(f"{'='*50}")


def check_houdini_running(log: Logger) -> bool:
    """Check if Houdini is currently running."""
    log.debug("Checking if Houdini is running...")

    try:
        result = subprocess.run(
            ["tasklist", "/FI", "IMAGENAME eq houdini.exe"],
            capture_output=True,
            text=True,
            timeout=10
        )

        # Also check houdinifx.exe and houdinicore.exe
        for exe in ["houdini.exe", "houdinifx.exe", "houdinicore.exe"]:
            result = subprocess.run(
                ["tasklist", "/FI", f"IMAGENAME eq {exe}"],
                capture_output=True,
                text=True,
                timeout=10
            )
            if exe.lower() in result.stdout.lower():
                log.warning(f"Found running Houdini process: {exe}")
                return True

        return False

    except subprocess.TimeoutExpired:
        log.warning("Timeout checking for Houdini - assuming not running")
        return False
    except Exception as e:
        log.warning(f"Error checking for Houdini: {e}")
        return False


def step_build(log: Logger, config: Config) -> bool:
    """Build the plugin using cmake."""
    log.step("Building HdCarWash")

    if not config.BUILD_DIR.exists():
        log.error(f"Build directory not found: {config.BUILD_DIR}")
        log.error("Run CMake configure first: cmake -B build -G \"Visual Studio 17 2022\"")
        return False

    log.info(f"Build directory: {config.BUILD_DIR}")

    try:
        result = subprocess.run(
            ["cmake", "--build", ".", "--config", "Release", "--parallel"],
            cwd=config.BUILD_DIR,
            capture_output=True,
            text=True,
            timeout=300  # 5 minute timeout
        )

        if result.returncode != 0:
            log.error("Build failed!")
            log.error(f"stderr: {result.stderr}")
            if log.verbose:
                log.debug(f"stdout: {result.stdout}")
            return False

        log.success("Build completed successfully")
        return True

    except subprocess.TimeoutExpired:
        log.error("Build timed out after 5 minutes")
        return False
    except FileNotFoundError:
        log.error("cmake not found in PATH")
        return False
    except Exception as e:
        log.error(f"Build error: {e}")
        return False


def step_copy_dll(log: Logger, config: Config) -> bool:
    """Copy the DLL to the installation directory."""
    log.step("Copying DLL")

    if not config.DLL_SOURCE.exists():
        log.error(f"DLL not found: {config.DLL_SOURCE}")
        log.error("Build may have failed or not been run")
        return False

    try:
        config.INSTALL_LIB.mkdir(parents=True, exist_ok=True)
        shutil.copy2(config.DLL_SOURCE, config.DLL_TARGET)
        log.success(f"Copied: {config.DLL_TARGET.name}")
        log.debug(f"  From: {config.DLL_SOURCE}")
        log.debug(f"  To: {config.DLL_TARGET}")
        return True

    except PermissionError:
        log.error(f"Permission denied copying DLL - is Houdini running?")
        log.error(f"Target: {config.DLL_TARGET}")
        return False
    except Exception as e:
        log.error(f"Failed to copy DLL: {e}")
        return False


def step_copy_pluginfo(log: Logger, config: Config) -> bool:
    """Copy plugInfo.json to the resources directory."""
    log.step("Copying plugInfo.json")

    if not config.PLUGINFO_SOURCE.exists():
        log.error(f"plugInfo.json not found: {config.PLUGINFO_SOURCE}")
        return False

    try:
        config.INSTALL_ROOT.mkdir(parents=True, exist_ok=True)
        shutil.copy2(config.PLUGINFO_SOURCE, config.PLUGINFO_TARGET)
        log.success(f"Copied: {config.PLUGINFO_TARGET.name}")
        log.debug(f"  From: {config.PLUGINFO_SOURCE}")
        log.debug(f"  To: {config.PLUGINFO_TARGET}")
        return True

    except Exception as e:
        log.error(f"Failed to copy plugInfo.json: {e}")
        return False


def step_deploy_schema_plugin(log: Logger, config: Config) -> bool:
    """Deploy the usdCarWash schema resource plugin so the CarWash tab appears.

    Mirrors Houdini's usdKarma layout: a parent plugInfo.json under
    <user>/dso/usd/ declares `{"Includes": ["*/resources/"]}`, and each plugin
    ships its own plugInfo.json + generatedSchema.usda under <name>/resources/.
    The schema's plugInfo carries the `SchemasForRenderers` map that tells
    Houdini's Render Settings LOP to show the CarWashRenderSettingsAPI tab when
    the HdCarWash renderer is selected.
    """
    log.step("Deploying usdCarWash Schema Plugin")

    schema_src = config.PROJECT_ROOT / "usdCarWash" / "resources"
    pluginfo_src = schema_src / "plugInfo.json"
    generated_src = schema_src / "generatedSchema.usda"

    if not pluginfo_src.exists():
        log.error(f"Schema plugInfo not found: {pluginfo_src}")
        log.error("Run usdGenSchema on schema/carWashRenderSettingsAPI.usda first.")
        return False

    try:
        # Parent plugInfo that scans <user>/dso/usd/*/resources/ for plugins.
        usd_root = config.HOUDINI_USER_DIR / "dso" / "usd"
        usd_root.mkdir(parents=True, exist_ok=True)
        parent_pluginfo = usd_root / "plugInfo.json"
        parent_pluginfo.write_text('{\n    "Includes": [ "*/resources/" ]\n}\n')
        log.success(f"Created parent plugInfo: {parent_pluginfo}")

        # usdCarWash resource plugin: plugInfo + generatedSchema under resources/.
        usd_carwash_resources = usd_root / "usdCarWash" / "resources"
        usd_carwash_resources.mkdir(parents=True, exist_ok=True)

        shutil.copy2(pluginfo_src, usd_carwash_resources / "plugInfo.json")
        log.success(f"Copied: usdCarWash plugInfo.json")

        if generated_src.exists():
            shutil.copy2(generated_src, usd_carwash_resources / "generatedSchema.usda")
            log.success(f"Copied: generatedSchema.usda")
        else:
            log.warning(f"generatedSchema.usda missing: {generated_src}")

        return True

    except Exception as e:
        log.error(f"Failed to deploy schema plugin: {e}")
        return False


def step_create_package(log: Logger, config: Config) -> bool:
    """Create or verify the Houdini package file."""
    log.step("Verifying Package Configuration")

    # PXR_PLUGINPATH_NAME needs two entries:
    #   1. the hdCarWash plugin root (plugInfo at root, library plugin)
    #   2. the parent <user>/dso/usd dir — its plugInfo declares
    #      {"Includes": ["*/resources/"]} so USD scans usdCarWash/resources/
    #      (the schema resource plugin that shows the CarWash tab).
    package_content = '''\
{
    "env": [
        {
            "var": "PXR_PLUGINPATH_NAME",
            "value": "$HOUDINI_USER_PREF_DIR/dso/usd/hdCarWash",
            "method": "append"
        },
        {
            "var": "PXR_PLUGINPATH_NAME",
            "value": "$HOUDINI_USER_PREF_DIR/dso/usd",
            "method": "append"
        }
    ]
}
'''

    try:
        config.PACKAGES_DIR.mkdir(parents=True, exist_ok=True)
        # Always (re)write rather than skipping an existing file as "verified":
        # an older package may carry the broken ".../hdCarWash/resources" path,
        # which a content-presence check would wrongly accept.
        with open(config.PACKAGE_FILE, 'w') as f:
            f.write(package_content)
        log.success(f"Created/updated: {config.PACKAGE_FILE.name}")
        return True

    except Exception as e:
        log.error(f"Failed to create package file: {e}")
        return False


def step_verify(log: Logger, config: Config) -> bool:
    """Verify all files are in place."""
    log.step("Verifying Installation")

    files_to_check = [
        ("DLL", config.DLL_TARGET),
        ("plugInfo.json", config.PLUGINFO_TARGET),
        ("Package file", config.PACKAGE_FILE),
        ("Schema plugInfo", config.HOUDINI_USER_DIR / "dso" / "usd" / "usdCarWash" / "resources" / "plugInfo.json"),
        ("generatedSchema.usda", config.HOUDINI_USER_DIR / "dso" / "usd" / "usdCarWash" / "resources" / "generatedSchema.usda"),
        ("Parent plugInfo", config.HOUDINI_USER_DIR / "dso" / "usd" / "plugInfo.json"),
    ]

    all_ok = True
    for name, path in files_to_check:
        if path.exists():
            size = path.stat().st_size
            log.success(f"{name}: OK ({size:,} bytes)")
        else:
            log.error(f"{name}: MISSING - {path}")
            all_ok = False

    return all_ok


def clear_debug_log(log: Logger, config: Config):
    """Clear the debug log file for fresh start."""
    if config.DEBUG_LOG.exists():
        try:
            config.DEBUG_LOG.unlink()
            log.info(f"Cleared debug log: {config.DEBUG_LOG}")
        except Exception as e:
            log.warning(f"Could not clear debug log: {e}")


def print_summary(log: Logger, config: Config, success: bool):
    """Print deployment summary."""
    log.step("Deployment Summary")

    if success:
        log.success("HdCarWash deployed successfully!")
        print()
        print("  Installation location:")
        print(f"    {config.INSTALL_ROOT}")
        print()
        print("  Next steps:")
        print("    1. Launch Houdini")
        print("    2. Create a Solaris stage")
        print("    3. Add Render Settings LOP")
        print("    4. Select 'CarWash Renderer' from dropdown")
        print()
        print("  Debug log:")
        print(f"    {config.DEBUG_LOG}")
        print()
    else:
        log.error("Deployment failed - see errors above")


def main():
    parser = argparse.ArgumentParser(
        description="HdCarWash Automated Deployment System",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python deploy_hdcarwash.py                          # Full build and deploy (auto-detect)
  python deploy_hdcarwash.py --houdini-version 21.0.729  # Pin a specific install
  python deploy_hdcarwash.py --skip-build             # Deploy only
  python deploy_hdcarwash.py --verbose                # Verbose output
"""
    )
    parser.add_argument(
        "--houdini-version",
        default=None,
        help="Houdini version to deploy to (e.g. 21.0.729, 22.0.500). "
             "Defaults to the newest installed Houdini.",
    )
    parser.add_argument(
        "--skip-build",
        action="store_true",
        help="Skip the cmake build step (deploy existing build)"
    )
    parser.add_argument(
        "--verbose", "-v",
        action="store_true",
        help="Enable verbose output"
    )
    parser.add_argument(
        "--log-file",
        type=Path,
        default=None,
        help="Write logs to file (in addition to console)"
    )

    args = parser.parse_args()

    # Initialize
    try:
        config = Config(version=args.houdini_version)
    except FileNotFoundError as e:
        print(f"\n[ERROR] {e}\n")
        return 1
    log_file = args.log_file or (config.PROJECT_ROOT / "deploy.log")
    log = Logger(verbose=args.verbose, log_file=log_file)

    print()
    print("=" * 60)
    print("       HdCarWash Automated Deployment System")
    print("=" * 60)
    print()

    # Pre-flight checks
    log.info(f"Project root: {config.PROJECT_ROOT}")
    log.info(f"Houdini install: {config.install_dir} (version {config.version})")
    log.info(f"User-pref dir: {config.HOUDINI_USER_DIR}")
    log.info(f"Target: {config.INSTALL_ROOT}")

    # Check Houdini not running
    if check_houdini_running(log):
        log.error("Houdini is running!")
        log.error("Please close Houdini before deploying.")
        log.error("The DLL may be locked by the running process.")
        return 1

    log.success("Houdini not running - safe to deploy")

    # Clear debug log for fresh start
    clear_debug_log(log, config)

    # Execute deployment steps
    success = True

    # Step 1: Build (unless skipped)
    if not args.skip_build:
        if not step_build(log, config):
            success = False
    else:
        log.info("Skipping build (--skip-build)")

    # Step 2: Copy DLL
    if success and not step_copy_dll(log, config):
        success = False

    # Step 3: Copy plugInfo.json
    if success and not step_copy_pluginfo(log, config):
        success = False

    # Step 4: Create package file
    if success and not step_create_package(log, config):
        success = False

    # Step 5: Deploy the usdCarWash schema resource plugin
    if success and not step_deploy_schema_plugin(log, config):
        success = False

    # Step 6: Verify installation
    if success:
        success = step_verify(log, config)

    # Print summary
    print_summary(log, config, success)

    return 0 if success else 1


if __name__ == "__main__":
    sys.exit(main())
