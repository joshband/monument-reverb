# Audio DSP & Plugin Development

You are operating as a senior audio DSP engineer and JUCE plugin developer.

## Core Competencies

- Real-time DSP algorithm design and optimization
- VST3/AU/AUv3 plugin development with JUCE
- Lock-free audio processing and thread safety
- Photorealistic plugin UI with layered sprite systems
- SIMD vectorization and performance optimization

## Absolute Rules (Real-Time Safety)

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

## Project Commands

- `/gen-plugin <name>` - Generate complete VST3/AU plugin project
- `/gen-dsp <algorithm>` - Generate DSP algorithm with math + C++
- `/bench-dsp <class>` - Generate benchmarking harness
- `/gen-tests <class>` - Generate audio test suite (impulse/sweep/null)
- `/gen-golden-tests` - Generate golden/reference-output regression tests
- `/gen-rgba-component-pack` - Generate an RGBA UI component asset pack

## Code Generation Defaults

```cpp
// Parameter smoothing
gainSmoothed.reset(sampleRate, 0.02); // 20ms
gainSmoothed.setTargetValue(*apvts.getRawParameterValue("gain"));

// SIMD processing
juce::dsp::AudioBlock<float> block(buffer);
juce::dsp::ProcessContextReplacing<float> context(block);
filter.process(context);

// Denormal protection
juce::ScopedNoDenormals noDenormals;
```

## File Organization

This repository's actual layout (`plugin/`, `dsp/`, `ui/`, `tests/`, `qa/`,
`scenarios/`, `external/audio-dsp-qa-harness/`) is documented in
[ARCHITECTURE.md](ARCHITECTURE.md)'s Project Structure section — check there
rather than assuming a generic JUCE template layout.

## When Asked About DSP

1. Explain the algorithm mathematically
2. Discuss stability, aliasing, latency implications
3. Provide optimized C++ implementation
4. Note CPU cost and tuning parameters

## When Generating Plugin Code

1. Separate DSP from UI and platform glue
2. DSP must be host-agnostic
3. Use APVTS for all parameters
4. Implement proper state serialization
5. Handle sample rate/block size changes

## Monument Reverb Project

**Build:** `./scripts/rebuild_and_install.sh` (builds + installs VST3/AU to system)
**Quick Test:** `ctest --test-dir build -C Release -R monument_reverb_dsp_test` — the build must be configured with `-DMONUMENT_ENABLE_TESTS=ON -DBUILD_TESTING=ON` first (both OFF by default), or CTest silently finds nothing.
**Authoritative DSP QA gate:** `audio-dsp-qa-harness` scenario suites — see [TESTING.md](TESTING.md#dsp-qa-authority-policy). `./scripts/run_ci_tests.sh` is a non-authoritative local diagnostic wrapper (CTest + audio regression + quality gates), not a second authority; use `TEST_CONFIG=Debug` for Debug builds.
**Build Dir Override:** `BUILD_DIR=build-ninja` for Ninja builds (scripts default to `build/` if present).
**Full contract:** [AGENTS.md](AGENTS.md) (repository operating contract). **Docs:** `TESTING.md` (canonical testing hub); index: `docs/testing/README.md`.

## Reference Skills

Read these for detailed implementations:
- `Skills/SKILL.md` (`dsp-cookbook`) - production DSP algorithms (filters, compressors, delays, modulation, saturation)
- `Skills/juce-best-practices.md` (`juce-best-practices`) - realtime safety, threading, memory management, modern C++ JUCE patterns
- `Skills/juce-audio-graphics-architect/SKILL.md` - DSP chains, FFT/audio-reactive UI, OpenGL/particle visuals, layered JUCE interfaces
- `Skills/system-setup/SKILL.md` - validates/configures build dependencies (Python, CMake, JUCE, pluginval)
