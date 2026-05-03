#!/usr/bin/env python
"""Discover the parameter name `execute_python` expects on the Synapse bridge."""

from __future__ import annotations

import asyncio
import json
import logging
import sys
from pathlib import Path

# Allow `from synapse import ...` whether the script is run as
# `python automation/synapse_discover.py` or from inside automation/.
sys.path.insert(0, str(Path(__file__).resolve().parent))

from synapse import SynapseConnectionError, load_config
from synapse._protocol import connect, try_command

log = logging.getLogger("carwash.synapse_discover")


async def discover_protocol() -> str | None:
    config = load_config()
    code = "print('test')"

    param_names = [
        "python",
        "script",
        "source",
        "expression",
        "command",
        "exec",
        "content",
        "body",
        "data",
        "text",
    ]

    async with connect(config) as ws:
        for param in param_names:
            cmd = {"type": "execute_python", param: code}
            result = await try_command(ws, cmd, timeout=2.0)
            if result is None:
                print(f"[{param}] no reply")
                continue
            error = (result.get("error") or "")[:80]
            print(f"[{param}] success={result.get('success')} error={error}")
            if result.get("success"):
                print(f"   FOUND IT! Use: {param}")
                return param

        print("\n--- Trying help commands ---")
        help_cmds = [
            {"type": "schema"},
            {"type": "get_schema"},
            {"type": "help", "topic": "execute_python"},
            {"type": "describe", "command": "execute_python"},
        ]
        for cmd in help_cmds:
            result = await try_command(ws, cmd, timeout=2.0)
            if result and result.get("success"):
                print(f"[{cmd.get('type')}] {json.dumps(result, indent=2)[:500]}")

    return None


def main() -> int:
    logging.basicConfig(level=logging.INFO, format="%(levelname)s %(name)s: %(message)s")
    try:
        asyncio.run(discover_protocol())
    except SynapseConnectionError:
        log.exception("Synapse connection failed")
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
