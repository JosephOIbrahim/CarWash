#!/usr/bin/env python
"""Discover Synapse bridge protocol."""

import asyncio
import json

try:
    import websockets
except ImportError:
    import subprocess
    subprocess.run(["pip", "install", "websockets"], check=True)
    import websockets


async def discover_protocol():
    uri = "ws://localhost:9999"

    async with websockets.connect(uri) as ws:
        # execute_python exists but needs different params
        # Try different parameter names
        code = "print('test')"

        param_names = ["python", "script", "source", "expression", "command", "exec", "content", "body", "data", "text"]

        for param in param_names:
            cmd = {"type": "execute_python", param: code}
            try:
                await ws.send(json.dumps(cmd))
                response = await asyncio.wait_for(ws.recv(), timeout=2.0)
                result = json.loads(response)
                print(f"[{param}] success={result.get('success')} error={result.get('error', 'none')[:80]}")
                if result.get("success"):
                    print(f"   FOUND IT! Use: {param}")
                    return param
            except asyncio.TimeoutError:
                print(f"[{param}] timeout")
            except Exception as e:
                print(f"[{param}] error: {e}")

        # Also try to get help/schema
        help_cmds = [
            {"type": "schema"},
            {"type": "get_schema"},
            {"type": "help", "topic": "execute_python"},
            {"type": "describe", "command": "execute_python"},
        ]

        print("\n--- Trying help commands ---")
        for cmd in help_cmds:
            try:
                await ws.send(json.dumps(cmd))
                response = await asyncio.wait_for(ws.recv(), timeout=2.0)
                result = json.loads(response)
                if result.get("success"):
                    print(f"[{cmd.get('type')}] {json.dumps(result, indent=2)[:500]}")
            except:
                pass

        return None


if __name__ == "__main__":
    asyncio.run(discover_protocol())
