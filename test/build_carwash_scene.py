r"""
CarWash Test Scene Builder
==========================
Run this in Houdini's Python Shell to create a test scene.

In Houdini Python Shell, paste:
    import runpy; runpy.run_path(r"C:\Users\User\CARWASH\test\build_carwash_scene.py")
"""

import hou

def build_carwash_test_scene():
    """Build a complete Solaris scene for testing CarWash renderer."""

    # Clear the scene
    hou.hipFile.clear(suppress_save_prompt=True)

    # Create /stage context for Solaris
    stage = hou.node("/stage")
    if not stage:
        stage = hou.node("/").createNode("stage", "stage")

    # Position for nodes
    x, y = 0, 0
    spacing = 3

    # 1. Create a sphere primitive
    sphere = stage.createNode("sphere", "test_sphere")
    sphere.setPosition([x, y])
    sphere.parm("radius").set(1.0)
    y -= spacing

    # 2. Create a grid/ground plane
    grid = stage.createNode("grid", "ground_plane")
    grid.setPosition([x, y])
    grid.parm("rows").set(10)
    grid.parm("cols").set(10)
    grid.parm("size").set(10)
    grid.parm("ty").set(-1.0)
    y -= spacing

    # 3. Merge the geometry
    merge = stage.createNode("merge", "merge_geo")
    merge.setPosition([x, y])
    merge.setInput(0, sphere)
    merge.setInput(1, grid)
    y -= spacing

    # 4. Add a material library
    material_lib = stage.createNode("materiallibrary", "materials")
    material_lib.setPosition([x - 4, y])

    # 5. Assign material
    assign_mat = stage.createNode("assignmaterial", "assign_material")
    assign_mat.setPosition([x, y])
    assign_mat.setInput(0, merge)
    y -= spacing

    # 6. Add dome light
    dome_light = stage.createNode("domelight", "env_light")
    dome_light.setPosition([x - 4, y])
    dome_light.parm("xn__inputsintensity_i0a").set(1.0)

    # 7. Add key light
    distant_light = stage.createNode("distantlight", "key_light")
    distant_light.setPosition([x - 2, y])
    distant_light.parm("xn__inputsintensity_i0a").set(2.0)
    distant_light.parm("ry").set(-45)
    distant_light.parm("rx").set(-30)

    # 8. Merge lights with scene
    merge_lights = stage.createNode("merge", "merge_all")
    merge_lights.setPosition([x, y])
    merge_lights.setInput(0, assign_mat)
    merge_lights.setInput(1, dome_light)
    merge_lights.setInput(2, distant_light)
    y -= spacing

    # 9. Add camera
    camera = stage.createNode("camera", "render_cam")
    camera.setPosition([x, y])
    camera.parm("tx").set(5)
    camera.parm("ty").set(3)
    camera.parm("tz").set(5)
    camera.parm("rx").set(-20)
    camera.parm("ry").set(45)
    y -= spacing

    # 10. Merge camera
    merge_cam = stage.createNode("merge", "merge_cam")
    merge_cam.setPosition([x, y])
    merge_cam.setInput(0, merge_lights)
    merge_cam.setInput(1, camera)
    y -= spacing

    # 11. Render Settings for CarWash
    render_settings = stage.createNode("rendersettings", "carwash_settings")
    render_settings.setPosition([x, y])
    render_settings.setInput(0, merge_cam)

    # Set CarWash as renderer
    render_settings.parm("renderer").set("HdCarWashRendererPlugin")
    render_settings.parm("resolutionx").set(1024)
    render_settings.parm("resolutiony").set(1024)
    render_settings.parm("camera").set("/render_cam")
    y -= spacing

    # 12. USD Render ROP
    usd_render = stage.createNode("usdrender_rop", "carwash_render")
    usd_render.setPosition([x, y])
    usd_render.setInput(0, render_settings)
    usd_render.parm("outputimage").set("$HIP/render/carwash_test.$F4.exr")

    # Set display flag
    render_settings.setDisplayFlag(True)

    # Layout nodes
    stage.layoutChildren()

    print("=" * 50)
    print("CarWash Test Scene Created!")
    print("=" * 50)
    print("")
    print("Scene: Sphere + Ground + Lights + Camera")
    print("")
    print("To test:")
    print("  1. Click 'carwash_settings' node")
    print("  2. Scene View should show CarWash render")
    print("  3. Check C:\\Temp\\hdcarwash_debug.txt")
    print("")

    return render_settings

# Run
build_carwash_test_scene()
