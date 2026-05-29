#!/usr/bin/env python3
"""
HdCarWash Automated Deployment System
======================================
Builds and deploys the HdCarWash Hydra delegate to Houdini 21.

Usage:
    python deploy_hdcarwash.py          # Full build and deploy
    python deploy_hdcarwash.py --skip-build  # Deploy only (skip cmake build)
    python deploy_hdcarwash.py --verbose     # Verbose output
"""

import argparse
import datetime
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Optional


class Config:
    """Centralized deployment configuration."""

    # Source paths (relative to project root)
    PROJECT_ROOT = Path(__file__).parent.resolve()
    BUILD_DIR = PROJECT_ROOT / "build"
    PLUGIN_DIR = PROJECT_ROOT / "plugin"
    SCHEMA_DIR = PROJECT_ROOT / "schema"

    # Source files
    DLL_SOURCE = BUILD_DIR / "plugin" / "hdCarWash" / "Release" / "hdCarWash.dll"
    PLUGINFO_SOURCE = PLUGIN_DIR / "plugInfo.json"

    # Target paths (Houdini installation)
    HOUDINI_USER_DIR = Path.home() / "houdini21.0"
    INSTALL_ROOT = HOUDINI_USER_DIR / "dso" / "usd" / "hdCarWash"
    INSTALL_LIB = INSTALL_ROOT / "lib"
    INSTALL_RESOURCES = INSTALL_ROOT / "resources"
    INSTALL_SCHEMA = INSTALL_RESOURCES / "schema"
    PACKAGES_DIR = HOUDINI_USER_DIR / "packages"

    # Target files
    DLL_TARGET = INSTALL_LIB / "hdCarWash.dll"
    PLUGINFO_TARGET = INSTALL_RESOURCES / "plugInfo.json"
    PACKAGE_FILE = PACKAGES_DIR / "hdCarWash.json"

    # Debug log
    DEBUG_LOG = Path("C:/Temp/hdcarwash_debug.txt")


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
        config.INSTALL_RESOURCES.mkdir(parents=True, exist_ok=True)
        shutil.copy2(config.PLUGINFO_SOURCE, config.PLUGINFO_TARGET)
        log.success(f"Copied: {config.PLUGINFO_TARGET.name}")
        log.debug(f"  From: {config.PLUGINFO_SOURCE}")
        log.debug(f"  To: {config.PLUGINFO_TARGET}")
        return True

    except Exception as e:
        log.error(f"Failed to copy plugInfo.json: {e}")
        return False


def step_create_schemas(log: Logger, config: Config) -> bool:
    """Create schema placeholder files."""
    log.step("Creating Schema Placeholders")

    schemas = {
        "carWashSchema.usda": '''\
#usda 1.0
(
    doc = "CarWash Renderer Schema - Placeholder"
    subLayers = []
)

def "CarWashSchema" (
    doc = "Schema for CarWash renderer settings"
)
{
}
''',
        "cognitiveSubstrate.usda": '''\
#usda 1.0
(
    doc = "Cognitive Substrate Schema - Placeholder"
    subLayers = []
)

def "CognitiveSubstrate" (
    doc = "Schema for cognitive substrate integration"
)
{
}
'''
    }

    try:
        config.INSTALL_SCHEMA.mkdir(parents=True, exist_ok=True)

        for filename, content in schemas.items():
            schema_path = config.INSTALL_SCHEMA / filename
            with open(schema_path, 'w') as f:
                f.write(content)
            log.success(f"Created: {filename}")
            log.debug(f"  Path: {schema_path}")

        return True

    except Exception as e:
        log.error(f"Failed to create schema files: {e}")
        return False


def step_create_package(log: Logger, config: Config) -> bool:
    """Create or verify the Houdini package file."""
    log.step("Verifying Package Configuration")

    package_content = '''\
{
    "env": [
        {
            "var": "PXR_PLUGINPATH_NAME",
            "value": "$HOUDINI_USER_PREF_DIR/dso/usd/hdCarWash/resources",
            "method": "append"
        }
    ]
}
'''

    try:
        config.PACKAGES_DIR.mkdir(parents=True, exist_ok=True)

        if config.PACKAGE_FILE.exists():
            log.info(f"Package file exists: {config.PACKAGE_FILE.name}")
            # Verify content
            with open(config.PACKAGE_FILE, 'r') as f:
                existing = f.read()
            if "PXR_PLUGINPATH_NAME" in existing and "hdCarWash" in existing:
                log.success("Package file verified")
                return True
            else:
                log.warning("Package file may be outdated, updating...")

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
        ("carWashSchema.usda", config.INSTALL_SCHEMA / "carWashSchema.usda"),
        ("cognitiveSubstrate.usda", config.INSTALL_SCHEMA / "cognitiveSubstrate.usda"),
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
        print("    1. Launch Houdini 21")
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
  python deploy_hdcarwash.py              # Full build and deploy
  python deploy_hdcarwash.py --skip-build # Deploy only
  python deploy_hdcarwash.py --verbose    # Verbose output
"""
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
    config = Config()
    log_file = args.log_file or (config.PROJECT_ROOT / "deploy.log")
    log = Logger(verbose=args.verbose, log_file=log_file)

    print()
    print("=" * 60)
    print("       HdCarWash Automated Deployment System")
    print("=" * 60)
    print()

    # Pre-flight checks
    log.info(f"Project root: {config.PROJECT_ROOT}")
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

    # Step 5: Create schema placeholders
    if success and not step_create_schemas(log, config):
        success = False

    # Step 6: Verify installation
    if success:
        success = step_verify(log, config)

    # Print summary
    print_summary(log, config, success)

    return 0 if success else 1


if __name__ == "__main__":
    sys.exit(main())
