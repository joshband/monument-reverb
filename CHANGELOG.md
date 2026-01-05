# Changelog

All notable changes to this project will be documented in this file.
The format is based on Keep a Changelog, and this project adheres to Semantic Versioning.

## [Unreleased]

### Notes

- Memory Echoes development has moved to a standalone repository. Monument's
  release line is intentionally memory-free until the planned v1.6 reintegration.

### Task 3 Complete: 9 New "Living" Presets (2026-01-04)

**Status**: ✅ Complete

**Implementation Summary**:

- Added 9 new "Living" presets showcasing creative modulation routings
- Total factory presets: 37 (28 existing + 9 new)
- Updated preset count in PresetManager.h from 28 to 37
- Enhanced destination type serialization to support all physical modeling parameters

**New Preset Categories**:

**Dynamic Response (AudioFollower-driven):**

1. **Pulsing Cathedral** - Reverb size swells with input level
2. **Dynamic Shimmer** - High-frequency air increases with intensity

**Chaotic Motion (ChaosAttractor-driven):**

1. **Quantum Shimmer** - Physics violations ebb and flow unpredictably
2. **Morphing Cathedral** - Tube complexity shifts chaotically while space drifts
3. **Fractal Space** - Topology morphs through chaotic attractor states

**Organic Evolution (BrownianMotion-driven):**

1. **Elastic Drift** - Walls breathe with random walk motion
2. **Spectral Wander** - Tube brightness drifts organically

**Experimental Combinations:**

1. **Impossible Hall** - Energy gain triggered by input (explores ParadoxGain)
2. **Breathing Chaos** - Elastic walls deform intermittently with chaos

**Files Modified**:

- [plugin/PresetManager.h:77](plugin/PresetManager.h) - Updated kNumFactoryPresets (28 → 37)
- [plugin/PresetManager.cpp:215-305](plugin/PresetManager.cpp) - Added 9 new presets
- [plugin/PresetManager.cpp:546-615](plugin/PresetManager.cpp) - Enhanced destination serialization

**Impact**:

- Demonstrates full modulation system capabilities across all 4 sources
- Provides ready-to-use starting points for sonic exploration
- Showcases physical modeling parameter modulation
- Enables instant creative discovery through preset browsing

### Task 2 Complete: Randomize Modulation Button (2026-01-04)

**Status**: ✅ Verified complete (already implemented in codebase)

**Implementation Verification**:

- Three randomization methods in ModulationMatrix (randomizeSparse, randomizeAll, randomizeDense)
- Thread-safe using SpinLock for connection vector access
- Musical safety constraints: depth ±60% max, smoothing ≥100ms
- UI popup menu with 4 options (Sparse/Normal/Dense/Clear All)
- Button located in top-right of modulation matrix panel
- JUCE DSP Expert Review: Grade A- (92/100) - Real-time safe, thread-safe, efficient

**Files Verified**:

- [dsp/ModulationMatrix.h:161-187](dsp/ModulationMatrix.h) - Method declarations
- [dsp/ModulationMatrix.cpp:668-761](dsp/ModulationMatrix.cpp) - Implementation
- [ui/ModMatrixPanel.h:70](ui/ModMatrixPanel.h) - Button declaration
- [ui/ModMatrixPanel.cpp:33-41,636-681](ui/ModMatrixPanel.cpp) - UI and menu

### Changed - Preset System v4 Migration (2026-01-04)

- **Preset Format v3→v4**: Migrated preset system to support 10 Ancient Monuments macros
  - Added 4 new macro fields: `patina`, `abyss`, `corona`, `breath`
  - Backward compatibility: v3 presets automatically migrated with defaults (patina=0.5, abyss=0.5, corona=0.5, breath=0.0)
  - All 28 factory presets updated with new macro support via default parameters
  - User presets from v3 will seamlessly load with sensible defaults for new macros
  - See [plugin/PresetManager.h:37-41](plugin/PresetManager.h) and [plugin/PresetManager.cpp:7](plugin/PresetManager.cpp)

### Added - Modulation Matrix UI Enhancements (2026-01-04)

