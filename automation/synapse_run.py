#!/usr/bin/env python
"""Execute code in Houdini via Synapse - protocol discovery."""

import asyncio
import json

try:
    import websockets
except ImportError:
    import subprocess
    subprocess.run(["pip", "install", "websockets"], check=True)
    import websockets


async def run_in_houdini():
    uri = "ws://localhost:9999"

    async with websockets.connect(uri) as ws:
        # The ping showed aliases: source, target, node, parent, parm, value, type, name, prim_path, prim_type
        # This looks like a node-operation protocol

        # Try node creation style commands
        commands = [
            # Create node style
            {"type": "create_node", "node_type": "sphere", "parent": "/obj"},
            {"type": "node.create", "parent": "/obj", "type": "geo"},

            # Get info style
            {"type": "get_node", "node": "/obj"},
            {"type": "node.info", "path": "/obj"},
            {"type": "list_nodes", "parent": "/"},
            {"type": "get_children", "node": "/obj"},

            # Parm style
            {"type": "get_parm", "node": "/obj", "parm": "tx"},
            {"type": "set_parm", "node": "/obj", "parm": "tx", "value": 0},

            # Scene query
            {"type": "scene_info"},
            {"type": "get_scene"},
            {"type": "hip_info"},

            # LOP/USD style (based on prim_path alias)
            {"type": "get_prim", "prim_path": "/"},
            {"type": "stage_info"},
            {"type": "list_prims"},
        ]

        for cmd in commands:
            try:
                await ws.send(json.dumps(cmd))
                response = await asyncio.wait_for(ws.recv(), timeout=2.0)
                result = json.loads(response)

                cmd_type = cmd.get("type", "unknown")
                success = result.get("success", False)
                error = result.get("error", "")
                data = result.get("data")

                if success:
                    print(f"[OK] {cmd_type}: {json.dumps(data)[:200] if data else 'success'}")
                elif "Unknown" not in str(error):
                    print(f"[--] {cmd_type}: {error[:100]}")
            except asyncio.TimeoutError:
                pass
            except Exception as e:
                print(f"[ERR] {cmd}: {e}")


if __name__ == "__main__":
    asyncio.run(run_in_houdini())
