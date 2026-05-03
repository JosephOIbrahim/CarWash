#!/usr/bin/env python
"""Test the Synapse bridge connection and probe for available commands."""

from __future__ import annotations

import asyncio
import json
import logging
import sys
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))

from synapse import SynapseConnectionError, load_config
from synapse._protocol import connect, send_command, try_command

log = logging.getLogger("carwash.synapse_test")


async def test_synapse() -> None:
    config = load_config()

    async with connect(config) as ws:
        print(f"[OK] Connected to {config.synapse_url}")

        ping = await send_command(ws, {"type": "ping"}, timeout=3.0)
        print(f"[PING] {json.dumps(ping)}")

        test_commands: list[dict[str, Any]] = [
            {"type": "help"},
            {"type": "info"},
            {"type": "list"},
            {"type": "commands"},
            {"type": "capabilities"},
            {"type": "run", "code": "print('hello')"},
            {"type": "exec", "code": "print('hello')"},
            {"type": "python", "code": "print('hello')"},
            {"action": "execute", "code": "print('hello')"},
            {"command": "python", "code": "print('hello')"},
            {"method": "execute", "params": {"code": "print('hello')"}},
        ]

        for cmd in test_commands:
            result = await try_command(ws, cmd, timeout=2.0)
            if result is None:
                continue
            if result.get("success") or "error" not in str(result).lower():
                print(f"[TRY] {cmd.get('type', cmd)} -> {json.dumps(result)[:200]}")


def main() -> int:
    logging.basicConfig(level=logging.INFO, format="%(levelname)s %(name)s: %(message)s")
    try:
        asyncio.run(test_synapse())
    except SynapseConnectionError:
        log.exception("Synapse connection failed")
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
