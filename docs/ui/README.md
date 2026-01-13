# UI Documentation

User interface design, implementation, and asset generation documentation for Monument Reverb.

## Current Direction (2026-01-07)

**Macro-Only Control Surface:**
- 10 primary macro controls visible by default
- Base parameters hidden (advanced controls accessible)
- Celestial asset layers for visual background
- Reactive macro overlay driven by JSON profiles
- ModMatrix panel for visual modulation editing

**Current Implementation (Debug/Engineering UI):**
- Scrollable control surface exposing all DSP parameters
- Debug toggle reveals advanced sections (memory, physical modeling, timeline, safety)
- Macro Mode selector switches Ancient vs Expressive macro panels
- Output safety clip control and live input/output level readouts

**Asset Sources:**
- `assets/ui/celestial/` - Current UI visual assets
- `assets/ui/macro_hints.json` - Macro visual hints
- `assets/ui/visual_profiles.json` - Visual behavior profiles
- `assets/ui/line6/` - Extracted Line 6 knob plates + knobs (layered UI assets)

### Knob Packs (Layered)

The editor will use layered knobs when assets are available.

Default discovery order:
- `assets/ui/archive` (Archive Instruments AI-generated pack)
- `assets/ui/line6` (Line 6 extraction pack)

Overrides:
- `MONUMENT_KNOB_DIR=<path>` to point at a custom pack directory
- `MONUMENT_KNOB_VARIANT=<variant>` to select a variant
- `MONUMENT_LINE6_DIR` + `MONUMENT_LINE6_KNOB` are still honored for backward compatibility

Layer files: `<variant>_plate.png`, `<variant>_plate_shadow.png` (optional), `<variant>_knob.png`, `<variant>_highlight.png`, `<variant>_shadow.png`, `<variant>_indicator.png`
Rotation override: `MONUMENT_KNOB_ROTATION=indicator|knob`

## Standalone UI Prototype App

The `monument_ui_prototype` target launches the editor without a plugin host for
rapid UI iteration.

```bash
cmake --build build --config Debug --target monument_ui_prototype
```

Run the app at:

```
build/monument_ui_prototype_artefacts/Debug/Monument UI Prototype.app
```

This prototype uses the same editor as the plugin and respects the
`MONUMENT_LEGACY_UI` build option.

## Master Plan

**Primary Document:** [UI_MASTER_PLAN.md](UI_MASTER_PLAN.md)

**Consolidated from:**
- ENHANCED_UI_SUMMARY.md (current state)
- MONUMENT_UI_STRATEGIC_DESIGN_PLAN.md (strategic vision)
- PHOTOREALISTIC_UI_IMPLEMENTATION_PLAN.md (implementation details)
- PHOTOREALISTIC_UI_PROGRESS.md (progress tracking)
- UI_UX_ROADMAP.md (comprehensive roadmap)

**Total:** ~1,100 lines consolidating ~3,800 lines from 5 source documents

## Quick Reference

### Current UI Components

**Implemented:**
- ✅ `EnhancedBackgroundComponent` - Animated stone texture + blue wisps
- ✅ `CollapsiblePanel` - Smooth expand/collapse (300ms animation)
- ✅ `PhotorealisticKnob` - LED glow with breathing pulse
- ✅ `HeaderBar` - Logo, presets, selectors, level meters
- ✅ `ModMatrixPanel` - 4×15 modulation grid editor

**Core Files:**
- `ui/LayeredKnob.h/.cpp` - Base layered knob component
- `ui/PhotorealisticKnob.h` - Enhanced knob with LED animation
- `ui/ModMatrixPanel.h/.cpp` - Visual modulation editor
- `ui/HeaderBar.h/.cpp` - Top toolbar
- `ui/CollapsiblePanel.h/.cpp` - Expandable sections
- `ui/EnhancedBackgroundComponent.h/.cpp` - Animated backgrounds

