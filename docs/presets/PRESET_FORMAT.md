# Monument Reverb - User Preset Format Specification

## Overview

Monument Reverb user presets are stored as JSON files with the `.json` extension. The format supports:
- Base and macro DSP parameters (22 normalized float fields)
- Modulation connections ("living" presets), unconditionally serialized/deserialized
- Metadata (name, description)
- Format versioning for backward compatibility

**Current Version:** `5` (`constexpr int kPresetVersion = 5;` in [PresetManager.cpp](../../plugin/PresetManager.cpp))

**Scope note:** This document covers the JSON preset format written by
`PresetManager::saveUserPreset()` / read by `PresetManager::loadUserPreset()`.
It does **not** cover the plugin's full host session state (the `macroMode`,
`routingPreset`, `timelinePreset` parameters, or the "Expressive Macros" —
`character`, `spaceType`, `energy`, `motion`, `color`, `dimension`, defined in
[ExpressiveMacroMapper.h](../../dsp/ExpressiveMacroMapper.h)). Those are
ordinary APVTS-managed plugin parameters, persisted via the host's own
plugin-state round-trip (`getStateInformation`/`setStateInformation`), but
they are **not** written into or read from this preset JSON file — saving a
user preset does not capture them, and loading one does not change them.

---

## File Location

**Default Directory:** `~/Documents/MonumentPresets/`

User presets are automatically saved to and loaded from this directory. The directory is created automatically if it doesn't exist.

**File Naming Convention:**
- User-specified names are converted to legal file names
- Spaces replaced with underscores
- Extension: `.json`
- Example: `My Preset` → `My_Preset.json`

---

## JSON Structure

### Complete Example (Format Version 5)

```json
{
  "formatVersion": 5,
  "name": "My Custom Hall",
  "description": "A warm, evolving space with subtle chaos",
  "parameters": {
    "time": 0.75,
    "mass": 0.60,
    "density": 0.45,
    "bloom": 0.50,
    "gravity": 0.55,
    "warp": 0.20,
    "drift": 0.25,
    "memory": 0.30,
    "memoryDepth": 0.50,
    "memoryDecay": 0.40,
    "memoryDrift": 0.35,
    "mix": 0.60,
    "material": 0.65,
    "topology": 0.40,
    "viscosity": 0.50,
    "evolution": 0.70,
    "chaosIntensity": 0.25,
    "elasticityDecay": 0.10,
    "patina": 0.55,
    "abyss": 0.45,
    "corona": 0.60,
    "breath": 0.15
  },
  "modulation": [
    {
      "source": "AudioFollower",
      "destination": "Bloom",
      "sourceAxis": 0,
      "depth": 0.30,
      "smoothingMs": 250.0,
      "curveType": "Linear",
      "curveAmount": 0.0,
      "enabled": true
    }
  ]
}
```

The `modulation` array is present only if a `ModulationMatrix` pointer was
supplied to `PresetManager` and it has at least one enabled connection at
save time; `PresetManager::saveUserPreset()` skips disabled connections. If
absent, `loadUserPreset()` simply loads an empty modulation list.

---

## Field Definitions

### Root Object

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `formatVersion` | Integer | Yes | Preset format version (current: 5) |
| `name` | String | Yes | Display name of the preset |
| `description` | String | Yes | Human-readable description of sonic character |
| `parameters` | Object | Yes | All base/macro parameter values (see below) |
| `modulation` | Array | No | Modulation connections; omitted if none are enabled |

---

### Parameters Object

