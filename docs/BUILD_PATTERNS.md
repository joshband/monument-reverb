# JUCE + CMake Build Patterns for Monument Reverb

**Purpose:** Document proven build patterns to avoid repeating mistakes.
**Last Updated:** 2026-01-05

---

## Core Principles

1. **JUCE plugins need `JuceHeader.h`** - Generated only by `juce_add_plugin()` or `juce_add_console_app()`
2. **Don't link to `Monument_SharedCode`** - It's an internal target, use explicit source files instead
3. **Test DSP modules in isolation** - Avoid full plugin instantiation in unit tests
4. **Leverage existing Python tools** - Don't reimplement what already works

---

## Pattern 1: Simple DSP Component Test (No Plugin Dependencies)

**Use When:** Testing a single DSP class with minimal dependencies

**Example:** [tests/DopplerShiftTest.cpp](../tests/DopplerShiftTest.cpp)

```cmake
# Simple executable with just the DSP files needed
if(BUILD_TESTING)
  add_executable(monument_doppler_shift_test
    tests/DopplerShiftTest.cpp
    dsp/SpatialProcessor.cpp  # Only include what you need
  )
  target_include_directories(monument_doppler_shift_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
  )
  target_link_libraries(monument_doppler_shift_test PRIVATE
      juce::juce_audio_basics
      juce::juce_core
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )
  add_test(NAME monument_doppler_shift_test COMMAND monument_doppler_shift_test)
endif()
```

**Key Points:**
- ✅ Use `add_executable()`, not `juce_add_console_app()`
- ✅ Only include DSP `.cpp` files you actually need
- ✅ No `JuceHeader.h` required if DSP headers don't use it
- ✅ Link only necessary JUCE modules (`juce::juce_audio_basics`, `juce::juce_core`)

**When This Fails:**
- ❌ If DSP headers include `<JuceHeader.h>` (most do in Monument)
- ❌ If DSP class needs JUCE GUI components
- ❌ If class depends on many other Monument classes

---

## Pattern 2: DSP Test with JuceHeader.h Requirements

**Use When:** DSP classes include `<JuceHeader.h>` but don't need full plugin

**Example:** [tests/SequenceSchedulerTest.cpp](../tests/SequenceSchedulerTest.cpp)

```cmake
# Must use juce_add_console_app to generate JuceHeader.h
if(BUILD_TESTING)
  juce_add_console_app(monument_sequence_scheduler_test
    PRODUCT_NAME "Monument Sequence Scheduler Test"
  )
  juce_generate_juce_header(monument_sequence_scheduler_test)
  target_sources(monument_sequence_scheduler_test
    PRIVATE
      tests/SequenceSchedulerTest.cpp
      dsp/SequenceScheduler.cpp
      dsp/SequencePresets.cpp
  )
  target_include_directories(monument_sequence_scheduler_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
  )
  target_compile_definitions(monument_sequence_scheduler_test
    PRIVATE
      MONUMENT_TESTING=1
  )
  target_link_libraries(monument_sequence_scheduler_test PRIVATE
      juce::juce_audio_basics
      juce::juce_core
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )
  add_test(NAME monument_sequence_scheduler_test COMMAND monument_sequence_scheduler_test)
endif()
```

**Key Points:**
- ✅ Use `juce_add_console_app()` to generate `JuceHeader.h`
- ✅ Call `juce_generate_juce_header()` immediately after
- ✅ Include only needed DSP source files in `target_sources()`
- ✅ No need for `Monument_SharedCode` or `MonumentAssets`

---

## Pattern 3: Full Plugin Test (Requires PluginProcessor)

**Use When:** Need to instantiate the full MonumentAudioProcessor

**Status:** ⚠️ **Complex - Use sparingly**

**Why Use Sparingly:**
1. Requires ALL plugin sources (20+ files)
2. Needs `MonumentAssets` binary data target
3. Requires plugin-specific defines (`JucePlugin_Name`, etc.)
4. Slow compile times
5. Breaks easily when plugin structure changes