### Enhanced Photorealistic Knob System (Optional/Future)

**Status:** Complete system available for future use

**Features:**
- Blender-rendered PBR materials
- 5 material variants (granite, marble, basalt, glass, brushed metal, oxidized copper)
- 6 layered architecture (base, ring, indicator, cap, LED ring, shadow)
- Automated generation pipeline

**Quick Start:**
```bash
# Generate granite knob (quick preview)
./scripts/run_blender_enhanced.sh --material granite --quick

# Preview composite
python3 scripts/preview_knob_composite_enhanced.py --material granite

# Generate all materials (production)
./scripts/run_blender_enhanced.sh
```

## Design System

### Ancient Monuments Color Palette

**Backgrounds:**
```
Main BG:    #FDFCFB (warm white, stone base)
Panel BG:   #F9F7F5 (slight grey, recessed)
Header BG:  #2B2621 (dark warm grey, stone texture)
```

**UI Elements:**
```
Button Inactive:  #3A342E (brushed metal)
Button Active:    #8B7355 (warm bronze)
Button Hover:     #B8956A (bronze highlight)
Text Primary:     #2B2621 (dark grey)
Text Secondary:   #8C7B6A (medium grey)
```

**Accents:**
```
Border:      #524A41 (subtle warm grey)
Glow/LED:    #FFA040 (warm amber LED)
Highlight:   #E6D5B8 (cream, raised edges)
Shadow:      #1A1612 (near black, depth)
```

**Modulation Colors:**
```
Chaos:       #FF8C3C (warm orange)
Audio:       #50C878 (green)
Brownian:    #B464DC (purple)
Envelope:    #6496FF (blue)
```

### Design Principles

**Monument = STONE + TIME**

The UI should feel like:
- **Exploring a monument** - Discovering chambers and spaces
- **Material tactility** - Stone, metal, glass feel real
- **Time and decay** - Visual feedback for evolution
- **No standard knobs** - Every control is a portal into the space

## Asset Generation

### Archive Instruments AI Knob Generation

```bash
python tools/generate_archive_knob_ai.py \
  --output-dir assets/ui/archive/raw \
  --name archive_brass_precision \
  --size 1024

python tools/extract_line6_knob_layers.py \
  --input-dir assets/ui/archive/raw \
  --output-dir assets/ui/archive \
  --size 1024 \
  --prefix archive_ \
  --single-name archive_brass_precision \
  --plate-alpha-cut 0.25 \
  --plate-erode 2 \
  --plate-shadow-strength 0.35 \
  --plate-shadow-blur 12 \
  --plate-shadow-offset 6 \
  --alpha-clip 0.12 \
  --plate-edge-width 18 \
  --plate-edge-darken 0.4
```

### Blender Knob Pipeline

**Script:** `scripts/generate_knob_blender_enhanced.py` (615 lines)

**Workflow:**
1. Define material properties (color, roughness, metallic)
2. Generate knob geometry (base, ring, indicator, cap, LED)
3. Set up 4-point studio lighting
4. Render 6 layers per material
5. Export to `assets/ui/knobs_enhanced/`

**Render Quality:**
- Quick: 64 samples (~30sec/layer)
- Standard: 256 samples (~2-3min/layer)
- High: 512 samples (~5-7min/layer)

**Output:**
- `layer_0_base_<material>.png` (rotates)
- `layer_1_ring_<material>.png` (static)
- `layer_2_indicator_brushed_aluminum.png` (rotates)
- `layer_3_cap_brushed_aluminum.png` (static)
- `layer_4_led_ring.png` (static, emission glow)
- `fx_shadow_<material>.png` (multiply blend)

### Material Variants

