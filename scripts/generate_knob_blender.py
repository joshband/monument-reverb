#!/usr/bin/env python3
"""Generate photorealistic knob layers using headless Blender.

Creates 4 knob layers with proper PBR materials and alpha channels:
- base_body: Industrial concrete disc with radial segments
- indicator: Metal bar pointer
- detail_ring: Engraved scale markings
- center_cap: Brushed metal center disc

Run with:
    blender --background --python generate_knob_blender.py

Or with custom output:
    blender --background --python generate_knob_blender.py -- --out /path/to/output --size 512
"""

import sys
import math
from pathlib import Path

# Blender imports (only available when running inside Blender)
try:
    import bpy
    import mathutils
except ImportError:
    print("❌ This script must be run inside Blender:")
    print("   blender --background --python generate_knob_blender.py")
    sys.exit(1)


def clear_scene():
    """Remove all objects from the scene."""
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)


def setup_camera(distance=5.0):
    """Create top-down orthographic camera."""
    bpy.ops.object.camera_add(location=(0, 0, distance))
    camera = bpy.context.object
    camera.data.type = 'ORTHO'
    camera.data.ortho_scale = 2.2  # Frame the knob nicely
    camera.rotation_euler = (0, 0, 0)  # Top-down view
    bpy.context.scene.camera = camera
    return camera


def setup_lighting():
    """Create studio lighting for photorealistic rendering."""
    # Key light (main illumination)
    bpy.ops.object.light_add(type='AREA', location=(2, 2, 4))
    key_light = bpy.context.object
    key_light.data.energy = 100
    key_light.data.size = 3.0
    key_light.rotation_euler = (math.radians(45), 0, math.radians(45))

    # Fill light (soften shadows)
    bpy.ops.object.light_add(type='AREA', location=(-2, -1, 3))
    fill_light = bpy.context.object
    fill_light.data.energy = 50
    fill_light.data.size = 2.5
    fill_light.rotation_euler = (math.radians(60), 0, math.radians(-30))

    # Rim light (edge definition)
    bpy.ops.object.light_add(type='AREA', location=(0, -3, 2))
    rim_light = bpy.context.object
    rim_light.data.energy = 30
    rim_light.data.size = 2.0
    rim_light.rotation_euler = (math.radians(75), 0, 0)


def setup_render_settings(resolution=512, transparent=True):
    """Configure render settings for knob layers."""
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.samples = 128  # Good quality, reasonable speed
    scene.render.resolution_x = resolution
    scene.render.resolution_y = resolution
    scene.render.resolution_percentage = 100

    # Enable transparent background
    if transparent:
        scene.render.film_transparent = True
        scene.render.image_settings.color_mode = 'RGBA'
    else:
        scene.render.image_settings.color_mode = 'RGB'

    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_depth = '8'


def create_concrete_material(name="Concrete"):
    """Create procedural concrete material with brutalist aesthetic."""
    mat = bpy.data.materials.new(name=name)
    # Materials have nodes enabled by default in Blender 5.0+
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links

    # Clear default nodes
    nodes.clear()

    # Create nodes
    output = nodes.new(type='ShaderNodeOutputMaterial')
    output.location = (400, 0)

    bsdf = nodes.new(type='ShaderNodeBsdfPrincipled')
    bsdf.location = (0, 0)
    bsdf.inputs['Base Color'].default_value = (0.35, 0.35, 0.37, 1.0)  # Gray concrete
    bsdf.inputs['Roughness'].default_value = 0.85  # Matte finish
    # Specular input removed in Blender 5.0 - using defaults

    # Add noise texture for variation
    noise = nodes.new(type='ShaderNodeTexNoise')
    noise.location = (-400, 0)
    noise.inputs['Scale'].default_value = 15.0
    noise.inputs['Detail'].default_value = 8.0
    noise.inputs['Roughness'].default_value = 0.6

    # Color ramp to control contrast
    ramp = nodes.new(type='ShaderNodeValToRGB')
    ramp.location = (-200, 0)
    ramp.color_ramp.elements[0].position = 0.4
    ramp.color_ramp.elements[0].color = (0.28, 0.28, 0.30, 1.0)
    ramp.color_ramp.elements[1].position = 0.6
    ramp.color_ramp.elements[1].color = (0.42, 0.42, 0.44, 1.0)

    # Connect nodes
    links.new(noise.outputs['Fac'], ramp.inputs['Fac'])
    links.new(ramp.outputs['Color'], bsdf.inputs['Base Color'])
    links.new(bsdf.outputs['BSDF'], output.inputs['Surface'])

    return mat


