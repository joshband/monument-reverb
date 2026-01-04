# Changelog

All notable changes to this project will be documented in this file.
The format is based on Keep a Changelog, and this project adheres to Semantic Versioning.

## [Unreleased]

### Notes

- Memory Echoes development has moved to a standalone repository. Monument's
  release line is intentionally memory-free until the planned v1.6 reintegration.

### Added - Phase 4: UI Enhancement (2026-01-03)

- **LayeredKnob Component**: Photorealistic multi-layer knob rendering system
  - Supports 4+ image layers with alpha compositing
  - Independent rotation control per layer (rotating vs static)
  - Proper transform chain (scale → translate → rotate) for uniform alignment
  - Drag-to-control interaction with DAW automation support
  - Real-time parameter synchronization via APVTS listener
- **Blender Knob Generation Pipeline**: Procedural knob layer generation
  - Headless Blender rendering (~2-3 minutes for 4 layers)
  - Concrete, metal, and engraved material presets
  - Fully customizable via Python scripts
  - 512×512 output with alpha transparency
  - See `scripts/generate_knob_blender.py` and `scripts/run_blender_knobs.sh`
- **Knob Preview Tool**: Pre-build composite verification
  - `scripts/preview_knob_composite.py` - Test composites before building
  - Supports arbitrary rotation angles for alignment verification
  - Saves preview images for inspection
  - Catches misalignment issues early (reduces iteration time)
- **Standardized Build Workflow**: Incremental builds with CMake
  - Single `build/` directory (no more build-fetch, build-harness, etc.)
  - Incremental builds in ~6 seconds (only changed files recompiled)
  - Auto-install to `~/Library/Audio/Plug-Ins/{Components,VST3}/`
  - Documented in `STANDARD_BUILD_WORKFLOW.md`
- **Documentation Reorganization**: Structured documentation hierarchy
  - Root docs limited to 8 essential files (AGENTS, ARCHITECTURE, CHANGELOG, CONTRIBUTING, MANIFEST, README, STANDARD_BUILD_WORKFLOW, ARCHITECTURE_QUICK_REFERENCE)
  - `docs/` subdirectories: ui/, development/, architecture/, testing/
  - `docs/INDEX.md` - Central navigation hub with learning paths
  - `ARCHITECTURE.md` - Consolidated architecture overview
  - Cross-referenced documentation with proper categorization

### Fixed - Phase 4

- **LayeredKnob Rendering**: Fixed transform application for rotating layers
  - Previous: Applied rotation to graphics context, then drew at renderBounds (coordinates got rotated, causing misalignment)
  - Fixed: Use `drawImageTransformed()` with proper transform chain (scale → translate to center → rotate around center)
  - All layers now render aligned and scaled uniformly (both rotating and static)
  - See `ui/LayeredKnob.cpp:125-132`

### Added - Phase 3: Modulation Sources Complete (2026-01-03)

- **ChaosAttractor**: Lorenz strange attractor with 3-axis output (X/Y/Z), deterministic but unpredictable motion
- **AudioFollower**: RMS envelope tracking with 10ms attack, 150ms release
- **BrownianMotion**: Smooth random walk with velocity smoothing and boundary reflection
- **EnvelopeTracker**: Multi-stage envelope detection (attack/sustain/release) with peak+RMS analysis
- **ModulationMatrix Integration**: Full per-block processing with connection routing and smoothing
- All sources output normalized values (bipolar [-1,1] or unipolar [0,1])
- Block-rate processing for efficiency (~0.3-0.5% CPU overhead)
- **"Living" Presets** (5 new factory presets with modulation):
  - Breathing Stone: AudioFollower → Bloom (dynamic expansion/contraction)
  - Drifting Cathedral: BrownianMotion → Drift + Gravity (slow spatial wandering)
  - Chaos Hall: ChaosAttractor (X,Y) → Warp + Density (organic mutations)
  - Living Pillars: EnvelopeTracker → PillarShape + AudioFollower → Width (musical morphing)
  - Event Horizon Evolved: ChaosAttractor (Z) → Mass + BrownianMotion → Drift (gravitational wobble)
- **Preset Architecture**: Modulation connections stored in PresetValues, applied on preset load
- **Under-the-Hood Magic**: No UI controls for modulation (discovery-focused experience)

### Fixed - Phase 3

- **UI Controls Update on Preset Load**: Macro parameters (Material, Topology, Viscosity, Evolution, Chaos, Elasticity) now properly update in the UI when loading presets. Previously, only base parameters would sync with the UI, leaving macro knobs at stale values.
- **Preset Menu Organization**: Added "Evolving Spaces" section (presets 18-22) to organize the new "Living" modulation presets separately from traditional presets.

### Added - Phase 2: Macro System Integration

- **PluginProcessor Integration**: Macro system fully integrated into audio processing pipeline.
  - Macro influence blending: parameters smoothly transition from base values to macro-driven targets.
  - Real-time parameter polling and MacroMapper computation per audio block.
  - ModulationMatrix processing integrated (stub sources active, ready for Phase 3 DSP).
  - All module setters updated to use macro-influenced effective parameters.

### Added - Phase 1: Macro System Foundation

