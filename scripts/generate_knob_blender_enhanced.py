#!/usr/bin/env python3
"""Generate photorealistic knob layers with advanced environmental effects.

Creates layered knob assets with:
- Base layers (body, indicator, ring, cap)
- Environmental effects (shadows, AO, reflections, highlights)
- Multiple material variants (granite, marble, basalt, glass, metal)
- State variations (normal, hover, active, disabled)

Run with:
    blender --background --python generate_knob_blender_enhanced.py -- --material granite

Or generate all materials:
    ./scripts/run_blender_enhanced.sh
"""

import sys
import math
from pathlib import Path

# Blender imports
try:
    import bpy
except ImportError:
    print("❌ This script must be run inside Blender")
    sys.exit(1)


# === MATERIAL DEFINITIONS ===

MATERIALS = {
    "granite": {
        "base_color": (0.35, 0.35, 0.37, 1.0),
        "roughness": 0.85,
        "metallic": 0.0,
        "noise_scale": 15.0,
        "noise_detail": 8.0
    },
    "marble": {
        "base_color": (0.85, 0.82, 0.78, 1.0),
        "roughness": 0.15,
        "metallic": 0.0,
        "noise_scale": 8.0,
        "noise_detail": 12.0
    },
    "basalt": {
        "base_color": (0.15, 0.15, 0.17, 1.0),
        "roughness": 0.75,
        "metallic": 0.0,
        "noise_scale": 20.0,
        "noise_detail": 6.0
    },
    "glass": {
        "base_color": (0.92, 0.95, 0.98, 1.0),
        "roughness": 0.0,
        "metallic": 0.0,
        "transmission": 0.95,
        "ior": 1.45
    },
    "brushed_metal": {
        "base_color": (0.6, 0.6, 0.62, 1.0),
        "roughness": 0.3,
        "metallic": 1.0,
        "anisotropic": 0.8
    },
    "brass": {
        "base_color": (0.78, 0.58, 0.28, 1.0),
        "roughness": 0.18,
        "metallic": 1.0,
        "anisotropic": 0.4,
        "use_procedural": True  # Enable procedural aging
    },
    "oxidized_copper": {
        "base_color": (0.42, 0.55, 0.48, 1.0),  # Verdigris patina
        "roughness": 0.6,
        "metallic": 0.4,
        "noise_scale": 25.0,
        "noise_detail": 10.0
    }
}

INDICATOR_MATERIALS = {
    "brushed_aluminum": {
        "base_color": (0.6, 0.6, 0.62, 1.0),
        "roughness": 0.3,
        "metallic": 1.0,
        "anisotropic": 0.8
    },
    "gold": {
        "base_color": (0.83, 0.69, 0.22, 1.0),
        "roughness": 0.2,
        "metallic": 1.0
    },
    "copper": {
        "base_color": (0.95, 0.64, 0.54, 1.0),
        "roughness": 0.25,
        "metallic": 1.0
    }
}


# === UTILITY FUNCTIONS ===

def clear_scene():
    """Remove all objects from scene."""
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete(use_global=False)

    # Clean up orphaned data
    for mesh in bpy.data.meshes:
        if mesh.users == 0:
            bpy.data.meshes.remove(mesh)
    for mat in bpy.data.materials:
        if mat.users == 0:
            bpy.data.materials.remove(mat)


def setup_camera(distance=5.0):
    """Create top-down orthographic camera."""
    bpy.ops.object.camera_add(location=(0, 0, distance))
    camera = bpy.context.object
    camera.data.type = 'ORTHO'
    camera.data.ortho_scale = 2.2
    camera.rotation_euler = (0, 0, 0)
    bpy.context.scene.camera = camera
    return camera


def setup_enhanced_lighting():
    """Create studio lighting with environmental effects."""
    # Key light (main illumination from upper right)
    bpy.ops.object.light_add(type='AREA', location=(2.5, 2.5, 5))
    key = bpy.context.object
    key.name = "KeyLight"
    key.data.energy = 150
    key.data.size = 4.0
    key.data.color = (1.0, 0.98, 0.95)  # Warm white
    key.rotation_euler = (math.radians(35), 0, math.radians(45))

    # Fill light (soften shadows from left)
    bpy.ops.object.light_add(type='AREA', location=(-2, -1, 4))
    fill = bpy.context.object
    fill.name = "FillLight"
    fill.data.energy = 60
    fill.data.size = 3.5
    fill.data.color = (0.95, 0.97, 1.0)  # Cool white
    fill.rotation_euler = (math.radians(50), 0, math.radians(-30))

    # Rim light (edge definition from back)
    bpy.ops.object.light_add(type='AREA', location=(0, -3.5, 3))
    rim = bpy.context.object
    rim.name = "RimLight"
    rim.data.energy = 80
    rim.data.size = 2.5
    rim.data.color = (1.0, 1.0, 1.0)
    rim.rotation_euler = (math.radians(70), 0, 0)

    # Ambient light (subtle overall illumination)
    bpy.ops.object.light_add(type='AREA', location=(0, 0, 6))
    ambient = bpy.context.object
    ambient.name = "AmbientLight"
    ambient.data.energy = 25
    ambient.data.size = 6.0
    ambient.data.color = (0.98, 0.98, 1.0)
    ambient.rotation_euler = (0, 0, 0)

    # World environment
    world = bpy.context.scene.world
    world.use_nodes = True
    bg = world.node_tree.nodes['Background']
    bg.inputs['Color'].default_value = (0.05, 0.05, 0.05, 1.0)  # Dark gray
    bg.inputs['Strength'].default_value = 0.1  # Subtle ambient


