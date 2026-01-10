# Phase 3: Foundation Module Test Plan

**Date:** 2026-01-09
**Status:** Planning Complete, Ready for Implementation
**Estimated Test Count:** 22 tests across 3 modules
**Estimated Implementation Time:** 4-5 hours

---

## Overview

Phase 3 tests the core foundation modules that power Monument Reverb's macro control system and diffusion network. These modules are critical for parameter mapping, musical expressiveness, and reverb quality.

**Modules Under Test:**

1. **AllpassDiffuser** (dsp/AllpassDiffuser.cpp) - 7 tests
2. **MacroMapper** (dsp/MacroMapper.cpp) - 8 tests
3. **ExpressiveMacroMapper** (dsp/ExpressiveMacroMapper.cpp) - 7 tests

---

## Module 1: AllpassDiffuser (7 Tests)

### Purpose
Classic allpass filter used for diffusion in reverb networks. Should provide flat magnitude response with frequency-dependent phase shift.

### Transfer Function
```
H(z) = (g + z^-D) / (1 + g*z^-D)
where:
  g = coefficient [-0.74, 0.74]
  D = delay in samples
```

### Test Cases

#### Test 1: Initialization
**Objective:** Verify prepare() allocates buffer correctly
- Call prepare() with various delay lengths (1, 10, 100, 1000 samples)
- Verify bufferLength = delaySamples + 1
- Verify buffer is allocated and zero-initialized
- **Pass Criteria:** Buffer size correct, no null pointers

#### Test 2: Unity Gain (Magnitude Response)
**Objective:** Allpass filter should have unity gain at all frequencies
- Generate white noise input (10000 samples)
- Process through allpass with coefficient=0.5, delay=10 samples
- Measure RMS of input vs output
- **Pass Criteria:** |RMS_output / RMS_input - 1.0| < 0.01 (1% tolerance)

#### Test 3: Coefficient Clamping
**Objective:** Coefficients outside [-0.74, 0.74] should be clamped for stability
- Test setCoefficient() with values: -1.0, -0.74, 0.0, 0.74, 1.0
- Verify internal coefficient is clamped to [-0.74, 0.74]
- **Pass Criteria:** Coefficient never exceeds safe range

#### Test 4: Phase Response Characteristics
**Objective:** Phase shift should vary with frequency
- Generate sine sweep from 20 Hz to 20 kHz
- Process through allpass with coefficient=0.7, delay=50 samples
- Measure phase shift at low (100 Hz) vs high (10 kHz) frequencies
- **Pass Criteria:** Phase shift at 10 kHz > phase shift at 100 Hz

#### Test 5: Delay Length Impact
**Objective:** Different delay lengths should affect frequency response
- Test with delays: 5, 20, 100 samples (coefficient=0.5)
- Process 1 kHz sine wave through each
- Measure phase shift for each delay
- **Pass Criteria:** Longer delays → more phase shift at 1 kHz

#### Test 6: Stability with Extreme Inputs
**Objective:** No runaway or denormals with large inputs
- Feed impulse of amplitude 10.0 (extreme input)
- Process 5000 samples
- Check for denormals, NaN, Inf
- Measure peak output over 5 seconds
- **Pass Criteria:** Output bounded, no denormals, peak < 100.0

#### Test 7: Reset Behavior
**Objective:** reset() should clear all state
- Process impulse, measure output after 100 samples (should be non-zero)
- Call reset()
- Process silence, verify output is zero
- **Pass Criteria:** All samples after reset are zero (< 1e-9)

### Expected CPU Usage
- **Budget:** < 0.5% (simple allpass, very efficient)
- **Measurement:** Process 48000 samples, time execution

---

## Module 2: MacroMapper (8 Tests)

### Purpose
Maps 10 "Ancient Monuments" themed macro controls to DSP parameters. Pure computational mapping with weighted blending of multiple influences.

### Macro Inputs (All [0, 1])
- **Core 6:** stone, labyrinth, mist, bloom, tempest, echo
- **Expanded 4:** patina, abyss, corona, breath

### Parameter Outputs (All [0, 1])
- Primary: time, mass, density, bloom, air, width, mix
- Advanced: warp, drift, gravity, pillarShape
- Physical modeling: 12 additional parameters

### Test Cases

