# Next Session - Ancient Monuments Macro Expansion

**Date:** 2026-01-04
**Status:** 🚧 Phase 5 In Progress - Macro System Expansion
**Theme:** Ancient Monuments 🗿 - Poetic architectural aesthetic

---

## 🎯 What We Completed Today

### ✅ UI Enhancements (Complete)
1. **Base Parameters Toggle** - Added "BASE PARAMS" button to show/hide 12 base parameters
2. **Compact Default View** - Hides base params by default (260px height), shows only 6 macros
3. **Dynamic Window Sizing** - 260px (macros only) → 580px (with base) → 1080px (with mod matrix)
4. **Fixed Gravity & Freeze** - Both now properly visible and labeled in base params grid

### ✅ Ancient Monuments Design (Complete)
**Final 10-Macro Set:**
```
STONE      LABYRINTH    MIST      BLOOM      TEMPEST    ECHO
PATINA     ABYSS        CORONA    BREATH
```

**Sonic Mappings:**
- **STONE** (was Material): Foundation hardness → mass, density, time
- **LABYRINTH** (was Topology): Spatial complexity → warp, drift
- **MIST** (was Viscosity): Atmospheric density → time, air, mass
- **BLOOM** (unchanged): Organic growth → bloom envelope, drift
- **TEMPEST** (was Chaos): Storm intensity → warp, drift chaos
- **ECHO** (was Elasticity): Resonating memory → temporal response
- **PATINA** ⭐ NEW: Surface weathering → density, air, bloom texture
- **ABYSS** ⭐ NEW: Infinite depth → size, time, width
- **CORONA** ⭐ NEW: Sacred radiance → bloom, air, warp shimmer
- **BREATH** ⭐ NEW: Living pulse → bloom, drift, gravity rhythm

### ✅ MacroMapper.h Updates (Complete)
- Updated `MacroInputs` struct with Ancient Monuments naming
- Added backward compatibility aliases (deprecated warnings)
- Updated all documentation comments to reflect poetic theme
- Renamed all private mapping functions to match new theme
- Added 4 new mapping function signatures (Patina, Abyss, Corona, Breath)

---

## 📋 What Remains (Priority Order)

### Priority 1: MacroMapper.cpp Implementation
**File:** `dsp/MacroMapper.cpp`
**Tasks:**
1. Update `computeTargets()` function signature (10 params)
2. Rename all existing mapping function implementations
3. Implement 4 new mapping functions:
   - `mapPatinaToDensity/Air/Bloom()` - Texture weathering
   - `mapAbyssToSize/Time/Width()` - Spatial depth
   - `mapCoronaToBloom/Air/Warp()` - Radiant shimmer
   - `mapBreathToBloom/Drift/Gravity()` - Living pulse
4. Add 3-influence `combineInfluences()` overload implementation

**Mapping Design:**
```cpp
// PATINA: Weathered texture (rough reflections)
density = combine(density, mapPatinaToDensity(patina), 0.6);
air = combine(air, mapPatinaToAir(patina), -0.4);  // Negative = inverse
bloom = combine(bloom, mapPatinaToBloom(patina), 0.3);

// ABYSS: Infinite depth
size = combine(size, mapAbyssToSize(abyss), 0.7);
time = combine(time, mapAbyssToTime(abyss), 0.5);
width = combine(width, mapAbyssToWidth(abyss), 0.3);

// CORONA: Sacred shimmer
bloom = combine(bloom, mapCoronaToBloom(corona), 0.8);
air = combine(air, mapCoronaToAir(corona), 0.6);
warp = combine(warp, mapCoronaToWarp(corona), 0.3);

// BREATH: Living pulse
bloom = combine(bloom, mapBreathToBloom(breath), 0.5);
drift = combine(drift, mapBreathToDrift(breath), 0.6);
gravity = combine(gravity, mapBreathToGravity(breath), 0.4);
```

---

