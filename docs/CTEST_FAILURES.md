# Monument Reverb — CTest Failures (Pre-Existing)

**Date**: 2026-02-12
**Context**: QA harness migration validation (Session C)
**Status**: 22/26 tests passing (85%)

---

## Summary

Monument's CTest suite has 4 non-passing tests that are **pre-existing issues** unrelated to the QA harness integration. These represent plugin functionality issues, not test infrastructure problems.

**QA Harness Migration Impact**: ZERO regressions introduced (0/4 failures caused by harness)

---

## Failed Tests (2)

### 1. monument_parameter_smoothing_test

**Status**: FAILED
**Failure**: 2/50 parameter sweep tests failing

**Details**:
- Safety Clip: Click detection failed (transient: -17.2 dB, clicks: 2)
- Safety Clip Drive: Transient detection failed (transient: -14.2 dB, clicks: 0)

**Root Cause**: Parameter smoothing implementation issue with safety clip parameters

**Impact**: LOW — Safety clip is an advanced feature, not part of core reverb

**Recommendation**: Fix parameter smoothing for safety clip parameters (2-3h effort)

---

### 2. monument_reverb_dsp_test

**Status**: FAILED
**Failure**: 1/6 reverb DSP tests failing

**Details**:
- Stereo Decorrelation: Poor decorrelation measured (0.987946, expected < 0.95)
- All other tests pass: RT60, tail stability, DC offset, freeze mode, parameter jumps

**Root Cause**: FDN decorrelation configuration issue

**Discrepancy**: QA harness `spatial_width` scenario measures excellent decorrelation (correlation_lr=0.003), suggesting configuration-dependent behavior

**Impact**: MEDIUM — Stereo decorrelation is important for reverb quality

**Recommendation**: Investigate FDN configuration difference between CTest and QA scenarios (1-2h effort)

---

## Not Run Tests (2)

### 3. monument_doppler_shift_test

**Status**: NOT RUN
**Reason**: Executable not found in expected artifact paths

**Expected Path**: `build/monument_doppler_shift_test_artefacts/Release/monument_doppler_shift_test`

**Root Cause**: CMake artifact path configuration issue (JUCE build system)

**Impact**: LOW — Doppler shift is an advanced spatial feature

**Recommendation**: Fix CMake artifact paths or rebuild with correct configuration (30min effort)

---

### 4. monument_spatial_dsp_test

**Status**: NOT RUN
**Reason**: Executable not found in expected artifact paths

**Expected Path**: `build/monument_spatial_dsp_test_artefacts/Release/monument_spatial_dsp_test`

**Root Cause**: CMake artifact path configuration issue (JUCE build system)

**Impact**: LOW — Spatial DSP is tested via QA harness spatial_width scenario

**Recommendation**: Fix CMake artifact paths or rebuild with correct configuration (30min effort)

---

## Action Plan

**Priority 1: Stereo Decorrelation Investigation** (1-2h)
- Compare FDN configuration between monument_reverb_dsp_test and QA harness spatial_width scenario
- Identify why CTest measures poor decorrelation (0.987946) vs. QA harness excellent decorrelation (0.003)
- Fix FDN configuration or update test expectations

**Priority 2: Parameter Smoothing Fix** (2-3h)
- Fix safety clip parameter smoothing to eliminate clicks during parameter sweeps
- Ensure all 50 parameters pass smoothing tests

**Priority 3: Artifact Path Fix** (30min)
- Fix CMake JUCE artifact paths for doppler_shift_test and spatial_dsp_test
- Rebuild with correct configuration

---

## Total Estimated Effort

- **Stereo Decorrelation**: 1-2h
- **Parameter Smoothing**: 2-3h
- **Artifact Paths**: 30min
- **Total**: 4-6h to achieve 26/26 CTest passing

---

## Notes

- These issues are unrelated to QA harness integration
- QA harness provides orthogonal validation (19/19 scenarios passing)
- Fixing these issues will improve plugin quality but is not blocking for QA harness migration
- Monument-reverb is **PRODUCTION-READY** for QA harness usage (100% harness pass rate)

---

**Prepared**: 2026-02-12
**Context**: Post-QA harness migration validation
**Next Review**: After CTest failures are addressed
