"""Tests for ``synapse.paths``: env overrides win, defaults are platform-aware."""

from __future__ import annotations

from pathlib import Path

import pytest
from synapse import paths


def test_debug_log_env_override(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setenv("CARWASH_DEBUG_LOG", "/tmp/explicit-debug.log")
    assert paths.debug_log_path() == Path("/tmp/explicit-debug.log")


def test_debug_log_default_lives_under_user_data_dir(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    monkeypatch.delenv("CARWASH_DEBUG_LOG", raising=False)
    monkeypatch.setenv("CARWASH_DATA_DIR", str(tmp_path))
    assert paths.debug_log_path() == tmp_path / "hdcarwash_debug.txt"


def test_user_data_dir_env_override(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    monkeypatch.setenv("CARWASH_DATA_DIR", str(tmp_path / "x"))
    assert paths.user_data_dir() == tmp_path / "x"


def test_carwash_dll_env_override(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setenv("CARWASH_DLL_PATH", "/opt/hd/hdCarWash.so")
    assert paths.carwash_dll_path() == Path("/opt/hd/hdCarWash.so")


def test_carwash_build_dll_env_override(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setenv("CARWASH_BUILD_DLL", "/builds/hd/hdCarWash.so")
    assert paths.carwash_build_dll() == Path("/builds/hd/hdCarWash.so")


def test_comfy_input_dir_env_override(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setenv("CARWASH_COMFY_INPUT_DIR", "/srv/comfy/input")
    assert paths.comfy_input_dir() == Path("/srv/comfy/input")
