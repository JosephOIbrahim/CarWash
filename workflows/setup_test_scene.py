"""
CarWash Renderer Test Scene Setup
Run this in Houdini's Python Shell (Windows > Python Shell)
"""

import hou

def setup_hdcarwash_test():
    """Create a simple test scene for CarWash Renderer"""

    # Get or create /obj for SOP geometry
    obj = hou.node('/obj')
    if not obj:
        obj = hou.node('/').createNode('objnet', 'obj')

    # Create SOP geo with polygon sphere
    sop_geo = obj.node('hdcarwash_geo')
    if sop_geo:
        sop_geo.destroy()

    sop_geo = obj.createNode('geo', 'hdcarwash_geo')
    # Create a polygon sphere inside
    sphere_sop = sop_geo.createNode('sphere', 'sphere1')
    sphere_sop.parm('type').set(2)  # Polygon mesh
    sphere_sop.parm('rows').set(32)
    sphere_sop.parm('cols').set(32)
    sphere_sop.setDisplayFlag(True)
    sphere_sop.setRenderFlag(True)

    # Get or create /stage
    stage = hou.node('/stage')
    if not stage:
        stage = hou.node('/').createNode('stage', 'stage')

    # Clear existing test nodes
    for name in ['hdcarwash_sphere', 'hdcarwash_sopimport', 'hdcarwash_camera', 'hdcarwash_light', 'hdcarwash_render', 'hdcarwash_rendersettings']:
        n = stage.node(name)
        if n:
            n.destroy()

    # Import SOP geometry as mesh (not primitive sphere)
    sopimport = stage.createNode('sopimport', 'hdcarwash_sopimport')
    sopimport.parm('soppath').set('/obj/hdcarwash_geo/sphere1')

    # Create camera
    camera = stage.createNode('camera', 'hdcarwash_camera')
    camera.setInput(0, sopimport)
    # Position camera to see the sphere
    camera.parm('tx').set(0)
    camera.parm('ty').set(0)
    camera.parm('tz').set(5)

    # Create a light
    light = stage.createNode('distantlight', 'hdcarwash_light')
    light.setInput(0, camera)
    light.parm('ry').set(-45)
    light.parm('rx').set(-30)

    # Create render settings
    render_settings = stage.createNode('karmarendersettings', 'hdcarwash_rendersettings')
    render_settings.setInput(0, light)

    # IMPORTANT: Override the renderer to CarWash Renderer
    # This is done via the render delegate setting
    render_settings.parm('renderer').set('HdCarWash')

    # Create USD Render ROP
    usd_render = stage.createNode('usdrender', 'hdcarwash_render')
    usd_render.setInput(0, render_settings)

    # Set output path
    usd_render.parm('outputimage').set('$HIP/render/hdcarwash_test.$F4.exr')

    # Layout nodes nicely
    stage.layoutChildren()

    # Set display flag on render node
    usd_render.setDisplayFlag(True)

    print("=" * 60)
    print("CarWash Renderer Test Scene Created!")
    print("=" * 60)
    print("")
    print("Nodes created:")
    print("  /obj/hdcarwash_geo (SOP polygon sphere)")
    print("  /stage/hdcarwash_sopimport (imports mesh to USD)")
    print("  /stage/hdcarwash_camera (camera)")
    print("  /stage/hdcarwash_light (distant light)")
    print("  /stage/hdcarwash_rendersettings (renderer: CarWash Renderer)")
    print("  /stage/hdcarwash_render (USD Render ROP)")
    print("")
    print("To render:")
    print("  1. Click on hdcarwash_render node")
    print("  2. Click 'Render to Disk' or 'Render to MPlay'")
    print("")
    print("Or run: hou.node('/stage/hdcarwash_render').parm('execute').pressButton()")
    print("=" * 60)

    return usd_render

# Run it
if __name__ == '__main__' or True:
    setup_hdcarwash_test()