**Current Usage:**
- `monument_smoke_test` in `CMakeLists.txt` follows this pattern and is now part of CTest.

**Alternative Approach:**
Use the **plugin analyzer tool** programmatically instead:

```bash
# Test preset loading via analyzer tool (already works)
./build/monument_plugin_analyzer_artefacts/Debug/monument_plugin_analyzer \
  --plugin ~/Library/Audio/Plug-Ins/VST3/Monument.vst3 \
  --preset 0 \
  --duration 5

# Wrap in shell script for automated testing
```

**If You Must Test Full Plugin in C++:**

```cmake
# WARNING: This pattern is fragile and should be avoided
if(BUILD_TESTING)
  juce_add_console_app(monument_full_plugin_test
    PRODUCT_NAME "Monument Full Plugin Test"
  )
  juce_generate_juce_header(monument_full_plugin_test)

  # Must include ALL plugin sources (maintenance nightmare)
  target_sources(monument_full_plugin_test
    PRIVATE
      tests/FullPluginTest.cpp
      plugin/PluginProcessor.cpp
      plugin/PresetManager.cpp
      # ... 15+ more DSP files ...
  )

  target_compile_definitions(monument_full_plugin_test
    PRIVATE
      MONUMENT_TESTING=1
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
      # Still missing plugin-specific defines!
  )

  target_link_libraries(monument_full_plugin_test
    PRIVATE
      MonumentAssets  # Binary data target
      juce::juce_audio_processors
      juce::juce_audio_utils
      juce::juce_dsp
    PUBLIC
      juce::juce_recommended_config_flags
  )
endif()
```

**Better Solution:** Use Python + plugin analyzer tool (Pattern 4)

---

## Pattern 4: Integration Tests via Python + Plugin Analyzer

**Use When:** Testing RT60, frequency response, preset loading, audio regression

**Example:** [scripts/run_ci_tests.sh](../scripts/run_ci_tests.sh)

```bash
#!/bin/bash
# Leverage existing tools instead of rewriting in C++

# 1. Capture all presets using plugin analyzer
./scripts/capture_all_presets.sh

# 2. Analyze with Python (RT60, frequency response)
./scripts/analyze_all_presets.sh

# 3. Compare against baseline
python3 tools/compare_baseline.py \
  test-results/baseline-v1.0.0 \
  test-results/preset-baseline \
  --threshold 0.001

# Exit with proper code for CI
```

**Advantages:**
- ✅ No build complexity
- ✅ Reuses working tools
- ✅ Fast execution (parallel processing)
- ✅ Easy to understand and maintain
- ✅ Works in CI/CD out of the box

---

## Common Mistakes & Solutions

### Mistake 1: Trying to link against `Monument_SharedCode`

**❌ Wrong:**
```cmake
target_link_libraries(my_test PRIVATE Monument_SharedCode)
```

**Error:**
```
ld: library 'Monument_SharedCode' not found
```

**✅ Correct:**
Include the specific source files you need:
```cmake
target_sources(my_test PRIVATE dsp/Chambers.cpp dsp/AllpassDiffuser.cpp)
```

---

### Mistake 2: `JuceHeader.h` not found

**❌ Wrong:**
```cmake
add_executable(my_test tests/MyTest.cpp dsp/SomeModule.cpp)
```

**Error:**
```
fatal error: 'JuceHeader.h' file not found
```

**✅ Correct:**
```cmake
juce_add_console_app(my_test PRODUCT_NAME "My Test")
juce_generate_juce_header(my_test)
target_sources(my_test PRIVATE tests/MyTest.cpp dsp/SomeModule.cpp)
```

---

### Mistake 3: Wrong `prepare()` arguments

**❌ Wrong:**
```cpp
Chambers chambers;
chambers.setSampleRate(48000.0);  // No such method!
chambers.prepare(48000.0, 512);    // Wrong signature!
```

**✅ Correct:**
```cpp
Chambers chambers;
chambers.prepare(48000.0, 512, 2);  // sampleRate, blockSize, numChannels
```

**Lesson:** Always check the actual API in the header file!

