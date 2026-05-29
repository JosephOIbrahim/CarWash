#!/usr/bin/env python3
"""Explore Synapse commands using aliases"""

import asyncio
import websockets
import json

SYNAPSE_URL = "ws://localhost:9999"

async def try_cmd(ws, cmd, desc):
    print(f"\n--- {desc} ---")
    await ws.send(json.dumps(cmd))
    try:
        r = await asyncio.wait_for(ws.recv(), timeout=3)
        d = json.loads(r)
        print(f"Success: {d.get('success')}, Error: {d.get('error')}, Data: {str(d.get('data'))[:200]}")
        return d
    except asyncio.TimeoutError:
        print("Timeout")
        return None

async def main():
    async with websockets.connect(SYNAPSE_URL) as ws:
        print("Connected")

        # Try create_node with various formats
        await try_cmd(ws, {
            "type": "create_node",
            "parent": "/stage",
            "node_type": "sphere"
        }, "create_node with parent/node_type")

        await try_cmd(ws, {
            "type": "create_node",
            "node": "/stage",
            "type": "sphere"  # Using 'type' alias
        }, "create_node with node/type aliases")

        # Try list_nodes
        await try_cmd(ws, {"type": "list_nodes"}, "list_nodes")
        await try_cmd(ws, {"type": "list"}, "list")
        await try_cmd(ws, {"type": "nodes"}, "nodes")

        # Try set_parm with parm/value aliases
        await try_cmd(ws, {
            "type": "set_parm",
            "node": "/stage",
            "parm": "tx",
            "value": 1.0
        }, "set_parm with aliases")

        # Try get_parm
        await try_cmd(ws, {
            "type": "get_parm",
            "node": "/stage",
            "parm": "tx"
        }, "get_parm")

        # Try command types that might exist
        for cmd_type in ["execute", "run", "exec", "script", "hscript", "python_exec", "exec_python"]:
            await try_cmd(ws, {"type": cmd_type, "code": "print(1)"}, cmd_type)

        # Try using 'source' alias for code
        await try_cmd(ws, {
            "type": "execute_python",
            "source": "print('test')"
        }, "execute_python with source alias")

if __name__ == "__main__":
    asyncio.run(main())
