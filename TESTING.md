# Testing

This is the canonical testing hub (CI/QA entrypoint, commands, and results).
For deep dives, see `docs/testing/`.

## Quick Start

### Run All Tests

```bash
# Build analyzer for preset capture + analysis
cmake --build build --config Release --target monument_plugin_analyzer

# Python deps for audio analysis (one-time)
python3 -m pip install -r tools/plugin-analyzer/python/requirements.txt

# Full CI/QA test suite (comprehensive)
./scripts/run_ci_tests.sh

# Use a non-default build directory (e.g., Ninja)
BUILD_DIR=build-ninja ./scripts/run_ci_tests.sh

# Continue after CTest failures (still exits non-zero at end)
CTEST_CONTINUE_ON_FAILURE=1 ./scripts/run_ci_tests.sh

# Continue after DSP critical failures (still exits non-zero at end)
DSP_CONTINUE_ON_FAILURE=1 ./scripts/run_ci_tests.sh

# Continue after preset capture failures (skips audio pipeline, exits non-zero)
PRESET_CAPTURE_CONTINUE_ON_FAILURE=1 ./scripts/run_ci_tests.sh

# Rerun only failed CTest tests from the last run
CTEST_RERUN_FAILED=1 ./scripts/run_ci_tests.sh

# Run a subset of CTest tests by regex (optional)
CTEST_FILTER=monument_dsp_ ./scripts/run_ci_tests.sh

# C++ tests only (fast)
ctest --test-dir build -C Release

# Capture baseline for regression testing
./scripts/capture_all_presets.sh

# Unattended capture/analysis (skip prompts)
NON_INTERACTIVE=1 ./scripts/capture_all_presets.sh
NON_INTERACTIVE=1 ./scripts/analyze_all_presets.sh
# (CI=1 is also supported)
```

Set `TEST_CONFIG=Debug` to run the harness against Debug builds.
Set `BUILD_DIR=build-ninja` (or another build folder) to point the harness at a Ninja build.

## Tooling Catalog (By Location)

**Core entrypoints:**
- `scripts/run_ci_tests.sh` - Master QA harness (CTest + audio regression + quality gates + optional UI/RT checks).
- `ctest --test-dir build -C Release` - Runs all registered C++ tests.
- `.github/workflows/ci.yml` - CI build + CTest smoke on macOS.
- `.github/workflows/build.yml` - CI build + artifact upload.

**Audio regression pipeline:**
- `scripts/capture_all_presets.sh` - Render IRs for all presets.
- `scripts/analyze_all_presets.sh` - RT60 + frequency response + spatial metrics.
- `tools/compare_baseline.py` - Baseline regression report.
- `tools/plot_preset_comparison.py` - Aggregate plots for RT60/frequency stats.
- `tools/plugin-analyzer/` - Analyzer sources + Python tooling (`python/requirements.txt`).

**Quality gates + validation:**
- `tools/check_audio_stability.py` - NaN/Inf/denormal/DC detection.
- `tools/check_cpu_thresholds.py` - CPU threshold enforcement.
- `tools/check_rt_allocations.sh` - Real-time allocation detection.
- `tools/validate_baseline.py` - Baseline integrity validation.
- `tools/validate_schemas.py` - JSON schema validation.

**UI regression + validation:**
- `tools/test_ui_visual.py` - Visual regression (writes `test-results/ui-current/`).
- `docs/testing/UI_TESTING.md` - UI testing workflow and baselines.

**Profiling + analysis:**
- `scripts/profile_cpu.sh` - Automated Instruments run (standalone).
- `scripts/profile_with_audio.sh` - Interactive Instruments run (standalone).
- `scripts/profile_in_reaper.sh` - Instruments run in REAPER host.
- `scripts/analyze_profile.py` - Trace analysis helper.

**Supplemental analysis:**
- `scripts/analyze_experimental_presets.py` - Experimental preset analysis.
- `scripts/generate_audio_demos.py` - Audio demo generation.

## Test Categories

### 1. Automated C++ Tests (CTest)

**Location:** `tests/*.cpp`

**Representative executables (see full inventory below):**
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
- `monument_macro_mode_integration_test` - Macro mode routing integration
- `monument_timeline_integration_test` - Timeline scheduler integration
- `monument_output_safety_clip_test` - Output limiter bounds
- `monument_parameter_smoothing_test` - Parameter smoothing correctness
- `monument_stereo_width_test` - Stereo width behavior
- `monument_latency_test` - Latency invariants
- `monument_state_management_test` - State serialization/restore
- `monument_feedback_mix_safety_test` - Feedback safety checks

**Run:**
```bash
cd build
ctest -C Release --output-on-failure
```

### 2. Audio Regression Testing

**Documentation:** `docs/testing/TESTING_GUIDE.md`

**Tools:**
- `scripts/capture_all_presets.sh` - Capture audio for all 37 presets
- `scripts/analyze_all_presets.sh` - Analyze RT60, frequency response, spatial metrics
- `tools/compare_baseline.py` - Compare against baseline

**Metrics Captured:**
- RT60 decay time (4.85s to 29.85s across presets)
- Frequency response (plus or minus 8.8 dB flatness)
- Spatial metrics (ITD/ILD/IACC, early-window)
- Audio regression detection

**Baseline:** `test-results/preset-baseline/`

### 3. Visual Regression Testing

**Documentation:** `docs/testing/UI_TESTING.md`

