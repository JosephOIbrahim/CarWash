"""
Quick CarWash Connection Test
=============================
Tests basic Synapse connectivity and built-in commands.
"""

import asyncio
import json

try:
    import websockets
except ImportError:
    import subprocess
    subprocess.run(["pip", "install", "websockets"], check=True)
    import websockets

SYNAPSE_URI = "ws://localhost:9999"


async def send_command(ws, cmd_type: str, params: dict = None) -> dict:
    """Send a command to Synapse and get response."""
    msg = {"type": cmd_type}
    if params:
        msg.update(params)
    await ws.send(json.dumps(msg))
    response = await asyncio.wait_for(ws.recv(), timeout=30)
    return json.loads(response)


async def main():
    print("=" * 50)
    print("CarWash Quick Connection Test")
    print("=" * 50)

    try:
        async with websockets.connect(SYNAPSE_URI, close_timeout=5) as ws:
            # Test ping
            print("\n[1] Testing connection...")
            resp = await send_command(ws, "ping")
            print(f"    Ping: {'OK' if resp.get('success') else 'FAIL'}")

            # Get scene info
            print("\n[2] Getting scene info...")
            resp = await send_command(ws, "scene_info")
            if resp.get("success"):
                data = resp.get("data", {})
                print(f"    Scene: {data.get('hip', 'unknown')}")
                print(f"    Frame: {data.get('frame', 0)}")

            # Check for CarWash (new command)
            print("\n[3] Checking CarWash plugin...")
            resp = await send_command(ws, "check_carwash")
            if resp.get("success"):
                data = resp.get("data", {})
                print(f"    Plugin found: {data.get('found', False)}")
                print(f"    Schema found: {data.get('schema_found', False)}")
                for p in data.get("plugins", []):
                    print(f"    - {p['name']} (loaded: {p['loaded']})")
            else:
                print(f"    Note: {resp.get('error')}")
                print("    (Synapse may need restart to get new commands)")

            # List renderers (new command)
            print("\n[4] Listing available renderers...")
            resp = await send_command(ws, "list_renderers")
            if resp.get("success"):
                renderers = resp.get("data", {}).get("renderers", [])
                print(f"    Found {len(renderers)} renderers:")
                for r in renderers:
                    marker = " <-- CarWash" if "carwash" in r.lower() else ""
                    print(f"    - {r}{marker}")
            else:
                print(f"    Note: {resp.get('error')}")

            # Build scene
            print("\n[5] Building test scene...")
            resp = await send_command(ws, "build_carwash_scene")
            if resp.get("success"):
                print(f"    {resp.get('data')}")
            else:
                print(f"    Error: {resp.get('error')}")

            print("\n" + "=" * 50)
            print("Test complete!")
            print("=" * 50)

    except ConnectionRefusedError:
        print("\nERROR: Cannot connect to Synapse")
        print("\nStart Synapse in Houdini Python Shell:")
        print("  import runpy")
        print('  runpy.run_path(r"C:\\Users\\User\\CARWASH\\houdini\\synapse_server.py")')


if __name__ == "__main__":
    asyncio.run(main())
