# Monument Reverb - Stress Testing & Performance Benchmark Plan

**Created:** 2026-01-09
**Purpose:** Comprehensive stress testing framework for production validation
**Target:** Identify edge cases, performance bottlenecks, and real-time safety violations

---

## Overview

This plan defines a systematic approach to stress testing Monument Reverb beyond normal functional tests. The goal is to ensure the plugin remains stable, performant, and real-time safe under extreme conditions.

## Test Categories

### Category 1: Performance Benchmarks (12 tests)

**Purpose:** Measure CPU usage, memory behavior, and SIMD utilization

| Test | Description | Success Criteria |
|------|-------------|------------------|
| **CPU-1** | Single Module CPU Profiling | Each module < 5% CPU per instance |
| **CPU-2** | Full Chain CPU Budget | Total DSP < 30% CPU at 48kHz |
| **CPU-3** | High Sample Rate (192kHz) | CPU < 60% at 192kHz |
| **CPU-4** | Low Latency Mode (64 samples) | CPU < 40% at 64-sample blocks |
| **MEM-1** | Zero Allocation Verification | No allocations in processBlock |
| **MEM-2** | Memory Footprint Baseline | < 50MB RAM per instance |
| **MEM-3** | Prepare/Release Cycles | No memory leaks over 1000 cycles |
| **SIMD-1** | Vectorization Verification | SIMD paths active for key modules |
| **CACHE-1** | Cache Line Optimization | < 10% L1 cache misses |
| **WCET-1** | Worst-Case Execution Time | Max execution < 80% of block time |
| **THREAD-1** | Lock Contention Detection | Zero lock waits in audio thread |
| **THREAD-2** | Priority Inversion Check | No priority inversions detected |

---

### Category 2: Parameter Stress Tests (15 tests)

**Purpose:** Test extreme parameter values and rapid automation

| Test | Description | Success Criteria |
|------|-------------|------------------|
| **PARAM-1** | All Parameters Zero | No crashes, output = 0 or silence |
| **PARAM-2** | All Parameters Maximum | No instability, bounded output |
| **PARAM-3** | All Parameters Random | No crashes over 10s |
| **PARAM-4** | Rapid Parameter Sweeps | No zipper noise > -40dB |
| **PARAM-5** | Parameter Jump Stress | Clicks < -30dB on instant changes |
| **PARAM-6** | Automation Storm | 47 params changing simultaneously |
| **PARAM-7** | Feedback at Maximum | No runaway amplification |
| **PARAM-8** | Resonance at Maximum | No instability or clipping |
| **PARAM-9** | Freeze + Feedback 100% | Bounded energy over 60s |
| **PARAM-10** | RT60 at Minimum (2s) | Fast decay verified |
| **PARAM-11** | RT60 at Maximum (35s) | Stable long decay |
| **PARAM-12** | Diffusion Extremes | 0% and 100% both stable |
| **PARAM-13** | Modulation at Maximum Rate | No aliasing or artifacts |
| **PARAM-14** | Invalid Parameter Values | Clamping/validation working |
| **PARAM-15** | Preset Switching Rapid | < 10ms switch time, no clicks |

---

### Category 3: Long-Duration Stability (8 tests)

**Purpose:** Verify stability over extended periods

| Test | Description | Success Criteria |
|------|-------------|------------------|
| **LONG-1** | 1 Hour Silence Processing | No drift, no denormals |
| **LONG-2** | 1 Hour Continuous Tone | Stable output, no artifacts |
| **LONG-3** | 1 Hour Random Noise | Bounded energy, no growth |
| **LONG-4** | 1 Hour Freeze Mode | Energy stable ±6dB |
| **LONG-5** | 1 Hour Automation Loop | Smooth parameter behavior |
| **LONG-6** | 24 Hour Idle Test | No resource leaks |
| **LONG-7** | Denormal Accumulation Test | Flush-to-zero working |
| **LONG-8** | DC Offset Long-Term | < 0.001 after 1 hour |

---

### Category 4: Edge Case Scenarios (10 tests)

**Purpose:** Test unusual but valid operating conditions

| Test | Description | Success Criteria |
|------|-------------|------------------|
| **EDGE-1** | Zero-Length Buffer | No crash on 0-sample blocks |
| **EDGE-2** | Mono Input Processing | Correct mono → stereo expansion |
| **EDGE-3** | 8-Channel Surround | Handles > 2 channels gracefully |
| **EDGE-4** | Sample Rate Change Live | Smooth transition, no clicks |
| **EDGE-5** | Block Size Change Live | Handles dynamic block size |
| **EDGE-6** | Denormal Numbers Input | FTZ active, no performance hit |
| **EDGE-7** | DC Input (1.0 constant) | DC blocking working |
| **EDGE-8** | Nyquist Frequency Input | Aliasing prevention working |
| **EDGE-9** | Inf/NaN Input Handling | Sanitized, no propagation |
| **EDGE-10** | Bypass → Active Transition | No pops or glitches |

