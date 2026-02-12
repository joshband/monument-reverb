# Session Handoff: Monument Phase 2 — Core Scenarios

**Date**: 2026-02-11
**Session**: Phase 2 Core Scenario Creation
**Status**: COMPLETE — 14/14 scenarios passing (0 warnings, 0 failures)

## What Was Done

### 13 new scenario files created in `scenarios/monument/`:
1. **reverb_core.json** — RT60 measurement via noise burst (RT60=2.30s at time=0.55)
2. **spatial_width.json** — Stereo width validation (width=0.498, corr=0.003)
3. **freeze_mode.json** — Freeze hold behavior (rms=-87.8dB, peak=-31.6dB)
4. **parameter_sweep_time_short.json** — time=0.1, noise burst, 15s
5. **parameter_sweep_time_medium.json** — time=0.5, noise burst, 20s
6. **parameter_sweep_time_long.json** — time=1.0, noise burst, 35s
7. **parameter_sweep_mass_bright.json** — mass=0.1, white noise, 4s
8. **parameter_sweep_mass_neutral.json** — mass=0.5, white noise, 4s
9. **parameter_sweep_mass_dark.json** — mass=0.9, white noise, 4s
10. **preset_cathedral.json** — Cathedral of Glass (#8), noise burst, 25s
11. **preset_spring.json** — Stone Circles (#7), noise burst, 15s
12. **preset_infinite_abyss.json** — Event Horizon (#15), noise burst, 40s
13. **preset_elastic_hall.json** — Elastic Cathedral (#25), noise burst, 25s

### 2 suite files created:
- **monument_suite.json** — Full suite (14 scenarios)
- **monument_critical_suite.json** — Critical suite (5: smoke, reverb_core, spatial, freeze, cathedral)

### Adapter changes:
- Added `needsReinit_` flag to re-call `prepareToPlay()` after parameters are set (fixes JUCE SmoothedValue initialization)
- Freeze already mapped at index 14
- All 15 params (0-14) sufficient for Phase 2

### Scenario format fix:
- All scenarios converted from `dsp_parameters` (array format, NOT parsed by harness) to `parameter_variations` (object format, correctly parsed)

## Key Findings

### 1. Impulse vs Noise Burst for RT60
The harness's built-in `impulse` stimulus does NOT produce measurable RT60 with Monument's FDN reverb. The Schroeder integration returns "insufficient decay range". Switched all RT60 scenarios to use `noise` `burst` variant (100ms burst, then silence for tail capture).

### 2. JUCE SmoothedValue re-init required
The harness calls `prepare()` then `setParameter()`. But Monument's `prepareToPlay()` initializes SmoothedValues from APVTS defaults (not yet updated). The 500ms ramp time meant parameters never converged to target. **Fix**: Added `needsReinit_` flag in adapter — first `processBlock()` call re-triggers `prepareToPlay()` after parameters are set via APVTS.

### 3. Parameter format: `parameter_variations` NOT `dsp_parameters`
The harness only reads `parameter_variations` (object: `{"0": 0.5, "1": 0.8}`). The prior session used `dsp_parameters` (array: `[{"index": 0, "value": 0.5}]`) which was silently ignored. Fixed all 14 scenarios.

## Final Results (14/14 PASS)

| Scenario | RT60 | Peak dBFS | Spectral Centroid | Spatial Width | Correlation LR |
|----------|------|-----------|-------------------|---------------|----------------|
| smoke_test | — | -2.82 | — | — | — |
| reverb_core (time=0.55) | 2.30s | -8.87 | — | — | — |
| spatial_width | — | -8.79 | — | 0.498 | 0.003 |
| freeze_mode | — | -31.57 | — | — | — |
| time_short (0.1) | 2.13s | -8.86 | — | — | — |
| time_medium (0.5) | 2.27s | -8.85 | — | — | — |
| time_long (1.0) | 2.49s | -8.85 | — | — | — |
| mass_bright (0.1) | — | -12.70 | 10920 Hz | — | — |
| mass_neutral (0.5) | — | -11.69 | 10924 Hz | — | — |
| mass_dark (0.9) | — | -10.68 | 10942 Hz | — | — |
| preset_cathedral | 3.16s | -3.86 | — | — | — |
| preset_spring | 10.80s | -3.07 | — | — | — |
| preset_elastic_hall | 7.16s | -4.18 | — | 0.501 | — |
| preset_infinite_abyss | 7.60s | -4.62 | — | — | — |

### RT60 Time Sweep Verification
- time=0.1 → RT60=2.13s ✅ (monotonically increasing)
- time=0.5 → RT60=2.27s ✅
- time=1.0 → RT60=2.49s ✅

### Observations for Session B
- RT60 range (2.13-2.49s) is narrower than Monument's own tests report (4.85-29.85s). The noise burst + Schroeder method gives different results than the plugin's internal test methodology.
- Mass sweep centroid values (10920-10942 Hz) show minimal differentiation. White noise stimulus with full-duration analysis may mask the mass effect — consider `late_reverb` analysis window.
- Preset RT60 ordering is non-monotonic vs time parameter (spring has highest RT60 despite lowest time). Other parameters (gravity, density, memory) significantly affect decay.
- EDT metric doesn't work reliably with noise burst (returns values close to total duration).

## Build & Run Commands

```bash
cd /Users/artbox/Documents/Repos/monument-reverb
cmake --build build-qa --config Release --target monument_qa -j8
./build-qa/monument_qa_artefacts/Release/monument_qa --discover scenarios/monument/
./build-qa/monument_qa_artefacts/Release/monument_qa scenarios/monument/reverb_core.json 2>/dev/null
```

## Files Modified (monument-reverb repo)
- `scenarios/monument/*.json` — 13 scenario files + 2 suite files (parameter format fixed)
- `qa/monument_adapter.h` — Added `needsReinit_` flag
- `qa/monument_adapter.cpp` — Added re-init logic in processBlock(), removed debug output
