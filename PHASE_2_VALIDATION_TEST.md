# Phase 2 Validation Test Guide
## Macro System Integration Testing

**Purpose:** Verify that the macro control system (Phase 2) is working correctly before implementing Phase 3 modulation sources.

**Date:** 2026-01-03
**Status:** Ready for Testing

---

## Prerequisites

- Phase 2 complete (MacroMapper + ModulationMatrix integrated)
- Build successful (AU/VST3 plugins)
- DAW available (Logic Pro, Ableton, Reaper, etc.)

---

## Step 1: Build the Plugin

### Build Commands

```bash
cd /Users/noisebox/Documents/3_Development/Repos/monument-reverb
./scripts/build_macos.sh
```

**Expected Output:**
```
-- Build files have been written to: build
[100%] Built target Monument_AU
[100%] Built target Monument_VST3
Build succeeded
```

**Troubleshooting:**
- If build fails, check [CMakeLists.txt](CMakeLists.txt:72-75) for MacroMapper.cpp and ModulationMatrix.cpp entries
- Verify C++17 standard is enabled
- Check for missing JUCE modules

---

## Step 2: Load Plugin in DAW

### Installation Paths

**AU Plugin:**
```
~/Library/Audio/Plug-Ins/Components/Monument.component
```

**VST3 Plugin:**
```
~/Library/Audio/Plug-Ins/VST3/Monument.vst3
```

### DAW Setup

1. Launch your DAW
2. Create a new audio track
3. Load Monument on the track
4. Create a test signal:
   - **Option A:** Record/import an impulse (clap, snare hit)
   - **Option B:** Use a synth plugin for sustained tone (250 Hz sine wave)

---

## Step 3: Macro Parameter Tests

### Test 1: Material Macro (Soft → Hard)

**What to Test:**
- Material parameter sweep: 0.0 (soft) → 1.0 (hard)

**Expected Behavior:**

| Material Value | Time (tail length) | Mass (darkness) | Density (complexity) |
|----------------|-------------------|-----------------|----------------------|
| 0.0 (soft)     | Short (0.3-0.4)   | Bright (0.2)    | Sparse (0.25)        |
| 0.5 (medium)   | Medium (0.55)     | Balanced (0.55) | Medium (0.60)        |
| 1.0 (hard)     | Long (0.77+)      | Dark (0.83+)    | Dense (0.91+)        |

**How to Test:**
1. Set Material = 0.0
2. Play impulse, listen to reverb tail
3. **Listen for:** Short, bright, sparse reflections
4. Slowly move Material slider to 1.0
5. **Listen for:** Tail lengthens, gets darker, more complex

**Success Criteria:**
- ✅ Reverb tail increases in length smoothly
- ✅ Sound gets progressively darker/warmer
- ✅ Reflections become more dense/complex
- ✅ No zipper noise or clicks during sweep

**Troubleshooting:**
- If no change: Check [plugin/PluginProcessor.cpp:197-310] - verify MacroMapper.computeTargets() is called
- If parameters jump: Check blending influence calculation (lines 225-253)

---

### Test 2: Viscosity Macro (Airy → Thick)

**What to Test:**
- Viscosity parameter sweep: 0.0 (airy) → 1.0 (thick)

**Expected Behavior:**

| Viscosity Value | Air (brightness) | Time (sustain) | Mass (damping) |
|-----------------|------------------|----------------|----------------|
| 0.0 (airy)      | Bright (0.8)     | Long (0.6)     | Light (0.0)    |
| 0.5 (medium)    | Balanced (0.5)   | Medium (0.5)   | Medium (0.15)  |
| 1.0 (thick)     | Dark (0.2)       | Short (0.4)    | Heavy (0.3)    |

**How to Test:**
1. Set Viscosity = 0.0
2. Play sustained tone (250 Hz sine)
3. **Listen for:** Open, airy, bright reverb
4. Move Viscosity to 1.0
5. **Listen for:** Muffled, dark, shorter tail

**Success Criteria:**
- ✅ High frequencies roll off progressively
- ✅ Reverb tail shortens (inverse relationship)
- ✅ Sound becomes "thicker" and more damped
- ✅ Smooth transition throughout sweep

---

### Test 3: Topology Macro (Regular → Non-Euclidean)

**What to Test:**
- Topology parameter sweep: 0.0 (regular) → 1.0 (non-Euclidean)

**Expected Behavior:**

| Topology Value | Warp (matrix) | Drift (motion) | Spatial Character |
|----------------|---------------|----------------|-------------------|
| 0.0 (regular)  | 0.0 (Hadamard) | Low (0.0)     | Stable, predictable |
| 0.5 (hybrid)   | 0.5 (blend)    | Medium (0.2)  | Subtle motion |
| 1.0 (chaotic)  | 1.0 (Householder) | High (0.4) | Complex, morphing |