---

### Category 5: Real-Time Safety (8 tests)

**Purpose:** Verify real-time audio constraints

| Test | Description | Success Criteria |
|------|-------------|------------------|
| **RT-1** | Allocation Detection | Zero allocations in hot path |
| **RT-2** | System Call Detection | No syscalls in processBlock |
| **RT-3** | Lock/Mutex Detection | No locks in audio thread |
| **RT-4** | Virtual Call Overhead | Minimal vtable overhead |
| **RT-5** | Exception Safety | No try/catch in hot path |
| **RT-6** | Unbounded Loop Detection | All loops have fixed bounds |
| **RT-7** | Stack Overflow Check | Stack usage < 16KB per call |
| **RT-8** | Thread Affinity | Audio thread on performance core |

---

### Category 6: Numerical Stability (7 tests)

**Purpose:** Verify numerical behavior and precision

| Test | Description | Success Criteria |
|------|-------------|------------------|
| **NUM-1** | Denormal Performance | No 100× slowdown with denormals |
| **NUM-2** | Float Precision Stability | No double → float precision loss |
| **NUM-3** | Filter Coefficient Stability | IIR filters remain stable |
| **NUM-4** | Fixed-Point Wrap Detection | No integer overflow |
| **NUM-5** | Trigonometric Precision | Sin/cos within 0.001 tolerance |
| **NUM-6** | Exponential Stability | Decay curves remain monotonic |
| **NUM-7** | Matrix Orthogonality | Feedback matrices ≤ 1.0 spectral radius |

---

## Implementation Strategy

### Phase 1: Performance Benchmarks (Priority 1)

**File:** `tests/PerformanceBenchmarkTest.cpp`
**Time Estimate:** 3-4 hours
**Dependencies:** None

**Implementation:**

```cpp
// CPU profiling with high-resolution timing
auto startTicks = juce::Time::getHighResolutionTicks();
module.process(buffer);
auto endTicks = juce::Time::getHighResolutionTicks();
double cpuPercent = calculateCPUUsage(startTicks, endTicks, sampleRate, blockSize);

// Memory allocation tracking
struct AllocationTracker {
    static bool detectedAllocation;
    static void* operator new(size_t size) {
        detectedAllocation = true;
        return ::operator new(size);
    }
};

// SIMD verification (check for vectorized paths)
#if JUCE_USE_SSE_INTRINSICS || JUCE_USE_NEON
    // Verify SIMD code paths are active
#endif
```

**CMake Configuration:**

```cmake
juce_add_console_app(monument_performance_benchmark
    PRODUCT_NAME "Monument Performance Benchmark")

target_sources(monument_performance_benchmark PRIVATE
    tests/PerformanceBenchmarkTest.cpp
    # All DSP modules
)

target_compile_definitions(monument_performance_benchmark PRIVATE
    JUCE_MODAL_LOOPS_PERMITTED=1
    MONUMENT_TESTING=1
    MONUMENT_BENCHMARK_MODE=1)  # Enable benchmarking instrumentation
```

---

### Phase 2: Parameter Stress Tests (Priority 2)

**File:** `tests/ParameterStressTest.cpp`
**Time Estimate:** 3-4 hours
**Dependencies:** None

**Implementation:**

```cpp
// All parameters to extreme values
for (auto* param : apvts.parameters) {
    param->setValue(0.0f);  // or 1.0f for max test
}

// Rapid automation (parameter changes every sample)
for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
    float paramValue = std::sin(sample * 0.1f) * 0.5f + 0.5f;
    paramSmoothed.setTargetValue(paramValue);
    // Process single sample
}

// Zipper noise detection
float maxJump = 0.0f;
for (int i = 1; i < buffer.getNumSamples(); ++i) {
    float jump = std::abs(buffer.getSample(0, i) - buffer.getSample(0, i-1));
    maxJump = std::max(maxJump, jump);
}
```

---

### Phase 3: Long-Duration Tests (Priority 3)

**File:** `tests/LongDurationTest.cpp`
**Time Estimate:** 2-3 hours (plus execution time)
**Dependencies:** None

**Implementation:**

```cpp
// Long test loop with progress reporting
const int totalBlocks = static_cast<int>(sampleRate * durationSeconds / blockSize);
for (int block = 0; block < totalBlocks; ++block) {
    // Process block
    module.process(buffer);

    // Sample energy every 1000 blocks
    if (block % 1000 == 0) {
        float energy = calculateRMS(buffer);
        energySamples.push_back(energy);

        // Progress report
        if (block % 10000 == 0) {
            std::cout << "Progress: " << (block * 100 / totalBlocks) << "%\n";
        }
    }
}

// Verify energy bounds
float minEnergy = *std::min_element(energySamples.begin(), energySamples.end());
float maxEnergy = *std::max_element(energySamples.begin(), energySamples.end());
float energyRange = 20.0f * std::log10(maxEnergy / minEnergy);
```

