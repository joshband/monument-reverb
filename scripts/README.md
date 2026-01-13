# Monument Reverb Scripts

Automation and workflow scripts for development, testing, profiling, and CI/CD.

## Quick Reference

| Script | Purpose | Usage |
|--------|---------|-------|
| [build_macos.sh](#build_macossh) | Build Monument (Release) | `./scripts/build_macos.sh` |
| [rebuild_and_install.sh](#rebuild_and_installsh) | Rebuild + install plugins | `./scripts/rebuild_and_install.sh [target]` |
| [install_macos.sh](#install_macossh) | Install AU/VST3 plugins | `./scripts/install_macos.sh` |
| [dev_loop.sh](#dev_loopsh) | Development watch loop | `./scripts/dev_loop.sh` |
| [open_xcode.sh](#open_xcodesh) | Generate Xcode project | `./scripts/open_xcode.sh` |
| [run_ci_tests.sh](#run_ci_testssh) | **Complete CI test suite** | `./scripts/run_ci_tests.sh` |
| [capture_all_presets.sh](#capture_all_presetssh) | Batch preset capture | `PARALLEL_JOBS=8 ./scripts/capture_all_presets.sh` |
| [analyze_all_presets.sh](#analyze_all_presetssh) | Batch audio analysis | `PARALLEL_JOBS=8 ./scripts/analyze_all_presets.sh` |
| [profile_cpu.sh](#profile_cpush) | CPU profiling (automatic) | `./scripts/profile_cpu.sh` |
| [profile_with_audio.sh](#profile_with_audiosh) | CPU profiling (interactive) | `./scripts/profile_with_audio.sh` |
| [profile_in_reaper.sh](#profile_in_reapersh) | Profile in Reaper DAW | `./scripts/profile_in_reaper.sh` |
| [analyze_profile.py](#analyze_profilepy) | Parse profiling results | `python3 scripts/analyze_profile.py <trace.xml>` |
| [run_pluginval.sh](#run_pluginvalsh) | VST3/AU validation | `./scripts/run_pluginval.sh` |

**Build directory overrides:**
- Most scripts respect `BUILD_DIR` (e.g., `BUILD_DIR=build-ninja ./scripts/run_ci_tests.sh`).
- Default preference is `build/` if present, otherwise `build-ninja/`.

---

## Build & Development Scripts

### build_macos.sh

**Purpose:** Configure and build Monument in Release mode with Ninja generator.

**Requirements:**
- `JUCE_SOURCE_DIR` environment variable (optional, uses FetchContent if not set)
- CMake >= 3.21
- Ninja build system

**Usage:**
```bash
./scripts/build_macos.sh
```

**Optional overrides:**
```bash
BUILD_DIR=build-ninja ./scripts/build_macos.sh
GENERATOR="Ninja Multi-Config" ./scripts/build_macos.sh
```

**Output:**
- `build-ninja/Monument_artefacts/VST3/Monument.vst3`
- `build-ninja/Monument_artefacts/AU/Monument.component`
- `build-ninja/Monument_artefacts/Standalone/Monument.app`

**Notes:**
- Uses ccache if available for faster rebuilds
- Release build with optimizations enabled

---

### rebuild_and_install.sh

**Purpose:** Full rebuild with optional clean, then install plugins to system directories.

**Usage:**
```bash
# Rebuild and install all formats
./scripts/rebuild_and_install.sh all

# Rebuild and install VST3 only
./scripts/rebuild_and_install.sh vst

# Rebuild and install AU only
./scripts/rebuild_and_install.sh au

# Rebuild and install Standalone only
./scripts/rebuild_and_install.sh standalone

# Clean build (remove build directory first)
./scripts/rebuild_and_install.sh clean
```

**Target Options:**
- `all` - VST3, AU, and Standalone (default)
- `vst` - VST3 only
- `au` - AU only
- `standalone` - Standalone app only
- `clean` - Remove build directory first

**Installation Locations:**
- VST3: `~/Library/Audio/Plug-Ins/VST3/Monument.vst3`
- AU: `~/Library/Audio/Plug-Ins/Components/Monument.component`
- Standalone: `/Applications/Monument.app`

**Features:**
- Color-coded output (✓ = success, ✗ = error)
- Build verification before installation
- Selective target building
- Post-install verification

**Exit Codes:**
- `0` - Success
- `1` - Build failed
- `2` - Installation failed

---

### install_macos.sh

**Purpose:** Copy pre-built AU/VST3 bundles into user plugin folders (no rebuild).

**Usage:**
```bash
./scripts/install_macos.sh
```

**Requirements:**
- Pre-built plugins in `build-ninja/Monument_artefacts/` or `build/Monument_artefacts/`

**Installation Locations:**
- VST3: `~/Library/Audio/Plug-Ins/VST3/Monument.vst3`
- AU: `~/Library/Audio/Plug-Ins/Components/Monument.component`

**Notes:**
- Use this after manual builds if you don't need to rebuild
- Faster than `rebuild_and_install.sh` if build is up-to-date

---

### dev_loop.sh

**Purpose:** Development iteration loop - configure, build Debug, install, and optionally watch for changes.

**Usage:**
```bash
# Single build + install
./scripts/dev_loop.sh

# Watch mode (requires fswatch)
./scripts/dev_loop.sh --watch
```

**Features:**
- Debug build configuration
- Automatic installation after build
- Optional file watching (with `fswatch`)
- Incremental rebuilds on source changes

**Requirements:**
- `fswatch` (optional, for watch mode)
  ```bash
  brew install fswatch
  ```

**Watched Directories:**
- `Source/`
- `dsp/`
- `tests/`

**Notes:**
- Useful for rapid iteration during development
- Debug symbols enabled for debugging
- Automatically installs plugins after successful build

---

### open_xcode.sh

**Purpose:** Generate and open the Xcode project for Monument.

**Usage:**
```bash
./scripts/open_xcode.sh
```

**Output:**
- `build-xcode/Monument.xcodeproj`
- Opens in Xcode.app automatically

**Requirements:**
- Xcode installed
- CMake >= 3.21

**Notes:**
- Uses Xcode generator instead of Ninja
- Useful for debugging, code navigation, and Instruments integration
- Regenerates project if already exists

---

## Testing & CI Scripts

### run_ci_tests.sh

**Purpose:** **Master CI/CD script** - Comprehensive test suite for Monument Reverb.

**What It Does:**
1. **Dependency Check** - Verify cmake, python3, pyroomacoustics
2. **Build Verification** - Check monument_plugin_analyzer exists
3. **C++ Unit Tests** - Run all CTest tests
4. **Preset Capture** - Batch capture all 37 presets (parallel)
5. **Audio Analysis** - RT60 and frequency response analysis (parallel)
6. **Baseline Data Validation** - Verify integrity of captured test data
7. **Baseline Regression** - Compare against baseline data
8. **UI Tests** - Visual regression (optional)
9. **Report Generation** - JSON report + UI report

**Usage:**
```bash
# Run full test suite
./scripts/run_ci_tests.sh

# Custom regression threshold (default: 1%)
THRESHOLD=0.05 ./scripts/run_ci_tests.sh

# Custom baseline directory
BASELINE_DIR=./test-results/baseline-v2 ./scripts/run_ci_tests.sh

# Enable UI tests (visual regression)
ENABLE_UI_TESTS=1 ./scripts/run_ci_tests.sh
```

**Environment Variables:**
- `THRESHOLD` - Regression tolerance (default: `0.01` = 1%)
- `BASELINE_DIR` - Baseline location (default: `test-results/baseline-ci`)
- `BUILD_DIR` - Build directory for artifacts/tests (default: `build/` if present, otherwise `build-ninja/`)
- `ENABLE_UI_TESTS` - Enable visual regression (default: `0`)
- `ENABLE_RT_ALLOCATION_CHECK` - Enable real-time allocation gate (default: `0`)
- `TEST_CONFIG` - Build config to search for binaries and to pass to CTest (default: `Release`)
- `CTEST_CONTINUE_ON_FAILURE` - Continue after CTest failures (default: `0`, still exits non-zero)
- `CTEST_RERUN_FAILED` - Rerun only failed tests from last CTest run (default: `0`)
- `CTEST_FILTER` - Regex filter for CTest (default: empty)
- `CTEST_EXCLUDE` - Regex exclude for CTest (default: empty)
- `PRESET_CAPTURE_CONTINUE_ON_FAILURE` - Continue after preset capture failures (default: `0`, skips audio pipeline)
- `DSP_CONTINUE_ON_FAILURE` - Continue after critical DSP failures (default: `0`, still exits non-zero)

**Exit Codes:**
- `0` - All tests passed
- `1` - Tests failed (see output for details)
- `2` - Dependency error

**Output:**
- Console: Color-coded test results
- `regression-report.json` - Baseline comparison results
- `test-results/` - Captured audio, metrics, and analysis

**Execution Time:**
- ~3-4 minutes (with preset capture)
- ~1-2 minutes (C++ tests only, if presets already captured)

**CI Integration:**
- Used by `.github/workflows/ci.yml`
- Automated on push to main and pull requests

**Notes:**
- Requires monument_plugin_analyzer to be built
- Python dependencies must be installed (see [tools/plugin-analyzer/python/requirements.txt](../tools/plugin-analyzer/python/requirements.txt))
- Parallel execution optimized for 8-core systems
- Baseline validation checks data integrity (file structure, metadata, RT60, frequency metrics, audio files)
- Supports flexible schema validation for both old and new baseline formats

---

### capture_all_presets.sh

**Purpose:** Batch capture impulse responses for all 37 Monument factory presets.

**Usage:**
```bash
# Auto-detect CPU cores (capped at 8)
./scripts/capture_all_presets.sh

# Manual parallel jobs
PARALLEL_JOBS=4 ./scripts/capture_all_presets.sh

# Sequential execution (no parallelism)
PARALLEL_JOBS=1 ./scripts/capture_all_presets.sh
```

**Environment Variables:**
- `PARALLEL_JOBS` - Number of parallel capture jobs (default: auto-detect, max 8)

**Output:**
```
test-results/preset-baseline/
├── preset_00/
│   ├── wet.wav          # 24-bit, 30s reverb output
│   ├── dry.wav          # 24-bit, 30s input signal
│   └── metadata.json    # Capture parameters
├── preset_01/
│   └── ...
...
└── preset_36/
    └── ...
```

**Execution Time:**
- ~53 seconds (8 parallel jobs)
- ~9+ minutes (sequential)

**Features:**
- Parallel execution with GNU parallel or xargs
- Per-preset error handling
- Progress bar with preset names
- Metadata JSON for each capture

**Requirements:**
- `monument_plugin_analyzer` built (see [build_macos.sh](#build_macossh))
- Monument VST3 plugin built

**Notes:**
- Captures 30-second impulse responses per preset
- Uses impulse signal type (single-sample impulse)
- Output directory created if doesn't exist

---

### analyze_all_presets.sh

**Purpose:** Batch RT60 and frequency response analysis for all captured presets.

**Usage:**
```bash
# Auto-detect CPU cores (capped at 8)
./scripts/analyze_all_presets.sh

# Manual parallel jobs
PARALLEL_JOBS=4 ./scripts/analyze_all_presets.sh

# Sequential execution
PARALLEL_JOBS=1 ./scripts/analyze_all_presets.sh
```

**Environment Variables:**
- `PARALLEL_JOBS` - Number of parallel analysis jobs (default: auto-detect, max 8)

**Input:**
- `test-results/preset-baseline/preset_XX/wet.wav`

**Output:**
```
test-results/preset-baseline/preset_XX/
├── rt60_metrics.json          # RT60 per frequency band
├── freq_metrics.json          # Frequency response metrics
├── frequency_response.png     # Magnitude response plot
└── rt60_analysis.log          # Analysis log
```

**Execution Time:**
- ~30-45 seconds (8 parallel jobs)
- ~3-5 minutes (sequential)

**Features:**
- Parallel execution
- RT60 analysis via `rt60_analysis_robust.py` (with fallbacks)
- Frequency response analysis via `frequency_response.py`
- Automatic plot generation

**Requirements:**
- Python 3 with dependencies:
  ```bash
  cd tools/plugin-analyzer/python
  pip3 install -r requirements.txt
  ```
- Captured preset audio (see [capture_all_presets.sh](#capture_all_presetssh))

**Expected Ranges:**
- RT60: 4.85s - 29.85s (across all 37 presets)
- Frequency flatness: ≤±8.8dB

**Notes:**
- Uses robust RT60 analysis with multiple fallback methods
- Generates publication-quality plots
- JSON output compatible with baseline comparison tools

---

## Profiling Scripts

### profile_cpu.sh

**Purpose:** Automatic CPU profiling using Xcode Instruments (Time Profiler).

**Usage:**
```bash
./scripts/profile_cpu.sh
```

**What It Does:**
1. Launches Monument Standalone app
2. Profiles for 30 seconds (automatic)
3. Kills app and stops profiling
4. Exports trace to XML
5. Analyzes with `analyze_profile.py`

**Output:**
- `monument_profile.trace` - Binary trace file (can open in Instruments.app)
- `monument_profile_export.xml` - Exported trace data
- Console: Top CPU bottlenecks and module breakdown

**Requirements:**
- Xcode Command Line Tools (for `xctrace`)
- Monument Standalone app built

**Profile Duration:**
- 30 seconds (hardcoded)

**Use Cases:**
- Identify CPU hotspots
- Measure per-module CPU usage
- Verify SIMD optimizations
- Detect unexpected allocations

**Notes:**
- Fully automatic (no user interaction)
- Uses Time Profiler template
- Exports call tree with symbol resolution

---

### profile_with_audio.sh

**Purpose:** Interactive CPU profiling during active audio processing.

**Usage:**
```bash
./scripts/profile_with_audio.sh
```

**Workflow:**
1. Script launches Monument Standalone app
2. User manually configures audio input/output
3. User starts audio playback/processing
4. User presses ENTER when ready to profile
5. 30-second profiling session begins
6. Trace exported and analyzed automatically

**Output:**
- `monument_profile.trace` - Binary trace file
- `monument_profile_export.xml` - Exported trace data
- Console: CPU analysis results

**Use Cases:**
- Profile with real audio input
- Measure CPU under realistic load
- Test specific audio scenarios (e.g., high reverb density)
- Capture worst-case CPU usage

**Requirements:**
- Xcode Command Line Tools
- Monument Standalone app built
- Audio interface configured

**Notes:**
- User controls when profiling starts
- Captures real-world audio processing
- More representative than idle profiling

---

### profile_in_reaper.sh

**Purpose:** Profile Monument plugin within Reaper DAW for realistic host environment.

**Usage:**
```bash
./scripts/profile_in_reaper.sh
```

**Workflow:**
1. Script launches Reaper
2. User manually:
   - Creates audio track
   - Inserts Monument plugin
   - Loads preset
   - Starts playback
3. User presses ENTER when ready to profile
4. 30-second profiling session
5. Trace exported and analyzed

**Output:**
- `monument_profile.trace` - Binary trace file
- `monument_profile_export.xml` - Exported trace data
- Console: CPU analysis

**Use Cases:**
- Profile in production DAW environment
- Measure plugin overhead vs. standalone
- Test with DAW automation
- Verify thread safety in host

**Requirements:**
- Reaper DAW installed
- Monument VST3/AU built and installed
- Xcode Command Line Tools

**Notes:**
- Most realistic profiling scenario
- Captures host-plugin interaction overhead
- Recommended for final performance validation

---

### analyze_profile.py

**Purpose:** Parse Xcode trace XML export and identify CPU bottlenecks.

**Usage:**
```bash
# Analyze trace exported from Instruments
python3 scripts/analyze_profile.py monument_profile_export.xml

# Or let profile_*.sh scripts call it automatically
```

**What It Analyzes:**
- Top 30 functions by CPU time
- Per-module CPU breakdown:
  - TubeRayTracer
  - Chambers
  - ElasticHallway
  - AlienAmplification
  - MemoryEchoes
  - ModulationMatrix
  - MacroMapper
  - ExpressiveMacroMapper
  - DspRoutingGraph
  - AllpassDiffuser
  - ParameterSmoother
- Estimated CPU load percentage
- Optimization recommendations

**Output Format:**
```
Top CPU Bottlenecks:
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Function                          File                    Time (ms)  % of Total
──────────────────────────────────────────────────────────────────────────────
processBlock                      TubeRayTracer.cpp        1250.3    35.2%
...

DSP Module Breakdown:
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
Module                             Time (ms)    % of Total
──────────────────────────────────────────────────────────
TubeRayTracer                      1250.3       35.2%
...

Estimated Total CPU Load: 8.5% (at 512 samples/block, 48kHz)

Optimization Recommendations:
• TubeRayTracer: Consider SIMD optimization (>1% CPU)
```

**Requirements:**
- Python 3 (stdlib only, no external dependencies)
- Trace XML exported from Instruments

**Features:**
- Automatic source file extraction from stack traces
- Per-module aggregation
- CPU percentage estimation
- SIMD optimization recommendations

**Notes:**
- Works with Time Profiler traces only
- Requires symbol resolution in trace export
- Percentages relative to profiling duration

---

## Validation Scripts

### run_pluginval.sh

**Purpose:** Validate VST3/AU plugins using pluginval (Tracktion plugin validator).

**Usage:**
```bash
./scripts/run_pluginval.sh
```

**What It Does:**
1. Locates Monument VST3 plugin
2. Runs pluginval with strictness level 10 (maximum)
3. Tests:
   - Plugin loading
   - Parameter validation
   - State save/recall
   - Audio processing
   - Threading safety
   - Memory leaks
   - GUI instantiation
4. Runs CTest suite after pluginval
5. Generates report

**Output:**
- Console: pluginval test results
- `build/pluginval-report/` - Detailed report files

**Strictness Level:**
- Level 10 (maximum) - All tests enabled including:
  - Automation compatibility
  - Thread sanitizer
  - Undefined behavior checks
  - Memory leak detection

**Requirements:**
- `pluginval` installed:
  ```bash
  brew install pluginval
  ```
- Monument VST3 built

**Exit Codes:**
- `0` - All validations passed
- `1` - Validation failed

**Use Cases:**
- Pre-release validation
- CI/CD quality gate
- DAW compatibility testing
- Regression detection

**Notes:**
- Runs ~100+ automated tests
- Takes 2-5 minutes depending on plugin complexity
- Required for AAX certification (if targeting Pro Tools)

---

## Quality Gate Scripts

### check_cpu_thresholds.py

**Purpose:** Enforce per-module CPU performance budgets to prevent regressions.

**Usage:**
```bash
# Check CPU profile against thresholds
python3 tools/check_cpu_thresholds.py test-results/cpu_profile.json

# With custom threshold file
python3 tools/check_cpu_thresholds.py test-results/cpu_profile.json \
  --thresholds config/cpu_thresholds.json
```

**What It Checks:**
- Per-module CPU time percentage
- Total plugin CPU load
- Threshold violations with detailed diagnostics
- Missing baseline modules

**Threshold Configuration:**
```json
{
  "modules": {
    "TubeRayTracer": {"max_percent": 25.0, "severity": "error"},
    "Chambers": {"max_percent": 20.0, "severity": "error"},
    "ModulationMatrix": {"max_percent": 5.0, "severity": "warning"}
  },
  "total_cpu_percent": 60.0
}
```

**Severity Levels:**
- `error` - Hard limit, fails CI if exceeded
- `warning` - Soft limit, logs but doesn't fail CI

**Output:**
```
✓ TubeRayTracer: 18.2% (threshold: 25.0%)
✓ Chambers: 15.4% (threshold: 20.0%)
✗ ModulationMatrix: 7.3% (threshold: 5.0%) [ERROR]
  ↳ Exceeded by 2.3 percentage points
```

**Exit Codes:**
- `0` - All thresholds passed
- `1` - One or more thresholds exceeded (severity: error)
- `2` - Invalid input or configuration error

**CI Integration:**
```bash
# Generate profile first
./scripts/profile_cpu.sh

# Check thresholds (optional in CI, enabled only if profile exists)
python3 tools/check_cpu_thresholds.py test-results/cpu_profile.json
```

**Requirements:**
- Python 3.7+
- Valid CPU profile JSON (generated by profile_cpu.sh)

**Use Cases:**
- Performance regression prevention
- Per-module optimization tracking
- CI quality gates
- Performance budget enforcement

---

### check_audio_stability.py

**Purpose:** Detect numerical instability (NaN, Inf, denormals, DC offset) in audio output.

**Usage:**
```bash
# Check all presets in baseline directory
python3 tools/check_audio_stability.py test-results/preset-baseline

# Check single preset
python3 tools/check_audio_stability.py test-results/preset-baseline/preset_01

# Custom thresholds
python3 tools/check_audio_stability.py test-results/preset-baseline \
  --dc-threshold 0.01 \
  --denormal-threshold 1e-30
```

**What It Checks:**
1. **NaN (Not a Number)** - Indicates divide-by-zero or invalid math
2. **Infinity** - Numerical overflow or underflow
3. **Denormals** - Very small values causing CPU spikes
4. **DC Offset** - Unintended DC bias in signal

**Thresholds:**
- DC offset: |mean| > 0.001 (0.1%)
- Denormals: Any sample |x| < 1e-30

**Output:**
```
Checking preset_01/wet.wav...
  ✓ No NaN values detected
  ✓ No Inf values detected
  ✓ No denormals detected (threshold: 1e-30)
  ✓ DC offset: 0.000123 (threshold: 0.001)

Checking preset_02/wet.wav...
  ✗ NaN detected: 3 samples [ERROR]
  ✓ No Inf values detected
  ✓ No denormals detected
  ✗ DC offset: 0.00234 (threshold: 0.001) [WARNING]

Summary: 1/37 presets failed
```

**Exit Codes:**
- `0` - All presets passed
- `1` - One or more presets failed (NaN/Inf detected)
- `2` - Invalid input or file read error

**CI Integration:**
```bash
# Runs automatically in run_ci_tests.sh after preset capture
python3 tools/check_audio_stability.py test-results/preset-baseline
```

**Requirements:**
- Python 3.7+
- NumPy
- SciPy
- Valid audio files (WAV, 16/24/32-bit)

**Use Cases:**
- DSP bug detection
- Numerical stability validation
- Production quality gate
- CI/CD integration

**Notes:**
- Checks all `wet.wav` files in preset directories
- Fast execution (~1 second for 37 presets)
- Zero false positives (strict numerical checks)

---

### check_rt_allocations.sh

**Purpose:** Detect memory allocations in audio thread using Xcode's Memory Profiler.

**Usage:**
```bash
# Enable in CI (optional, can be slow)
ENABLE_RT_ALLOCATION_CHECK=1 ./scripts/run_ci_tests.sh

# Or run standalone
./tools/check_rt_allocations.sh

# Specify custom binary
./tools/check_rt_allocations.sh build/Monument_artefacts/Debug/Standalone/Monument.app
```

**What It Detects:**
- `malloc`, `calloc`, `realloc` calls in audio callback
- `new` operator usage
- `std::vector::push_back` reallocations
- STL container allocations

**How It Works:**
1. Launches Monument Standalone with Instruments
2. Enables "System Trace" template
3. Records 30 seconds of audio processing
4. Analyzes trace for allocations in audio thread
5. Reports violations with stack traces

**Output:**
```
Starting allocation detection for Monument...
Recording 30-second trace...
Analyzing trace for allocations...

✓ No real-time allocations detected
  Audio thread: 0 malloc/free calls
  Total allocations: 142 (all on message thread)

Summary: PASS - Audio thread is allocation-free
```

**Failure Example:**
```
✗ Real-time allocations detected: 5 malloc calls

Stack traces:
1. malloc() in std::vector::push_back()
   ↳ ModulationMatrix::getConnections() [ModulationMatrix.cpp:658]
   ↳ Called from audio thread

2. malloc() in std::vector::resize()
   ↳ DspRoutingGraph::loadRoutingPreset() [DspRoutingGraph.cpp:389]
   ↳ Called from audio thread

Summary: FAIL - Fix allocations before shipping
```

**Exit Codes:**
- `0` - No allocations detected
- `1` - Allocations detected in audio thread
- `2` - Tool error (Instruments not found, app not built)

**CI Integration:**
```bash
# Optional in CI (set environment variable to enable)
ENABLE_RT_ALLOCATION_CHECK=1 ./scripts/run_ci_tests.sh
```

**Requirements:**
- macOS only
- Xcode Command Line Tools
- Monument Standalone app built
- `xctrace` command available

**Performance:**
- Recording time: 30 seconds
- Analysis time: 10-30 seconds
- Total: ~1 minute per run

**Use Cases:**
- Real-time safety validation
- Audio glitch debugging
- Pre-release quality gate
- Performance optimization verification

**Notes:**
- Disabled by default (slow, macOS-only)
- Enable only for critical pre-release testing
- Requires user interaction to start audio
- Can produce false positives (analyze stack traces carefully)

---

### Phase 4 Enhanced Tests

**Location:** Steps 10-13 in CI pipeline
**Duration:** ~5 seconds total
**Purpose:** Validate production-ready behavior correctness

#### Tests

1. **Parameter Smoothing Test** (Warning Mode)
   - **Validates:** No clicks/pops during parameter changes
   - **Success Criteria:** THD+N < -60dB during sweeps
   - **Status:** ⚠ Needs threshold tuning (currently too strict for reverb)
   - **Test:** 10 macro parameters swept from 0.0 to 1.0

2. **Stereo Width Test** (Warning Mode)
   - **Validates:** Stereo processing and channel correlation
   - **Success Criteria:** Correlation 0.0-1.0, mono compatibility > -6dB
   - **Status:** ⚠ Needs investigation (correlation out of range)
   - **Test:** Both mono and stereo inputs

3. **Latency Test** (CRITICAL)
   - **Validates:** DAW Plugin Delay Compensation (PDC)
   - **Success Criteria:** Reported latency matches actual (±1 block)
   - **Status:** ✅ Passing (0 samples latency)
   - **Test:** Impulse response peak detection

4. **State Management Test** (Warning Mode)
   - **Validates:** Parameter save/restore for automation
   - **Success Criteria:** All parameters restored (< 0.001 tolerance)
   - **Status:** ⚠ Needs fixes (5 parameters fail to restore)
   - **Test:** Random values → save → change → restore → verify

#### Usage

```bash
# Run with verbose output (shows JUCE assertions)
MONUMENT_VERBOSE_TESTS=1 ./scripts/run_ci_tests.sh

# Run normally (suppresses stderr noise)
./scripts/run_ci_tests.sh

# Run Phase 4 tests individually
./build/monument_parameter_smoothing_test_artefacts/Debug/monument_parameter_smoothing_test
./build/monument_stereo_width_test_artefacts/Debug/monument_stereo_width_test
./build/monument_latency_test_artefacts/Debug/monument_latency_test
./build/monument_state_management_test_artefacts/Debug/monument_state_management_test
```

#### Current Issues

- **Parameter Smoothing:** All 46 tests report -16.1dB transient (threshold is -60dB). Likely needs relaxation to -40dB or -30dB for reverb plugins.
- **Stereo Width:** Stereo input correlation = -0.000 (expects 0.0-1.0). May indicate phase inversion or width processing issue.
- **State Management:** 5 parameters fail to restore with max error 0.245. Likely discrete parameters or modulation state.

---

## Workflow Examples

### Complete Development Workflow

```bash
# 1. Build and install plugin
./scripts/rebuild_and_install.sh all

# 2. Run comprehensive tests
./scripts/run_ci_tests.sh

# 3. If tests fail, profile to find issues
./scripts/profile_with_audio.sh

# 4. Validate with pluginval before release
./scripts/run_pluginval.sh
```

### Preset Development Workflow

```bash
# 1. Capture all presets
PARALLEL_JOBS=8 ./scripts/capture_all_presets.sh

# 2. Analyze audio characteristics
PARALLEL_JOBS=8 ./scripts/analyze_all_presets.sh

# 3. Compare against baseline
python3 tools/compare_baseline.py \
  test-results/baseline-v1.0.0 \
  test-results/preset-baseline \
  --threshold 0.01

# 4. Visualize preset distribution
python3 tools/plot_preset_comparison.py \
  test-results/preset-baseline \
  --output test-results/comparisons
```

### Performance Optimization Workflow

```bash
# 1. Profile with automatic timing
./scripts/profile_cpu.sh

# 2. Profile with real audio load
./scripts/profile_with_audio.sh

# 3. Profile in production DAW
./scripts/profile_in_reaper.sh

# 4. Compare results and identify hotspots
python3 scripts/analyze_profile.py monument_profile_export.xml

# 5. After optimization, validate no regressions
./scripts/run_ci_tests.sh
```

### CI/CD Integration

```bash
# .github/workflows/ci.yml calls:
./scripts/run_ci_tests.sh

# Which internally runs:
# - ctest (C++ unit tests)
# - capture_all_presets.sh (audio capture)
# - analyze_all_presets.sh (RT60 + freq)
# - compare_baseline.py (regression detection)
```

---

## Dependencies

### System Requirements

**macOS:**
- macOS 12.0+ (Monterey or later)
- Xcode Command Line Tools
- CMake >= 3.21
- Ninja (recommended) or Xcode build system

**Optional:**
- `fswatch` - For dev_loop.sh watch mode
- `pluginval` - For plugin validation
- Reaper - For DAW profiling

### Python Requirements

```bash
# Install Python dependencies for analysis tools
cd tools/plugin-analyzer/python
pip3 install -r requirements.txt
```

**Required packages:**
- numpy >= 1.20.0
- scipy >= 1.7.0
- pyroomacoustics >= 0.7.0
- matplotlib >= 3.5.0

### Build-Time Dependencies

- **JUCE framework** - Auto-downloaded via CMake FetchContent
- **VST3 SDK** - Included with JUCE
- **AU SDK** - macOS native

---

## Environment Variables

| Variable | Default | Description |
|----------|---------|-------------|
| `JUCE_SOURCE_DIR` | FetchContent | Path to JUCE source (optional) |
| `PARALLEL_JOBS` | Auto-detect | Number of parallel jobs (max 8) |
| `THRESHOLD` | 0.01 (1%) | Regression tolerance for baseline comparison |
| `BASELINE_DIR` | test-results/baseline-ci | Baseline data location |
| `ENABLE_UI_TESTS` | 0 | Enable visual regression tests |

---

## Troubleshooting

### "Command not found: xctrace"

**Solution:** Install Xcode Command Line Tools
```bash
xcode-select --install
```

### "Command not found: ninja"

**Solution:** Install Ninja build system
```bash
brew install ninja
```

### "pyroomacoustics not found"

**Solution:** Install Python dependencies
```bash
cd tools/plugin-analyzer/python
pip3 install -r requirements.txt
```

### Profiling fails with "No processes found"

**Solution:** Ensure Monument Standalone app is built
```bash
./scripts/rebuild_and_install.sh standalone
```

### Parallel capture/analysis hangs

**Solution:** Reduce parallel jobs
```bash
PARALLEL_JOBS=4 ./scripts/capture_all_presets.sh
PARALLEL_JOBS=4 ./scripts/analyze_all_presets.sh
```

### CI tests fail on baseline comparison

**Solution:** Baseline data may be stale or missing
```bash
# Regenerate baseline
./scripts/capture_all_presets.sh
./scripts/analyze_all_presets.sh

# Copy to baseline directory
mkdir -p test-results/baseline-ci
cp -r test-results/preset-baseline/* test-results/baseline-ci/
```

---

## Contributing

When adding new scripts:

1. **Follow naming convention:**
   - Use snake_case: `my_script.sh`
   - Descriptive names: `profile_cpu.sh` not `prof.sh`

2. **Add documentation:**
   - Update this README with usage and purpose
   - Add inline comments in script
   - Include usage examples

3. **Environment variables:**
   - Use ALL_CAPS for environment variables
   - Provide sensible defaults
   - Document in script header

4. **Error handling:**
   - Check for required tools/files
   - Provide clear error messages
   - Exit with meaningful exit codes (0=success, 1=error, 2=dependency)

5. **Color output:**
   - Use color for clarity (green=success, red=error, yellow=warning)
   - Ensure output works in CI (no-color mode)

---

## See Also

- [TESTING.md](../TESTING.md) - Testing hub (canonical)
- [docs/BUILD_PATTERNS.md](../docs/BUILD_PATTERNS.md) - Build system architecture
- [tools/plugin-analyzer/README.md](../tools/plugin-analyzer/README.md) - Plugin analyzer documentation
- [docs/testing/TESTING_AUDIT.md](../docs/testing/TESTING_AUDIT.md) - Testing infrastructure audit

---

**Last Updated:** 2026-01-08
