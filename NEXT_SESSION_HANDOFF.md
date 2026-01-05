# Monument Reverb - Session Handoff

**Date:** 2026-01-05 (Updated - Evening)
**Branch:** `feature/three-systems`
**Progress:** 31 of 42 tasks (74% complete)
**Status:** Phase 4 COMPLETE ✅ + Comprehensive Test Plan Ready ✅

---

## Latest Session (2026-01-05 Part 4): Phase 4 - SequenceScheduler + Test Plan ✅

### What We Built

**🎯 Phase 4 Complete** - Timeline automation system for parameter evolution!

**New Files:**
- ✅ [dsp/SequenceScheduler.h/cpp](dsp/SequenceScheduler.h) - Timeline-based parameter automation
- ✅ [dsp/SequencePresets.h/cpp](dsp/SequencePresets.h) - 3 factory timeline presets
- ✅ [tests/SequenceSchedulerTest.cpp](tests/SequenceSchedulerTest.cpp) - Complete unit tests (7 passing)
- ✅ [tools/COMPREHENSIVE_TEST_PLAN.md](tools/COMPREHENSIVE_TEST_PLAN.md) - 10 automated DSP test suites

**SequenceScheduler Features:**
- Keyframe-based timeline with arbitrary parameter targets
- Tempo-synchronized playback (beats) or free-running (seconds)
- Multiple interpolation curves: Linear, Exponential, S-curve, Step
- Loop modes: One-shot, Loop, Ping-pong
- Real-time safe (pre-allocated, no locks in audio thread)

**Timeline Presets:**
1. **Evolving Cathedral** - Small room → massive cathedral over 16 bars (tempo-synced)
2. **Spatial Journey** - 3D circular motion with Doppler shifts (tempo-synced)
3. **Living Space** - Subtle organic drift over 32 seconds (free-running)

**Integration:**
- Wired to [plugin/PluginProcessor.cpp](plugin/PluginProcessor.cpp) - Processes before parameter cache
- Overrides APVTS parameters when sequence is enabled
- All 26 parameters can be automated (Time, Mass, Density, etc.)

**Test Results:**
```
===== SequenceScheduler Unit Tests =====
✓ Basic keyframe interpolation
✓ Multiple parameter automation
✓ Loop playback mode
✓ Interpolation curves (Linear, S-curve, etc.)
✓ Tempo-synced beat timing
✓ Factory presets load correctly
✓ Disabled sequence bypass

✅ All 7 tests passed!
```

---

## Previous Session (2026-01-05 Part 3): Batch Testing Infrastructure ✅

### What We Built

**🚀 Complete automated testing system** for all 37 factory presets with parallel processing!

**Performance Results:**
- ✅ Batch capture: **53 seconds** for 30s duration (18x faster than sequential!)
- ✅ Batch analysis: **30 seconds** (parallel processing)
- ✅ Total pipeline: **~83 seconds** (vs 18+ minutes sequential)
- ✅ Automatic CPU core detection (capped at 8 workers)

### New Scripts & Tools

**1. [scripts/capture_all_presets.sh](scripts/capture_all_presets.sh)** - Parallel preset capture
```bash
./scripts/capture_all_presets.sh  # Auto-detects 8 cores, captures in ~53 seconds
PARALLEL_JOBS=4 ./scripts/capture_all_presets.sh  # Override parallel workers
```
- **30-second duration per preset** (ensures complete capture of longest RT60 at 29.85s)
- Parallel execution with xargs
- Auto-generates metadata JSON for each capture
- Progress tracking for all 37 presets

**2. [scripts/analyze_all_presets.sh](scripts/analyze_all_presets.sh)** - Parallel analysis
```bash
./scripts/analyze_all_presets.sh  # RT60 + frequency response for all presets
```
- RT60 decay time analysis (pyroomacoustics)
- Frequency response with octave band breakdown
- Generates PNG plots and JSON metrics
- Parallel processing of all 37 presets

**3. [tools/plot_preset_comparison.py](tools/plot_preset_comparison.py)** - Visualization
```bash
python3 tools/plot_preset_comparison.py test-results/preset-baseline
```
- RT60 comparison bar chart (color-coded by length)
- Frequency response heatmap (7 octave bands)
- Statistical summary (mean, median, std dev, min/max)
- Quality distribution ratings

