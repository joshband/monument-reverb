# Monument Status Snapshot

**Date:** 2026-01-07
**Scope:** Macro-only UI direction + reactive visuals.

## DSP Feature Status

| Feature | Status | % | Notes |
| --- | --- | --- | --- |
| Core reverb chain (Foundation/Pillars/Chambers/Weathering/Buttress/Facade) | Complete | 90 | Stable in routing graph. |
| Routing graph + 3 processing modes + 8 presets | Complete | 90 | RT-safe switching in place. |
| Physical modeling modules (TubeRayTracer/ElasticHallway/AlienAmplification) | Complete | 85 | Integrated into presets + routing. |
| Ancient Monuments macro mapping (10 macros) | Complete | 80 | Active macro set in processor. |
| Modulation matrix (sources/destinations + snapshot) | In progress | 75 | UI + DSP wired, still expanding. |
| SequenceScheduler + Timeline automation | In progress | 60 | UI present, refinement ongoing. |
| Spatial positioning (PositionX/Y/Z) | In progress | 50 | Hooked into Chambers, needs deeper mapping. |
| Preset system (factory + user) | Complete | 80 | User preset save/load implemented. |
| Memory Echoes (module + params) | In progress | 35 | Implemented, integration gated by build flag. |
| Expressive macro set (alternate macros) | Deferred | 25 | Implemented but disabled. |
| Experimental modulation (gates/quantize/cross-mod/morph) | Prototype | 20 | Not yet integrated. |

## UI + Visual Feature Status

| Feature | Status | % | Notes |
| --- | --- | --- | --- |
| Macro-only layout + collapsible panels | Complete | 80 | Base parameters hidden in UI. |
| Celestial macro knobs (layered RGBA) | In progress | 70 | New component in place; visuals still tuning. |
| MacroVisualOverlay (OpenGL, reactive rings) | In progress | 70 | Audio + cursor reactive cues active. |
| Macro hint glyphs (JSON-driven) | In progress | 60 | Glyph overlays from `macro_hints.json`. |
| Preset visual profiles (JSON-driven) | In progress | 40 | `visual_profiles.json` partial coverage. |
| Particle system integration | Complete | 80 | Uses `Source/Particles` presets. |
| Mod Matrix UI panel | In progress | 70 | Functional, visual polish pending. |
| Header bar (presets, architecture, meters) | Complete | 80 | Macro mode selector removed. |
| Asset pipeline (RGBA extraction + file packs) | In progress | 60 | `assets/ui/celestial` in active use. |
| Accessibility + host naming alignment | Unstarted | 20 | Naming pass pending. |

## Direction Notes
- The UI exposes only the 10 macro controls; base parameters remain internal.
- Reactive visuals are driven by `assets/ui/macro_hints.json` + `assets/ui/visual_profiles.json`.
- OpenGL is used for the macro overlay; particle presets live in `Source/Particles/presets`.
