# Monument Reverb - Comprehensive Automated Test Plan

**Date:** 2026-01-05
**Status:** Phase 4 Complete - Expanding Test Coverage
**Goal:** 100% automated DSP quality assurance with zero manual DAW testing

---

## Test Infrastructure Overview

**Existing Tools:**
- ✅ `monument_plugin_analyzer` - VST3/AU loading, audio capture, preset testing
- ✅ `rt60_analysis_robust.py` - Decay time measurement with pyroomacoustics
- ✅ `frequency_response.py` - FFT analysis with octave bands
- ✅ `plot_preset_comparison.py` - Visualization generation
- ✅ `compare_baseline.py` - Regression detection framework
- ✅ Batch scripts: `capture_all_presets.sh`, `analyze_all_presets.sh`

**New Tests to Implement:**
10 comprehensive automated test suites covering all DSP aspects

---

## 1. RT60 Accuracy Test (High Priority)

**File:** `tests/RT60AccuracyTest.cpp`
**Purpose:** Verify RT60 measurements match `time` parameter settings

### Test Cases

```cpp
1. testRT60Scaling()
   - Set time=0.0, measure RT60, verify < 2 seconds
   - Set time=0.5, measure RT60, verify ~10 seconds
   - Set time=1.0, measure RT60, verify > 25 seconds
   - Tolerance: ±15% (accounts for nonlinear mapping)

2. testRT60ConsistencyAcrossPresets()
   - Load all 37 factory presets
   - Measure RT60 for each
   - Verify RT60 ∈ [4.85s, 29.85s] (from baseline data)
   - Flag outliers (>2σ from mean)

3. testRT60SampleRateIndependence()
   - Test at 44.1, 48, 88.2, 96 kHz
   - RT60 should be consistent (±5% tolerance)
   - Time is in seconds, not samples

4. testRT60StabilityUnderModulation()
   - Enable modulation matrix with Time as destination
   - Verify RT60 doesn't jump discontinuously
   - Max change per block: <10% of current RT60
```

**Integration:** Uses `monument_plugin_analyzer` + `rt60_analysis_robust.py`

---

## 2. Frequency Response Test (High Priority)

**File:** `tests/FrequencyResponseTest.cpp`
**Purpose:** Verify frequency-dependent behavior

### Test Cases

```cpp
1. testFrequencyFlatness()
   - Generate white noise input
   - Measure output spectrum (octave bands)
   - Verify flatness ±10dB (Fair rating or better)
   - Flag excessive coloration (>±15dB)

2. testLowFrequencyMassParameter()
   - Set mass=0.0 (light), measure <500Hz response
   - Set mass=1.0 (heavy), measure <500Hz response
   - Verify heavy setting has more low-freq energy

3. testHighFrequencyGravityParameter()
   - Set gravity=0.0 (no damping), measure >5kHz response
   - Set gravity=1.0 (max damping), measure >5kHz response
   - Verify high gravity attenuates highs by >6dB

4. testNoUnwantedResonances()
   - Sweep 20Hz-20kHz
   - Detect peaks >+10dB (potential resonances)
   - Fail if any resonances detected
```

**Integration:** Uses `monument_plugin_analyzer` + `frequency_response.py`

---

## 3. CPU Performance Benchmark (High Priority)

**File:** `tests/CPUPerformanceBenchmark.cpp`
**Purpose:** Prevent performance regressions

### Test Cases

```cpp
1. testWorstCasePresetCPU()
   - Load preset with highest CPU (likely Preset 14: longest RT60)
   - Measure processBlock() time for 1000 iterations
   - Max threshold: 2.0ms per block @ 512 samples, 48kHz
   - 2ms = 0.4% CPU (2ms / 512 samples * 48000 Hz = 9.6% DSP load)

2. testBaselinePresetCPU()
   - Load Preset 0 (Init Patch)
   - Benchmark as baseline reference
   - Store result for regression comparison

3. testCPURegressionDetection()
   - Compare current CPU vs stored baseline
   - Fail if increase >10% without justification
   - Example: v1.1 = 1.5ms, v1.2 = 1.65ms → FAIL

4. testIndividualModuleCPU()
   - Profile FDN, Physical Modeling, Memory Echoes separately
   - Identify bottlenecks
   - Report top 3 CPU-intensive modules
```

