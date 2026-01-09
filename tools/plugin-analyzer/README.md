# Monument Plugin Analyzer

Automated testing and analysis tool for VST3/AU audio plugins. Captures impulse responses, frequency sweeps, and other test signals to measure reverb characteristics.

## Features

- **Plugin Loading**: Dynamically loads VST3/AU plugins using JUCE
- **Test Signal Generation**: Impulse, sine sweep, white/pink noise
- **Audio Capture**: Records dry (input) and wet (processed) signals
- **WAV Export**: High-quality 24-bit WAV files
- **RT60 Analysis**: Python integration with pyroomacoustics
- **Extensible**: Easy to add new analysis modules

## Quick Start

### 1. Build the Analyzer

```bash
# From monument-reverb root
cmake -B build -DMONUMENT_BUILD_ANALYZER=ON
cmake --build build --target monument_plugin_analyzer

# Binary location: ./build/monument_plugin_analyzer_artefacts/
```

### 2. Install Python Dependencies

```bash
cd tools/plugin-analyzer/python
pip3 install -r requirements.txt
```

### 3. Run Basic Analysis

```bash
# Capture impulse response from Monument
./build/monument_plugin_analyzer_artefacts/Debug/monument_plugin_analyzer \
  --plugin ./build/Monument_artefacts/VST3/Monument.vst3

# Output: ./test-results/dry.wav and ./test-results/wet.wav
```

### 4. Analyze RT60

```bash
python3 tools/plugin-analyzer/python/rt60_analysis_robust.py ./test-results/wet.wav
```

## Usage

### Command-Line Options

```bash
monument_plugin_analyzer [options]

Required:
  --plugin <path>         Path to VST3/AU plugin

Optional:
  --output <dir>          Output directory (default: ./test-results)
  --test <type>           Test type: impulse|sweep|noise|pink (default: impulse)
  --duration <seconds>    Test duration (default: 5.0)
  --samplerate <hz>       Sample rate (default: 48000)
  --channels <num>        Number of channels (default: 2)
```

### Examples

#### Capture Impulse Response
```bash
monument_plugin_analyzer --plugin Monument.vst3
```

#### Capture Sine Sweep (for frequency response)
```bash
monument_plugin_analyzer \
  --plugin Monument.vst3 \
  --test sweep \
  --duration 10
```

#### Capture White Noise
```bash
monument_plugin_analyzer \
  --plugin Monument.vst3 \
  --test noise \
  --duration 5
```

#### Custom Sample Rate and Channels
```bash
monument_plugin_analyzer \
  --plugin Monument.vst3 \
  --samplerate 96000 \
  --channels 2 \
  --output ./my-tests
```

## Analysis Tools

### RT60 Analysis (Python)

```bash
python3 python/rt60_analysis_robust.py <impulse_response.wav> [options]

Options:
  --output <file>    Save metrics to JSON
  --no-plot          Skip plot generation

Examples:
  # Analyze with plot
  python3 python/rt60_analysis_robust.py wet.wav

  # Save metrics to JSON
  python3 python/rt60_analysis_robust.py wet.wav --output metrics.json
```

**Output:**
- Console: RT60 values per frequency band
- JSON: Structured metrics for CI/testing
- PNG: Impulse response and energy decay curve visualization

### Frequency Response Analysis (TODO)

Coming soon:
- FFT analysis of sine sweep
- Frequency response plot
- Phase response

### Stereo Width Analysis (TODO)

Coming soon:
- L/R decorrelation measurement
- Stereo field visualization

## Architecture

```
tools/plugin-analyzer/
├── src/
│   ├── main.cpp                    # CLI entry point
│   ├── PluginLoader.{h,cpp}        # VST3/AU loading
│   ├── TestSignalGenerator.{h,cpp} # Signal generation
│   └── AudioCapture.{h,cpp}        # WAV export
├── python/
│   ├── rt60_analysis_robust.py     # RT60 calculation (with fallbacks)
│   ├── frequency_response.py       # Frequency response analysis
│   └── requirements.txt
└── README.md
```

