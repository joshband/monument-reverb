# Monument Reverb - Parameter Stress Test Results

**Date:** 2026-01-09
**Test Suite:** Phase 2 - Parameter Stress Tests (Quick Mode)
**Purpose:** Validate parameter behavior under extreme conditions
**Build:** monument_parameter_stress_test

---

## Executive Summary

**Status:** ⚠️ Partial Success (3/6 tests passing)
**Critical Issues:** 3 failures related to parameter smoothing and validation
**Performance:** Plugin remains stable under extreme parameter values
**Next Steps:** Improve parameter smoothing and validation

---

## Test Results Overview

| Test | Status | Result | Notes |
|------|--------|--------|-------|
| **PARAM-1** | ✅ PASS | All parameters at zero | No crashes, stable output |
| **PARAM-2** | ✅ PASS | All parameters at maximum | Peak = +6.2 dB, bounded |
| **PARAM-3** | ⏸️ SKIP | Random parameters (10s) | Not run in quick mode |
| **PARAM-4** | ❌ FAIL | Rapid parameter sweeps | 13 dB zipper noise (threshold: -40dB) |
| **PARAM-5** | ❌ FAIL | Parameter jump stress | 6.8 dB clicks (threshold: -30dB) |
| **PARAM-6** | ⏸️ SKIP | Automation storm (47 params) | Not run in quick mode |
| **PARAM-7** | ⏸️ SKIP | Feedback at maximum | Not run in quick mode |
| **PARAM-8** | ⏸️ SKIP | Resonance at maximum | Not run in quick mode |
| **PARAM-9** | ⏸️ SKIP | Freeze + feedback 100% | Not run in quick mode |
| **PARAM-10** | ⏸️ SKIP | RT60 at minimum | Not run in quick mode |
| **PARAM-11** | ⏸️ SKIP | RT60 at maximum | Not run in quick mode |
| **PARAM-12** | ✅ PASS | Diffusion extremes | 0% and 100% both stable |
| **PARAM-13** | ⏸️ SKIP | Modulation at maximum | Not run in quick mode |
| **PARAM-14** | ❌ FAIL | Invalid parameter values | Clamping validation failed |
| **PARAM-15** | ⏸️ SKIP | Preset switching rapid | Not run in quick mode |

---

## Detailed Findings

### ✅ PARAM-1: All Parameters Zero

**Result:** PASS
**Description:** Set all 47 parameters to minimum (0.0) and process 100 blocks

```
Status: No crashes, output stable
Inf/NaN: None detected
Peak Level: N/A (silence expected)
```

**Conclusion:** Plugin handles all-zero parameter state correctly. No instability.

---

### ✅ PARAM-2: All Parameters Maximum

**Result:** PASS
**Description:** Set all 47 parameters to maximum (1.0) and process 100 blocks

```
Status: Stable
Peak Level: +6.2 dB
Runaway: No (threshold: +40 dB)
Inf/NaN: None detected
```

**Conclusion:** Plugin remains bounded at maximum parameter values. Peak output is reasonable (~6dB) with no runaway amplification.

---

### ❌ PARAM-4: Rapid Parameter Sweeps (Zipper Noise)

**Result:** FAIL
**Description:** Sweep a parameter at 10 Hz (sine wave) to test parameter smoothing

```
Status: FAILED
Zipper Noise: 13.0 dB
Threshold: -40 dB (failed)
Measured Sample-to-Sample Jump: 13 dB peak
```

**Issue:** Parameter smoothing is insufficient for rapid automation.
**Expected:** Zipper noise < -40dB
**Actual:** 13 dB (53 dB above threshold)

**Root Cause Analysis:**
- Parameter changes may not be using juce::SmoothedValue or equivalent
- Smoothing time constant may be too short (< 20ms recommended)
- Block-rate parameter updates instead of sample-rate interpolation

**Recommendation:**
```cpp
// In prepareToPlay()
parameterSmoothed.reset(sampleRate, 0.02);  // 20ms smoothing

// In processBlock()
for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
{
    float smoothedValue = parameterSmoothed.getNextValue();
    // Use smoothedValue for processing
}
```

---

### ❌ PARAM-5: Parameter Jump Stress (Instant Changes)

**Result:** FAIL
**Description:** Jump parameter between 0.0 and 1.0 every block (instant changes)

```
Status: FAILED
Click Level: 6.8 dB
Threshold: -30 dB (failed)
Measured Sample-to-Sample Jump: 6.8 dB peak
```

**Issue:** Instant parameter changes produce audible clicks.
**Expected:** Clicks < -30dB with proper smoothing
**Actual:** 6.8 dB (36.8 dB above threshold)

**Root Cause Analysis:**
- Same root cause as PARAM-4
- Even more severe as parameter jumps are instant (not gradual)
- Smoothing should prevent this

**Recommendation:** Same as PARAM-4 - implement proper parameter smoothing with juce::SmoothedValue.

---

### ✅ PARAM-12: Diffusion Extremes

**Result:** PASS
**Description:** Test 0% and 100% diffusion (density parameter)

```
Status: Stable at both extremes
0% Diffusion: Stable, no Inf/NaN
100% Diffusion: Stable, no Inf/NaN
```

**Conclusion:** Diffusion algorithm handles both extremes correctly.

---

### ❌ PARAM-14: Invalid Parameter Values (Out of Range)

**Result:** FAIL
**Description:** Attempt to set parameters outside [0, 1] range

```
Status: FAILED
Issue: Parameter clamping validation failed
Values Tested: -1.0, 2.0
Expected: All values clamped to [0, 1]
Actual: Some parameters accepted out-of-range values
```

