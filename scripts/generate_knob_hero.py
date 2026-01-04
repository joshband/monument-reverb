#!/usr/bin/env python3
"""
Hero Knob Generator - Reference Quality Rendering
Implements advanced shaders to match high-quality reference knob:
- Subsurface scattering for stone
- Anisotropic brass with threading
- HDRI environment lighting
- Multi-level depth architecture
"""

import bpy
import math
import os
import sys

# === CONFIGURATION ===

RENDER_SAMPLES = 512  # High quality
OUTPUT_DIR = "assets/ui/knobs_hero"
RESOLUTION = 1024  # Higher res for detail

# Hero material: Dark weathered stone + aged brass
HERO_MATERIAL = {
    "stone": {
        "base_color": (0.15, 0.15, 0.15, 1.0),  # Dark charcoal
        "roughness": 0.8,
        "metallic": 0.0,
        "subsurface": 0.05,  # Light penetration
        "subsurface_radius": (0.2, 0.2, 0.2),
        "subsurface_color": (0.3, 0.3, 0.3, 1.0),
        "specular": 0.3,
        "weathering_scale": 8.0,
    },
    "aged_brass": {
        "base_color": (0.60, 0.48, 0.35, 1.0),  # Aged brass tone
        "roughness": 0.4,
        "metallic": 0.9,
        "anisotropic": 0.8,  # Threading effect
        "anisotropic_rotation": 0.0,  # Radial
        "patina_scale": 15.0,
    },
    "inner_glow": {
        "emission_color": (0.8, 0.6, 0.3, 1.0),  # Warm inner glow
        "emission_strength": 2.5,
        "roughness": 0.3,
    }
}

# === SCENE SETUP ===

def setup_scene():
    """Initialize Blender scene with HDRI lighting."""
    # Clear existing objects
    bpy.ops.object.select_all(action='SELECT')
    bpy.ops.object.delete()

    # Clear materials
    for mat in bpy.data.materials:
        bpy.data.materials.remove(mat)

    scene = bpy.context.scene
    scene.render.engine = 'CYCLES'
    scene.cycles.samples = RENDER_SAMPLES
    scene.cycles.use_adaptive_sampling = True
    scene.cycles.adaptive_threshold = 0.01

    # Enable GPU if available
    scene.cycles.device = 'GPU'

    # Render settings
    scene.render.resolution_x = RESOLUTION
    scene.render.resolution_y = RESOLUTION
    scene.render.film_transparent = True
    scene.render.image_settings.file_format = 'PNG'
    scene.render.image_settings.color_mode = 'RGBA'
    scene.render.image_settings.color_depth = '16'

    # World HDRI setup
    setup_hdri_lighting()

    # Camera
    cam_data = bpy.data.cameras.new(name="Camera")
    cam_obj = bpy.data.objects.new(name="Camera", object_data=cam_data)
    scene.collection.objects.link(cam_obj)
    scene.camera = cam_obj
    cam_obj.location = (0, 0, 8.0)
    cam_obj.rotation_euler = (0, 0, 0)
    cam_data.lens = 50

    return scene


def setup_hdri_lighting():
    """Setup HDRI environment for realistic lighting (uses downloaded HDRI if available)."""
    world = bpy.context.scene.world
    world.use_nodes = True
    nodes = world.node_tree.nodes
    links = world.node_tree.links
    nodes.clear()

    # Output
    output = nodes.new(type='ShaderNodeOutputWorld')
    output.location = (600, 0)

    # Background shader
    background = nodes.new(type='ShaderNodeBackground')
    background.location = (400, 0)
    background.inputs['Strength'].default_value = 1.2  # Slightly brighter

    # Try to load downloaded HDRI
    hdri_path = os.path.join(os.path.dirname(__file__), "../assets/textures/hdri/studio.hdr")

    if os.path.exists(hdri_path):
        print(f"✓ Using HDRI: {hdri_path}")

        # Environment texture
        env_tex = nodes.new(type='ShaderNodeTexEnvironment')
        env_tex.location = (0, 0)

        # Load HDRI image
        hdri_image = bpy.data.images.load(hdri_path, check_existing=True)
        env_tex.image = hdri_image

        # Texture coordinate
        tex_coord = nodes.new(type='ShaderNodeTexCoord')
        tex_coord.location = (-400, 0)

        # Mapping node (to rotate HDRI)
        mapping = nodes.new(type='ShaderNodeMapping')
        mapping.location = (-200, 0)
        mapping.inputs['Rotation'].default_value[2] = math.radians(90)  # Rotate 90 degrees

        # Connect
        links.new(tex_coord.outputs['Generated'], mapping.inputs['Vector'])
        links.new(mapping.outputs['Vector'], env_tex.inputs['Vector'])
        links.new(env_tex.outputs['Color'], background.inputs['Color'])

    else:
        print("⚠ HDRI not found, using procedural sky")

        # Sky texture (fallback)
        sky = nodes.new(type='ShaderNodeTexSky')
        sky.location = (0, 0)
        sky.sky_type = 'HOSEK_WILKIE'
        sky.sun_elevation = math.radians(45)
        sky.sun_rotation = math.radians(120)
        sky.turbidity = 3.0
        sky.ground_albedo = 0.3

        links.new(sky.outputs['Color'], background.inputs['Color'])

    links.new(background.outputs['Background'], output.inputs['Surface'])


