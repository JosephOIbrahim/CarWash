#!/usr/bin/env python3
"""Final Synapse attempt with nested params"""

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
        print(f"Success: {d.get('success')}")
        if d.get('error'):
            print(f"Error: {d.get('error')}")
        if d.get('data'):
            print(f"Data: {json.dumps(d.get('data'), indent=2)[:500]}")
        return d
    except asyncio.TimeoutError:
        print("Timeout")
        return None

async def main():
    async with websockets.connect(SYNAPSE_URL) as ws:
        print("Connected to Synapse")

        # Try create_node with params wrapper
        await try_cmd(ws, {
            "type": "create_node",
            "params": {
                "parent": "/stage",
                "type": "sphere"
            }
        }, "create_node with params wrapper")

        # Try with data wrapper
        await try_cmd(ws, {
            "type": "create_node",
            "data": {
                "parent": "/stage",
                "type": "sphere"
            }
        }, "create_node with data wrapper")

        # Try set_parm with path instead of node
        await try_cmd(ws, {
            "type": "set_parm",
            "path": "/stage",
            "parm": "tx",
            "value": 1.0
        }, "set_parm with path")

        await try_cmd(ws, {
            "type": "set_parm",
            "node_path": "/stage",
            "parm": "tx",
            "value": 1.0
        }, "set_parm with node_path")

        # Try get_parm with path
        await try_cmd(ws, {
            "type": "get_parm",
            "path": "/stage",
            "parm": "tx"
        }, "get_parm with path")

        # Try execute_python with params wrapper
        await try_cmd(ws, {
            "type": "execute_python",
            "params": {
                "code": "print(1+1)"
            }
        }, "execute_python with params.code")

        # Try hython
        await try_cmd(ws, {
            "type": "hython",
            "code": "print(1)"
        }, "hython")

        # Try hou_exec
        await try_cmd(ws, {
            "type": "hou_exec",
            "code": "print(1)"
        }, "hou_exec")

        # List available command types by trying common ones
        known_commands = []
        for cmd in ["ping", "execute_python", "create_node", "set_parm", "get_parm",
                    "connect_nodes", "disconnect_nodes", "delete_node", "render"]:
            await ws.send(json.dumps({"type": cmd}))
            try:
                r = await asyncio.wait_for(ws.recv(), timeout=1)
                d = json.loads(r)
                if "Unknown command type" not in str(d.get('error', '')):
                    known_commands.append(cmd)
            except:
                pass

        print(f"\nKnown command types: {known_commands}")

if __name__ == "__main__":
    asyncio.run(main())
