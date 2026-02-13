# Monument Reverb — Feature Checklist Evaluation

**Date**: 2026-02-12
**Session**: B (Feature Adoption + Threshold Tuning)
**Reference**: audio-dsp-qa-harness `PLUGIN_MIGRATION_REFERENCE.md` Section 3

Evaluation of ALL harness features for monument-reverb applicability.

---

## 3.1 Scenario Authoring Features

### `dsp_parameters` / `parameter_variations`

**Status**: ADOPTED
**Scenarios**: All 19 scenarios use `parameter_variations` (index-based)
**Notes**: Monument uses index-based mapping (0-14). All 15 core parameters mapped in adapter. Parameter names not yet implemented in adapter's `getParameterName()`.

---

### `parameter_automation` (LFO/Envelope/Ramp)

**Status**: ADOPTED
**Scenarios**: 3 scenarios
- `reverb_modulation.json` — LFO on warp (index 7), sine 0.5 Hz, range 0.2-0.8
- `reverb_time_ramp.json` — Ramp on time (index 1), 0.1 → 0.9
- `reverb_air_envelope.json` — Envelope on air (index 5), 5-point envelope
**Notes**: Monument's warp and drift parameters create modulation. LFO validates modulation stability. Ramp validates smooth parameter transitions. Envelope validates ADSR-like damping changes.

---

### `default_parameters` (suite-level)

**Status**: ADOPTED
**Notes**: Both `monument_suite.json` and `monument_critical_suite.json` declare `default_parameters` with sample_rate=48000, buffer_size=512, channels=2.

---

### `parameter_sweep` (cartesian product)

**Status**: ADOPTED (manual sweep, not cartesian)
**Scenarios**: 6 manual sweep scenarios
- Time sweep: time=0.1, 0.5, 1.0 (3 scenarios)
- Mass sweep: mass=0.1, 0.5, 0.9 (3 scenarios)
**Notes**: Manual sweep preferred over cartesian for monument — tests specific DSP behaviors at known operating points. Cartesian product of time × mass × density would generate 27+ scenarios which is excessive for current needs.

---

### `multi_pass`

**Status**: NOT_APPLICABLE
**Rationale**: Monument is a true reverb, not a looper/overdub effect. Multi-pass processing doesn't model any real monument-reverb usage pattern.
**Exception considered**: Freeze/unfreeze cycle could use multi-pass (pass 1: freeze on, pass 2: freeze off). Deferred — current freeze_mode.json validates freeze behavior adequately with single-pass.

---

### `stimulus_matrix` (multi-stimulus)

**Status**: DEFERRED
**Rationale**: Could create multi-stimulus variant of reverb_core.json with impulse + noise + piano_model inputs. All current scenarios use focused stimulus types (noise burst for RT60, white noise for spectral, impulse for smoke test). Adding stimulus_matrix would improve robustness validation but is not critical for Phase 3+4.
**Future**: Consider for Phase 5 validation if stimulus-dependent regressions are observed.

---

### `test_seed`

**Status**: DEFERRED
**Rationale**: Monument is deterministic given the same input/parameters/sample_rate. Current scenarios use `seed: 42` in stimulus parameters for reproducibility. Formal `test_seed` validation would confirm bit-exact output across runs but is lower priority than spectral/spatial metrics.
**Future**: Add 1 determinism scenario in Phase 5 if cross-platform consistency becomes a concern.

---

### `preset_scan`

**Status**: NOT_APPLICABLE
**Rationale**: Monument has 8+ factory presets, tested individually via 4 preset scenarios (cathedral, spring, infinite_abyss, elastic_hall). Individual scenarios allow preset-specific thresholds and metrics. Formal `preset_scan` automates iteration but loses per-preset metric granularity.

---

### `configurations`

**Status**: NOT_APPLICABLE
**Rationale**: Monument doesn't have discrete modes/configurations in the harness sense. All behavior is controlled via continuous parameters. Reverb character varies continuously with time/mass/density/bloom rather than switching between distinct modes.

---

## 3.2 Analysis & Metrics

### Tier A Metrics (ALWAYS use)

| Metric | Status | Scenarios | Notes |
|--------|--------|-----------|-------|
| `non_finite` | ADOPTED | All 19 (hard_fail) | Auto-applied default + explicit in every scenario |
| `peak_level` | ADOPTED | All 19 (soft_warn) | Auto-applied default + explicit in every scenario |
| `signal_present` | NOT USED | 0 | Reverb always produces output; peak_level sufficient |

---

### Tier B Audio Metrics

| Metric | Status | Scenarios | Notes |
|--------|--------|-----------|-------|
| `rms_energy` | ADOPTED | 3 (freeze, modulation, air_envelope) | Validates sustained energy in freeze/modulation |
| `monotonic_tail_decay` | ADOPTED | 1 (reverb_core) | Validates reverb tail decays monotonically |
| `discontinuity_count` | NOT USED | 0 | Monument doesn't produce audible discontinuities; threshold_preset provides defaults if added later |
| `energy_growth` | ADOPTED | 2 (freeze, infinite_abyss) | Critical for freeze stability and long-tail safety |
| `dc_offset` | NOT USED | 0 | No DC accumulation observed in testing |
| `initial_silence_duration` | NOT USED | 0 | Monument has minimal latency; not a concern |

---

### Tier B Spectral Metrics

