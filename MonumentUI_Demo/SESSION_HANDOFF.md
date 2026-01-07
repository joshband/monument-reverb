# Monument UI Demo - Session Handoff

**Date:** 2026-01-05 (Late Evening)
**Branch:** main (standalone project)
**Progress:** Foundation architecture complete (5 of 12 tasks - 42%)
**Status:** Core systems implemented, ready for component development

---

## Session Summary

Created a **standalone JUCE UI demo project** for Monument's new photorealistic UI system. This is designed to be built and tested independently, then reintegrated with Monument's DSP when the UI is working perfectly.

### What We Accomplished ✅

1. **✅ Project Structure** - CMake-based JUCE GUI application
2. **✅ Asset Organization** - Cleaned and organized panel/header/knob assets
3. **✅ Theme System** - Ancient/Void/Cathedral with material definitions
4. **✅ AssetManager** - Centralized asset loading with semantic keys
5. **✅ MonumentBodyComponent** - Asymmetric sculptural masking

### Files Created

**Project Root:**
- [MonumentUI_Demo/CMakeLists.txt](CMakeLists.txt) - JUCE project configuration
- [MonumentUI_Demo/SESSION_HANDOFF.md](SESSION_HANDOFF.md) - This file

**Assets (Organized):**
- `Assets/panels/` - 4 panel backgrounds (macro, foundation, modulation, temporal)
- `Assets/headers/` - 4 header bars
- `Assets/knobs/stone/` - 4 stone knob variants
- `Assets/knobs/crystal/` - 2 crystal glow overlays

**Source Code:**
- [Source/UI/Theme.h](Source/UI/Theme.h) - Theme definitions (100 lines)
- [Source/UI/AssetManager.h/cpp](Source/UI/AssetManager.h) - Asset management (150 lines)
- [Source/UI/MonumentBodyComponent.h/cpp](Source/UI/MonumentBodyComponent.h) - Sculptural container (130 lines)

---

## Architecture Overview

### Design Philosophy

**Key Principle:** Build UI standalone first, integrate DSP later

This approach:
- Reduces complexity during development
- Allows rapid iteration on visuals
- Minimizes risk to stable DSP code
- Makes testing easier

### Layered PBR Rendering System

The knob system is designed for **multi-layer RGBA compositing**:

```
Knob Visual Stack (bottom to top):
├─ Stone Base (albedo + roughness baked)
├─ Crystal Glow (RGBA with alpha channel)
├─ Metal Core (specular highlights)
└─ Rotation Indicator (vector or raster)
```

**Asset Requirements (To Be Created):**

You mentioned you'll create new artifacts with proper layered PBR:

```
knobs/
├─ stone/
│  ├─ albedo_01.png (diffuse color)
│  ├─ normal_01.png (surface bumps)
│  ├─ roughness_01.png (matte vs glossy)
│  └─ ao_01.png (ambient occlusion)
├─ crystal/
│  ├─ glow_blue_01.png (RGBA with alpha)
│  ├─ glow_white_01.png
│  └─ glow_gold_01.png
├─ core/
│  ├─ metal_brushed_01.png
│  └─ metal_polished_01.png
└─ indicator/
   └─ pointer_line.png
```

### Theme System

Three built-in themes:

**Ancient** (Default)
- Warm weathered stone
- Deep blue crystals
- Glow intensity: 0.4
- Panel response: 0.15
- Knob response: 0.35

**Void** (High Contrast)
- Black obsidian
- Blue-white glow
- Glow intensity: 0.8
- Panel response: 0.25
- Knob response: 0.50

**Cathedral** (Refined)
- Pale marble
- Gold crystal accents
- Glow intensity: 0.6
- Panel response: 0.10
- Knob response: 0.30

### Asset Manager

**Semantic Keys:**
- `"panel.macro.bg"` - Macro Cosmos panel background
- `"panel.foundation.bg"` - Foundation panel background
- `"panel.modulation.bg"` - Modulation Nexus panel background
- `"panel.temporal.bg"` - Temporal Vault panel background
- `"header.macro"` - Macro Cosmos header bar
- `"knob.stone.01"` - Stone knob variant 1
- `"knob.crystal.01"` - Crystal glow variant 1

**Key Features:**
- Centralized caching via `juce::ImageCache`
- Deterministic asset loading (no runtime file I/O)
- Theme-aware asset selection
- Debug helpers (`getAvailableKeys()`, `hasImage()`)

