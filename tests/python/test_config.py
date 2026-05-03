"""Tests for ``synapse.config``.

Covers the layering contract: defaults < file < env < kwargs, and the
unknown-key rejection that protects against silent typos in config files.
"""

from __future__ import annotations

import json
from pathlib import Path

import pytest
from synapse import config as cfg_mod
from synapse.config import CarWashConfig, load_config


def test_defaults_are_addressable() -> None:
    cfg = CarWashConfig()
    assert cfg.comfy_url.startswith("http://")
    assert cfg.synapse_url.startswith("ws://")
    assert cfg.inference_steps > 0
    assert cfg.guidance_scale > 0
    assert cfg.max_dim > 0


def test_env_overrides_defaults(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setenv("CARWASH_COMFY_URL", "http://example.test:9000")
    monkeypatch.setenv("CARWASH_INFERENCE_STEPS", "33")
    monkeypatch.setenv("CARWASH_GUIDANCE_SCALE", "9.25")
    # Make sure the fallback config-file path doesn't pick up a stale file.
    monkeypatch.setenv("CARWASH_CONFIG_FILE", "/nonexistent/cw-config.json")

    cfg = load_config()
    assert cfg.comfy_url == "http://example.test:9000"
    assert cfg.inference_steps == 33
    assert cfg.guidance_scale == pytest.approx(9.25)


def test_kwargs_override_env(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setenv("CARWASH_COMFY_URL", "http://from-env:1234")
    monkeypatch.setenv("CARWASH_CONFIG_FILE", "/nonexistent/cw-config.json")
    cfg = load_config(comfy_url="http://from-kwarg:5678")
    assert cfg.comfy_url == "http://from-kwarg:5678"


def test_file_overrides_defaults(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    config_file = tmp_path / "cw.json"
    config_file.write_text(json.dumps({"comfy_url": "http://file:7000", "seed": 7}))
    monkeypatch.setenv("CARWASH_CONFIG_FILE", str(config_file))
    # Test the file layer in isolation: drop any inherited overrides.
    monkeypatch.delenv("CARWASH_COMFY_URL", raising=False)
    monkeypatch.delenv("CARWASH_SEED", raising=False)

    cfg = load_config()
    assert cfg.comfy_url == "http://file:7000"
    assert cfg.seed == 7


def test_env_beats_file(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    config_file = tmp_path / "cw.json"
    config_file.write_text(json.dumps({"comfy_url": "http://file:1"}))
    monkeypatch.setenv("CARWASH_CONFIG_FILE", str(config_file))
    monkeypatch.setenv("CARWASH_COMFY_URL", "http://env:2")
    cfg = load_config()
    assert cfg.comfy_url == "http://env:2"


def test_unknown_keys_in_file_are_rejected(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> None:
    config_file = tmp_path / "cw.json"
    config_file.write_text(json.dumps({"definitely_not_a_field": True}))
    monkeypatch.setenv("CARWASH_CONFIG_FILE", str(config_file))
    with pytest.raises(ValueError, match="unknown config keys"):
        load_config()


def test_dump_roundtrip() -> None:
    cfg = CarWashConfig(comfy_url="http://x:1", seed=99)
    text = cfg_mod.dump(cfg)
    parsed = json.loads(text)
    assert parsed["comfy_url"] == "http://x:1"
    assert parsed["seed"] == 99