| Material | Base Color | Roughness | Metallic | Style |
|----------|-----------|-----------|----------|-------|
| granite | Gray (0.35) | 0.85 (matte) | 0.0 | Industrial, brutalist |
| marble | White (0.85) | 0.15 (polished) | 0.0 | Elegant, classical |
| basalt | Black (0.15) | 0.75 (matte) | 0.0 | Dark, modern |
| brushed_metal | Aluminum (0.6) | 0.3 (brushed) | 1.0 | High-tech |
| oxidized_copper | Verdigris | 0.6 (weathered) | 0.4 | Aged, architectural |

## Implementation Patterns

### LayeredKnob Integration

```cpp
// Create custom knob with 5 layers
class MonumentTimeKnobGranite : public LayeredKnob {
public:
    MonumentTimeKnobGranite(juce::AudioProcessorValueTreeState& state)
        : LayeredKnob(state, "time", "TIME")
    {
        // Add layers (base, ring, indicator, cap, LED)
        addLayer(BinaryData::layer_0_base_granite_png,
                 BinaryData::layer_0_base_granite_pngSize, true);
        addLayer(BinaryData::layer_1_ring_granite_png,
                 BinaryData::layer_1_ring_granite_pngSize, false);
        // ... remaining layers

        setRotationRange(-135.0f, +135.0f);
    }
};
```

### ModMatrixPanel Usage

```cpp
// 4×15 modulation grid
// 4 sources: Chaos, Audio, Brownian, Envelope
// 15 destinations: Time, Mass, Density, ...

// Click inactive button → Create connection (depth: 0.5)
// Click active button → Select for editing
// Click selected button → Remove connection
// Adjust sliders → Modify depth (-1 to +1) and smoothing (20-1000ms)
```

## Future Roadmap

**See:** [UI_MASTER_PLAN.md - Section V](UI_MASTER_PLAN.md#v-future-roadmap)

**6-Phase Enhancement Plan:** (4-6 weeks post-DSP)
1. **Foundation** (Week 1) - Color palette, design system, toolbar redesign
2. **Preset Browser** (Week 2) - Hierarchical organization, categories, badges
3. **Base Params** (Week 2) - Functional grouping (Chamber/Geometry/Atmosphere)
4. **ModMatrix** (Week 3) - Destination grouping, template presets
5. **Advanced Browser** (Week 4-5) - Optional visual preset browser
6. **Polish & Testing** (Week 6) - Accessibility, performance, DAW testing

## Additional Documentation

### Design References
- [design-references/VINTAGE_CONTROL_PANEL_REFERENCES.md](design-references/VINTAGE_CONTROL_PANEL_REFERENCES.md) - Vintage industrial inspiration

### Technical Guides
- [LAYERED_KNOB_DESIGN.md](LAYERED_KNOB_DESIGN.md) - Architecture details
- [LAYERED_KNOB_WORKFLOW.md](LAYERED_KNOB_WORKFLOW.md) - Original workflow
- [JUCE_BLEND_MODES_RESEARCH.md](JUCE_BLEND_MODES_RESEARCH.md) - Rendering techniques
- [LAYERCOMPOSITOR_INTEGRATION_SUMMARY.md](LAYERCOMPOSITOR_INTEGRATION_SUMMARY.md) - Layer compositing

## Performance Targets

| Metric | Target | Critical |
|--------|--------|----------|
| Frame rate | 60 FPS | 30 FPS minimum |
| CPU (idle) | <2% | <5% |
| CPU (playing) | <8% | <15% |
| Memory | <50 MB | <100 MB |
| Load time | <200ms | <500ms |

## Accessibility

- Keyboard navigation for all controls
- ARIA labels for screen readers
- WCAG 2.1 AA contrast compliance (minimum)
- Focus indicators (2px solid, 8:1 contrast)
- Color-independent interaction (icons + text + shape)

## Related Documentation

- [../README.md](../../README.md) - Project overview
- [../architecture/README.md](../architecture/README.md) - Architecture documentation
- [../development/README.md](../development/README.md) - Development guides
- [../testing/README.md](../testing/README.md) - Testing documentation
