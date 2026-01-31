#!/usr/bin/env python
"""Test Synapse bridge connection and discover available commands."""

import asyncio
import json

try:
    import websockets
except ImportError:
    print("Installing websockets...")
    import subprocess
    subprocess.run(["pip", "install", "websockets"], check=True)
    import websockets


async def test_synapse():
    uri = "ws://localhost:9999"

    try:
        async with websockets.connect(uri) as ws:
            print(f"[OK] Connected to {uri}")

            # Test ping
            await ws.send(json.dumps({"type": "ping"}))
            response = await ws.recv()
            print(f"[PING] {response}")

            # Try help/info commands
            test_commands = [
                {"type": "help"},
                {"type": "info"},
                {"type": "list"},
                {"type": "commands"},
                {"type": "capabilities"},
                {"type": "run", "code": "print('hello')"},
                {"type": "exec", "code": "print('hello')"},
                {"type": "python", "code": "print('hello')"},
                {"action": "execute", "code": "print('hello')"},
                {"command": "python", "code": "print('hello')"},
                {"method": "execute", "params": {"code": "print('hello')"}},
            ]

            for cmd in test_commands:
                try:
                    await ws.send(json.dumps(cmd))
                    response = await asyncio.wait_for(ws.recv(), timeout=2.0)
                    result = json.loads(response)
                    if result.get("success") or "error" not in str(result).lower():
                        print(f"[TRY] {cmd.get('type', cmd)} -> {response[:200]}")
                except asyncio.TimeoutError:
                    print(f"[TIMEOUT] {cmd}")
                except Exception as e:
                    print(f"[ERROR] {cmd}: {e}")

    except ConnectionRefusedError:
        print(f"[ERROR] Cannot connect to {uri} - Synapse not running?")
    except Exception as e:
        print(f"[ERROR] {e}")


if __name__ == "__main__":
    asyncio.run(test_synapse())
