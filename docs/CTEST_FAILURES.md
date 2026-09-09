# Monument Reverb — CTest Failures (Pre-Existing)

**Date**: 2026-09-08
**Status**: 31/32 tests passing (97%), measured via a real full-suite build+run (`-DMONUMENT_ENABLE_TESTS=ON -DBUILD_TESTING=ON`, Release config, local JUCE)

---

## Summary

Monument's CTest suite currently has 32 registered targets (see `TESTING.md`'s "CTest Inventory" for the full, current list — this file only tracks non-passing ones, it is not an inventory). One failure remains: a consistent, pre-existing DSP issue unrelated to this session's report-driven fixes.

While getting these numbers, this pass also found and fixed a real, unrelated bug: `monument_doppler_shift_test` and `monument_spatial_dsp_test` were not previously being exercised at all, and the reason had nothing to do with CMake artifact paths (as a prior version of this doc claimed) — both test files called `SpatialProcessor::process()` with no arguments, while the actual method signature is `process(int numSamples)`. This was a stale test/API mismatch, not a build-configuration issue. Both files were fixed to pass the block size already established via `prepare()`; both compile and pass now.

---

## Consistent Failures (1)

### 1. monument_parameter_smoothing_test

**Status**: FAILED (2/50 parameter sweep sub-tests)

**Details**:
- Safety Clip: transient -17.2 dB, 2 clicks detected
- Safety Clip Drive: transient -14.2 dB, 0 clicks (but transient exceeds threshold)
- All other 48 parameters pass cleanly

**Root cause**: Parameter smoothing implementation issue specific to the safety-clip parameters (unrelated to the smoothing fixes this session made elsewhere — Chambers/ModulationMatrix RNG determinism, allocation fixes, etc. do not touch this path).

**Impact**: LOW — Safety clip is an advanced/optional feature, not part of the core reverb signal path.

---

## Previously Flaky, Not Reproduced This Run

`monument_modulation_matrix_test` and `monument_performance_benchmark` were previously observed to fail only as part of the full 32-test suite (passing cleanly in isolation), pointing at test-order interference or Debug-build timing sensitivity rather than a real defect. Both passed cleanly in this run's full-suite pass too (Release config). No code in either area was touched by this pass's fix — this is a further data point that they're environmental/build-config sensitivity, not something newly fixed here. If this recurs in a Debug build or under load, capture `ctest --output-on-failure -R <name>` output from the full-suite run specifically (not an isolated re-run) before treating it as actionable.

---

## Fixed This Pass: Stereo Decorrelation

`monument_reverb_dsp_test`'s Stereo Decorrelation sub-test fed an identical mono impulse to both L and R and measured 0.987946 correlation in the tail (expected < 0.95) — a real, reproducible FDN design weakness, not test noise. This threshold had already been relaxed once before (from < 0.5, per git history) specifically to paper over this same issue; even the relaxed threshold eventually failed.

**Root cause**: with a mono input, `Chambers`' 8-line FDN injects a single scalar signal (only sign-alternated per line, `kInputMid`) through `kMatrixHadamard` — the default (`warp = 0`) feedback matrix, chosen for smooth mono diffusion because it fully mixes all 8 lines together every iteration. That same full-mixing property homogenizes the 8 lines' content toward each other over time, so L and R — being different static per-line pan-weighted sums (`kOutputLeft`/`kOutputRight`) of those increasingly-similar lines — end up highly correlated too, regardless of how the pan weights are chosen. This is a structural FDN tradeoff, not a tunable-constant bug.

**Fix**: a small, fixed, real-time-safe stereo-decorrelation stage — two `AllpassDiffuser` instances (`outputDecorrelators`), one per channel, with distinct incommensurate delay/coefficient pairs, applied to the *live* wet signal only, before the live/frozen blend. Being phase-only, it doesn't measurably affect RT60, frequency response, or DC offset. It's explicitly gated off during freeze (`!freezeActive`): the frozen tail converges toward a sustained, near-periodic resonance, and this allpass's group delay beats against that periodicity (and against `wetLiveL/R`'s own internal crossfade toward the frozen snapshot mid-ramp) to produce slow amplitude modulation over tens of seconds — this was caught because it broke Freeze Mode Stability's long-window RMS check on the first two attempts (applying unconditionally to the final blended wet signal, then applying to the live signal without the freeze gate), before landing on gating strictly by `!freezeActive`.

Result: 0.987946 → 0.859905 correlation (comfortable margin below the 0.95 threshold), with Freeze Mode Stability's max RMS unchanged to 6 decimal places (3.627546 dB vs. the pre-fix 3.627548 dB) and no change in the DSP QA harness's critical-suite result (6 pass / 2 warn / 0 fail, same pre-existing `monument_freeze_mode` warning at the same -87.79 dBFS value).

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

- The remaining Safety Clip smoothing failure is unrelated to this session's RT-safety/architecture fixes and predates them.
- The authoritative DSP QA gate is the `audio-dsp-qa-harness` scenario suites (see `TESTING.md`'s "DSP QA Authority Policy"), not this CTest inventory — CTest failures here don't block CI merges, and a green CTest run doesn't substitute for the harness suites either.
- For the current, full CTest target list (all 32), see `TESTING.md`'s "CTest Inventory" section rather than this file — this file is a failures log, not an inventory.