**Issue:** JUCE AudioParameter validation may not be enforcing range constraints properly.

**Root Cause Analysis:**
- `setValueNotifyingHost()` accepts normalized [0, 1] values
- The test may be incorrectly assuming validation
- OR: Custom parameter classes not implementing range validation

**Recommendation:**
- Verify all AudioParameter subclasses properly clamp input values
- Add explicit validation in parameter change callbacks
- Consider using NormalisableRange with proper clamping

---

## JUCE Assertions (Non-Critical)

During testing, many JUCE assertions were triggered:

```
JUCE Assertion failure in juce_IIRFilter_Impl.h:106
JUCE Assertion failure in juce_IIRFilter_Impl.h:107
```

**Analysis:**
- IIR filter coefficients being used before proper initialization
- Likely in ElasticHallway or Chambers modules (both use IIR filters)
- Assertions are non-fatal in Release builds but indicate improper initialization order

**Recommendation:**
- Ensure `prepareToPlay()` is called before any processing
- Initialize all IIR filter coefficients in `prepare()` methods
- Add defensive checks: `if (!isPrepared) return;`

---

## Performance Observations

Despite failures, the plugin demonstrated:

1. **Stability:** No crashes or hangs under any extreme parameter configuration
2. **Bounded Output:** Peak levels remain reasonable even at max settings (+6.2 dB)
3. **No Runaway:** No feedback loops or amplification spirals detected
4. **Numerical Safety:** No Inf/NaN values generated

---

## Priority Issues to Fix

### Priority 1: Parameter Smoothing (PARAM-4, PARAM-5)

**Impact:** HIGH - Affects user experience during automation
**Difficulty:** MEDIUM - Standard JUCE pattern

**Files to Check:**
- `plugin/PluginProcessor.cpp`: Parameter update logic
- All DSP modules: Parameter application

**Fix:**
```cpp
// Add to each DSP module class
juce::SmoothedValue<float> parameterSmoothed;

// In prepare()
parameterSmoothed.reset(sampleRate, 0.02);  // 20ms

// In process()
for (int i = 0; i < numSamples; ++i)
{
    float value = parameterSmoothed.getNextValue();
    // Use smoothed value
}
```

---

### Priority 2: Parameter Validation (PARAM-14)

**Impact:** MEDIUM - Edge case but indicates validation gap
**Difficulty:** LOW - Add validation layer

**Files to Check:**
- `plugin/PluginProcessor.cpp`: createParameterLayout()

**Fix:**
```cpp
// Ensure all parameters use proper range validation
params.push_back(std::make_unique<juce::AudioParameterFloat>(
    "paramId",
    "Param Name",
    juce::NormalisableRange<float>(0.0f, 1.0f),  // Enforces [0, 1]
    0.5f));
```

---

### Priority 3: IIR Filter Initialization (Assertions)

**Impact:** LOW - Cosmetic in Release builds
**Difficulty:** LOW - Initialization order

**Files to Check:**
- `dsp/ElasticHallway.cpp`
- `dsp/Chambers.cpp`

**Fix:**
```cpp
void prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    // Initialize ALL IIR coefficients before use
    filter.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(
        sampleRate, 1000.0, 0.707);

    filter.prepare({sampleRate, (juce::uint32)maxBlockSize, (juce::uint32)numChannels});
}
```

---

## Next Steps

### Immediate Actions

1. ✅ **Phase 2 Complete:** Quick parameter tests executed
2. ⏭️ **Fix Parameter Smoothing:** Implement juce::SmoothedValue for all parameters
3. ⏭️ **Re-run Tests:** Verify fixes with full parameter stress suite (non-quick mode)
4. ⏭️ **Phase 3:** Run extended parameter tests (PARAM-3, PARAM-6, etc.)

### Extended Testing

Once fixes are applied:

1. Run **full parameter stress suite** (not --quick mode)
2. Test PARAM-3, PARAM-6, PARAM-7, PARAM-8, PARAM-9, PARAM-10, PARAM-11, PARAM-13, PARAM-15
3. Measure improvement in zipper noise and clicks
4. Document updated baselines

---

## Command Reference

```bash
# Build parameter stress test
cmake --build build --target monument_parameter_stress_test -j8

# Run quick tests (6 tests, ~30 seconds)
./build/monument_parameter_stress_test_artefacts/Debug/monument_parameter_stress_test --quick

# Run full suite (15 tests, ~10 minutes)
./build/monument_parameter_stress_test_artefacts/Debug/monument_parameter_stress_test
```

---

## Appendix: Test Methodology

### Test Environment
- **Sample Rate:** 48,000 Hz
- **Block Size:** 512 samples
- **Channels:** 2 (stereo)
- **Test Duration:** 10 seconds (per test)
- **Total Parameters:** 47

### Measurement Techniques

1. **Zipper Noise Detection:**
   - Calculate max sample-to-sample jump
   - Convert to dB: 20 * log10(maxJump)
   - Threshold: -40 dB

2. **Click Detection:**
   - Same as zipper noise for instant changes
   - Threshold: -30 dB (less strict)

3. **Stability Testing:**
   - Check for Inf/NaN values every block
   - Monitor peak levels over time
   - Detect runaway amplification (> +40 dB)

4. **Parameter Validation:**
   - Set values outside [0, 1]
   - Verify clamping to valid range

---

**Status:** Phase 2 Partially Complete ⚠️
**Next Phase:** Fix parameter smoothing → Re-run full suite → Phase 3 (Long Duration Tests)
