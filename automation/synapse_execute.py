#!/usr/bin/env python
"""Execute Python in Houdini via Synapse bridge."""

import asyncio
import json
import sys

try:
    import websockets
except ImportError:
    import subprocess
    subprocess.run(["pip", "install", "websockets"], check=True)
    import websockets


async def execute_in_houdini(code: str):
    """Execute Python code in Houdini via Synapse."""
    uri = "ws://localhost:9999"

    async with websockets.connect(uri) as ws:
        # Try different command formats based on aliases
        commands_to_try = [
            # Using node/parm style
            {"type": "node.create", "node": "python", "code": code},
            {"type": "execute_python", "code": code},
            {"type": "python.exec", "code": code},
            {"type": "hou.session.exec", "code": code},
            {"type": "eval", "expression": code},
            # JSON-RPC style
            {"jsonrpc": "2.0", "method": "execute", "params": {"code": code}, "id": 1},
            # Simple action style
            {"action": "run_python", "code": code},
        ]

        for cmd in commands_to_try:
            try:
                await ws.send(json.dumps(cmd))
                response = await asyncio.wait_for(ws.recv(), timeout=3.0)
                result = json.loads(response)

                # Check if it worked
                if result.get("success") == True:
                    print(f"[SUCCESS] Command format: {list(cmd.keys())}")
                    print(f"[RESULT] {json.dumps(result, indent=2)}")
                    return result
                elif "Unknown" not in str(result.get("error", "")):
                    print(f"[RESPONSE] {cmd.get('type', 'custom')}: {response[:300]}")
            except asyncio.TimeoutError:
                pass
            except Exception as e:
                print(f"[ERROR] {e}")

        print("[INFO] Could not find working command format")
        return None


async def main():
    # Test with simple code
    code = """
import hou
print("Synapse test successful!")
print(f"Houdini version: {hou.applicationVersionString()}")
"""
    await execute_in_houdini(code)


if __name__ == "__main__":
    asyncio.run(main())
