"""Run diagnostic via Synapse WebSocket."""
import asyncio
import json
import websockets

DIAGNOSTIC_CODE = '''
import hou
import os

results = {
    'usdrenderers_files': [],
    'carwash_in_usdrenderers': False,
    'plugins_found': [],
    'renderers': [],
    'carwash_renderer_found': False,
    'schema_types': [],
    'carwash_parms': [],
    'env_vars': {},
    'api_schemas_for_rendersettings': []
}

# 1. Check UsdRenderers.json files
try:
    files = hou.findFiles('UsdRenderers.json')
    results['usdrenderers_files'] = list(files)
    for f in files:
        try:
            with open(f, 'r') as fp:
                if 'carwash' in fp.read().lower():
                    results['carwash_in_usdrenderers'] = True
        except:
            pass
except:
    pass

# 2. Check USD plugin registry
try:
    import pxr.Plug
    registry = pxr.Plug.Registry()
    for plugin in registry.GetAllPlugins():
        if 'carwash' in plugin.name.lower():
            results['plugins_found'].append({
                'name': plugin.name,
                'path': str(plugin.path),
                'loaded': plugin.isLoaded
            })
        try:
            meta = plugin.metadata
            if 'Info' in meta:
                types = meta['Info'].get('Types', {})
                for tname, tinfo in types.items():
                    applies_to = tinfo.get('apiSchemaCanOnlyApplyTo', [])
                    if 'RenderSettings' in applies_to:
                        results['api_schemas_for_rendersettings'].append({
                            'type': tname,
                            'plugin': plugin.name
                        })
        except:
            pass
except Exception as e:
    results['plugin_error'] = str(e)

# 3. Check Hydra renderer plugins
try:
    from pxr import Hd
    registry = Hd.RendererPluginRegistry()
    plugin_ids = registry.GetRegisteredPlugins()
    results['renderers'] = [str(p) for p in plugin_ids]
    results['carwash_renderer_found'] = any('carwash' in str(p).lower() for p in plugin_ids)
except Exception as e:
    results['renderer_error'] = str(e)

# 4. Check schema types
try:
    from pxr import Tf
    for name in ['CarWashRenderSettingsAPI', 'usdCarWashCarWashRenderSettingsAPI']:
        tf_type = Tf.Type.FindByName(name)
        if tf_type and not tf_type.isUnknown:
            results['schema_types'].append(name)
except Exception as e:
    results['schema_error'] = str(e)

# 5. Check render settings LOP
try:
    stage = hou.node('/stage')
    if not stage:
        stage = hou.node('/').createNode('stage')
    rs = stage.createNode('rendersettings', '_diag_rs')
    menu_items = rs.parm('renderer').menuItems()
    menu_labels = rs.parm('renderer').menuLabels()

    carwash_renderers = [(i, l) for i, l in zip(menu_items, menu_labels) if 'carwash' in i.lower()]
    if carwash_renderers:
        rs.parm('renderer').set(carwash_renderers[0][0])
        for parm in rs.parms():
            if 'carwash' in parm.name().lower():
                results['carwash_parms'].append(parm.name())
    rs.destroy()
except Exception as e:
    results['lop_error'] = str(e)

# 6. Check env vars
for var in ['PXR_PLUGINPATH_NAME']:
    results['env_vars'][var] = os.environ.get(var, '(not set)')

results
'''

async def run_diagnostic():
    async with websockets.connect('ws://localhost:9999', close_timeout=5) as ws:
        await ws.send(json.dumps({'type': 'ping'}))
        resp = await asyncio.wait_for(ws.recv(), timeout=5)
        print('Connected to Synapse')

        await ws.send(json.dumps({'type': 'run_code', 'code': DIAGNOSTIC_CODE}))
        resp = await asyncio.wait_for(ws.recv(), timeout=30)
        result = json.loads(resp)

        if result.get('success'):
            data = result.get('data', {})
            print()
            print('=' * 60)
            print('  CarWash Diagnostic Results')
            print('=' * 60)

            print('\n1. UsdRenderers.json Discovery:')
            for f in data.get('usdrenderers_files', []):
                print('   - ' + f)
            print('   CarWash entry found: ' + str(data.get('carwash_in_usdrenderers', False)))

            print('\n2. USD Plugin Registry:')
            plugins = data.get('plugins_found', [])
            if plugins:
                for p in plugins:
                    print('   FOUND: ' + p['name'] + ' (loaded: ' + str(p['loaded']) + ')')
                    print('     Path: ' + p['path'])
            else:
                print('   NO CarWash plugins found in registry!')

            print('\n3. API Schemas for RenderSettings:')
            api_schemas = data.get('api_schemas_for_rendersettings', [])
            if api_schemas:
                for s in api_schemas:
                    marker = ' <-- CARWASH' if 'carwash' in s['type'].lower() else ''
                    print('   - ' + s['type'] + ' (from ' + s['plugin'] + ')' + marker)
            else:
                print('   NONE found!')

            print('\n4. Hydra Renderer Plugins:')
            print('   CarWash renderer found: ' + str(data.get('carwash_renderer_found', False)))
            for r in data.get('renderers', []):
                marker = ' <-- CARWASH' if 'carwash' in r.lower() else ''
                print('   - ' + r + marker)

            print('\n5. Schema Types (TfType):')
            schemas = data.get('schema_types', [])
            if schemas:
                for s in schemas:
                    print('   FOUND: ' + s)
            else:
                print('   NO CarWash schema types registered!')

            print('\n6. CarWash Parameters in Render Settings:')
            parms = data.get('carwash_parms', [])
            if parms:
                for p in parms:
                    print('   - ' + p)
            else:
                print('   NONE found (render settings tab not appearing)')

            print('\n7. PXR_PLUGINPATH_NAME:')
            env = data.get('env_vars', {})
            path = env.get('PXR_PLUGINPATH_NAME', '(not set)')
            for p in path.split(';'):
                if p:
                    marker = ' <-- CARWASH' if 'carwash' in p.lower() else ''
                    print('   - ' + p + marker)

            print('\n' + '=' * 60)
        else:
            print('Error: ' + str(result.get('error')))

if __name__ == '__main__':
    asyncio.run(run_diagnostic())
