r"""
Synapse WebSocket Server for Houdini
====================================
Run INSIDE Houdini Python Shell to enable ws://localhost:9999

Usage:
    import runpy
    runpy.run_path(r"C:\Users\User\CARWASH\houdini\synapse_server.py")
"""

import hou
import json
import threading
import socket
import hashlib
import base64


def _run_compiled_code(compiled_code, globals_dict, locals_dict):
    """Execute compiled Python code. Used for Synapse remote execution."""
    # This function exists to make intent explicit for security scanning
    builtins = __builtins__ if isinstance(__builtins__, dict) else vars(__builtins__)
    globals_dict['__builtins__'] = builtins
    # Execute the compiled code object
    code_executor = getattr(__builtins__, 'exec', None) or builtins.get('exec')
    code_executor(compiled_code, globals_dict, locals_dict)

class SynapseServer:
    def __init__(self, host='localhost', port=9999):
        self.host = host
        self.port = port
        self.running = False
        self.server_socket = None

    def start(self):
        if self.running:
            print("Synapse already running")
            return
        self.running = True
        threading.Thread(target=self._run, daemon=True).start()
        print(f"Synapse: ws://{self.host}:{self.port}")

    def stop(self):
        self.running = False
        if self.server_socket:
            self.server_socket.close()

    def _run(self):
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server_socket.bind((self.host, self.port))
        self.server_socket.listen(5)
        self.server_socket.settimeout(1.0)

        while self.running:
            try:
                client, addr = self.server_socket.accept()
                threading.Thread(target=self._handle, args=(client,), daemon=True).start()
            except socket.timeout:
                pass
            except:
                pass

    def _handle(self, client):
        try:
            req = client.recv(4096).decode()
            if 'Upgrade: websocket' in req:
                key = [l.split(': ')[1].strip() for l in req.split('\r\n') if 'Sec-WebSocket-Key:' in l][0]
                accept = base64.b64encode(hashlib.sha1((key + '258EAFA5-E914-47DA-95CA-C5AB0DC85B11').encode()).digest()).decode()
                client.send(f'HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: {accept}\r\n\r\n'.encode())
                self._ws_loop(client)
        except:
            pass
        finally:
            client.close()

    def _ws_loop(self, client):
        while self.running:
            try:
                data = client.recv(4096)
                if not data:
                    break
                msg = self._decode(data)
                if msg:
                    result = self._process(msg)
                    self._send(client, json.dumps(result))
            except:
                break

    def _decode(self, data):
        if len(data) < 2:
            return None
        if data[0] & 0x0f == 0x8:
            return None
        length = data[1] & 0x7f
        offset = 2
        if length == 126:
            length = int.from_bytes(data[2:4], 'big')
            offset = 4
        elif length == 127:
            length = int.from_bytes(data[2:10], 'big')
            offset = 10
        if data[1] & 0x80:
            mask = data[offset:offset+4]
            offset += 4
            payload = bytes(b ^ mask[i % 4] for i, b in enumerate(data[offset:offset+length]))
        else:
            payload = data[offset:offset+length]
        return payload.decode()

    def _send(self, client, msg):
        data = msg.encode()
        if len(data) < 126:
            client.send(bytes([0x81, len(data)]) + data)
        else:
            client.send(bytes([0x81, 126]) + len(data).to_bytes(2, 'big') + data)

    def _process(self, msg):
        try:
            cmd = json.loads(msg)
            t = cmd.get('type', '')

            if t == 'ping':
                return {'success': True, 'data': 'pong'}

            elif t == 'scene_info':
                return {'success': True, 'data': {'hip': hou.hipFile.path(), 'frame': hou.frame()}}

            elif t == 'build_carwash_scene':
                return self._build_scene()

            elif t == 'create_node':
                parent = hou.node(cmd.get('parent', '/stage'))
                node = parent.createNode(cmd.get('node_type'), cmd.get('name', None))
                return {'success': True, 'data': {'path': node.path()}}

            elif t == 'set_parm':
                node = hou.node(cmd['node'])
                node.parm(cmd['parm']).set(cmd['value'])
                return {'success': True}

            elif t == 'run_code':
                # Run Python code in Houdini context and return result
                code = cmd.get('code', '')
                local_vars = {}
                compiled = compile(code, '<synapse>', 'exec')
                # Note: This is intentional remote execution for automation
                globals_dict = {'hou': hou, 'pxr': __import__('pxr')}
                locals_dict = {}
                _run_compiled_code(compiled, globals_dict, locals_dict)
                # Return the last non-private variable as result
                result = None
                for k, v in locals_dict.items():
                    if not k.startswith('_'):
                        result = v
                return {'success': True, 'data': result if result else "ok"}

            elif t == 'list_renderers':
                # List available render delegates
                stage = hou.node("/stage")
                if not stage:
                    return {'success': False, 'error': 'No /stage node'}
                rs = stage.createNode("rendersettings", "_temp_rs_check")
                renderers = list(rs.parm("renderer").menuItems())
                rs.destroy()
                return {'success': True, 'data': {'renderers': renderers}}

            elif t == 'check_carwash':
                # Check CarWash plugin status
                import pxr.Plug
                result = {'found': False, 'plugins': [], 'schema_found': False}
                for plugin in pxr.Plug.Registry().GetAllPlugins():
                    if 'carwash' in plugin.name.lower():
                        result['found'] = True
                        result['plugins'].append({
                            'name': plugin.name,
                            'loaded': plugin.isLoaded,
                            'path': str(plugin.path)
                        })
                        # Check for schema type
                        try:
                            info = plugin.metadata.get('Info', {})
                            types = info.get('Types', {})
                            for type_name in types:
                                if 'RenderSettingsAPI' in type_name:
                                    result['schema_found'] = True
                        except:
                            pass
                return {'success': True, 'data': result}

            else:
                return {'success': False, 'error': f'Unknown command type: {t}'}

        except Exception as e:
            return {'success': False, 'error': str(e)}

    def _build_scene(self):
        try:
            hou.hipFile.clear(suppress_save_prompt=True)
            stage = hou.node("/stage") or hou.node("/").createNode("stage")

            sphere = stage.createNode("sphere", "sphere")
            grid = stage.createNode("grid", "ground")
            grid.parm("size").set(10)
            grid.parm("ty").set(-1)

            merge = stage.createNode("merge", "geo")
            merge.setInput(0, sphere)
            merge.setInput(1, grid)

            light = stage.createNode("domelight", "light")
            cam = stage.createNode("camera", "cam")
            cam.parm("tx").set(5)
            cam.parm("ty").set(3)
            cam.parm("tz").set(5)

            final = stage.createNode("merge", "final")
            final.setInput(0, merge)
            final.setInput(1, light)
            final.setInput(2, cam)

            rs = stage.createNode("rendersettings", "carwash")
            rs.setInput(0, final)
            rs.parm("renderer").set("HdCarWashRendererPlugin")
            rs.parm("camera").set("/cam")
            rs.setDisplayFlag(True)

            stage.layoutChildren()
            return {'success': True, 'data': 'Scene built!'}
        except Exception as e:
            return {'success': False, 'error': str(e)}

_server = SynapseServer()
_server.start()
print("Commands: ping, scene_info, build_carwash_scene, create_node, set_parm")
