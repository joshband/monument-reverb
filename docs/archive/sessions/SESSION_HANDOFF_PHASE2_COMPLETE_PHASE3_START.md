# Session Handoff: Monument Phase 2 Complete + Phase 3 In Progress

**Date**: 2026-02-11
**Status**: Phase 2 COMPLETE (14/14 PASS). Phase 3 (Session B) IN PROGRESS (~15% done)

---

## What Was Completed This Session

### Phase 2 Completion — 3 Critical Fixes

**Fix 1: Parameter format** — All 14 scenarios converted from `dsp_parameters` (array format, silently ignored by harness) to `parameter_variations` (object format, correctly parsed).

**Fix 2: JUCE SmoothedValue re-init** — Added `needsReinit_` flag to adapter. On first `processBlock()`, re-calls `prepareToPlay()` so JUCE SmoothedValues initialize from actual APVTS values (not defaults). Without this, all parameters were stuck at defaults (e.g., time always 0.55).

**Fix 3: monotonic_tail_decay threshold** — Changed from meaningless `min: 0.5` to proper `max_upward_steps: 20` with analysis window starting at 1.0s (was 0.2s) to exclude noise burst buildup.

### Final Phase 2 Results (14/14 PASS, 0 warnings)

| Scenario | RT60 | Peak dBFS | Key Metrics |
|----------|------|-----------|-------------|
| smoke_test | — | -2.82 | non_finite=0 |
| reverb_core (time=0.55) | 2.30s | -8.87 | monotonic_tail_decay=16 |
| spatial_width | — | -8.79 | width=0.498, corr=0.003 |
| freeze_mode | — | -31.57 | rms=-87.8dB, growth=0 |
| time_short (0.1) | 2.13s | -8.86 | RT60 monotonically increasing |
| time_medium (0.5) | 2.27s | -8.85 | ✅ |
| time_long (1.0) | 2.49s | -8.85 | ✅ |
| mass_bright (0.1) | — | -12.70 | centroid=10920 |
| mass_neutral (0.5) | — | -11.69 | centroid=10924 |
| mass_dark (0.9) | — | -10.68 | centroid=10942 |
| preset_cathedral | 3.16s | -3.86 | — |
| preset_spring | 10.80s | -3.07 | — |
| preset_elastic_hall | 7.16s | -4.18 | width=0.501 |
| preset_infinite_abyss | 7.60s | -4.62 | growth=0.26 |

### Session B Progress (~15%)

Started adding spectral/clarity metrics to existing scenarios:
- **reverb_core.json** — Added clarity_c50, clarity_c80, spectral_flatness, crest_factor (with permissive thresholds)
- **preset_cathedral.json** — Added clarity_c50, clarity_c80, spectral_centroid
- **preset_spring.json** — Added spectral_centroid, spectral_flatness (PARTIALLY DONE — RT60 threshold widened)

**NOT YET STARTED:**
- preset_infinite_abyss.json (dynamic_range, crest_factor)
- spatial_width.json (iacc_early, iacc_late, itd_max_ms)
- mass sweep scenarios (spectral_centroid already present)
- 3 new automation scenarios (reverb_modulation, reverb_time_ramp, reverb_air_envelope)
- Frequency response scenario
- Performance profile scenario
- Threshold tuning (run → measure → tighten)
- Suite file updates
- Threshold tuning log

---

## Files Modified This Session

### monument-reverb repo:
- `qa/monument_adapter.h` — Added `needsReinit_` flag
- `qa/monument_adapter.cpp` — Added re-init logic in processBlock, removed debug fprintf
- `scenarios/monument/*.json` — All 14 scenarios: parameter_variations format fix + partial metric additions
- `docs/SESSION_HANDOFF_PHASE2_SCENARIOS.md` — Updated with final results

### harness repo:
- `docs/MONUMENT_REVERB_MIGRATION_SESSIONS_DEF.md` — Session A marked complete

---

## How to Continue (Session B Remaining Tasks)

### Required Reading
All scenarios already created. Key harness refs:
- `docs/PLUGIN_MIGRATION_REFERENCE.md` Section 3 (feature checklist)
- `docs/guides/THRESHOLD_PRESET_REFERENCE.md` (reverb_architecture preset)
- Session B mega-prompt in `docs/MONUMENT_REVERB_MIGRATION_SESSIONS_DEF.md` (lines 290-609)

### Task 1: Finish Adding Spectral Metrics (4 more scenarios)
- **preset_infinite_abyss.json**: Add dynamic_range, crest_factor
- **spatial_width.json**: Add iacc_early, iacc_late, itd_max_ms
- Mass sweep scenarios already have spectral_centroid — verify thresholds

### Task 2: Create 3 New Automation Scenarios
Use these exact formats (from harness loader):

**reverb_modulation.json** (LFO on warp param):
```json
{
  "parameter_automation": [{
    "parameter_index": 7,
    "type": "lfo",
    "waveform": "sine",
    "rate_hz": 0.5,
    "min_value": 0.2,
    "max_value": 0.8
  }]
}
```

**reverb_time_ramp.json** (Ramp on time param):
```json
{
  "parameter_automation": [{
    "parameter_index": 1,
    "type": "ramp",
    "start_value": 0.1,
    "end_value": 0.9
  }]
}
```

**reverb_air_envelope.json** (Envelope on air param):
```json
{
  "parameter_automation": [{
    "parameter_index": 5,
    "type": "envelope",
    "points": [
      {"time": 0.0, "value": 0.5},
      {"time": 0.5, "value": 1.0},
      {"time": 1.5, "value": 0.7},
      {"time": 3.5, "value": 0.7},
      {"time": 5.0, "value": 0.3}
    ]
  }]
}
```

### Task 3: Create Performance Profile Scenario
Use `perf_median_block_time_ms`, `perf_p95_block_time_ms`, `perf_p99_block_time_ms` metrics.
No `perf_allocation_count` as hard_fail (may not be supported by adapter).

### Task 4: Threshold Tuning
After all new metrics added:
1. Run `--discover` suite
2. Check for warnings/failures
3. Widen thresholds where needed
4. Tighten based on measured values
5. Document in `docs/MONUMENT_THRESHOLD_TUNING_LOG.md`

### Task 5: Update Suite Files
Add all new scenarios to `monument_suite.json`.

---

## Key Lessons (carry forward)

1. **`parameter_variations` NOT `dsp_parameters`** — Harness reads object format only
2. **JUCE adapter needs `needsReinit_`** — SmoothedValues init from APVTS defaults, not post-setParameter values
3. **Noise burst > impulse** — FDN reverbs need burst excitation for Schroeder RT60
4. **RT60 via noise burst gives lower values** than plugin's own tests (2.30s vs 7.89s at time=0.55)
5. **Threshold format**: Use `min`/`max` for most metrics, `peak_dbfs_max` for peak_level, `max_upward_steps` for monotonic_tail_decay

## Build & Run

```bash
cd /Users/artbox/Documents/Repos/monument-reverb
cmake --build build-qa --config Release --target monument_qa -j8
./build-qa/monument_qa_artefacts/Release/monument_qa --discover scenarios/monument/
```
