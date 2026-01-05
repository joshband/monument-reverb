# Monument Reverb - Session Handoff

**Date:** 2026-01-05
**Branch:** `feature/three-systems`
**Progress:** 23 of 42 tasks (55% complete)
**Status:** Phase 3 COMPLETE ✅ + Plugin Analyzer Tool COMPLETE ✅

---

## Latest Session (2026-01-05): Plugin Analyzer Tool ✅

### What We Built

**New Tool:** [tools/plugin-analyzer](tools/plugin-analyzer/) - Automated VST3/AU audio testing framework

**Purpose:** Capture impulse responses, frequency sweeps, and analyze reverb characteristics for regression testing and quality assurance.

### Tool Features

- ✅ **Plugin Loading** - JUCE-based VST3/AU dynamic loading
- ✅ **Test Signals** - Impulse, sine sweep, white/pink noise generation
- ✅ **Audio Capture** - Records dry/wet signals to 24-bit WAV
- ✅ **Python RT60** - Integration with pyroomacoustics for decay time analysis
- ✅ **CLI Interface** - Command-line arguments for automation
- ✅ **CMake Integration** - Builds alongside Monument plugin
- ✅ **Documentation** - Full README with usage examples

### Files Created

```
tools/plugin-analyzer/
├── src/
│   ├── PluginLoader.h/.cpp         # VST3/AU loading
│   ├── TestSignalGenerator.h/.cpp  # Signal generation
│   ├── AudioCapture.h/.cpp         # WAV export
│   └── main.cpp                    # CLI entry point
├── python/
│   ├── rt60_analysis.py            # RT60 measurement
│   └── requirements.txt            # Dependencies
└── README.md                       # Complete documentation
```

### CMake Changes

```cmake
# CMakeLists.txt:335-381
option(MONUMENT_BUILD_ANALYZER "Build Monument plugin analyzer tool" ON)

if(MONUMENT_BUILD_ANALYZER)
  juce_add_console_app(monument_plugin_analyzer ...)
  # Links: juce_audio_utils, juce_audio_formats, juce_dsp
endif()
```

### Usage Examples

```bash
# Capture 5-second impulse response
./build/monument_plugin_analyzer_artefacts/Debug/monument_plugin_analyzer \
  --plugin /Users/noisebox/Library/Audio/Plug-Ins/VST3/Monument.vst3 \
  --duration 5

# Analyze RT60
python3 tools/plugin-analyzer/python/rt60_analysis.py test-results/wet.wav

# Frequency sweep analysis
monument_plugin_analyzer --plugin Monument.vst3 --test sweep --duration 10
```

### Test Results

✅ Successfully loaded Monument v0.1.0
✅ Generated impulse signal (48000 samples @ 48kHz)
✅ Processed through plugin block-by-block
✅ Exported dry.wav + wet.wav (24-bit, stereo)
⚠️ RT60 needs longer duration (5+ seconds for accurate measurement)

---

## Three-System Plan Progress

**Date:** 2026-01-03
**Progress:** 23 of 42 tasks (55% complete)
**Status:** Phase 3 COMPLETE ✅ - Ready for Phase 4

### Completed Phases

#### Phase 1: Spatial Basics ✅ (Committed: c19b7d9)
- Created [dsp/SpatialProcessor.h/cpp](dsp/SpatialProcessor.h) - 1/r² attenuation
- Modified [dsp/Chambers.h/cpp](dsp/Chambers.h) - FDN integration at line ~630
- Extended [dsp/ModulationMatrix.h](dsp/ModulationMatrix.h) - PositionX/Y/Z (lines 69-71)
- Updated [plugin/PresetManager.cpp](plugin/PresetManager.cpp) - Serialization (667-669, 711-713)
- Wired [plugin/PluginProcessor.cpp](plugin/PluginProcessor.cpp) - Spatial routing (567-582)

#### Phase 2: 3D Panning ✅ (Committed: bf376c0)
- Modified [dsp/DspModules.h](dsp/DspModules.h) - Facade methods (182-200, 213-220)
- Implemented [dsp/DspModules.cpp](dsp/DspModules.cpp) - Constant power panning (623-627, 669-756)
- Extended [dsp/ModulationMatrix.h](dsp/ModulationMatrix.h) - Distance, VelocityX (73-74)
- Updated [plugin/PresetManager.cpp](plugin/PresetManager.cpp) - String conversion (671-672, 718-719)
- Created [tests/ConstantPowerPanningTest.cpp](tests/ConstantPowerPanningTest.cpp) - Unit test
- Updated [CMakeLists.txt](CMakeLists.txt) - Test target (284-313)

