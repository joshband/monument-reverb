# Monument Reverb - Architecture Overview

> **Quick Reference**: See [docs/architecture/ARCHITECTURE_QUICK_REFERENCE.md](docs/architecture/ARCHITECTURE_QUICK_REFERENCE.md) for visual diagrams and fast navigation.
>
> **Detailed Review**: See [docs/architecture/ARCHITECTURE_REVIEW.md](docs/architecture/ARCHITECTURE_REVIEW.md) for a deeper architectural review. Long-form docs under `docs/` describe code and sometimes drift from it faster than this file does — when they disagree, trust the source and this file's citations.

---

## System Architecture

Monument is a JUCE 8.0 reverb plugin (VST3/AU/Standalone) built in C++17, coordinating a fixed DSP signal chain, two selectable macro-control systems, and a modulation matrix. Only macOS (arm64/Intel) is built and tested by this repository's scripts and CI; JUCE's format declarations are not evidence of Windows/Linux support.

### Reachable signal chain (`AncientWay`, the default)

```
Host audio + APVTS atomics
  → processor block snapshot
  → timeline overrides
  → selected macro mapper + influence blend
  → modulation sources/connections
  → smoothing and ParameterBuffer views
  → module parameter setters
  → Foundation → Pillars → Chambers → Weathering
       → TubeRayTracer → ElasticHallway → AlienAmplification
       → Buttress → Facade
  → mode transition gain
  → equal-power mix with preserved dry input
  → preset transition gain/reset and optional output soft clip
  → host output and atomic metering
```

`processBlock()` dispatches through `ProcessingMode` to one of three fixed chain
implementations in `DspRoutingGraph`. No caller of `setProcessingMode()` was
found outside the initialized default, so `AncientWay` (above) is the only
demonstrated ordinary runtime path — treat the other two as unverified.
(`dsp/DspRoutingGraph.cpp`, `processAncientWay()`.)

**Routing preset — bypass mask only:** the host-visible `routingPreset`
parameter publishes an index and a bypass mask that the fixed chains honor
(some modules can be bypassed). `DspRoutingGraph` previously also carried a
generic graph executor (`DspRoutingGraph::process()`, implementing
series/parallel/feedback/crossfeed topologies) that a routing preset's name
(e.g. "Parallel Universe") described but the processor never called — it was
removed as dead code, since nothing in the plugin ever executed that path.
A routing preset's name is now purely historical/cosmetic; only its bypass
effect is real, and that effect remains a preserved behavioral contract.

**Memory Echoes is prepared but not reached:** `MemoryEchoes::prepare/reset`
and its parameter setters run every block, and its host parameter IDs remain
a compatibility surface, but `memoryEchoes.process()` is never called in
ordinary builds — its recall buffer is only mixed to output behind the
non-production `MONUMENT_MEMORY_PROVE` debug flag
(`plugin/PluginProcessor.cpp` around the memory parameter setters and the
`#if defined(MONUMENT_MEMORY_PROVE)` block). Do not describe Memory Echoes as
part of the rendered sound, and do not remove it as "dead code" without a
separate product decision — it is disconnected, not deleted.

### Macro control (two independently selectable systems, `macroMode` parameter)

- **Mode 0 - Ancient macros** (`dsp/MacroMapper.h/cpp`): `material` (Stone),
  `topology` (Labyrinth), `viscosity` (Mist), `evolution` (Bloom),
  `chaosIntensity` (Tempest), `elasticityDecay` (Echo) drive the core/physical
  parameters.
- **Mode 1 - Expressive macros** (`dsp/ExpressiveMacroMapper.h/cpp`):
  `breath`, `character`, `spaceType`, `energy`, `motion`, `color`, `dimension`.

Both are live, selectable control models — keep both; do not silently prefer
one during cleanup.

### Modulation matrix

`dsp/ModulationMatrix.h/cpp` routes 4 sources (Chaos Attractor, Audio
Follower, Brownian Motion, Envelope Tracker) to 27 destinations
(`DestinationType` enum: core parameters, physical-modeling parameters,
spatial position/Doppler). Publication uses two snapshot slots without a
reader-ownership handshake before a writer reuses a slot — this is a
source-derived concurrency hazard, not something proven safe by absence of a
`std::mutex`/`CriticalSection` in the DSP sources.

### Physical/algorithmic modules

- **TubeRayTracer** - metal-tube resonance modeling; `tubeCount`,
  `radiusVariation`, `metallicResonance`, `couplingStrength`. Changing tube
  count triggers `reconfigureTubes()` on the audio thread, which allocates
  (modal-frequency vector, IIR coefficients) — proven by
  `tests/RealtimeAllocationCharacterizationTest.cpp` (see `TESTING.md`).
- **ElasticHallway** - walls deform under acoustic pressure and slowly
  recover; `elasticity`, `recoveryTime`, `absorptionDrift`.
- **AlienAmplification** - nonlinear/impossible-physics saturation stage.

---

## Preset and state ownership

- **Factory programs**: `PresetManager` holds 37 factory entries
  (`plugin/PresetManager.h`, `kNumFactoryPresets`); JUCE program APIs delegate
  to these. Keep index order and parameter mappings.
- **User presets**: JSON format version 5, written by `PresetManager`, capture
  a specific parameter subset plus modulation connections. This subset omits
  some controls (e.g. expressive macros, routing selection, timeline
  selection) — it is not a full-state snapshot.
