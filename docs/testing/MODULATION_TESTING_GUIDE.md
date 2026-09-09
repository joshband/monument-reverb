# Modulation Testing Guide

How to test Monument's `ModulationMatrix` system — the "living" preset
modulation sources (chaos, audio-follower, Brownian motion, envelope
tracking, LFOs, MIDI) that drive parameter destinations at block rate.

---

## Automated Tests (Run These First)

These are the durable, repeatable tests. Registered CTest targets require
configuring with `-DMONUMENT_ENABLE_TESTS=ON -DBUILD_TESTING=ON` (both OFF
by default) — see [TESTING.md](../../TESTING.md).

| CTest target | Covers |
|---|---|
| `monument_modulation_matrix_test` | Core modulation routing: sources, destinations, curve shaping, connection management |
| `monument_experimental_modulation_test` | Experimental modulation source/destination coverage |
| `monument_modulation_matrix_concurrency_stress_test` | Concurrency hazard between the message-thread connection-publish path and the audio-thread read path (see below) |

```sh
ctest --test-dir build -C Release -R modulation
```

### Concurrency stress test in detail

[tests/ModulationMatrixConcurrencyStressTest.cpp](../../tests/ModulationMatrixConcurrencyStressTest.cpp)
characterizes a specific hazard: `ModulationMatrix::publishConnectionsSnapshot()`'s
two-slot double-buffer has no reader-ownership handshake before a writer
reuses a slot, so a message-thread publish can overwrite data the audio
thread is concurrently reading. One thread stands in for the audio thread
(tight `process()` loop), the other stands in for the message thread
(mutating connections as fast as possible, deliberately faster than
realistic UI-driven rates). This is a best-effort stress test under a plain
build; it becomes a verified characterization under ThreadSanitizer:

```sh
cmake -S . -B build-tsan -DMONUMENT_ENABLE_TESTS=ON -DBUILD_TESTING=ON -DMONUMENT_TSAN=ON
cmake --build build-tsan --target monument_modulation_matrix_concurrency_stress_test
./build-tsan/monument_modulation_matrix_concurrency_stress_test_artefacts/*/monument_modulation_matrix_concurrency_stress_test
```

`MONUMENT_TSAN` is an `option()` defined next to this target in
`CMakeLists.txt`; it is OFF by default because TSan instrumentation slows
every other test target too, so build a dedicated directory for it rather
than enabling it in your normal test build.

---

## Manual / Listening Tests

Automated tests catch routing and concurrency regressions; they don't tell
you whether a modulation routing *sounds* right. For that, load the plugin
in a DAW and audition the factory "Living" presets, which exercise real
modulation connections end to end.

### How to find the Living presets

There is no single contiguous "Living presets" range — see
[PRESET_GALLERY.md](../PRESET_GALLERY.md) for the verified, current index
ranges and preset names. As of this writing, two groups carry hardcoded
modulation connections: presets 19-23 (Breathing Stone, Drifting Cathedral,
Chaos Hall, Living Pillars, Event Horizon Evolved) and presets 29-37
(Pulsing Cathedral, Dynamic Shimmer, Quantum Shimmer, Morphing Cathedral,
Fractal Space, Elastic Drift, Spectral Wander, Impossible Hall, Breathing
Chaos). Check `plugin/PresetManager.cpp`'s `kFactoryPresets` array directly
if you need the exact current list — presets get added over time.

### Suggested listening procedure

1. **A/B against a static preset.** Play audio through a related non-living
   preset (e.g. "Event Horizon"), then switch to its living counterpart
   (e.g. "Event Horizon Evolved"). Listen for the added motion: drift,
   wobble, or dynamic response that the static preset lacks.
2. **Match source material to preset character.** Percussive/vocal material
   exercises `AudioFollower`- and `EnvelopeTracker`-driven presets well;
   sustained pads and drones better reveal `BrownianMotion`- and
   `ChaosAttractor`-driven slow evolution.
3. **Listen for what should NOT happen:** aliasing/clicks/pops (block-rate
   modulation should be smooth), abrupt parameter jumps (smoothing should
   prevent this), or modulation so deep it overwhelms the underlying reverb
   character.
4. **Confirm chaos is deterministic.** `ChaosAttractor`-driven presets
   should produce patterns that evolve continuously rather than jumping
   randomly — if you hear discontinuities, that's a bug, not "chaos."

### Reading a preset's modulation routing

To see exactly what a given factory preset connects, read its
`makeModConnection(...)` call(s) in
[plugin/PresetManager.cpp](../../plugin/PresetManager.cpp) — each call
specifies `source`, `destination`, `depth`, `sourceAxis`, and
`smoothingMs`. For the full source/destination/curve enum surface, see
[dsp/ModulationMatrix.h](../../dsp/ModulationMatrix.h).

---

## Extending: Adding a New Modulation Source or Destination

1. Add the new enum value to `SourceType` or `DestinationType` in
   `dsp/ModulationMatrix.h`.
2. Implement the source's per-block update (see `ChaosAttractor`,
   `AudioFollower`, `BrownianMotion`, or `EnvelopeTracker` for patterns) or
   wire the new destination into the parameter-application path in
   `plugin/PluginProcessor.cpp`.
3. Add string round-trip cases to `sourceTypeToString()`/`stringToSourceType()`
   or `destinationTypeToString()`/`stringToDestinationType()` in
   `plugin/PresetManager.cpp` — a connection using an enum value with no
   string mapping will serialize as `"Unknown"` and fail to round-trip
   through a saved preset. See
   [PRESET_FORMAT.md](../presets/PRESET_FORMAT.md) for the full preset
   serialization contract.
4. Add or extend a case in `monument_modulation_matrix_test` /
   `monument_experimental_modulation_test` exercising the new source or
   destination.
5. Optionally, add a factory preset in `plugin/PresetManager.cpp` using
   `makeModConnection()`/`makePresetWithMod()` to make the new routing
   audible and discoverable.

---

## Results as of 2026-01-03 (Phase 3 completion, historical)

At the time the modulation system's first 5 "Living" presets shipped, CPU
overhead was measured on an M1 Mac at 48kHz/512 samples:

- `ModulationMatrix` processing: ~0.3-0.5% CPU total
- `ChaosAttractor`: ~0.1% (10 iterations/block)
- `AudioFollower`: ~0.05% (RMS + smoothing)
- `BrownianMotion`: ~0.02% (PRNG + smoothing)
- `EnvelopeTracker`: ~0.08% (peak/RMS + stage detection)
- Memory: ~2KB for `ModulationMatrix` + ~100 bytes/preset for stored connections

These numbers predate the later addition of LFO/MIDI sources, curve
shaping (v5 preset format), and the concurrency stress test, and have not
been re-measured since — treat them as a rough historical data point, not
a current guarantee. Re-run `monument_performance_benchmark` (see
[STRESS_TEST_PLAN.md](STRESS_TEST_PLAN.md)) for current numbers if you need
up-to-date figures.
