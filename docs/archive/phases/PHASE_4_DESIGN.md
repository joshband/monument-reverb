# Phase 4: Enhanced Testing Infrastructure - Design Document

**Created:** 2026-01-08
**Status:** Design Phase
**Goal:** Unified analyzer + comprehensive test coverage for production readiness

---

## Overview

Phase 4 enhances Monument's testing infrastructure with:

1. **Unified Plugin Analyzer** - Single command for capture + analysis
2. **Parameter Smoothing Test** - Click/pop detection
3. **Stereo Width Test** - Spatial correctness validation
4. **Latency & Phase Test** - DAW compatibility (PDC)
5. **State Save/Recall Test** - Automation compatibility

---

## 1. Unified Plugin Analyzer Architecture

### Current Architecture (Fragmented)

```bash
# Step 1: Capture audio (C++ tool)
./build/monument_plugin_analyzer --plugin Monument.vst3 --preset 0

# Step 2: Analyze RT60 (Python)
python3 tools/plugin-analyzer/python/rt60_analysis_robust.py test-results/wet.wav

# Step 3: Analyze frequency (Python)
python3 tools/plugin-analyzer/python/frequency_response.py test-results/wet.wav

# Step 4: Check results manually
cat test-results/rt60_metrics.json
```

**Problems:**
- 3-4 separate commands
- No atomic success/failure
- Easy to forget analysis steps
- Manual result checking

### Proposed Architecture (Unified)

```bash
# Single command - captures AND analyzes
./build/monument_plugin_analyzer \
  --plugin Monument.vst3 \
  --preset 0 \
  --analyze all \
  --output test-results/preset_00

# Exit code: 0 = pass, 1 = fail (analysis included)
```

**Benefits:**
- Single command workflow
- Atomic success/failure reporting
- Integrated error handling
- CI/CD ready

### Implementation Options

#### Option A: C++ Subprocess (Recommended)

Add Python subprocess calls to C++ tool:

```cpp
// main.cpp
bool runPythonAnalysis(const juce::File& audioFile, const juce::File& outputDir)
{
    // RT60 Analysis
    juce::String rt60Cmd = "python3 tools/plugin-analyzer/python/rt60_analysis_robust.py "
                         + audioFile.getFullPathName()
                         + " --output " + outputDir.getFullPathName();

    int rt60Result = std::system(rt60Cmd.toRawUTF8());
    if (rt60Result != 0) {
        std::cerr << "RT60 analysis failed with code " << rt60Result << "\n";
        return false;
    }

    // Frequency Analysis
    juce::String freqCmd = "python3 tools/plugin-analyzer/python/frequency_response.py "
                         + audioFile.getFullPathName()
                         + " --output " + outputDir.getFullPathName();

    int freqResult = std::system(freqCmd.toRawUTF8());
    if (freqResult != 0) {
        std::cerr << "Frequency analysis failed with code " << freqResult << "\n";
        return false;
    }

    return true;
}
```

**Pros:**
- Simple implementation
- Reuses existing Python tools
- No Python binding complexity

**Cons:**
- Requires Python on target system
- Subprocess overhead (~100ms)

#### Option B: Shell Wrapper

Create `analyze_preset.sh` wrapper:

```bash
#!/bin/bash
# Captures audio + runs analysis

PLUGIN=$1
PRESET=$2
OUTPUT=$3

# Step 1: Capture
./build/monument_plugin_analyzer --plugin "$PLUGIN" --preset "$PRESET" --output "$OUTPUT"
if [ $? -ne 0 ]; then
    echo "Capture failed"
    exit 1
fi

# Step 2: RT60
python3 tools/plugin-analyzer/python/rt60_analysis_robust.py "$OUTPUT/wet.wav" --output "$OUTPUT"
if [ $? -ne 0 ]; then
    echo "RT60 analysis failed"
    exit 1
fi

# Step 3: Frequency
python3 tools/plugin-analyzer/python/frequency_response.py "$OUTPUT/wet.wav" --output "$OUTPUT"
if [ $? -ne 0 ]; then
    echo "Frequency analysis failed"
    exit 1
fi

echo "✓ Preset $PRESET analyzed successfully"
exit 0
```