| Metric | Status | Scenarios | Notes |
|--------|--------|-----------|-------|
| `spectral_centroid` | ADOPTED | 8 | Validates mass/air damping behavior |
| `spectral_flatness` | ADOPTED | 2 (reverb_core, preset_spring) | Validates diffusion character |
| `crest_factor` | ADOPTED | 3 (reverb_core, infinite_abyss, time_ramp) | Validates dynamic character of reverb tail |
| `dynamic_range` | ADOPTED | 1 (infinite_abyss) | Validates wide dynamics in extreme reverb |
| `flux_rate` | ADOPTED | 1 (reverb_modulation) | Validates warp LFO produces spectral change |
| `correlation_lr` | ADOPTED | 1 (spatial_width) | Validates FDN decorrelation |
| `spatial_width` | ADOPTED | 2 (spatial_width, elastic_hall) | Validates width parameter effect |

---

### Tier B Reverb Metrics

| Metric | Status | Scenarios | Notes |
|--------|--------|-----------|-------|
| `rt60` | ADOPTED | 8 | Core reverb metric — measures decay time via Schroeder integration |
| `edt` | NOT USED | 0 | EDT returned unreliable values in initial testing; preset provides defaults. Could be added to reverb_core if harness EDT measurement is refined. |
| `clarity_c50` | ADOPTED | 2 (reverb_core, cathedral) | Speech clarity ratio — validates reverberant character |
| `clarity_c80` | ADOPTED | 2 (reverb_core, cathedral) | Music clarity ratio — validates reverberant character |

---

### Tier B Spatial Metrics

| Metric | Status | Scenarios | Notes |
|--------|--------|-----------|-------|
| `iacc_early` | ADOPTED | 1 (spatial_width) | Early interaural cross-correlation — measured 0.049 |
| `iacc_late` | ADOPTED | 1 (spatial_width) | Late interaural cross-correlation — measured 0.008 |
| `itd_max_ms` | ADOPTED | 1 (spatial_width) | Interaural time difference — measured 0.375 ms |
| `ild_max_db` | ADOPTED | 1 (spatial_width) | Interaural level difference — measured 0.002 dB |

---

### Performance Metrics

| Metric | Status | Scenarios | Notes |
|--------|--------|-----------|-------|
| `perf_median_block_time_ms` | ADOPTED | 1 (performance_profile) | Median block processing time |
| `perf_p95_block_time_ms` | ADOPTED | 1 (performance_profile) | 95th percentile block time |
| `perf_p99_block_time_ms` | ADOPTED | 1 (performance_profile) | 99th percentile block time |
| `perf_allocation_count` | ADOPTED | 1 (performance_profile) | RT-safety: zero heap allocations (hard_fail) |

---

### Frequency Response

**Status**: ADOPTED
**Scenarios**: 1 (frequency_response_air.json)
**Notes**: Measures transfer function H(f) = FFT(wet)/FFT(dry) using log sine sweep. Air=0.1 heavily damps HF. Current thresholds use -120 dB baseline with ±40 dB tolerance (reverb attenuates sweep significantly vs. bypass). Spectral centroid validates overall HF rolloff.

---

### Visual Regression (Spectrogram)

**Status**: DEFERRED
**Rationale**: Visual regression via spectrogram comparison is a nice-to-have for monument-reverb. Current spectral metrics (centroid, flatness, crest_factor) provide quantitative validation. Spectrogram diff would add visual confirmation but requires baseline capture infrastructure.
**Future**: Consider adding for cathedral and elastic_hall presets if visual regression becomes a priority.

---

## 3.3 Infrastructure Features

### Baseline Tracking

**Status**: ADOPTED
**Scenarios**: All 19 scenarios have `baseline_tracking: { "enabled": true }`
**Notes**: Enables regression detection across builds. Baselines captured from initial run.

---

### Auto-Discovery (`--discover`)

**Status**: ADOPTED
**Notes**: `./monument_qa --discover scenarios/monument/` discovers and runs all 19 scenarios. Non-recursive discovery covers the single scenario directory.

---

### Result Export

**Status**: ADOPTED (automatic)
**Notes**: TestSuiteExecutor auto-exports `result.json` per scenario in qa_output/. Used for threshold tuning workflow.

---

### Threshold Auto-Tuning

**Status**: ADOPTED (manual workflow)
**Notes**: Used `result.json` measured values to manually tune thresholds. Did not use the `threshold_autotune` CLI tool — manual tuning provided better control over architecture-specific tolerances. Rationale documented in MONUMENT_THRESHOLD_TUNING_LOG.md.

---

### Architecture-Specific Threshold Presets (HO-13)

**Status**: ADOPTED
**Preset**: `reverb_architecture`
**Scenarios**: All 18 reverb-category scenarios + smoke_test
**Notes**: Preset provides sensible defaults for RT60 (0.2-10s), EDT (0.1-5s), clarity (C50/C80), IACC, and discontinuity detection threshold. Scenario-level thresholds override preset where specified.

---

## Summary

| Category | ADOPTED | NOT_APPLICABLE | DEFERRED | Total |
|----------|---------|----------------|----------|-------|
| Authoring Features | 4 | 3 | 2 | 9 |
| Tier A Metrics | 2 | 0 | 0 | 2 |
| Tier B Audio Metrics | 3 | 0 | 0 | 3 |
| Tier B Spectral Metrics | 7 | 0 | 0 | 7 |
| Tier B Reverb Metrics | 3 | 0 | 1 (edt) | 4 |
| Tier B Spatial Metrics | 4 | 0 | 0 | 4 |
| Performance Metrics | 4 | 0 | 0 | 4 |
| Frequency Response | 1 | 0 | 0 | 1 |
| Visual Regression | 0 | 0 | 1 | 1 |
| Infrastructure | 4 | 0 | 0 | 4 |
| **Total** | **32** | **3** | **4** | **39** |

**Unique metrics adopted**: 17 across 19 scenarios
**Parameter automation types**: 3 (LFO, Ramp, Envelope)
**Preset coverage**: reverb_architecture on all scenarios
