# Monument UI Master Plan

**Created:** 2026-01-09 (Consolidated from 5 source documents)
**Status:** **Current Direction: Macro-Only UI** (2026-01-07 update)
**Purpose:** Unified UI documentation and strategic roadmap

**Important Note:** As of 2026-01-07, the plugin UI has shifted to a macro-only control surface using celestial asset layers and reactive macro overlay. Base parameters are hidden in the UI, and reactive visuals are driven by `assets/ui/macro_hints.json` + `assets/ui/visual_profiles.json`. The enhanced Blender pipeline remains optional for future asset packs.

---

## Table of Contents

1. [Current State](#i-current-state)
2. [Strategic Vision](#ii-strategic-vision)
3. [Implementation Plan](#iii-implementation-plan)
4. [Progress Tracking](#iv-progress-tracking)
5. [Future Roadmap](#v-future-roadmap)

---

## I. Current State

### Quick Overview

**Implemented System (2026-01-07):**
- ✅ **Macro-Only Control Surface** - 10 primary controls visible
- ✅ **Celestial Asset Layers** - Background visual system
- ✅ **Reactive Macro Overlay** - JSON-driven visual profiles
- ✅ **ModMatrix Panel** - Visual modulation editor (4×15 grid)
- ✅ **Base Parameters Hidden** - Advanced controls accessible but not default UI

### Enhanced Photorealistic Knob System (Optional/Future)

**Complete enhanced UI system available:**
- ✅ **Enhanced Blender Rendering** - Advanced PBR materials, environmental effects, shadows
- ✅ **Multiple Material Variants** - Granite, marble, basalt, glass, brushed metal, oxidized copper
- ✅ **Layered Architecture** - Base, ring, indicator, cap + LED ring + shadow layers
- ✅ **Preview & Compositing Tools** - Test before building
- ✅ **Automated Workflow** - Batch generation scripts

### Quick Start: Enhanced Knobs (5-Minute Test)

```bash
cd /Users/noisebox/Documents/3_Development/Repos/monument-reverb

# Generate granite knob (quick preview)
./scripts/run_blender_enhanced.sh --material granite --quick

# Preview composite
python3 scripts/preview_knob_composite_enhanced.py --material granite

# If preview looks good → generate all materials
./scripts/run_blender_enhanced.sh
```

### File Structure

```
monument-reverb/
├── scripts/
│   ├── generate_knob_blender_enhanced.py  # Enhanced Blender script
│   ├── run_blender_enhanced.sh            # Batch generation
│   └── preview_knob_composite_enhanced.py # Preview composites
│
├── assets/ui/
│   ├── celestial/                         # Current macro UI assets
│   ├── knobs_enhanced/                    # Optional photorealistic knobs
│   │   ├── granite/
│   │   │   ├── layer_0_base_granite.png
│   │   │   ├── layer_1_ring_granite.png
│   │   │   ├── layer_2_indicator_brushed_aluminum.png
│   │   │   ├── layer_3_cap_brushed_aluminum.png
│   │   │   ├── layer_4_led_ring.png
│   │   │   └── fx_shadow_granite.png
│   │   └── ... (other materials)
│   ├── macro_hints.json                   # Macro visual hints
│   └── visual_profiles.json               # Visual behavior profiles
│
└── ui/
    ├── LayeredKnob.h/.cpp                 # Works with enhanced assets
    ├── PhotorealisticKnob.h              # LED animation, filmstrip rendering
    ├── ModMatrixPanel.h/.cpp              # Visual modulation editor
    ├── HeaderBar.h/.cpp                   # Top toolbar
    ├── CollapsiblePanel.h/.cpp            # Expandable sections
    └── EnhancedBackgroundComponent.h/.cpp # Animated backgrounds
```

### Material Variants (Enhanced Knobs)

| Material | Base Color | Roughness | Metallic | Style |
|----------|-----------|-----------|----------|-------|
| **granite** | Gray (0.35) | 0.85 (matte) | 0.0 | Industrial, brutalist |
| **marble** | White (0.85) | 0.15 (polished) | 0.0 | Elegant, classical |
| **basalt** | Black (0.15) | 0.75 (matte) | 0.0 | Dark, modern |
| **brushed_metal** | Aluminum (0.6) | 0.3 (brushed) | 1.0 | Industrial, high-tech |
| **oxidized_copper** | Verdigris (0.42, 0.55, 0.48) | 0.6 (weathered) | 0.4 | Aged, architectural |

### Component Classes

#### 1. PhotorealisticKnob - Full PBR Implementation

```cpp
/**
 * Knob with 128-frame filmstrip + real-time lighting
 *
 * Assets per knob:
 * - knob_albedo_128frames.png (diffuse color)
 * - knob_normal_128frames.png (surface bumps)
 * - knob_roughness_128frames.png (matte/glossy)
 * - knob_ao_128frames.png (ambient occlusion)
 */
class PhotorealisticKnob : public juce::Slider
{
    // Rendering layers:
    // - Layer 1: Ambient Occlusion (shadow base)
    // - Layer 2: Albedo (diffuse color)
    // - Layer 3: Normal map (lighting simulation)
    // - Layer 4: Roughness/Specular highlights
    // - Layer 5: Indicator line (glowing when active)
    // - Layer 6: Glow if modulated
};
```

#### 2. ModMatrixPanel - Visual Modulation Editor

**Status:** ✅ Fully Implemented & Integrated (2026-01-03)

**Features:**
- 4×15 grid (60 connection points)
- 4 modulation sources: Chaos, Audio, Brownian, Envelope
- 15 parameter destinations
- Color-coded sources for quick identification
- Real-time thread-safe updates via SpinLock

**Interaction Model:**
1. **Click inactive button** → Create connection (depth: 0.5, smoothing: 200ms)
2. **Click active button** → Select for editing
3. **Click selected button** → Remove connection
4. **Adjust sliders** → Modify depth (-1 to +1) and smoothing (20-1000ms)

### UI Components Implemented

**Core Components:**
- ✅ `EnhancedBackgroundComponent` - Dark stone texture with animated blue wisps
- ✅ `CollapsiblePanel` - Smooth expand/collapse animation (300ms)
- ✅ `PhotorealisticKnob` - LED center glow with breathing pulse
- ✅ `HeaderBar` - Logo, presets, selectors, level meters
- ✅ `ModMatrixPanel` - 4×15 modulation grid

### Current UI Analysis (Pre-Macro Surface)

**Strengths:**
- Clean white background with high contrast readability
- Unified hero knob aesthetic (codex brushed aluminum)
- Macro-first interface design (10 primary controls visible by default)
- ModMatrix panel provides excellent visual feedback
- Base parameters hidden by default reduces initial complexity

**Issues Identified (Historical):**
1. **Top Toolbar Overlap** - Mode selector competed for space with Architecture selector
2. **Grey/Black Dropdowns** - Poor contrast on white background
3. **Flat Preset List** - No categorization or sonic guidance
4. **Base Parameters Spacing** - Cluttered when expanded, no functional grouping

---

## II. Strategic Vision

### Core Principles

**Monument is not a utility reverb—it's an architectural experience.**

The UI should feel like:
- **Exploring a monument** - Discovering chambers, spaces, and hidden resonances
- **Material tactility** - Stone, metal, glass, air—each element feels real and responds to touch
- **Time and decay** - Visual feedback for evolution, bloom, weathering, and memory
- **No standard knobs** - Every control is a portal into the space, not a parameter dial

### Elemental Theme System

Each plugin embodies an element. Monument = **STONE + TIME**

**Visual Motifs:**
- **Stone textures**: Granite, marble, limestone, basalt—each preset has material identity
- **Architectural geometry**: Pillars, chambers, vaults, alcoves—3D depth illusion
- **Light and shadow**: Baked ambient occlusion, soft shadows, edge lighting
- **Temporal decay**: Weathering effects, patina, moss growth for Memory Echoes
- **Spatial indicators**: Depth cues via parallax, scale, and atmospheric perspective

### Design Goals

1. **Establish Clear Visual Hierarchy**
   - Primary controls (10 macros) immediately recognizable
   - Secondary controls (mode/architecture/presets) visually subordinate but accessible
   - Tertiary controls (base parameters) hidden but discoverable

2. **Integrate Ancient Monuments Aesthetic**
   - Stone, earth, architectural materials
   - Warm, aged color palette (granite, bronze, weathered metal)
   - Subtle texture and depth (not flat UI)
   - Cohesive theme from top to bottom

3. **Improve Preset Discovery**
   - Categorized, browsable preset system
   - Visual + textual information about each space
   - Quick preview or sonic indicators
   - Encourage exploration and experimentation

4. **Optimize Spacing and Layout**
   - Generous whitespace around all interactive elements
   - Clear visual grouping (chamber/geometry/atmosphere sections)
   - Consistent sizing and alignment
   - Responsive design for different plugin window sizes

5. **Maintain Professional Polish**
   - No visual glitches or overlaps
   - Smooth animations and transitions
   - Consistent interaction patterns
   - Accessible (keyboard navigation, screen reader support)

### Ancient Monuments Color Palette

**Primary Background:**
```
Main BG:           #FDFCFB (warm white, stone base)
Panel BG:          #F9F7F5 (slight grey, recessed panels)
Header BG:         #2B2621 (dark warm grey, stone texture)
```

**UI Elements:**
```
Button Inactive:   #3A342E (medium warm grey, brushed metal)
Button Active:     #8B7355 (warm bronze, 60% opacity)
Button Hover:      #B8956A (brushed bronze, highlight)
Text Primary:      #2B2621 (dark warm grey, high contrast)
Text Secondary:    #8C7B6A (medium warm grey, labels)
Text Active:       #D4AF86 (golden bronze, emphasis)
```

**Accents:**
```
Border:            #524A41 (subtle warm grey, dividers)
Glow/LED:          #FFA040 (warm amber, 2500K LED)
Highlight:         #E6D5B8 (cream, raised edges)
Shadow:            #1A1612 (near black, depth)
```

**Modulation Source Colors:**
```
Chaos Attractor:   #FF8C3C (warm orange)
Audio Follower:    #50C878 (green)
Brownian Motion:   #B464DC (purple)
Envelope Tracker:  #6496FF (blue)
```

---

## III. Implementation Plan

### Layer Stack Architecture (Photorealistic Knobs)

**Rendering Pipeline (Back to Front):**
```
Layer 0: Stone Texture Base (PBR albedo)
    ↓
Layer 1: Normal Map (surface detail, grooves, weathering)
    ↓
Layer 2: Roughness Map (matte stone vs polished metal)
    ↓
Layer 3: Ambient Occlusion (shadows in crevices)
    ↓
Layer 4: Edge/Rim Lighting (highlight raised surfaces)
    ↓
Layer 5: Blueprint Grid Overlay (architectural guides)
    ↓
Layer 6: UI Controls (photorealistic knob sprites)
    ↓
Layer 7: Glow/Emission (active modulation, playhead)
    ↓
Layer 8: Particle Effects (dust motes, echo visualization)
    ↓
Layer 9: Audio Reactivity (level meters, waveform)
```

### LED Emission Layer Implementation

**Created:** 2026-01-03
**Status:** ✅ Complete

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

    # Apply modifiers for clean geometry
    bpy.context.view_layer.objects.active = led_ring
    bpy.ops.object.modifier_apply(modifier="Subdivision")

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

### Integration with JUCE

#### 1. Add Assets to CMakeLists.txt

```cmake
# Add enhanced knob assets
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
    # ... other materials
)
```

#### 2. Use with LayeredKnob

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

        setRotationRange(-135.0f, +135.0f);
    }
};
```

### Performance Targets

| Metric | Target | Critical |
|--------|--------|----------|
| Frame rate | 60 FPS | 30 FPS minimum |
| CPU usage (idle) | <2% | <5% |
| CPU usage (playing) | <8% | <15% |
| Memory footprint | <50 MB | <100 MB |
| Asset load time | <200ms | <500ms |

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

## IV. Progress Tracking

### Completed Components (2026-01-05)

#### 1. EnhancedBackgroundComponent
**Status:** ✅ Complete and building

**Features:**
- Dark weathered stone texture (#0d0d0d to #1a1a1a gradient)
- Animated blue ethereal wisps (8-12 floating fog effects)
- Breathing animation with 30 FPS smooth updates
- Panel dividers with embossed shadow effect

#### 2. CollapsiblePanel
**Status:** ✅ Complete and building

**Features:**
- Smooth expand/collapse animation (300ms with ease-out cubic)
- Expandable header with arrow indicator (▶/▼)
- Dark theme styling (#1a1a1a background)
- Content component management
- Callback support for expanded state changes

#### 3. Enhanced PhotorealisticKnob
**Status:** ✅ Complete and building

**New Features:**
- **Animated blue LED center glow** (matches mockup design)
- Breathing pulse effect (slow sine wave, 0.6-1.0 intensity)
- Hover glow intensity increase
- Multi-layer rendering:
  - Outer glow (soft, large radius)
  - Middle glow (bright, medium radius)
  - Inner core (cyan #88ccff)
  - Bright center dot (white LED point)

#### 4. HeaderBar
**Status:** ✅ Complete and building

**Features:**
- MONUMENT logo/title (left aligned)
- Preset selector dropdown (center-left)
- Hall/Wall selector (center)
- Architecture dropdown (center-right)
- Input/output level meters with gradient (right)

### Build Status

**CMakeLists.txt:** ✅ Updated with all new components
**Compilation:** ✅ 100% successful (all warnings resolved)
**Targets Built:**
- Monument (Shared Code)
- Monument_Standalone
- Monument_AU
- Monument_VST3

### Next Steps (Post-Current Implementation)

#### Phase 1: Test Single Material
1. ✅ Enhanced scripts created
2. ⏳ Generate granite knob (--quick)
3. ⏳ Preview composite
4. ⏳ Integrate into JUCE
5. ⏳ Test in DAW

#### Phase 2: Scale to All Parameters (Optional)

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

---

## V. Future Roadmap

### Implementation Phases (Post-DSP Finalization)

**Timeline:** 4-6 weeks
**Priority:** Medium-High (impacts UX but not core functionality)

#### Phase 1: Foundation (Week 1)

**Goal:** Fix critical visual issues and establish design system.

**Tasks:**
1. Implement new color palette across all components
2. Create MonumentLookAndFeel class for consistent styling
3. Redesign top toolbar with two-row layout
4. Fix dropdown styling (stone texture, proper contrast)
5. Update spacing system (8px base unit)

**Deliverables:**
- MonumentLookAndFeel.h/.cpp
- Updated PluginEditor.cpp with new color palette
- Two-row header layout implemented
- Visual consistency across all UI elements

**Success Criteria:**
- No visual overlaps or cramping
- Cohesive ancient monuments aesthetic
- WCAG AA contrast compliance

**Estimated Effort:** 5 days

#### Phase 2: Preset Browser (Week 2)

**Goal:** Implement hierarchical preset organization.

**Tasks:**
1. Add category metadata to PresetManager
2. Implement nested ComboBox with category headers
3. Add modulation/physical modeling badges
4. Style preset menu with icons and indentation
5. Test all 28 presets in categorized view

**Structure:**
```
PRESETS ▾
├─ 🏛️ Foundational Spaces (0-5)
│  ├─ Init Patch
│  ├─ Stone Hall
│  └─ ...
├─ 🌱 Living Spaces (6-11)
│  ├─ Breathing Stone [MOD: AudioFollower → Bloom]
│  └─ ...
├─ 📜 Remembering Spaces (12-17)
├─ 🌀 Evolving Spaces (18-22)
└─ 🔬 Physical Modeling Spaces (23-28)
```

**Success Criteria:**
- All presets organized into logical categories
- Clear visual distinction between preset types
- Faster preset discovery

**Estimated Effort:** 3 days

#### Phase 3: Base Parameters Reorganization (Week 2)

**Goal:** Create clear functional grouping when base params revealed.

**Layout Proposal: Functional Zones**
```
┌─────────────────────────────────────────────────┐
│  BASE PARAMETERS                     [HIDE ▴]   │
├─────────────────────────────────────────────────┤
│                                                  │
│  CHAMBER            GEOMETRY      ATMOSPHERE     │
│  ┌──────────────┐  ┌──────────┐  ┌───────────┐ │
│  │ Time   Mass  │  │ Warp     │  │ Air       │ │
│  │   ⚪     ⚪   │  │   ⚪      │  │   ⚪       │ │
│  │              │  │          │  │           │ │
│  │ Density Bloom│  │ Drift    │  │ Width     │ │
│  │   ⚪     ⚪   │  │   ⚪      │  │   ⚪       │ │
│  │              │  │          │  │           │ │
│  │ Size         │  │ Gravity  │  │ Mix       │ │
│  │   ⚪          │  │   ⚪      │  │   ⚪       │ │
│  │              │  │          │  │           │ │
│  │    [FREEZE]  │  │          │  │           │ │
│  └──────────────┘  └──────────┘  └───────────┘ │
└─────────────────────────────────────────────────┘
```

**Success Criteria:**
- Clear visual grouping when expanded
- No clutter or confusion
- Smooth animation (300ms ease)

**Estimated Effort:** 2 days

#### Phase 4: ModMatrix Enhancements (Week 3)

**Goal:** Add destination grouping and template presets.

**Enhancement 1: Destination Grouping**
- Add vertical dividers between destination groups
- Column header categories: MACRO CONTROLS | BASE PARAMS | PHYSICAL

**Enhancement 2: Modulation Template Presets**
1. **Breathing** - AudioFollower → Bloom (depth: 0.5)
2. **Chaotic** - ChaosAttractor X/Y → Warp/Density (depth: 0.4)
3. **Organic** - BrownianMotion → Drift/Gravity (depth: 0.6)
4. **Responsive** - EnvelopeTracker → PillarShape + AudioFollower → Width
5. **Clear All** - Remove all connections

**Success Criteria:**
- Templates apply instantly without clicks
- Visual grouping improves clarity
- Users can quickly explore modulation patterns

**Estimated Effort:** 2 days

#### Phase 5: Advanced Preset Browser (Week 4-5) - OPTIONAL

**Decision Point:** Implement only if Phase 2 nested ComboBox proves insufficient based on user feedback.

**Goal:** Implement dedicated preset panel with visual previews.

**Key Features:**
- Preset cards with visual chamber thumbnails
- Category filtering with segmented buttons
- Detail panel with full descriptions
- Search/filter capability
- Tag filters: #long-tail #short #bright #dark #modulated #tubes

**Success Criteria:**
- Preset browsing is explorative and intuitive
- Visual feedback helps users understand sonic character
- Panel integrates seamlessly with existing UI

**Estimated Effort:** 7-10 days

#### Phase 6: Polish and Testing (Week 6)

**Goal:** Final refinements and accessibility testing.

**Tasks:**
1. Add keyboard navigation to all components
2. Implement ARIA labels and screen reader support
3. Test in multiple DAWs (Ableton, Logic, Reaper, FL Studio)
4. Test on different screen sizes and DPI settings
5. Performance profiling (UI rendering <5% CPU)
6. User testing with 5-10 musicians
7. Fix all reported bugs and polish edge cases

**Success Criteria:**
- Keyboard navigation works flawlessly
- Screen reader announces all interactions
- No performance regressions
- Positive user feedback on discoverability

**Estimated Effort:** 5 days

### Accessibility Guidelines

**Keyboard Navigation:**
- Tab through entire UI
- Adjust knobs with arrow keys
- Shortcuts: Cmd+P (presets), Cmd+S (save), Cmd+M (modulation), Cmd+B (base params)

**Screen Reader Support:**
- Implement ARIA labels for all interactive elements
- State announcements: "Loaded preset: Cathedral of Glass"
- Connection announcements: "Created connection: Chaos Attractor to Warp, depth 50%"

**Visual Accessibility:**
- Never rely on color alone (use icons + text + shape)
- High contrast mode support (15:1 text contrast)
- Focus indicators (2px solid outline, 8:1 contrast)
- WCAG 2.1 AA compliance minimum

### Future Enhancements (Post-v1.0)

**v1.1 Enhancements:**
- Preset favorites and tags system
- User-created preset categories
- Preset sharing/export functionality
- Dark mode toggle (alternative color palette)

**v1.2 Enhancements:**
- Animated modulation strength visualization (flowing connections)
- Preset sonic visualization (spectrum/impulse response curves)
- Chamber thumbnail rendering system for presets
- Multi-preset comparison view

**v1.3 Enhancements:**
- Resizable UI with multiple window sizes
- Advanced modulation: probability gates, quantization UI
- Gesture recording and playback for automation
- Preset morphing interface (2D XY pad)

---

## Reference Documents

This master plan consolidates content from:

1. **ENHANCED_UI_SUMMARY.md** - Current state and enhanced knob system
2. **MONUMENT_UI_STRATEGIC_DESIGN_PLAN.md** - Strategic vision and design philosophy
3. **PHOTOREALISTIC_UI_IMPLEMENTATION_PLAN.md** - Technical implementation details
4. **PHOTOREALISTIC_UI_PROGRESS.md** - Progress tracking (historical)
5. **UI_UX_ROADMAP.md** - Comprehensive future roadmap

**Related Documentation:**
- [LAYERED_KNOB_DESIGN.md](LAYERED_KNOB_DESIGN.md) - Architecture details
- [LAYERED_KNOB_WORKFLOW.md](LAYERED_KNOB_WORKFLOW.md) - Original workflow
- [JUCE_BLEND_MODES_RESEARCH.md](JUCE_BLEND_MODES_RESEARCH.md) - Rendering techniques

**Scripts:**
- `generate_knob_blender_enhanced.py` - Main rendering script (615 lines)
- `run_blender_enhanced.sh` - Batch wrapper
- `preview_knob_composite_enhanced.py` - Composite preview tool

**JUCE Components:**
- [ui/LayeredKnob.h/.cpp](../../ui/LayeredKnob.h) - Base class
- [ui/PhotorealisticKnob.h](../../ui/PhotorealisticKnob.h) - Enhanced knob
- [ui/ModMatrixPanel.h/.cpp](../../ui/ModMatrixPanel.h) - Modulation editor

---

## Summary

✅ **Current Direction:** Macro-only UI with celestial assets and reactive overlay (2026-01-07)

✅ **Complete Enhanced System Available:** Photorealistic Blender-rendered knobs with 5 material variants (optional/future use)

✅ **Core Components Implemented:** EnhancedBackground, CollapsiblePanel, PhotorealisticKnob, HeaderBar, ModMatrixPanel

**Next Actions:**
1. Continue with current macro-only UI direction
2. Enhanced knob system available when needed for future phases
3. Follow 6-phase roadmap for comprehensive UI improvements post-DSP

**Estimated Timeline:** 4-6 weeks for complete UI enhancement (Phases 1-6)

**Total Consolidated:** 3,800+ lines of documentation → organized master plan

---

**Document Version:** 1.0 (Consolidated)
**Last Updated:** 2026-01-09
**Status:** Active - Consolidated from 5 source documents
