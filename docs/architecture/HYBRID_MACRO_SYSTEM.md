# Hybrid Macro System Design

**Date:** 2026-01-04
**Goal:** Merge Ancient Monuments (10 macros) + Expressive Macros (6 macros) into a single coherent system
**Status:** Design Complete, Ready for Implementation
**Backward Compatibility:** ✅ Preserves all 28 existing presets

---

## Problem

The codebase currently has TWO macro systems:

1. **Ancient Monuments Macros (10 macros)** - User-facing, thematic, used in all 28 presets
   - Stone, Labyrinth, Mist, Bloom, Tempest, Echo, Patina, Abyss, Corona, Breath

2. **Expressive Macros (6 macros)** - Internal mapping system with clean orthogonal design
   - Character, Space Type, Energy, Motion, Color, Dimension

**Conflict:** Cannot run both simultaneously. Migration would break existing presets.

---

## Solution: Hybrid Two-Layer Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    USER INTERFACE                            │
│  Ancient Monuments Macros (10 knobs - thematic names)       │
│  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐                  │
│  │Stone│ │Laby │ │ Mist│ │Bloom│ │Temp │ ...              │
│  └─────┘ └─────┘ └─────┘ └─────┘ └─────┘                  │
└─────────────────────────────────────────────────────────────┘
                         ↓
         ┌───────────────────────────────┐
         │  Hybrid Macro Mapper          │
         │  (10 → 6 translation layer)   │
         └───────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────────┐
│                DSP ENGINE INTERFACE                          │
│  Expressive Macros (6 internal controls - orthogonal)       │
│  ┌─────────┐ ┌─────────┐ ┌─────────┐                       │
│  │Character│ │SpaceType│ │ Energy  │ ...                   │
│  └─────────┘ └─────────┘ └─────────┘                       │
└─────────────────────────────────────────────────────────────┘
                         ↓
         ┌───────────────────────────────┐
         │  ExpressiveMacroMapper        │
         │  (6 → 50+ DSP parameters)     │
         └───────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────────┐
│                  DSP ROUTING GRAPH                           │
│  DspRoutingGraph + all 9 modules                            │
└─────────────────────────────────────────────────────────────┘
```

**Key Insight:** Ancient Monuments macros are the **INTERFACE**, Expressive Macros are the **ENGINE**.

---

## Mapping: Ancient Monuments → Expressive Macros

### Core Principle: Weighted Blending

Each Ancient Monuments macro contributes to multiple Expressive Macros with carefully tuned weights.

```cpp
// File: dsp/HybridMacroMapper.h

namespace monument::dsp
{

/**
 * @brief Translates Ancient Monuments macros to Expressive Macros
 *
 * Preserves thematic names while gaining benefits of clean parameter separation.
 */
class HybridMacroMapper
{
public:
    struct AncientMonumentsInput
    {
        float stone{0.5f};       // Material solidity (intensity)
        float labyrinth{0.5f};   // Spatial complexity (size/depth)
        float mist{0.5f};        // Atmospheric character (softness)
        float bloom{0.5f};       // Energy growth (decay behavior)
        float tempest{0.5f};     // Dynamic motion (modulation)
        float echo{0.5f};        // Reflective depth (space size)
        float patina{0.5f};      // Tonal warmth (age/darkness)
        float abyss{0.5f};       // Infinite depth (feedback/sustain)
        float corona{0.5f};      // Spectral brightness (high freq)
        float breath{0.5f};      // Organic drift (slow evolution)
    };

    struct ExpressiveMacroOutput
    {
        float character{0.5f};   // Subtle → Extreme
        float spaceType{0.2f};   // Chamber → Metallic
        float energy{0.1f};      // Decay → Chaos
        float motion{0.2f};      // Still → Random
        float color{0.5f};       // Dark → Spectral
        float dimension{0.5f};   // Intimate → Infinite
    };

