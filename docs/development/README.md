# Development Documentation

Development guides, build patterns, and workflow documentation for Monument Reverb.

## Quick Start Guides

### Essential Guides
- [QUICK_START_BLENDER_KNOBS.md](QUICK_START_BLENDER_KNOBS.md) - Generate photorealistic knobs with Blender
- [QUICK_START_MACRO_TESTING.md](QUICK_START_MACRO_TESTING.md) - Test macro control system
- [BUILD_PATTERNS.md](BUILD_PATTERNS.md) - Build system patterns and best practices

### Token Optimization (AI-Assisted Development)
- [TOKEN_OPTIMIZATION_STRATEGIES.md](TOKEN_OPTIMIZATION_STRATEGIES.md) - Cost management for Claude Code sessions
- [CLAUDE_MD_OPTIMIZATION_RESULTS.md](CLAUDE_MD_OPTIMIZATION_RESULTS.md) - Optimization results and savings

## Development Workflows

### Building Monument

```bash
# Standard build
./scripts/rebuild_and_install.sh all

# Build specific target
cmake --build build --target Monument_AU --config Release

# Run tests
./scripts/run_ci_tests.sh
```

### Blender Knob Generation

```bash
# Quick preview (64 samples, ~30sec/layer)
./scripts/run_blender_enhanced.sh --material granite --quick

# Production quality (256 samples, ~2-3min/layer)
./scripts/run_blender_enhanced.sh --material granite

# Preview composite
python3 scripts/preview_knob_composite_enhanced.py --material granite
```

### Testing Workflow

```bash
# Full CI test suite
./scripts/run_ci_tests.sh

# C++ tests only
ctest --test-dir build

# Capture all presets for regression testing
./scripts/capture_all_presets.sh
```

## Development Tools

### Scripts
- `scripts/rebuild_and_install.sh` - Build and install plugin
- `scripts/run_ci_tests.sh` - Run full test suite
- `scripts/run_blender_enhanced.sh` - Generate photorealistic knobs
- `scripts/capture_all_presets.sh` - Capture audio for all presets
- `scripts/analyze_all_presets.sh` - Analyze preset characteristics

### Analyzers
- `build/monument_plugin_analyzer` - Standalone plugin analyzer
- `scripts/analyze_experimental_presets.py` - Experimental preset analysis
- `scripts/generate_audio_demos.py` - Generate audio demonstrations

## Build System

### CMake Configuration

```bash
# Configure for macOS (Apple Silicon)
cmake -S . -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES=arm64

# Configure with testing enabled (default)
cmake -S . -B build -DMONUMENT_ENABLE_TESTS=ON

# Configure with profiling
cmake -S . -B build -DCMAKE_CXX_FLAGS="-DMONUMENT_TESTING=1"
```

### Build Targets
- `Monument` - Shared code
- `Monument_Standalone` - Standalone application
- `Monument_AU` - Audio Unit plugin
- `Monument_VST3` - VST3 plugin
- `Monument_All` - All targets
- `monument_*_test` - Individual C++ test executables

## Coding Standards

### Real-Time Safety Rules

**Never in audio callback (processBlock):**
- Memory allocation (`new`, `malloc`, `vector::push_back`)
- Locks/mutexes (`std::mutex`, `CriticalSection`)
- System calls (file I/O, logging, network)
- Exceptions (`throw`, `try/catch`)
- Unbounded loops or recursion

**Always:**
- Pre-allocate buffers in `prepareToPlay()`
- Use `juce::SmoothedValue` for parameter changes
- Use `ScopedNoDenormals` in processBlock
- Access parameters via `apvts.getRawParameterValue()->load()`

### Code Style
- Modern C++17
- Const correctness
- Descriptive naming
- Minimal comments (self-documenting code)
- Comment only non-obvious logic

## Performance Guidelines

### CPU Targets
- Total DSP: <15% at 48kHz, 512 samples (target: 10-12%)
- Individual modules: See performance baselines in [../PERFORMANCE_BASELINE.md](../PERFORMANCE_BASELINE.md)

### Memory Targets
- Total footprint: ~2.0 MB per instance
- Delay lines: Pre-allocated in prepareToPlay()
- No dynamic allocation in audio thread

## Integration Testing

### DAW Testing Checklist
- Load plugin in Ableton Live, Logic Pro, Reaper, FL Studio
- Test all 37 presets
- Verify no audio glitches during preset switching
- Check CPU usage in performance monitor
- Test automation of all parameters
- Verify freeze functionality
- Test modulation matrix connections

### Validation
- Run `pluginval` for automated validation
- Check for NaN/denormal flags
- Verify parameter automation
- Stress test with 50-100 instances

## Asset Management

### UI Assets
- Knob renders: `assets/ui/knobs_enhanced/`
- Celestial assets: `assets/ui/celestial/`
- Visual profiles: `assets/ui/visual_profiles.json`
- Macro hints: `assets/ui/macro_hints.json`

### Binary Data
All assets are embedded in the binary via `juce_add_binary_data()` in CMakeLists.txt

## Related Documentation

- [../README.md](../../README.md) - Project overview
- [../architecture/README.md](../architecture/README.md) - Architecture documentation
- [../testing/README.md](../testing/README.md) - Testing guides
- [../ui/README.md](../ui/README.md) - UI development
- [../../STANDARD_BUILD_WORKFLOW.md](../../STANDARD_BUILD_WORKFLOW.md) - Build workflow (root)
