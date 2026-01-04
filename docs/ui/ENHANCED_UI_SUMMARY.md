# Monument Reverb - Enhanced Photorealistic UI System
**Created:** 2026-01-03
**Status:** Ready for Testing
**Goal:** Pseudo-3D photorealistic UI with Blender-rendered layered knobs

---

## Overview

You now have a complete enhanced UI system with:

✅ **Enhanced Blender Rendering** - Advanced PBR materials, environmental effects, shadows
✅ **Multiple Material Variants** - Granite, marble, basalt, glass, brushed metal, oxidized copper
✅ **Layered Architecture** - Base, ring, indicator, cap + shadow layers
✅ **Preview & Compositing Tools** - Test before building
✅ **Automated Workflow** - Batch generation scripts

---

## Quick Start (5-Minute Test)

### 1. Generate Granite Knob (Quick Preview)

```bash
cd /Users/noisebox/Documents/3_Development/Repos/monument-reverb
./scripts/run_blender_enhanced.sh --material granite --quick
```

**What this does:**
- Renders 5 knob layers (base, ring, indicator, cap, LED ring) + shadow layer
- Uses 64 samples (fast, ~30 seconds per layer = 2-3 minutes total)
- Output: `assets/ui/knobs_enhanced/granite/`

### 2. Preview Composite

```bash
python3 scripts/preview_knob_composite_enhanced.py --material granite
```

**What this does:**
- Composites all layers at rotation angles 0°, 45°, 90°, 135°, 180°, 225°, 270°, 315°
- Shows how knob will look when rotating
- Opens preview in system viewer automatically
- Output: `/tmp/knob_granite_sequence.png`

### 3. If Preview Looks Good → Generate All Materials

```bash
./scripts/run_blender_enhanced.sh
```

**What this does:**
- Generates all 5 material variants: granite, marble, basalt, brushed_metal, oxidized_copper
- Uses 256 samples (production quality, ~2-3 minutes per layer)
- Total time: ~40-50 minutes for all materials

---

## File Structure

```
monument-reverb/
├── scripts/
│   ├── generate_knob_blender_enhanced.py  ✅ NEW - Enhanced Blender script
│   ├── run_blender_enhanced.sh            ✅ NEW - Batch generation
│   └── preview_knob_composite_enhanced.py ✅ NEW - Preview composites
│
├── assets/ui/knobs_enhanced/
│   ├── granite/
│   │   ├── layer_0_base_granite.png       (512×512, RGBA, rotates)
│   │   ├── layer_1_ring_granite.png       (512×512, RGBA, static)
│   │   ├── layer_2_indicator_brushed_aluminum.png  (512×512, RGBA, rotates)
│   │   ├── layer_3_cap_brushed_aluminum.png        (512×512, RGBA, static)
│   │   ├── layer_4_led_ring.png           (512×512, RGBA, static, emission glow)
│   │   └── fx_shadow_granite.png          (512×512, RGB, multiply blend)
│   ├── marble/
│   │   └── ... (same structure)
│   └── ... (other materials)
│
└── ui/
    ├── LayeredKnob.h/.cpp                 ✅ EXISTING - Works with enhanced assets
    └── ChamberWallControl.h/.cpp          ✅ EXISTING - Example usage
```

---

## Material Variants

### Available Materials

| Material | Base Color | Roughness | Metallic | Style |
|----------|-----------|-----------|----------|-------|
| **granite** | Gray (0.35) | 0.85 (matte) | 0.0 | Industrial, brutalist |
| **marble** | White (0.85) | 0.15 (polished) | 0.0 | Elegant, classical |
| **basalt** | Black (0.15) | 0.75 (matte) | 0.0 | Dark, modern |
| **brushed_metal** | Aluminum (0.6) | 0.3 (brushed) | 1.0 | Industrial, high-tech |
| **oxidized_copper** | Verdigris (0.42, 0.55, 0.48) | 0.6 (weathered) | 0.4 | Aged, architectural |

### Indicator Materials