**Integration:** Uses C++ `<chrono>` for high-resolution timing

---

## 4. Audio Regression Test (Critical - Automated)

**File:** `tests/AudioRegressionTest.cpp`
**Purpose:** Detect unintended DSP changes

### Test Cases

```cpp
1. testImpulseResponseConsistency()
   - For all 37 presets:
     - Load preset
     - Generate impulse response (30s capture)
     - Compute L2 norm vs baseline
   - Tolerance: <0.1% difference (numerical drift only)

2. testBitExactReproduction()
   - Process same audio twice
   - Verify output is bit-exact (L2 norm = 0.0)
   - Tests determinism and thread safety

3. testCrossVersionCompatibility()
   - Load baseline from v1.0
   - Process with current version
   - Verify difference <1% (allow intentional improvements)
```

**Integration:** Uses `compare_baseline.py` + stored baselines

---

## 5. Modulation Depth Test (High Priority)

**File:** `tests/ModulationDepthTest.cpp`
**Purpose:** Verify modulation system safety

### Test Cases

```cpp
1. testModulationSourceBounds()
   - For each source (Chaos, Audio Follower, Brownian, Envelope):
     - Run for 10 seconds
     - Verify output ∈ [-1, +1] at all times
     - Flag any out-of-bounds values

2. testExtremeModulationDepth()
   - Set depth=±1.0 (maximum)
   - Target all parameters simultaneously
   - Process 100 blocks
   - Verify no NaN, no Inf, no instability

3. testModulationSmoothing()
   - Instant depth change: 0.0 → 1.0
   - Measure THD+N during transition
   - Verify THD+N < -60dB (no zipper noise)

4. testProbabilityGating()
   - Set probability=0.5 for a connection
   - Run 10,000 blocks
   - Verify active ~50% of time (±5% tolerance)
```

**Integration:** Standalone unit test

---

## 6. Parameter Smoothing Test (Medium Priority)

**File:** `tests/ParameterSmoothingTest.cpp`
**Purpose:** Verify click-free parameter changes

### Test Cases

```cpp
1. testInstantParameterJump()
   - For each smoothed parameter:
     - Instant jump 0.0 → 1.0
     - Measure output THD+N
     - Verify THD+N < -60dB

2. testSmoothingRampTime()
   - Set parameter from 0.0 → 1.0
   - Measure time to reach 0.99 (99% of target)
   - Verify ramp time = 50ms ± 5ms

3. testNoOvershoot()
   - Jump to target value
   - Verify output never exceeds target
   - No overshoot/ringing
```

**Integration:** Standalone unit test with audio analysis

---

## 7. Stereo Width Test (Medium Priority)

**File:** `tests/StereoWidthTest.cpp`
**Purpose:** Verify spatial processing

### Test Cases

```cpp
1. testMonoAtZeroWidth()
   - Set width=0.0
   - Measure L/R correlation
   - Verify correlation = 1.0 (perfect mono)

2. testWideAtMaxWidth()
   - Set width=1.0
   - Measure L/R correlation
   - Verify correlation < 0.5 (decorrelated)

3. testConstantPowerPanning()
   - Set width=0.5
   - Measure L+R RMS power
   - Verify constant power (±0.5dB) across width sweep

4. testNoPhaseCancellation()
   - Set width=0.5
   - Sum L+R to mono
   - Verify no >3dB loss (phase cancellation)
```

**Integration:** Uses `monument_plugin_analyzer` with stereo analysis

---

## 8. Latency & Phase Test (Medium Priority)

**File:** `tests/LatencyPhaseTest.cpp`
**Purpose:** Verify DAW compatibility

### Test Cases

```cpp
1. testReportedLatency()
   - Call getTailLengthSeconds()
   - Generate impulse, measure actual tail
   - Verify reported ≈ actual (±1 second tolerance)

2. testActualProcessingLatency()
   - Send impulse through plugin
   - Measure delay to first non-zero sample
   - Verify latency < 100 samples (acceptable for reverb)

3. testStereoPhaseCoherence()
   - Mono input → stereo output
   - Measure L/R phase difference
   - Verify phase coherent (no polarity inversions)

4. testNoPhaseInversion()
   - Send sine wave through plugin
   - Verify output phase matches input (±180° tolerance)
```

**Integration:** Uses `monument_plugin_analyzer` with impulse analysis