    /**
     * @brief Convert Ancient Monuments → Expressive Macros
     */
    ExpressiveMacroOutput translate(const AncientMonumentsInput& ancient) const noexcept;

private:
    // Weighted mapping coefficients (tuned for musical results)
    // ...
};

} // namespace monument::dsp
```

---

## Detailed Mapping Table

| Ancient Monuments Macro | Primary Target | Secondary Target | Tertiary Target | Rationale |
|------------------------|----------------|------------------|-----------------|-----------|
| **Stone** | Character (80%) | Color (20%) | - | Solidity = intensity + tonal weight |
| **Labyrinth** | Dimension (60%) | Energy (30%) | Motion (10%) | Complexity = size + feedback + drift |
| **Mist** | Color (70%) | Energy (30%) | - | Atmospheric = spectral character + soft decay |
| **Bloom** | Energy (90%) | - | - | Growth = decay behavior (sustain/grow) |
| **Tempest** | Motion (80%) | Character (20%) | - | Dynamic = modulation + intensity |
| **Echo** | Dimension (70%) | Energy (30%) | - | Reflective = space size + feedback depth |
| **Patina** | Color (80%) | Motion (20%) | - | Warmth = tonal darkness + slow drift |
| **Abyss** | Energy (80%) | Dimension (20%) | - | Infinite = feedback/chaos + infinite space |
| **Corona** | Color (90%) | - | - | Brightness = spectral character (high freq) |
| **Breath** | Motion (70%) | Energy (30%) | - | Organic = drift + gentle sustain |

**Space Type:** Determined by combination of Stone, Labyrinth, Mist, Corona
- Low Stone + Low Labyrinth = Chamber (0.2)
- Mid Stone + Mid Labyrinth = Hall (0.4)
- High Corona + Low Mist = Metallic (0.8)
- High Mist + Mid Stone = Shimmer (0.6)

---

## Implementation

### Step 1: Create HybridMacroMapper.h

```cpp
#pragma once

#include "ExpressiveMacroMapper.h"

namespace monument::dsp
{

class HybridMacroMapper final
{
public:
    struct AncientMonumentsInput
    {
        float stone{0.5f};
        float labyrinth{0.5f};
        float mist{0.5f};
        float bloom{0.5f};
        float tempest{0.5f};
        float echo{0.5f};
        float patina{0.5f};
        float abyss{0.5f};
        float corona{0.5f};
        float breath{0.5f};
    };

    using ExpressiveMacroOutput = ExpressiveMacroMapper::MacroInputs;

    HybridMacroMapper() = default;

    /**
     * @brief Translate Ancient Monuments → Expressive Macros
     */
    ExpressiveMacroOutput translate(const AncientMonumentsInput& ancient) const noexcept;

private:
    // Determine Space Type from macro combinations
    float computeSpaceType(const AncientMonumentsInput& ancient) const noexcept;

    // Weighted blending helper
    float blend(float value, float weight) const noexcept { return value * weight; }
};

} // namespace monument::dsp
```

---

### Step 2: Implement HybridMacroMapper.cpp

```cpp
#include "HybridMacroMapper.h"
#include <algorithm>

