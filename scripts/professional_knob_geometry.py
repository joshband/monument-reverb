"""
Professional Knob Geometry - Enhanced photorealistic models
Based on vintage audio equipment (Neve, API, SSL-style hardware)
"""

import bpy
import math


def create_professional_base_body():
    """Create professional knob base with knurled grip surface.

    Features:
    - Multi-tiered construction (base, grip ring, top platform)
    - Knurled/diamond grip pattern for tactile feedback
    - Chamfered edges for premium look
    - Proper proportions matching studio hardware
    """
    # Tier 1: Bottom base (wider, provides stability)
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=128,  # High poly for smoothness
        radius=1.1,
        depth=0.15,
        location=(0, 0, 0.075)
    )
    base_bottom = bpy.context.object
    base_bottom.name = "BaseBottom"

    # Add chamfer to bottom edge
    bevel = base_bottom.modifiers.new(name="BottomChamfer", type='BEVEL')
    bevel.width = 0.08
    bevel.segments = 4
    bevel.limit_method = 'ANGLE'
    bevel.angle_limit = math.radians(30)

    # Tier 2: Middle grip section (knurled surface)
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=128,
        radius=1.0,
        depth=0.35,
        location=(0, 0, 0.325)
    )
    grip_section = bpy.context.object
    grip_section.name = "GripSection"

    # Create knurled pattern using displacement
    # Method 1: Diamond pattern via array of small indentations
    # TODO: Re-enable after testing basic geometry
    # create_knurled_pattern(grip_section, pattern_type='diamond')

    # Tier 3: Top platform (smaller, houses indicator/cap)
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=128,
        radius=0.95,
        depth=0.08,
        location=(0, 0, 0.54)
    )
    top_platform = bpy.context.object
    top_platform.name = "TopPlatform"

    # Chamfer top edge
    bevel_top = top_platform.modifiers.new(name="TopChamfer", type='BEVEL')
    bevel_top.width = 0.04
    bevel_top.segments = 3

    # Join all tiers
    bpy.context.view_layer.objects.active = base_bottom
    base_bottom.select_set(True)
    grip_section.select_set(True)
    top_platform.select_set(True)
    bpy.ops.object.join()

    base = bpy.context.object
    base.name = "BaseBody"

    # Apply all modifiers for clean mesh
    for mod in base.modifiers:
        bpy.ops.object.modifier_apply(modifier=mod.name)

    # Add edge split for hard edges where needed
    edge_split = base.modifiers.new(name="EdgeSplit", type='EDGE_SPLIT')
    edge_split.split_angle = math.radians(30)

    return base


def create_knurled_pattern(obj, pattern_type='diamond', density=48):
    """Add knurled grip pattern to cylinder surface.

    Args:
        obj: Cylinder object to add pattern to
        pattern_type: 'diamond', 'vertical', or 'horizontal'
        density: Number of pattern repeats around circumference
    """
    if pattern_type == 'diamond':
        # Create diamond knurl using two helical arrays of indentations
        # Right-hand helix
        bpy.ops.mesh.primitive_cube_add(
            size=0.025,
            location=(1.0, 0, 0.175)
        )
        knurl_r = bpy.context.object
        knurl_r.scale = (0.8, 1.0, 0.6)

        # Array around circumference
        array_circ = knurl_r.modifiers.new(name="CircArray", type='ARRAY')
        array_circ.count = density
        array_circ.use_object_offset = True

        # Create empty for rotation
        bpy.ops.object.empty_add(location=(0, 0, 0))
        empty_rot = bpy.context.object
        empty_rot.rotation_euler.z = math.radians(360 / density)
        array_circ.offset_object = empty_rot

        # Array vertically with slight rotation for helix
        array_vert = knurl_r.modifiers.new(name="VertArray", type='ARRAY')
        array_vert.count = 12
        array_vert.relative_offset_displace = (0, 0, 0.08)
        array_vert.use_constant_offset = True
        array_vert.constant_offset_displace = (0.015, 0, 0.03)

        # Boolean subtract from main body
        bpy.context.view_layer.objects.active = obj
        boolean_r = obj.modifiers.new(name="KnurlRight", type='BOOLEAN')
        boolean_r.operation = 'DIFFERENCE'
        boolean_r.object = knurl_r

        # Left-hand helix (mirror of right)
        knurl_l = knurl_r.copy()
        knurl_l.data = knurl_r.data.copy()
        knurl_l.location.x = -1.0
        bpy.context.collection.objects.link(knurl_l)

        boolean_l = obj.modifiers.new(name="KnurlLeft", type='BOOLEAN')
        boolean_l.operation = 'DIFFERENCE'
        boolean_l.object = knurl_l

        # Apply modifiers
        bpy.ops.object.modifier_apply(modifier="KnurlRight")
        bpy.ops.object.modifier_apply(modifier="KnurlLeft")

        # Cleanup
        bpy.data.objects.remove(knurl_r, do_unlink=True)
        bpy.data.objects.remove(knurl_l, do_unlink=True)
        bpy.data.objects.remove(empty_rot, do_unlink=True)