---

## 9. Preset Loading Test (Low Priority)

**File:** `tests/PresetLoadingTest.cpp`
**Purpose:** Verify preset system integrity

### Test Cases

```cpp
1. testAllPresetsLoadWithoutError()
   - Load all 37 factory presets sequentially
   - Verify no exceptions, no crashes
   - Verify all parameters ∈ [0, 1]

2. testNoNaNInfValues()
   - Load each preset
   - Read all parameter values
   - Verify no NaN, no Inf

3. testPresetTransitionSmooth()
   - Load Preset 0 → process 100 blocks
   - Load Preset 7 → process 100 blocks
   - Measure THD+N during transition
   - Verify no clicks (<-60dB)

4. testModulationConnectionsLoad()
   - Load preset with modulation (Presets 18-22)
   - Verify connection count > 0
   - Verify all depths ∈ [-1, +1]
```

**Integration:** Uses `monument_plugin_analyzer`

---

## 10. State Save/Recall Test (Low Priority)

**File:** `tests/StateSaveRecallTest.cpp`
**Purpose:** Verify DAW automation

### Test Cases

```cpp
1. testParameterSaveRecall()
   - Set all parameters to known values
   - Save state to MemoryBlock
   - Randomize all parameters
   - Load state from MemoryBlock
   - Verify all parameters match original

2. testSampleRateChangeRecall()
   - Save state @ 48kHz
   - Change to 96kHz
   - Load state
   - Verify parameters unchanged
   - Verify audio output scales correctly

3. testVersionCompatibility()
   - Load v1.0 state with v1.1 plugin
   - Verify no crashes
   - Verify parameters within expected ranges
```

**Integration:** Standalone unit test with APVTS

---

## Test Execution Strategy

### Local Development Workflow

```bash
# 1. Run all unit tests
ctest --test-dir build --output-on-failure

# 2. Run plugin analyzer tests
./scripts/run_comprehensive_tests.sh

# 3. View results
open test-results/comprehensive-report.html
```

### CI Integration (GitHub Actions)

```yaml
name: Comprehensive DSP Tests
on: [push, pull_request]

jobs:
  dsp-quality:
    runs-on: macos-latest
    steps:
      - uses: actions/checkout@v3
      - name: Build Monument
        run: cmake --build build --target Monument_All -j8

      - name: Run Unit Tests
        run: ctest --test-dir build --output-on-failure

      - name: Capture Baseline
        run: ./scripts/capture_all_presets.sh

      - name: Compare Regression
        run: |
          python3 tools/compare_baseline.py \
            test-results/baseline-v1.0.0 \
            test-results/preset-baseline \
            --threshold 0.001  # 0.1% tolerance

      - name: Upload Results
        uses: actions/upload-artifact@v3
        with:
          name: test-results
          path: test-results/
```

---

## Success Criteria

**All tests must pass for release:**

| Test Category | Pass Criteria |
|---------------|---------------|
| RT60 Accuracy | ±15% of target |
| Frequency Response | ±10dB flatness |
| CPU Performance | <2ms per block |
| Audio Regression | <0.1% L2 norm difference |
| Modulation Depth | Always ∈ [-1, +1] |
| Parameter Smoothing | <-60dB THD+N |
| Stereo Width | Correlation matches expected |
| Latency | Reported ≈ actual (±1s) |
| Preset Loading | No NaN/Inf, no crashes |
| State Save/Recall | Perfect match after load |

**Total Test Coverage:** ~50 individual test cases
**Execution Time:** ~5 minutes (parallel execution)
**Automation:** 100% (zero manual testing required)

---

## Implementation Timeline

**Phase 1 (High Priority - Week 1):**
- RT60 Accuracy Test
- Frequency Response Test
- CPU Performance Benchmark
- Audio Regression Test

**Phase 2 (Medium Priority - Week 2):**
- Modulation Depth Test
- Parameter Smoothing Test
- Stereo Width Test
- Latency & Phase Test

**Phase 3 (Low Priority - Week 3):**
- Preset Loading Test
- State Save/Recall Test
- CI integration
- Documentation

---

## Notes

- All tests designed for automated CI/CD pipelines
- Zero manual DAW interaction required
- Tests can run on headless servers
- Results stored in JSON for tracking over time
- Visual reports generated automatically
