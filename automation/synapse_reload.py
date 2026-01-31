#!/usr/bin/env python
"""Reload HdCarWash plugin via Synapse."""

import asyncio
import json

try:
    import websockets
except ImportError:
    import subprocess
    subprocess.run(["pip", "install", "websockets"], check=True)
    import websockets


async def reload_plugin():
    uri = "ws://localhost:9999"

    async with websockets.connect(uri) as ws:
        print("Connected to Synapse")

        # Check scene first
        code = """
import hou

result = []
stage = hou.node('/stage')
if stage:
    result.append('Stage: ' + stage.path())
    for child in stage.children():
        result.append('  ' + child.name() + ' (' + child.type().name() + ')')
else:
    result.append('No stage node')

# Check for sopimport
sopimport = hou.node('/stage/sopimport1')
if sopimport:
    result.append('SOP Import found: ' + sopimport.path())

'|'.join(result)
"""
        await ws.send(json.dumps({"type": "execute_python", "payload": {"code": code}}))
        response = await asyncio.wait_for(ws.recv(), timeout=5.0)
        result = json.loads(response)

        if result.get("success"):
            data = result.get("data", {}).get("result", "")
            if data:
                for line in data.split("|"):
                    print(line)
        else:
            print("Error:", result.get("error"))

        # Suggest restart
        print("\n" + "=" * 50)
        print("To update the plugin, please:")
        print("1. Save your scene (if needed)")
        print("2. Close Houdini")
        print("3. Run: python synapse_reload.py --install")
        print("4. Reopen Houdini")
        print("=" * 50)


async def install_and_notify():
    """Copy DLL after Houdini is closed."""
    import shutil
    import os

    src = r"C:\Users\User\Downloads\HDCARWAASH\HdCarWash\build\plugin\hdCarWash\Release\hdCarWash.dll"
    dst = r"C:\Users\User\houdini21.0\dso\usd\hdCarWash\lib\hdCarWash.dll"

    if not os.path.exists(src):
        print(f"ERROR: Source DLL not found: {src}")
        return False

    try:
        shutil.copy2(src, dst)
        print(f"SUCCESS: Copied {src}")
        print(f"     to: {dst}")
        return True
    except PermissionError:
        print("ERROR: DLL is locked. Close Houdini first.")
        return False
    except Exception as e:
        print(f"ERROR: {e}")
        return False


if __name__ == "__main__":
    import sys

    if "--install" in sys.argv:
        success = asyncio.run(asyncio.to_thread(install_and_notify)) if hasattr(asyncio, 'to_thread') else install_and_notify()
        if not isinstance(success, bool):
            # Direct call for older Python
            import shutil
            src = r"C:\Users\User\Downloads\HDCARWAASH\HdCarWash\build\plugin\hdCarWash\Release\hdCarWash.dll"
            dst = r"C:\Users\User\houdini21.0\dso\usd\hdCarWash\lib\hdCarWash.dll"
            try:
                shutil.copy2(src, dst)
                print(f"SUCCESS: Plugin updated!")
            except PermissionError:
                print("ERROR: Close Houdini first")
            except Exception as e:
                print(f"ERROR: {e}")
    else:
        asyncio.run(reload_plugin())
