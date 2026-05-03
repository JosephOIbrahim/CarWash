#!/usr/bin/env python
"""Execute Python in Houdini via the Synapse bridge."""

from __future__ import annotations

import asyncio
import json
import logging
import sys
from pathlib import Path
from typing import Any

sys.path.insert(0, str(Path(__file__).resolve().parent))

from synapse import SynapseConnectionError, load_config
from synapse._protocol import connect, try_command

log = logging.getLogger("carwash.synapse_execute")


async def execute_in_houdini(code: str) -> dict[str, Any] | None:
    """Run ``code`` inside Houdini, trying known command shapes in order."""
    config = load_config()
    commands_to_try: list[dict[str, Any]] = [
        {"type": "node.create", "node": "python", "code": code},
        {"type": "execute_python", "code": code},
        {"type": "python.exec", "code": code},
        {"type": "hou.session.exec", "code": code},
        {"type": "eval", "expression": code},
        {"jsonrpc": "2.0", "method": "execute", "params": {"code": code}, "id": 1},
        {"action": "run_python", "code": code},
    ]

    async with connect(config) as ws:
        for cmd in commands_to_try:
            result = await try_command(ws, cmd, timeout=3.0)
            if result is None:
                continue
            if result.get("success") is True:
                print(f"[SUCCESS] Command format: {list(cmd.keys())}")
                print(f"[RESULT] {json.dumps(result, indent=2)}")
                return result
            if "Unknown" not in str(result.get("error", "")):
                print(f"[RESPONSE] {cmd.get('type', 'custom')}: {json.dumps(result)[:300]}")

    print("[INFO] Could not find working command format")
    return None


async def main_async() -> None:
    code = """
import hou
print("Synapse test successful!")
print(f"Houdini version: {hou.applicationVersionString()}")
"""
    await execute_in_houdini(code)


def main() -> int:
    logging.basicConfig(level=logging.INFO, format="%(levelname)s %(name)s: %(message)s")
    try:
        asyncio.run(main_async())
    except SynapseConnectionError:
        log.exception("Synapse connection failed")
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
