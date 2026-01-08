# 01072026 Performance, Memory, and Resource Review

Date: 2026-01-07
Scope: Runtime audio thread safety, CPU hot paths, memory usage, asset footprint, and test tooling overhead.

## Summary
The core DSP path is generally structured for real-time use, but there are a few hotspots and thread-safety risks that could cause audio glitches under load. The playground app is intentionally heavier (FFT + particles), but some optimizations would help if it becomes a shared runtime foundation.

## Findings (ordered by severity)

### High
1) **Routing preset changes can allocate on the audio thread**
- `plugin/PluginProcessor.cpp:300-304` calls `routingGraph.loadRoutingPreset(...)` in `processBlock()`.
- `dsp/DspRoutingGraph.cpp:259-331` clears and pushes to `routingConnections`, which can allocate and reallocate memory.
- Impact: potential audio dropouts when routing preset parameters change in realtime.

2) **SpinLock contention risk in the modulation matrix**
- `dsp/ModulationMatrix.cpp:455-470` holds `juce::SpinLock` while iterating connections.
- If the UI thread holds the lock (e.g., editing connections), the audio thread can busy-wait and miss deadlines.
- Impact: rare but severe glitch risk under heavy UI interaction.

### Medium
1) **Particle system uses RTTI in inner loop**
- `Source/Particles/ParticleSystem.cpp:205-212` uses `dynamic_cast` per particle per force.
- Impact: elevated CPU cost when particle counts are high; avoid RTTI in hot paths.

2) **Particle removal shifts the array per dead particle**
- `Source/Particles/ParticleSystem.cpp:232-236` uses `particles.remove(i)` while iterating.
- Impact: O(n) per removal; becomes visible at higher particle counts or shorter lifetimes.

3) **Playground FFT runs on the audio callback thread**
- `playground/AudioEngine.cpp:96-138` computes RMS and FFT in `processFft()` during the audio callback.
- Impact: acceptable for a playground, but could under-run at small buffers; consider off-thread analysis or decimation if promoted to production.

### Low
1) **Debug logging in the playground timer**
- `playground/MainComponent.cpp:573-584` logs metrics once per second. This is fine for development but should be gated if it becomes user-facing.

## Memory Leak Review
- No explicit leaks found in the reviewed components.
- Particle forces use `std::unique_ptr` and are rebuilt cleanly in `ParticleSystem::rebuildForceStack()`.
- No long-lived heap growth was detected via static inspection.

## Resource Usage Notes
- Asset packs under `assets/knob_*` are 512x512 PNGs; embedding them into `MonumentAssets` will increase plugin size if integrated.
- Playground loads assets and presets from disk at runtime; distribution builds should ensure path validity or embed assets.

## Recommendations
1) Make routing preset changes RT-safe (precompute and swap or move to message thread).
2) Replace SpinLock reads with a lock-free snapshot or double-buffered connection list.
3) Cache a direct pointer to `CurlNoiseForce` to avoid RTTI in the particle loop.
4) Use `removeQuick()` or swap-and-pop for particle removal.
5) If the playground becomes production UI, move FFT analysis off the audio callback or compute at a lower rate.

## Suggested Validation
- `./scripts/profile_cpu.sh` for CPU hotspots.
- `./scripts/profile_with_audio.sh` for realtime stability.
- `ctest --test-dir build -C Release` for smoke tests after changes.
