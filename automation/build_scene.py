r"""
Build CarWash scene via Synapse
===============================
Requires: Synapse server running in Houdini (port 9999)

Step 1: In Houdini Python Shell run:
    import runpy
    runpy.run_path(r"C:\Users\User\CARWASH\houdini\synapse_server.py")

Step 2: Run this script externally:
    python build_scene.py
"""

import asyncio
import json

try:
    import websockets
except ImportError:
    import subprocess
    subprocess.run(["pip", "install", "websockets"], check=True)
    import websockets


async def build_scene():
    uri = "ws://localhost:9999"

    print("Connecting to Synapse...")
    try:
        async with websockets.connect(uri, close_timeout=5) as ws:
            # Test connection
            await ws.send(json.dumps({"type": "ping"}))
            resp = json.loads(await asyncio.wait_for(ws.recv(), timeout=5))
            if resp.get("success"):
                print("Connected to Houdini!")
            else:
                print("Connection failed")
                return

            # Build scene
            print("Building CarWash test scene...")
            await ws.send(json.dumps({"type": "build_carwash_scene"}))
            resp = json.loads(await asyncio.wait_for(ws.recv(), timeout=10))

            if resp.get("success"):
                print("SUCCESS:", resp.get("data"))
            else:
                print("ERROR:", resp.get("error"))

    except ConnectionRefusedError:
        print("ERROR: Cannot connect to ws://localhost:9999")
        print("")
        print("Make sure Synapse server is running in Houdini:")
        print("  1. Open Houdini")
        print("  2. Windows > Python Shell")
        print("  3. Run:")
        print('     import runpy')
        print('     runpy.run_path(r"C:\\Users\\User\\CARWASH\\houdini\\synapse_server.py")')


if __name__ == "__main__":
    asyncio.run(build_scene())