**Pros:**
- No C++ changes required
- Flexible scripting
- Easy to debug

**Cons:**
- Shell-only (not cross-platform)
- Harder to integrate into C++ CI

#### Option C: Python Wrapper (Alternative)

Create `unified_analyzer.py` that calls C++ and analysis:

```python
#!/usr/bin/env python3
"""Unified plugin analyzer - captures + analyzes in one command"""

import subprocess
import sys
from pathlib import Path

def capture_audio(plugin, preset, output):
    cmd = [
        "./build/monument_plugin_analyzer",
        "--plugin", plugin,
        "--preset", str(preset),
        "--output", output
    ]
    result = subprocess.run(cmd, capture_output=True)
    return result.returncode == 0

def analyze_rt60(audio_file, output_dir):
    cmd = [
        "python3",
        "tools/plugin-analyzer/python/rt60_analysis_robust.py",
        audio_file,
        "--output", output_dir
    ]
    result = subprocess.run(cmd, capture_output=True)
    return result.returncode == 0

def analyze_frequency(audio_file, output_dir):
    cmd = [
        "python3",
        "tools/plugin-analyzer/python/frequency_response.py",
        audio_file,
        "--output", output_dir
    ]
    result = subprocess.run(cmd, capture_output=True)
    return result.returncode == 0

def main():
    import argparse
    parser = argparse.ArgumentParser(description='Unified plugin analyzer')
    parser.add_argument('--plugin', required=True, help='Path to plugin')
    parser.add_argument('--preset', type=int, required=True, help='Preset index')
    parser.add_argument('--output', required=True, help='Output directory')
    args = parser.parse_args()

    output_path = Path(args.output)
    output_path.mkdir(parents=True, exist_ok=True)

    # Step 1: Capture
    if not capture_audio(args.plugin, args.preset, args.output):
        print("✗ Audio capture failed")
        sys.exit(1)

    audio_file = output_path / "wet.wav"

    # Step 2: Analyze RT60
    if not analyze_rt60(str(audio_file), args.output):
        print("✗ RT60 analysis failed")
        sys.exit(1)

    # Step 3: Analyze frequency
    if not analyze_frequency(str(audio_file), args.output):
        print("✗ Frequency analysis failed")
        sys.exit(1)

    print(f"✓ Preset {args.preset} analyzed successfully")
    sys.exit(0)

if __name__ == "__main__":
    main()
```

**Pros:**
- Clean Python code
- Easy error handling
- Cross-platform

**Cons:**
- Another layer of abstraction
- Python dependency

### Recommended Approach: Option A (C++ Subprocess)

Integrate Python analysis directly into C++ tool for simplicity and atomicity.

**Implementation Plan:**
1. Add `--analyze` flag to C++ tool (default: false)
2. After audio capture, check `--analyze` flag
3. If true, call Python scripts via `std::system()`
4. Report aggregate success/failure
5. Update `capture_all_presets.sh` to use `--analyze`

---

## 2. Parameter Smoothing Test

### Purpose

Detect audible clicks/pops during parameter changes (< -60dB THD+N).

### Test Design

**Approach:** Record audio while sweeping parameter, detect transients

```cpp
// tests/ParameterSmoothingTest.cpp

TEST_CASE("Parameter sweeps produce no clicks (THD+N < -60dB)", "[smoothing]")
{
    // Setup
    MonumentAudioProcessor processor;
    processor.prepareToPlay(48000.0, 512);

    // Test each parameter
    std::vector<std::string> params = {
        "time", "mass", "viscosity", "evolution", "chaos",
        "elasticity", "patina", "abyss", "corona", "breath"
    };

    for (const auto& paramId : params)
    {
        AudioBuffer<float> buffer(2, 48000);  // 1 second
        buffer.clear();

        // Generate test tone (1kHz sine)
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float value = std::sin(2.0 * M_PI * 1000.0 * sample / 48000.0);
            buffer.setSample(0, sample, value);
            buffer.setSample(1, sample, value);
        }

        // Sweep parameter from 0.0 to 1.0 during processing
        auto* param = processor.getParameters().getParameter(paramId);

        for (int block = 0; block < 100; ++block)  // 100 blocks of 480 samples
        {
            float normalizedValue = block / 100.0f;
            param->setValueNotifyingHost(normalizedValue);

            AudioBuffer<float> blockBuffer(2, 480);
            // Copy from main buffer...

            processor.processBlock(blockBuffer, MidiBuffer());
        }

        // Analyze for clicks
        float thdPlusN = calculateTHDplusN(buffer);

        REQUIRE(thdPlusN < -60.0f);  // Must be below -60dB
    }
}

float calculateTHDplusN(const AudioBuffer<float>& buffer)
{
    // FFT analysis to detect harmonics + noise
    // Return dB level of distortion/noise
    // Implementation: Use JUCE FFT or scipy from Python
}
```

