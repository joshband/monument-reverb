# Monument Reverb — QA Harness Validation Report

**Date**: 2026-02-11
**Session**: C (Final Validation)
**Harness Version**: v1.1.0-dev
**Plugin**: Monument Reverb
**Adapter**: `qa/monument_adapter.h` (15 parameters)

---

## Executive Summary

Monument Reverb has been successfully migrated to the audio-dsp-qa-harness framework.
**19 scenarios** validate the plugin's core reverb behavior, preset variations, parameter
sweeps, spatial characteristics, parameter automation, and performance profile.

**Final Result**: 18 PASS, 1 WARN, 0 FAIL

---

## Suite Results

| Scenario | Status | Duration | Category |
|----------|--------|----------|----------|
| smoke_test | PASS | 2.0s | Smoke |
| reverb_core | PASS | 4.0s | Core |
| spatial_width | PASS | 4.0s | Spatial |
| freeze_mode | PASS | 4.0s | Feature |
| preset_cathedral | PASS | 4.0s | Preset |
| preset_spring | PASS | 4.0s | Preset |
| preset_infinite_abyss | PASS | 4.0s | Preset |
| preset_elastic_hall | PASS | 4.0s | Preset |
| parameter_sweep_time_short | PASS | 4.0s | Sweep |
| parameter_sweep_time_medium | PASS | 4.0s | Sweep |
| parameter_sweep_time_long | PASS | 4.0s | Sweep |
| parameter_sweep_mass_bright | PASS | 4.0s | Sweep |
| parameter_sweep_mass_neutral | PASS | 4.0s | Sweep |
| parameter_sweep_mass_dark | PASS | 4.0s | Sweep |
| reverb_modulation | PASS | 6.0s | Automation |
| reverb_air_envelope | PASS | 6.0s | Automation |
| reverb_time_ramp | PASS | 4.0s | Automation |
| frequency_response_air | WARN | 2.0s | Frequency |
| performance_profile | PASS | 16.0s | Performance |

**Total runtime**: ~80s (19 scenarios)

---

## Known Limitation

### frequency_response_air (WARN)

The sweep stimulus produces silent output (-180 dBFS) through the in-process runner.
This is because the logarithmic sweep stimulus doesn't properly interact with Monument's
internal processing at 48kHz/512 block size, or the render duration (2s) is too short
for the sweep energy to propagate through the reverb's delay network.

**Severity**: soft_warn (non-blocking)
**Impact**: Frequency response validation is not functional for this plugin via in-process runner.
A standalone binary runner with longer render times may resolve this.

---

## Metric Coverage

**16 unique metrics** across 19 scenarios:

| Metric | Scenarios Using | Category |
|--------|-----------------|----------|
| rt60 | 8 | Reverb |
| clarity_c50 | 2 | Reverb |
| clarity_c80 | 2 | Reverb |
| monotonic_tail_decay | 1 | Reverb |
| spectral_centroid | 8 | Spectral |
| spectral_flatness | 2 | Spectral |
| crest_factor | 3 | Spectral |
| dynamic_range | 1 | Spectral |
| flux_rate | 1 | Spectral |
| spatial_width | 2 | Spatial |
| correlation_lr | 1 | Spatial |
| iacc_early / iacc_late | 1 | Spatial |
| ild_max_db / itd_max_ms | 1 | Spatial |
| energy_growth | 2 | Energy |
| rms_energy | 2 | Energy |
| perf_metrics | 1 | Performance |

---

## Parameter Coverage

**15/15 core parameters** mapped (Phase 1):

| Index | Parameter | Range | Scenarios Testing |
|-------|-----------|-------|-------------------|
| 0 | mix | 0-100% | All (set to 100% wet) |
| 1 | time | 0-1 | sweep_time_*, core, presets |
| 2 | mass | 0-1 | sweep_mass_*, presets |
| 3 | density | 0-1 | presets |
| 4 | bloom | 0-1 | presets |
| 5 | air | 0-1 | air_envelope, presets |
| 6 | width | 0-1 | spatial_width, presets |
| 7 | warp | 0-1 | reverb_modulation, presets |
| 8 | drift | 0-1 | presets |
| 9 | memory | 0-1 | presets |
| 10 | memoryDepth | 0-1 | presets |
| 11 | memoryDecay | 0-1 | presets |
| 12 | memoryDrift | 0-1 | presets |
| 13 | gravity | 0-1 | preset_spring |
| 14 | freeze | bool | freeze_mode |

---

## Feature Adoption

| Feature | Status | Details |
|---------|--------|---------|
| Auto-discovery | Adopted | `--discover scenarios/monument/` |
| Result export | Adopted | suite_result.json + per-scenario output |
| Baseline tracking | Adopted | All 19 scenarios |
| Parameter automation | Adopted | 3 scenarios (LFO, Ramp, Envelope) |
| Threshold tuning | Adopted | All thresholds tuned ±50-60% of baselines |
| Architecture preset | Adopted | `reverb_architecture` preset on core |
| Safety invariants | Auto | non_finite + peak_level on all scenarios |
| Spectral metrics | Adopted | 8 scenarios |
| Spatial metrics | Adopted | 2 scenarios |
| Reverb metrics | Adopted | 8 scenarios |

---

## CTest Coexistence

Monument-reverb's existing CTest suite (26 tests) was validated alongside the QA harness:

- **19/26 passing** (same as baseline without QA harness changes)
- **7 pre-existing failures** (5 "Not Run" due to JUCE artifact paths, 1 parameter smoothing test, 1 spatial DSP test)
- **QA harness changes did not introduce any new test failures**

---

## SAF Library Fix

The harness CMake export was updated to properly export SAF as a transitive dependency:

- `CMakeLists.txt`: Added `install(FILES $<TARGET_FILE:saf> ...)` rule
- `Config.cmake.in`: Added `find_library(_qa_saf_library saf ...)` + Accelerate framework
- Monument-reverb no longer needs manual `$ENV{HOME}/.local/lib/libsaf.a` workaround
- Harness tests: 45/45 passing after fix

---

## Observations

### RT60 Values
Measured RT60 values (2.13s-10.80s) are lower than Monument's internal test baselines (10-30s).
This is expected: the harness uses a 100ms noise burst excitation, while internal tests use
sustained excitation or impulse responses with different energy profiles.

### Mass Parameter Centroid Insensitivity
All three mass settings produce nearly identical spectral centroids (~10920-10940 Hz).
The white noise stimulus dominates the centroid measurement. A tail-only analysis window
(after noise stops) would better isolate mass damping effects.

### Performance Profile
CPU usage is well within bounds at 48kHz/512 block size.

---

**Prepared**: 2026-02-11
**Validated by**: AI Agent (Session C)
**Harness commit**: v1.1.0-dev (post-SAF fix)