namespace monument::dsp
{

HybridMacroMapper::ExpressiveMacroOutput HybridMacroMapper::translate(
    const AncientMonumentsInput& ancient) const noexcept
{
    ExpressiveMacroOutput expressive;

    // CHARACTER: Stone (80%) + Tempest (20%)
    expressive.character = juce::jlimit(0.0f, 1.0f,
        blend(ancient.stone, 0.8f) +
        blend(ancient.tempest, 0.2f)
    );

    // SPACE TYPE: Computed from combinations
    expressive.spaceType = computeSpaceType(ancient);

    // ENERGY: Bloom (90%) + Mist (30%) + Labyrinth (30%) + Echo (30%) + Abyss (80%) + Breath (30%)
    // (weighted sum, then normalized)
    const float energySum =
        blend(ancient.bloom, 0.9f) +
        blend(ancient.mist, 0.3f) +
        blend(ancient.labyrinth, 0.3f) +
        blend(ancient.echo, 0.3f) +
        blend(ancient.abyss, 0.8f) +
        blend(ancient.breath, 0.3f);
    expressive.energy = juce::jlimit(0.0f, 1.0f, energySum / 3.4f);  // Normalize

    // MOTION: Tempest (80%) + Labyrinth (10%) + Patina (20%) + Breath (70%)
    const float motionSum =
        blend(ancient.tempest, 0.8f) +
        blend(ancient.labyrinth, 0.1f) +
        blend(ancient.patina, 0.2f) +
        blend(ancient.breath, 0.7f);
    expressive.motion = juce::jlimit(0.0f, 1.0f, motionSum / 1.8f);  // Normalize

    // COLOR: Mist (70%) + Stone (20%) + Patina (80%) + Corona (90%)
    const float colorSum =
        blend(ancient.mist, 0.7f) +
        blend(ancient.stone, 0.2f) +
        blend(ancient.patina, 0.8f) +
        blend(ancient.corona, 0.9f);
    expressive.color = juce::jlimit(0.0f, 1.0f, colorSum / 2.7f);  // Normalize

    // DIMENSION: Labyrinth (60%) + Echo (70%) + Abyss (20%)
    const float dimensionSum =
        blend(ancient.labyrinth, 0.6f) +
        blend(ancient.echo, 0.7f) +
        blend(ancient.abyss, 0.2f);
    expressive.dimension = juce::jlimit(0.0f, 1.0f, dimensionSum / 1.5f);  // Normalize

    return expressive;
}

float HybridMacroMapper::computeSpaceType(const AncientMonumentsInput& ancient) const noexcept
{
    // Space Type ranges: 0.0-0.2 Chamber, 0.2-0.4 Hall, 0.4-0.6 Shimmer, 0.6-0.8 Granular, 0.8-1.0 Metallic

    // High Corona + Low Mist = Metallic (0.8-1.0)
    if (ancient.corona > 0.7f && ancient.mist < 0.4f)
        return 0.8f + (ancient.corona - 0.7f) / 1.5f;  // 0.8-1.0 range

    // High Mist + Mid Stone = Shimmer (0.4-0.6)
    if (ancient.mist > 0.6f && ancient.stone >= 0.3f && ancient.stone <= 0.7f)
        return 0.4f + (ancient.mist - 0.6f) * 0.5f;  // 0.4-0.6 range

    // Mid-High Labyrinth = Hall (0.2-0.4)
    if (ancient.labyrinth > 0.4f && ancient.labyrinth <= 0.7f)
        return 0.2f + (ancient.labyrinth - 0.4f) * 0.67f;  // 0.2-0.4 range

    // Low Stone + Low Labyrinth = Chamber (0.0-0.2)
    if (ancient.stone < 0.4f && ancient.labyrinth < 0.4f)
        return (ancient.labyrinth / 0.4f) * 0.2f;  // 0.0-0.2 range

    // Default: Hall (0.3)
    return 0.3f;
}

} // namespace monument::dsp
```

---

### Step 3: Integrate into PluginProcessor

```cpp
// plugin/PluginProcessor.h

class MonumentAudioProcessor : public juce::AudioProcessor
{
private:
    // Ancient Monuments macros (user-facing parameters)
    juce::AudioParameterFloat* stoneParam{nullptr};
    juce::AudioParameterFloat* labyrinthParam{nullptr};
    juce::AudioParameterFloat* mistParam{nullptr};
    juce::AudioParameterFloat* bloomParam{nullptr};
    juce::AudioParameterFloat* tempestParam{nullptr};
    juce::AudioParameterFloat* echoParam{nullptr};
    juce::AudioParameterFloat* patinaParam{nullptr};
    juce::AudioParameterFloat* abyssParam{nullptr};
    juce::AudioParameterFloat* coronaParam{nullptr};
    juce::AudioParameterFloat* breathParam{nullptr};

