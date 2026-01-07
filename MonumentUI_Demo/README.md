# Monument UI Demo

**Standalone JUCE UI prototype for Monument Reverb's photorealistic interface**

![Status](https://img.shields.io/badge/status-in%20development-yellow)
![Progress](https://img.shields.io/badge/progress-42%25-blue)
![JUCE](https://img.shields.io/badge/JUCE-7.0.12-green)

---

## Overview

This is a **standalone UI demo project** for Monument Reverb's new photorealistic interface. It's designed to be built and tested independently before reintegration with the DSP code.

### Why Standalone?

- ✅ **Faster Iteration** - No DSP complexity during UI development
- ✅ **Reduced Risk** - Stable DSP code remains untouched
- ✅ **Easier Testing** - Pure UI focus without audio setup
- ✅ **Clean Integration** - Copy working components back when ready

---

## Features

### Implemented ✅

- **Theme System** - Ancient / Void / Cathedral material palettes
- **Asset Manager** - Centralized loading with semantic keys
- **Asymmetric Masking** - Sculptural UI shape with rectangular window
- **PBR Layer Support** - Multi-layer RGBA compositing architecture

### In Progress 🚧

- **StoneKnob** - Photorealistic knobs with layered rendering
- **PanelComponent** - 4 panel types (Macro/Foundation/Modulation/Temporal)
- **ModulationMatrix** - Animated node grid (30 Hz)
- **Advanced Animation** - Gesture-dependent curves + physics

### Planned 📋

- **Preset Morphing** - Visual parameter interpolation
- **Audio-Reactive** - Glow responds to audio energy
- **Performance Optimization** - <2% CPU target

---

## Quick Start

### Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j8
```

### Run

```bash
# macOS
open build/MonumentUI_Demo_artefacts/Debug/Monument\ UI\ Demo.app

# Linux
./build/MonumentUI_Demo_artefacts/Debug/MonumentUI_Demo
```

---

## Project Structure

```
MonumentUI_Demo/
├── CMakeLists.txt              # JUCE project configuration
├── README.md                    # This file
├── SESSION_HANDOFF.md          # Detailed session notes
│
├── Assets/                      # All UI assets
│   ├── panels/                  # Panel backgrounds (4)
│   ├── headers/                 # Header bars (4)
│   └── knobs/                   # Knob layers
│       ├── stone/               # Stone base layers
│       ├── crystal/             # Crystal glow overlays
│       └── core/                # Metal core centers
│
└── Source/
    ├── Main.cpp                 # Application entry (TODO)
    ├── MainComponent.h/cpp      # Top-level UI (TODO)
    │
    ├── UI/                      # Core UI systems
    │   ├── Theme.h              # Theme definitions
    │   ├── AssetManager.h/cpp   # Asset loading
    │   └── MonumentBodyComponent.h/cpp  # Sculptural container
    │
    └── Components/              # UI components (TODO)
        ├── StoneKnob.h/cpp
        ├── PanelComponent.h/cpp
        └── ModulationMatrixComponent.h/cpp
```

---

## Design Philosophy

### 1. Layered PBR Rendering

Knobs use **multi-layer RGBA compositing** for photorealistic appearance:

```
Knob Visual Stack:
├─ Stone Base (albedo + roughness)
├─ Crystal Glow (RGBA with alpha)
├─ Metal Core (specular)
└─ Indicator (rotation pointer)
```

### 2. Deterministic Taxonomy

Parameter names map to consistent visual variants:
- `"density"` → always gets the same stone/crystal/core combo
- No random variation at runtime
- Visual identity stable across sessions

### 3. Performance First

- 30 Hz animation cap (not 60 Hz)
- Cached rendering for static elements
- GPU-accelerated where available
- Target: <2% CPU idle, <8% playing

### 4. Theme System

Three material palettes:
- **Ancient** - Warm stone, blue crystals, subdued glow
- **Void** - Black obsidian, blue-white glow, high intensity
- **Cathedral** - Pale marble, gold accents, refined

---

## Asset Requirements

### Current Assets (Placeholder)

Located in `Assets/`:
- 4 panel backgrounds
- 4 header bars
- 4 stone knob variants
- 2 crystal glow overlays

### Needed Assets (TODO)

For full PBR rendering, generate:

```
knobs/
├── stone/
│   ├── albedo_XX.png (1024×1024 RGBA)
│   ├── normal_XX.png (bump maps)
│   ├── roughness_XX.png (matte/glossy)
│   └── ao_XX.png (ambient occlusion)
├── crystal/
│   ├── glow_blue_XX.png (RGBA with alpha)
│   ├── glow_white_XX.png
│   └── glow_gold_XX.png
├── core/
│   ├── metal_brushed_XX.png
│   └── metal_polished_XX.png
└── indicator/
    └── pointer_line.png
```

**Generation Tools:**
- Midjourney / DALL-E / Stable Diffusion
- Blender (3D renders with PBR materials)
- Substance Designer/Painter
- Photoshop/GIMP (RGBA compositing)

**Prompts:** See `UI Mockup/prompts.md`

---

## Architecture

### Theme System

```cpp
// Define themes with material properties
Theme ancient {
    "Ancient",
    Colour(0xff0e0e10),  // Background
    0.4f,                 // Glow intensity
    "stone",              // Stone set
    "crystal_blue"        // Crystal set
};
```

### Asset Manager

```cpp
// Load assets with semantic keys
auto& am = AssetManager::instance();
auto panel = am.getImage("panel.macro.bg");
auto knob = am.getImage("knob.stone.01");
```

### Layered Rendering

```cpp
// Composite multiple RGBA layers
g.drawImageAt(stoneBase, x, y);
g.drawImageAt(crystalGlow, x, y);  // Alpha blend
g.drawImageAt(metalCore, x, y);
```

---

## Performance Targets

| Metric | Target | Measured |
|--------|--------|----------|
| CPU (idle) | <2% | TBD |
| CPU (playing) | <8% | TBD |
| Frame rate | 30-60 Hz | TBD |
| Memory | <50 MB | TBD |
| Load time | <200 ms | TBD |

**Measurement:**
- macOS: Instruments (Time Profiler + Metal)
- Linux: perf / gprof
- Windows: Visual Studio Profiler

---

## References

All design docs in `UI Mockup/`:

**Core:**
- `juceDemoProject.md` - Complete architecture example
- `assetStrategy.md` - Asset organization
- `SESSION_HANDOFF.md` - Current progress

**Visual Design:**
- `prompts.md` - Asset generation prompts
- `panelReplacementMapping.md` - Panel specs
- `knobAnimation.md` - Animation system

**Advanced:**
- `juceMasking_animation.md` - Masking + animation
- `knobCurvesAudioReactive.md` - Gesture curves
- `presetMorphingVisuals.md` - Preset transitions

---

## Integration Plan

### Phase 1: Build UI Demo (Current)

1. ✅ Foundation architecture
2. 🚧 Implement components
3. Test standalone thoroughly
4. Polish visuals

### Phase 2: Reintegrate with DSP

1. Copy components to Monument's `ui/` directory
2. Wire to AudioProcessor
3. Add audio-reactive features
4. Replace old PluginEditor
5. Test in DAW hosts

### Phase 3: Polish & Ship

1. Performance optimization
2. Cross-platform testing
3. Visual regression tests
4. Final polish

---

## Contributing

This is a prototype project - see `../monument-reverb` for the main DSP code.

When ready to integrate:
1. Ensure all tests pass
2. Profile performance
3. Copy components to main project
4. Wire to AudioProcessor
5. Update main project's PluginEditor

---

## License

Same as Monument Reverb (see parent project)

---

## Status

**Progress:** 5 of 12 tasks complete (42%)

**Next Session:**
- Generate proper PBR asset layers, OR
- Implement StoneKnob component

**Questions?** See `SESSION_HANDOFF.md` for detailed notes

---

**Built with [JUCE](https://juce.com) 7.0.12**