---

## Next Session Priorities

### **PRIORITY 1: Create Proper PBR Asset Layers** ⭐ (Recommended First)

Before continuing with code, generate proper layered assets:

**Tools to Use:**
- **Midjourney / DALL-E / Stable Diffusion** for base renders
- **Blender** for 3D knob renders with proper PBR materials
- **Substance Designer/Painter** for PBR texture maps
- **Photoshop/GIMP** for RGBA compositing

**Asset Generation Workflow:**
1. Generate stone base renders (orthographic, 512×512 or 1024×1024)
2. Create crystal glow overlays (RGBA with proper alpha channels)
3. Render metal core variants (brushed, polished, matte)
4. Create rotation indicator assets
5. Organize into proper folder structure

**Reference Prompts Available:**
- `UI Mockup/prompts.md` - Detailed generation prompts
- `UI Mockup/assetStrategy.md` - Asset organization strategy

### **PRIORITY 2: Implement StoneKnob Component** (2-3 hours)

**File:** `Source/Components/StoneKnob.h/cpp`

**Requirements:**
- Multi-layer RGBA compositing (stone + crystal + core)
- Deterministic knob taxonomy (parameter name → visual variant)
- Advanced rotation curves (non-linear, gesture-dependent)
- Physics-based animation (velocity, stiffness, damping)
- Audio-reactive glow (placeholder for DSP integration)
- 30 Hz animation cap for performance

**Key Features:**
```cpp
class StoneKnob : public juce::Slider
{
    // Layered rendering
    juce::Image stoneLayer;
    juce::Image crystalLayer;
    juce::Image coreLayer;

    // Animation state
    float visualValue = 0.f;
    float velocity = 0.f;
    float energyState = 0.f;

    // Gesture detection
    InputGesture currentGesture = Mouse;
};
```

**Reference:** `UI Mockup/knobAnimation.md` for complete spec

### **PRIORITY 3: Build PanelComponent** (1-2 hours)

**File:** `Source/Components/PanelComponent.h/cpp`

**Requirements:**
- Support for 4 panel types (Macro/Foundation/Modulation/Temporal)
- Header bar integration
- Child component management
- Subtle panel lighting (audio-reactive placeholder)

**Panel Specifications:**
| Panel | Background | Header | Glow | Response |
|-------|-----------|---------|------|----------|
| Macro Cosmos | Polished basalt with blue veins | Beveled with warm seam | Subtle | 0.15 |
| Foundation | Heavy uniform basalt | Thick squared | None | 0.05 |
| Modulation Nexus | Crystal cavern cut-open | Fractured with fissure | Strong | 0.25 |
| Temporal Vault | Obsidian with pressure rings | Minimal with inlay | Sealed | 0.10 |

**Reference:** `UI Mockup/panelReplacementMapping.md` for exact mapping

### **PRIORITY 4: Create ModulationMatrixComponent** (1 hour)

**File:** `Source/Components/ModulationMatrixComponent.h/cpp`

**Requirements:**
- 30 Hz animation (not 60 Hz!)
- Breathing animation (slow sine wave)
- Node grid (6 sources × 8 destinations)
- Connection lines (alpha-modulated)
- No audio-thread work

**Visual Language:**
- Nodes = modulation destinations
- Vertical columns = sources
- Lines = modulation depth (0-1)
- Motion = low-frequency breathing, not twitchy

**Reference:** `UI Mockup/juceMasking_animation.md`

### **PRIORITY 5: Main Application Scaffolding** (30 min)

**Files to Create:**
- `Source/Main.cpp` - JUCE application entry point
- `Source/MainComponent.h/cpp` - Top-level UI component

**MainComponent Structure:**
```cpp
class MainComponent : public juce::Component
{
    Monument::MonumentBodyComponent body;

    // Panels (to be added)
    // std::unique_ptr<PanelComponent> macroPanel;
    // std::unique_ptr<PanelComponent> foundationPanel;

    // Knobs (to be added after PanelComponent)
    // std::array<StoneKnob, 21> knobs;
};
```

---

## Remaining Tasks (7 of 12)

