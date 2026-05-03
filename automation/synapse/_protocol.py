"""Shared Synapse WebSocket bridge helpers.

Constitution: C2 (errors are loud), C5 (pure cores, impure shells), C8
(smallest reversible change). The five `synapse_*.py` scripts each had a
near-identical try/except dance to send a JSON command and parse a JSON
reply. That logic now lives here exactly once and surfaces typed errors
instead of swallowing them.
"""

from __future__ import annotations

import asyncio
import json
import logging
from collections.abc import AsyncIterator
from contextlib import asynccontextmanager
from typing import TYPE_CHECKING, Any

from .config import CarWashConfig, load_config
from .errors import SynapseCommandError, SynapseConnectionError, SynapseTimeoutError

if TYPE_CHECKING:
    from websockets.legacy.client import WebSocketClientProtocol

log = logging.getLogger("carwash.synapse")


def _import_websockets() -> Any:
    try:
        import websockets
    except ImportError as exc:
        raise SynapseConnectionError(
            "The `websockets` package is required. "
            "Install it with: pip install -r automation/requirements.txt"
        ) from exc
    return websockets


@asynccontextmanager
async def connect(
    config: CarWashConfig | None = None,
) -> AsyncIterator[WebSocketClientProtocol]:
    """Open a WebSocket connection to the Synapse bridge.

    Wraps the underlying ``websockets.connect`` so callers receive a typed
    :class:`SynapseConnectionError` on failure instead of an opaque
    ``OSError``.
    """
    cfg = config or load_config()
    websockets = _import_websockets()
    try:
        async with websockets.connect(cfg.synapse_url) as ws:
            yield ws
    except (ConnectionRefusedError, OSError) as exc:
        raise SynapseConnectionError(
            f"Could not connect to Synapse bridge at {cfg.synapse_url}: {exc}"
        ) from exc


async def send_command(
    ws: WebSocketClientProtocol,
    command: dict[str, Any],
    *,
    timeout: float = 3.0,
) -> dict[str, Any]:
    """Send one JSON command, await the reply, and decode it.

    Raises
    ------
    SynapseTimeoutError
        If no reply arrives within ``timeout`` seconds.
    SynapseCommandError
        If the reply is not valid JSON or is not a JSON object.
    """
    await ws.send(json.dumps(command))
    try:
        raw = await asyncio.wait_for(ws.recv(), timeout=timeout)
    except asyncio.TimeoutError as exc:
        raise SynapseTimeoutError(
            f"Synapse command {command!r} timed out after {timeout}s"
        ) from exc

    try:
        decoded = json.loads(raw)
    except json.JSONDecodeError as exc:
        raise SynapseCommandError(
            f"Synapse returned non-JSON reply: {raw!r}",
            command=command,
            response=raw,
        ) from exc

    if not isinstance(decoded, dict):
        raise SynapseCommandError(
            f"Synapse returned non-object reply: {decoded!r}",
            command=command,
            response=decoded,
        )
    return decoded


async def try_command(
    ws: WebSocketClientProtocol,
    command: dict[str, Any],
    *,
    timeout: float = 3.0,
) -> dict[str, Any] | None:
    """Best-effort variant for protocol-discovery loops.

    Returns the decoded reply, or ``None`` for the two recoverable cases
    (timeout and command rejection). All other errors propagate. Each
    swallowed case is logged at DEBUG level so failures are never silent.
    """
    try:
        return await send_command(ws, command, timeout=timeout)
    except SynapseTimeoutError as exc:
        log.debug("synapse try_command timeout: %s", exc)
        return None
    except SynapseCommandError as exc:
        log.debug("synapse try_command rejected: %s", exc)
        return None