### Key Classes

- **PluginLoader**: JUCE AudioPluginFormatManager wrapper
- **TestSignalGenerator**: Pure functions for test signals
- **AudioCapture**: Streaming audio recorder with WAV export

## Development

### Adding New Test Signals

```cpp
// In TestSignalGenerator.h
static juce::AudioBuffer<float> generateMySignal(
    double durationSeconds,
    double sampleRate,
    int numChannels = 2);

// In TestSignalGenerator.cpp
juce::AudioBuffer<float> TestSignalGenerator::generateMySignal(...)
{
    // Implementation
}
```

### Adding New Analysis Modules

1. Create Python script in `python/`
2. Accept WAV file as input
3. Output JSON metrics to stdout
4. Document usage in README

## Workflow Integration

### Testing Monument Presets

```bash
#!/bin/bash
# test_all_presets.sh

PRESETS=("Small Room" "Cathedral" "Plate")

for preset in "${PRESETS[@]}"; do
  echo "Testing preset: $preset"
  monument_plugin_analyzer \
    --plugin Monument.vst3 \
    --preset "$preset" \
    --output "./results/$preset"

  python3 python/rt60_analysis_robust.py \
    "./results/$preset/wet.wav" \
    --output "./results/$preset/metrics.json"
done
```

### CI Integration

```yaml
# .github/workflows/audio-tests.yml
- name: Test Monument Plugin
  run: |
    ./build/monument_plugin_analyzer --plugin Monument.vst3
    python3 python/rt60_analysis_robust.py wet.wav --output metrics.json

- name: Verify RT60
  run: |
    # Parse metrics.json and check thresholds
    python3 scripts/verify_metrics.py metrics.json
```

## Troubleshooting

### "Failed to load plugin"

- Check plugin path is correct
- Ensure plugin is built (check `./build/Monument_artefacts/`)
- On macOS: Plugin must be in `.vst3` or `.component` format
- Try absolute path instead of relative

### "No compatible plugin format found"

- Rebuild Monument with plugin formats enabled
- Check CMakeLists.txt: `FORMATS AU VST3 Standalone`

### Python RT60 errors

```bash
# Install missing dependencies
pip3 install numpy scipy pyroomacoustics matplotlib

# Check installation
python3 -c "import pyroomacoustics; print(pyroomacoustics.__version__)"
```

### Empty or Silent WAV files

- Verify plugin processes audio (test in DAW first)
- Check plugin's dry/wet mix is not 0%
- Ensure input signal amplitude is sufficient

## Future Enhancements

### Phase 2: Advanced Metrics
- [ ] Frequency response analyzer
- [ ] THD (Total Harmonic Distortion)
- [ ] Phase response
- [ ] Stereo width/decorrelation
- [ ] CPU usage profiling

### Phase 3: Automation
- [ ] Preset scanner (test all presets)
- [ ] Parameter sweep (test ranges)
- [ ] Regression testing (compare builds)
- [ ] Visual reports (HTML dashboard)

### Phase 4: Extract to Separate Repo
- [ ] Generic plugin analyzer framework
- [ ] Plugin-agnostic
- [ ] Reusable for other projects

## Contributing

This tool is currently Monument-specific but designed for extraction.

**To contribute:**
1. Keep PluginLoader generic (no Monument hardcoding)
2. Add test signals to TestSignalGenerator
3. Create standalone Python analyzers in `python/`
4. Document usage in this README

## License

Same as Monument (TBD)

## References

- [JUCE AudioPluginFormatManager](https://docs.juce.com/master/classAudioPluginFormatManager.html)
- [pyroomacoustics RT60](https://github.com/LCAV/pyroomacoustics)
- [pluginval](https://github.com/Tracktion/pluginval) - Stability testing
- [PluginDoctor](https://ddmf.eu/plugindoctor/) - Commercial analysis tool