- [ ] **StoneKnob** - Layered rendering with deterministic taxonomy
- [ ] **PanelComponent** - 4 panel types with headers
- [ ] **ModulationMatrixComponent** - Animated node grid
- [ ] **Main.cpp + MainComponent** - Application scaffolding
- [ ] **Advanced Knob Animation** - Gesture curves + physics
- [ ] **Preset Morphing** - Visual-only parameter interpolation
- [ ] **Audio-Reactive Placeholders** - Hooks for future DSP integration

---

## Build Instructions

### Prerequisites

- CMake 3.22+
- JUCE 7.0.12 (auto-fetched by CMake)
- macOS 10.13+ / Windows 10+ / Linux
- C++20 compiler

### Build Steps

```bash
cd monument-reverb/MonumentUI_Demo

# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build -j8

# Run (macOS)
open build/MonumentUI_Demo_artefacts/Debug/Monument\ UI\ Demo.app

# Run (Linux/Windows)
./build/MonumentUI_Demo_artefacts/Debug/MonumentUI_Demo
```

### Expected Build Output

```
[  0%] Building BinaryData assets
[ 20%] Building UI components
[ 40%] Building Components
[ 60%] Linking Monument UI Demo
[100%] Build complete
```

---

## Design References

All reference documents in `UI Mockup/`:

**Core Architecture:**
- `juceDemoProject.md` - Complete JUCE architecture example
- `assetStrategy.md` - Asset organization and loading
- `projucerBinaryData.md` - BinaryData configuration

**Visual Design:**
- `prompts.md` - Midjourney/DALL-E generation prompts
- `panelReplacementMapping.md` - Panel composition rules
- `knobAnimation.md` - Advanced knob animation system
- `knobCurvesAudioReactive.md` - Gesture curves + audio reactivity

**Advanced Features:**
- `juceMasking_animation.md` - Asymmetric masking + animation
- `presetMorphingVisuals.md` - Preset transition system
- `projectHardeningPrompts.md` - Claude-ready implementation prompts

**Image Assets:**
- `images/panels/` - Panel backgrounds and headers
- `images/knobs/` - Knob variants (placeholders, need PBR layers)
- `images/Mockup-01.png` - UI mockup reference
- `images/Mockup-02.png` - Detail views
- `images/Mockup-03.png` - Animated states
- `images/finalUI.png` - Target final design

---

## Critical Design Decisions

### 1. Standalone First, DSP Integration Later

**Why:** Reduces complexity, allows rapid UI iteration, minimizes risk

**Integration Plan (Future):**
1. Build UI demo standalone
2. Test thoroughly
3. Copy components to Monument's `ui/` directory
4. Wire to AudioProcessor
5. Add audio-reactive features
6. Replace old PluginEditor

### 2. Layered PBR Rendering

**Why:** Maximum visual fidelity, full control over glow/lighting

**Trade-offs:**
- ✅ Pros: Photorealistic, flexible, theme-swappable
- ⚠️ Cons: Requires proper asset creation, slightly more GPU work

**Performance Target:** <2% CPU idle, <8% playing (same as current Monument)

### 3. Deterministic Knob Taxonomy

**Why:** No two knobs look identical, but visuals remain stable

**Implementation:**
```cpp
// Parameter name hash → deterministic stone/crystal/core variants
int hash = paramID.hashCode();
int stoneVariant = hash % 3 + 1;
int crystalVariant = hash % 2 + 1;
```

**Result:** "density" parameter always gets the same visual identity

### 4. 30 Hz Animation Cap

**Why:** Smooth enough for perception, efficient enough for 40+ knobs

**Exceptions:**
- Expand/collapse animations: 60 Hz (short duration)
- Knob drag: Event-driven (no timer)
- Static elements: No animation

### 5. Asymmetric Masking (Not Window Shape)

**Why:** Host compatibility - plugins cannot use non-rectangular windows

**Solution:** Rectangular window, Path-based masking for sculptural appearance

---

## Performance Envelope (Hard Limits)

| Element | Max FPS | Max Count | Cost per Frame |
|---------|---------|-----------|----------------|
| Background | Static | 1 | 0 ms |
| Panels | 20-30 Hz | 4 | 0.2 ms |
| Knobs (idle) | 0 Hz | 21 | 0 ms |
| Knobs (active) | 30 Hz | 1-3 | 0.15 ms/knob |
| Modulation Matrix | 30 Hz | 1 | 0.3 ms |
| **Total Target** | - | - | **<2% CPU** |