---

### Mistake 4: Assuming DSP module has `process(left, right, size)`

**❌ Wrong:**
```cpp
Chambers chambers;
chambers.process(leftPtr, rightPtr, blockSize);  // Doesn't exist!
```

**✅ Correct:**
Check the header - most Monument DSP modules use JUCE AudioBuffer:
```cpp
Chambers chambers;
juce::AudioBuffer<float> buffer(2, blockSize);
// chambers.process() likely takes a reference to AudioBuffer
```

**Lesson:** Check the actual `process()` signature before writing tests!

---

## Testing Philosophy for Monument

### ✅ Good Test Practices

1. **Test individual DSP algorithms in isolation**
   - Example: Test Doppler shift calculation (Phase 3)
   - Example: Test keyframe interpolation (Phase 4)

2. **Use Python for audio quality metrics**
   - RT60 measurement (pyroomacoustics)
   - Frequency response (FFT + octave bands)
   - Audio regression (waveform comparison)

3. **Use plugin analyzer for integration tests**
   - Preset loading
   - Multi-preset batch testing
   - DAW compatibility

4. **Keep C++ tests simple and focused**
   - No full plugin instantiation
   - No complex dependency chains
   - Fast compile and run times

### ❌ Anti-Patterns to Avoid

1. **Don't reimplement working Python tools in C++**
   - RT60 analysis ✅ in Python, ❌ in C++
   - Frequency response ✅ in Python, ❌ in C++

2. **Don't test full plugin in unit tests**
   - Use plugin analyzer tool instead
   - Wrap in shell scripts for automation

3. **Don't assume DSP APIs without checking headers**
   - Always read the actual `.h` file first
   - API assumptions waste hours of debugging

4. **Don't fight CMake/JUCE build system**
   - Use `juce_add_console_app()` when you need JuceHeader.h
   - Include explicit source files, don't link to SharedCode

---

## Quick Decision Tree

```
Need to test Monument functionality?
│
├─ Testing DSP algorithm in isolation?
│  ├─ DSP headers include <JuceHeader.h>?
│  │  ├─ YES → Use Pattern 2 (juce_add_console_app)
│  │  └─ NO  → Use Pattern 1 (simple add_executable)
│  └─
│
├─ Testing audio quality (RT60, frequency)?
│  └─ Use Pattern 4 (Python + plugin analyzer)
│
├─ Testing preset loading?
│  └─ Use Pattern 4 (plugin analyzer tool)
│
└─ Testing full plugin behavior?
   └─ Use Pattern 4 (Python + plugin analyzer)
      Don't use Pattern 3 (C++ full plugin test)
```

---

## Build Speed Optimization

### Use Ninja (30-50% faster than Make)

```bash
# One-time switch to Ninja
rm -rf build
cmake -B build -G "Ninja Multi-Config"

# Build as usual
cmake --build build --target Monument_All -j8
```

### Incremental Builds

```bash
# After code changes, CMake automatically detects what to rebuild
cmake --build build --target monument_sequence_scheduler_test -j8

# No need for `make clean` unless you have build corruption
```

---

## Summary

| Pattern | Use Case | Complexity | Compile Time |
|---------|----------|------------|--------------|
| **Pattern 1** | Simple DSP test | Low | <10s |
| **Pattern 2** | DSP test with JuceHeader | Medium | ~30s |
| **Pattern 3** | Full plugin test | ⚠️ High | ~2min |
| **Pattern 4** | Integration/quality tests | Low | N/A (Python) |

**Recommendation:** Use Pattern 4 (Python) for 90% of tests. Use Pattern 2 for focused C++ unit tests.

---

**See Also:**
- [TESTING_GUIDE.md](TESTING_GUIDE.md) - Complete testing documentation
- [STANDARD_BUILD_WORKFLOW.md](../STANDARD_BUILD_WORKFLOW.md) - Build workflow
- [tools/COMPREHENSIVE_TEST_PLAN.md](../tools/COMPREHENSIVE_TEST_PLAN.md) - Original test plan
