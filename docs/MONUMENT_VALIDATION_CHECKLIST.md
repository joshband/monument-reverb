# Monument Reverb — Migration Validation Checklist

**Date**: 2026-02-11
**Session**: C (Final Validation)

---

## Phase 1: Adapter Implementation
- [x] `qa/monument_adapter.h` — DspUnderTest adapter created
- [x] `qa/monument_adapter.cpp` — Implementation complete
- [x] `qa/main.cpp` — QA executable entry point
- [x] 15 core parameters mapped (mix, time, mass, density, bloom, air, width, warp, drift, memory, memoryDepth, memoryDecay, memoryDrift, gravity, freeze)
- [x] Capabilities: REVERB | STATEFUL
- [x] OptionalFeatures: latencyReport=true
- [x] JUCE AudioBuffer integration (processBlock wrapper)
- [x] Re-init logic for SmoothedValue initialization

## Phase 2: Scenario Development
- [x] 19 scenario JSON files in `scenarios/monument/`
- [x] smoke_test (basic non_finite + peak_level)
- [x] reverb_core (RT60 + clarity + spectral + tail decay)
- [x] spatial_width (6 spatial metrics)
- [x] freeze_mode (energy_growth stability)
- [x] 4 preset scenarios (cathedral, spring, infinite_abyss, elastic_hall)
- [x] 3 time sweep scenarios (short, medium, long)
- [x] 3 mass sweep scenarios (bright, neutral, dark)
- [x] 3 automation scenarios (LFO on warp, envelope on air, ramp on time)
- [x] frequency_response_air (WARN — known limitation)
- [x] performance_profile (perf_metrics)
- [x] 2 suite files (monument_suite.json, monument_critical_suite.json)

## Phase 3: Threshold Tuning
- [x] All thresholds tuned from permissive ranges to ±50-60% of measured baselines
- [x] RT60: ±60% of measured values
- [x] Clarity (C50/C80): ±8-9 dB of measured values
- [x] Spectral metrics: ±30-50% of measured values
- [x] Spatial metrics: Tightened to physically reasonable bounds
- [x] Energy metrics: Tightened based on measured baseline with margin
- [x] Tuning log: `docs/MONUMENT_THRESHOLD_TUNING_LOG.md`

## Phase 4: Feature Adoption
- [x] Auto-discovery (`--discover` CLI mode)
- [x] Result export (suite_result.json)
- [x] Baseline tracking (all 19 scenarios)
- [x] Parameter automation (3 scenarios: LFO, Ramp, Envelope)
- [x] Architecture-specific threshold preset (reverb_architecture on reverb_core)
- [x] Default safety invariants (non_finite + peak_level auto-applied)
- [x] Spectral metrics (spectral_centroid, spectral_flatness, crest_factor, dynamic_range, flux_rate)
- [x] Spatial metrics (spatial_width, correlation_lr, iacc_early, iacc_late, ild_max_db, itd_max_ms)
- [x] Reverb metrics (rt60, clarity_c50, clarity_c80, monotonic_tail_decay)
- [x] Energy metrics (energy_growth, rms_energy)
- [x] Performance metrics (perf_metrics)

## Phase 5: Validation
- [x] Full suite: 18 PASS, 1 WARN, 0 FAIL
- [x] CTest coexistence: 19/26 passing (same as baseline — no regressions)
- [x] SAF fix verified: monument_qa links without manual libsaf.a workaround
- [x] Validation report: `docs/MONUMENT_VALIDATION_REPORT.md`
- [x] Validation checklist: `docs/MONUMENT_VALIDATION_CHECKLIST.md` (this file)

## Build Integration
- [x] `BUILD_QA_HARNESS` CMake option (OFF by default)
- [x] `monument_qa` JUCE console app target
- [x] `find_package(audio_dsp_qa_harness)` integration
- [x] Links: qa::qa_core, qa::qa_runners, qa::qa_scenario_engine
- [x] No manual SAF workaround needed (post harness fix)

## Documentation
- [x] MONUMENT_THRESHOLD_TUNING_LOG.md (176 lines)
- [x] MONUMENT_VALIDATION_REPORT.md
- [x] MONUMENT_VALIDATION_CHECKLIST.md (this file)
- [x] Session handoff documents

---

## Summary

| Metric | Value |
|--------|-------|
| Total scenarios | 19 |
| PASS | 18 |
| WARN | 1 (known limitation) |
| FAIL | 0 |
| Parameters mapped | 15/15 core |
| Unique metrics | 16 |
| Feature adoption | 11/11 applicable features |
| CTest regressions | 0 |

**Status**: PRODUCTION-READY