def setup_render_settings(resolution=512, samples=256, transparent=True):
    """Configure high-quality render settings."""
    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'

    # High quality settings
    scene.cycles.samples = samples
    scene.cycles.use_denoising = True
    scene.cycles.denoiser = 'OPENIMAGEDENOISE'

    # Enable advanced features
    scene.cycles.use_adaptive_sampling = True
    scene.cycles.adaptive_threshold = 0.01

    # Resolution
    scene.render.resolution_x = resolution
    scene.render.resolution_y = resolution
    scene.render.resolution_percentage = 100

    # Transparency
    if transparent:
        scene.render.film_transparent = True
        scene.render.image_settings.color_mode = 'RGBA'
    else:
        scene.render.image_settings.color_mode = 'RGB'

    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_depth = '16'  # 16-bit for better gradients
    scene.render.image_settings.compression = 15  # Max compression


# === MATERIAL CREATION ===

def create_stone_material(name, material_props):
    """Create procedural stone material with noise variation."""
    mat = bpy.data.materials.new(name=name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()

    # Output
    output = nodes.new(type='ShaderNodeOutputMaterial')
    output.location = (600, 0)

    # BSDF
    bsdf = nodes.new(type='ShaderNodeBsdfPrincipled')
    bsdf.location = (300, 0)
    bsdf.inputs['Roughness'].default_value = material_props['roughness']
    bsdf.inputs['Metallic'].default_value = material_props['metallic']

    # Noise texture for color variation
    noise = nodes.new(type='ShaderNodeTexNoise')
    noise.location = (-600, 100)
    noise.inputs['Scale'].default_value = material_props['noise_scale']
    noise.inputs['Detail'].default_value = material_props['noise_detail']
    noise.inputs['Roughness'].default_value = 0.6

    # Color ramp for contrast
    ramp = nodes.new(type='ShaderNodeValToRGB')
    ramp.location = (-400, 100)
    base_color = material_props['base_color']
    ramp.color_ramp.elements[0].position = 0.45
    ramp.color_ramp.elements[0].color = (
        base_color[0] * 0.8, base_color[1] * 0.8, base_color[2] * 0.8, 1.0
    )
    ramp.color_ramp.elements[1].position = 0.55
    ramp.color_ramp.elements[1].color = (
        base_color[0] * 1.2, base_color[1] * 1.2, base_color[2] * 1.2, 1.0
    )

    # Bump map for surface detail
    bump = nodes.new(type='ShaderNodeBump')
    bump.location = (0, -200)
    bump.inputs['Strength'].default_value = 0.3
    bump.inputs['Distance'].default_value = 0.05

    # Noise for bump
    bump_noise = nodes.new(type='ShaderNodeTexNoise')
    bump_noise.location = (-300, -200)
    bump_noise.inputs['Scale'].default_value = material_props['noise_scale'] * 3
    bump_noise.inputs['Detail'].default_value = 16.0

    # Connect nodes
    links.new(noise.outputs['Fac'], ramp.inputs['Fac'])
    links.new(ramp.outputs['Color'], bsdf.inputs['Base Color'])
    links.new(bump_noise.outputs['Fac'], bump.inputs['Height'])
    links.new(bump.outputs['Normal'], bsdf.inputs['Normal'])
    links.new(bsdf.outputs['BSDF'], output.inputs['Surface'])

    return mat


def create_glass_material(name, material_props):
    """Create transparent glass material."""
    mat = bpy.data.materials.new(name=name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()

    output = nodes.new(type='ShaderNodeOutputMaterial')
    output.location = (400, 0)

    bsdf = nodes.new(type='ShaderNodeBsdfPrincipled')
    bsdf.location = (0, 0)
    bsdf.inputs['Base Color'].default_value = material_props['base_color']
    bsdf.inputs['Roughness'].default_value = material_props['roughness']
    bsdf.inputs['Metallic'].default_value = material_props['metallic']
    bsdf.inputs['Transmission Weight'].default_value = material_props['transmission']
    bsdf.inputs['IOR'].default_value = material_props['ior']

    links.new(bsdf.outputs['BSDF'], output.inputs['Surface'])

    return mat


def create_metal_material(name, material_props):
    """Create brushed metal material with anisotropic reflection.

    For brass, includes procedural aging with micro-scratches and surface variation.
    """
    mat = bpy.data.materials.new(name=name)
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()

    output = nodes.new(type='ShaderNodeOutputMaterial')
    output.location = (600, 0)

    bsdf = nodes.new(type='ShaderNodeBsdfPrincipled')
    bsdf.location = (200, 0)
    bsdf.inputs['Base Color'].default_value = material_props['base_color']
    bsdf.inputs['Metallic'].default_value = material_props['metallic']

    if 'anisotropic' in material_props:
        bsdf.inputs['Anisotropic'].default_value = material_props['anisotropic']
        bsdf.inputs['Anisotropic Rotation'].default_value = 0.0

    # Add procedural aging for brass
    if material_props.get('use_procedural', False):
        # Noise texture for subtle surface variation
        noise = nodes.new(type='ShaderNodeTexNoise')
        noise.location = (-600, -200)
        noise.inputs['Scale'].default_value = 80.0
        noise.inputs['Detail'].default_value = 10.0
        noise.inputs['Roughness'].default_value = 0.6
        noise.inputs['Distortion'].default_value = 0.2

        # Color ramp to control variation amount
        color_ramp = nodes.new(type='ShaderNodeValToRGB')
        color_ramp.location = (-400, -200)
        color_ramp.color_ramp.elements[0].position = 0.45
        color_ramp.color_ramp.elements[1].position = 0.55
        color_ramp.color_ramp.elements[0].color = (0.0, 0.0, 0.0, 1.0)
        color_ramp.color_ramp.elements[1].color = (0.08, 0.08, 0.08, 1.0)

        # Mix subtle variation into roughness
        mix_roughness = nodes.new(type='ShaderNodeMix')
        mix_roughness.location = (-200, -200)
        mix_roughness.data_type = 'RGBA'
        mix_roughness.inputs[0].default_value = 0.3  # Mix factor
        mix_roughness.inputs[6].default_value = (material_props['roughness'],
                                                   material_props['roughness'],
                                                   material_props['roughness'], 1.0)

        # Voronoi for micro-scratches
        voronoi = nodes.new(type='ShaderNodeTexVoronoi')
        voronoi.location = (-600, -400)
        voronoi.feature = 'DISTANCE_TO_EDGE'
        voronoi.inputs['Scale'].default_value = 300.0

        # Remap voronoi for scratches
        scratch_ramp = nodes.new(type='ShaderNodeValToRGB')
        scratch_ramp.location = (-400, -400)
        scratch_ramp.color_ramp.elements[0].position = 0.95
        scratch_ramp.color_ramp.elements[1].position = 1.0
        scratch_ramp.color_ramp.elements[0].color = (0.0, 0.0, 0.0, 1.0)
        scratch_ramp.color_ramp.elements[1].color = (0.15, 0.15, 0.15, 1.0)

        # Mix scratches into roughness
        mix_scratches = nodes.new(type='ShaderNodeMix')
        mix_scratches.location = (0, -200)
        mix_scratches.data_type = 'RGBA'
        mix_scratches.inputs[0].default_value = 0.15  # Subtle scratches

        # Connect procedural aging nodes
        links.new(noise.outputs['Fac'], color_ramp.inputs['Fac'])
        links.new(color_ramp.outputs['Color'], mix_roughness.inputs[7])
        links.new(voronoi.outputs['Distance'], scratch_ramp.inputs['Fac'])
        links.new(mix_roughness.outputs[2], mix_scratches.inputs[6])
        links.new(scratch_ramp.outputs['Color'], mix_scratches.inputs[7])
        links.new(mix_scratches.outputs[2], bsdf.inputs['Roughness'])
    else:
        # Simple roughness for non-brass metals
        bsdf.inputs['Roughness'].default_value = material_props['roughness']

    links.new(bsdf.outputs['BSDF'], output.inputs['Surface'])

    return mat


def create_material(material_name):
    """Create material based on type and properties."""
    if material_name not in MATERIALS:
        material_name = "granite"  # Default fallback

    props = MATERIALS[material_name]

    if material_name == "glass":
        return create_glass_material(f"Material_{material_name}", props)
    if props.get('metallic', 0) > 0.5:
        return create_metal_material(f"Material_{material_name}", props)
    return create_stone_material(f"Material_{material_name}", props)


def create_indicator_material(indicator_type="brushed_aluminum"):
    """Create material for indicator bar."""
    if indicator_type not in INDICATOR_MATERIALS:
        indicator_type = "brushed_aluminum"

    props = INDICATOR_MATERIALS[indicator_type]
    return create_metal_material(f"Indicator_{indicator_type}", props)


# === GEOMETRY CREATION ===

def create_base_body():
    """Create knob base body with radial segments."""
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=64,
        radius=1.0,
        depth=0.3,
        location=(0, 0, 0)
    )
    base = bpy.context.object
    base.name = "BaseBody"

    # Bevel modifier for rounded edges
    bevel = base.modifiers.new(name="Bevel", type='BEVEL')
    bevel.width = 0.05
    bevel.segments = 4

    # Create radial segment cutouts
    bpy.ops.mesh.primitive_cube_add(size=0.1, location=(0.85, 0, 0))
    segment = bpy.context.object
    segment.scale = (0.15, 0.02, 0.35)

    # Array modifier for radial pattern
    array = segment.modifiers.new(name="Array", type='ARRAY')
    array.count = 32
    array.use_object_offset = True

    # Create empty for rotation offset
    bpy.ops.object.empty_add(location=(0, 0, 0))
    empty = bpy.context.object
    empty.rotation_euler.z = math.radians(360 / 32)
    array.offset_object = empty

    # Boolean modifier to cut segments
    boolean = base.modifiers.new(name="Boolean", type='BOOLEAN')
    boolean.operation = 'DIFFERENCE'
    boolean.object = segment

    # Apply modifiers
    bpy.context.view_layer.objects.active = base
    bpy.ops.object.modifier_apply(modifier="Boolean")
    bpy.ops.object.modifier_apply(modifier="Bevel")

    # Cleanup
    bpy.data.objects.remove(segment, do_unlink=True)
    bpy.data.objects.remove(empty, do_unlink=True)

    return base


def create_indicator():
    """Create indicator pointer bar."""
    bpy.ops.mesh.primitive_cube_add(location=(0.5, 0, 0.16))
    indicator = bpy.context.object
    indicator.name = "Indicator"
    indicator.scale = (0.45, 0.04, 0.05)

    # Bevel for rounded edges
    bevel = indicator.modifiers.new(name="Bevel", type='BEVEL')
    bevel.width = 0.01
    bevel.segments = 3
    bpy.ops.object.modifier_apply(modifier="Bevel")

    return indicator


def create_detail_ring():
    """Create detail ring with scale markings."""
    bpy.ops.mesh.primitive_torus_add(
        major_radius=0.95,
        minor_radius=0.05,
        major_segments=128,
        minor_segments=32,
        location=(0, 0, 0.16)
    )
    ring = bpy.context.object
    ring.name = "DetailRing"

    # Add tick marks at 12 positions
    for i in range(12):
        angle = math.radians(i * 30)
        x = 0.95 * math.cos(angle)
        y = 0.95 * math.sin(angle)

        bpy.ops.mesh.primitive_cube_add(location=(x, y, 0.16))
        tick = bpy.context.object
        tick.scale = (0.02, 0.03, 0.08)
        tick.rotation_euler.z = angle

        # Boolean union with ring
        bpy.context.view_layer.objects.active = ring
        boolean = ring.modifiers.new(name=f"Tick{i}", type='BOOLEAN')
        boolean.operation = 'UNION'
        boolean.object = tick
        bpy.ops.object.modifier_apply(modifier=boolean.name)

        bpy.data.objects.remove(tick, do_unlink=True)

    return ring


def create_center_cap():
    """Create center cap disc."""
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=64,
        radius=0.16,
        depth=0.05,
        location=(0, 0, 0.18)
    )
    cap = bpy.context.object
    cap.name = "CenterCap"

    # Bevel for rounded edge
    bevel = cap.modifiers.new(name="Bevel", type='BEVEL')
    bevel.width = 0.02
    bevel.segments = 4
    bpy.ops.object.modifier_apply(modifier="Bevel")

    return cap


