# Monument Reverb - Testing Infrastructure

Complete automated testing system for batch analyzing all 37 factory presets.

Note: The canonical testing hub is `TESTING.md` in the repo root. This file focuses on the audio regression tooling details.

## Quick Start

```bash
# 1. Capture all presets (15 seconds each, parallelized)
./scripts/capture_all_presets.sh

# 2. Analyze all captures (RT60 + frequency response + spatial metrics, parallelized)
./scripts/analyze_all_presets.sh

# 3. Generate comparison visualizations
python3 tools/plot_preset_comparison.py test-results/preset-baseline

# 4. View results
open test-results/comparisons/rt60_comparison.png
open test-results/comparisons/frequency_response_comparison.png
cat test-results/comparisons/summary_statistics.txt
```

## Features

### 🚀 Parallel Processing
- Auto-detects CPU cores (capped at 8 workers)
- Batch capture: ~2-3 minutes (vs 9+ minutes sequential)
- Batch analysis: ~1-2 minutes (vs 5+ minutes sequential)
- Override: `PARALLEL_JOBS=4 ./scripts/capture_all_presets.sh`

### 📊 Metrics Captured

**RT60 Analysis** ([tools/plugin-analyzer/python/rt60_analysis_robust.py](tools/plugin-analyzer/python/rt60_analysis_robust.py))
- Reverb decay time (60dB)
- Schroeder integration method
- Fallback to manual calculation if pyroomacoustics fails
- JSON output: `test-results/preset-baseline/preset_XX/rt60_metrics.json`

**Frequency Response** ([tools/plugin-analyzer/python/frequency_response.py](tools/plugin-analyzer/python/frequency_response.py))
- FFT-based spectrum analysis
- Octave band breakdown (Sub, Bass, Low-Mid, Mid, High-Mid, Presence, Brilliance)
- Overall flatness rating (±dB std deviation)
- JSON output: `test-results/preset-baseline/preset_XX/freq_metrics.json`
- PNG plot: `test-results/preset-baseline/preset_XX/frequency_response.png`

**Spatial Metrics** ([tools/plugin-analyzer/python/spatial_metrics.py](tools/plugin-analyzer/python/spatial_metrics.py))
- ITD (GCC-PHAT), ILD (RMS + band energy), IACC (normalized cross-correlation)
- Early-window analysis for spatial cues
- JSON output: `test-results/preset-baseline/preset_XX/spatial_metrics.json`

### 📈 Visualization Tools

**RT60 Comparison** ([tools/plot_preset_comparison.py](tools/plot_preset_comparison.py))
- Bar chart showing decay times for all 37 presets
- Color-coded by length (short/medium/long/very long)
- Reference lines at 1s, 3s, 6s

**Frequency Response Heatmap**
- Band-by-band comparison across all presets
- Flatness bar chart
- Quality distribution (Excellent/Good/Fair/Colored)

**Summary Statistics**
- Mean, median, std dev for RT60 and flatness
- Min/max values with preset indices
- Quality rating distribution

### 🔬 Regression Testing

**Compare Baseline vs Current** ([tools/compare_baseline.py](tools/compare_baseline.py))
```bash
# Capture baseline (before code changes)
./scripts/capture_all_presets.sh
mv test-results/preset-baseline test-results/baseline-v1.0.0

# After DSP modifications, capture current state
./scripts/capture_all_presets.sh

# Compare
python3 tools/compare_baseline.py \
  test-results/baseline-v1.0.0 \
  test-results/preset-baseline \
  --threshold 0.05 \
  --output regression-report.json
```

**Comparison Metrics:**
- RT60 change (% difference)
- Frequency flatness change (dB difference)
- Spatial metrics deltas (ITD ms, ILD dB, IACC)
- Waveform correlation (should be > 0.95)
- RMS waveform difference
- Spectral difference (dB)

**Exit Codes:**
- `0` = All presets passed (no regressions)
- `1` = One or more presets failed (regression detected)

## File Structure

```
test-results/
└── preset-baseline/
    ├── preset_00/
    │   ├── wet.wav                    # Processed audio (24-bit)
    │   ├── dry.wav                    # Dry input signal
    │   ├── rt60_metrics.json          # Decay time metrics
    │   ├── freq_metrics.json          # Frequency response data
    │   ├── spatial_metrics.json       # ITD/ILD/IACC metrics
    │   ├── frequency_response.png     # Spectrum plot
    │   ├── metadata.json              # Capture info
    │   ├── capture.log                # Analyzer output
    │   ├── rt60_analysis.log          # RT60 analysis log
    │   ├── freq_analysis.log          # Frequency analysis log
    │   └── spatial_analysis.log       # Spatial analysis log
    ├── preset_01/
    │   └── ...
    └── preset_36/
        └── ...

test-results/comparisons/
├── rt60_comparison.png                # RT60 bar chart
├── frequency_response_comparison.png  # Frequency heatmap
└── summary_statistics.txt             # Text summary
```

