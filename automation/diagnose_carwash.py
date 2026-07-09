r"""
CarWash Plugin Diagnostic Script
================================
Run directly in Houdini Python Shell to diagnose why render settings tab
is not appearing.

Usage in Houdini Python Shell:
    import runpy
    runpy.run_path(r"C:\\Users\\User\\CARWASH\\automation\\diagnose_carwash.py")
"""

import hou
import os
import sys

def divider(title):
    print("\n" + "=" * 60)
    print("  " + title)
    print("=" * 60)

def check_usdrenderers():
    """Check if UsdRenderers.json is found"""
    divider("1. UsdRenderers.json Discovery")

    try:
        files = hou.findFiles('UsdRenderers.json')
        print("Found " + str(len(files)) + " UsdRenderers.json file(s):")
        for f in files:
            print("  - " + f)
            # Check if CarWash is in this file
            try:
                with open(f, 'r') as fp:
                    content = fp.read()
                    if 'CarWash' in content or 'carwash' in content.lower():
                        print("    ^ Contains CarWash entry!")
            except:
                pass
    except Exception as e:
        print("ERROR: " + str(e))

def check_plugin_registry():
    """Check USD plugin registry"""
    divider("2. USD Plugin Registry")

    try:
        import pxr.Plug
        registry = pxr.Plug.Registry()

        carwash_plugins = []
        for plugin in registry.GetAllPlugins():
            name_lower = plugin.name.lower()
            if 'carwash' in name_lower:
                carwash_plugins.append(plugin)
                print("Found CarWash plugin: " + plugin.name)
                print("  Path: " + str(plugin.path))
                print("  Loaded: " + str(plugin.isLoaded))

                # Try to load if not loaded
                if not plugin.isLoaded:
                    try:
                        plugin.Load()
                        print("  -> Loaded successfully!")
                    except Exception as e:
                        print("  -> Load failed: " + str(e))

                # Get metadata
                try:
                    meta = plugin.metadata
                    if 'Info' in meta:
                        info = meta['Info']
                        print("  Types: " + str(list(info.get('Types', {}).keys())))
                except Exception as e:
                    print("  (Metadata error: " + str(e) + ")")

        if not carwash_plugins:
            print("NO CarWash plugins found in registry!")
            print("\nAll registered plugins with 'hd' in name:")
            for plugin in registry.GetAllPlugins():
                if 'hd' in plugin.name.lower():
                    print("  - " + plugin.name)

    except Exception as e:
        print("ERROR: " + str(e))
        import traceback
        traceback.print_exc()

def check_renderer_plugins():
    """Check registered Hydra renderer plugins"""
    divider("3. Hydra Renderer Plugins")

    try:
        import pxr.Plug
        from pxr import Hd

        # Get render delegate registry
        registry = Hd.RendererPluginRegistry()
        plugin_ids = registry.GetRegisteredPlugins()

        print("Registered renderer plugins (" + str(len(plugin_ids)) + "):")
        for pid in plugin_ids:
            marker = " <-- CARWASH" if 'carwash' in str(pid).lower() else ""
            print("  - " + str(pid) + marker)

    except Exception as e:
        print("ERROR: " + str(e))
        import traceback
        traceback.print_exc()

def check_schema_registry():
    """Check if CarWashRenderSettingsAPI schema is registered"""
    divider("4. Schema Registry")

    try:
        import pxr.Plug
        from pxr import Tf, Usd

        # Try to get the schema type
        type_names = [
            "CarWashRenderSettingsAPI",
            "usdCarWashCarWashRenderSettingsAPI",
        ]

        for type_name in type_names:
            try:
                tf_type = Tf.Type.FindByName(type_name)
                if tf_type and not tf_type.isUnknown:
                    print("Found schema type: " + type_name)
                    print("  TfType: " + str(tf_type))
                else:
                    print("NOT found: " + type_name)
            except Exception as e:
                print("Error checking " + type_name + ": " + str(e))

        # List all API schemas that can apply to RenderSettings
        print("\nAPI schemas that can apply to RenderSettings:")
        registry = pxr.Plug.Registry()
        for plugin in registry.GetAllPlugins():
            try:
                if not plugin.isLoaded:
                    continue
                meta = plugin.metadata
                if 'Info' not in meta:
                    continue
                types = meta['Info'].get('Types', {})
                for tname, tinfo in types.items():
                    applies_to = tinfo.get('apiSchemaCanOnlyApplyTo', [])
                    if 'RenderSettings' in applies_to:
                        print("  - " + tname + " (from " + plugin.name + ")")
            except:
                pass

    except Exception as e:
        print("ERROR: " + str(e))
        import traceback
        traceback.print_exc()

