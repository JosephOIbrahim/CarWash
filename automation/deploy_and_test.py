r"""
CarWash Deployment and Testing via Synapse
==========================================
Connects to Houdini via ws://localhost:9999 and:
1. Deploys updated schema files
2. Tests CarWash renderer registration
3. Verifies Render Settings tab availability

Prerequisites:
    - Synapse server running in Houdini Python Shell:
      import runpy
      runpy.run_path(r"C:\Users\User\Downloads\HDCARWAASH\HdCarWash\houdini\synapse_server.py")
"""

import asyncio
import json
import shutil
import os
from pathlib import Path

try:
    import websockets
except ImportError:
    import subprocess
    subprocess.run(["pip", "install", "websockets"], check=True)
    import websockets


# Paths
SOURCE_DIR = Path(r"C:\Users\User\Downloads\HDCARWAASH\HdCarWash")
HOUDINI_USD_DIR = Path(r"C:\Users\User\houdini21.0\dso\usd")
SYNAPSE_URI = "ws://localhost:9999"


async def deploy_schema_files():
    """Deploy the two-plugin structure (hdCarWash + usdCarWash) to Houdini."""
    print("\n=== Deploying CarWash Plugins ===")

    # Target directories
    hd_carwash_dir = HOUDINI_USD_DIR / "hdCarWash"
    usd_carwash_dir = HOUDINI_USD_DIR / "usdCarWash"

    hd_resources = hd_carwash_dir / "resources"
    usd_resources = usd_carwash_dir / "resources"

    # Create all directories
    for d in [hd_resources, usd_resources]:
        d.mkdir(parents=True, exist_ok=True)

    # 1. Deploy top-level plugInfo.json (includes both plugins)
    top_pluginfo = HOUDINI_USD_DIR / "plugInfo.json"
    with open(top_pluginfo, 'w') as f:
        json.dump({"Includes": ["*/resources/"]}, f, indent=4)
    print(f"  Created: {top_pluginfo}")

    # 2. Deploy hdCarWash plugInfo.json (render delegate only)
    hd_pluginfo_src = SOURCE_DIR / "plugin" / "plugInfo.json"
    hd_pluginfo_dst = hd_resources / "plugInfo.json"
    if hd_pluginfo_src.exists():
        shutil.copy2(hd_pluginfo_src, hd_pluginfo_dst)
        print(f"  Copied: plugin/plugInfo.json -> {hd_pluginfo_dst}")
    else:
        print(f"  MISSING: {hd_pluginfo_src}")

    # 3. Deploy usdCarWash plugInfo.json (schema only)
    usd_pluginfo_src = SOURCE_DIR / "usdCarWash" / "resources" / "plugInfo.json"
    usd_pluginfo_dst = usd_resources / "plugInfo.json"
    if usd_pluginfo_src.exists():
        shutil.copy2(usd_pluginfo_src, usd_pluginfo_dst)
        print(f"  Copied: usdCarWash/resources/plugInfo.json -> {usd_pluginfo_dst}")
    else:
        print(f"  MISSING: {usd_pluginfo_src}")

    # 4. Deploy generatedSchema.usda to usdCarWash
    schema_src = SOURCE_DIR / "usdCarWash" / "resources" / "generatedSchema.usda"
    schema_dst = usd_resources / "generatedSchema.usda"
    if schema_src.exists():
        shutil.copy2(schema_src, schema_dst)
        print(f"  Copied: generatedSchema.usda -> {schema_dst}")
    else:
        # Fall back to schema directory
        schema_src = SOURCE_DIR / "schema" / "generatedSchema.usda"
        if schema_src.exists():
            shutil.copy2(schema_src, schema_dst)
            print(f"  Copied: schema/generatedSchema.usda -> {schema_dst}")
        else:
            print(f"  MISSING: generatedSchema.usda")

    print("\nPlugin deployment complete.")
    print("  hdCarWash: Render delegate (HdRendererPlugin)")
    print("  usdCarWash: Schema (CarWashRenderSettingsAPI)")
    return True


async def send_command(ws, cmd_type: str, params: dict = None) -> dict:
    """Send a command to Synapse and get response."""
    msg = {"type": cmd_type}
    if params:
        msg.update(params)

    await ws.send(json.dumps(msg))
    response = await asyncio.wait_for(ws.recv(), timeout=30)
    return json.loads(response)