def create_fluted_grip_knob():
    """Create fluted grip knob with vertical fins (vintage white knob style).

    Features:
    - Wide base platform
    - Vertical fluted/grooved grip section
    - Smooth top cap
    - Indicator dot on top
    """
    # Base platform (wider, provides stability)
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=128,
        radius=1.15,
        depth=0.12,
        location=(0, 0, 0.06)
    )
    base_platform = bpy.context.object
    base_platform.name = "BasePlatform"

    # Chamfer bottom edge
    bevel = base_platform.modifiers.new(name="Chamfer", type='BEVEL')
    bevel.width = 0.06
    bevel.segments = 3
    bevel.limit_method = 'ANGLE'
    bevel.angle_limit = math.radians(30)

    # Fluted grip cylinder (vertical grooves)
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=64,  # Must match flute count for clean boolean
        radius=1.0,
        depth=0.50,
        location=(0, 0, 0.37)
    )
    grip_cylinder = bpy.context.object
    grip_cylinder.name = "FlutedGrip"

    # Create flutes using array of vertical grooves
    num_flutes = 64
    for i in range(num_flutes):
        angle = math.radians(i * (360 / num_flutes))
        x = 1.0 * math.cos(angle)
        y = 1.0 * math.sin(angle)

        # Create thin vertical groove
        bpy.ops.mesh.primitive_cube_add(
            size=1.0,
            location=(x, y, 0.37)
        )
        groove = bpy.context.object
        groove.scale = (0.03, 0.08, 0.55)
        groove.rotation_euler.z = angle

        # Boolean subtract groove
        bpy.context.view_layer.objects.active = grip_cylinder
        boolean = grip_cylinder.modifiers.new(name=f"Flute{i}", type='BOOLEAN')
        boolean.operation = 'DIFFERENCE'
        boolean.object = groove

        # Apply and cleanup immediately (keep scene clean)
        bpy.ops.object.modifier_apply(modifier=boolean.name)
        bpy.data.objects.remove(groove, do_unlink=True)

    # Top cap (smooth dome)
    bpy.ops.mesh.primitive_uv_sphere_add(
        segments=64,
        ring_count=32,
        radius=0.95,
        location=(0, 0, 0.62)
    )
    top_cap = bpy.context.object
    top_cap.name = "TopCap"
    top_cap.scale.z = 0.4  # Flatten to low dome

    # Cut bottom half of sphere
    bpy.ops.mesh.primitive_cube_add(
        size=3.0,
        location=(0, 0, 0.30)
    )
    cutter = bpy.context.object
    bpy.context.view_layer.objects.active = top_cap
    boolean = top_cap.modifiers.new(name="CutBottom", type='BOOLEAN')
    boolean.operation = 'DIFFERENCE'
    boolean.object = cutter
    bpy.ops.object.modifier_apply(modifier="CutBottom")
    bpy.data.objects.remove(cutter, do_unlink=True)

    # Indicator dot
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=32,
        radius=0.08,
        depth=0.03,
        location=(0, 0.55, 0.67)
    )
    indicator_dot = bpy.context.object
    indicator_dot.name = "IndicatorDot"

    # Join all parts
    bpy.context.view_layer.objects.active = base_platform
    base_platform.select_set(True)
    grip_cylinder.select_set(True)
    top_cap.select_set(True)
    indicator_dot.select_set(True)
    bpy.ops.object.join()

    knob = bpy.context.object
    knob.name = "FlutedKnob"

    # Apply remaining modifiers
    for mod in knob.modifiers:
        bpy.ops.object.modifier_apply(modifier=mod.name)

    # Smooth shading
    bpy.ops.object.shade_smooth()

    return knob


