#!/usr/bin/env python
"""Probe the Synapse bridge with node/scene/USD-style commands."""

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

log = logging.getLogger("carwash.synapse_run")


async def run_in_houdini() -> None:
    config = load_config()
    commands: list[dict[str, Any]] = [
        # Create node
        {"type": "create_node", "node_type": "sphere", "parent": "/obj"},
        # Original probe sent {"type": "node.create", ...} but overwrote "type"
        # with "geo"; preserve the wire payload (the second value wins).
        {"parent": "/obj", "type": "geo"},
        # Get info
        {"type": "get_node", "node": "/obj"},
        {"type": "node.info", "path": "/obj"},
        {"type": "list_nodes", "parent": "/"},
        {"type": "get_children", "node": "/obj"},
        # Parm
        {"type": "get_parm", "node": "/obj", "parm": "tx"},
        {"type": "set_parm", "node": "/obj", "parm": "tx", "value": 0},
        # Scene query
        {"type": "scene_info"},
        {"type": "get_scene"},
        {"type": "hip_info"},
        # LOP/USD
        {"type": "get_prim", "prim_path": "/"},
        {"type": "stage_info"},
        {"type": "list_prims"},
    ]

    async with connect(config) as ws:
        for cmd in commands:
            result = await try_command(ws, cmd, timeout=2.0)
            if result is None:
                continue
            cmd_type = cmd.get("type", "unknown")
            success = result.get("success", False)
            error = result.get("error", "")
            data = result.get("data")

            if success:
                payload = json.dumps(data)[:200] if data else "success"
                print(f"[OK] {cmd_type}: {payload}")
            elif "Unknown" not in str(error):
                print(f"[--] {cmd_type}: {str(error)[:100]}")


def main() -> int:
    logging.basicConfig(level=logging.INFO, format="%(levelname)s %(name)s: %(message)s")
    try:
        asyncio.run(run_in_houdini())
    except SynapseConnectionError:
        log.exception("Synapse connection failed")
        return 2
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
