# Monument Reverb - Three-System Plan Session Handoff

**Date:** 2026-01-04
**Branch:** `feature/three-systems`
**Progress:** 18 of 42 tasks (43% complete)
**Status:** Phase 2 COMPLETE ✅ - Ready to commit

---

## Session Summary

**Completed:** Phases 1 and 2 of Three-System Plan
- ✅ **Phase 1:** Spatial positioning with 1/r² distance attenuation
- ✅ **Phase 2:** 3D azimuth/elevation panning in Facade

**Build:** ✅ All plugins compile successfully

---

## Completed Work (Tasks 1-18)

### Phase 1: Spatial Basics ✅ (Committed: c19b7d9)
- Created [dsp/SpatialProcessor.h/cpp](dsp/SpatialProcessor.h) - 1/r² attenuation
- Modified [dsp/Chambers.h/cpp](dsp/Chambers.h) - FDN integration at line ~630
- Extended [dsp/ModulationMatrix.h](dsp/ModulationMatrix.h) - PositionX/Y/Z (lines 69-71)
- Updated [plugin/PresetManager.cpp](plugin/PresetManager.cpp) - Serialization (667-669, 711-713)
- Wired [plugin/PluginProcessor.cpp](plugin/PluginProcessor.cpp) - Spatial routing (567-582)

### Phase 2: 3D Panning ✅ (UNCOMMITTED)
- Modified [dsp/DspModules.h](dsp/DspModules.h) - Facade methods (182-200, 213-220)
- Implemented [dsp/DspModules.cpp](dsp/DspModules.cpp) - Constant power panning (623-627, 669-756)
- Extended [dsp/ModulationMatrix.h](dsp/ModulationMatrix.h) - Distance, VelocityX (73-74)
- Updated [plugin/PresetManager.cpp](plugin/PresetManager.cpp) - String conversion (671-672, 718-719)
- Created [tests/ConstantPowerPanningTest.cpp](tests/ConstantPowerPanningTest.cpp) - Unit test
- Updated [CMakeLists.txt](CMakeLists.txt) - Test target (284-313)

---

## NEXT: Commit Phase 2

```bash
# Verify build works
cmake --build build --target Monument

# Commit Phase 2
git add -A
git commit -m "feat: implement Phase 2 - 3D azimuth/elevation panning

Three-System Plan Phase 2 Complete:
- Facade 3D panning with constant power law (L²+R²=1)
- set3DPanning(bool) toggle between stereo width and 3D mode
- setSpatialPositions(azimuth, elevation) with smoothed transitions
- Elevation scaling simulates height/distance
- Extended ModMatrix: Distance, VelocityX destinations (Doppler prep)
- Unit test for panning verification

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Sonnet 4.5 <noreply@anthropic.com>"
```

---

## Phase 3: Doppler Effects (Next - Tasks 19-23)

**Task 19:** Manual plugin testing (verify 3D panning works)
**Task 20:** Implement getDopplerShift() integration in Chambers
**Task 21:** Modify [dsp/Chambers.cpp:~630](dsp/Chambers.cpp#L630) fractional delay
**Task 22:** Write Doppler stability unit test
**Task 23:** Build and verify Phase 3

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