**Tools:**
- `tools/capture_ui_reference.py` - Capture UI screenshots (`--window-method cgwindow` for automated capture without AppleScript, `--manual` fallback)
- Visual comparison against baseline

**Detects:**
- Background color changes
- Layout issues
- Font rendering changes
- Component sizing problems

### 4. Modulation Testing

**Documentation:** `docs/testing/MODULATION_TESTING_GUIDE.md`

**Coverage:**
- 4 modulation sources (Chaos, Audio, Brownian, Envelope)
- 27 parameter destinations
- Connection depth and smoothing validation
- Real-time modulation accuracy

### 5. Performance Testing

**Documentation:** `docs/testing/PARAMETER_STRESS_RESULTS.md`, `docs/testing/STRESS_TEST_PLAN.md`

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

**Documentation:** `docs/testing/TESTING.md`

**Checklist:**
- Preset switching during playback
- Freeze toggle while playing
- Extreme parameter automation
- DAW compatibility (Ableton, Logic, Reaper, FL Studio)
- CPU monitoring with 50-100 instances

## Outputs & Artifacts

- `build/Testing/Temporary/LastTest.log` - Last CTest run log (local).
- `test-results/README.md` - Generated artifacts map and expectations.
- `test-results/preset-baseline/` - Per-preset IRs + metrics (RT60/frequency/spatial).
- `test-results/comparisons/` - Aggregate plots and statistics.
- `test-results/ui-baseline/` + `test-results/ui-current/` - UI screenshot baselines + reports.
- `test-results/experimental-analysis/` - Experimental preset analysis outputs.
- `test-results/audio-demos/` - Demo audio + visualizations.
- `test-results/regression-report.json` - Baseline compare report (if generated).
- `build/pluginval-report/` - pluginval report output (if generated).

## CTest Inventory (Registered in CMake)

All tests live under `tests/` and are registered in `CMakeLists.txt` with `add_test`.
Naming convention: `monument_<area>_test`.

CTest list:
- `monument_smoke_test` - basic sanity checks.
- `monument_memory_echoes_test` - Memory Echoes algorithm.
- `monument_experimental_modulation_test` - experimental modulation coverage.
- `monument_constant_power_panning_test` - panning law tests.
- `monument_doppler_shift_test` - Doppler shift DSP.
- `monument_sequence_scheduler_test` - scheduler timing.
- `monument_macro_mode_integration_test` - macro mode routing integration.
- `monument_timeline_integration_test` - timeline scheduler integration.
- `monument_output_safety_clip_test` - safety clip limiter bounds.
- `monument_parameter_smoothing_test` - smoothing correctness.
- `monument_stereo_width_test` - stereo width behavior.
- `monument_latency_test` - latency invariants.
- `monument_state_management_test` - state serialization/restore.
- `monument_dsp_routing_graph_test` - routing graph validation.
- `monument_feedback_mix_safety_test` - feedback safety.
- `monument_modulation_matrix_test` - modulation routing.
- `monument_novel_algorithms_test` - novel DSP modules.
- `monument_foundation_test` - foundation stage tests.
- `monument_parameter_buffer_test` - ParameterBuffer infrastructure.
- `monument_performance_benchmark` - performance benchmark (quick).
- `monument_parameter_stress_test` - automation stress (quick).
- `monument_pillars_zipper_test` - zipper noise tests.
- `monument_dsp_initialization_test` - prepare/reset lifecycle.
- `monument_reverb_dsp_test` - reverb DSP correctness.
- `monument_delay_dsp_test` - delay DSP correctness.
- `monument_spatial_dsp_test` - spatial processing.

## Adding Tests (Standard Workflow)

1. Add new test source in `tests/`.
2. Register the target + `add_test()` entry in `CMakeLists.txt`.
3. Run `ctest --test-dir build -C Release -R <test_name>` locally.
4. Update this file if the test is user-facing or part of the QA harness.

## Test Results & Baselines

### Current Test Status

**CTest:** Run `ctest --test-dir build -C Release --output-on-failure` for current status.

**Baseline Data:**
- 37 factory presets analyzed
- RT60 range: 4.85s to 29.85s
- Frequency response: plus or minus 8.8 dB flatness
- All data stored in `test-results/preset-baseline/`

### Historical Validation

**Pre-Memory Validation:** `docs/testing/Monument_v1.0_Pre-Memory_Validation.md`

**Phase Testing:**
- `docs/archive/phases/PHASE_2_VALIDATION_TEST.md`
- `docs/archive/phases/PHASE_3_COMPLETE_SUMMARY.md`

## Testing Audits

### Comprehensive Audits
- `docs/testing/TESTING_AUDIT.md` - Complete testing audit and recommendations

## Test Data Schemas

**Location:** `docs/schemas/`

**Schemas:**
- `capture_metadata.schema.json` - Audio capture metadata
- `cpu_profile.schema.json` - CPU profiling data
- `frequency_response.schema.json` - Frequency response analysis
- `regression_report.schema.json` - Regression test results
- `rt60_metrics.schema.json` - RT60 decay measurements
- `spatial_metrics.schema.json` - Spatial metrics (ITD/ILD/IACC)
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

- `README.md` - Project overview
- `docs/architecture/README.md` - Architecture documentation
- `docs/development/README.md` - Development guides
- `docs/PERFORMANCE_BASELINE.md` - Performance metrics
- `STANDARD_BUILD_WORKFLOW.md` - Build workflow
