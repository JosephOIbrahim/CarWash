"""Cross-platform path resolution.

Constitution: C9 — Windows-specific bits are isolated behind a thin shim.

The original automation scripts hardcoded ``C:\\Temp\\hdcarwash_debug.txt`` and
user-home DLL paths inline. This module centralises those lookups and respects
``%APPDATA%`` on Windows / ``$XDG_DATA_HOME`` elsewhere, plus explicit
overrides via environment variables. Every caller goes through here so we can
move debug artefacts in one place.
"""

from __future__ import annotations

import os
import sys
from pathlib import Path


def _windows_local_appdata() -> Path:
    appdata = os.environ.get("LOCALAPPDATA") or os.environ.get("APPDATA")
    if appdata:
        return Path(appdata)
    return Path.home() / "AppData" / "Local"


def user_data_dir() -> Path:
    """Return the per-user data directory for CarWash artefacts.

    Layered: ``CARWASH_DATA_DIR`` overrides everything; otherwise platform
    defaults apply (``%LOCALAPPDATA%\\CarWash`` on Windows,
    ``$XDG_DATA_HOME/carwash`` or ``~/.local/share/carwash`` elsewhere).
    """
    override = os.environ.get("CARWASH_DATA_DIR")
    if override:
        return Path(override).expanduser()

    if sys.platform.startswith("win"):
        return _windows_local_appdata() / "CarWash"

    xdg = os.environ.get("XDG_DATA_HOME")
    base = Path(xdg).expanduser() if xdg else Path.home() / ".local" / "share"
    return base / "carwash"


def debug_log_path() -> Path:
    """Default path for the plugin debug log.

    Override with ``CARWASH_DEBUG_LOG``; falls back to a file inside
    :func:`user_data_dir`.
    """
    override = os.environ.get("CARWASH_DEBUG_LOG")
    if override:
        return Path(override).expanduser()
    return user_data_dir() / "hdcarwash_debug.txt"


def houdini_user_pref_dir(version: str = "21.0") -> Path:
    """Return the Houdini user preferences directory.

    Honours ``HOUDINI_USER_PREF_DIR`` if set (Houdini's own override mechanism);
    otherwise reconstructs the conventional ``~/houdiniXY.Z`` path.
    """
    override = os.environ.get("HOUDINI_USER_PREF_DIR")
    if override:
        return Path(override).expanduser()
    return Path.home() / f"houdini{version}"


def carwash_dll_path(houdini_version: str = "21.0") -> Path:
    """Resolve the installed plugin DLL.

    Override with ``CARWASH_DLL_PATH`` (full path to the DLL/so) for CI or
    non-default Houdini installs.
    """
    override = os.environ.get("CARWASH_DLL_PATH")
    if override:
        return Path(override).expanduser()
    suffix = "hdCarWash.dll" if sys.platform.startswith("win") else "libhdCarWash.so"
    return houdini_user_pref_dir(houdini_version) / "dso" / "usd" / "hdCarWash" / "lib" / suffix


def carwash_build_dll(repo_root: Path | None = None) -> Path:
    """Resolve the *built* plugin DLL inside the repo (pre-install location).

    Override with ``CARWASH_BUILD_DLL``.
    """
    override = os.environ.get("CARWASH_BUILD_DLL")
    if override:
        return Path(override).expanduser()
    root = repo_root or Path(__file__).resolve().parents[2]
    suffix = "hdCarWash.dll" if sys.platform.startswith("win") else "libhdCarWash.so"
    return root / "build" / "plugin" / "hdCarWash" / "Release" / suffix


def comfy_input_dir() -> Path:
    """Default ComfyUI input directory.

    Override with ``CARWASH_COMFY_INPUT_DIR``.
    """
    override = os.environ.get("CARWASH_COMFY_INPUT_DIR")
    if override:
        return Path(override).expanduser()
    if sys.platform.startswith("win"):
        return Path("C:/ComfyUI/input")
    return Path.home() / "ComfyUI" / "input"
