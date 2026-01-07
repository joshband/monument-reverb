# Next Session Checklist

Quick reference for continuing the Monument UI Demo project.

---

## Before You Start

### Option A: Asset Creation Path

**If you want to create proper PBR layers first:**

- [ ] Set up Blender / Midjourney / Stable Diffusion
- [ ] Review generation prompts in `UI Mockup/prompts.md`
- [ ] Generate 12 stone base variants (1024×1024 RGBA)
- [ ] Create crystal glow overlays with proper alpha channels
- [ ] Render metal core variants
- [ ] Organize into `Assets/knobs/{stone,crystal,core}/`
- [ ] Update `CMakeLists.txt` BinaryData section
- [ ] Rebuild project

### Option B: Code-First Path

**If you want to continue with placeholder assets:**

- [ ] Review `SESSION_HANDOFF.md` for context
- [ ] Check current progress (5 of 12 tasks complete)
- [ ] Decide which component to implement first

---

## Priority 1: StoneKnob Component

### Files to Create

- [ ] `Source/Components/StoneKnob.h`
- [ ] `Source/Components/StoneKnob.cpp`

### Implementation Checklist

- [ ] **Multi-layer rendering**
  - [ ] Load stone base layer from AssetManager
  - [ ] Load crystal glow layer
  - [ ] Load core metal layer
  - [ ] Composite with proper alpha blending

- [ ] **Deterministic taxonomy**
  - [ ] Hash parameter name to select variants
  - [ ] `"density"` → consistent stone/crystal/core
  - [ ] No randomness

- [ ] **Rotation system**
  - [ ] Value → angle mapping (-135° to +135°)
  - [ ] Non-linear perceptual curve
  - [ ] Visual-only physics (velocity, stiffness, damping)

- [ ] **Animation state**
  - [ ] `float visualValue` (smooth)
  - [ ] `float velocity` (inertia)
  - [ ] `float energyState` (glow intensity)

- [ ] **Gesture detection**
  - [ ] Mouse: precise, stiff
  - [ ] Touch: fluid, forgiving
  - [ ] MIDI: snappy, authoritative

- [ ] **Hit-testing**
  - [ ] Circular hit area
  - [ ] Respect knob radius (~48% of width)

- [ ] **Performance**
  - [ ] 30 Hz timer cap
  - [ ] Stop timer in destructor
  - [ ] Only repaint when needed

### Reference

See `UI Mockup/knobAnimation.md` for complete spec

---

## Priority 2: PanelComponent

### Files to Create

- [ ] `Source/Components/PanelComponent.h`
- [ ] `Source/Components/PanelComponent.cpp`

### Implementation Checklist

- [ ] **Constructor**
  - [ ] Accept panel type enum (Macro/Foundation/Modulation/Temporal)
  - [ ] Load appropriate background from AssetManager
  - [ ] Load appropriate header bar

- [ ] **Layout**
  - [ ] Header bar at top (48px height)
  - [ ] Background fills remainder
  - [ ] Support for child components

- [ ] **Audio-reactive lighting (placeholder)**
  - [ ] `float panelEnergy` state
  - [ ] Ultra-slow smoothing (1-2 second rise)
  - [ ] Panel-specific response weights
  - [ ] Subtle glow/vein enhancement

- [ ] **Panel-specific styling**
  - [ ] Macro Cosmos: Subtle blue halo
  - [ ] Foundation: Almost inert
  - [ ] Modulation Nexus: Vein glow
  - [ ] Temporal Vault: Pressure ring animation

### Reference

See `UI Mockup/panelReplacementMapping.md` for exact specs

---

## Priority 3: ModulationMatrixComponent

### Files to Create

- [ ] `Source/Components/ModulationMatrixComponent.h`
- [ ] `Source/Components/ModulationMatrixComponent.cpp`

### Implementation Checklist

- [ ] **Grid layout**
  - [ ] 6 sources (vertical columns)
  - [ ] 8 destinations (horizontal rows)
  - [ ] Calculate node positions dynamically

- [ ] **Animation**
  - [ ] 30 Hz timer (not 60 Hz!)
  - [ ] Breathing sine wave
  - [ ] Phase offset per node

- [ ] **Rendering**
  - [ ] Blue nodes (alpha-modulated)
  - [ ] Connection lines (optional)
  - [ ] Subtle glow

- [ ] **Performance**
  - [ ] Stop timer when not visible
  - [ ] No audio-thread work

### Reference

See `UI Mockup/juceMasking_animation.md` section 3

---

## Priority 4: Main Application Scaffolding

### Files to Create

- [ ] `Source/Main.cpp`
- [ ] `Source/MainComponent.h`
- [ ] `Source/MainComponent.cpp`

### Implementation Checklist

- [ ] **Main.cpp**
  - [ ] JUCE application class
  - [ ] Create main window
  - [ ] Set window size
  - [ ] Set window title

- [ ] **MainComponent**
  - [ ] Add MonumentBodyComponent
  - [ ] Layout panels inside body
  - [ ] Add test knobs
  - [ ] Theme selector (dropdown)