**Python Analysis Alternative:**

```python
# tools/test_parameter_smoothing.py

import numpy as np
import scipy.signal as signal
from scipy.fft import fft

def detect_clicks(audio, sample_rate, threshold_db=-60):
    """Detect clicks using high-pass filtering + threshold"""

    # High-pass filter (above 10kHz to isolate clicks)
    sos = signal.butter(4, 10000, 'hp', fs=sample_rate, output='sos')
    filtered = signal.sosfilt(sos, audio)

    # RMS of filtered signal (click energy)
    rms = np.sqrt(np.mean(filtered ** 2))
    rms_db = 20 * np.log10(rms + 1e-10)

    return rms_db > threshold_db

def measure_thd_plus_n(audio, sample_rate, fundamental_freq=1000):
    """Measure THD+N of audio signal"""

    # FFT
    fft_result = np.abs(fft(audio))
    freqs = np.fft.fftfreq(len(audio), 1/sample_rate)

    # Find fundamental and harmonics
    fundamental_bin = np.argmin(np.abs(freqs - fundamental_freq))
    fundamental_power = fft_result[fundamental_bin] ** 2

    # Total power (excluding DC)
    total_power = np.sum(fft_result[1:] ** 2)

    # THD+N = (total - fundamental) / fundamental
    noise_power = total_power - fundamental_power
    thd_plus_n_ratio = noise_power / fundamental_power
    thd_plus_n_db = 10 * np.log10(thd_plus_n_ratio)

    return thd_plus_n_db
```

**Success Criteria:**
- All 10 macro parameters: THD+N < -60dB during sweep
- No transients > -60dB
- Smooth parameter interpolation confirmed

---

## 3. Stereo Width Test

### Purpose

Validate spatial processing correctness and channel correlation.

### Test Design

**Test Cases:**
1. Mono input → Stereo output (check width expansion)
2. Stereo input → Check correlation remains valid
3. Phase coherence (mono compatibility)

```cpp
// tests/StereoWidthTest.cpp

TEST_CASE("Stereo width remains in valid range", "[spatial]")
{
    MonumentAudioProcessor processor;
    processor.prepareToPlay(48000.0, 512);

    // Test 1: Mono input
    AudioBuffer<float> monoInput(2, 48000);
    // Fill both channels with identical sine wave
    for (int ch = 0; ch < 2; ++ch) {
        for (int sample = 0; sample < monoInput.getNumSamples(); ++sample) {
            float value = std::sin(2.0 * M_PI * 440.0 * sample / 48000.0);
            monoInput.setSample(ch, sample, value);
        }
    }

    processor.processBlock(monoInput, MidiBuffer());

    // Measure stereo width
    float correlation = calculateCrossCorrelation(
        monoInput.getReadPointer(0),
        monoInput.getReadPointer(1),
        monoInput.getNumSamples()
    );

    // Reverb should create stereo width (correlation < 1.0)
    // But not completely decorrelated (correlation > 0.0)
    REQUIRE(correlation >= 0.0f);
    REQUIRE(correlation <= 1.0f);

    // Test 2: Phase coherence (mono compatibility)
    AudioBuffer<float> monoSum(1, 48000);
    for (int sample = 0; sample < monoSum.getNumSamples(); ++sample) {
        float summed = (monoInput.getSample(0, sample) + monoInput.getSample(1, sample)) / 2.0f;
        monoSum.setSample(0, sample, summed);
    }

    // Mono sum should not have significant level drop (phase cancellation)
    float stereoRMS = calculateRMS(monoInput);
    float monoRMS = calculateRMS(monoSum);
    float levelDiff = 20 * std::log10(monoRMS / stereoRMS);

    REQUIRE(levelDiff > -6.0f);  // Less than 6dB drop in mono
}
```

