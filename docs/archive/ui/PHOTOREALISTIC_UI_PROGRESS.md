# Monument Reverb - Photorealistic UI Rebuild Progress

**Date:** 2026-01-05 (Night Session)
**Goal:** Rebuild UI from ground up using mockup designs
**Status:** Core components complete, ready for PluginEditor integration

**Update (2026-01-07):** This document is now historical. The plugin UI has moved to a macro-only surface with celestial asset layers and a reactive macro overlay.

---

## ✅ Completed Components (Session 1)

### 1. EnhancedBackgroundComponent
**File:** [ui/EnhancedBackgroundComponent.h/cpp](../ui/EnhancedBackgroundComponent.h)

**Features:**
- Dark weathered stone texture (#0d0d0d to #1a1a1a gradient)
- Animated blue ethereal wisps (8-12 floating fog effects)
- Breathing animation with 30 FPS smooth updates
- Panel dividers with embossed shadow effect
- Performance optimized with cached stone texture generation

**Visual Effects:**
- Blue/cyan wisps with radial gradients
- Slow horizontal/vertical drift animation
- Wrap-around screen edges
- Subtle alpha pulsing based on time
- Multi-octave Perlin noise for stone texture

**Status:** ✅ **Complete and building successfully**

---

### 2. CollapsiblePanel
**File:** [ui/CollapsiblePanel.h/cpp](../ui/CollapsiblePanel.h)

**Features:**
- Smooth expand/collapse animation (300ms with ease-out cubic)
- Expandable header with arrow indicator (▶/▼)
- Dark theme styling (#1a1a1a background)
- Content component management
- Callback support for expanded state changes

**Animation:**
- 60 FPS timer for smooth transitions
- Eased animation using cubic easing function
- Dynamic height adjustment
- Parent resize notification for layout updates

**Status:** ✅ **Complete and building successfully**

---

### 3. Enhanced PhotorealisticKnob
**File:** [ui/PhotorealisticKnob.h/cpp](../ui/PhotorealisticKnob.h) (modified)

**New Features:**
- **Animated blue LED center glow** (matches mockup design)
- Breathing pulse effect (slow sine wave, 0.6-1.0 intensity)
- Hover glow intensity increase
- Multi-layer rendering:
  - Outer glow (soft, large radius)
  - Middle glow (bright, medium radius)
  - Inner core (cyan #88ccff)
  - Bright center dot (white LED point)

**Animation:**
- 30 FPS timer for smooth breathing
- Gradual hover intensity ramp (0.1/frame in, 0.05/frame out)
- Total intensity = pulse + hover bonus

**Visual Characteristics:**
- LED radius: 15% of knob size
- Colors: #88ccff (cyan core) + #4488ff (blue glow)
- 4-layer radial gradient compositing
- Matches mockup's glowing blue centers

**Status:** ✅ **Complete and building successfully**

---

### 4. HeaderBar
**File:** [ui/HeaderBar.h/cpp](../ui/HeaderBar.h)

**Features:**
- MONUMENT logo/title (left aligned)
- Preset selector dropdown (center-left)
- Hall/Wall selector (center)
- Architecture dropdown (center-right)
- Input/output level meters with gradient (right)

**Level Meters:**
- Vertical bars with rounded corners
- Green → Yellow → Red gradient (clip warning)
- Glow effect scales with level
- Dark background (#0d0d0d)

**Status:** ✅ **Complete and building successfully**

---

## 🏗️ Build Status

**CMakeLists.txt:** Updated with all new components
**Compilation:** ✅ **100% successful** (all warnings resolved)
**Targets Built:**
- Monument (Shared Code)
- Monument_Standalone
- Monument_AU
- Monument_VST3

---

## 📋 Next Steps (Priority Order)

### Step 1: Rebuild PluginEditor Layout (2-3 hours)

**Goal:** Replace current layout with collapsible panel system

**Tasks:**
1. Replace `StoneBackgroundComponent` with `EnhancedBackgroundComponent`
2. Add `HeaderBar` at top (60px height)
3. Create three `CollapsiblePanel` instances:
   - **"THE MACRO CONTROL"** (12 knobs, 2 rows of 6)
   - **"THE FOUNDATION"** (11 knobs, single row)
   - **"THE MODULATION NEXUS"** (timeline editor)
4. Reorganize knobs into panel content components
5. Implement panel layout with proper spacing

**Layout Structure:**
```
┌───────────────────────────────────────┐
│ HeaderBar (60px)                       │  ← MONUMENT logo, presets, meters
├───────────────────────────────────────┤
│ ▶ THE MACRO CONTROL                    │  ← Collapsible
│   [6 knobs: Material, Topology, ...]  │
│   [6 knobs: Time, Bloom, ...]         │
├───────────────────────────────────────┤
│ ▶ THE FOUNDATION                       │  ← Collapsible
│   [11 knobs in single row]            │
├───────────────────────────────────────┤
│ ▶ THE MODULATION NEXUS                 │  ← Collapsible
│   [Timeline component]                 │
└───────────────────────────────────────┘
```

**Files to Modify:**
- [plugin/PluginEditor.h](../plugin/PluginEditor.h) - Update member variables
- [plugin/PluginEditor.cpp](../plugin/PluginEditor.cpp) - Rebuild constructor and resized()

---

### Step 2: Style Timeline Component (1-2 hours)

**Goal:** Match mockup's dark theme with orange keyframes

**Current State:** Timeline has white background (from earlier fix)

**Changes Needed:**
- Background: Dark stone (#1a1a1a)
- Keyframe dots: Orange (#ff8844) with glow
- Parameter lanes: Subtle dark grey dividers
- Playhead: Blue glowing beam
- Labels: Light grey text (#c0c0c0)

**Files to Modify:**
- [ui/TimelineComponent.cpp](../ui/TimelineComponent.cpp)

---

### Step 3: Add Modulation Glow Effects (2-3 hours)

**Goal:** Visual feedback for modulated parameters

**Features to Add:**
- Blue glow rings around modulated knobs
- Animated connection lines from mod matrix to knobs
- Pulsing intensity based on modulation depth
- Flow animation along connection lines

**Implementation:**
- Extend `PhotorealisticKnob::setModulated()` to show outer glow
- Create `ModulationConnectionOverlay` component
- Add to PluginEditor as transparent overlay layer

---

### Step 4: Performance Optimization (1-2 hours)

**Goal:** Achieve 60 FPS target with minimal CPU usage

**Tasks:**
- Profile rendering with Instruments
- Cache filmstrip frames if needed
- Optimize wisp animation (reduce count if needed)
- Test on different screen sizes/resolutions
- Measure CPU usage (target: <2% idle, <8% playing)

---

## 🎨 Design Reference

**Mockup Images:**
- `~/Desktop/Monument Mockups/ChatGPT Image Jan 5, 2026, 04_44_42 PM.png`
- `~/Desktop/Monument Mockups/ChatGPT Image Jan 5, 2026, 04_45_25 PM.png`

**Knob Sources:**
- `~/Desktop/knob_sources/` - 32 stone knob PNGs with glowing LED centers
- Already processed into filmstrips in `assets/ui/knobs_photorealistic/`

---

## 🔧 Technical Details

### Color Palette

| Element | Color | Hex Code |
|---------|-------|----------|
| Dark Base | Very dark grey | #0d0d0d |
| Dark Mid | Dark grey | #1a1a1a |
| Dark Highlight | Lighter grey | #242428 |
| Text | Light grey | #c0c0c0 |
| Blue Wisp | Deep blue | #4488ff |
| Cyan Wisp | Bright cyan | #88ccff |
| LED Core | Bright cyan | #88ccff |
| Timeline Keyframe | Orange | #ff8844 |
| Accent | Blue | #4488ff |

### Performance Targets

| Metric | Target | Current |
|--------|--------|---------|
| Frame Rate | 60 FPS | TBD (needs testing) |
| CPU Idle | <2% | TBD |
| CPU Playing | <8% | TBD |
| Memory | <50 MB | TBD |
| Load Time | <200ms | TBD |

### Animation Timers

| Component | FPS | Purpose |
|-----------|-----|---------|
| EnhancedBackground | 30 | Wisp animation |
| PhotorealisticKnob | 30 | LED breathing |
| CollapsiblePanel | 60 | Expand/collapse |
| TimelineComponent | 30 | Playhead updates |

---

## 📝 Implementation Notes

### JUCE API Changes

**Font Constructor:**
- ❌ Old: `juce::Font(14.0f, juce::Font::bold)`
- ✅ New: `juce::FontOptions(14.0f, juce::Font::bold)`

**Radial Gradients:**
- ❌ Old: `gradient.setGradientRadius(radius)`
- ✅ New: Specify edge point in constructor to define radius
```cpp
juce::Point<float> center(x, y);
juce::Point<float> edge(x + radius, y);
juce::ColourGradient gradient(colorStart, center, colorEnd, edge, true);
```

### Best Practices

1. **Timer Management:**
   - Always call `stopTimer()` in destructor
   - Use visibility checks to pause timers when hidden
   - Target 30 FPS for animations (60 FPS for critical UI)

2. **Performance:**
   - Cache generated textures (stone, noise)
   - Use `repaint()` only when needed
   - Avoid allocations in timer callbacks

3. **Memory:**
   - Use `std::unique_ptr` for owned components
   - Clean up LookAndFeel objects properly
   - Avoid memory leaks in component hierarchies

---

## 🚀 Quick Commands

### Build & Test
```bash
# Full rebuild
cmake --build build --target Monument_All -j8

# Run standalone for testing
open build/monument_standalone_artefacts/Debug/Monument.app

# Install to system
./scripts/rebuild_and_install.sh all
```

### Development Workflow
```bash
# 1. Make changes to UI components
# 2. Build (see above)
# 3. Test in standalone
# 4. Iterate until design matches mockup
# 5. Profile performance with Instruments
# 6. Optimize if needed
```

---

## 📸 Visual Comparison

**Before (Current):**
- Basic dark background
- Stone knobs without LED animation
- White timeline background
- No collapsible panels
- Static UI

**After (Target):**
- Animated ethereal wisps
- Glowing blue LED centers (breathing)
- Dark timeline with orange keyframes
- Three collapsible sections
- Dynamic, atmospheric UI

---

## ✨ Key Achievements

1. **Component Architecture:** Clean separation of concerns with reusable components
2. **Animation System:** Smooth 30-60 FPS animations with proper easing
3. **Visual Fidelity:** Matches mockup design (blue ethereal wisps, LED glow)
4. **Build System:** All components integrated and compiling successfully
5. **Performance Conscious:** Timer-based updates, cached textures, efficient rendering

---

## 🎯 Next Session Priority

**Recommended:** Complete PluginEditor layout integration (Step 1)

**Why:** All building blocks are ready - just need to assemble them into the final layout. This will immediately reveal the new UI design and allow visual testing.

**Estimated Time:** 2-3 hours to fully integrate and test

**Expected Result:** Fully functional photorealistic UI matching mockup design

---

**Status:** Ready for PluginEditor integration 🚀