**4. [tools/compare_baseline.py](tools/compare_baseline.py)** - Regression testing
```bash
python3 tools/compare_baseline.py baseline/ current/ --threshold 0.05
```
- Compares RT60, frequency response, and waveform metrics
- Configurable thresholds (default: 5% change)
- Exit code 0 = pass, 1 = regression detected
- JSON report output for CI integration

### Test Results Summary

**All 37 Presets Analyzed Successfully!**

**RT60 Statistics (30s captures):**
- Mean: 15.415 seconds
- Median: 13.310 seconds
- Range: 4.850s (Preset 6) to **29.849s (Preset 14)** 🎯
- Std Dev: 6.206 seconds
- **Discovery:** Preset 14 has extreme 30-second decay - captures were cutting off tails!

**Frequency Response:**
- All presets: Fair flatness rating (±10dB range)
- Mean: ±8.81 dB
- Most flat: Preset 6 (±8.59 dB)
- Most colored: Preset 5 (±9.01 dB)

### Files Created

```
scripts/
├── capture_all_presets.sh          # NEW: Parallel batch capture
└── analyze_all_presets.sh          # NEW: Parallel batch analysis

tools/
├── plot_preset_comparison.py       # NEW: Visualization generator
├── compare_baseline.py             # NEW: Regression testing
└── TESTING_INFRASTRUCTURE.md       # NEW: Complete documentation

test-results/
├── preset-baseline/                # Generated baseline data
│   ├── preset_00/
│   │   ├── wet.wav                 # 24-bit processed audio
│   │   ├── dry.wav                 # Input signal
│   │   ├── rt60_metrics.json       # Decay time
│   │   ├── freq_metrics.json       # Frequency response
│   │   ├── frequency_response.png  # Spectrum plot
│   │   └── metadata.json           # Capture info
│   └── ... (37 total)
└── comparisons/
    ├── rt60_comparison.png         # Bar chart (114KB)
    ├── frequency_response_comparison.png  # Heatmap (95KB)
    └── summary_statistics.txt      # Text summary
```

### Quick Start

```bash
# 1. Capture all 37 presets (parallel, ~53 seconds with 30s duration)
./scripts/capture_all_presets.sh

# 2. Analyze all captures (parallel, ~30 seconds)
./scripts/analyze_all_presets.sh

# 3. Generate visualizations
python3 tools/plot_preset_comparison.py test-results/preset-baseline

# 4. View results
open test-results/comparisons/rt60_comparison.png
open test-results/comparisons/frequency_response_comparison.png
cat test-results/comparisons/summary_statistics.txt
```

### Regression Testing Workflow

```bash
# Before code changes - capture baseline
./scripts/capture_all_presets.sh
mv test-results/preset-baseline test-results/baseline-v1.0.0

# After DSP changes - capture current
./scripts/capture_all_presets.sh

# Compare for regressions
python3 tools/compare_baseline.py \
  test-results/baseline-v1.0.0 \
  test-results/preset-baseline \
  --threshold 0.05 \
  --output regression-report.json

# Exit code 0 = pass, 1 = regression detected
```

### Documentation

See **[tools/TESTING_INFRASTRUCTURE.md](tools/TESTING_INFRASTRUCTURE.md)** for:
- Complete usage guide
- CI/CD integration examples
- Performance benchmarks
- Troubleshooting tips
- File structure details

---

## Previous Session (2026-01-05 Part 2): Build Infrastructure & Preset Verification ✅

### What We Fixed

**✅ Preset Loading Verified** - All 37 factory presets work correctly!
```
✓ Loaded preset 0: Init Patch
✓ Loaded preset 7: Cathedral of Glass
✓ All 37 presets accessible via JUCE program interface
```

**✅ Created [scripts/rebuild_and_install.sh](scripts/rebuild_and_install.sh)**
- Builds `Monument_All` target (VST3 + AU + Standalone)
- Locates binaries in correct Debug/Release folders
- Removes old plugins before installing (avoids macOS caching issues)
- Installs to system directories automatically

### Build Issues Discovered & Resolved