def create_metal_material(name="Metal"):
    """Create brushed metal material."""
    mat = bpy.data.materials.new(name=name)
    # Materials have nodes enabled by default in Blender 5.0+
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links

    nodes.clear()

    output = nodes.new(type='ShaderNodeOutputMaterial')
    output.location = (400, 0)

    bsdf = nodes.new(type='ShaderNodeBsdfPrincipled')
    bsdf.location = (0, 0)
    bsdf.inputs['Base Color'].default_value = (0.6, 0.6, 0.62, 1.0)  # Aluminum
    bsdf.inputs['Metallic'].default_value = 1.0
    bsdf.inputs['Roughness'].default_value = 0.3  # Brushed finish

    # Anisotropic for brushed effect (if available in Blender 5.0+)
    if 'Anisotropic' in bsdf.inputs:
        bsdf.inputs['Anisotropic'].default_value = 0.8
    if 'Anisotropic Rotation' in bsdf.inputs:
        bsdf.inputs['Anisotropic Rotation'].default_value = 0.0

    links.new(bsdf.outputs['BSDF'], output.inputs['Surface'])

    return mat


def create_base_body():
    """Create knob base body with radial segments."""
    # Create cylinder
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=64,  # Smooth circle
        radius=1.0,
        depth=0.3,
        location=(0, 0, 0)
    )
    base = bpy.context.object
    base.name = "BaseBody"

    # Add bevel for rounded edge
    bpy.ops.object.modifier_add(type='BEVEL')
    base.modifiers["Bevel"].width = 0.05
    base.modifiers["Bevel"].segments = 4

    # Add radial segments using array + boolean
    bpy.ops.mesh.primitive_cube_add(
        size=0.1,
        location=(0.85, 0, 0)
    )
    segment = bpy.context.object
    segment.scale = (0.15, 0.02, 0.35)

    # Array modifier for radial pattern
    bpy.ops.object.modifier_add(type='ARRAY')
    segment.modifiers["Array"].count = 32
    segment.modifiers["Array"].use_object_offset = True

    # Create empty for rotation
    bpy.ops.object.empty_add(location=(0, 0, 0))
    empty = bpy.context.object
    empty.rotation_euler.z = math.radians(360 / 32)
    segment.modifiers["Array"].offset_object = empty

    # Boolean to cut segments into base
    base.select_set(True)
    bpy.context.view_layer.objects.active = base
    bpy.ops.object.modifier_add(type='BOOLEAN')
    base.modifiers["Boolean"].operation = 'DIFFERENCE'
    base.modifiers["Boolean"].object = segment

    # Apply modifiers
    bpy.ops.object.modifier_apply(modifier="Boolean")
    bpy.ops.object.modifier_apply(modifier="Bevel")

    # Delete helper objects
    bpy.data.objects.remove(segment, do_unlink=True)
    bpy.data.objects.remove(empty, do_unlink=True)

    # Assign material
    mat = create_concrete_material("ConcreteMaterial")
    base.data.materials.append(mat)

    return base


def create_indicator():
    """Create indicator pointer bar."""
    # Create thin bar from center to edge
    bpy.ops.mesh.primitive_cube_add(
        location=(0.5, 0, 0.16)  # Slightly above base
    )
    indicator = bpy.context.object
    indicator.name = "Indicator"
    indicator.scale = (0.45, 0.04, 0.05)  # Long, thin bar

    # Bevel for rounded edges
    bpy.ops.object.modifier_add(type='BEVEL')
    indicator.modifiers["Bevel"].width = 0.01
    indicator.modifiers["Bevel"].segments = 3
    bpy.ops.object.modifier_apply(modifier="Bevel")

    # Assign metal material
    mat = create_metal_material("IndicatorMetal")
    indicator.data.materials.append(mat)

    return indicator