    // Translation layer
    std::unique_ptr<monument::dsp::HybridMacroMapper> hybridMapper;
    std::unique_ptr<monument::dsp::ExpressiveMacroMapper> expressiveMapper;

    // DSP engine
    std::unique_ptr<monument::dsp::DspRoutingGraph> routingGraph;
};
```

```cpp
// plugin/PluginProcessor.cpp

void MonumentAudioProcessor::updateDspParameters()
{
    // 1. Read Ancient Monuments macros (user input)
    monument::dsp::HybridMacroMapper::AncientMonumentsInput ancient;
    ancient.stone = stoneParam->get();
    ancient.labyrinth = labyrinthParam->get();
    ancient.mist = mistParam->get();
    ancient.bloom = bloomParam->get();
    ancient.tempest = tempestParam->get();
    ancient.echo = echoParam->get();
    ancient.patina = patinaParam->get();
    ancient.abyss = abyssParam->get();
    ancient.corona = coronaParam->get();
    ancient.breath = breathParam->get();

    // 2. Translate to Expressive Macros (internal representation)
    const auto expressive = hybridMapper->translate(ancient);

    // 3. Compute DSP parameter targets
    const auto targets = expressiveMapper->computeTargets(expressive);

    // 4. Apply to routing graph
    routingGraph->setChambersParams(
        targets.time, targets.mass, targets.density, targets.bloom, targets.gravity
    );
    routingGraph->setPillarsParams(
        targets.density, targets.pillarShape, targets.warp
    );
    // ... apply remaining parameters

    // 5. Load routing preset if Space Type changed
    if (targets.routingPreset != routingGraph->getCurrentPreset())
        routingGraph->loadRoutingPreset(targets.routingPreset);
}
```

---

## Benefits of Hybrid System

### ✅ Preserves Existing User Experience
- Users keep familiar Ancient Monuments macro names
- All 28 factory presets work without migration
- Automation data in DAWs remains valid

### ✅ Gains Technical Benefits
- Clean parameter separation (no conflicts)
- Orthogonal macro design under the hood
- Easier to add new routing presets
- Flexible routing graph system accessible

### ✅ Backward Compatible
- No breaking changes to preset format
- Old presets load directly (no conversion needed)
- DAW sessions from v1.x work in v2.0

### ✅ Future-Proof
- Easy to add new Ancient Monuments macros (just update mapping weights)
- Easy to tweak Expressive Macro behavior (users see no change)
- Can expose "Advanced Mode" UI with 6 Expressive Macros for power users

---

## UI Design

### Standard Mode (Default)

```
┌──────────────────────────────────────────────────────────┐
│  Monument Reverb - Ancient Monuments                     │
├──────────────────────────────────────────────────────────┤
│                                                           │
│  Foundation (1-6)                     Time-Bent (7-10)   │
│  ┌─────┐ ┌─────┐ ┌─────┐             ┌─────┐ ┌─────┐   │
│  │Stone│ │Laby │ │ Mist│             │Patina│ │Abyss│   │
│  └──█──┘ └──█──┘ └──█──┘             └──█──┘ └──█──┘   │
│                                                           │
│  ┌─────┐ ┌─────┐ ┌─────┐             ┌─────┐ ┌─────┐   │
│  │Bloom│ │Temp │ │ Echo│             │Corona│ │Breath│  │
│  └──█──┘ └──█──┘ └──█──┘             └──█──┘ └──█──┘   │
│                                                           │
├──────────────────────────────────────────────────────────┤
│ Routing: [Ancient Way ▼]  Base: Drive █ Air █ Width █   │
└──────────────────────────────────────────────────────────┘
```

### Advanced Mode (Optional Toggle)

```
┌──────────────────────────────────────────────────────────┐
│  Monument Reverb - Advanced Controls                     │
├──────────────────────────────────────────────────────────┤
│  Expressive Macros (Internal Engine)                     │
│                                                           │
│  ┌───────────┐ ┌───────────┐ ┌───────────┐             │
│  │CHARACTER  │ │ SPACE TYPE│ │  ENERGY   │             │
│  │   [==]    │ │   [==]    │ │   [==]    │             │
│  │Computed   │ │ Hall      │ │ Sustain   │             │
│  └───────────┘ └───────────┘ └───────────┘             │
│                                                           │
│  ┌───────────┐ ┌───────────┐ ┌───────────┐             │
│  │  MOTION   │ │   COLOR   │ │ DIMENSION │             │
│  │   [==]    │ │   [==]    │ │   [==]    │             │
│  │ Drift     │ │ Balanced  │ │ Cathedral │             │
│  └───────────┘ └───────────┘ └───────────┘             │
│                                                           │
│  [Read-Only - Computed from Ancient Monuments Above]     │
└──────────────────────────────────────────────────────────┘
```

**Advanced Mode shows computed Expressive Macros (read-only) for transparency.**

---

## Testing Strategy

### Phase 1: Unit Tests

```cpp
TEST_CASE("HybridMacroMapper translation", "[hybrid]")
{
    monument::dsp::HybridMacroMapper mapper;

    SECTION("High Stone → High Character")
    {
        monument::dsp::HybridMacroMapper::AncientMonumentsInput ancient;
        ancient.stone = 1.0f;  // Max intensity

        auto expressive = mapper.translate(ancient);

        REQUIRE(expressive.character >= 0.8f);  // Should be high
    }

    SECTION("High Corona → Metallic Space Type")
    {
        monument::dsp::HybridMacroMapper::AncientMonumentsInput ancient;
        ancient.corona = 0.9f;
        ancient.mist = 0.1f;

        auto expressive = mapper.translate(ancient);

        REQUIRE(expressive.spaceType >= 0.8f);  // Metallic range
    }
}
```

### Phase 2: Preset Validation

Load all 28 factory presets and verify:
- No crashes
- Output matches v1.5 reference recordings
- All Ancient Monuments macro positions preserved

### Phase 3: A/B Testing

Compare v1.5 (direct parameter mapping) vs v2.0 (hybrid system):
- Load same preset in both versions
- Record 30-second audio
- Measure spectral difference (should be minimal)

---

## Migration Path: None Needed!

**Advantage:** No migration required. Existing presets work as-is.

**Preset Format (unchanged):**
```json
{
  "version": 4,
  "name": "Foundational Cathedral",
  "macros": {
    "stone": 0.45,
    "labyrinth": 0.55,
    "mist": 0.40,
    "bloom": 0.50,
    "tempest": 0.30,
    "echo": 0.60,
    "patina": 0.35,
    "abyss": 0.25,
    "corona": 0.50,
    "breath": 0.40
  },
  "routing_mode": "ancient_way",
  "base_params": { ... }
}
```

**Loading Process:**
1. Read Ancient Monuments macro values → stays the same
2. Translate to Expressive Macros → NEW (automatic)
3. Compute DSP targets → NEW (automatic)
4. Apply to routing graph → stays the same

**User sees NO difference.**

---

## Summary

✅ **Hybrid system solves the dual macro conflict**
✅ **Preserves all 28 existing presets (no migration)**
✅ **Keeps Ancient Monuments UI (familiar to users)**
✅ **Gains Expressive Macro benefits (clean separation, routing control)**
✅ **Backward compatible with v1.x**
✅ **Future-proof for power users (optional Advanced Mode)**

**Implementation Effort:** 2-3 days (Week 2, Days 8-10 in plan)

---

**Ready to implement the hybrid system?**