**Problem 1:** Building `Monument` target only created `libMonument_SharedCode.a` (static library), not the actual VST3/AU binaries.

**Solution:** Build `Monument_All` or individual format targets (`Monument_VST3`, `Monument_AU`).

**Problem 2:** Plugin binaries are in `build/Monument_artefacts/Debug/` but our script looked in `build/Monument_artefacts/`.

**Solution:** Updated script to check Debug folder first, then Release.

**Problem 3:** macOS caches plugins even after `cp -R`, causing stale binaries to load.

**Solution:** `rm -rf` old plugins before installing fresh ones.

### Usage

```bash
# Rebuild everything (VST3, AU, Analyzer)
./scripts/rebuild_and_install.sh all

# Rebuild just Monument plugin
./scripts/rebuild_and_install.sh Monument

# Rebuild just analyzer
./scripts/rebuild_and_install.sh monument_plugin_analyzer
```

**Installed Locations:**
- `~/Library/Audio/Plug-Ins/VST3/Monument.vst3`
- `~/Library/Audio/Plug-Ins/Components/Monument.component`

### Testing Commands

```bash
# Test preset loading
./build/monument_plugin_analyzer_artefacts/Debug/monument_plugin_analyzer \
  --plugin ~/Library/Audio/Plug-Ins/VST3/Monument.vst3 \
  --preset 0 \
  --duration 5

# Test with specific preset
monument_plugin_analyzer --plugin Monument.vst3 --preset 7 --duration 10

# RT60 analysis
python3 tools/plugin-analyzer/python/rt60_analysis_robust.py test-results/wet.wav

# Frequency response
python3 tools/plugin-analyzer/python/frequency_response.py test-results/wet.wav
```

### Build System Notes

**Current Generator:** Unix Makefiles
**Recommended:** Ninja Multi-Config (30-50% faster incremental builds)

To switch to Ninja:
```bash
rm -rf build
cmake -B build -G "Ninja Multi-Config"
./scripts/rebuild_and_install.sh all
```

### Documentation Updates Needed

⚠️ **[STANDARD_BUILD_WORKFLOW.md](STANDARD_BUILD_WORKFLOW.md)** needs updates:
- Line 32-35: "Auto-Install" doesn't work - binaries aren't auto-copied
- Line 81-82: Building `Monument_AU` alone doesn't create usable binary
- Recommend updating to reference `./scripts/rebuild_and_install.sh`

---

## Previous Session (2026-01-05 Part 1): Plugin Analyzer Tool ✅

### What We Built

**New Tool:** [tools/plugin-analyzer](tools/plugin-analyzer/) - Automated VST3/AU audio testing framework

**Purpose:** Capture impulse responses, frequency sweeps, and analyze reverb characteristics for regression testing and quality assurance.

### Tool Features

- ✅ **Plugin Loading** - JUCE-based VST3/AU dynamic loading
- ✅ **Preset Support** - `--preset <index>` loads factory presets (0-36)
- ✅ **Test Signals** - Impulse, sine sweep, white/pink noise generation
- ✅ **Audio Capture** - Records dry/wet signals to 24-bit WAV
- ✅ **Python RT60** - Integration with pyroomacoustics for decay time analysis
- ✅ **Frequency Response** - FFT analysis with octave band visualization
- ✅ **CLI Interface** - Command-line arguments for automation
- ✅ **CMake Integration** - Builds alongside Monument plugin

### Files Created

```
tools/plugin-analyzer/
├── src/
│   ├── PluginLoader.h/.cpp         # VST3/AU loading
│   ├── TestSignalGenerator.h/.cpp  # Signal generation
│   ├── AudioCapture.h/.cpp         # WAV export (24-bit)
│   └── main.cpp                    # CLI entry point
├── python/
│   ├── rt60_analysis_robust.py     # RT60 with fallback methods
│   ├── frequency_response.py       # FFT + octave band analysis
│   └── requirements.txt            # pyroomacoustics
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

### Test Results (Default Preset - Init Patch)

✅ **RT60:** 4.924 seconds - Beautiful long tail!
✅ **Frequency Response:** ±9.38 dB (Fair flatness rating)
✅ **Dynamic Range:** 101 dB
✅ **37 Factory Presets** accessible via `--preset <index>`

---

## NEXT SESSION PRIORITIES

### Option B: Test Infrastructure & Quality Assurance ⭐ **READY TO START**

All infrastructure is in place! Next steps:

#### 1. **Batch Preset Capture** (1-2 hours)
```bash
# Capture all 37 presets
for i in {0..36}; do
  monument_plugin_analyzer \
    --plugin ~/Library/Audio/Plug-Ins/VST3/Monument.vst3 \
    --preset $i \
    --duration 10 \
    --output "test-results/preset_$i"