- **Enhanced Randomization**: 4 randomization modes for creative modulation exploration
  - **Sparse Mode** (2-3 connections, ±20-40% depth): Subtle, focused modulation for gentle movement
  - **Normal Mode** (4-8 connections, ±20-60% depth): Balanced modulation density (default behavior)
  - **Dense Mode** (8-12 connections, ±40-80% depth): Extreme, complex modulation for chaotic spaces
  - **Clear All**: Remove all connections, reset to static state
  - Accessed via dropdown menu on "Randomize" button in ModMatrixPanel
  - See [ui/ModMatrixPanel.h](ui/ModMatrixPanel.h) and [ui/ModMatrixPanel.cpp](ui/ModMatrixPanel.cpp)
- **Connection Presets**: 5 preset slots for saving/loading complete modulation routing configurations
  - **Save Button**: Store current routing to slots 1-5 (shows connection count in labels)
  - **Load Button**: Recall routing from saved presets (prevents loading from empty slots)
  - Preserves all connection properties: source, destination, depth, smoothing, probability
  - Use cases: snapshot random configs, switch between performance states, share routing templates
  - See [ui/ModMatrixPanel.cpp:460-520](ui/ModMatrixPanel.cpp)
- **Probability Gates**: Per-connection probability parameter (0-100%) for intermittent modulation
  - **0%**: Connection never applies modulation (effectively disabled)
  - **50%**: Modulation active ~50% of the time (unpredictable gating)
  - **100%**: Always active (traditional continuous modulation)
  - Block-rate probability evaluation (not per-sample) for efficiency
  - Creates evolving, non-repeating soundscapes and rhythmic texture variations
  - Orange-colored slider in ModMatrixPanel for visual distinction
  - See [dsp/ModulationMatrix.h:23](dsp/ModulationMatrix.h) and [dsp/ModulationMatrix.cpp:82-95](dsp/ModulationMatrix.cpp)
- **Character Encoding Fixes**: Removed emoji and special characters for ASCII compatibility
  - "Randomize" button text (removed emoji that displayed as superscript 2)
  - Bullets changed from "•" to "-" (was showing as "â€¢")
  - Arrows changed from "→" to "->" (ASCII-safe)

### Added - Phase 5: Physical Modeling Integration Complete (2026-01-04)

- **TubeRayTracer Module**: Metal tube network with ray-traced acoustic propagation
  - Simulates interconnected metal pipes (1-8 tubes) with realistic resonance
  - Ray-tracing algorithm for physically-modeled sound propagation through tube network
  - 4 control parameters: Tube Count, Radius Variation, Metallic Resonance, Coupling Strength
  - Creates metallic, resonant spaces reminiscent of industrial ductwork and organ pipes
  - See `dsp/TubeRayTracer.h` and `dsp/TubeRayTracer.cpp`
- **ElasticHallway Module**: Deformable walls responding to acoustic pressure
  - Walls physically deform under sound pressure and recover over time
  - Absorption properties drift slowly, creating evolving acoustic character
  - 4 control parameters: Wall Elasticity, Recovery Time, Absorption Drift, Elastic Nonlinearity
  - Creates breathing, organic spaces that respond to input dynamics
  - See `dsp/ElasticHallway.h` and `dsp/ElasticHallway.cpp`
- **AlienAmplification Module**: Non-Euclidean physics with impossible amplification
  - Violates physical acoustic laws for surreal, impossible spaces
  - Sound can gain energy, fold through non-Euclidean topology, and evolve harmonically
  - 4 control parameters: Impossibility Degree, Pitch Evolution, Paradox Frequency, Paradox Gain
  - Creates physics-breaking effects impossible in the real world
  - See `dsp/AlienAmplification.h` and `dsp/AlienAmplification.cpp`
- **12 New APVTS Parameters**: All normalized [0,1] following Monument conventions
  - Tube: tubeCount, tubeRadiusVariation, metallicResonance, tubeCouplingStrength
  - Elastic: wallElasticity, recoveryTime, absorptionDrift, elasticNonlinearity
  - Alien: impossibilityDegree, pitchEvolution, paradoxFrequency, paradoxGain
  - Parameters exposed in AudioProcessorValueTreeState with default values