def create_smooth_dome_knob():
    """Create smooth dome knob (brass/gold style from references).

    Features:
    - Low profile base
    - Smooth domed top
    - Indicator dot on top
    - Subtle concentric rings (optional detail)
    """
    # Base disc (low profile)
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=128,
        radius=1.0,
        depth=0.15,
        location=(0, 0, 0.075)
    )
    base = bpy.context.object
    base.name = "SmoothBase"

    # Chamfer edge
    bevel = base.modifiers.new(name="Bevel", type='BEVEL')
    bevel.width = 0.05
    bevel.segments = 4
    bpy.ops.object.modifier_apply(modifier="Bevel")

    # Domed top (hemisphere)
    bpy.ops.mesh.primitive_uv_sphere_add(
        segments=128,
        ring_count=64,
        radius=0.90,
        location=(0, 0, 0.15)
    )
    dome = bpy.context.object
    dome.name = "Dome"
    dome.scale.z = 0.6  # Flatten for subtle dome

    # Cut bottom half
    bpy.ops.mesh.primitive_cube_add(
        size=3.0,
        location=(0, 0, -0.5)
    )
    cutter = bpy.context.object
    bpy.context.view_layer.objects.active = dome
    boolean = dome.modifiers.new(name="CutBottom", type='BOOLEAN')
    boolean.operation = 'DIFFERENCE'
    boolean.object = cutter
    bpy.ops.object.modifier_apply(modifier="CutBottom")
    bpy.data.objects.remove(cutter, do_unlink=True)

    # Indicator dot (small raised cylinder)
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=32,
        radius=0.06,
        depth=0.04,
        location=(0, 0.50, 0.72)
    )
    indicator = bpy.context.object
    indicator.name = "IndicatorDot"

    # Smooth indicator edges
    bevel_ind = indicator.modifiers.new(name="Bevel", type='BEVEL')
    bevel_ind.width = 0.01
    bevel_ind.segments = 3
    bpy.ops.object.modifier_apply(modifier="Bevel")

    # Join parts
    bpy.context.view_layer.objects.active = base
    base.select_set(True)
    dome.select_set(True)
    indicator.select_set(True)
    bpy.ops.object.join()

    knob = bpy.context.object
    knob.name = "SmoothDomeKnob"

    bpy.ops.object.shade_smooth()

    return knob