**Note:** Long tests can be run separately with environment variable:

```bash
MONUMENT_RUN_LONG_TESTS=1 ./build/monument_long_duration_test
```

---

### Phase 4: Edge Cases & Real-Time Safety (Priority 4)

**File:** `tests/EdgeCaseTest.cpp`
**File:** `tests/RealTimeSafetyTest.cpp`
**Time Estimate:** 2-3 hours each
**Dependencies:** Profiling tools

**Implementation:**

```cpp
// Denormal detection
bool hasDenormals(const juce::AudioBuffer<float>& buffer) {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float sample = buffer.getSample(ch, i);
            if (std::fpclassify(sample) == FP_SUBNORMAL) return true;
        }
    }
    return false;
}

// Inf/NaN detection
bool hasInvalidNumbers(const juce::AudioBuffer<float>& buffer) {
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            float sample = buffer.getSample(ch, i);
            if (!std::isfinite(sample)) return true;
        }
    }
    return false;
}
```

---

## CI Integration

### Quick Stress Test Suite (< 30 seconds)

Runs automatically on every commit:

```bash
# Add to scripts/run_ci_tests.sh
echo "Phase STRESS: Quick Stress Tests..."
./build/monument_performance_benchmark_artefacts/Debug/monument_performance_benchmark --quick
./build/monument_parameter_stress_test_artefacts/Debug/monument_parameter_stress_test --quick
./build/monument_edge_case_test_artefacts/Debug/monument_edge_case_test
```

### Full Stress Test Suite (< 5 minutes)

Runs nightly or on release branches:

```bash
# Full suite with all stress tests
./scripts/run_full_stress_tests.sh
```

### Extended Stability Tests (1-24 hours)

Runs manually before releases:

```bash
MONUMENT_RUN_LONG_TESTS=1 ./build/monument_long_duration_test --duration=3600
```

---

## Success Criteria Summary

### Performance Targets

- **CPU Usage:** < 30% full chain at 48kHz, 512 samples
- **Memory:** Zero allocations in audio callback
- **Latency:** < 80% of available time budget (WCET)
- **Stability:** Bounded output over 1+ hour tests

### Stability Targets

- **Energy Growth:** < ±6dB over any 1-minute window
- **Denormals:** Flush-to-zero active, no performance degradation
- **DC Offset:** < 0.001 absolute value
- **Numerical Stability:** No Inf, NaN, or runaway values

### Real-Time Safety

- **Zero allocations** in processBlock
- **Zero locks** in audio thread
- **Zero system calls** during processing
- **Bounded execution** time every block

---

## Deliverables

### Test Files

1. ✅ `tests/PerformanceBenchmarkTest.cpp` - CPU/memory/SIMD profiling
2. ✅ `tests/ParameterStressTest.cpp` - Extreme parameter testing
3. ✅ `tests/LongDurationTest.cpp` - Extended stability tests
4. ✅ `tests/EdgeCaseTest.cpp` - Edge cases and corner scenarios
5. ✅ `tests/RealTimeSafetyTest.cpp` - RT constraint verification
6. ✅ `tests/NumericalStabilityTest.cpp` - Precision and stability

### Documentation

1. ✅ `docs/STRESS_TEST_PLAN.md` (this document)
2. ✅ `docs/PERFORMANCE_BASELINE.md` - Baseline measurements
3. ✅ `docs/STRESS_TEST_RESULTS.md` - Test execution report

### Scripts

1. ✅ `scripts/run_full_stress_tests.sh` - Comprehensive test suite
2. ✅ `scripts/run_long_tests.sh` - Extended duration tests
3. ✅ `scripts/profile_cpu.sh` - CPU profiling helper

### CMake Targets

1. ✅ `monument_performance_benchmark` - Performance profiling
2. ✅ `monument_parameter_stress_test` - Parameter stress tests
3. ✅ `monument_long_duration_test` - Stability tests
4. ✅ `monument_edge_case_test` - Edge case coverage
5. ✅ `monument_realtime_safety_test` - RT verification
6. ✅ `monument_numerical_stability_test` - Numerical tests

---

## Timeline

**Phase 1 (Performance):** 1 session (~4 hours)
**Phase 2 (Parameters):** 1 session (~4 hours)
**Phase 3 (Long Duration):** 1 session (~3 hours + execution time)
**Phase 4 (Edge Cases + RT):** 1 session (~5 hours)
**Phase 5 (Numerical):** 1 session (~3 hours)
**Phase 6 (Documentation):** 1 session (~2 hours)

**Total:** 6 sessions (~21 hours development + test execution time)

---

## Notes

- Tests are designed to be **non-destructive** (no changes to source DSP code)
- All tests follow existing pattern (color output, TestResult structure)
- Long tests can be skipped in CI with environment flag
- Performance baselines will be documented for regression detection
- Real-time safety tests may require instrumentation builds

---

**Status:** Planning Complete ✅
**Next Step:** Implement Phase 1 (Performance Benchmarks)