# === ADVANCED MATERIALS ===

def create_weathered_stone_material():
    """Create stone material with subsurface scattering and weathering."""
    mat = bpy.data.materials.new(name="WeatheredStone")
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()

    props = HERO_MATERIAL["stone"]

    # Output
    output = nodes.new(type='ShaderNodeOutputMaterial')
    output.location = (800, 0)

    # Principled BSDF with subsurface
    bsdf = nodes.new(type='ShaderNodeBsdfPrincipled')
    bsdf.location = (500, 0)
    bsdf.inputs['Roughness'].default_value = props['roughness']
    bsdf.inputs['Metallic'].default_value = props['metallic']
    bsdf.inputs['Specular'].default_value = props['specular']

    # Subsurface scattering
    bsdf.inputs['Subsurface'].default_value = props['subsurface']
    bsdf.inputs['Subsurface Radius'].default_value = props['subsurface_radius']
    bsdf.inputs['Subsurface Color'].default_value = props['subsurface_color']

    # Weathering texture (color variation)
    noise_color = nodes.new(type='ShaderNodeTexNoise')
    noise_color.location = (-700, 200)
    noise_color.inputs['Scale'].default_value = props['weathering_scale']
    noise_color.inputs['Detail'].default_value = 12.0
    noise_color.inputs['Roughness'].default_value = 0.7

    # Color ramp for weathering spots
    ramp = nodes.new(type='ShaderNodeValToRGB')
    ramp.location = (-400, 200)
    base = props['base_color']
    ramp.color_ramp.elements[0].position = 0.4
    ramp.color_ramp.elements[0].color = (base[0] * 0.7, base[1] * 0.7, base[2] * 0.7, 1.0)
    ramp.color_ramp.elements[1].position = 0.6
    ramp.color_ramp.elements[1].color = (base[0] * 1.3, base[1] * 1.3, base[2] * 1.3, 1.0)

    # Add more variation with Voronoi
    voronoi = nodes.new(type='ShaderNodeTexVoronoi')
    voronoi.location = (-700, -100)
    voronoi.inputs['Scale'].default_value = 20.0
    voronoi.feature = 'DISTANCE_TO_EDGE'

    # Mix color variation
    mix_color = nodes.new(type='ShaderNodeMixRGB')
    mix_color.location = (-100, 100)
    mix_color.blend_type = 'MULTIPLY'
    mix_color.inputs['Fac'].default_value = 0.3

    # Surface displacement (fine detail)
    noise_disp = nodes.new(type='ShaderNodeTexNoise')
    noise_disp.location = (-700, -400)
    noise_disp.inputs['Scale'].default_value = 50.0
    noise_disp.inputs['Detail'].default_value = 16.0

    # Bump node
    bump = nodes.new(type='ShaderNodeBump')
    bump.location = (200, -200)
    bump.inputs['Strength'].default_value = 0.4
    bump.inputs['Distance'].default_value = 0.02

    # Displacement node (for actual geometry displacement)
    displacement = nodes.new(type='ShaderNodeDisplacement')
    displacement.location = (500, -400)
    displacement.inputs['Scale'].default_value = 0.02

    # Connect color variation
    links.new(noise_color.outputs['Fac'], ramp.inputs['Fac'])
    links.new(ramp.outputs['Color'], mix_color.inputs['Color1'])
    links.new(voronoi.outputs['Distance'], mix_color.inputs['Color2'])
    links.new(mix_color.outputs['Color'], bsdf.inputs['Base Color'])

    # Connect bump
    links.new(noise_disp.outputs['Fac'], bump.inputs['Height'])
    links.new(bump.outputs['Normal'], bsdf.inputs['Normal'])

    # Connect displacement
    links.new(noise_disp.outputs['Fac'], displacement.inputs['Height'])
    links.new(displacement.outputs['Displacement'], output.inputs['Displacement'])

    # Connect shader
    links.new(bsdf.outputs['BSDF'], output.inputs['Surface'])

    return mat