def create_pointer_knob():
    """Create compact pointer knob with line indicator.

    Features:
    - Compact cylindrical body
    - Beveled edges
    - Engraved line pointer
    - Scale ring optional
    """
    # Main body (compact cylinder)
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=128,
        radius=0.70,
        depth=0.40,
        location=(0, 0, 0.20)
    )
    body = bpy.context.object
    body.name = "PointerBody"

    # Heavy bevel on top and bottom edges
    bevel = body.modifiers.new(name="EdgeBevel", type='BEVEL')
    bevel.width = 0.08
    bevel.segments = 4
    bpy.ops.object.modifier_apply(modifier="EdgeBevel")

    # Pointer line (engraved)
    bpy.ops.mesh.primitive_cube_add(
        size=1.0,
        location=(0.35, 0, 0.41)
    )
    pointer_line = bpy.context.object
    pointer_line.scale = (0.40, 0.03, 0.02)

    # Boolean subtract for engraved look
    bpy.context.view_layer.objects.active = body
    boolean = body.modifiers.new(name="PointerEngrave", type='BOOLEAN')
    boolean.operation = 'DIFFERENCE'
    boolean.object = pointer_line
    bpy.ops.object.modifier_apply(modifier="PointerEngrave")
    bpy.data.objects.remove(pointer_line, do_unlink=True)

    # Optional: Add subtle grip rings
    for i in range(3):
        z_pos = 0.10 + (i * 0.10)
        bpy.ops.mesh.primitive_torus_add(
            major_radius=0.71,
            minor_radius=0.015,
            major_segments=128,
            minor_segments=16,
            location=(0, 0, z_pos)
        )
        ring = bpy.context.object

        # Boolean subtract for engraved rings
        bpy.context.view_layer.objects.active = body
        boolean = body.modifiers.new(name=f"GripRing{i}", type='BOOLEAN')
        boolean.operation = 'DIFFERENCE'
        boolean.object = ring
        bpy.ops.object.modifier_apply(modifier=boolean.name)
        bpy.data.objects.remove(ring, do_unlink=True)

    knob = body
    knob.name = "PointerKnob"

    bpy.ops.object.shade_smooth()

    return knob


def create_led_ring():
    """Create LED ring with warm amber emission beneath knob cap.

    Creates a recessed channel design with rounded profile for realistic
    LED housing appearance, similar to vintage control panel indicators.
    """
    knob_radius = 1.0  # Match base body radius
    major_radius = knob_radius * 1.15  # 1.15 for positioning around knob
    minor_radius = 0.08  # Larger profile for visible LED channel

    bpy.ops.mesh.primitive_torus_add(
        major_radius=major_radius,
        minor_radius=minor_radius,
        major_segments=128,  # Increased for smooth circle
        minor_segments=32,   # Increased for smooth profile
        location=(0, 0, 0.12)  # Below cap, above base
    )
    led_ring = bpy.context.object
    led_ring.name = "LEDRing"

    # Add subdivision surface for smooth, organic appearance
    subsurf = led_ring.modifiers.new(name="Subdivision", type='SUBSURF')
    subsurf.levels = 2
    subsurf.render_levels = 3

    # Add bevel for refined edges
    bevel = led_ring.modifiers.new(name="Bevel", type='BEVEL')
    bevel.width = 0.005
    bevel.segments = 4

    # Apply modifiers for clean geometry
    bpy.context.view_layer.objects.active = led_ring
    bpy.ops.object.modifier_apply(modifier="Subdivision")
    bpy.ops.object.modifier_apply(modifier="Bevel")

    return led_ring


