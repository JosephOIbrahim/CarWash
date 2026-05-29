#!/usr/bin/env python3
"""
CarWash Renderer Synapse Setup Script
Connects to Houdini via Synapse and runs the test scene setup
"""

import asyncio
import websockets
import json
import sys

SYNAPSE_URL = "ws://localhost:9999"

SETUP_CODE = '''
import hou

def setup_hdcarwash_test():
    stage = hou.node('/stage')
    if not stage:
        stage = hou.node('/').createNode('stage', 'stage')

    for name in ['hdcarwash_sphere', 'hdcarwash_camera', 'hdcarwash_light', 'hdcarwash_render']:
        n = stage.node(name)
        if n:
            n.destroy()

    sphere = stage.createNode('sphere', 'hdcarwash_sphere')
    sphere.parm('rows').set(32)
    sphere.parm('cols').set(32)

    camera = stage.createNode('camera', 'hdcarwash_camera')
    camera.setInput(0, sphere)
    camera.parm('tx').set(0)
    camera.parm('ty').set(0)
    camera.parm('tz').set(5)

    light = stage.createNode('distantlight', 'hdcarwash_light')
    light.setInput(0, camera)
    light.parm('ry').set(-45)
    light.parm('rx').set(-30)

    render_settings = stage.createNode('karmarendersettings', 'hdcarwash_rendersettings')
    render_settings.setInput(0, light)
    render_settings.parm('renderer').set('HdCarWash')

    usd_render = stage.createNode('usdrender', 'hdcarwash_render')
    usd_render.setInput(0, render_settings)
    usd_render.parm('outputimage').set('$HIP/render/hdcarwash_test.$F4.exr')

    stage.layoutChildren()
    usd_render.setDisplayFlag(True)

    print("CarWash Renderer test scene created!")
    return usd_render

setup_hdcarwash_test()
'''

async def main():
    print("Connecting to Synapse at", SYNAPSE_URL)

    try:
        async with websockets.connect(SYNAPSE_URL, ping_timeout=5) as ws:
            print("[OK] Connected to Synapse")

            # Try execute_python command
            cmd = {
                "type": "execute_python",
                "code": SETUP_CODE
            }

            print("Sending setup script...")
            await ws.send(json.dumps(cmd))

            # Wait for response
            try:
                response = await asyncio.wait_for(ws.recv(), timeout=10)
                data = json.loads(response)
                print(f"Response: {json.dumps(data, indent=2)}")

                if data.get("success") or data.get("status") == "success":
                    print("[OK] Setup completed!")
                    return 0
                else:
                    print(f"[WARN] Response: {data}")
                    # Try alternative: direct python execution

            except asyncio.TimeoutError:
                print("[WARN] No response (might still work)")

    except ConnectionRefusedError:
        print("[FAIL] Cannot connect - is Houdini running with Synapse?")
        return 1
    except Exception as e:
        print(f"[FAIL] Error: {e}")
        return 1

    return 0

if __name__ == "__main__":
    sys.exit(asyncio.run(main()))