- **Host session state**: `getStateInformation()`/`setStateInformation()`
  save/restore only `parameters.copyState()` as XML — the APVTS tree. Custom
  modulation connections and other non-APVTS runtime state are not part of
  host session state.
- User preset JSON and host session state are **different, incomplete
  contracts** — do not assume one implies the other, and characterize both
  before extracting or refactoring `PresetManager`.

## UI ownership

`createEditor()` unconditionally returns `MonumentAudioProcessorEditorV2`
(`plugin/PluginProcessor.cpp`). The legacy `PluginEditor` and the
`MONUMENT_LEGACY_UI` build option were removed (see
`docs/architecture/EDITOR_PARITY_FINDINGS.md` for the parity investigation
that preceded the decision); V2 exposes parameter, modulation, and timeline
sections and dynamically loads knob-layer assets from environment-selected
directories at runtime (not bundled via `BinaryData`) — packaging must be
verified against a real installed build, not assumed from source layout.

Do not add a third editor generation or treat `plugin/PluginEditor_NEW.h`
(no build/include consumer found) as live.

## Project Structure

```
monument-reverb/
├── plugin/                 # JUCE processor, preset manager, editor
│   ├── PluginProcessor.cpp/h
│   ├── PluginEditorV2.cpp/h  # the only editor
│   └── PresetManager.cpp/h
├── ui/                      # Reusable UI controls (PhotorealisticKnob, etc.)
├── dsp/                     # DSP algorithms and routing graph
│   ├── DspRoutingGraph.cpp/h  # fixed-chain dispatch + preset bypass-mask
│   ├── MacroMapper.h/cpp       # Ancient macros
│   ├── ExpressiveMacroMapper.h/cpp
│   ├── ModulationMatrix.h/cpp
│   ├── TubeRayTracer.h/cpp
│   ├── ElasticHallway.h/cpp
│   └── MemoryEchoes.h/cpp     # prepared, not reached by ordinary processing
├── assets/ui/               # Knob layer PNGs (runtime-loaded, not bundled)
├── qa/                      # Processor adapter + CLI for the QA harness
├── scenarios/monument/      # QA harness scenario definitions
├── external/audio-dsp-qa-harness/  # pinned upstream QA dependency
├── tests/                   # CTest coverage (see TESTING.md for gating flags)
├── scripts/                 # Build, install, and profiling scripts
├── docs/                    # Documentation (mixed currency — verify against source)
└── build/                   # CMake build output (gitignored)
```

---

## Build System

**Standard Build**: See [STANDARD_BUILD_WORKFLOW.md](STANDARD_BUILD_WORKFLOW.md)

```bash
cmake --build build --target Monument_AU --config Release -j8
```

**Auto-installs to** (when `MONUMENT_COPY_PLUGIN_AFTER_BUILD` is ON, the default):
- `~/Library/Audio/Plug-Ins/Components/Monument.component` (AU)
- `~/Library/Audio/Plug-Ins/VST3/Monument.vst3` (VST3)

---

## Key Technologies

- **JUCE 8.0** - Audio plugin framework
- **CMake** - Build system
- **C++17** - Language standard

---

## Realtime-safety limitations (read before trusting a green check)

- `MONUMENT_TESTING` is test-access/editor-suppression scaffolding, not a
  production-equivalent build — it also enables string construction and file
  logging inside the audio callback, which distorts any allocation
  measurement taken under it.
- The QA harness's allocation counter wraps global `new`/`delete` only,
  including warmup; it does not cover `malloc`/`calloc` or every
  platform/thread allocation path. A prior defect let the harness report
  missing performance data as a passing measurement — fixed upstream (see
  `TESTING.md`); treat any performance PASS as meaningful only if it names
  actual measured values.
- Two allocation paths are proven still present, without `MONUMENT_TESTING`,
  by `tests/RealtimeAllocationCharacterizationTest.cpp`: the first
  post-timeline-change `processBlock`, and every `TubeRayTracer` tube-count
  boundary crossing. Both are open findings, not yet fixed — see `TESTING.md`.
- No `SpinLock`/`std::mutex`/`CriticalSection` was found in the active DSP
  sources, but that does not prove the modulation-matrix snapshot publication
  (above) or transitive JUCE calls are safe under concurrent access.

---

## Documentation Index

**Getting Started**:
- [README.md](README.md) - Project overview
- [AGENTS.md](AGENTS.md) - Operating contract, invariants, definition of done

**Architecture**:
- [docs/architecture/ARCHITECTURE_QUICK_REFERENCE.md](docs/architecture/ARCHITECTURE_QUICK_REFERENCE.md) - Visual diagrams & fast navigation
- [docs/architecture/ARCHITECTURE_REVIEW.md](docs/architecture/ARCHITECTURE_REVIEW.md) - Detailed review
- [docs/architecture/PARAMETER_BEHAVIOR.md](docs/architecture/PARAMETER_BEHAVIOR.md) - Parameter contracts
- [docs/presets/PRESET_FORMAT.md](docs/presets/PRESET_FORMAT.md) - Preset JSON format contract

**Testing**:
- [TESTING.md](TESTING.md) - Canonical validation taxonomy, commands, known-red tests
- [docs/testing/README.md](docs/testing/README.md) - Testing docs index

**Process**:
- [CONTRIBUTING.md](CONTRIBUTING.md) - Contribution guidelines
- [CHANGELOG.md](CHANGELOG.md) - Version history
- [MANIFEST.md](MANIFEST.md) - Project manifest
