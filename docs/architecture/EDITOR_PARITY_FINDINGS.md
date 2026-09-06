# Editor Parity Findings: PluginEditor (legacy) vs. PluginEditorV2 (default)

Characterization only — this document does not recommend keeping, retiring,
or merging either editor. It exists to give a human the evidence needed to
make that call, per the repository reset report's "INVESTIGATE parity/usage
before removal" guidance for report step 10.

## Selection mechanism

`plugin/PluginProcessor.cpp:1291-1302` (`createEditor()`):

```cpp
#if defined(MONUMENT_LEGACY_UI)
    return new MonumentAudioProcessorEditor(*this);
#else
    return new MonumentAudioProcessorEditorV2(*this);
#endif
```

V2 is the unconditional default; `MonumentAudioProcessorEditor` (legacy) is
reachable only by configuring with `-DMONUMENT_LEGACY_UI=ON`
(`CMakeLists.txt:218-227`, `option(MONUMENT_LEGACY_UI "Use legacy UI layout" OFF)`).
`ARCHITECTURE.md:110-120` already documents this split and explicitly warns
against adding a third editor generation.

## File sizes

| File | Lines |
|---|---|
| `plugin/PluginEditor.cpp` | 866 |
| `plugin/PluginEditor.h` | 189 |
| `plugin/PluginEditorV2.cpp` | 1292 |
| `plugin/PluginEditorV2.h` | 222 |

## Parameter/control surface: identical

Both headers declare the same named control set (`plugin/PluginEditor.h:89-146`,
`plugin/PluginEditorV2.h:102-159`): 4 combo controls (`macroModeControl`,
`routingPresetControl`, `pillarModeControl`, `timelinePresetControl`), 3
toggle controls (`freezeControl`, `timelineEnabledControl`,
`safetyClipControl`), and 44 slider/knob controls covering the same
parameters — `mix`, `time`, `mass`, `density`, `bloom`, `air`, `width`,
`warp`, `drift`, `gravity`, `pillarShape`, `material`, `topology`,
`viscosity`, `evolution`, `chaosIntensity`, `elasticityDecay`, `patina`,
`abyss`, `corona`, `breath`, `character`, `spaceType`, `energy`, `motion`,
`color`, `dimension`, `memory`, `memoryDepth`, `memoryDecay`, `memoryDrift`,
`tubeCount`, `radiusVariation`, `metallicResonance`, `couplingStrength`,
`elasticity`, `recoveryTime`, `absorptionDrift`, `nonlinearity`,
`impossibilityDegree`, `pitchEvolutionRate`, `paradoxResonanceFreq`,
`paradoxGain`, `safetyClipDrive`.

Verified by diffing the APVTS parameter IDs passed to each editor's
`setupSlider`/`setupKnob`, `setupCombo`, and `setupToggle` call sites in
`plugin/PluginEditor.cpp` and `plugin/PluginEditorV2.cpp`: the two ID sets
are identical (45 slider/knob setup calls, 5 combo setup calls, 4 toggle
setup calls each — the combo/toggle setup call count includes one helper
definition alongside the invocation sites). Both editors also share the same
`kEditorWidth`/`kEditorHeight` (1100x820, `PluginEditor.cpp:18-19`,
`PluginEditorV2.cpp:25-26`) and the same three-tab `SectionView` model
(`BaseParams`/`Modulation`/`Timeline`).

**Conclusion: no parameter is exposed in one editor and missing from the
other.** The control *surface* is at parity.

## Where they differ

1. **Widget rendering.** Legacy uses plain `juce::Slider` wrapped in
   `LabeledSlider` (`plugin/PluginEditor.h:37-42`). V2 uses
   `monument::PhotorealisticKnob` wrapped in `LabeledKnob`
   (`plugin/PluginEditorV2.h:38-43`, `#include "ui/PhotorealisticKnob.h"`).

2. **Runtime asset dependency.** V2's knob rendering resolves an
   asset-root directory at runtime by probing the current working directory,
   the executable's parent directory, and the application bundle's parent
   directory in sequence (`plugin/PluginEditorV2.cpp:110-120`,
   `findKnobRootFromBase`). Legacy's plain sliders have no equivalent
   external asset dependency. `ARCHITECTURE.md:114-117` already flags that
   V2's asset loading is not bundled via `BinaryData` and must be verified
   against a real installed build.

3. **Per-section collapse/expand state.** V2 adds 11
   `juce::TextButton` group-collapse toggles (`macroModeToggle`,
   `ancientMacroToggle`, `expressiveMacroToggle`, `coreToggle`,
   `routingToggle`, `modulationToggle`, `memoryToggle`, `physicalToggle`,
   `timelineToggle`, `safetyToggle`, `diagnosticsToggle` —
   `plugin/PluginEditorV2.h:90-100`) plus matching `bool ...Expanded` state
   (`plugin/PluginEditorV2.h:171-181`) and `loadUiState()`/`persistUiState()`
   methods (`plugin/PluginEditorV2.cpp:699-745`). Legacy has no group
   collapse mechanism — `plugin/PluginEditor.h` has no equivalent toggles,
   expanded-state booleans, or load/persist methods.

