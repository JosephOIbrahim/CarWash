"""Tests for ``synapse._protocol``: connect/send/try semantics with a fake socket.

The real WebSocket transport is replaced by a lightweight fake so we can
verify the typed-error contract (no swallowed failures) without standing up
a Synapse bridge.
"""

from __future__ import annotations

import asyncio
import json
from typing import Any

import pytest
from synapse import _protocol
from synapse.errors import SynapseCommandError, SynapseConnectionError, SynapseTimeoutError


class _FakeWs:
    def __init__(self, replies: list[Any]) -> None:
        self.sent: list[str] = []
        self._replies = list(replies)

    async def send(self, payload: str) -> None:
        self.sent.append(payload)

    async def recv(self) -> Any:
        if not self._replies:
            # Simulate hang -> caller's wait_for must time out.
            await asyncio.sleep(10)
        item = self._replies.pop(0)
        if isinstance(item, BaseException):
            raise item
        return item


def test_send_command_returns_decoded_object() -> None:
    ws = _FakeWs([json.dumps({"success": True, "data": 42})])
    result = asyncio.run(_protocol.send_command(ws, {"type": "ping"}, timeout=1.0))
    assert result == {"success": True, "data": 42}
    assert json.loads(ws.sent[0]) == {"type": "ping"}


def test_send_command_raises_on_timeout() -> None:
    ws = _FakeWs([])  # never replies
    with pytest.raises(SynapseTimeoutError):
        asyncio.run(_protocol.send_command(ws, {"type": "x"}, timeout=0.01))


def test_send_command_raises_on_non_json() -> None:
    ws = _FakeWs(["this is not json"])
    with pytest.raises(SynapseCommandError):
        asyncio.run(_protocol.send_command(ws, {"type": "x"}, timeout=1.0))


def test_send_command_raises_on_non_object_reply() -> None:
    ws = _FakeWs([json.dumps([1, 2, 3])])
    with pytest.raises(SynapseCommandError):
        asyncio.run(_protocol.send_command(ws, {"type": "x"}, timeout=1.0))


def test_try_command_swallows_recoverable_failures() -> None:
    ws = _FakeWs([json.dumps({"success": True})])
    result = asyncio.run(_protocol.try_command(ws, {"type": "x"}, timeout=1.0))
    assert result == {"success": True}

    ws_timeout = _FakeWs([])
    assert asyncio.run(_protocol.try_command(ws_timeout, {"type": "x"}, timeout=0.01)) is None

    ws_bad = _FakeWs(["nope"])
    assert asyncio.run(_protocol.try_command(ws_bad, {"type": "x"}, timeout=1.0)) is None


def test_connect_translates_oserror_to_typed_exception(monkeypatch: pytest.MonkeyPatch) -> None:
    class _ExplodingWebsockets:
        @staticmethod
        def connect(_url: str) -> Any:
            class _Cm:
                async def __aenter__(self) -> Any:
                    raise OSError("simulated connection refused")

                async def __aexit__(self, *a: Any) -> None:
                    return None

            return _Cm()

    monkeypatch.setattr(_protocol, "_import_websockets", lambda: _ExplodingWebsockets)

    async def _run() -> None:
        async with _protocol.connect():
            pass  # pragma: no cover — should not be reached

    with pytest.raises(SynapseConnectionError):
        asyncio.run(_run())