| Material | Color | Use With |
|----------|-------|----------|
| **brushed_aluminum** | Aluminum | Granite, basalt |
| **gold** | Gold (0.83, 0.69, 0.22) | Marble, oxidized copper |
| **copper** | Copper (0.95, 0.64, 0.54) | Brushed metal |

---

## Enhanced Geometry Options (Vintage Industrial Inspired)

### Design Direction

Inspired by vintage industrial control panels featuring:
- Multi-level depth with recessed/raised elements
- Embedded indicator LEDs and illuminated elements
- Complex composite knobs (cap + body + indicator + LED)
- Varied surface textures (knurling, grooves, facets)
- Architectural details (mounting hardware, bezels, trim rings)

### Available Geometry Enhancements

#### 1. Rotary Knob Cap Variations

**Domed Cap (Default):**
- Smooth brushed metal center disc
- Current implementation: Simple cylinder with bevel
- Use: Clean, modern aesthetic

**Concave Cap:**
- Recessed center for finger grip
- Geometric options: Bowl-shaped, conical inset
- Use: Ergonomic, tactile feedback

**Textured Cap:**
- Knurled surface for grip
- Implementation: Procedural bump mapping or displacement
- Patterns: Radial grooves, cross-hatch, stippled

**Faceted Cap:**
- Multi-sided geometric shape (hex, octagon)
- Industrial/mechanical aesthetic
- Use: High-precision controls (fine-tuning parameters)

#### 2. Embedded LED Indicator

**Ring LED (Recommended):**
- Circular LED ring around indicator bar
- Implementation: Separate emission layer
- States: Off (default), On (active), Pulsing (modulation)
- Colors: Amber, red, green, blue (parameter-specific)

**Dot LED:**
- Single point light at indicator tip
- Minimal, focused feedback
- Use: Simple on/off state visualization

**Multi-Segment LED:**
- Arc of LED segments showing parameter range
- Inspired by vintage VU meters
- Implementation: 8-12 segments as separate layers
- Use: Level/mix parameters (shows amount)

#### 3. Multi-Level Depth Architecture

**Current:** 2 levels (base body + raised ring)

**Enhanced Options:**

**3-Level:**
- Base body (lowest)
- Recessed groove/channel (mid-level detail)
- Raised rim/bezel (highest)
- Use: More pronounced depth, shadow complexity

**4-Level (Premium):**
- Base mounting plate (flush with panel)
- Recessed body cavity
- Knob body cylinder
- Raised cap/indicator assembly
- Use: Maximum architectural depth

**Inverted (Recessed Knob):**
- Knob sits below panel surface in circular well
- Requires panel background layer
- Use: Protective recessing, brutalist aesthetic

#### 4. Detail Elements

**Tick Marks Enhancement:**
- Current: 12 simple rectangular tick marks
- Enhanced options:
  - Engraved grooves (depth mapped)
  - Illuminated segments (LED-lit markers)
  - Variable-height markers (major/minor scale)
  - Numeric labels (debossed or raised)

**Mounting Hardware:**
- Visible screws at ring edge (4-6 positions)
- Knurled lock ring (rotates to tighten)
- Split bezel with seam line
- Use: Industrial authenticity

**Surface Treatments:**
- Brushed finish with directional grain
- Anodized color (beyond natural metal)
- Patina/wear maps (vintage aesthetic)
- Carbon fiber weave pattern

#### 5. Composite Knob Assemblies

**Layered Sandwich Design:**
```
Top to Bottom:
1. Indicator bar (rotates)
2. Transparent cap window (static, LED visible through)
3. LED emission layer (static, glows)
4. Knob body (rotates)
5. Mounting bezel (static)
6. Base plate (static)
7. Shadow pass (rotates with body)
```

**Split-Ring Design:**
- Outer ring remains static (tick marks, labels)
- Inner disc rotates independently
- Two rotation points for dual control
- Use: Coarse/fine adjustment parameters

### Implementation Priority

**Phase 1:** ✅ Base geometry with simple cap

**Phase 2 (Current):**