**Success Criteria:**
- Correlation: 0.0 ≤ r ≤ 1.0
- Mono compatibility: < 6dB level drop
- No phase inversions

---

## 4. Latency & Phase Test

### Purpose

Validate DAW compatibility and Plugin Delay Compensation (PDC).

### Test Design

**Key Measurements:**
1. Reported latency (via `getLatencySamples()`)
2. Actual latency (impulse→output delay)
3. Phase response (all-pass vs. minimum-phase)

```cpp
// tests/LatencyTest.cpp

TEST_CASE("Reported latency matches actual latency", "[latency]")
{
    MonumentAudioProcessor processor;
    processor.prepareToPlay(48000.0, 512);

    // Get reported latency
    int reportedLatency = processor.getLatencySamples();

    // Measure actual latency with impulse
    AudioBuffer<float> buffer(2, 48000);  // 1 second
    buffer.clear();

    // Impulse at sample 1000
    buffer.setSample(0, 1000, 1.0f);
    buffer.setSample(1, 1000, 1.0f);

    processor.processBlock(buffer, MidiBuffer());

    // Find output impulse peak
    int peakSample = 0;
    float peakValue = 0.0f;
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        float value = std::abs(buffer.getSample(0, sample));
        if (value > peakValue) {
            peakValue = value;
            peakSample = sample;
        }
    }

    int actualLatency = peakSample - 1000;

    // Latency should match within 1 block size
    REQUIRE(std::abs(actualLatency - reportedLatency) <= 512);
}

TEST_CASE("Phase response is valid", "[phase]")
{
    MonumentAudioProcessor processor;
    processor.prepareToPlay(48000.0, 512);

    // Sweep from 20Hz to 20kHz
    AudioBuffer<float> sweep = generateLogSweep(20.0, 20000.0, 48000.0, 5.0);

    processor.processBlock(sweep, MidiBuffer());

    // Measure phase response via FFT
    std::vector<float> phaseResponse = calculatePhaseResponse(sweep);

    // Phase should be continuous (no wrapping discontinuities)
    for (size_t i = 1; i < phaseResponse.size(); ++i) {
        float phaseDiff = std::abs(phaseResponse[i] - phaseResponse[i-1]);
        REQUIRE(phaseDiff < M_PI);  // No phase wrapping
    }
}
```

**Success Criteria:**
- Reported latency matches actual (within 1 block)
- Phase response continuous
- DAW PDC compatibility verified

---

## 5. State Save/Recall Test

### Purpose

Validate automation compatibility and preset management.

### Test Design

**Test Scenarios:**
1. Save state → Recall state (parameters restored)
2. Automation recording (parameter changes tracked)
3. Preset switching during playback (no glitches)