All parameter values are **normalized floats** in the range `[0.0, 1.0]`,
read/written via `readFloatProperty()` and `DynamicObject::setProperty()` in
[PresetManager.cpp](../../plugin/PresetManager.cpp). There are 22 fields
total, matching the `PresetValues` struct in
[PresetManager.h](../../plugin/PresetManager.h#L13-L45).

#### Base Parameters (12)

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| `time` | 0.0–1.0 | 0.5 | Reverb decay time (short → long) |
| `mass` | 0.0–1.0 | 0.5 | Material density/weight (light → heavy) |
| `density` | 0.0–1.0 | 0.5 | Echo density (sparse → dense) |
| `bloom` | 0.0–1.0 | 0.5 | Early reflection diffusion |
| `gravity` | 0.0–1.0 | 0.5 | Pitch drift/pull effect |
| `warp` | 0.0–1.0 | 0.0 | Non-Euclidean space distortion |
| `drift` | 0.0–1.0 | 0.0 | Slow parameter evolution over time |
| `memory` | 0.0–1.0 | 0.0 | Memory buffer engagement (0=off) |
| `memoryDepth` | 0.0–1.0 | 0.5 | Memory buffer depth/length |
| `memoryDecay` | 0.0–1.0 | 0.4 | Memory feedback decay rate |
| `memoryDrift` | 0.0–1.0 | 0.3 | Memory playback drift/detuning |
| `mix` | 0.0–1.0 | 0.5 | Dry/wet mix (0=dry, 1=wet) |

**Note:** These `memory*` fields configure `MemoryEchoes` (see
[11-strata.md](../architecture/dsp/memory-system/11-strata.md)), whose
`process()` call is not reached in ordinary builds — see
[ARCHITECTURE.md](../../ARCHITECTURE.md#reachable-signal-chain-ancientway-the-default).
The parameters still round-trip through presets and the host parameter
surface; they just don't currently affect the rendered sound outside the
`MONUMENT_MEMORY_PROVE` debug build.

#### Ancient Macro Parameters (6) — Added in Format Version 2

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| `material` | 0.0–1.0 | 0.5 | Soft → Hard material (affects Time, Mass, Density) |
| `topology` | 0.0–1.0 | 0.5 | Regular → Non-Euclidean space (affects Warp) |
| `viscosity` | 0.0–1.0 | 0.5 | Airy → Thick medium (affects Air/damping) |
| `evolution` | 0.0–1.0 | 0.5 | Static → Blooming/changing (affects Bloom, Drift) |
| `chaosIntensity` | 0.0–1.0 | 0.0 | Stable → Chaotic behavior |
| `elasticityDecay` | 0.0–1.0 | 0.0 | Instant recovery → Slow deformation |

These are the "Ancient" macros (`dsp/MacroMapper.h/cpp`, `macroMode == 0`).
They control multiple base parameters simultaneously; they are distinct
from the "Expressive" macros (`character`, `spaceType`, `energy`, `motion`,
`color`, `dimension`, `macroMode == 1`), which are **not** part of this
preset format (see the Scope note above).

#### Ancient Monuments Macros 7-10 (4) — Added in Format Version 4

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| `patina` | 0.0–1.0 | 0.5 | Surface weathering/coloration character |
| `abyss` | 0.0–1.0 | 0.5 | Depth/void character |
| `corona` | 0.0–1.0 | 0.5 | Halo/shimmer character |
| `breath` | 0.0–1.0 | 0.0 | Breathing/pulsing motion amount |

**Note:** Macro parameters control multiple base parameters simultaneously through the MacroMapper system. See [ARCHITECTURE.md](../../ARCHITECTURE.md) for mapping details.

---

## Format Version History

Verified against `kPresetVersion` and the load/save code in
[PresetManager.cpp](../../plugin/PresetManager.cpp); dates for versions 3-5
are not recorded in source comments and are left unspecified below rather
than guessed.

### Version 5 (Current)
**Changes:**
- Added modulation curve metadata (`curveType`, `curveAmount`) to each
  serialized modulation connection.

### Version 4
**Changes:**
- Added the `patina`, `abyss`, `corona`, `breath` macro fields (Ancient
  Monuments macros 7-10).
- v3 presets missing these fields default to `patina=0.5, abyss=0.5,
  corona=0.5, breath=0.0` on load.

### Version 3
**Changes:**
- Added `modulation` array serialization for modulation connections
  (source, destination, sourceAxis, depth, smoothingMs, enabled). This was
  previously described as a "future" feature in this document; it has
  shipped since this version.

### Version 2
**Date:** 2026-01-03
**Changes:**
- Added 6 macro parameters (`material`, `topology`, `viscosity`,
  `evolution`, `chaosIntensity`, `elasticityDecay`) to serialization.
- Backward compatible with v1 (missing macros default to their struct
  defaults).

### Version 1 (Legacy)
**Date:** 2025-12-09
**Limitations:**
- Only saved the 12 base parameters.
- Macro parameters were **not** serialized.

**Migration:** Older presets are automatically upgraded on load — any field
missing from the JSON falls back to the `PresetValues` struct default
(`readFloatProperty()` returns the fallback when a key is absent).

---

## Modulation Connections

**Status:** ✅ Implemented since format version 3 — this is not a future
feature. `PresetManager::saveUserPreset()` unconditionally serializes every
*enabled* connection from the `ModulationMatrix` passed to the
`PresetManager` constructor, and `loadUserPreset()` unconditionally
deserializes them back into `PresetValues::modulationConnections`.

### Structure

```json
{
  "formatVersion": 5,
  "name": "Living Preset Example",
  "description": "Breathing walls that respond to input",
  "parameters": { "...": "..." },
  "modulation": [
    {
      "source": "AudioFollower",
      "destination": "Bloom",
      "sourceAxis": 0,
      "depth": 0.30,
      "smoothingMs": 250.0,
      "curveType": "Linear",
      "curveAmount": 0.0,
      "enabled": true
    },
    {
      "source": "ChaosAttractor",
      "destination": "Warp",
      "sourceAxis": 0,
      "depth": 0.45,
      "smoothingMs": 300.0,
      "curveType": "EaseIn",
      "curveAmount": 0.5,
      "enabled": true
    }
  ]
}
```

#### Modulation Field Reference

| Field | Type | Description |
|-------|------|-------------|
| `source` | String | Enum, see `sourceTypeToString()`: `AudioFollower`, `BrownianMotion`, `ChaosAttractor`, `EnvelopeTracker`, `Lfo1`-`Lfo6`, `MidiCC`, `MidiPitchBend`, `MidiChannelPressure` |
| `destination` | String | Enum, see `destinationTypeToString()`: e.g. `Time`, `Mass`, `Density`, `Bloom`, `Air`, `Width`, `Mix`, `Warp`, `Drift`, `Gravity`, `PillarShape`, `TubeCount`, `RadiusVariation`, `MetallicResonance`, `CouplingStrength`, `Elasticity`, `RecoveryTime`, `AbsorptionDrift`, `Nonlinearity`, `ImpossibilityDegree`, `PitchEvolutionRate`, `ParadoxResonanceFreq`, `ParadoxGain`, `PositionX`, `PositionY`, `PositionZ`, `Distance`, `VelocityX` |
| `sourceAxis` | Integer | Axis for multi-dimensional sources (e.g. Chaos X/Y/Z: 0/1/2) |
| `depth` | Float | Modulation amount (bipolar, -1 to +1 per the `Connection` struct, though factory presets use 0-1) |
| `smoothingMs` | Float | Lag filter time constant, milliseconds |
| `curveType` | String | Enum, see `curveTypeToString()`: `Linear`, `EaseIn`, `EaseOut`, `Sine`, `SCurve`, `Steps` (added in v5) |
| `curveAmount` | Float | 0.0-1.0 curve intensity, ignored for `Linear` (added in v5) |
| `enabled` | Boolean | Always `true` for a connection present in this array — disabled connections are skipped at save time |

**Known gap:** `ModulationMatrix::Connection` also has a `probability` field
(probability gate for intermittent modulation, `dsp/ModulationMatrix.h`),
but `PresetManager` does not read or write it — a connection's probability
is not persisted through this preset format. This is a pre-existing gap in
the current implementation, not something this document is proposing to
fix.

**Implementation:** See [ModulationMatrix.h](../../dsp/ModulationMatrix.h) for full source/destination/curve enum definitions.

---

## Validation Rules

### Required Fields
- All root fields (`formatVersion`, `name`, `description`, `parameters`) must be present
- No individual parameter field is strictly required — any missing key falls back to the `PresetValues` struct default
- `modulation` is optional; if present but empty or malformed entries are skipped

### Value Constraints
- All parameter values are normalized `[0.0, 1.0]`; `setParamNormalized()` clamps out-of-range values to `[0.0, 1.0]` when applying them to APVTS
- `formatVersion` is informational only — `loadUserPreset()` does not branch on it; it relies on per-field presence/absence for backward compatibility

### Error Handling
- Invalid JSON / missing `parameters` object with no top-level fallback: `loadUserPreset()` returns `false`, preset not applied
- Missing individual parameters: use `PresetValues` struct defaults (see [PresetManager.h:13-45](../../plugin/PresetManager.h#L13-L45))
- Unknown parameters/fields: ignored (forward compatibility)
- Unknown `source`/`destination`/`curveType` strings in a modulation entry fall back to a default enum value (`ChaosAttractor`/`Warp`/`Linear` respectively) rather than being rejected

---

## Usage Examples

### Save Current State as User Preset (C++)

```cpp
// In PluginProcessor or UI callback
presetManager.saveUserPreset("My Custom Sound", "A description of the sonic character");
// Saves to: ~/Documents/MonumentPresets/My_Custom_Sound.json
```

### Load User Preset from File (C++)

```cpp
juce::File presetFile = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
    .getChildFile("MonumentPresets/My_Custom_Sound.json");

if (presetManager.loadUserPreset(presetFile)) {
    // Preset loaded successfully
    // UI and DSP parameters updated
    // presetManager.getLastLoadedModulationConnections() now holds the
    // connections read from this preset, for the caller to apply
} else {
    // Handle load failure
}
```

### Manually Create a Preset (JSON)

1. Create a new `.json` file in `~/Documents/MonumentPresets/`
2. Use the structure from the example above
3. Set parameter values in range `[0.0, 1.0]`
4. Load via the plugin's user preset browser

---

## Factory Presets

**Location:** Hardcoded in [PresetManager.cpp](../../plugin/PresetManager.cpp), the `kFactoryPresets` array.

**Count:** 37 factory presets (`kNumFactoryPresets = 37`), organized in the
array in this order (1-indexed; index ranges are not all contiguous by
category — see [PRESET_GALLERY.md](../PRESET_GALLERY.md) for the full,
verified breakdown):

1. **Foundational Spaces (1-18):** 18 architectural reverbs without modulation — Init Patch, Stone Hall, High Vault, Cold Chamber, Night Atrium, Monumental Void, Stone Circles, Cathedral of Glass, Zero-G Garden, Weathered Nave, Dust in the Columns, Frozen Monument (Engage Freeze), Ruined Monument (Remembers), What the Hall Kept, Event Horizon, Folded Atrium, Hall of Mirrors, Tesseract Chamber.
2. **Living Spaces, Phase 3 (19-23):** 5 presets with hardcoded modulation connections — Breathing Stone, Drifting Cathedral, Chaos Hall, Living Pillars, Event Horizon Evolved.
3. **Physical Modeling Spaces, Phase 5 (24-28):** 5 presets exercising the tube/elastic/alien-physics modules — Metallic Corridor, Elastic Cathedral, Impossible Chamber, Breathing Tubes, Quantum Hall.
4. **Living Spaces, Phase 6 (29-37):** 9 further modulation-driven presets — Pulsing Cathedral, Dynamic Shimmer, Quantum Shimmer, Morphing Cathedral, Fractal Space, Elastic Drift, Spectral Wander, Impossible Hall, Breathing Chaos.

**Special:** Presets in groups 2 and 4 (19-23, 29-37 — 14 presets total)
include hardcoded modulation connections. Their connections are *readable*
by a user via `getLastLoadedModulationConnections()` after loading, and a
user preset saved from that state will now correctly persist those
connections (format v3+) — this is no longer "not user-editable via JSON."

---

## Technical References

### Related Files
- **Implementation:** [PresetManager.cpp](../../plugin/PresetManager.cpp)
- **Header:** [PresetManager.h](../../plugin/PresetManager.h)
- **Architecture:** [ARCHITECTURE.md](../../ARCHITECTURE.md)
- **Modulation Matrix:** [ModulationMatrix.h](../../dsp/ModulationMatrix.h)
- **Preset Gallery:** [PRESET_GALLERY.md](../PRESET_GALLERY.md)

### Key Functions (see file for current line numbers; not repeated here to avoid drift)
- `PresetManager::saveUserPreset()`
- `PresetManager::loadUserPreset()`
- `PresetManager::captureCurrentValues()`
- `PresetManager::applyPreset()`

---

## Troubleshooting

### Preset Loads but Sounds Different Than Expected
**Cause:** A connection's `probability` field is never saved (see "Known
gap" above), and Expressive Macro / `macroMode` / `routingPreset` /
`timelinePreset` state is not part of this preset format at all — it
carries over from whatever the plugin's current session state is.

### Can't Find Preset Directory
**Check:** `~/Documents/MonumentPresets/`
**Create manually:** `mkdir -p ~/Documents/MonumentPresets/`

### JSON Parse Error
**Validate:** Use `jsonlint` or online JSON validators
**Check:** Trailing commas (not allowed in JSON), quotes around strings, value ranges

---

## Schema Summary

```typescript
interface MonumentPreset {
  formatVersion: number;  // Current: 5
  name: string;
  description: string;
  parameters: {
    // Base Parameters (12)
    time: number;         // [0.0, 1.0]
    mass: number;
    density: number;
    bloom: number;
    gravity: number;
    warp: number;
    drift: number;
    memory: number;
    memoryDepth: number;
    memoryDecay: number;
    memoryDrift: number;
    mix: number;

    // Ancient Macro Parameters (6) — v2+
    material: number;     // [0.0, 1.0]
    topology: number;
    viscosity: number;
    evolution: number;
    chaosIntensity: number;
    elasticityDecay: number;

    // Ancient Monuments Macros 7-10 (4) — v4+
    patina: number;       // [0.0, 1.0]
    abyss: number;
    corona: number;
    breath: number;
  };

  // Modulation connections — v3+ (curveType/curveAmount added v5)
  modulation?: Array<{
    source: string;
    destination: string;
    sourceAxis: number;
    depth: number;
    smoothingMs: number;
    curveType: string;
    curveAmount: number;
    enabled: boolean;
  }>;
}
```

---

**Last Updated:** 2026-09-05
**Format Version:** 5
**Maintainer:** Monument Reverb Development Team