### Priority 2: PluginProcessor APVTS Updates
**File:** `plugin/PluginProcessor.cpp` (createParameterLayout)
**Tasks:**
1. Add 4 new APVTS parameters:
   ```cpp
   params.push_back(std::make_unique<juce::AudioParameterFloat>(
       "patina", "Patina",
       juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
   params.push_back(std::make_unique<juce::AudioParameterFloat>(
       "abyss", "Abyss",
       juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
   params.push_back(std::make_unique<juce::AudioParameterFloat>(
       "corona", "Corona",
       juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
   params.push_back(std::make_unique<juce::AudioParameterFloat>(
       "breath", "Breath",
       juce::NormalisableRange<float>(0.0f, 1.0f), 0.0f));
   ```
2. Update `processBlock()` to read new macro values
3. Pass 10 macros to MacroMapper::computeTargets()

---

### Priority 3: PluginEditor UI Updates
**File:** `plugin/PluginEditor.h` + `plugin/PluginEditor.cpp`
**Tasks:**
1. Add 4 new HeroKnob member variables:
   ```cpp
   HeroKnob patinaKnob;
   HeroKnob abyssKnob;
   HeroKnob coronaKnob;
   HeroKnob breathKnob;
   ```
2. Initialize in constructor with APVTS attachments
3. Update `resized()` layout for 10-knob single row:
   ```cpp
   const auto macroWidth = macroArea.getWidth() / 10;  // Was /6
   // Add 4 new knob bounds...
   ```
4. Optional: Rename existing knob variables to match Ancient Monuments theme

---

### Priority 4: Preset Migration
**File:** `plugin/PresetManager.cpp`
**Tasks:**
1. Update preset format version to v4
2. Add migration logic for v3 → v4:
   - Old presets default new macros to 0.5 (patina, abyss, corona)
   - Breath defaults to 0.0
3. Update factory presets to include new macro values
4. Test backward compatibility

---

### Priority 5: Documentation Updates
**Files:** `CHANGELOG.md`, `README.md`, `docs/`
**Tasks:**
1. Add Phase 5 changelog entry
2. Update README with Ancient Monuments theme
3. Document macro mapping relationships
4. Update UI screenshots (if applicable)

---

## 🚀 Quick Start Commands

### Build & Test
```bash
# Build plugin
cmake --build build

# Launch standalone
open build/Monument_artefacts/Debug/Standalone/Monument.app

# Kill running instance
killall Monument
```

### Current Build Status
✅ Builds successfully with MacroMapper.h updates
⚠️ MacroMapper.cpp needs implementation before functional

---

## 📊 Current Status

**Phase Completion:**
- ✅ Phase 1-4: Core reverb + 6 macros (Complete)
- 🚧 Phase 5: Ancient Monuments expansion (40% complete)
  - ✅ Design & architecture
  - ✅ Header file updates
  - ⏳ Implementation (MacroMapper.cpp)
  - ⏳ UI integration
  - ⏳ Preset migration

**Token Budget:** ~97K remaining (safe to continue)

---

## 🎨 Visual Reference

**Current UI (6 macros):**
```
[STONE] [LABYRINTH] [MIST] [BLOOM] [TEMPEST] [ECHO]
```

**Target UI (10 macros):**
```
[STONE] [LABYRINTH] [MIST] [BLOOM] [TEMPEST] [ECHO] [PATINA] [ABYSS] [CORONA] [BREATH]
```

---

## 💡 Notes

- **Backward Compatibility**: Old presets will load correctly (deprecated aliases in MacroInputs)
- **Build Status**: Compiles with warnings (expected during transition)
- **Testing**: Test with factory presets after implementation complete
- **UI Sizing**: 10 knobs fit comfortably in 900px width (90px per knob)

---

**Ready to continue Phase 5! 🗿**
**Next steps:** Implement MacroMapper.cpp → PluginProcessor → PluginEditor
