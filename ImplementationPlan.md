# Implementation Plan

Date: 2026-01-07
Scope: Address review findings, update documentation, and implement a macro-only UI direction based on Playground learnings.

## Workstreams

### 1) Stability & Real-Time Safety
- ✅ Guard `getPlayHead()` access in `plugin/PluginProcessor.cpp` and pass an empty optional if unavailable.
- ✅ Make routing preset changes RT-safe by precomputing routing graphs and swapping without allocation.
- ✅ Reduce audio-thread lock contention in `dsp/ModulationMatrix` by snapshotting connections off-thread.

### 2) Test & Build Hygiene
- Make `monument_smoke_test` CTest command config-aware.
- Confirm Release smoke tests run with `ctest --test-dir build -C Release`.

### 3) Performance Optimizations (Playground + Particles)
- Remove RTTI from particle force hot loops and use constant-time removal in the particle array.
- If the playground becomes a production UI path, move FFT analysis off the audio callback or decimate.

### 4) Documentation Consolidation
- Keep `README.md`, `ARCHITECTURE.md`, `ARCHITECTURE_QUICK_REFERENCE.md`, and `docs/INDEX.md` aligned with current architecture and playground status.
- Mark vision/phase docs as historical where appropriate.

---

## UI/UX Specification (Playground -> Plugin)

### Problem framing
- **User**: audio producers and sound designers using Monument to sculpt large, evolving ambience.
- **Primary task**: adjust core spatial parameters quickly while perceiving changes visually and aurally.
- **Constraints**: JUCE UI, real-time audio thread safety, cross-platform plugin hosts, and performance limits under heavy DSP load.

### UX intent
- **Primary action**: macro-only adjustment via tactile layered knobs (no base parameters shown).
- **Secondary actions**: preset selection, architecture routing, modulation view, and audio-reactive visuals.
- **Loading/empty/error states**: show fallback test pattern if assets fail, and a clear status banner when particles/audio are disabled.

### Interaction model
- **States**: idle, editing (dragging), reactive (audio peaks), debug (visualization overlays).
- **Transitions**: 
  - Hover -> highlight ring; drag -> knob rotates with velocity smoothing.
  - Audio peak -> particle burst + glow intensification.
  - Cursor move -> emitter follows with soft easing.
- **Feedback**: numeric readout in status bar, glow intensity, and subtle motion cues.

### Design system mapping
- **Tokens (W3C design tokens)**:
```json
{
  "color": {
    "surface": { "value": "#0B0D10" },
    "surfaceAlt": { "value": "#14181F" },
    "textPrimary": { "value": "#F2F2F2" },
    "textSecondary": { "value": "#B7B9C2" },
    "accentWarm": { "value": "#E07A3F" },
    "accentCool": { "value": "#57B7C7" }
  },
  "space": {
    "xs": { "value": "4px" },
    "sm": { "value": "8px" },
    "md": { "value": "16px" },
    "lg": { "value": "24px" }
  },
  "radius": {
    "sm": { "value": "6px" },
    "md": { "value": "10px" },
    "lg": { "value": "16px" }
  },
  "motion": {
    "fast": { "value": "120ms" },
    "base": { "value": "180ms" },
    "slow": { "value": "300ms" }
  }
}
```
- **Components**:
  - `LayeredKnob` (variants: geode/metal/industrial; states: default/hover/dragging).
  - `ParticleField` (states: off/idle/reactive; variants: embers/smoke/sparks).
  - `StatusBar` (states: normal/warning; shows audio state and metrics).
  - `MacroCluster` (10 macros; macro-only layout with labels).
  - `MacroVisualOverlay` (OpenGL rings + glyph hints from JSON profiles).

### Frontend implementation notes
- **Component structure**: 
  - `MainComponent` owns layout + state.
  - `LayeredKnob` handles rotation and compositing.
  - `ParticleSystem` updates at fixed timestep.
- **Layout approach**: grid-based placement using `juce::Grid` (or manual layout with a grid mental model).
- **State ownership**: APVTS for parameters; UI-only state for pack selection, debug overlays, and particle mode.
- **Accessibility hooks**: keyboard focus traversal for knobs; visible focus ring; status text for mode changes.

### Failure modes
- Missing asset packs -> fallback test pattern, disable pack switching.
- High CPU load -> particle update throttling and glow clamp.
- UI/Audio mismatch -> stale meter values; provide status banner and update indicators.

### Iteration & validation ideas
- Verify knob drag feels consistent across packs (A/B pack comparisons).
- Measure UI frame rate with particles on/off (60fps target).
- Validate audio-reactive bursts with real material (RMS/peak correlation).

---

## Codex-Ready Prompts

1) **Fix playhead null safety**
- Task: Guard `getPlayHead()` in `plugin/PluginProcessor.cpp` before calling `getPosition()`; pass an empty optional when missing.
- Acceptance: No crash in hosts that return null; unit tests still pass.

2) **Make routing preset changes RT-safe**
- Task: Precompute routing presets in `DspRoutingGraph` and swap using a preallocated buffer (no allocation in `processBlock()`).
- Acceptance: No allocations in audio thread when changing `routingPreset` parameter.

3) **Make smoke test config-aware**
- Task: Update `add_test` for `monument_smoke_test` to use the active build configuration instead of Debug hardcoding.
- Acceptance: `ctest --test-dir build -C Release` passes.

4) **Optimize particle hot loop**
- Task: Remove `dynamic_cast` from `ParticleSystem::integrate` by caching a pointer to `CurlNoiseForce` or storing modulation flags.
- Acceptance: No RTTI calls in the particle integration loop; behavior unchanged.

5) **Fast particle removal**
- Task: Replace `particles.remove(i)` with `removeQuick(i)` or swap-pop to avoid O(n) shifts.
- Acceptance: Particle counts remain stable; no ordering-dependent logic breaks.

6) **UI update: layered PBR + particles**
- Task: Implement a new UI layout in `playground/MainComponent.*` that groups macros, core controls, and status into a grid, adds particle overlay toggles, and exposes audio-reactive settings.
- Acceptance: Works at 800x600 and scales to 1280x720; particles toggle without crash; audio-reactive glow visible.

7) **Asset pipeline decision doc**
- Task: Document and implement the chosen asset strategy (embedded BinaryData vs file-based packs) and update `docs/INDEX.md` + README.
- Acceptance: Assets load reliably in development builds; docs explain how to package for distribution.
