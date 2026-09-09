# Monument Reverb — CTest Failures (Pre-Existing)

**Date**: 2026-09-08
**Status**: 28/32 tests passing (88%), measured via a real full-suite build+run (`-DMONUMENT_ENABLE_TESTS=ON -DBUILD_TESTING=ON`, Debug config, local JUCE)

---

## Summary

Monument's CTest suite currently has 32 registered targets (see `TESTING.md`'s "CTest Inventory" for the full, current list — this file only tracks non-passing ones, it is not an inventory). Two failures are consistent, pre-existing DSP issues unrelated to this session's report-driven fixes. Two more only fail when run as part of the full suite and pass cleanly in isolation — order/timing-dependent, not confirmed regressions.

While getting these numbers, this pass also found and fixed a real, unrelated bug: `monument_doppler_shift_test` and `monument_spatial_dsp_test` were not previously being exercised at all, and the reason had nothing to do with CMake artifact paths (as a prior version of this doc claimed) — both test files called `SpatialProcessor::process()` with no arguments, while the actual method signature is `process(int numSamples)`. This was a stale test/API mismatch, not a build-configuration issue. Both files were fixed to pass the block size already established via `prepare()`; both compile and pass now.

---

## Consistent Failures (2)

### 1. monument_parameter_smoothing_test

**Status**: FAILED (2/50 parameter sweep sub-tests)

**Details**:
- Safety Clip: transient -17.2 dB, 2 clicks detected
- Safety Clip Drive: transient -14.2 dB, 0 clicks (but transient exceeds threshold)
- All other 48 parameters pass cleanly

**Root cause**: Parameter smoothing implementation issue specific to the safety-clip parameters (unrelated to the smoothing fixes this session made elsewhere — Chambers/ModulationMatrix RNG determinism, allocation fixes, etc. do not touch this path).

**Impact**: LOW — Safety clip is an advanced/optional feature, not part of the core reverb signal path.

### 2. monument_reverb_dsp_test

**Status**: FAILED (1/6 sub-tests)

**Details**:
- Stereo Decorrelation: measured 0.987946 (expected < 0.95)
- All other sub-tests pass: RT60 decay, late-tail stability, DC offset, freeze-mode stability, parameter-jump transients

**Root cause**: FDN decorrelation configuration issue. This exact value (0.987946) has been reproduced identically across multiple runs this session (see git history for earlier confirmations) — it is a stable, reproducible, pre-existing finding, not new.

**Impact**: MEDIUM — stereo decorrelation affects reverb quality, but this is isolated to this specific CTest's configuration; it is not a regression from any change made this session.

---

## Order/Timing-Dependent Failures (2) — not confirmed regressions

These two failed when run as part of the full 32-test suite, but **passed cleanly when re-run individually** (`ctest -R <name>`). This points to test-order interference or Debug-build timing sensitivity rather than a real, standalone defect. Listed here for transparency, not as confirmed bugs.

### 3. monument_modulation_matrix_test

Failed in the full-suite run; passed 100% in isolation. No specific assertion detail was captured from the full-suite failure (output wasn't retained) — if this recurs, capture `ctest --output-on-failure -R monument_modulation_matrix_test` output from a full-suite run specifically, not an isolated one.

### 4. monument_performance_benchmark

Failed on "Single Module CPU Profiling" (Chambers measured 9.44% p99 CPU against whatever threshold this benchmark enforces) in the full-suite run. This is a Debug-build wall-clock CPU benchmark — Debug builds are unoptimized and their timing is sensitive to machine load and to how many other tests already ran in the same process invocation. Consistent with a class of Debug-build timing flakiness already noted elsewhere in this session's PRs (e.g. `monument_dsp_routing_graph_test`'s "CPU Performance Budget" sub-test). Not confirmed as a real regression; re-run and check against a Release build before treating as actionable.

---

## Fixed This Pass: Stale SpatialProcessor API Usage

`tests/DopplerShiftTest.cpp` (9 call sites) and `tests/SpatialDspTest.cpp` (7 call sites) called `spatial.process()` with no arguments. `dsp/SpatialProcessor.h` declares `void process(int numSamples) noexcept;` — a required argument, not defaulted. Both files now call `spatial.process(kBlockSize)`, using each file's own existing `kBlockSize` constant (matching what's already passed to `spatial.prepare()`). Both targets now compile and pass:

```
30/32 monument_delay_dsp_test .............. Passed
32/32 monument_spatial_dsp_test ............ Passed
```

(`monument_doppler_shift_test` also builds and runs as part of the same fix — see the full suite run above.)

This is a test-file-only fix (no DSP/production code touched) — it corrects test code that had drifted out of sync with an API change, so the suite can actually exercise `SpatialProcessor`'s Doppler/spatial-positioning logic instead of silently not running at all.

---

## Notes

- These consistent failures (Safety Clip smoothing, Stereo Decorrelation) are unrelated to this session's RT-safety/architecture fixes and predate them.
- The authoritative DSP QA gate is the `audio-dsp-qa-harness` scenario suites (see `TESTING.md`'s "DSP QA Authority Policy"), not this CTest inventory — CTest failures here don't block CI merges, and a green CTest run doesn't substitute for the harness suites either.
- For the current, full CTest target list (all 32), see `TESTING.md`'s "CTest Inventory" section rather than this file — this file is a failures log, not an inventory.