def check_rendersettings_lop():
    """Check what renderers are available in Render Settings LOP"""
    divider("5. Render Settings LOP")

    try:
        stage = hou.node("/stage")
        if not stage:
            print("No /stage context found. Creating...")
            stage = hou.node("/").createNode("stage")

        # Create a temporary render settings node
        rs = stage.createNode("rendersettings", "_carwash_diag_rs")

        # Get renderer parameter
        renderer_parm = rs.parm("renderer")
        menu_items = renderer_parm.menuItems()
        menu_labels = renderer_parm.menuLabels()

        print("Available renderers (" + str(len(menu_items)) + "):")
        for item, label in zip(menu_items, menu_labels):
            marker = " <-- CARWASH" if 'carwash' in item.lower() or 'carwash' in label.lower() else ""
            print("  - " + item + ": " + label + marker)

        # Try to set CarWash renderer
        carwash_renderers = [r for r in menu_items if 'carwash' in r.lower()]
        if carwash_renderers:
            print("\nSetting renderer to: " + carwash_renderers[0])
            renderer_parm.set(carwash_renderers[0])

            # Check for CarWash-specific parameters
            print("\nParameters with 'carwash' in name:")
            found_any = False
            for parm in rs.parms():
                if 'carwash' in parm.name().lower():
                    found_any = True
                    print("  - " + parm.name() + ": " + str(parm.rawValue()))

            if not found_any:
                print("  (none found)")

            # Check for parameter folders/tabs
            print("\nParameter groups/folders:")
            parm_template_group = rs.parmTemplateGroup()
            def show_folders(entries, indent=0):
                for entry in entries:
                    if hasattr(entry, 'name'):
                        prefix = "  " * indent
                        marker = " <-- CHECK" if 'carwash' in entry.name().lower() else ""
                        print(prefix + "- " + entry.name() + ": " + entry.label() + marker)
                    if hasattr(entry, 'parmTemplates'):
                        show_folders(entry.parmTemplates(), indent + 1)
            show_folders(parm_template_group.entries())
        else:
            print("\nCarWash renderer not found in menu!")

        # Cleanup
        rs.destroy()

    except Exception as e:
        print("ERROR: " + str(e))
        import traceback
        traceback.print_exc()

def check_render_delegate_settings():
    """Check render delegate GetRenderSettingDescriptors"""
    divider("6. Render Delegate Settings")

    try:
        from pxr import Hd

        registry = Hd.RendererPluginRegistry()
        plugin_ids = registry.GetRegisteredPlugins()

        # Find CarWash plugin ID
        carwash_id = None
        for pid in plugin_ids:
            if 'carwash' in str(pid).lower():
                carwash_id = pid
                break

        if not carwash_id:
            print("CarWash renderer plugin not found!")
            return

        print("Found CarWash plugin: " + str(carwash_id))

        # Try to get plugin and create delegate
        plugin = registry.GetRendererPlugin(carwash_id)
        if plugin:
            print("Plugin retrieved: " + str(plugin))

    except Exception as e:
        print("ERROR: " + str(e))
        import traceback
        traceback.print_exc()

def check_debug_log():
    """Check the debug log"""
    divider("7. Debug Log")

    debug_path = r"C:\Temp\hdcarwash_debug.txt"
    if os.path.exists(debug_path):
        print("Debug log exists: " + debug_path)
        try:
            with open(debug_path, 'r') as f:
                content = f.read()
            lines = content.strip().split('\n')
            print("Last 20 lines:")
            for line in lines[-20:]:
                print("  " + line)
        except Exception as e:
            print("Could not read: " + str(e))
    else:
        print("No debug log at: " + debug_path)

def check_env_vars():
    """Check relevant environment variables"""
    divider("8. Environment Variables")

    relevant = [
        'PXR_PLUGINPATH_NAME',
        'HOUDINI_PATH',
        'HOUDINI_DSO_PATH',
        'HOUDINI_PACKAGE_DIR',
    ]

    for var in relevant:
        val = os.environ.get(var, "(not set)")
        if len(val) > 100:
            val = val[:100] + "..."
        print(var + ":")
        print("  " + val)

def run_all():
    """Run all diagnostics"""
    print("\n" + "#" * 60)
    print("  CarWash Plugin Diagnostic Report")
    print("#" * 60)

    check_env_vars()
    check_usdrenderers()
    check_plugin_registry()
    check_renderer_plugins()
    check_schema_registry()
    check_rendersettings_lop()
    check_render_delegate_settings()
    check_debug_log()

    print("\n" + "#" * 60)
    print("  Diagnostic Complete")
    print("#" * 60)

# Run diagnostics
run_all()