async def test_connection(ws) -> bool:
    """Test Synapse connection."""
    print("\n=== Testing Synapse Connection ===")
    resp = await send_command(ws, "ping")
    if resp.get("success"):
        print("  Connected to Houdini!")
        return True
    else:
        print("  Connection failed!")
        return False


async def test_plugin_registration(ws) -> dict:
    """Test if CarWash plugin is registered."""
    print("\n=== Testing Plugin Registration ===")

    # Execute Python in Houdini to check plugin
    code = '''
import pxr.Plug
results = {"found": False, "plugins": [], "renderer_available": False}

# Find CarWash plugins
for plugin in pxr.Plug.Registry().GetAllPlugins():
    name_lower = plugin.name.lower()
    if 'carwash' in name_lower:
        results["plugins"].append({
            "name": plugin.name,
            "path": str(plugin.path),
            "loaded": plugin.isLoaded
        })
        results["found"] = True

# Check if renderer is available
try:
    import hou
    stage = hou.node("/stage")
    if stage:
        rs = stage.createNode("rendersettings", "carwash_test_rs")
        menu_items = rs.parm("renderer").menuItems()
        results["available_renderers"] = list(menu_items)
        results["renderer_available"] = any("carwash" in r.lower() for r in menu_items)
        rs.destroy()
except Exception as e:
    results["error"] = str(e)

results
'''

    resp = await send_command(ws, "run_code", {"code": code})

    if resp.get("success"):
        data = resp.get("data", {})
        print(f"  Plugin found: {data.get('found', False)}")
        for p in data.get("plugins", []):
            print(f"    - {p['name']} (loaded: {p['loaded']})")
        print(f"  Renderer available: {data.get('renderer_available', False)}")
        if data.get("available_renderers"):
            print(f"  Available renderers: {data.get('available_renderers')}")
        return data
    else:
        print(f"  ERROR: {resp.get('error')}")
        return {"error": resp.get("error")}


async def test_schema_registration(ws) -> dict:
    """Test if CarWashRenderSettingsAPI schema is registered."""
    print("\n=== Testing Schema Registration ===")

    code = '''
from pxr import Usd, Plug
results = {"schema_found": False, "types": []}

# Look for CarWash schema types
registry = Plug.Registry()
for plugin in registry.GetAllPlugins():
    if 'carwash' in plugin.name.lower():
        # Get types from this plugin
        try:
            plugin.Load()
            info = plugin.metadata.get("Info", {})
            types = info.get("Types", {})
            for type_name, type_info in types.items():
                results["types"].append({
                    "name": type_name,
                    "bases": type_info.get("bases", []),
                    "schemaKind": type_info.get("schemaKind", "")
                })
                if "RenderSettingsAPI" in type_name:
                    results["schema_found"] = True
        except Exception as e:
            results["load_error"] = str(e)

results
'''

    resp = await send_command(ws, "run_code", {"code": code})

    if resp.get("success"):
        data = resp.get("data", {})
        print(f"  Schema found: {data.get('schema_found', False)}")
        for t in data.get("types", []):
            print(f"    - {t['name']}")
            if t.get("schemaKind"):
                print(f"      schemaKind: {t['schemaKind']}")
        return data
    else:
        print(f"  ERROR: {resp.get('error')}")
        return {"error": resp.get("error")}


async def build_test_scene(ws) -> dict:
    """Build a test scene with CarWash renderer."""
    print("\n=== Building Test Scene ===")

    resp = await send_command(ws, "build_carwash_scene")

    if resp.get("success"):
        data = resp.get("data", {})
        print(f"  Scene built: {data}")
        return data
    else:
        print(f"  ERROR: {resp.get('error')}")
        return {"error": resp.get("error")}