**How to Test:**
1. Set Topology = 0.0
2. Play impulse
3. **Listen for:** Stable, predictable spatial character
4. Move Topology to 1.0
5. **Listen for:** Spatial geometry morphs, more complex reflections

**Success Criteria:**
- ✅ Spatial character changes from stable to complex
- ✅ FDN matrix morphs smoothly (no clicks)
- ✅ Drift increases subtly

---

### Test 4: Evolution Macro (Static → Evolving)

**What to Test:**
- Evolution parameter sweep: 0.0 (static) → 1.0 (evolving)

**Expected Behavior:**

| Evolution Value | Bloom (envelope) | Drift (motion) | Temporal Behavior |
|-----------------|------------------|----------------|-------------------|
| 0.0 (static)    | 0.0 (no bloom)   | Low (0.0)      | Frozen, unchanging |
| 0.5 (moderate)  | 0.5 (subtle)     | Medium (0.175) | Gentle evolution |
| 1.0 (evolving)  | 1.0 (strong)     | High (0.35)    | Dynamic, swelling |

**How to Test:**
1. Set Evolution = 0.0
2. Play sustained tone
3. **Listen for:** Static, unchanging reverb
4. Move Evolution to 1.0
5. **Listen for:** Reverb tail swells and evolves over time

**Success Criteria:**
- ✅ Bloom creates envelope swell in late reflections
- ✅ Drift increases (subtle micro-motion)
- ✅ Reverb feels "alive" and dynamic

---

### Test 5: Chaos Intensity (Stable → Chaotic)

**What to Test:**
- Chaos parameter sweep: 0.0 (stable) → 1.0 (chaotic)

**Expected Behavior:**

| Chaos Value | Warp (additive) | Drift (additive) | Behavior |
|-------------|-----------------|------------------|----------|
| 0.0 (stable) | +0.0           | +0.0             | Predictable |
| 0.5 (moderate) | +0.15        | +0.25            | Moderate motion |
| 1.0 (chaotic) | +0.3          | +0.5             | Erratic, unpredictable |

**How to Test:**
1. Set Chaos = 0.0
2. Play impulse
3. **Listen for:** Stable, repeatable behavior
4. Move Chaos to 1.0
5. **Listen for:** Erratic topology shifts, stronger motion

**Success Criteria:**
- ✅ Chaos adds unpredictability to spatial character
- ✅ Combines with Topology macro (additive influence)
- ✅ Drift increases significantly

---

### Test 6: Elasticity Macro (Reserved)

**Status:** Reserved for Phase 3 (physical modeling)

**Note:** This parameter is currently inactive (no DSP implementation yet). Moving it should have no audible effect in Phase 2.

---

## Step 4: Macro Interaction Tests

### Test 7: Combined Macros (Material + Viscosity)

**Scenario:** "Hard stone space in thick medium"

**Settings:**
- Material = 0.9 (very hard)
- Viscosity = 0.8 (very thick)

**Expected Behavior:**
- Long tail (Material influence)
- Dark/muffled highs (Viscosity influence)
- Dense reflections (Material influence)
- Shorter effective tail due to thickness

**How Material and Viscosity Interact:**
- Both influence `time`, but Material increases it (0.77) while Viscosity decreases it (0.44)
- Weighted combining: Material gets 60% weight, Viscosity gets 40%
- Final `time` ≈ 0.638 (see [docs/DSP_ARCHITECTURE.md:246-263])

**Success Criteria:**
- ✅ Parameters blend musically (not conflicting)
- ✅ Weighted influence creates coherent sound
- ✅ Influence factor saturates (approaches 1.0)

---

### Test 8: Influence Blending Behavior

**Scenario:** Verify distance-based blending algorithm

**Test Cases:**

| Macro Settings | Total Δ | Influence | Base Param Weight | Macro Weight |
|----------------|---------|-----------|-------------------|--------------|
| All at defaults (0.5) | 0.0 | 0.0 | 100% | 0% |
| Material = 0.8 | 0.3 | 0.6 | 40% | 60% |
| Multiple moved | 0.7+ | 1.0 (saturated) | 0% | 100% |

**How to Test:**
1. **Case A:** Set all macros to defaults (Material=0.5, Topology=0.5, etc.)
   - **Expected:** User has full manual control (base parameters dominate)
2. **Case B:** Move Material to 0.8
   - **Expected:** 60% macro influence, base still matters
3. **Case C:** Move multiple macros away from defaults
   - **Expected:** Full macro control (influence saturates at 1.0)

**Success Criteria:**
- ✅ Default macros = base parameters have full control
- ✅ Single macro moved = blended control (40/60 split)
- ✅ Multiple macros = full macro control

---

## Step 5: Performance Validation

### Test 9: CPU Usage

**What to Measure:**
- DAW CPU meter with Monument loaded