#### Test 8: Initialization and Default Values
**Objective:** Default macro values produce sane parameter outputs
- Create MacroInputs with all defaults (0.5, 0.0, etc.)
- Compute targets
- Verify all output parameters in [0, 1] range
- **Pass Criteria:** No NaN, all values reasonable (0.3-0.7 for most)

#### Test 9: Input Clamping
**Objective:** Out-of-range inputs are clamped to [0, 1]
- Test computeTargets() with extreme inputs: -10.0, 5.0
- Verify internal clamping prevents out-of-range outputs
- **Pass Criteria:** All outputs remain in [0, 1] despite bad inputs

#### Test 10: Boundary Conditions
**Objective:** Test all macros at extremes (0.0 and 1.0)
- Set all macros to 0.0, compute targets
- Set all macros to 1.0, compute targets
- Verify outputs are meaningful (no divide-by-zero, etc.)
- **Pass Criteria:** All outputs in [0, 1], no crashes or NaN

#### Test 11: Single Macro Influence - STONE
**Objective:** STONE macro affects time, mass, density as expected
- Set stone=0.0 (soft limestone), all others neutral (0.5)
- Compute targets, record values
- Set stone=1.0 (hard granite), compute again
- **Pass Criteria:**
  - time increases (harder = longer tails)
  - mass increases (harder = more damping)
  - density increases (harder = more reflections)

#### Test 12: Single Macro Influence - LABYRINTH
**Objective:** LABYRINTH macro drives warp and drift
- Set labyrinth=0.0 (simple hall), all others neutral
- Record warp, drift values
- Set labyrinth=1.0 (twisted maze)
- **Pass Criteria:**
  - warp increases significantly (0.0 → >0.5)
  - drift increases (spatial complexity)

#### Test 13: Single Macro Influence - ABYSS
**Objective:** ABYSS macro controls spatial depth (size, time, width)
- Set abyss=0.0 (shallow), all others neutral
- Record size, time, width
- Set abyss=1.0 (infinite void)
- **Pass Criteria:**
  - time increases (deeper space = longer tails)
  - width increases (infinite = wider stereo)

#### Test 14: Multiple Macro Blending
**Objective:** Combined influences are weighted correctly
- Set stone=1.0, mist=1.0 (both affect time)
- Compute targets
- Verify time is influenced by both (weighted average)
- **Pass Criteria:** time value reflects both influences (not just one)

#### Test 15: Deterministic and Thread-Safe
**Objective:** Same inputs always produce same outputs (pure function)
- Compute targets with fixed inputs 1000 times
- Verify all outputs are identical (bit-exact)
- **Pass Criteria:** Zero variance in outputs across calls

### Expected CPU Usage
- **Budget:** < 0.1% (pure computation, no DSP processing)
- **Measurement:** Call computeTargets() 10000 times, measure total time

---

## Module 3: ExpressiveMacroMapper (7 Tests)

### Purpose
Alternative macro system with 6 performance-oriented controls. Maps high-level musical intent to DSP parameters and routing presets.

### Macro Inputs (All [0, 1])
- character, spaceType, energy, motion, color, dimension

### Key Differences from MacroMapper
- Fewer macros (6 vs 10) for simpler performance control
- spaceType selects discrete routing presets (not just parameter blending)
- More musically immediate (less conceptual)

### Test Cases

#### Test 16: Initialization and Default Values
**Objective:** Default expressive macros produce sane outputs
- Create MacroInputs with defaults
- Compute targets
- Verify routing preset is valid
- **Pass Criteria:** All parameters in range, valid routing preset

#### Test 17: Character Scaling
**Objective:** Character macro scales intensity globally
- Set character=0.0 (subtle), all others neutral
- Record parameter values
- Set character=1.0 (extreme)
- **Pass Criteria:** Feedback, density, and drive parameters increase with character

#### Test 18: Space Type Selection
**Objective:** spaceType selects discrete routing presets
- Test breakpoints: 0.1 (Chamber), 0.3 (Hall), 0.5 (Shimmer), 0.7 (Granular), 0.9 (Metallic)
- Verify correct RoutingPresetType for each
- **Pass Criteria:**
  - 0.0-0.2 → Chamber
  - 0.2-0.4 → Hall
  - 0.4-0.6 → Shimmer
  - 0.6-0.8 → Granular
  - 0.8-1.0 → Metallic