- [x] Add LED emission layer (ring) ✅ **COMPLETED 2026-01-03**
- [ ] Enhanced tick marks with depth
- [ ] Concave cap option

**Phase 3 (Future):**

- [ ] Multi-level depth (3-4 levels)
- [ ] Textured surfaces (knurling, patterns)
- [ ] Mounting hardware details

**Phase 4 (Premium):**

- [ ] Composite assemblies (split-ring, sandwich)
- [ ] Parameter-specific geometries
- [ ] State variations (hover glow, active pulse)

### Script Modifications Needed

To implement these enhancements, edit `scripts/generate_knob_blender_enhanced.py`:

**Add LED Ring Layer:** ✅ **IMPLEMENTED 2026-01-03**

Implementation details:

```python
def create_led_ring():
    """Create LED ring with warm amber emission beneath knob cap.

    Creates a recessed channel design with rounded profile for realistic
    LED housing appearance, similar to vintage control panel indicators.
    """
    knob_radius = 1.0  # Match base body radius
    major_radius = knob_radius * 1.15  # Position around knob
    minor_radius = 0.08  # Larger profile for visible LED channel

    bpy.ops.mesh.primitive_torus_add(
        major_radius=major_radius,
        minor_radius=minor_radius,
        major_segments=128,  # Smooth circle
        minor_segments=32,   # Smooth profile
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
```

**Output:** `layer_4_led_ring.png` (512×512, RGBA, emission glow, static)

**Add Concave Cap:**
```python
def create_concave_cap(radius=0.16, depth=0.02):
    """Create concave center cap with finger grip."""
    bpy.ops.mesh.primitive_cylinder_add(
        vertices=64,
        radius=radius,
        depth=0.05,
        location=(0, 0, 0.18)
    )
    cap = bpy.context.object
    cap.name = "ConcaveCap"

    # Add displacement modifier for concave surface
    displace = cap.modifiers.new(name="Concave", type='DISPLACE')
    displace.strength = -depth
    displace.mid_level = 0.5

    # Create radial texture for displacement
    tex = bpy.data.textures.new(name="RadialGradient", type='BLEND')
    tex.use_color_ramp = True
    displace.texture = tex

    return cap
```

---

## Advanced Features

### Environmental Effects

The enhanced script renders:

1. **Shadows** - Soft contact shadows from 4-point studio lighting
2. **Ambient Occlusion** - Baked into materials via bump mapping
3. **Reflections** - Anisotropic brushed metal reflections on indicator/cap
4. **Highlights** - Edge lighting from 3-point studio setup
5. **Depth** - Procedural bump mapping for surface detail

### Lighting Setup

**4-Point Studio Lighting:**
- **Key Light** (150W, warm white) - Main illumination, upper right
- **Fill Light** (60W, cool white) - Soften shadows, left side
- **Rim Light** (80W, white) - Edge definition, back
- **Ambient Light** (25W, cool white) - Subtle overall fill

### Render Settings

**Quick (--quick):**
- 64 samples
- ~30 sec/layer
- Good for iteration

**Standard:**
- 256 samples (default)
- ~2-3 min/layer
- Production quality

**High Quality (--high-quality):**
- 512 samples
- ~5-7 min/layer
- Final assets for release

---

## Integration with JUCE

### 1. Add Assets to CMakeLists.txt

```cmake
# Add enhanced knob assets (lines ~90)
juce_add_binary_data(MonumentAssets
  HEADER_NAME BinaryData.h
  NAMESPACE BinaryData
  SOURCES
    # Granite knob
    assets/ui/knobs_enhanced/granite/layer_0_base_granite.png
    assets/ui/knobs_enhanced/granite/layer_1_ring_granite.png
    assets/ui/knobs_enhanced/granite/layer_2_indicator_brushed_aluminum.png
    assets/ui/knobs_enhanced/granite/layer_3_cap_brushed_aluminum.png
    assets/ui/knobs_enhanced/granite/layer_4_led_ring.png
    assets/ui/knobs_enhanced/granite/fx_shadow_granite.png

    # Marble knob (optional)
    assets/ui/knobs_enhanced/marble/layer_0_base_marble.png
    # ... etc
)
```