- **Macro System Integration**: Physical modeling parameters driven by 6 macro controls
  - Material → Metallic resonance (0.3→0.9), Tube uniformity via radius variation (0.8→0.2)
  - Topology → Tube network complexity via count (0.15→0.85), Coupling strength (0.2→0.8)
  - Viscosity → Recovery time for elastic walls (0.2→0.8)
  - Evolution → Absorption drift (0.0→0.6), Pitch evolution (0.0→0.5)
  - Chaos → Impossibility physics (0.0→0.7), Elastic nonlinearity (0.1→0.9)
  - Elasticity → Wall deformation amount (0.1→0.9, primary macro use)
  - See `dsp/MacroMapper.cpp:245-324` for full mapping implementation
- **ModulationMatrix Integration**: 8 new physical modeling destinations
  - Added to ModulationMatrix: TubeCount, MetallicResonance, WallElasticity, ImpossibilityDegree
  - Plus 4 additional destinations for complete physical modeling coverage
  - All physical parameters can be modulated by Chaos, Audio, Brownian, Envelope sources
  - See `dsp/ModulationMatrix.h:30-37` for destination enum
- **5 New Factory Presets**: Physical modeling showcase (Total: 28 presets)
  - **Metallic Corridor** (Preset #24): Resonant 5-tube network, high coupling, medium metallicity
  - **Elastic Cathedral** (Preset #25): Highly elastic walls (0.85), slow recovery (0.7), breathing space
  - **Impossible Chamber** (Preset #26): Maximum impossibility (0.9), strong paradox effects, alien physics
  - **Breathing Tubes** (Preset #27): 6 tubes + elastic walls + modulation (AudioFollower → WallElasticity)
  - **Quantum Hall** (Preset #28): Tube network + chaos modulation (ChaosAttractor → ImpossibilityDegree)
  - Presets demonstrate solo modules and hybrid combinations with core reverb
  - See `plugin/PresetManager.cpp:356-461` for preset definitions
- **PluginProcessor Integration**: Full DSP pipeline integration with existing modules
  - Physical modules process in series after Chambers, before Buttress
  - Signal flow: Foundation → Pillars → Chambers → **Physical Modules** → Buttress → Facade
  - processBlock updated to poll physical parameters and call module process() methods
  - All 12 parameters integrated into parameter cache and smoothing system
  - Thread-safe with existing SpinLock for modulation/macro coordination
  - See `plugin/PluginProcessor.cpp:389-398` for processing order
- **Build System Updates**: CMakeLists.txt updated with new source files
  - Added 6 new files: TubeRayTracer.h/.cpp, ElasticHallway.h/.cpp, AlienAmplification.h/.cpp
  - Incremental builds still fast (~6 seconds for changed files)
  - No external dependencies added (pure C++ implementation)

### Added - Phase 4: UI Enhancement Complete (2026-01-04)

- **Unified Codex Knob Implementation**: All 18 knobs now use consistent brushed aluminum texture
  - Replaced all MonumentKnob instances with HeroKnob (LayeredKnob-based)
  - Single albedo RGBA texture from codex pre-generated assets
  - Smooth rotation with vertical drag interaction
  - Clean white background with dark text for high contrast
  - Hover effects temporarily disabled for cleaner appearance
  - See `ui/HeroKnob.h` and `plugin/PluginEditor.cpp:6-24`

### Added - Phase 4: UI Enhancement (2026-01-03)

- **ModMatrixPanel Component**: Professional modulation matrix UI (2026-01-03 Evening)
  - Visual 4×15 grid (60 connection points) for routing modulation sources to destinations
  - Color-coded sources: Chaos (orange), Audio (green), Brownian (purple), Envelope (blue)
  - Custom ConnectionButton component with hover/active/selected states
  - Hover effects with 20% alpha overlay for discoverability
  - Active connections render with 60% alpha fill (80% when selected)
  - Selection indicator (white dot) for currently editing connection
  - Real-time connection list display with monospaced font
  - Depth slider (-1 to +1) and smoothing slider (20-1000ms) for live editing
  - Toggle button in main UI expands window from 580px → 1080px
  - Thread-safe via ModulationMatrix SpinLock
  - Abbreviated destination labels (Tim, Mas, Den, Blm, Air, Wid, Mix, Wrp, Drf, Grv, Pil, Tub, Met, Els, Imp)
  - Interaction model: click to create → click to select → click selected to remove
  - See `ui/ModMatrixPanel.h` and `ui/ModMatrixPanel.cpp`
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
