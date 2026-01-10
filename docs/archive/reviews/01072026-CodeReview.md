# 01072026 Code Review

Date: 2026-01-07
Scope: plugin/, dsp/, ui/, playground/, Source/Particles/, tests/, CMakeLists.txt

## Summary
The codebase is generally well-structured and consistent with JUCE conventions, but there are a few correctness and real-time safety risks that should be addressed before shipping. The most urgent items relate to audio-thread safety and a potential null dereference in the playhead handling.

## Findings (ordered by severity)

### Critical
1) Possible null dereference of the host playhead pointer.
- `plugin/PluginProcessor.cpp:265` calls `getPlayHead()->getPosition()` without checking for null. Some hosts return `nullptr` for the playhead, which would crash the plugin during audio processing.
- Recommendation: guard `getPlayHead()` and pass an empty `juce::Optional<PositionInfo>` when unavailable.

### High
1) Real-time unsafe routing preset mutation on the audio thread.
- `plugin/PluginProcessor.cpp:300-304` invokes `routingGraph.loadRoutingPreset(...)` inside `processBlock()`.
- `dsp/DspRoutingGraph.cpp:259-331` clears and pushes into `routingConnections`, which can allocate and is not RT-safe. This can cause audio dropouts when routing preset changes.
- Recommendation: precompute routing graphs, or swap in a preallocated graph outside the audio thread (message thread + lock-free atomic pointer swap).

2) Test invocation path is hard-coded to Debug artifacts.
- `CMakeLists.txt:288` uses `monument_smoke_test_artefacts/Debug/monument_smoke_test`, which fails for Release or non-Debug CTest runs.
- Recommendation: use generator expressions (e.g. `$<CONFIG>`) or run `ctest -C` with config-aware target paths.

### Medium
1) Particle system uses RTTI in the hot loop.
- `Source/Particles/ParticleSystem.cpp:205-212` performs `dynamic_cast` per particle per force when modulation overrides are active.
- Recommendation: cache a pointer to `CurlNoiseForce` during `rebuildForceStack()` or add a flag to avoid RTTI inside the inner loop.

2) Particle removal uses order-preserving removal inside the update loop.
- `Source/Particles/ParticleSystem.cpp:232-236` uses `particles.remove(i)` which shifts the array on each removal.
- Recommendation: use `removeQuick(i)` or a swap-and-pop strategy since particle ordering is not user-visible.

3) Sequence parameter parsing silently maps unknown strings to `Time`.
- `dsp/SequenceScheduler.cpp:144-173` returns `ParameterId::Time` on unknown strings, potentially masking data errors in sequence files.
- Recommendation: return an invalid optional/enum value or log and ignore unknown keys.

### Low
1) Routing graph TODOs for cycle detection and topological ordering remain unimplemented.
- `dsp/DspRoutingGraph.cpp:577-585` indicates incomplete validation. It is safe today because presets are hard-coded, but becomes a risk if user-authored graphs are added.

## Strengths
- Clear separation of DSP, UI, and playground utilities.
- Test infrastructure expanded to instantiate the processor in a console app.
- Particle system code is readable and modular (forces, behaviors, presets).

## Test Coverage & Gaps
- No automated tests were executed for this review.
- CTest coverage exists (`monument_smoke_test`, `monument_memory_echoes_test`) but `monument_smoke_test` invocation path is config-specific (see High findings).

## Recommended Next Actions
1) Fix the playhead null dereference in `plugin/PluginProcessor.cpp`.
2) Make routing preset changes RT-safe (precompute/swap).
3) Fix CTest command path to work across build configurations.
4) Optimize particle force modulation path to avoid RTTI and O(n) removal.