**Measurement Strategy:**
1. Profile with Instruments (macOS)
2. Check GPU usage (metal/opengl)
3. Test with 21 knobs all animating
4. Verify <8% CPU during playback

---

## Common Gotchas

### 1. Asset Keys Must Match Exactly

❌ Wrong: `"panel.macro"` (missing .bg)
✅ Correct: `"panel.macro.bg"`

**Fix:** Check `AssetManager::loadAllAssets()` for exact keys

### 2. Image Alpha Channels

PBR layers **MUST** have proper RGBA:
- Stone base: Opaque alpha (255)
- Crystal glow: Variable alpha for blend
- Core: Opaque or masked

**Fix:** Check in image editor (Photoshop/GIMP) that alpha channel exists

### 3. BinaryData Name Mangling

CMake converts paths to C++ identifiers:
- `Assets/panels/macro_cosmos_bg.png`
- → `BinaryData::macro_cosmos_bg_png`
- → `BinaryData::macro_cosmos_bg_pngSize`

**Fix:** Match the pattern in `AssetManager::loadAllAssets()`

### 4. Animation Timer Cleanup

**Always** stop timers in destructor:

```cpp
~MyComponent() override
{
    stopTimer();  // CRITICAL!
}
```

**Why:** Prevents crashes when component destroyed while timer running

### 5. Hit-Testing with Asymmetric Mask

Child components inside MonumentBodyComponent automatically clip to mask:

```cpp
// This is automatic - no extra code needed
body.addAndMakeVisible(knob);
```

**Why:** `reduceClipRegion()` in paint affects all children

---

## Testing Strategy

### Phase 1: Visual Verification
1. Build and run standalone app
2. Verify all assets load correctly
3. Check asymmetric mask shape
4. Test with different window sizes

### Phase 2: Component Testing
1. Add one knob, verify layered rendering
2. Test knob rotation (mouse drag)
3. Verify 30 Hz animation cap (use profiler)
4. Test all 4 panel types

### Phase 3: Performance
1. Profile with Instruments
2. Add 21 knobs, measure CPU
3. Test modulation matrix animation
4. Verify <2% idle CPU

### Phase 4: Theme Switching
1. Test Ancient → Void transition
2. Test Void → Cathedral transition
3. Verify no asset load failures

---

## Next Session Quick Start

**Option A: Continue with Code** (if happy with current placeholder assets)

```bash
cd monument-reverb/MonumentUI_Demo

# 1. Implement StoneKnob
touch Source/Components/StoneKnob.{h,cpp}
# Copy template from UI Mockup/juceDemoProject.md section 5

# 2. Update CMakeLists.txt to add new files
# 3. Build and test

cmake --build build -j8
open build/MonumentUI_Demo_artefacts/Debug/Monument\ UI\ Demo.app
```

**Option B: Generate Proper PBR Assets First** (recommended)

```bash
# 1. Use prompts from UI Mockup/prompts.md
# 2. Generate with Midjourney/Stable Diffusion/Blender
# 3. Process in Photoshop (RGBA layers)
# 4. Organize into Assets/knobs/{stone,crystal,core}/
# 5. Update CMakeLists.txt BinaryData section
# 6. Rebuild
```

---

## Questions for Next Session

1. **Asset Creation:** Use Blender for 3D knob renders, or stick with AI generation?
2. **Filmstrip vs Layers:** Pre-render 128 rotation frames, or rotate layers in realtime?
3. **Theme Scope:** Keep 3 themes, or add more (Industrial, Ethereal, etc.)?
4. **Integration Timeline:** When to merge back into Monument? (After all 12 tasks? Or earlier?)

---

## Token Budget Notes

**Session Cost:** ~$6-7 (within $12/day ceiling)
**Context Used:** ~97K/200K tokens (48%)
**Recommendation:** Good stopping point. Use this handoff for next session.

---

## Status: Ready for Next Session 🚀

**Completed:** 5 of 12 tasks (42%)
**Next Priority:** Generate proper PBR asset layers OR implement StoneKnob
**Estimated Time to MVP:** 8-12 hours (2-3 sessions)
**Estimated Time to Complete:** 20-25 hours (4-5 sessions)

**See Also:**
- Monument main project: `../NEXT_SESSION_HANDOFF.md`
- UI reference docs: `UI Mockup/*.md`
- Asset examples: `UI Mockup/images/`
