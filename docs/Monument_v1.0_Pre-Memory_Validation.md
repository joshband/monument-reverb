# Monument v1.0 Pre-Memory Validation

## Scope
- Hardening pass for Freeze, presets, parameter extremes, Pillars bounds, and test hooks.
- No creative DSP changes and no UI layout changes.

## Expected behavior (locked)
- Freeze engages/releases with 100 ms crossfades and preserves the captured Bloom envelope.
- Preset loads apply Init Patch first, clear Freeze, and reset DSP state via a short output fade.
- Pillars output is bounded (tap RMS target ~1.6, output ceiling +/-1.25) with DC removed by mode high-pass.
- Parameter ranges are clamped and warning-guarded in JUCE_DEBUG.

## Known limitations
- Presets cover core parameters only (Time, Mass, Density, Bloom, Gravity, Warp, Drift, Mix).
- Preset changes briefly fade output (~60 ms) to avoid clicks when clearing state.

## Assumptions for Memory Echoes
- Preset switching is stable during playback with no residual freeze/mutation state.
- Freeze toggles are click-free and level-stable.
- Pillars early-space output remains bounded and DC-free.
- Time/Mass/Density/Warp/Drift/Bloom ranges are final for v1.0 baseline.

## Release gate
- Tag the validation commit as `monument-v1.0-pre-memory` after pluginval + stress passes.
