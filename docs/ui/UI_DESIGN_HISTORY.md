# UI Design History

This is a short pointer document, not a full history. Its job is to keep
whoever reads the archived UI-design documents in
[`docs/archive/ui/`](../archive/ui/) from mistaking them for the current
design.

## What the archived documents describe

Between roughly 2026-01-03 and 2026-01-09, this repository's UI design
documents described (and briefly implemented) a **macro-only interface**
direction:

- A small control surface — 3 to 10 primary "macro" knobs — with the full
  set of base DSP parameters hidden by default.
- A `CelestialKnob` component rendering layered RGBA assets from
  `assets/ui/celestial/`, with a JSON-driven reactive overlay
  (`assets/ui/macro_hints.json`, `assets/ui/visual_profiles.json`) and
  particle/wisp effects.
- A photorealistic Blender-rendered knob pipeline (multiple PBR material
  variants: granite, marble, basalt, brushed metal, oxidized copper) as an
  optional/future asset source for the same macro-only surface.
- `docs/archive/ui/UI_MASTER_PLAN.md` (dated 2026-01-09) is the most
  complete statement of this direction — it explicitly declares "Current
  Direction: Macro-Only UI" and consolidates five earlier documents.
- `docs/archive/ui/UI_RESET_2026_01_08.md` and
  `docs/archive/ui/UI_FULL_RESET_2026_01_08.md` both record a 2026-01-08
  event where this system (and ~150MB of associated assets/code) was
  archived down to a plain 10-slider JUCE UI. The two documents cover the
  same event; `UI_FULL_RESET_2026_01_08.md` is the more complete/later
  account (it references `UI_RESET_2026_01_08.md` as "Day 1"), so treat it
  as the more authoritative of the pair if only one is consulted.

## What actually shipped

None of the above is the shipping UI today. `createEditor()`
(`plugin/PluginProcessor.cpp`) defaults to `MonumentAudioProcessorEditorV2`
(`plugin/PluginEditorV2.cpp/h`), which:

- Exposes all **44 individual DSP parameters** as sliders/knobs, plus
  routing/timeline/macro-mode combo controls — not a reduced macro set.
- Renders knobs with `monument::PhotorealisticKnob`
  (`ui/PhotorealisticKnob.h`), not `CelestialKnob`.
- Has dedicated Base Parameters, Modulation, and Timeline sections with
  per-section collapse/expand state.
- Loads knob-layer assets from an environment-selected directory at
  runtime rather than the `assets/ui/celestial/` pipeline described above.

The current, accurate descriptions of this shipping editor are:

- [`ARCHITECTURE.md`](../../ARCHITECTURE.md), "UI ownership" section.
- [`docs/architecture/EDITOR_PARITY_FINDINGS.md`](../architecture/EDITOR_PARITY_FINDINGS.md),
  which characterizes `PluginEditorV2` against the older `PluginEditor`
  (reachable only via `-DMONUMENT_LEGACY_UI=ON`) in detail.

## Why the pivot happened

Unknown. No decision record explaining the reversal from macro-only back to
full-parameter exposure was found in this repository. The fact of the
change is documented above; the rationale is not — treat any explanation
you encounter elsewhere as unverified unless it cites a specific commit or
document. If you want the original design thinking behind the macro-only
direction, read the archived documents directly at their new location in
[`docs/archive/ui/`](../archive/ui/).

## Current knob-asset workflow

`docs/archive/ui/QUICK_START_BLENDER_KNOBS.md` and
`docs/archive/ui/LAYERED_KNOB_WORKFLOW.md` (both archived alongside this
cluster) describe a Blender/Midjourney-based knob-asset pipeline that is
**abandoned** and does not reflect how knob assets are produced today. No
replacement workflow document was found in this repository as of this
writing. If a current pipeline exists, it should be verified and documented
separately — this document does not attempt to guess at one.

For current UI architecture generally, see [`ARCHITECTURE.md`](../../ARCHITECTURE.md).