async def check_render_settings_tab(ws) -> dict:
    """Check if CarWash tab appears in Render Settings."""
    print("\n=== Checking Render Settings Tab ===")

    code = '''
import hou
results = {"tab_found": False, "parms": [], "error": None}

try:
    # Find or create render settings node
    stage = hou.node("/stage")
    rs_nodes = [n for n in stage.children() if n.type().name() == "rendersettings"]

    if rs_nodes:
        rs = rs_nodes[0]
    else:
        rs = stage.createNode("rendersettings", "carwash_rs_test")

    # Set renderer to CarWash if available
    renderer_parm = rs.parm("renderer")
    menu_items = renderer_parm.menuItems()

    carwash_renderer = None
    for item in menu_items:
        if "carwash" in item.lower():
            carwash_renderer = item
            break

    if carwash_renderer:
        renderer_parm.set(carwash_renderer)
        results["renderer_set"] = carwash_renderer

        # Look for CarWash-specific parameters
        for parm in rs.parms():
            parm_name = parm.name()
            if "carwash" in parm_name.lower():
                results["parms"].append({
                    "name": parm_name,
                    "label": parm.description(),
                    "value": str(parm.rawValue())
                })
                results["tab_found"] = True
    else:
        results["error"] = "CarWash renderer not found in menu"
        results["available"] = list(menu_items)

except Exception as e:
    results["error"] = str(e)

results
'''

    resp = await send_command(ws, "run_code", {"code": code})

    if resp.get("success"):
        data = resp.get("data", {})
        print(f"  Tab found: {data.get('tab_found', False)}")
        if data.get("renderer_set"):
            print(f"  Renderer set to: {data.get('renderer_set')}")
        if data.get("parms"):
            print(f"  CarWash parameters found: {len(data.get('parms'))}")
            for p in data.get("parms")[:5]:  # Show first 5
                print(f"    - {p['name']}: {p['value']}")
        if data.get("error"):
            print(f"  Note: {data.get('error')}")
        return data
    else:
        print(f"  ERROR: {resp.get('error')}")
        return {"error": resp.get("error")}


async def reload_plugins(ws) -> dict:
    """Attempt to reload USD plugins."""
    print("\n=== Reloading USD Plugins ===")

    code = '''
import pxr.Plug
results = {"reloaded": False}

try:
    # Note: Full plugin reload typically requires Houdini restart
    # But we can try to load any unloaded plugins
    registry = pxr.Plug.Registry()
    for plugin in registry.GetAllPlugins():
        if 'carwash' in plugin.name.lower() and not plugin.isLoaded:
            plugin.Load()
            results["reloaded"] = True
            results["loaded_plugin"] = plugin.name

    if not results["reloaded"]:
        results["note"] = "Plugins already loaded or restart may be required"

except Exception as e:
    results["error"] = str(e)

results
'''

    resp = await send_command(ws, "run_code", {"code": code})

    if resp.get("success"):
        data = resp.get("data", {})
        print(f"  Reloaded: {data.get('reloaded', False)}")
        if data.get("note"):
            print(f"  Note: {data.get('note')}")
        return data
    else:
        print(f"  ERROR: {resp.get('error')}")
        return {"error": resp.get("error")}


async def main():
    """Main deployment and testing sequence."""
    print("=" * 60)
    print("CarWash Deployment and Testing")
    print("=" * 60)

    # Step 1: Deploy schema files (local operation)
    await deploy_schema_files()

    # Step 2: Connect to Synapse
    print(f"\nConnecting to Synapse at {SYNAPSE_URI}...")

    try:
        async with websockets.connect(SYNAPSE_URI, close_timeout=5) as ws:
            # Test connection
            if not await test_connection(ws):
                return

            # Get scene info
            resp = await send_command(ws, "scene_info")
            if resp.get("success"):
                print(f"  Scene: {resp.get('data', {}).get('hip_path', 'untitled')}")

            # Reload plugins
            await reload_plugins(ws)

            # Test plugin registration
            await test_plugin_registration(ws)

            # Test schema registration
            await test_schema_registration(ws)

            # Build test scene
            await build_test_scene(ws)

            # Check for Render Settings tab
            await check_render_settings_tab(ws)

            print("\n" + "=" * 60)
            print("Deployment and Testing Complete")
            print("=" * 60)
            print("\nNOTE: If CarWash tab doesn't appear, restart Houdini")
            print("      to fully reload USD plugins.")

    except ConnectionRefusedError:
        print("\nERROR: Cannot connect to Synapse at ws://localhost:9999")
        print("\nMake sure Synapse server is running in Houdini:")
        print("  1. Open Houdini")
        print("  2. Windows > Python Shell")
        print("  3. Run:")
        print('     import runpy')
        print('     runpy.run_path(r"C:\\Users\\User\\Downloads\\HDCARWAASH\\HdCarWash\\houdini\\synapse_server.py")')
    except Exception as e:
        print(f"\nERROR: {e}")


if __name__ == "__main__":
    asyncio.run(main())