def create_aged_brass_material():
    """Create aged brass with anisotropic reflections (threading simulation)."""
    mat = bpy.data.materials.new(name="AgedBrass")
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()

    props = HERO_MATERIAL["aged_brass"]

    # Output
    output = nodes.new(type='ShaderNodeOutputMaterial')
    output.location = (800, 0)

    # Principled BSDF with anisotropic
    bsdf = nodes.new(type='ShaderNodeBsdfPrincipled')
    bsdf.location = (500, 0)
    bsdf.inputs['Roughness'].default_value = props['roughness']
    bsdf.inputs['Metallic'].default_value = props['metallic']
    bsdf.inputs['Anisotropic'].default_value = props['anisotropic']
    bsdf.inputs['Anisotropic Rotation'].default_value = props['anisotropic_rotation']

    # Patina (greenish oxidation spots)
    noise_patina = nodes.new(type='ShaderNodeTexNoise')
    noise_patina.location = (-700, 200)
    noise_patina.inputs['Scale'].default_value = props['patina_scale']
    noise_patina.inputs['Detail'].default_value = 8.0

    # Patina color ramp
    ramp_patina = nodes.new(type='ShaderNodeValToRGB')
    ramp_patina.location = (-400, 200)
    base = props['base_color']
    # Add patina color (greenish tint)
    ramp_patina.color_ramp.elements[0].position = 0.5
    ramp_patina.color_ramp.elements[0].color = base
    ramp_patina.color_ramp.elements[1].position = 0.7
    ramp_patina.color_ramp.elements[1].color = (base[0] * 0.8, base[1] * 1.1, base[2] * 0.9, 1.0)

    # Scratches/wear (fine lines)
    scratch = nodes.new(type='ShaderNodeTexWave')
    scratch.location = (-700, -100)
    scratch.wave_type = 'RINGS'  # Circular scratches
    scratch.inputs['Scale'].default_value = 100.0
    scratch.inputs['Distortion'].default_value = 2.0

    # Mix scratches into roughness
    mix_rough = nodes.new(type='ShaderNodeMath')
    mix_rough.location = (200, -200)
    mix_rough.operation = 'ADD'
    mix_rough.inputs[0].default_value = props['roughness']
    mix_rough.use_clamp = True

    # Bump for threading detail
    bump = nodes.new(type='ShaderNodeBump')
    bump.location = (200, -400)
    bump.inputs['Strength'].default_value = 0.6
    bump.inputs['Distance'].default_value = 0.01

    # Threading pattern (wave texture)
    threads = nodes.new(type='ShaderNodeTexWave')
    threads.location = (-400, -400)
    threads.wave_type = 'RINGS'
    threads.inputs['Scale'].default_value = 200.0  # Dense threading
    threads.inputs['Distortion'].default_value = 0.0

    # Connect color with patina
    links.new(noise_patina.outputs['Fac'], ramp_patina.inputs['Fac'])
    links.new(ramp_patina.outputs['Color'], bsdf.inputs['Base Color'])

    # Connect scratches to roughness
    links.new(scratch.outputs['Fac'], mix_rough.inputs[1])
    links.new(mix_rough.outputs['Value'], bsdf.inputs['Roughness'])

    # Connect threading bump
    links.new(threads.outputs['Fac'], bump.inputs['Height'])
    links.new(bump.outputs['Normal'], bsdf.inputs['Normal'])

    # Connect shader
    links.new(bsdf.outputs['BSDF'], output.inputs['Surface'])

    return mat


