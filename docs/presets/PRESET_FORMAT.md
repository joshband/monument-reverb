# Monument Reverb - User Preset Format Specification

## Overview

Monument Reverb user presets are stored as JSON files with the `.json` extension. The format supports:
- Base DSP parameters (12 parameters)
- Macro control parameters (6 parameters)
- Metadata (name, description)
- Format versioning for backward compatibility

**Current Version:** `2` (as of Phase 2 completion)

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

### Complete Example (Format Version 2)

```json
{
  "formatVersion": 2,
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
    "elasticityDecay": 0.10
  }
}
```

---

## Field Definitions

### Root Object

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `formatVersion` | Integer | Yes | Preset format version (current: 2) |
| `name` | String | Yes | Display name of the preset |
| `description` | String | Yes | Human-readable description of sonic character |
| `parameters` | Object | Yes | All parameter values (see below) |

---

### Parameters Object

All parameter values are **normalized floats** in the range `[0.0, 1.0]`.

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

#### Macro Parameters (6) — Added in Format Version 2

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| `material` | 0.0–1.0 | 0.5 | Soft → Hard material (affects Time, Mass, Density) |
| `topology` | 0.0–1.0 | 0.5 | Regular → Non-Euclidean space (affects Warp) |
| `viscosity` | 0.0–1.0 | 0.5 | Airy → Thick medium (affects Air/damping) |
| `evolution` | 0.0–1.0 | 0.5 | Static → Blooming/changing (affects Bloom, Drift) |
| `chaosIntensity` | 0.0–1.0 | 0.0 | Stable → Chaotic behavior |
| `elasticityDecay` | 0.0–1.0 | 0.0 | Instant recovery → Slow deformation |

**Note:** Macro parameters control multiple base parameters simultaneously through the MacroMapper system. See [ARCHITECTURE_REVIEW.md](../architecture/ARCHITECTURE_REVIEW.md) for mapping details.

---

## Format Version History

### Version 2 (Current)
**Date:** 2026-01-03
**Changes:**
- Added 6 macro parameters to serialization
- Macro parameters now persist in user presets
- Backward compatible with v1 (missing macros default to 0.5)

### Version 1 (Legacy)
**Date:** 2025-12-09
**Limitations:**
- Only saved 12 base parameters
- Macro parameters were **not** serialized
- User presets lost macro control values on save/load

**Migration:** Version 1 presets are automatically upgraded on load. Missing macro parameters use default values (0.5).

---

## Modulation Connections (Future)

**Status:** ❌ Not Yet Implemented in User Presets

Factory presets in [PresetManager.cpp](../../plugin/PresetManager.cpp) support modulation connections internally, but these are **not yet serialized to JSON** for user presets.

### Planned Structure (Format Version 3)

```json
{
  "formatVersion": 3,
  "name": "Living Preset Example",
  "description": "Breathing walls that respond to input",
  "parameters": { ... },
  "modulation": [
    {
      "source": "AudioFollower",
      "destination": "Bloom",
      "depth": 0.30,
      "sourceAxis": 0,
      "smoothingMs": 250.0,
      "enabled": true
    },
    {
      "source": "ChaosAttractor",
      "destination": "Warp",
      "depth": 0.45,
      "sourceAxis": 0,
      "smoothingMs": 300.0,
      "enabled": true
    }
  ]
}
```

#### Modulation Field Reference

| Field | Type | Description |
|-------|------|-------------|
| `source` | String | Enum: `AudioFollower`, `BrownianMotion`, `ChaosAttractor`, `EnvelopeTracker` |
| `destination` | String | Target parameter (e.g., `Bloom`, `Warp`, `Density`) |
| `depth` | Float (0.0–1.0) | Modulation intensity |
| `sourceAxis` | Integer (0–2) | Axis for multi-dimensional sources (Chaos: X/Y/Z) |
| `smoothingMs` | Float | Smoothing time in milliseconds |
| `enabled` | Boolean | Enable/disable connection |

**Implementation:** See [ModulationMatrix.h](../../plugin/dsp/ModulationMatrix.h) for full source/destination enum definitions.

---

## Validation Rules

### Required Fields
- All root fields (`formatVersion`, `name`, `description`, `parameters`) must be present
- All 12 base parameters must be present
- Macro parameters are optional (use defaults if missing for v1 compatibility)

### Value Constraints
- All parameter values must be in range `[0.0, 1.0]`
- Values outside range are clamped during load
- `formatVersion` must be a positive integer

### Error Handling
- Invalid JSON: File load returns `false`, preset not applied
- Missing parameters: Use struct default values (see [PresetManager.h:14-38](../../plugin/PresetManager.h#L14-L38))
- Unknown parameters: Ignored (forward compatibility)

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
} else {
    // Handle load failure
}
```

### Manually Create a Preset (JSON)

1. Create a new `.json` file in `~/Documents/MonumentPresets/`
2. Use the structure from the example above
3. Set parameter values in range `[0.0, 1.0]`
4. Load via plugin UI (when user preset browser is implemented)

---

## Factory Presets

**Location:** Hardcoded in [PresetManager.cpp:79-151](../../plugin/PresetManager.cpp#L79-L151)

**Count:** 23 factory presets organized into 5 categories:
1. **🏛️ Foundational Spaces (0-5):** Clean starting points
2. **🌱 Living Spaces (6-11):** Organic, evolving characters
3. **📜 Remembering Spaces (12-14):** Memory buffer presets
4. **⏳ Time-Bent/Abstract (15-17):** Non-Euclidean geometry
5. **🌀 Evolving Spaces (18-22):** Phase 3 "Living" presets with modulation

**Special:** Presets 18-22 include hardcoded modulation connections that are **not** user-editable via JSON (yet).

---

## Future Enhancements

### Planned Features (Roadmap)
- [ ] **Format v3:** Modulation connection serialization
- [ ] **User Preset Browser UI:** Visual card-based selection
- [ ] **Preset Tags/Categories:** User-defined organization
- [ ] **Preset Thumbnails:** Generated PBR chamber visualizations
- [ ] **Preset Morphing:** Interpolate between two presets
- [ ] **Cloud Preset Sharing:** Community preset library

---

## Technical References

### Related Files
- **Implementation:** [PresetManager.cpp](../../plugin/PresetManager.cpp)
- **Header:** [PresetManager.h](../../plugin/PresetManager.h)
- **Architecture:** [ARCHITECTURE_REVIEW.md](../architecture/ARCHITECTURE_REVIEW.md)
- **Macro System:** [QUICK_START_MACRO_TESTING.md](../development/QUICK_START_MACRO_TESTING.md)

### Key Functions
- `PresetManager::saveUserPreset()` — Line 200
- `PresetManager::loadUserPreset()` — Line 244
- `PresetManager::captureCurrentValues()` — Line 289
- `PresetManager::applyPreset()` — Line 324

---

## Troubleshooting

### Preset Doesn't Save Macro Values
**Solution:** Ensure you're using format version 2 or later. Rebuild plugin after updating PresetManager.cpp.

### Preset Loads but Sounds Different
**Cause:** Modulation connections are not saved in user presets (format v2).
**Workaround:** Use factory presets 18-22 as starting points for "living" sounds.

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
  formatVersion: number;  // Current: 2
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

    // Macro Parameters (6) — v2+
    material: number;     // [0.0, 1.0]
    topology: number;
    viscosity: number;
    evolution: number;
    chaosIntensity: number;
    elasticityDecay: number;
  };

  // Future (v3)
  modulation?: ModulationConnection[];
}
```

---

**Last Updated:** 2026-01-03
**Format Version:** 2
**Maintainer:** Monument Reverb Development Team