def create_led_emission_material():
    """Create emission material for LED ring with warm amber glow."""
    mat = bpy.data.materials.new(name="LED_Emission")
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()

    # Output node
    output = nodes.new(type='ShaderNodeOutputMaterial')
    output.location = (400, 0)

    # Mix shader for transparency control
    mix = nodes.new(type='ShaderNodeMixShader')
    mix.location = (200, 0)
    mix.inputs['Fac'].default_value = 0.3  # 30% transparent, 70% emissive

    # Transparent shader
    transparent = nodes.new(type='ShaderNodeBsdfTransparent')
    transparent.location = (0, -100)

    # Emission shader
    emission = nodes.new(type='ShaderNodeEmission')
    emission.location = (0, 100)
    emission.inputs['Color'].default_value = (1.0, 0.6, 0.2, 1.0)  # Warm amber
    emission.inputs['Strength'].default_value = 3.0

    # Connect nodes
    links.new(transparent.outputs['BSDF'], mix.inputs[1])
    links.new(emission.outputs['Emission'], mix.inputs[2])
    links.new(mix.outputs['Shader'], output.inputs['Surface'])

    return mat


def create_panel_bezel():
    """Create panel bezel/mounting plate around knob.

    Creates a recessed frame with circular cutout for the knob,
    giving the appearance of a professional control panel mounting.
    """
    # Outer panel square
    bpy.ops.mesh.primitive_plane_add(size=2.8, location=(0, 0, -0.05))
    bezel = bpy.context.object
    bezel.name = "PanelBezel"

    # Convert to mesh for boolean operations
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.mesh.extrude_region_move(TRANSFORM_OT_translate={"value": (0, 0, 0.08)})
    bpy.ops.object.mode_set(mode='OBJECT')

    # Create circular cutout for knob
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=64,
        radius=1.3,
        depth=0.12,
        location=(0, 0, -0.01)
    )
    cutout = bpy.context.object

    # Boolean modifier to cut hole
    bpy.context.view_layer.objects.active = bezel
    boolean = bezel.modifiers.new(name="KnobCutout", type='BOOLEAN')
    boolean.operation = 'DIFFERENCE'
    boolean.object = cutout
    bpy.ops.object.modifier_apply(modifier="KnobCutout")

    # Bevel edges for refined look
    bevel = bezel.modifiers.new(name="Bevel", type='BEVEL')
    bevel.width = 0.02
    bevel.segments = 4
    bpy.ops.object.modifier_apply(modifier="Bevel")

    # Cleanup cutout object
    bpy.data.objects.remove(cutout, do_unlink=True)

    return bezel


def create_led_bloom():
    """Create soft bloom/glow halo effect around LED ring.

    Renders as a large, soft gradient for compositing with
    additive blending to create LED light spillage effect.
    """
    knob_radius = 1.0
    bloom_radius = knob_radius * 1.4  # Larger than LED ring

    bpy.ops.mesh.primitive_plane_add(size=bloom_radius * 2, location=(0, 0, 0.12))
    bloom = bpy.context.object
    bloom.name = "LEDBloom"

    # Rotate to face camera
    bloom.rotation_euler = (0, 0, 0)

    return bloom


def create_led_bloom_material():
    """Create soft gradient emission material for bloom effect."""
    mat = bpy.data.materials.new(name="LED_Bloom")
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()

    # Output node
    output = nodes.new(type='ShaderNodeOutputMaterial')
    output.location = (600, 0)

    # Mix shader for transparency
    mix = nodes.new(type='ShaderNodeMixShader')
    mix.location = (400, 0)

    # Transparent shader
    transparent = nodes.new(type='ShaderNodeBsdfTransparent')
    transparent.location = (200, -100)

    # Emission shader
    emission = nodes.new(type='ShaderNodeEmission')
    emission.location = (200, 100)
    emission.inputs['Color'].default_value = (1.0, 0.6, 0.2, 1.0)  # Match LED color
    emission.inputs['Strength'].default_value = 0.8  # Subtle glow

    # Radial gradient for soft falloff
    gradient = nodes.new(type='ShaderNodeTexGradient')
    gradient.location = (-200, 0)
    gradient.gradient_type = 'RADIAL'

    # Color ramp for soft edge
    ramp = nodes.new(type='ShaderNodeValToRGB')
    ramp.location = (0, 0)
    ramp.color_ramp.elements[0].position = 0.3  # Start glow
    ramp.color_ramp.elements[1].position = 0.8  # End glow
    ramp.color_ramp.elements[0].color = (1, 1, 1, 1)  # Center: full
    ramp.color_ramp.elements[1].color = (0, 0, 0, 1)  # Edge: transparent

    # Connect nodes
    links.new(gradient.outputs['Fac'], ramp.inputs['Fac'])
    links.new(transparent.outputs['BSDF'], mix.inputs[1])
    links.new(emission.outputs['Emission'], mix.inputs[2])
    links.new(ramp.outputs['Color'], mix.inputs['Fac'])
    links.new(mix.outputs['Shader'], output.inputs['Surface'])

    return mat


def create_text_overlay():
    """Create text layer for parameter labels.

    Renders parameter name text above the knob for compositing.
    In practice, this would be replaced with dynamic text rendering
    in the JUCE UI layer, but useful for previews.
    """
    # Create text object
    bpy.ops.object.text_add(location=(0, 1.8, 0.2))
    text_obj = bpy.context.object
    text_obj.name = "ParameterLabel"

    # Set text properties
    text_obj.data.body = "TIME"
    text_obj.data.align_x = 'CENTER'
    text_obj.data.align_y = 'CENTER'
    text_obj.data.size = 0.25
    text_obj.data.extrude = 0.02  # Give it depth

    # Rotate to face camera
    text_obj.rotation_euler = (math.radians(90), 0, 0)

    # Convert to mesh for consistent rendering
    bpy.ops.object.convert(target='MESH')

    return text_obj