#### Test 19: Energy Mapping
**Objective:** Energy controls decay behavior (decay/sustain/grow/chaos)
- Test energy=0.1 (decay), 0.4 (sustain), 0.7 (grow), 0.95 (chaos)
- Check feedback, bloom, paradoxGain parameters
- **Pass Criteria:**
  - Decay mode: low feedback, low bloom
  - Sustain mode: medium-high feedback
  - Grow mode: high bloom, increasing feedback
  - Chaos mode: high paradox gain

#### Test 20: Motion Mapping
**Objective:** Motion controls temporal evolution (still/drift/pulse/random)
- Test motion=0.1 (still), 0.4 (drift), 0.7 (pulse), 0.95 (random)
- Check drift, warp, modulation depth
- **Pass Criteria:**
  - Still: drift ≈ 0, warp ≈ 0
  - Drift: drift > 0.3
  - Pulse: modulation depth > 0.5
  - Random: warp > 0.6

#### Test 21: Color Mapping
**Objective:** Color controls spectral character (dark/balanced/bright/spectral)
- Test color=0.1 (dark), 0.5 (balanced), 0.8 (bright), 0.95 (spectral)
- Check mass, air, gravity, metallicResonance
- **Pass Criteria:**
  - Dark: high mass (>0.6), low air (<0.4)
  - Balanced: mass ≈ 0.5, air ≈ 0.5
  - Bright: low mass (<0.4), high air (>0.6)
  - Spectral: high metallic resonance (>0.7)

#### Test 22: Dimension Mapping
**Objective:** Dimension controls space size (intimate/room/cathedral/infinite)
- Test dimension=0.1 (intimate), 0.4 (room), 0.7 (cathedral), 0.95 (infinite)
- Check time, density, width, impossibilityDegree
- **Pass Criteria:**
  - Intimate: low time (<0.3), low width (<0.3)
  - Room: medium time (0.4-0.6)
  - Cathedral: high time (>0.7), high width
  - Infinite: impossibility degree > 0.5

---

## Implementation Strategy

### Order of Implementation

1. **Start with AllpassDiffuser** (simplest, establishes DSP test pattern)
   - Tests 1-7
   - ~1.5 hours

2. **MacroMapper next** (pure computation, no DSP)
   - Tests 8-15
   - ~1.5 hours

3. **ExpressiveMacroMapper last** (similar to MacroMapper)
   - Tests 16-22
   - ~1.5 hours

4. **CMake + CI integration**
   - ~30 minutes

### Test File Structure

```cpp
// tests/FoundationTest.cpp
#include <JuceHeader.h>
#include "../dsp/AllpassDiffuser.h"
#include "../dsp/MacroMapper.h"
#include "../dsp/ExpressiveMacroMapper.h"

// Follow existing test pattern from NovelAlgorithmsTest.cpp
// - Color output (green/red/yellow)
// - Helper functions: assertTrue, assertAlmostEqual
// - Test summary at end
// - Return 0 on success, 1 on failure

int main()
{
    // Test AllpassDiffuser (7 tests)
    // Test MacroMapper (8 tests)
    // Test ExpressiveMacroMapper (7 tests)
    // Print summary
}
```

### CMake Target

```cmake
juce_add_console_app(monument_foundation_test
    PRODUCT_NAME "Monument Foundation Test")

target_sources(monument_foundation_test PRIVATE
    tests/FoundationTest.cpp
    dsp/AllpassDiffuser.cpp
    dsp/MacroMapper.cpp
    dsp/ExpressiveMacroMapper.cpp)

target_link_libraries(monument_foundation_test PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_dsp)

add_test(NAME FoundationTest COMMAND monument_foundation_test)
```

---

## Success Criteria

**Phase 3 Complete When:**
- ✅ All 22 tests passing (100%)
- ✅ CMake target builds successfully
- ✅ Integrated into run_ci_tests.sh
- ✅ CPU usage within budget (AllpassDiffuser < 0.5%, mappers < 0.1%)
- ✅ No memory leaks or denormals detected
- ✅ Test output matches existing pattern (colors, summary)

---

## Next Steps After Phase 3

**Phase 4 Options:**
1. Additional reverb modules (if any remain untested)
2. Advanced integration tests (full preset chains)
3. Performance benchmarking suite
4. Memory leak detection (Valgrind/AddressSanitizer)

---

**Ready to implement! 🚀**