def create_detail_ring():
    """Create static detail ring with scale markings."""
    # Create ring (torus)
    bpy.ops.mesh.primitive_torus_add(
        major_radius=0.95,
        minor_radius=0.05,
        major_segments=128,
        minor_segments=32,
        location=(0, 0, 0.16)
    )
    ring = bpy.context.object
    ring.name = "DetailRing"

    # Create tick marks at 12 positions (clock positions)
    for i in range(12):
        angle = math.radians(i * 30)  # Every 30 degrees
        x = 0.95 * math.cos(angle)
        y = 0.95 * math.sin(angle)

        bpy.ops.mesh.primitive_cube_add(
            location=(x, y, 0.16)
        )
        tick = bpy.context.object
        tick.scale = (0.02, 0.03, 0.08)
        tick.rotation_euler.z = angle

        # Boolean union with ring
        ring.select_set(True)
        bpy.context.view_layer.objects.active = ring
        bpy.ops.object.modifier_add(type='BOOLEAN')
        ring.modifiers[-1].operation = 'UNION'
        ring.modifiers[-1].object = tick
        bpy.ops.object.modifier_apply(modifier=ring.modifiers[-1].name)

        bpy.data.objects.remove(tick, do_unlink=True)

    # Assign material
    mat = create_concrete_material("RingMaterial")
    ring.data.materials.append(mat)

    return ring


def create_center_cap():
    """Create center cap disc."""
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=64,
        radius=0.16,  # 80px / 512px ≈ 16% of knob radius
        depth=0.05,
        location=(0, 0, 0.18)  # Above other layers
    )
    cap = bpy.context.object
    cap.name = "CenterCap"

    # Bevel for rounded edge
    bpy.ops.object.modifier_add(type='BEVEL')
    cap.modifiers["Bevel"].width = 0.02
    cap.modifiers["Bevel"].segments = 4
    bpy.ops.object.modifier_apply(modifier="Bevel")

    # Assign brushed metal material
    mat = create_metal_material("CapMetal")
    cap.data.materials.append(mat)

    return cap


def render_layer(layer_obj, output_path):
    """Render a single layer to PNG with alpha channel."""
    # Hide all other objects
    for obj in bpy.data.objects:
        if obj.type == 'MESH' and obj != layer_obj:
            obj.hide_render = True
        elif obj != layer_obj:
            obj.hide_render = False

    # Show target layer
    layer_obj.hide_render = False

    # Render
    bpy.context.scene.render.filepath = str(output_path)
    bpy.ops.render.render(write_still=True)

    print(f"✅ Rendered: {output_path}")


def main():
    """Generate all knob layers."""
    # Parse arguments
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []

    output_dir = Path("~/Documents/monument-reverb/assets/ui/knobs_test").expanduser()
    resolution = 512

    i = 0
    while i < len(args):
        if args[i] == "--out" and i + 1 < len(args):
            output_dir = Path(args[i + 1])
            i += 2
        elif args[i] == "--size" and i + 1 < len(args):
            resolution = int(args[i + 1])
            i += 2
        else:
            i += 1

    output_dir.mkdir(parents=True, exist_ok=True)

    print("🎨 Generating Monument Reverb knob layers with Blender")
    print(f"📁 Output: {output_dir}")
    print(f"📐 Resolution: {resolution}x{resolution}\n")

    # Setup scene
    clear_scene()
    setup_camera()
    setup_lighting()
    setup_render_settings(resolution=resolution, transparent=True)

    # Create layers
    print("Creating base body...")
    base = create_base_body()

    print("Creating indicator...")
    indicator = create_indicator()

    print("Creating detail ring...")
    ring = create_detail_ring()

    print("Creating center cap...")
    cap = create_center_cap()

    print("\nRendering layers...")

    # Render each layer
    render_layer(base, output_dir / "base_body_concrete.png")
    render_layer(indicator, output_dir / "indicator_metal.png")
    render_layer(ring, output_dir / "detail_ring_engraved.png")
    render_layer(cap, output_dir / "center_cap_brushed_metal.png")

    print("\n" + "="*60)
    print("✨ Knob layer generation complete!")
    print(f"📦 4 layers saved to: {output_dir}")
    print("\nNext step:")
    print("  Add these PNGs to CMakeLists.txt and build Monument")
    print("="*60)


if __name__ == "__main__":
    main()
