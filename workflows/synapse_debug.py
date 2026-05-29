#!/usr/bin/env python3
"""Debug Synapse execute_python command"""

import asyncio
import websockets
import json
import uuid

SYNAPSE_URL = "ws://localhost:9999"
TEST_CODE = "print('Hello!')"

async def try_command(ws, cmd, desc):
    print(f"\n=== {desc} ===")
    print(f"Sending: {json.dumps(cmd, indent=2)}")
    await ws.send(json.dumps(cmd))
    try:
        response = await asyncio.wait_for(ws.recv(), timeout=3)
        data = json.loads(response)
        print(f"Response: {json.dumps(data, indent=2)}")
        return data
    except asyncio.TimeoutError:
        print("Timeout")
        return None

async def main():
    async with websockets.connect(SYNAPSE_URL) as ws:
        print("Connected to Synapse v2.3.0")

        # Try with message ID
        await try_command(ws, {
            "id": str(uuid.uuid4()),
            "type": "execute_python",
            "code": TEST_CODE
        }, "With UUID id")

        # Try with numeric id
        await try_command(ws, {
            "id": 1,
            "type": "execute_python",
            "code": TEST_CODE
        }, "With numeric id")

        # Try with empty string id
        await try_command(ws, {
            "id": "",
            "type": "execute_python",
            "code": TEST_CODE
        }, "With empty id")

        # Try getting info about a node to see working command format
        await try_command(ws, {
            "type": "get_node",
            "path": "/stage"
        }, "get_node /stage")

        await try_command(ws, {
            "type": "get_node",
            "node_path": "/stage"
        }, "get_node with node_path")

        # Try listing children
        await try_command(ws, {
            "type": "get_children",
            "path": "/stage"
        }, "get_children /stage")

        # Try ping
        await try_command(ws, {
            "type": "ping"
        }, "ping")

        # Try get_info
        await try_command(ws, {
            "type": "get_info"
        }, "get_info")

        await try_command(ws, {
            "type": "info"
        }, "info")

        await try_command(ws, {
            "type": "status"
        }, "status")

if __name__ == "__main__":
    asyncio.run(main())