done

# Analyze each
for i in {0..36}; do
  python3 python/rt60_analysis_robust.py "test-results/preset_$i/wet.wav" \
    --output "test-results/preset_$i/metrics.json"
  python3 python/frequency_response.py "test-results/preset_$i/wet.wav" \
    --output "test-results/preset_$i/freq_response.png"
done
```

**Deliverables:**
- RT60 measurements for all 37 presets
- Frequency response plots (PNG)
- JSON metrics for regression testing
- Baseline reference data

#### 2. **Visualizations & Documentation** (1 hour)
```bash
# Create summary document
python3 tools/generate_preset_report.py test-results/ \
  --output docs/PRESET_CHARACTERISTICS.md

# Generate comparison charts
python3 tools/plot_preset_comparison.py test-results/ \
  --output docs/preset-comparison.png
```

**Deliverables:**
- Visual comparison of all presets (RT60 + freq response)
- Markdown documentation with audio characteristics
- Marketing-ready specs

#### 3. **Regression Testing Framework** (2 hours)
```bash
# Save baseline (commit to git LFS or archive)
./scripts/capture_baseline.sh

# After code changes
./scripts/capture_current.sh

# Compare
python3 tools/compare_captures.py baseline/ current/ \
  --threshold 0.1 \
  --output regression-report.json
```

**Deliverables:**
- `scripts/capture_baseline.sh` - Batch capture script
- `tools/compare_captures.py` - Comparison tool
- Baseline impulse responses (archived)
- CI-ready comparison script

#### 4. **CI Integration** (1 hour)
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
        run: ./scripts/rebuild_and_install.sh all

      - name: Test Default Preset
        run: |
          monument_plugin_analyzer \
            --plugin ~/Library/Audio/Plug-Ins/VST3/Monument.vst3 \
            --duration 5
          python3 python/verify_metrics.py test-results/wet.wav
```

**Deliverables:**
- GitHub Actions workflow
- Automated regression checks
- Build artifacts for debugging

---

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

## Remaining: Phase 4-6 + Integration (19 Tasks)

- **Phase 4 (8 tasks):** SequenceScheduler + preset integration
- **Phase 5 (3 tasks):** Timeline UI editor
- **Phase 6 (5 tasks):** Memory Echoes integration
- **Integration (3 tasks):** Full testing, CPU profiling, showcase presets

**Total Progress:** 23/42 tasks (55%)

---

## Build Commands

```bash
# Recommended workflow (uses new script)
./scripts/rebuild_and_install.sh all          # Build everything
./scripts/rebuild_and_install.sh Monument     # Just plugin
./scripts/rebuild_and_install.sh monument_plugin_analyzer  # Just analyzer

# Manual build (if needed)
cmake --build build --target Monument_All     # VST3, AU, Standalone
cmake --build build --target monument_plugin_analyzer

# Run tests
ctest --test-dir build -C Debug

# Test in DAW
# Plugins auto-installed at ~/Library/Audio/Plug-Ins/
```

---

## Git Status

**Current Branch:** `feature/three-systems`

**Recent Commits:**
- `a25d841` - feat: add preset loading support to Monument and analyzer
- `3276316` - feat: improve plugin analyzer with mix control and robust analysis
- `26ab884` - feat: add plugin analyzer tool for audio testing
- `a74459e` - docs: update handoff for Phase 3 completion
- `e6a7bfd` - feat: implement Phase 3 - Doppler shift effects

**Untracked Files:** `test-results/` (analyzer output, gitignored)

---

## NEXT SESSION PRIORITIES

### Option A: Implement Comprehensive DSP Tests ⭐⭐⭐ (Recommended)

