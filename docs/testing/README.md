# Testing Documentation

Comprehensive testing guides and methodologies for Monument Reverb.

## Quick Start

### Run All Tests

```bash
# Full CI test suite (comprehensive)
./scripts/run_ci_tests.sh

# C++ tests only (fast)
ctest --test-dir build

# Capture baseline for regression testing
./scripts/capture_all_presets.sh
```

## Test Categories

### 1. Automated C++ Tests (CTest)

**Location:** `tests/*.cpp`

**Test Executables:**
- `monument_foundation_test` - Input stage tests
- `monument_pillars_zipper_test` - Zipper noise verification
- `monument_reverb_dsp_test` - Core reverb algorithm tests
- `monument_delay_dsp_test` - Delay line tests
- `monument_spatial_dsp_test` - Spatial processing tests
- `monument_dsp_routing_graph_test` - Routing graph tests
- `monument_modulation_matrix_test` - Modulation system tests
- `monument_novel_algorithms_test` - Physical modeling tests
- `monument_smoke_test` - Basic sanity checks
- `monument_parameter_buffer_test` - Parameter buffer tests
- `monument_parameter_stress_test` - Parameter automation stress tests
- `monument_dsp_initialization_test` - Initialization tests

**Run:**
```bash
cd build
ctest -C Release --output-on-failure
```

### 2. Audio Regression Testing

**Documentation:** [TESTING_GUIDE.md](TESTING_GUIDE.md)

**Tools:**
- `scripts/capture_all_presets.sh` - Capture audio for all 37 presets
- `scripts/analyze_all_presets.sh` - Analyze RT60, frequency response
- `scripts/compare_baseline.py` - Compare against baseline

**Metrics Captured:**
- RT60 decay time (4.85s to 29.85s across presets)
- Frequency response (±8.8 dB flatness)
- Audio regression detection

**Baseline:** `test-results/preset-baseline/`

### 3. Visual Regression Testing

**Documentation:** [UI_TESTING.md](UI_TESTING.md)

**Tools:**
- `tools/capture_ui_reference.py` - Capture UI screenshots
- Visual comparison against baseline

**Detects:**
- Background color changes
- Layout issues
- Font rendering changes
- Component sizing problems

### 4. Modulation Testing

**Documentation:** [MODULATION_TESTING_GUIDE.md](MODULATION_TESTING_GUIDE.md)

**Coverage:**
- 4 modulation sources (Chaos, Audio, Brownian, Envelope)
- 27 parameter destinations
- Connection depth and smoothing validation
- Real-time modulation accuracy

### 5. Performance Testing

**Documentation:** [PARAMETER_STRESS_RESULTS.md](PARAMETER_STRESS_RESULTS.md), [STRESS_TEST_PLAN.md](STRESS_TEST_PLAN.md)

**Tests:**
- CPU usage profiling
- Memory footprint validation
- Parameter automation stress testing
- 50-100 instance stress tests
- Real-time safety validation

### 6. Plugin Validation

**Tool:** pluginval

```bash
PLUGINVAL=/path/to/pluginval \
BUILD_DIR=build CONFIG=Release \
./scripts/run_pluginval.sh
```

**Checks:**
- Parameter automation
- State serialization
- NaN/denormal handling
- Thread safety
- Memory leaks

### 7. Manual Testing

**Documentation:** [TESTING.md](TESTING.md)

**Checklist:**
- Preset switching during playback
- Freeze toggle while playing
- Extreme parameter automation
- DAW compatibility (Ableton, Logic, Reaper, FL Studio)
- CPU monitoring with 50-100 instances

## Test Results & Baselines

### Current Test Status

**CTest:** 17/21 passing (81%)

**Baseline Data:**
- 37 factory presets analyzed
- RT60 range: 4.85s to 29.85s
- Frequency response: ±8.8 dB flatness
- All data stored in `test-results/preset-baseline/`

### Historical Validation

**Pre-Memory Validation:** [Monument_v1.0_Pre-Memory_Validation.md](Monument_v1.0_Pre-Memory_Validation.md)

**Phase Testing:**
- [PHASE_2_VALIDATION_TEST.md](../archive/phases/PHASE_2_VALIDATION_TEST.md)
- [PHASE_3_COMPLETE_SUMMARY.md](../archive/phases/PHASE_3_COMPLETE_SUMMARY.md)

## Testing Audits

### Comprehensive Audits
- [TESTING_AUDIT.md](TESTING_AUDIT.md) - Complete testing audit and recommendations

## Test Data Schemas

**Location:** [../schemas/](../schemas/)

**Schemas:**
- `capture_metadata.schema.json` - Audio capture metadata
- `cpu_profile.schema.json` - CPU profiling data
- `frequency_response.schema.json` - Frequency response analysis
- `regression_report.schema.json` - Regression test results
- `rt60_metrics.schema.json` - RT60 decay measurements
- `test_output_schemas.md` - Complete schema documentation

## Continuous Integration

### CI Workflow

```bash
# Standard CI test run
./scripts/run_ci_tests.sh

# Output:
# - Runs all CTest executables
# - Captures exit codes
# - Reports pass/fail status
# - Logs errors for debugging
```

### Pre-Commit Checks

```bash
# Build verification
cmake --build build --config Release

# Smoke tests
ctest --test-dir build -R "smoke" -C Release

# Real-time safety checks
# (manual code review for audio thread violations)
```

## Debug & Profiling

### Enable Testing Mode

```cmake
# Add to CMake configuration
-DCMAKE_CXX_FLAGS="-DMONUMENT_TESTING=1"
```

**Output:**
```
Monument MONUMENT_TESTING peak=0.892314 blockMs=0.312
```

### Profiling Tools
- **macOS:** Xcode Instruments (Leaks, Time Profiler)
- **Windows:** Visual Studio Performance Profiler
- **Linux:** Valgrind, perf

### Memory Tools
- **macOS:** Instruments (Leaks, Allocations)
- **Clang:** AddressSanitizer (`-fsanitize=address`)
- **Linux:** Valgrind (memcheck)

## Best Practices

### Test-Driven Development
1. Write failing test first
2. Implement feature to make test pass
3. Refactor while keeping tests green
4. Update documentation

### Regression Prevention
1. Capture baseline before changes
2. Run full test suite after changes
3. Compare audio output against baseline
4. Check CPU/memory impact
5. Validate in multiple DAWs

### Performance Testing
1. Profile before optimization
2. Set clear performance targets
3. Measure after changes
4. Verify no audio quality degradation
5. Document optimization results

## Related Documentation

- [../README.md](../../README.md) - Project overview
- [../architecture/README.md](../architecture/README.md) - Architecture documentation
- [../development/README.md](../development/README.md) - Development guides
- [../PERFORMANCE_BASELINE.md](../PERFORMANCE_BASELINE.md) - Performance metrics
- [../../STANDARD_BUILD_WORKFLOW.md](../../STANDARD_BUILD_WORKFLOW.md) - Build workflow