- [ ] **Testing**
  - [ ] Build successfully
  - [ ] Window opens
  - [ ] Asymmetric mask visible
  - [ ] No crashes

---

## Update CMakeLists.txt

After creating new files, add them to `target_sources()`:

```cmake
target_sources(MonumentUI_Demo PRIVATE
    Source/Main.cpp
    Source/MainComponent.h
    Source/MainComponent.cpp
    # ... existing files ...
    Source/Components/StoneKnob.h        # ADD
    Source/Components/StoneKnob.cpp      # ADD
    Source/Components/PanelComponent.h   # ADD
    Source/Components/PanelComponent.cpp # ADD
)
```

---

## Testing Checklist

### Visual Tests

- [ ] All assets load without errors
- [ ] Asymmetric mask shape looks correct
- [ ] Panels render with proper backgrounds
- [ ] Headers align with panels
- [ ] Knobs render all layers correctly

### Animation Tests

- [ ] Knobs rotate smoothly (30 Hz)
- [ ] Modulation matrix breathes (30 Hz)
- [ ] No animation when not visible
- [ ] Timers stop in destructors

### Performance Tests

- [ ] <2% CPU when idle
- [ ] <8% CPU with all animations
- [ ] Smooth resize
- [ ] No memory leaks

### Theme Tests

- [ ] Switch Ancient → Void
- [ ] Switch Void → Cathedral
- [ ] Switch Cathedral → Ancient
- [ ] No crashes during theme change

---

## Build Commands

```bash
# Configure (first time only)
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build -j8

# Run (macOS)
open build/MonumentUI_Demo_artefacts/Debug/Monument\ UI\ Demo.app

# Run (Linux)
./build/MonumentUI_Demo_artefacts/Debug/MonumentUI_Demo

# Clean build
rm -rf build && cmake -B build && cmake --build build -j8
```

---

## Debug Checklist

### If Build Fails

- [ ] Check CMakeLists.txt syntax
- [ ] Verify all files added to `target_sources()`
- [ ] Check for missing semicolons
- [ ] Verify JUCE headers included

### If Assets Don't Load

- [ ] Check BinaryData key names (case-sensitive!)
- [ ] Verify PNG files are valid RGBA
- [ ] Check AssetManager::loadAllAssets() calls
- [ ] Enable DBG() logging in AssetManager

### If Animation is Janky

- [ ] Check timer is 30 Hz (not 60 Hz or higher)
- [ ] Verify `stopTimer()` in destructor
- [ ] Profile with Instruments/perf
- [ ] Reduce animation complexity

### If Hit-Testing is Wrong

- [ ] Check `hitTest()` override present
- [ ] Verify circular radius calculation
- [ ] Test with different knob sizes
- [ ] Check MonumentBodyComponent mask

---

## Quick Reference

**Project Root:** `/Users/noisebox/Documents/3_Development/Repos/monument-reverb/MonumentUI_Demo`

**Key Files:**
- `SESSION_HANDOFF.md` - Detailed progress notes
- `README.md` - Project overview
- `CMakeLists.txt` - Build configuration
- `UI Mockup/*.md` - Design references

**Asset Keys:**
- `"panel.macro.bg"` - Macro Cosmos background
- `"panel.foundation.bg"` - Foundation background
- `"panel.modulation.bg"` - Modulation Nexus background
- `"panel.temporal.bg"` - Temporal Vault background
- `"knob.stone.01"` - Stone knob variant 1
- `"knob.crystal.01"` - Crystal glow variant 1

**Themes:**
- `Themes::Ancient` - Default warm stone
- `Themes::Void` - High contrast obsidian
- `Themes::Cathedral` - Refined marble

---

## Time Estimates

| Task | Estimated Time |
|------|----------------|
| Asset generation | 4-6 hours |
| StoneKnob | 2-3 hours |
| PanelComponent | 1-2 hours |
| ModulationMatrix | 1 hour |
| Main scaffolding | 30 minutes |
| Testing | 1 hour |
| Polish | 2-3 hours |

**Total to MVP:** 8-12 hours (2-3 sessions)
**Total to Complete:** 20-25 hours (4-5 sessions)

---

## Questions to Resolve

1. **Filmstrip vs Realtime Rotation?**
   - Pre-render 128 frames per knob, or rotate layers in realtime?
   - Filmstrip: Better quality, more memory
   - Realtime: Smaller size, slight perf cost

2. **Animation Frame Budget?**
   - 30 Hz is sufficient for smooth perception
   - 60 Hz only for critical UI (expand/collapse)

3. **Theme Selector Location?**
   - Top-right dropdown?
   - Settings panel?
   - Keyboard shortcut only?

4. **Integration Timeline?**
   - Merge after all 12 tasks? (safer)
   - Merge after MVP (6-7 tasks)? (faster feedback)

---

**Ready to continue! 🚀**

See `SESSION_HANDOFF.md` for detailed context.
