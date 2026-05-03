"""Layered configuration object for CarWash automation.

Constitution: C3 — config is data, not code.

Layering (highest precedence wins):
    1. Explicit kwargs to :func:`load_config`
    2. Process environment variables (prefixed ``CARWASH_``)
    3. JSON file at ``CARWASH_CONFIG_FILE`` or ``<user_data_dir>/config.json``
    4. Hard-coded defaults

The dataclass is also the single source of truth for the C++ side eventually
(R-CFG-1 C++ port, deferred to a follow-up); keeping the field names
ASCII-snake_case keeps that port mechanical.
"""

from __future__ import annotations

import json
import os
from dataclasses import asdict, dataclass, fields
from pathlib import Path
from typing import Any, get_type_hints

from .paths import comfy_input_dir, debug_log_path, user_data_dir


@dataclass(frozen=True)
class CarWashConfig:
    """All runtime knobs for the CarWash automation surface."""

    # Networking
    comfy_url: str = "http://127.0.0.1:8188"
    synapse_url: str = "ws://127.0.0.1:9999"
    request_timeout_sec: float = 60.0

    # Inference defaults (kept here so they are not magic numbers in source).
    seed: int = 42
    inference_steps: int = 20
    guidance_scale: float = 7.5
    max_dim: int = 8192

    # Paths (resolved lazily so user/CI can override with env vars).
    comfy_input_dir: str = ""
    debug_log_path: str = ""

    def resolved_comfy_input_dir(self) -> Path:
        return Path(self.comfy_input_dir) if self.comfy_input_dir else comfy_input_dir()

    def resolved_debug_log_path(self) -> Path:
        return Path(self.debug_log_path) if self.debug_log_path else debug_log_path()


_ENV_PREFIX = "CARWASH_"


def _coerce(field_type: type, raw: str) -> Any:
    if field_type is bool:
        return raw.strip().lower() in {"1", "true", "yes", "on"}
    if field_type is int:
        return int(raw)
    if field_type is float:
        return float(raw)
    return raw


def _from_env() -> dict[str, Any]:
    # `from __future__ import annotations` makes Field.type a string, so we
    # resolve it through get_type_hints to recover the real type for coercion.
    hints = get_type_hints(CarWashConfig)
    out: dict[str, Any] = {}
    for f in fields(CarWashConfig):
        env_key = _ENV_PREFIX + f.name.upper()
        if env_key in os.environ:
            out[f.name] = _coerce(hints[f.name], os.environ[env_key])
    return out


def _from_file(path: Path) -> dict[str, Any]:
    if not path.is_file():
        return {}
    with path.open(encoding="utf-8") as handle:
        data = json.load(handle)
    if not isinstance(data, dict):
        raise TypeError(f"{path}: expected a JSON object at top level")
    valid = {f.name for f in fields(CarWashConfig)}
    unknown = set(data) - valid
    if unknown:
        raise ValueError(f"{path}: unknown config keys: {sorted(unknown)}")
    return data


def _config_file_path() -> Path:
    override = os.environ.get(_ENV_PREFIX + "CONFIG_FILE")
    if override:
        return Path(override).expanduser()
    return user_data_dir() / "config.json"


def load_config(**overrides: Any) -> CarWashConfig:
    """Build a :class:`CarWashConfig` from layered sources."""
    file_data = _from_file(_config_file_path())
    env_data = _from_env()
    merged: dict[str, Any] = {**file_data, **env_data, **overrides}
    return CarWashConfig(**merged)


def dump(config: CarWashConfig) -> str:
    """Serialise a config to a JSON string (used for diagnostics)."""
    return json.dumps(asdict(config), indent=2, sort_keys=True)
