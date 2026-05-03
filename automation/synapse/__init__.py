"""Shared library for the CarWash automation scripts.

Modules
-------
config    Layered configuration (defaults < JSON file < env vars).
paths     Cross-platform path resolution (DLL, debug log, ComfyUI input dir).
errors    Typed exceptions raised by the automation surface.
_protocol Shared async helpers for talking to the Synapse WebSocket bridge.
"""

from .config import CarWashConfig, load_config
from .errors import (
    SynapseCommandError,
    SynapseConnectionError,
    SynapseError,
    SynapseTimeoutError,
)

__all__ = [
    "CarWashConfig",
    "SynapseCommandError",
    "SynapseConnectionError",
    "SynapseError",
    "SynapseTimeoutError",
    "load_config",
]