def create_inner_glow_material():
    """Create emissive material for inner dome glow."""
    mat = bpy.data.materials.new(name="InnerGlow")
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    nodes.clear()

    props = HERO_MATERIAL["inner_glow"]

    # Output
    output = nodes.new(type='ShaderNodeOutputMaterial')
    output.location = (600, 0)

    # Mix shader (emission + diffuse)
    mix = nodes.new(type='ShaderNodeMixShader')
    mix.location = (400, 0)
    mix.inputs['Fac'].default_value = 0.7  # 70% emission, 30% diffuse

    # Emission shader
    emission = nodes.new(type='ShaderNodeEmission')
    emission.location = (0, 100)
    emission.inputs['Color'].default_value = props['emission_color']
    emission.inputs['Strength'].default_value = props['emission_strength']

    # Diffuse for subtle surface detail
    diffuse = nodes.new(type='ShaderNodeBsdfDiffuse')
    diffuse.location = (0, -100)
    diffuse.inputs['Roughness'].default_value = props['roughness']
    diffuse.inputs['Color'].default_value = props['emission_color']

    # Connect
    links.new(emission.outputs['Emission'], mix.inputs[1])
    links.new(diffuse.outputs['BSDF'], mix.inputs[2])
    links.new(mix.outputs['Shader'], output.inputs['Surface'])

    return mat


# === GEOMETRY CREATION ===

def create_hero_knob():
    """Create multi-level hero knob with architectural depth."""

    # Layer 1: Outer stone body (large sphere with flat top)
    bpy.ops.mesh.primitive_uv_sphere_add(
        segments=128,
        ring_count=64,
        radius=1.0,
        location=(0, 0, 0)
    )
    outer_body = bpy.context.object
    outer_body.name = "OuterBody"

    # Flatten top with array of cuts
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.transform.resize(value=(1, 1, 0.4))  # Flatten to dome
    bpy.ops.object.mode_set(mode='OBJECT')

    # Apply subdivision for smooth surface
    subsurf = outer_body.modifiers.new(name="Subdivision", type='SUBSURF')
    subsurf.levels = 2
    subsurf.render_levels = 3

    # Apply weathered stone material
    mat_stone = create_weathered_stone_material()
    outer_body.data.materials.append(mat_stone)

    # Layer 2: Outer brass ring (threaded)
    bpy.ops.mesh.primitive_torus_add(
        major_radius=0.92,
        minor_radius=0.08,
        major_segments=128,
        minor_segments=32,
        location=(0, 0, 0.15)
    )
    outer_ring = bpy.context.object
    outer_ring.name = "OuterBrassRing"

    # Apply aged brass material
    mat_brass = create_aged_brass_material()
    outer_ring.data.materials.append(mat_brass)

    # Layer 3: Middle recession (dark stone channel)
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=128,
        radius=0.75,
        depth=0.3,
        location=(0, 0, 0.05)
    )
    middle_channel = bpy.context.object
    middle_channel.name = "MiddleChannel"
    middle_channel.data.materials.append(mat_stone)

    # Layer 4: Inner brass ring (threaded, smaller)
    bpy.ops.mesh.primitive_torus_add(
        major_radius=0.68,
        minor_radius=0.06,
        major_segments=128,
        minor_segments=32,
        location=(0, 0, 0.1)
    )
    inner_ring = bpy.context.object
    inner_ring.name = "InnerBrassRing"
    inner_ring.data.materials.append(mat_brass)

    # Layer 5: Inner dome (glowing center)
    bpy.ops.mesh.primitive_uv_sphere_add(
        segments=64,
        ring_count=32,
        radius=0.5,
        location=(0, 0, 0.0)
    )
    inner_dome = bpy.context.object
    inner_dome.name = "InnerDome"

    # Flatten slightly
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.transform.resize(value=(1, 1, 0.6))
    bpy.ops.object.mode_set(mode='OBJECT')

    # Apply glow material
    mat_glow = create_inner_glow_material()
    inner_dome.data.materials.append(mat_glow)

    # Join all into single object for rendering
    bpy.ops.object.select_all(action='DESELECT')
    outer_body.select_set(True)
    outer_ring.select_set(True)
    middle_channel.select_set(True)
    inner_ring.select_set(True)
    inner_dome.select_set(True)
    bpy.context.view_layer.objects.active = outer_body
    bpy.ops.object.join()

    return outer_body


# === MAIN EXECUTION ===

def main():
    """Generate hero knob."""
    print("=== Hero Knob Generator ===")
    print(f"Samples: {RENDER_SAMPLES}")
    print(f"Resolution: {RESOLUTION}x{RESOLUTION}")

    # Setup scene
    scene = setup_scene()

    # Create hero knob
    knob = create_hero_knob()

    # Render single frame
    output_path = os.path.join(OUTPUT_DIR, "hero_knob_test.png")
    os.makedirs(OUTPUT_DIR, exist_ok=True)
    scene.render.filepath = output_path

    print(f"Rendering to: {output_path}")
    bpy.ops.render.render(write_still=True)
    print("✅ Render complete!")


if __name__ == "__main__":
    main()