def create_professional_indicator():
    """Create indicator pointer with depth and chamfers.

    Features:
    - Recessed channel for pointer
    - Beveled edges
    - Proper depth hierarchy
    - Optional luminous tip
    """
    # Main pointer bar
    bpy.ops.mesh.primitive_cube_add(location=(0.45, 0, 0.58))
    indicator = bpy.context.object
    indicator.name = "IndicatorPointer"
    indicator.scale = (0.40, 0.05, 0.04)

    # Add bevels for rounded edges
    bevel = indicator.modifiers.new(name="Bevel", type='BEVEL')
    bevel.width = 0.008
    bevel.segments = 4
    bevel.limit_method = 'WEIGHT'

    # Mark specific edges for bevel (top/bottom edges only)
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.mesh.region_to_loop()
    bpy.ops.transform.edge_bevelweight(value=1.0)
    bpy.ops.object.mode_set(mode='OBJECT')

    # Apply bevel
    bpy.ops.object.modifier_apply(modifier="Bevel")

    # Create indicator tip (luminous dot)
    bpy.ops.mesh.primitive_uv_sphere_add(
        segments=32,
        ring_count=16,
        radius=0.04,
        location=(0.75, 0, 0.58)
    )
    tip = bpy.context.object
    tip.name = "IndicatorTip"

    # Join tip with pointer
    bpy.context.view_layer.objects.active = indicator
    indicator.select_set(True)
    tip.select_set(True)
    bpy.ops.object.join()

    return indicator


def create_professional_detail_ring():
    """Create professional-grade detail ring with enhanced tick marks.

    Features:
    - Precision tick marks (major/minor divisions)
    - Engraved appearance
    - Proper depth and shadowing
    - Optional numbering positions
    """
    # Main ring body
    bpy.ops.mesh.primitive_torus_add(
        major_radius=1.05,
        minor_radius=0.06,
        major_segments=256,
        minor_segments=48,
        location=(0, 0, 0.58)
    )
    ring = bpy.context.object
    ring.name = "DetailRing"

    # Add major tick marks (12 positions - like clock)
    for i in range(12):
        angle = math.radians(i * 30)
        x = 1.05 * math.cos(angle)
        y = 1.05 * math.sin(angle)

        # Major tick (taller)
        bpy.ops.mesh.primitive_cube_add(location=(x, y, 0.58))
        tick = bpy.context.object
        tick.scale = (0.025, 0.04, 0.12)
        tick.rotation_euler.z = angle

        # Bevel tick edges
        bevel = tick.modifiers.new(name="Bevel", type='BEVEL')
        bevel.width = 0.004
        bevel.segments = 2
        bpy.ops.object.modifier_apply(modifier="Bevel")

        # Boolean subtract (engraved look)
        bpy.context.view_layer.objects.active = ring
        boolean = ring.modifiers.new(name=f"Tick{i}", type='BOOLEAN')
        boolean.operation = 'DIFFERENCE'
        boolean.object = tick
        bpy.ops.object.modifier_apply(modifier=boolean.name)

        bpy.data.objects.remove(tick, do_unlink=True)

    # Add minor tick marks (between majors)
    for i in range(12):
        angle = math.radians(i * 30 + 15)  # Offset by 15 degrees
        x = 1.05 * math.cos(angle)
        y = 1.05 * math.sin(angle)

        # Minor tick (shorter)
        bpy.ops.mesh.primitive_cube_add(location=(x, y, 0.58))
        tick = bpy.context.object
        tick.scale = (0.02, 0.03, 0.08)
        tick.rotation_euler.z = angle

        # Boolean subtract
        bpy.context.view_layer.objects.active = ring
        boolean = ring.modifiers.new(name=f"MinorTick{i}", type='BOOLEAN')
        boolean.operation = 'DIFFERENCE'
        boolean.object = tick
        bpy.ops.object.modifier_apply(modifier=boolean.name)

        bpy.data.objects.remove(tick, do_unlink=True)

    return ring