## Configuration

### Capture Script
- **Duration:** 15 seconds (configurable at top of script)
- **Plugin Path:** `~/Library/Audio/Plug-Ins/VST3/Monument.vst3`
- **Output:** `./test-results/preset-baseline`
- **Parallel Jobs:** Auto-detect (max 8)

### Analysis Scripts
- **RT60 Method:** Schroeder backward integration
- **Frequency Range:** 20 Hz - 20 kHz
- **FFT Window:** Hann window, 4096 samples
- **Octave Bands:** 7 bands (Sub to Brilliance)
- **Spatial Window:** 80 ms (early), max lag 1.0 ms

### Regression Thresholds
- **RT60:** ±5% change (default)
- **Frequency:** ±0.5dB flatness change
- **Waveform Correlation:** > 0.95
- **RMS Difference:** < 0.05

## Dependencies

Python packages (auto-installed if missing):
```bash
pip3 install numpy scipy matplotlib pyroomacoustics
```

## CI Integration Example

```yaml
# .github/workflows/audio-regression-tests.yml
name: Audio Quality Tests

on: [push, pull_request]

jobs:
  test-audio:
    runs-on: macos-latest
    steps:
      - uses: actions/checkout@v3

      - name: Install dependencies
        run: pip3 install numpy scipy matplotlib pyroomacoustics

      - name: Build Monument
        run: ./scripts/rebuild_and_install.sh all

      - name: Capture baseline (cached)
        uses: actions/cache@v3
        with:
          path: test-results/baseline
          key: audio-baseline-${{ github.base_ref }}

      - name: Capture current state
        run: ./scripts/capture_all_presets.sh

      - name: Analyze captures
        run: ./scripts/analyze_all_presets.sh

      - name: Compare vs baseline
        run: |
          python3 tools/compare_baseline.py \
            test-results/baseline \
            test-results/preset-baseline \
            --output regression-report.json

      - name: Upload artifacts
        if: failure()
        uses: actions/upload-artifact@v3
        with:
          name: regression-report
          path: |
            regression-report.json
            test-results/preset-baseline/*/
```

## Performance

**On 8-core M1 Mac:**
- Sequential capture: ~9.5 minutes (37 × 15s)
- Parallel capture (8 workers): ~2-3 minutes (4.5x speedup)
- Sequential analysis: ~5 minutes
- Parallel analysis (8 workers): ~1-2 minutes (3-4x speedup)
- **Total pipeline: ~4-5 minutes** (vs 15+ minutes sequential)

## Troubleshooting

**"Plugin analyzer not found"**
```bash
./scripts/rebuild_and_install.sh monument_plugin_analyzer
```

**"Monument plugin not found"**
```bash
./scripts/rebuild_and_install.sh Monument
```

**"pyroomacoustics import error"**
```bash
pip3 install pyroomacoustics
# Or continue anyway - script has fallback RT60 calculation
```

**Capture/analysis failures**
- Check individual logs in `test-results/preset-baseline/preset_XX/*.log`
- Common issue: Plugin state not initialized (rare)
- Solution: Re-run capture for specific preset

**Some presets timeout or hang**
- Disable parallel processing: `PARALLEL_JOBS=1 ./scripts/capture_all_presets.sh`
- Reduce duration in capture script if needed

## Next Steps

1. **Commit baseline** to git or archive:
   ```bash
   tar -czf baseline-v1.0.0.tar.gz test-results/preset-baseline/
   ```

2. **Generate documentation** for all presets:
   ```bash
   python3 tools/generate_preset_report.py  # TODO: Create this script
   ```

3. **Set up CI/CD** with GitHub Actions (see example above)

4. **Profile CPU usage** during batch capture:
   ```bash
   instruments -t "Time Profiler" -D trace.trace \
     ./build/monument_plugin_analyzer_artefacts/Debug/monument_plugin_analyzer \
     --plugin ~/Library/Audio/Plug-Ins/VST3/Monument.vst3 \
     --preset 0 --duration 15
   ```
   For Ninja builds, swap `build/` with `build-ninja/` or set `BUILD_DIR`.

---

**Status:** ✅ Infrastructure complete and tested
**Ready for:** Production use, CI integration, regression testing
