#!/usr/bin/env python3
"""Test various Synapse protocol variations"""

import asyncio
import websockets
import json

SYNAPSE_URL = "ws://localhost:9999"

TEST_CODE = "print('Hello from Synapse!')"

async def try_command(ws, cmd, desc):
    """Try a command and show result"""
    print(f"\n--- Testing: {desc} ---")
    print(f"Command: {json.dumps(cmd)}")
    await ws.send(json.dumps(cmd))
    try:
        response = await asyncio.wait_for(ws.recv(), timeout=3)
        data = json.loads(response)
        success = data.get("success", False)
        error = data.get("error", "")
        print(f"Success: {success}, Error: {error}")
        return data
    except asyncio.TimeoutError:
        print("Timeout (no response)")
        return None

async def main():
    async with websockets.connect(SYNAPSE_URL) as ws:
        print("Connected to Synapse")

        # Get available commands first
        await try_command(ws, {"type": "help"}, "help")
        await try_command(ws, {"type": "list_commands"}, "list_commands")
        await try_command(ws, {"type": "commands"}, "commands")

        # Try different field names for execute_python
        await try_command(ws, {
            "type": "execute_python",
            "code": TEST_CODE
        }, "execute_python with 'code'")

        await try_command(ws, {
            "type": "execute_python",
            "script": TEST_CODE
        }, "execute_python with 'script'")

        await try_command(ws, {
            "type": "execute_python",
            "source": TEST_CODE
        }, "execute_python with 'source'")

        await try_command(ws, {
            "type": "execute_python",
            "python": TEST_CODE
        }, "execute_python with 'python'")

        await try_command(ws, {
            "type": "execute_python",
            "command": TEST_CODE
        }, "execute_python with 'command'")

        # Try run_python
        await try_command(ws, {
            "type": "run_python",
            "code": TEST_CODE
        }, "run_python")

        # Try eval
        await try_command(ws, {
            "type": "eval",
            "code": TEST_CODE
        }, "eval")

        await try_command(ws, {
            "type": "python",
            "code": TEST_CODE
        }, "python")

        # Try execute with nested data
        await try_command(ws, {
            "type": "execute_python",
            "data": {"code": TEST_CODE}
        }, "execute_python with data.code")

        await try_command(ws, {
            "type": "execute_python",
            "params": {"code": TEST_CODE}
        }, "execute_python with params.code")

if __name__ == "__main__":
    asyncio.run(main())