def create_text_material():
    """Create metallic material for engraved text."""
    mat = bpy.data.materials.new(name="Text_Material")
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()

    output = nodes.new(type='ShaderNodeOutputMaterial')
    output.location = (400, 0)

    bsdf = nodes.new(type='ShaderNodeBsdfPrincipled')
    bsdf.location = (0, 0)
    bsdf.inputs['Base Color'].default_value = (0.8, 0.8, 0.82, 1.0)  # Light gray
    bsdf.inputs['Roughness'].default_value = 0.4
    bsdf.inputs['Metallic'].default_value = 0.8

    links.new(bsdf.outputs['BSDF'], output.inputs['Surface'])

    return mat


def create_panel_bezel_material():
    """Create dark brushed metal material for panel bezel."""
    mat = bpy.data.materials.new(name="Bezel_Material")
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()

    output = nodes.new(type='ShaderNodeOutputMaterial')
    output.location = (400, 0)

    bsdf = nodes.new(type='ShaderNodeBsdfPrincipled')
    bsdf.location = (0, 0)
    bsdf.inputs['Base Color'].default_value = (0.15, 0.15, 0.16, 1.0)  # Dark gray
    bsdf.inputs['Roughness'].default_value = 0.5
    bsdf.inputs['Metallic'].default_value = 0.6

    links.new(bsdf.outputs['BSDF'], output.inputs['Surface'])

    return mat


# === ENVIRONMENTAL EFFECTS ===

def create_shadow_catcher():
    """Create invisible plane to catch shadows."""
    bpy.ops.mesh.primitive_plane_add(size=4.0, location=(0, 0, -0.2))
    plane = bpy.context.object
    plane.name = "ShadowCatcher"

    # Blender 5.0+ API change: is_shadow_catcher moved to object.is_shadow_catcher
    if hasattr(plane, 'is_shadow_catcher'):
        plane.is_shadow_catcher = True
    elif hasattr(plane.cycles, 'is_shadow_catcher'):
        plane.cycles.is_shadow_catcher = True

    return plane


def render_layer(layer_obj, output_path, hide_others=True):
    """Render a single layer with alpha channel."""
    # Hide/show objects
    for obj in bpy.data.objects:
        if obj.type == 'MESH' and obj != layer_obj:
            obj.hide_render = hide_others
        elif obj.type != 'LIGHT' and obj.type != 'CAMERA':
            obj.hide_render = False

    layer_obj.hide_render = False

    # Render
    bpy.context.scene.render.filepath = str(output_path)
    bpy.ops.render.render(write_still=True)

    print(f"✅ Rendered: {output_path}")


def render_shadow_layer(objects, output_path):
    """Render shadow pass with transparent background and shadow catcher.

    Renders darker, more pronounced shadows by reducing ambient lighting
    during the shadow pass.
    """
    # Show all main objects
    for obj in objects:
        obj.hide_render = False

    # Keep transparent background enabled for proper alpha channel
    # Shadow catcher will capture shadows on transparent background
    bpy.context.scene.render.film_transparent = True

    # Store original lighting values
    original_strength = bpy.context.scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value
    original_key_energy = None
    original_fill_energy = None

    # Find and adjust key lights for more pronounced shadows
    for obj in bpy.data.objects:
        if obj.type == 'LIGHT':
            if obj.name == 'KeyLight':
                original_key_energy = obj.data.energy
                obj.data.energy = 200  # Increase from 150 for stronger shadows
            elif obj.name == 'FillLight':
                original_fill_energy = obj.data.energy
                obj.data.energy = 30  # Decrease from 60 to reduce shadow fill

    # Reduce world ambient to make shadows darker
    bpy.context.scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value = 0.05

    bpy.context.scene.render.filepath = str(output_path)
    bpy.ops.render.render(write_still=True)

    # Restore original lighting
    bpy.context.scene.world.node_tree.nodes['Background'].inputs['Strength'].default_value = original_strength
    for obj in bpy.data.objects:
        if obj.type == 'LIGHT':
            if obj.name == 'KeyLight' and original_key_energy:
                obj.data.energy = original_key_energy
            elif obj.name == 'FillLight' and original_fill_energy:
                obj.data.energy = original_fill_energy

    print(f"✅ Rendered shadow layer: {output_path}")


# === MAIN GENERATION ===