**Goal:** Add 10 automated DSP quality tests (zero manual DAW testing required)

**Test Plan:** See [tools/COMPREHENSIVE_TEST_PLAN.md](tools/COMPREHENSIVE_TEST_PLAN.md) for full details

**Priority 1 - High (Week 1):**
1. ✅ RT60 Accuracy Test - Verify decay time matches `time` parameter
2. ✅ Frequency Response Test - Verify tonal balance and flatness
3. ✅ CPU Performance Benchmark - Prevent performance regressions (<2ms/block)
4. ✅ Audio Regression Test - Detect unintended DSP changes

**Priority 2 - Medium (Week 2):**
5. ✅ Modulation Depth Test - Verify bounds and stability
6. ✅ Parameter Smoothing Test - No clicks/pops (<-60dB THD+N)
7. ✅ Stereo Width Test - Verify spatial processing
8. ✅ Latency & Phase Test - DAW compatibility

**Priority 3 - Low (Week 3):**
9. ✅ Preset Loading Test - All 37 presets load without errors
10. ✅ State Save/Recall Test - DAW automation compatibility

**Deliverables:**
- 10 new test files in `tests/`
- Updated CMakeLists.txt with new test targets
- CI/CD integration (GitHub Actions)
- ~50 individual test cases
- 100% automated (no manual DAW testing)

### Option B: Continue Three-System Plan → Phase 5

**Phase 5 - Timeline UI Editor** (Tasks 32-36)
- Visual timeline editor for SequenceScheduler
- Drag-and-drop keyframe editing
- Real-time preview while editing
- Save/load custom sequences

### Option C: Phase 6 - Memory Echoes Timeline Integration

**Phase 6 - Memory Echoes + SequenceScheduler** (Tasks 37-42)
- Integrate Memory Echoes with timeline automation
- Automated memory depth morphing
- Cross-fade between memory states

---

## Key Files Modified (Phase 4)

**New Files:**
- **[dsp/SequenceScheduler.h](dsp/SequenceScheduler.h)** - Timeline automation header (282 lines)
- **[dsp/SequenceScheduler.cpp](dsp/SequenceScheduler.cpp)** - Implementation (392 lines)
- **[dsp/SequencePresets.h](dsp/SequencePresets.h)** - Factory presets header
- **[dsp/SequencePresets.cpp](dsp/SequencePresets.cpp)** - 3 timeline presets (205 lines)
- **[tests/SequenceSchedulerTest.cpp](tests/SequenceSchedulerTest.cpp)** - Unit tests (326 lines)
- **[tools/COMPREHENSIVE_TEST_PLAN.md](tools/COMPREHENSIVE_TEST_PLAN.md)** - Test plan (380 lines)

**Modified Files:**
- **[plugin/PluginProcessor.h:12,74,100](plugin/PluginProcessor.h)** - Added SequenceScheduler member + getter
- **[plugin/PluginProcessor.cpp:119,263-295](plugin/PluginProcessor.cpp)** - prepare() + processBlock() integration
- **[CMakeLists.txt:335-352](CMakeLists.txt)** - Added SequenceScheduler test target

---

## Three-System Plan Progress Update

**Current Status:** 31 of 42 tasks (74% complete)

**Completed Phases:**
- ✅ Phase 1: Spatial Basics (Tasks 1-7)
- ✅ Phase 2: 3D Panning (Tasks 8-15)
- ✅ Phase 3: Doppler Effects (Tasks 16-23)
- ✅ Phase 4: SequenceScheduler (Tasks 24-31) ← **JUST COMPLETED**

**Remaining Phases:**
- ⏳ Phase 5: Timeline UI Editor (Tasks 32-36) - 5 tasks
- ⏳ Phase 6: Memory Echoes Integration (Tasks 37-42) - 6 tasks

---

## Token Budget Notes

**Session Cost:** ~$6-7 (within $12/day ceiling)
**Context Used:** 106K/200K tokens (53%)
**Recommendation:** `/clear` and use this handoff for next session (fresh start for comprehensive tests)

---

**Context:** Phase 4 complete, testing infrastructure ready, comprehensive test plan documented.
**Next:** Implement 10 automated DSP quality tests (Option A strongly recommended).