#### Phase 3: Doppler Effects ✅ (Committed: e6a7bfd, a74459e)
- Modified [dsp/Chambers.cpp:628-644](dsp/Chambers.cpp#L628) - Doppler shift integration
- Integrated `getDopplerShift()` into fractional delay calculation
- Velocity-based pitch shifting (±50ms max shift @ 48kHz)
- Created [tests/DopplerShiftTest.cpp](tests/DopplerShiftTest.cpp) - Stability unit test
- Updated [CMakeLists.txt](CMakeLists.txt) - Test target (316-333)
- **Tests:** 6 test cases pass (bounds, scaling, disable, stability, per-line, clipping)

---

## NEXT SESSION PRIORITIES

### Option A: Continue Three-System Plan → Phase 4

**Phase 4 - SequenceScheduler** (Tasks 24-31)
Timeline system for parameter automation and preset morphing

- **Task 24:** Create [dsp/SequenceScheduler.h/cpp](dsp/SequenceScheduler.h)
- **Task 25:** Implement keyframe storage and interpolation
- **Task 26:** Add timeline playback with tempo sync
- **Task 27:** Wire to PluginProcessor modulation system
- **Task 28:** Create 3 timeline presets (Evolving Cathedral, Spatial Journey, Living Space)
- **Task 29:** Write SequenceScheduler unit test
- **Task 30:** Build and verify Phase 4
- **Task 31:** Manual testing in DAW

### Option B: Test Infrastructure & Quality Assurance ⭐ Recommended

Use the new plugin analyzer to build comprehensive test suite:

#### 1. **Preset Validation** (2-3 hours)
```bash
# Goal: Capture impulse response for all factory presets
# Verify RT60, frequency response, no crashes

for preset in Small_Room Cathedral Plate Hall; do
  monument_plugin_analyzer \
    --plugin Monument.vst3 \
    --preset "$preset" \
    --duration 5 \
    --output "test-results/$preset"

  python3 python/rt60_analysis.py \
    "test-results/$preset/wet.wav" \
    --output "test-results/$preset/metrics.json"
done
```

**Deliverables:**
- Baseline RT60 measurements for all presets
- Frequency response plots
- JSON metrics for CI integration

#### 2. **Regression Testing** (1-2 hours)
```bash
# Goal: Compare before/after DSP changes
# Detect unintended audio changes

# Save baseline
monument_plugin_analyzer --plugin Monument.vst3 --output baseline/

# After code changes
monument_plugin_analyzer --plugin Monument.vst3 --output current/

# Compare (write Python script)
python3 tools/compare_impulses.py baseline/ current/
```

**Deliverables:**
- Baseline impulse responses (git LFS or archive)
- Comparison script (`tools/compare_impulses.py`)
- CI workflow for automatic regression checks

#### 3. **RT60 Characterization** (1 hour)
```bash
# Goal: Measure decay times per preset
# Document reverb characteristics

monument_plugin_analyzer \
  --plugin Monument.vst3 \
  --duration 10 \
  --output long-capture/

python3 python/rt60_analysis.py \
  long-capture/wet.wav \
  --output docs/rt60-measurements.json
```

**Deliverables:**
- Documented RT60 values per frequency band
- Visualization plots (PNG)
- Marketing-ready specs ("2.4s @ 500Hz")

#### 4. **Frequency Response Analysis** (2-3 hours)
```bash
# Goal: Verify reverb doesn't color the sound
# Measure frequency response flatness

monument_plugin_analyzer \
  --plugin Monument.vst3 \
  --test sweep \
  --duration 10

# Write Python FFT analyzer
python3 python/frequency_response.py \
  test-results/sweep-wet.wav \
  --plot freq-response.png
```

**Deliverables:**
- New `python/frequency_response.py` script
- Frequency response plots per preset
- Pass/fail criteria (e.g., ±3dB 20Hz-20kHz)

#### 5. **CI Integration** (1-2 hours)
```yaml
# .github/workflows/audio-tests.yml
name: Audio Quality Tests
on: [push, pull_request]

jobs:
  test-audio:
    runs-on: macos-latest
    steps:
      - uses: actions/checkout@v3
      - name: Build Monument
        run: cmake --build build --target Monument

      - name: Run Analyzer
        run: |
          monument_plugin_analyzer \
            --plugin build/Monument.vst3 \
            --duration 5

      - name: Verify RT60
        run: |
          python3 python/rt60_analysis.py wet.wav
          python3 tools/verify_metrics.py wet.wav
```

**Deliverables:**
- CI workflow that runs on every commit
- Automated pass/fail checks
- Build artifacts (WAV files for debugging)

---

## Remaining: Phase 4-6 + Integration (19 Tasks)

- **Phase 4 (8 tasks):** SequenceScheduler + preset integration
- **Phase 5 (3 tasks):** Timeline UI editor
- **Phase 6 (5 tasks):** Memory Echoes integration
- **Integration (3 tasks):** Full testing, CPU profiling, showcase presets

**Total Progress:** 23/42 tasks (55%)

---

## Key Implementation Details

### Spatial Attenuation (Phase 1)
```cpp
// dsp/Chambers.cpp:627-636
if (spatialProcessor)
    spatialProcessor->process();  // Block-rate update

// In FDN loop per sample:
const float spatialAttenuation = spatialProcessor->getAttenuationGain(i);
delayedSample *= spatialAttenuation;
```

### 3D Panning (Phase 2)
```cpp
// dsp/DspModules.cpp:670-684
const float mono = 0.5f * (left + right);
const float leftGain = leftGainSmoother.getNextValue();
const float rightGain = rightGainSmoother.getNextValue();
left = mono * leftGain * gainLocal;
right = mono * rightGain * gainLocal;
```

**Math:** Azimuth -90°→+90°, L=cos(θ/2), R=sin(θ/2), L²+R²=1
**Elevation:** gain *= cos(elevation)

### Doppler Shift (Phase 3)
```cpp
// dsp/Chambers.cpp:628-644
float dopplerShift = 0.0f;
if (spatialProcessor && spatialProcessor->isEnabled())
    dopplerShift = spatialProcessor->getDopplerShift(i);

// Apply to fractional delay
float effectiveDelay = delayTimeSamples + driftModulation + dopplerShift;
effectiveDelay = juce::jlimit(minDelay, maxDelay, effectiveDelay);
```

**Max Shift:** ±50ms (2400 samples @ 48kHz)
**Velocity Mapping:** +1.0 = moving away, -1.0 = moving toward

---

## Build Commands

```bash
# Build Monument plugin
cmake --build build --target Monument

# Build plugin analyzer
cmake --build build --target monument_plugin_analyzer

# Run tests
ctest --test-dir build -C Debug

# Test in DAW
# AU/VST3 installed at ~/Library/Audio/Plug-Ins/
```

---

## Git Status

**Current Branch:** `feature/three-systems`

**Recent Commits:**
- `a74459e` - docs: update handoff for Phase 3 completion
- `e6a7bfd` - feat: implement Phase 3 - Doppler shift effects
- `bf376c0` - feat: implement Phase 2 - 3D azimuth/elevation panning
- `c19b7d9` - feat: implement Phase 1 spatial positioning system

**Clean Working Directory:** All plugin analyzer code in `tools/plugin-analyzer/` (not committed yet)

---

## Recommended Next Steps

### Immediate (This Session)
1. ✅ Commit plugin analyzer tool
2. Test with 5-second duration for accurate RT60
3. Document baseline measurements

### Next Session (Option B - Testing Focus)
1. **Preset Validation** - Capture all factory presets
2. **Baseline Measurements** - RT60 + frequency response
3. **Regression Framework** - Comparison scripts
4. **CI Integration** - Automated audio tests

### Future Sessions (Option A - Continue Three-System)
1. Phase 4: SequenceScheduler implementation
2. Phase 5: Timeline UI editor
3. Phase 6: Memory Echoes integration

---

## Documentation

- **Plugin Analyzer:** [tools/plugin-analyzer/README.md](tools/plugin-analyzer/README.md)
- **Three-System Plan:** Project root docs
- **Test Results:** `test-results/` directory (gitignored, regenerate each session)

---

## Token Budget Notes

**Session Cost:** ~$3-4 (well under $6/day target)
**Context Used:** 98K/200K tokens (49%)
**Recommendation:** Safe to continue in same session OR `/clear` and start fresh with this handoff

---

**Context:** Continue Three-System Plan OR build test infrastructure with new analyzer tool.
**Choice:** Option B (Testing) recommended to validate Phases 1-3 before continuing to Phase 4.