def create_professional_center_cap():
    """Create center cap with concave surface (ergonomic).

    Features:
    - Concave center for finger grip
    - Chamfered outer edge
    - Optional logo/text area
    - Smooth finish
    """
    # Main cap disc
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=128,
        radius=0.20,
        depth=0.08,
        location=(0, 0, 0.62)
    )
    cap = bpy.context.object
    cap.name = "CenterCap"

    # Add concave center using simple deform or boolean
    bpy.ops.mesh.primitive_uv_sphere_add(
        segments=64,
        ring_count=32,
        radius=0.18,
        location=(0, 0, 0.62)
    )
    sphere_cutter = bpy.context.object
    sphere_cutter.scale.z = 0.6  # Flatten for subtle concave

    # Boolean subtract for concave shape
    bpy.context.view_layer.objects.active = cap
    boolean = cap.modifiers.new(name="Concave", type='BOOLEAN')
    boolean.operation = 'DIFFERENCE'
    boolean.object = sphere_cutter
    bpy.ops.object.modifier_apply(modifier="Concave")

    bpy.data.objects.remove(sphere_cutter, do_unlink=True)

    # Outer edge bevel/chamfer
    bevel = cap.modifiers.new(name="EdgeBevel", type='BEVEL')
    bevel.width = 0.03
    bevel.segments = 5
    bpy.ops.object.modifier_apply(modifier="EdgeBevel")

    # Add subtle edge loop for definition
    bpy.ops.object.mode_set(mode='EDIT')
    bpy.ops.mesh.select_all(action='SELECT')
    bpy.ops.mesh.subdivide(number_cuts=1)
    bpy.ops.object.mode_set(mode='OBJECT')

    return cap


def create_professional_led_ring():
    """Create recessed LED ring channel with realistic housing.

    Features:
    - Recessed channel (like real LED mounting)
    - Frosted diffuser surface
    - Proper depth for light spillage
    - Warm amber color characteristic
    """
    knob_radius = 1.0
    led_major_radius = knob_radius * 1.20  # Outside knob perimeter
    led_channel_depth = 0.10

    # Outer ring (housing)
    bpy.ops.mesh.primitive_torus_add(
        major_radius=led_major_radius,
        minor_radius=0.10,
        major_segments=256,
        minor_segments=48,
        location=(0, 0, 0.15)
    )
    led_housing = bpy.context.object
    led_housing.name = "LEDHousing"

    # Inner LED strip (actual emitter)
    bpy.ops.mesh.primitive_torus_add(
        major_radius=led_major_radius - 0.02,
        minor_radius=0.06,
        major_segments=256,
        minor_segments=32,
        location=(0, 0, 0.15)
    )
    led_emitter = bpy.context.object
    led_emitter.name = "LEDEmitter"

    # Boolean subtract emitter from housing for recessed look
    bpy.context.view_layer.objects.active = led_housing
    boolean = led_housing.modifiers.new(name="Recess", type='BOOLEAN')
    boolean.operation = 'DIFFERENCE'
    boolean.object = led_emitter
    bpy.ops.object.modifier_apply(modifier="Recess")

    # Join into single object
    led_housing.select_set(True)
    led_emitter.select_set(True)
    bpy.ops.object.join()

    led_ring = bpy.context.object
    led_ring.name = "LEDRing"

    # Add subdivision for smooth organic look
    subsurf = led_ring.modifiers.new(name="Smooth", type='SUBSURF')
    subsurf.levels = 2
    subsurf.render_levels = 3

    return led_ring


# Export functions for use in main script
__all__ = [
    'create_professional_base_body',
    'create_professional_indicator',
    'create_professional_detail_ring',
    'create_professional_center_cap',
    'create_professional_led_ring'
]