```cpp
// tests/StateManagementTest.cpp

TEST_CASE("State save/recall preserves parameters", "[state]")
{
    MonumentAudioProcessor processor;
    processor.prepareToPlay(48000.0, 512);

    // Set random parameter values
    std::map<std::string, float> originalValues;
    for (auto* param : processor.getParameters()) {
        float value = juce::Random::getSystemRandom().nextFloat();
        param->setValueNotifyingHost(value);
        originalValues[param->getName({})] = value;
    }

    // Save state
    juce::MemoryBlock stateData;
    processor.getStateInformation(stateData);

    // Change all parameters
    for (auto* param : processor.getParameters()) {
        param->setValueNotifyingHost(juce::Random::getSystemRandom().nextFloat());
    }

    // Restore state
    processor.setStateInformation(stateData.getData(), stateData.getSize());

    // Verify parameters restored
    for (auto* param : processor.getParameters()) {
        float restoredValue = param->getValue();
        float originalValue = originalValues[param->getName({})];
        REQUIRE(std::abs(restoredValue - originalValue) < 0.001f);
    }
}

TEST_CASE("Preset switching produces no clicks", "[presets]")
{
    MonumentAudioProcessor processor;
    processor.prepareToPlay(48000.0, 512);

    AudioBuffer<float> buffer(2, 48000);  // 1 second
    buffer.clear();

    // Generate test tone
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        float value = std::sin(2.0 * M_PI * 440.0 * sample / 48000.0);
        buffer.setSample(0, sample, value);
        buffer.setSample(1, sample, value);
    }

    // Process with preset switching every 0.1 seconds
    for (int preset = 0; preset < 37; ++preset) {
        processor.setCurrentProgram(preset);

        AudioBuffer<float> chunk(2, 4800);  // 0.1 seconds
        // Copy from main buffer...

        processor.processBlock(chunk, MidiBuffer());

        // Check for clicks (sudden level changes)
        float maxDiff = 0.0f;
        for (int sample = 1; sample < chunk.getNumSamples(); ++sample) {
            float diff = std::abs(chunk.getSample(0, sample) - chunk.getSample(0, sample - 1));
            maxDiff = std::max(maxDiff, diff);
        }

        // No sample-to-sample difference > 0.1 (click threshold)
        REQUIRE(maxDiff < 0.1f);
    }
}
```

**Success Criteria:**
- All parameters restored accurately
- No glitches during preset switching
- Automation compatible

---

## Implementation Priority

| Task | Priority | Effort | Dependencies |
|------|----------|--------|--------------|
| Unified Analyzer (Option A) | High | 2-4 hours | None |
| Parameter Smoothing Test | High | 3-5 hours | None |
| Stereo Width Test | Medium | 2-3 hours | None |
| Latency Test | Medium | 2-3 hours | None |
| State Save/Recall Test | Low | 1-2 hours | None |

**Total Estimated Time:** 10-17 hours

---

## Success Metrics

### Code Metrics
- Test coverage: 95%+ (all new tests passing)
- CI integration: < 5 minute total test time
- Zero false positives

### Quality Metrics
- Parameter smoothing: < -60dB THD+N
- Stereo correlation: 0.0-1.0 range
- Latency accuracy: ±1 block
- State recall: 100% parameter restoration

---

## CI Integration

```bash
#!/bin/bash
# scripts/run_ci_tests.sh (updated)

# ... (existing tests 1-7) ...

# Step 8: Parameter Smoothing Test
echo "Step 8/12: Parameter Smoothing Test..."
./build/monument_parameter_smoothing_test || exit 1

# Step 9: Stereo Width Test
echo "Step 9/12: Stereo Width Test..."
./build/monument_stereo_width_test || exit 1

# Step 10: Latency Test
echo "Step 10/12: Latency Test..."
./build/monument_latency_test || exit 1

# Step 11: State Management Test
echo "Step 11/12: State Management Test..."
./build/monument_state_management_test || exit 1

# Step 12: Quality Gates (existing)
echo "Step 12/12: Quality Gates..."
python3 tools/check_audio_stability.py test-results/preset-baseline || exit 1

echo "✅ All tests passed"
```

---

## Future Enhancements

### Phase 5: Advanced Testing
1. **CPU Stress Test** - Maximum polyphony load
2. **Memory Leak Test** - Long-running stability
3. **Cross-Platform Test** - Windows/macOS/Linux validation
4. **Host Compatibility** - Test in 10+ DAWs
5. **Automation Stress** - Rapid parameter changes

### Automation Improvements
1. **Visual Regression** - Screenshot comparison
2. **Performance Regression** - CPU trend tracking
3. **Audio Regression** - Bit-exact comparison
4. **Preset Regression** - RT60/frequency drift

---

## References

- [JUCE Plugin Testing Best Practices](https://docs.juce.com/master/tutorial_plugin_test.html)
- [PHASE_3_STEP_2_QUALITY_GATES_COMPLETE.md](PHASE_3_STEP_2_QUALITY_GATES_COMPLETE.md)
- [TESTING_GUIDE.md](TESTING_GUIDE.md)

---

**Document Version:** 1.0
**Status:** Design Complete - Ready for Implementation