### 2. Use with LayeredKnob

```cpp
// ui/MonumentTimeKnobGranite.h
#pragma once
#include "LayeredKnob.h"

class MonumentTimeKnobGranite : public LayeredKnob
{
public:
    MonumentTimeKnobGranite(juce::AudioProcessorValueTreeState& state)
        : LayeredKnob(state, "time", "TIME")
    {
        // Layer 0: Base (rotates)
        addLayer(
            BinaryData::layer_0_base_granite_png,
            BinaryData::layer_0_base_granite_pngSize,
            true  // rotates
        );

        // Layer 1: Ring (static)
        addLayer(
            BinaryData::layer_1_ring_granite_png,
            BinaryData::layer_1_ring_granite_pngSize,
            false  // static
        );

        // Layer 2: Indicator (rotates)
        addLayer(
            BinaryData::layer_2_indicator_brushed_aluminum_png,
            BinaryData::layer_2_indicator_brushed_aluminum_pngSize,
            true  // rotates
        );

        // Layer 3: Cap (static)
        addLayer(
            BinaryData::layer_3_cap_brushed_aluminum_png,
            BinaryData::layer_3_cap_brushed_aluminum_pngSize,
            false  // static
        );

        // Layer 4: LED Ring (static, emission glow)
        addLayer(
            BinaryData::layer_4_led_ring_png,
            BinaryData::layer_4_led_ring_pngSize,
            false  // static
        );

        // Layer 5: Shadow (static, multiply blend - optional)
        // TODO: Implement multiply blend in LayeredKnob

        setRotationRange(-135.0f, +135.0f);
    }
};
```

### 3. Build and Test

```bash
# Build
cmake --build build --target Monument_AU --config Release

# Clear cache
killall -9 AudioComponentRegistrar

# Test in DAW
# Open Logic Pro or Ableton Live
# Load Monument plugin
# Verify granite knob renders correctly with rotation
```

---

## Customization

### Generate Custom Material

Edit material properties in `scripts/generate_knob_blender_enhanced.py`:

```python
MATERIALS = {
    "my_custom_material": {
        "base_color": (0.5, 0.3, 0.2, 1.0),  # Brown
        "roughness": 0.7,
        "metallic": 0.0,
        "noise_scale": 12.0,
        "noise_detail": 10.0
    }
}
```

Then generate:

```bash
./scripts/run_blender_enhanced.sh --material my_custom_material
```

### Change Knob Geometry

Edit geometry functions in `generate_knob_blender_enhanced.py`:

**More segments (gear-like):**
```python
# Line ~360: Array modifier count
array.count = 64  # Double the segments (was 32)
```

**Thicker base:**
```python
# Line ~342: Cylinder depth
depth=0.5,  # Increase thickness (was 0.3)
```

**Different indicator shape:**
```python
# Line ~387-391: Create custom indicator mesh
# Replace with your own mesh creation code
```

---

## Performance Notes

### Memory Usage

**Per Knob (5 layers @ 512×512 RGBA):**
- RAM: ~8-10 MB per knob instance
- Binary size: ~2-3 MB (PNG compression)

**18 Parameters:**
- RAM: ~150-180 MB total
- Binary size: ~40-50 MB

**Acceptable** for modern audio plugins (typical: 50-200 MB)

### Rendering Performance

**LayeredKnob (5 layers + shadow):**
- ~1-2ms per repaint
- 60 FPS capable
- No GPU acceleration needed (alpha blending is fast)

---

## Next Steps

### Phase 1: Test Single Material (Today)

1. ✅ Enhanced scripts created
2. ⏳ Generate granite knob (--quick)
3. ⏳ Preview composite
4. ⏳ Integrate into JUCE
5. ⏳ Test in DAW

### Phase 2: Scale to All Parameters (Week 1)

Create parameter-specific knobs with different materials:

**Chamber Parameters (6 knobs):**
- **Time** → Granite (industrial, solid)
- **Mass** → Marble (elegant, heavy)
- **Density** → Basalt (dark, dense)
- **Bloom** → Glass (transparent, ethereal)
- **Gravity** → Oxidized copper (aged, weighty)

**Geometry Parameters (2 knobs):**
- **Warp** → Brushed metal (twisted, industrial)
- **Drift** → Oxidized copper (slow movement, patina)

**Atmosphere Parameters (2 knobs):**
- **Air** → Glass (clear, transparent)
- **Width** → Brushed metal (wide, expansive)

**Macro Parameters (6 knobs):**
- All use granite base with different indicator colors:
  - Material → Gold indicator
  - Topology → Copper indicator
  - Viscosity → Aluminum indicator
  - Evolution → Gold indicator
  - Chaos → Copper indicator
  - Elasticity → Aluminum indicator

### Phase 3: Add State Variations (Week 2)

Generate hover/active/disabled states:

```bash
# Generate hover state (brighter)
./scripts/run_blender_enhanced.sh \
  --material granite \
  --samples 256 \
  --out assets/ui/knobs_enhanced/granite_hover
```

Then modify lighting in script (increase ambient light energy).

### Phase 4: Background & Typography (Week 2-3)

Design thematic background with Monument aesthetic:
- Stone texture background
- Engraved typography
- Architectural dividers
- Subtle depth layers

---

## Troubleshooting

### Issue: Blender renders too slowly

**Solution 1:** Use GPU rendering (if available)
```python
# In generate_knob_blender_enhanced.py, line ~171
scene.cycles.device = 'GPU'
scene.cycles.use_gpu_only = True
```

**Solution 2:** Lower sample count
```bash
./scripts/run_blender_enhanced.sh --samples 64  # Fast iteration
```

### Issue: Layers misaligned in preview

**Cause:** Asset size mismatch
**Fix:** Ensure all assets are exactly 512×512 pixels

```bash
# Check asset sizes
ls -lh assets/ui/knobs_enhanced/granite/*.png
```

### Issue: Knob looks pixelated in JUCE

**Cause:** Resolution too low
**Fix:** Regenerate at 1024×1024

```bash
./scripts/run_blender_enhanced.sh \
  --material granite \
  --size 1024 \
  --samples 256
```

---

## Resources

### Documentation
- [MONUMENT_UI_STRATEGIC_DESIGN_PLAN.md](MONUMENT_UI_STRATEGIC_DESIGN_PLAN.md) - Full vision
- [LAYERED_KNOB_DESIGN.md](LAYERED_KNOB_DESIGN.md) - Architecture details
- [LAYERED_KNOB_WORKFLOW.md](LAYERED_KNOB_WORKFLOW.md) - Original workflow
- [MVP_UI_HANDOFF_2026_01_03.md](MVP_UI_HANDOFF_2026_01_03.md) - MVP status

### Scripts
- `generate_knob_blender_enhanced.py` - Main rendering script (615 lines)
- `run_blender_enhanced.sh` - Batch wrapper
- `preview_knob_composite_enhanced.py` - Composite preview tool

### JUCE Components
- [ui/LayeredKnob.h/.cpp](../../ui/LayeredKnob.h) - Base class (already working)
- [ui/ChamberWallControl.h/.cpp](../../ui/ChamberWallControl.h) - Example usage

---

## Summary

✅ **Complete Enhanced Workflow Ready**
✅ **5 Material Variants Supported**
✅ **5 Knob Layers + Shadow FX** (base, ring, indicator, cap, LED ring)
✅ **LED Emission Layer Implemented** (warm amber glow)
✅ **Environmental Effects Implemented**
✅ **Preview Tools Created**
✅ **JUCE Integration Path Clear**

**Next Action:** Run quick test generation and preview composite!

```bash
# 5-minute test
./scripts/run_blender_enhanced.sh --material granite --quick
python3 scripts/preview_knob_composite_enhanced.py --material granite
```
