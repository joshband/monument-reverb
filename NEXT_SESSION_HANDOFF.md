# Monument Reverb - Three-System Plan Session Handoff

**Date:** 2026-01-05
**Branch:** `feature/three-systems`
**Progress:** 23 of 42 tasks (55% complete)
**Status:** Phase 3 COMPLETE ✅ - Ready for Phase 4

---

## Session Summary

**Completed:** Phases 1, 2, and 3 of Three-System Plan
- ✅ **Phase 1:** Spatial positioning with 1/r² distance attenuation
- ✅ **Phase 2:** 3D azimuth/elevation panning in Facade
- ✅ **Phase 3:** Doppler shift effects in Chambers

**Build:** ✅ All plugins compile successfully
**Tests:** ✅ Doppler stability tests pass

---

## Completed Work (Tasks 1-23)

### Phase 1: Spatial Basics ✅ (Committed: c19b7d9)
- Created [dsp/SpatialProcessor.h/cpp](dsp/SpatialProcessor.h) - 1/r² attenuation
- Modified [dsp/Chambers.h/cpp](dsp/Chambers.h) - FDN integration at line ~630
- Extended [dsp/ModulationMatrix.h](dsp/ModulationMatrix.h) - PositionX/Y/Z (lines 69-71)
- Updated [plugin/PresetManager.cpp](plugin/PresetManager.cpp) - Serialization (667-669, 711-713)
- Wired [plugin/PluginProcessor.cpp](plugin/PluginProcessor.cpp) - Spatial routing (567-582)

### Phase 2: 3D Panning ✅ (Committed: bf376c0)

- Modified [dsp/DspModules.h](dsp/DspModules.h) - Facade methods (182-200, 213-220)
- Implemented [dsp/DspModules.cpp](dsp/DspModules.cpp) - Constant power panning (623-627, 669-756)
- Extended [dsp/ModulationMatrix.h](dsp/ModulationMatrix.h) - Distance, VelocityX (73-74)
- Updated [plugin/PresetManager.cpp](plugin/PresetManager.cpp) - String conversion (671-672, 718-719)
- Created [tests/ConstantPowerPanningTest.cpp](tests/ConstantPowerPanningTest.cpp) - Unit test
- Updated [CMakeLists.txt](CMakeLists.txt) - Test target (284-313)

### Phase 3: Doppler Effects ✅ (Committed: e6a7bfd)

- Modified [dsp/Chambers.cpp:628-644](dsp/Chambers.cpp#L628) - Doppler shift integration
- Integrated `getDopplerShift()` into fractional delay calculation
- Velocity-based pitch shifting (±50ms max shift @ 48kHz)
- Created [tests/DopplerShiftTest.cpp](tests/DopplerShiftTest.cpp) - Stability unit test
- Updated [CMakeLists.txt](CMakeLists.txt) - Test target (316-333)
- **Tests:** 6 test cases pass (bounds, scaling, disable, stability, per-line, clipping)

---

## NEXT: Phase 4 - SequenceScheduler (Tasks 24-31)

**Overview:** Timeline system for parameter automation and preset morphing

**Task 24:** Create [dsp/SequenceScheduler.h/cpp](dsp/SequenceScheduler.h)
**Task 25:** Implement keyframe storage and interpolation
**Task 26:** Add timeline playback with tempo sync
**Task 27:** Wire to PluginProcessor modulation system
**Task 28:** Create 3 timeline presets (Evolving Cathedral, Spatial Journey, Living Space)
**Task 29:** Write SequenceScheduler unit test
**Task 30:** Build and verify Phase 4
**Task 31:** Manual testing in DAW

---

## Key Implementation Details

### Spatial Attenuation (Phase 1)
```cpp
// dsp/Chambers.cpp:627-636
if (spatialProcessor)
    spatialProcessor->process();  // Block-rate update

// In FDN loop per sample:
const float spatialAttenuation = spatialProcessor->getAttenuationGain(i);
delayedSample *= spatialAttenuation;
```

### 3D Panning (Phase 2)
```cpp
// dsp/DspModules.cpp:670-684
const float mono = 0.5f * (left + right);
const float leftGain = leftGainSmoother.getNextValue();
const float rightGain = rightGainSmoother.getNextValue();
left = mono * leftGain * gainLocal;
right = mono * rightGain * gainLocal;
```

**Math:** Azimuth -90°→+90°, L=cos(θ/2), R=sin(θ/2), L²+R²=1
**Elevation:** gain *= cos(elevation)

---

## Git Status

**Commits:**
- `297b7d7` - Task 3 (9 Living presets)
- `c19b7d9` - Phase 1 spatial positioning

**Uncommitted:**
```
M dsp/DspModules.h
M dsp/DspModules.cpp
M dsp/ModulationMatrix.h
M plugin/PresetManager.cpp
M CMakeLists.txt
A tests/ConstantPowerPanningTest.cpp
```

---

## Remaining: 24 Tasks (Phases 3-6 + Integration)

- **Phase 3 (5 tasks):** Doppler effects
- **Phase 4 (8 tasks):** SequenceScheduler + preset integration
- **Phase 5 (3 tasks):** Timeline UI editor
- **Phase 6 (5 tasks):** Memory Echoes integration
- **Integration (3 tasks):** Full testing, CPU profiling, showcase presets

**Total Progress:** 18/42 tasks (43%)

---

## Commands

```bash
# Continue from Task 19
cmake --build build --target Monument

# Test in DAW (AU/VST3 in ~/Library/Audio/Plug-Ins/)
# Verify 3D panning works, then commit Phase 2

# Next: Implement Doppler (Phase 3, Tasks 20-23)
```

**Context:** Continue Three-System Plan implementation at Phase 3.