def main():
    """Generate enhanced knob layers with environmental effects."""
    # Parse arguments
    args = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []

    material_name = "granite"
    indicator_type = "brushed_aluminum"
    knob_style = "original"  # original, fluted, dome, pointer
    output_dir = Path("~/Documents/monument-reverb/assets/ui/knobs_enhanced").expanduser()
    resolution = 512
    samples = 256

    i = 0
    while i < len(args):
        if args[i] == "--material" and i + 1 < len(args):
            material_name = args[i + 1]
            i += 2
        elif args[i] == "--indicator" and i + 1 < len(args):
            indicator_type = args[i + 1]
            i += 2
        elif args[i] == "--style" and i + 1 < len(args):
            knob_style = args[i + 1]
            i += 2
        elif args[i] == "--out" and i + 1 < len(args):
            output_dir = Path(args[i + 1])
            i += 2
        elif args[i] == "--size" and i + 1 < len(args):
            resolution = int(args[i + 1])
            i += 2
        elif args[i] == "--samples" and i + 1 < len(args):
            samples = int(args[i + 1])
            i += 2
        else:
            i += 1

    # Organize output by style subdirectories (prevents overwriting)
    style_output_dir = output_dir / knob_style
    style_output_dir.mkdir(parents=True, exist_ok=True)

    print("=" * 70)
    print("🎨 Monument Reverb - Enhanced Knob Generation with Environmental FX")
    print("=" * 70)
    print(f"📁 Output: {style_output_dir}")
    print(f"🎨 Material: {material_name}")
    print(f"🎛️  Knob Style: {knob_style}")
    print(f"📐 Resolution: {resolution}x{resolution}")
    print(f"🔬 Samples: {samples}")
    print("=" * 70 + "\n")

    # Setup scene
    print("Setting up scene...")
    clear_scene()
    setup_camera()
    setup_enhanced_lighting()
    setup_render_settings(resolution=resolution, samples=samples, transparent=True)

    # Create shadow catcher
    shadow_catcher = create_shadow_catcher()

    # Create geometry based on selected style
    print(f"\nCreating {knob_style} knob geometry with {material_name} material...")
    bezel = create_panel_bezel()

    # Select knob base geometry based on style
    if knob_style == "fluted":
        print("  ↳ Using fluted grip knob (vintage white style)")
        base = create_fluted_grip_knob()
        # Fluted knob has integrated indicator, so create dummy for compatibility
        indicator = None
        ring = None
        cap = None
    elif knob_style == "dome":
        print("  ↳ Using smooth dome knob (brass style)")
        base = create_smooth_dome_knob()
        # Dome knob has integrated indicator, so create dummy for compatibility
        indicator = None
        ring = None
        cap = None
    elif knob_style == "pointer":
        print("  ↳ Using pointer knob (compact style)")
        base = create_pointer_knob()
        # Pointer knob has integrated indicator, so create dummy for compatibility
        indicator = None
        ring = None
        cap = None
    else:  # "original" or default
        print("  ↳ Using original segmented knob design")
        base = create_base_body()
        indicator = create_indicator()
        ring = create_detail_ring()
        cap = create_center_cap()

    led_ring = create_led_ring()
    bloom = create_led_bloom()
    text = create_text_overlay()

    # Assign materials
    bezel_mat = create_panel_bezel_material()
    bezel.data.materials.append(bezel_mat)

    base_mat = create_material(material_name)
    base.data.materials.append(base_mat)

    # Only assign materials to components that exist
    if indicator:
        indicator_mat = create_indicator_material(indicator_type)
        indicator.data.materials.append(indicator_mat)

    if ring:
        ring_mat = create_material(material_name)
        ring.data.materials.append(ring_mat)

    if cap:
        cap_mat = create_indicator_material(indicator_type)
        cap.data.materials.append(cap_mat)

    led_mat = create_led_emission_material()
    led_ring.data.materials.append(led_mat)

    bloom_mat = create_led_bloom_material()
    bloom.data.materials.append(bloom_mat)

    text_mat = create_text_material()
    text.data.materials.append(text_mat)

    # Render individual layers
    print("\nRendering layers...")
    render_layer(bezel, style_output_dir / "layer_0_panel_bezel.png")
    render_layer(base, style_output_dir / f"layer_1_base_{material_name}.png")

    # Only render components that exist
    if ring:
        render_layer(ring, style_output_dir / f"layer_2_ring_{material_name}.png")
    if indicator:
        render_layer(indicator, style_output_dir / f"layer_3_indicator_{indicator_type}.png")
    if cap:
        render_layer(cap, style_output_dir / f"layer_4_cap_{indicator_type}.png")

    render_layer(led_ring, style_output_dir / "layer_5_led_ring.png")
    render_layer(bloom, style_output_dir / "layer_6_led_bloom.png")
    render_layer(text, style_output_dir / "layer_7_text_label.png")

    # Render environmental effects
    print("\nRendering environmental effects...")
    # Build shadow layer object list (only include existing objects)
    shadow_objects = [base]
    if ring:
        shadow_objects.append(ring)
    if indicator:
        shadow_objects.append(indicator)
    render_shadow_layer(shadow_objects, style_output_dir / f"fx_shadow_{material_name}.png")

    # Cleanup shadow catcher
    bpy.data.objects.remove(shadow_catcher, do_unlink=True)

    print("\n" + "=" * 70)
    print("✨ Enhanced knob generation complete!")
    print(f"📦 Layers + FX saved to: {style_output_dir}")
    print(f"\n🎛️  Knob Style: {knob_style}")
    print("\nGenerated files:")
    print("  - layer_0_panel_bezel.png (static, background frame)")
    print(f"  - layer_1_base_{material_name}.png (knob body, rotates)")

    if knob_style == "original":
        print(f"  - layer_2_ring_{material_name}.png (static)")
        print(f"  - layer_3_indicator_{indicator_type}.png (rotates)")
        print(f"  - layer_4_cap_{indicator_type}.png (static)")
    else:
        print(f"    ↳ {knob_style} knob has integrated indicator/cap")

    print("  - layer_5_led_ring.png (static, warm amber emission)")
    print("  - layer_6_led_bloom.png (static, additive blend glow)")
    print("  - layer_7_text_label.png (static, parameter name)")
    print(f"  - fx_shadow_{material_name}.png (static, multiply blend)")
    print("\nNext: Preview composite with preview_knob_composite_enhanced.py")
    print("=" * 70)


if __name__ == "__main__":
    main()
