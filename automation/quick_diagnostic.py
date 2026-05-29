"""
Quick CarWash Diagnostic - Run in Houdini Python Shell
======================================================
Copy and paste this entire script into Houdini's Python Shell.

IMPORTANT: Restart Houdini AFTER updating package files!
"""

import hou
import os

print("\n" + "=" * 60)
print("  CarWash Quick Diagnostic v2")
print("=" * 60)

# 0. Show expected vs actual paths
print("\n0. Path Configuration:")
expected_path = os.path.expandvars("$HOUDINI_USER_PREF_DIR/dso/usd")
print("   Expected PXR_PLUGINPATH_NAME to include:")
print("     " + expected_path)
actual_path = os.environ.get('PXR_PLUGINPATH_NAME', '(not set)')
if expected_path.replace('\\', '/') in actual_path.replace('\\', '/'):
    print("   STATUS: CORRECT - Path includes expected location")
else:
    print("   STATUS: WRONG - Expected path NOT found!")
    print("   Did you restart Houdini AFTER updating hdCarWash.json?")

# 1. Check UsdRenderers.json
print("\n1. UsdRenderers.json Discovery:")
try:
    files = hou.findFiles('UsdRenderers.json')
    for f in files:
        print("   - " + f)
        try:
            with open(f, 'r') as fp:
                if 'carwash' in fp.read().lower():
                    print("     ^ Contains CarWash!")
        except:
            pass
except Exception as e:
    print("   Error: " + str(e))

# 2. Check USD Plugin Registry
print("\n2. USD Plugin Registry:")
try:
    import pxr.Plug
    registry = pxr.Plug.Registry()
    found = False
    for plugin in registry.GetAllPlugins():
        if 'carwash' in plugin.name.lower():
            found = True
            print("   FOUND: " + plugin.name)
            print("     Path: " + str(plugin.path))
            print("     Loaded: " + str(plugin.isLoaded))
    if not found:
        print("   NO CarWash plugins found!")
except Exception as e:
    print("   Error: " + str(e))

# 3. Check Hydra Renderer Plugins
print("\n3. Hydra Renderer Plugins:")
try:
    from pxr import Hd
    registry = Hd.RendererPluginRegistry()
    plugin_ids = registry.GetRegisteredPlugins()
    found = False
    for pid in plugin_ids:
        if 'carwash' in str(pid).lower():
            found = True
            print("   FOUND: " + str(pid))
        else:
            print("   - " + str(pid))
    if not found:
        print("   CarWash NOT in renderer list!")
except Exception as e:
    print("   Error: " + str(e))

# 4. Check Schema Types
print("\n4. Schema Types:")
try:
    from pxr import Tf
    for name in ['CarWashRenderSettingsAPI', 'usdCarWashCarWashRenderSettingsAPI']:
        tf_type = Tf.Type.FindByName(name)
        if tf_type and not tf_type.isUnknown:
            print("   FOUND: " + name)
        else:
            print("   NOT FOUND: " + name)
except Exception as e:
    print("   Error: " + str(e))

# 5. Check Render Settings LOP Parameters
print("\n5. Render Settings LOP:")
try:
    stage = hou.node('/stage')
    if not stage:
        stage = hou.node('/').createNode('stage')

    rs = stage.createNode('rendersettings', '_diag_check')
    menu_items = list(rs.parm('renderer').menuItems())
    menu_labels = list(rs.parm('renderer').menuLabels())

    print("   Available renderers:")
    carwash_found = False
    for item, label in zip(menu_items, menu_labels):
        if 'carwash' in item.lower():
            carwash_found = True
            print("   * " + item + ": " + label + " <-- CARWASH")
        else:
            print("   - " + item + ": " + label)

    if carwash_found:
        # Set to CarWash and check parameters
        carwash_id = [i for i in menu_items if 'carwash' in i.lower()][0]
        rs.parm('renderer').set(carwash_id)

        print("\n   CarWash-specific parameters:")
        parm_count = 0
        for parm in rs.parms():
            if 'carwash' in parm.name().lower():
                parm_count += 1
                print("   - " + parm.name())
        if parm_count == 0:
            print("   NONE found (this is the problem!)")
    else:
        print("\n   CarWash renderer NOT in menu!")

    rs.destroy()
except Exception as e:
    print("   Error: " + str(e))
    import traceback
    traceback.print_exc()

# 6. Check PXR_PLUGINPATH_NAME
print("\n6. PXR_PLUGINPATH_NAME:")
path = os.environ.get('PXR_PLUGINPATH_NAME', '(not set)')
for p in path.split(';'):
    if p:
        print("   - " + p)
        if 'carwash' in p.lower():
            print("     ^ CarWash path!")

print("\n" + "=" * 60)
print("  Diagnostic Complete")
print("=" * 60)

# Summary
print("\n  SUMMARY:")
path_ok = expected_path.replace('\\', '/') in actual_path.replace('\\', '/') if 'expected_path' in dir() else False
hdplugin_ok = 'found' in dir() and found  # from section 2
usdplugin_found = False
try:
    import pxr.Plug
    for plugin in pxr.Plug.Registry().GetAllPlugins():
        if plugin.name == 'usdCarWash':
            usdplugin_found = True
except:
    pass

if not path_ok:
    print("  [X] PXR_PLUGINPATH_NAME wrong - restart Houdini!")
else:
    print("  [OK] PXR_PLUGINPATH_NAME correct")

if usdplugin_found:
    print("  [OK] usdCarWash plugin found")
else:
    print("  [X] usdCarWash plugin NOT found")

if carwash_found if 'carwash_found' in dir() else False:
    print("  [OK] CarWash renderer in menu")
    if parm_count if 'parm_count' in dir() else 0 > 0:
        print("  [OK] CarWash parameters visible!")
    else:
        print("  [X] CarWash parameters NOT visible - schema issue")
else:
    print("  [X] CarWash NOT in renderer menu")
