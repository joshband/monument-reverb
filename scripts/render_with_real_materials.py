#!/usr/bin/env python3
"""
Render knobs using REAL PBR materials from materialize pipeline.
No procedural generation - uses extracted textures from Midjourney images.
"""

import bpy
import math
import os
import sys

# === CONFIGURATION ===

# Path to materialize output directory
MATERIALIZE_DIR = os.path.expanduser("~/Documents/3_Development/Repos/materialize/dist")

# Choose which material to use (change this to try different ones)
MATERIAL_NAME = "flat_retro_audio_plugin_UI_2D_vintage_industrial_interface_to_dc18d488-bac9-4bfa_04"

RENDER_SAMPLES = 256
OUTPUT_DIR = "assets/ui/knobs_real_materials"
RESOLUTION = 1024
FRAMES = 60  # Number of rotation frames for filmstrip


def setup_scene():
    """Initialize Blender scene."""
    # Clear scene
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete()
    for mat in bpy.data.materials:
        bpy.data.materials.remove(mat)

    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.samples = RENDER_SAMPLES
    scene.cycles.device = 'GPU'

    scene.render.resolution_x = RESOLUTION
    scene.render.resolution_y = RESOLUTION
    scene.render.film_transparent = True
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGBA'
    scene.render.image_settings.color_depth = '16'

    # HDRI lighting
    setup_hdri()

    # Camera
    cam_data = bpy.data.cameras.new(name="Camera")
    cam_obj = bpy.data.objects.new(name="Camera", object_data=cam_data)
    scene.collection.objects.link(cam_obj)
    scene.camera = cam_obj
    cam_obj.location = (0, 0, 5.0)
    cam_data.lens = 50

    return scene


def setup_hdri():
    """Load downloaded HDRI."""
    world = bpy.context.scene.world
    world.use_nodes = True
    nodes = world.node_tree.nodes
    links = world.node_tree.links
    nodes.clear()

    output = nodes.new(type='ShaderNodeOutputWorld')
    output.location = (400, 0)

    background = nodes.new(type='ShaderNodeBackground')
    background.location = (200, 0)
    background.inputs['Strength'].default_value = 1.5

    hdri_path = os.path.join(os.path.dirname(__file__), "../assets/textures/hdri/studio.hdr")

    if os.path.exists(hdri_path):
        env_tex = nodes.new(type='ShaderNodeTexEnvironment')
        env_tex.location = (0, 0)
        env_tex.image = bpy.data.images.load(hdri_path, check_existing=True)
        links.new(env_tex.outputs['Color'], background.inputs['Color'])

    links.new(background.outputs['Background'], output.inputs['Surface'])


def load_real_pbr_material(material_name):
    """Load PBR texture maps from materialize output."""
    mat_dir = os.path.join(MATERIALIZE_DIR, material_name)

    if not os.path.exists(mat_dir):
        print(f"❌ Material directory not found: {mat_dir}")
        return None

    # Create material
    mat = bpy.data.materials.new(name="RealPBR")
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()

    # Output
    output = nodes.new(type='ShaderNodeOutputMaterial')
    output.location = (600, 0)

    # Principled BSDF
    bsdf = nodes.new(type='ShaderNodeBsdfPrincipled')
    bsdf.location = (300, 0)

    # Load textures
    albedo_path = os.path.join(mat_dir, "albedo.png")
    normal_path = os.path.join(mat_dir, "normal.png")
    roughness_path = os.path.join(mat_dir, "roughness.png")
    metallic_path = os.path.join(mat_dir, "metallic.png")
    ao_path = os.path.join(mat_dir, "ao.png")

    y_pos = 200

    # Albedo/Base Color
    if os.path.exists(albedo_path):
        tex_albedo = nodes.new(type='ShaderNodeTexImage')
        tex_albedo.location = (-300, y_pos)
        tex_albedo.image = bpy.data.images.load(albedo_path, check_existing=True)
        links.new(tex_albedo.outputs['Color'], bsdf.inputs['Base Color'])
        print(f"✓ Loaded albedo")
        y_pos -= 300

    # Roughness
    if os.path.exists(roughness_path):
        tex_rough = nodes.new(type='ShaderNodeTexImage')
        tex_rough.location = (-300, y_pos)
        tex_rough.image = bpy.data.images.load(roughness_path, check_existing=True)
        tex_rough.image.colorspace_settings.name = 'Non-Color'
        links.new(tex_rough.outputs['Color'], bsdf.inputs['Roughness'])
        print(f"✓ Loaded roughness")
        y_pos -= 300

    # Metallic
    if os.path.exists(metallic_path):
        tex_metal = nodes.new(type='ShaderNodeTexImage')
        tex_metal.location = (-300, y_pos)
        tex_metal.image = bpy.data.images.load(metallic_path, check_existing=True)
        tex_metal.image.colorspace_settings.name = 'Non-Color'
        links.new(tex_metal.outputs['Color'], bsdf.inputs['Metallic'])
        print(f"✓ Loaded metallic")
        y_pos -= 300

    # Normal map
    if os.path.exists(normal_path):
        tex_normal = nodes.new(type='ShaderNodeTexImage')
        tex_normal.location = (-600, -400)
        tex_normal.image = bpy.data.images.load(normal_path, check_existing=True)
        tex_normal.image.colorspace_settings.name = 'Non-Color'

        normal_map = nodes.new(type='ShaderNodeNormalMap')
        normal_map.location = (-300, -400)

        links.new(tex_normal.outputs['Color'], normal_map.inputs['Color'])
        links.new(normal_map.outputs['Normal'], bsdf.inputs['Normal'])
        print(f"✓ Loaded normal map")

    # Connect shader
    links.new(bsdf.outputs['BSDF'], output.inputs['Surface'])

    return mat


def create_simple_knob():
    """Create simple cylinder for material application."""
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=64,
        radius=1.0,
        depth=0.3,
        location=(0, 0, 0)
    )
    knob = bpy.context.object
    knob.name = "Knob"

    # Add bevel for smooth edges
    bevel = knob.modifiers.new(name="Bevel", type='BEVEL')
    bevel.width = 0.05
    bevel.segments = 4

    return knob


def render_filmstrip(knob, output_path):
    """Render rotation sequence."""
    scene = bpy.context.scene

    for frame in range(FRAMES):
        angle = (frame / FRAMES) * 360.0
        knob.rotation_euler[2] = math.radians(angle)

        frame_path = os.path.join(output_path, f"frame_{frame:03d}.png")
        scene.render.filepath = frame_path

        print(f"Rendering frame {frame+1}/{FRAMES} ({angle:.1f}°)...")
        bpy.ops.render.render(write_still=True)


def main():
    """Main execution."""
    print("=== Real Material Knob Renderer ===")
    print(f"Material: {MATERIAL_NAME}")
    print(f"Samples: {RENDER_SAMPLES}")
    print(f"Frames: {FRAMES}")
    print("")

    # Setup
    scene = setup_scene()

    # Load real PBR material
    mat = load_real_pbr_material(MATERIAL_NAME)
    if not mat:
        sys.exit(1)

    # Create knob geometry
    knob = create_simple_knob()
    knob.data.materials.append(mat)

    # Render filmstrip
    output_path = os.path.join(OUTPUT_DIR, MATERIAL_NAME)
    os.makedirs(output_path, exist_ok=True)

    render_filmstrip(knob, output_path)

    print(f"✅ Filmstrip complete: {output_path}")
    print(f"Next: Stitch frames into single PNG filmstrip")


if __name__ == "__main__":
    main()
