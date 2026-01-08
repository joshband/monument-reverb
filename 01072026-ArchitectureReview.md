# 01072026 Architecture Review

Date: 2026-01-07
Scope: Core plugin architecture, UI systems, playground app, particle system, asset pipelines, build/test topology, and documentation alignment.

## Summary
The architecture is functionally rich and modular, but the project has grown multiple parallel UI and asset pipelines (plugin UI, playground app, and legacy demo assets). This creates ambiguity about the canonical UI stack and how assets are packaged and shipped. Documentation also diverges across phases, and the build/testing workflows are described inconsistently.

## Current System Map (High Level)

### Core Plugin (JUCE)
- **Entry points**: `plugin/PluginProcessor.*`, `plugin/PluginEditor.*`
- **DSP modules**: `dsp/*` (Foundation, Pillars, Chambers, Weathering, physical modules, modulation, routing graph)
- **Control plane**: APVTS parameters -> MacroMapper -> ModulationMatrix -> ParameterSmoother
- **Routing**: `dsp/DspRoutingGraph.*` (preset-based routing modes)

### UI Systems
- **Primary plugin UI**: `ui/*` (LayeredKnob, ModMatrixPanel, Monument controls)
- **Playground app**: `playground/*` (standalone app for photorealistic knobs + audio-reactive particles)
- **Legacy/demo assets**: `MonumentUI_Demo/` and `UI Mockup/` directories

### New Subsystems
- **Particle engine**: `Source/Particles/*` (behavior DSL + simulation + presets)
- **Asset packs**: `assets/knob_geode/`, `assets/knob_metal/`, `assets/knob_industrial/`

### Tests & Tooling
- **CTest apps**: `monument_smoke_test`, `monument_memory_echoes_test`
- **Analyzer tooling**: `tools/plugin-analyzer/` and related scripts

## Findings (ordered by severity)

### High
1) **Multiple UI tracks without a single canonical path**
- The plugin UI (`ui/`) is the shipping UI, but the playground (`playground/`) now contains the most active UI experimentation (particles, audio-reactive components, asset pack switching). Meanwhile, `MonumentUI_Demo/` and `UI Mockup/` still exist with overlapping artifacts.
- Risk: unclear direction for UI implementation, duplicated effort, and conflicting documentation.

2) **Asset pipeline split between binary-embedded and file-based assets**
- The plugin uses `MonumentAssets` binary data (embedded PNGs), while the playground uses file-based assets from `assets/knob_*` and JSON manifests at runtime.
- Risk: inconsistent packaging and potential asset path failures when distributing the playground app or integrating it into the plugin.

### Medium
1) **Particle system is colocated outside dsp/ and ui/**
- `Source/Particles/` sits outside of the primary `dsp/` and `ui/` trees. It is currently only used by `playground/`.
- Risk: unclear ownership (DSP vs UI), makes it harder to determine integration readiness for the plugin.

2) **Control-plane additions (SequenceScheduler) are not documented**
- `dsp/SequenceScheduler.*` is now part of the control plane but is not reflected in `ARCHITECTURE.md` or the quick reference.

3) **Memory Echoes narrative mismatch**
- Some docs state the plugin is memory-free, while the code still includes `MemoryEchoes` and related preset parameters. This creates architectural ambiguity.

### Low
1) **Routing graph TODOs suggest future extensibility without validation**
- Cycle detection and topological sorting are noted as TODOs in `dsp/DspRoutingGraph.cpp`, which is fine for preset-only routing but becomes risky if user-authored routing is introduced.

## Documentation Alignment Gaps
- `README.md`, `ARCHITECTURE.md`, and `ARCHITECTURE_QUICK_REFERENCE.md` reference outdated phase statuses and incomplete module lists.
- `docs/INDEX.md` does not include the new particle/Playground subsystems or the latest DSP signal flow document.
- Multiple older UI design docs (MVP handoff, UI demo assets) are not marked as historical.

## Recommendations
1) **Declare a canonical UI path**
   - Either promote the playground as the canonical UI experimentation environment or explicitly mark it as a sandbox. Update docs accordingly.

2) **Unify asset packaging strategy**
   - Decide whether the playground should use embedded assets (BinaryData) or if file-based packs are acceptable for dev only. Document the distinction.

3) **Move or label particle system ownership**
   - If the particle system is a UI layer, consider placing it under `ui/` or documenting it clearly as a UI-only subsystem.

4) **Refresh architecture docs**
   - Update `ARCHITECTURE.md`, `ARCHITECTURE_QUICK_REFERENCE.md`, and `docs/INDEX.md` to reflect current control plane and UI/particle systems.

5) **Establish a documentation lifecycle**
   - Mark phase summaries and early handoff documents as historical or archived.