- **MacroMapper**: High-level macro control system mapping 6 conceptual parameters to coordinated parameter sets.
  - Material (0=soft → 1=hard): Influences time, mass, and density for surface character.
  - Topology (0=regular → 1=non-Euclidean): Influences warp and drift for spatial geometry.
  - Viscosity (0=airy → 1=thick): Influences time, air, and mass for medium resistance.
  - Evolution (0=static → 1=evolving): Influences bloom and drift for temporal evolution.
  - Chaos Intensity (0=stable → 1=chaotic): Influences warp and drift for unpredictable motion.
  - Elasticity Decay (0=instant → 1=slow): Reserved for future physical modeling modules.
- **ModulationMatrix**: Modulation routing infrastructure with support for multiple sources and destinations.
  - 4 modulation sources (stub implementations for Phase 1): Chaos Attractor, Audio Follower, Brownian Motion, Envelope Tracker.
  - 16 parameter destinations with per-connection depth and smoothing control.
  - Block-rate processing with juce::SmoothedValue for zipper-free modulation.
  - Fully real-time safe with no allocations in process().
- **New APVTS Parameters**: 6 macro control parameters exposed in AudioProcessorValueTreeState.
  - All parameters use normalized [0,1] ranges following JUCE conventions.
  - Default values chosen for neutral/balanced starting points.

### Added
- Factory preset descriptions, Init Patch, and JSON user preset save/load support.
- Pillars early-reflection upgrades: geometry bending, fractal tap clusters, pseudo-IR loading, and Glass/Stone/Fog modes.
- Testing hooks for peak/CPU logging, plus pluginval runner and testing docs.
- Advanced feature scaffolding notes for algorithm switching, colour modes, tape loop, and sound-on-sound.
- Expanded factory preset pool to 18 with narrative descriptions.

### Changed
- Time feedback mapping widened to 0.35-0.995; mass and density ranges extended for darker and sparser tails.
- Drift depth limit set to +/-1 sample for subtle motion without pitch wobble.
- Freeze crossfade timing extended to 100 ms for click-free engage/release.
- Preset loading now applies Init Patch first and schedules a short DSP reset fade.
- Pillars tap energy normalization and output headroom clamp for bounded early-space output.
- Chambers parameter setters now sanitize out-of-range/NaN values (JUCE_DEBUG warnings).

### Fixed
- Preset switches clear freeze state and reset Pillars mutation timers to avoid residual behavior.

### Legacy (Memory Echoes, extracted)
- Memory Echoes Phase 1: optional wet-memory recall with recent/distant buffers, plus the "Ruined Monument (Remembers)" preset.
- Memory Echoes Phase 2 complete: age-based filtering/saturation/decay, subtle drift, and recall density shaping with new Memory Decay/Memory Drift parameters.
- Standalone Memory Echoes harness for isolated renders and tuning.
- Memory Echoes is disabled in the plugin by default; enable with `-DMONUMENT_ENABLE_MEMORY=ON` for dev builds.
- Memory Echoes recall scheduling now triggers fragments correctly; added a MemoryEchoes unit test and pluginval script test hook.

## [0.1.7] - 2026-01-01

### Added
- Preset manager with indexed and name-based loading, plus a curated preset list for core parameters.
- Editor preset picker for quick auditioning of curated spaces.
- Visual manual (HTML) and infographic assets for signal flow, control compass, and freeze transitions.

### Changed
- README expanded into a concise user guide with diagrams, parameter tables, and build/install steps.
- Time mapping extended for minute-scale T60 targets.

### Fixed
- Freeze transitions now crossfade between live and frozen wet paths to eliminate clicks.
- Freeze release now ramps early mix and envelope return instead of hard resets.

## [0.1.6] - 2026-01-01

### Added
- Drift micro-modulation with per-line LFOs for subtle, living tails.

## [0.1.5] - 2026-01-01

### Added
- Warp control with orthogonal FDN matrix morphing for slow spatial motion.

## [0.1.4] - 2026-01-01

### Added
- True Freeze semantics in Chambers with state hold and unity feedback.

### Fixed
- Hardened Freeze state hold for stability under sustained feedback.

## [0.1.3] - 2026-01-01

### Changed
- Mono-safe output mixing with constant-power balance across FDN taps.
- Gravity low-end control tuning for tighter decay behavior.

## [0.1.2] - 2026-01-01

### Added
- Per-parameter smoothing in Chambers to prevent zipper noise.

## [0.1.1] - 2026-01-01

### Added
- Monument DSP architecture and tooling scaffold.

## [0.1.0] - 2026-01-01

### Added
- Initial JUCE plugin skeleton with macOS build scripts and CMake workflow.
- CI workflows with cache and build artifacts, plus CTest smoke coverage.
- Baseline documentation: manifesto, DSP architecture, parameter behavior, and C++ standard alignment.

[Unreleased]: https://github.com/joshband/monument-reverb/compare/v0.1.7...HEAD
[0.1.7]: https://github.com/joshband/monument-reverb/releases/tag/v0.1.7
[0.1.6]: https://github.com/joshband/monument-reverb/releases/tag/v0.1.6
[0.1.5]: https://github.com/joshband/monument-reverb/releases/tag/v0.1.5
[0.1.4]: https://github.com/joshband/monument-reverb/releases/tag/v0.1.4
[0.1.3]: https://github.com/joshband/monument-reverb/releases/tag/v0.1.3
[0.1.2]: https://github.com/joshband/monument-reverb/releases/tag/v0.1.2
[0.1.1]: https://github.com/joshband/monument-reverb/releases/tag/v0.1.1
[0.1.0]: https://github.com/joshband/monument-reverb/releases/tag/v0.1.0