**Expected Performance:**
- **Phase 2 (Macro System):** < 0.1% overhead
- **Baseline (Phase 1):** ~0.5% CPU per instance
- **Total (Phase 2):** ~0.6% CPU per instance

**How to Test:**
1. Open DAW CPU meter
2. Load Monument on empty track
3. Play audio through plugin
4. Note CPU usage
5. Add 5 instances, verify linear scaling

**Success Criteria:**
- ✅ CPU usage < 1% per instance
- ✅ No audio dropouts or glitches
- ✅ Scales linearly with instance count

---

### Test 10: Real-Time Safety

**What to Test:**
- No allocations in audio thread
- No locks or mutex contention
- No file I/O or logging

**How to Test:**
1. Build with address sanitizer (if available):
   ```bash
   cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON ..
   make
   ```
2. Run plugin in DAW
3. Check for warnings/errors in console

**Success Criteria:**
- ✅ No allocation warnings
- ✅ No lock contention
- ✅ No file I/O during processing

---

## Step 6: Edge Case Testing

### Test 11: Parameter Extremes

**What to Test:**
- All macros at minimum (0.0)
- All macros at maximum (1.0)
- Rapid parameter changes

**Expected Behavior:**
- No NaN or Infinity values
- No clipping or distortion
- Smooth transitions even at extremes

**How to Test:**
1. Set all macros to 0.0
2. Play audio, verify stable output
3. Set all macros to 1.0
4. Play audio, verify stable output
5. Rapidly sweep macros (automation)
6. Verify no clicks or artifacts

**Success Criteria:**
- ✅ Stable output at all parameter extremes
- ✅ No NaN/Inf values
- ✅ No clicks during rapid changes

---

## Test Results Template

### Copy this template and fill it out:

```
# Phase 2 Validation Test Results
Date: ________
Tester: ________
DAW: ________
Plugin Format: AU / VST3
OS Version: ________

## Test Results:

[ ] Test 1: Material Macro (Pass/Fail)
    Notes: _______________________________________

[ ] Test 2: Viscosity Macro (Pass/Fail)
    Notes: _______________________________________

[ ] Test 3: Topology Macro (Pass/Fail)
    Notes: _______________________________________

[ ] Test 4: Evolution Macro (Pass/Fail)
    Notes: _______________________________________

[ ] Test 5: Chaos Intensity (Pass/Fail)
    Notes: _______________________________________

[ ] Test 7: Combined Macros (Pass/Fail)
    Notes: _______________________________________

[ ] Test 8: Influence Blending (Pass/Fail)
    Notes: _______________________________________

[ ] Test 9: CPU Usage (Pass/Fail)
    CPU: ___% per instance
    Notes: _______________________________________

[ ] Test 10: Real-Time Safety (Pass/Fail)
    Notes: _______________________________________

[ ] Test 11: Edge Cases (Pass/Fail)
    Notes: _______________________________________

## Overall Status:
[ ] Pass - Ready for Phase 3
[ ] Fail - Issues found (see notes)

## Issues Found:
1. _______________________________________
2. _______________________________________
3. _______________________________________
```

---

## Next Steps

### If All Tests Pass ✅

Proceed to **Phase 3: Implement Modulation Sources**

1. Start with [BrownianMotion](dsp/BrownianMotion.h) (simplest)
2. Follow TDD approach (write tests first)
3. Reference [ARCHITECTURE_REVIEW.md:469-490] for guidance

### If Tests Fail ❌

**Common Issues:**

1. **No parameter changes:**
   - Check [plugin/PluginProcessor.cpp:197-310]
   - Verify MacroMapper.computeTargets() is called per block
   - Verify APVTS parameters are polled correctly

2. **Zipper noise:**
   - Check ParameterSmoother integration
   - Verify smoothing time constants (20-50ms recommended)

3. **Parameters jump:**
   - Check influence blending calculation
   - Verify totalΔ computation (lines 225-253 in DSP_ARCHITECTURE.md)

4. **CPU spikes:**
   - Profile processBlock() with Instruments (Xcode)
   - Check for accidental allocations in audio thread

5. **No sound:**
   - Verify DSP chain is intact (Foundation → ... → Facade)
   - Check wet/dry mix isn't set to 0.0

---

## Reference Documents

- [ARCHITECTURE_REVIEW.md](ARCHITECTURE_REVIEW.md) - Phase 1-4 roadmap
- [docs/DSP_ARCHITECTURE.md](docs/DSP_ARCHITECTURE.md) - Macro system details
- [CHANGELOG.md](CHANGELOG.md) - Phase 2 implementation notes
- [plugin/PluginProcessor.cpp:197-310](plugin/PluginProcessor.cpp) - Integration code

---

**Good luck with testing!** 🎛️

Once validation passes, we'll move to Phase 3 and implement the modulation sources.