4. **UI state is written into the shared APVTS tree.** V2's
   `persistUiState()` adds a `"ui"` child `ValueTree` under
   `processorRef.getAPVTS().state` (`plugin/PluginEditorV2.cpp:726-734`) to
   store the 11 collapse-state booleans. `getStateInformation()` /
   `setStateInformation()` (`plugin/PluginProcessor.cpp:1313-1325`) save and
   restore the *entire* `parameters.copyState()` tree, so a host session or
   preset saved while V2 is active will carry this extra `"ui"` node. Legacy
   never reads or writes it. No test in this repository exercises
   round-tripping a V2-saved state through the legacy editor's parameter
   attachments (or vice versa) — whether the extra node is silently ignored
   by legacy's `SliderAttachment`/`ComboBoxAttachment`/`ButtonAttachment`
   construction has not been executed/verified here, only inferred from
   JUCE's attachment classes looking up parameters by ID rather than
   iterating all children.

5. **`updateDebugVisibility()`.** Legacy declares and presumably calls
   `updateDebugVisibility()` (`plugin/PluginEditor.h:183`). V2's private
   method list has no equivalent name — V2's debug-mode handling, if any, is
   folded into its collapse/expand mechanism instead. Not traced further
   here (out of scope for a parity headline finding); a human comparing
   debug-mode behavior specifically should diff `updateDebugVisibility()`
   against V2's `updateSectionVisibility()`/group-toggle code directly.

## Maintenance history

```
$ git log --oneline -- plugin/PluginEditor.cpp plugin/PluginEditor.h | head -1
9e9179b feat: expand testing harness, UI, and DSP systems

$ git log --oneline -- plugin/PluginEditorV2.cpp plugin/PluginEditorV2.h | head -1
9e9179b feat: expand testing harness, UI, and DSP systems
```

Both files' most recent touch is the **same commit** (`9e9179b`,
2026-01-12), which simultaneously added 933 lines to `PluginEditor.cpp`
(`git show --stat 9e9179b`) and introduced `PluginEditorV2.cpp` as a new
1292-line file. Neither file has been touched in any of the 53 commits
between `9e9179b` and the current `main` tip (`a406e57`) — i.e. legacy is
not "stale relative to V2"; both editors have been equally untouched since
they were built up together in the same commit.

## CI / test coverage

- No CTest target, source file, or CI workflow references
  `plugin/PluginEditor.h`, `MonumentAudioProcessorEditor` (legacy class
  name), or `MONUMENT_LEGACY_UI`: `grep -rn "MONUMENT_LEGACY_UI"` across
  `.github/`, `scripts/`, and `CMakeLists.txt` returns only the option
  declaration and its `if()` guard in `CMakeLists.txt:218-230` — no
  workflow passes the flag.
- `.github/workflows/qa_legacy_shadow.yml` is unrelated — it is a
  "Legacy DSP Diagnostics" workflow (`name: Legacy DSP Diagnostics`) that
  runs `scripts/run_ci_tests.sh` in diagnostic mode; it has nothing to do
  with `MONUMENT_LEGACY_UI` or the legacy editor.
- **No automated build in this repository has ever compiled with
  `-DMONUMENT_LEGACY_UI=ON`** prior to this task.

## Build verification (done as part of this task)

Reconfigured the existing build directory with
`-DMONUMENT_LEGACY_UI=ON` and built the `Monument` target from clean object
state for `plugin/PluginEditor.cpp`:

```
cmake -S . -B /tmp/mr-task12-build -DMONUMENT_LEGACY_UI=ON
cmake --build /tmp/mr-task12-build --target Monument -j8
```

Result: **builds cleanly** — `plugin/PluginEditor.cpp` compiles with only
pre-existing warnings (an incomplete `switch` over
`ModulationMatrix::DestinationType` missing `Lfo1`/`Lfo2`/`Lfo3` cases at
`plugin/PluginEditor.cpp:26`, deprecated `juce::Font` constructor usage, and
sign-conversion warnings), no errors, and `libMonument_SharedCode.a` links
successfully. This is the first time this configuration has been
compiled in this session's history of the repository (see CI coverage
above) — it was not previously known-good from CI, only now spot-checked.

## What a human needs to decide

This document intentionally stops short of a recommendation. Open
questions for a human:

1. Is the photorealistic knob rendering (V2) required for the shipping
   product, making legacy purely a fallback/debug skin, or is legacy kept
   for a specific reason (e.g. avoiding the runtime asset-root dependency
   in some deployment context)?
2. Is per-section collapse/expand state (and its side effect of adding a
   `"ui"` node to the saved APVTS state tree) desired behavior that should
   also exist in legacy, or is it V2-only by design?
3. Should `-DMONUMENT_LEGACY_UI=ON` get a CI job (even a lightweight
   compile-only check) now that it's confirmed to still build, so future
   changes to `plugin/PluginEditor.cpp` or shared parameter IDs don't
   silently break it again? Or should the flag/file be retired instead,
   given zero CI coverage and 53 commits of no maintenance?
4. If legacy is retired: does anything outside this repository (a user's
   saved project, a support workflow, a debugging habit) depend on being
   able to opt into it?

No deletion, merge, or behavior change was made to either editor or to
`MONUMENT_LEGACY_UI` as part of this task.
