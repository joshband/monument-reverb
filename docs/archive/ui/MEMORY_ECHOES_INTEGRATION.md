# MemoryEchoes Integration into Experimental Redesign

**Date**: 2026-01-04
**Status**: Design Proposal

---

## Overview

**MemoryEchoes** is Monument's existing "memory surfacing" feature that captures reverb output and randomly recalls past audio fragments. This creates unpredictable, organic sonic moments - perfect for the experimental redesign.

**Current Implementation**:
- Captures post-Chambers wet output into short/long buffers
- Randomly surfaces old audio with fade in/hold/fade out envelopes
- Supports drift (pitch modulation), decay (filtering), and freeze

**Why Integrate**:
1. **Already experimental** - fits the "playful exploration" theme perfectly
2. **Creates dramatic diversity** - memories create unpredictable, interesting results
3. **Routable module** - can be positioned anywhere in the signal chain
4. **Macro-compatible** - Evolution/Chaos macros naturally control memory behavior
5. **Modulation target** - audio follower can trigger memory surfacing dynamically

---

## Enhancement #1: Make MemoryEchoes Routable

### Current Problem
MemoryEchoes is hardcoded between Chambers and Weathering:
```
Chambers → MemoryEchoes.capture() → Weathering → ... → MemoryEchoes.process()
```

### Solution: Add MemoryEchoes to DspRoutingGraph

Update `dsp/DspRoutingGraph.h`:

```cpp
enum class ModuleType
{
    Foundation,
    Pillars,
    Chambers,
    MemoryEchoes,      // ← Add as routable module
    Weathering,
    TubeRayTracer,
    ElasticHallway,
    AlienAmplification,
    Buttress,
    Facade,
    Count
};

class DspRoutingGraph
{
public:
    // New parameter setter
    void setMemoryEchoesParams(float memory, float depth, float decay, float drift);

private:
    std::unique_ptr<MemoryEchoes> memoryEchoes;  // ← Add to module list
};
```

### New Routing Possibilities

**Preset #1: "Ghostly Cathedral"**
```
Foundation → Pillars → Chambers → MemoryEchoes → Weathering → Facade
```
*Traditional placement, memories of reverb tail*

**Preset #2: "Fragmented Reality"**
```
Foundation → Pillars → MemoryEchoes → Chambers → Weathering → Facade
```
*Memories of early reflections (more percussive, rhythmic)*

**Preset #3: "Recursive Haunt"**
```
Foundation → Chambers → MemoryEchoes ⟲ Feedback to Pillars → Facade
```
*Memories feed back into early reflections (unstable, chaotic)*

**Preset #4: "Metallic Memories"**
```
Foundation → TubeRayTracer → MemoryEchoes → AlienAmplification → Facade
```
*Memories of tube resonances (ringing, spectral)*

---

## Enhancement #2: Macro Control Integration

### Expressive Macro Mappings

**Energy → Memory Intensity**
- **Decay (0.0-0.2)**: Memory = 0.0 (no memories)
- **Sustain (0.3-0.5)**: Memory = 0.3 (occasional memories)
- **Grow (0.6-0.8)**: Memory = 0.7 (frequent memories)
- **Chaos (0.9-1.0)**: Memory = 1.0 (constant memory surfacing)

**Motion → Memory Drift**
- **Still (0.0-0.2)**: Drift = 0.0 (stable pitch)
- **Drift (0.3-0.5)**: Drift = 0.4 (subtle detuning)
- **Pulse (0.6-0.8)**: Drift = 0.7 (rhythmic pitch shifts)
- **Random (0.9-1.0)**: Drift = 1.0 (extreme pitch chaos)

**Character → Memory Depth**
- **Subtle (0.0-0.3)**: Depth = 0.2 (quiet memories, background)
- **Balanced (0.4-0.6)**: Depth = 0.5 (balanced mix)
- **Extreme (0.7-1.0)**: Depth = 0.9 (loud, prominent memories)

### Updated ExpressiveMacroMapper

Add to `dsp/ExpressiveMacroMapper.cpp`:

```cpp
ParameterTargets ExpressiveMacroMapper::computeTargets(const MacroInputs& macros) const
{
    ParameterTargets targets;

    // ... existing macro mappings

    // Memory Echoes mappings
    targets.memoryAmount = mapEnergyToMemory(macros.energy);
    targets.memoryDrift = mapMotionToDrift(macros.motion);
    targets.memoryDepth = mapCharacterToMemoryDepth(macros.character);
    targets.memoryDecay = mapColorToMemoryDecay(macros.color);

    return targets;
}

float ExpressiveMacroMapper::mapEnergyToMemory(float energy) const
{
    // Decay = 0.0, Sustain = 0.3, Grow = 0.7, Chaos = 1.0
    if (energy < 0.2f) return 0.0f;
    if (energy < 0.5f) return juce::jmap(energy, 0.2f, 0.5f, 0.0f, 0.3f);
    if (energy < 0.8f) return juce::jmap(energy, 0.5f, 0.8f, 0.3f, 0.7f);
    return juce::jmap(energy, 0.8f, 1.0f, 0.7f, 1.0f);
}

float ExpressiveMacroMapper::mapCharacterToMemoryDepth(float character) const
{
    return juce::jmap(character, 0.0f, 1.0f, 0.2f, 0.9f);
}
```

---

## Enhancement #3: Experimental Modulation Targets

### Add MemoryEchoes to ModulationMatrix

Update `dsp/ModulationMatrix.h`:

```cpp
enum class DestinationType
{
    // ... existing destinations
    MemoryAmount,          // Memory surfacing frequency
    MemoryDepth,           // Memory loudness
    MemoryDecay,           // Memory filtering
    MemoryDrift,           // Memory pitch modulation
    Count
};
```

### Experimental Modulation Examples

**Example #1: Dynamic Memories**
```yaml
Modulation:
  - AudioFollower → Memory Amount (Depth: 0.8, Probability: 60%)

Behavior:
  - Loud input triggers memory surfacing 60% of the time
  - Creates dynamic, responsive ghosting effect
  - Quiet passages = no memories, loud passages = frequent memories
```

**Example #2: Chaotic Memory Drift**
```yaml
Modulation:
  - ChaosAttractor.X → Memory Drift (Depth: 0.7, Quantized: 12 steps)

Behavior:
  - Chaos modulates pitch of surfaced memories
  - Quantized to 12 steps (semitone-like jumps)
  - Creates melodic, unstable hauntings
```

**Example #3: Cross-Modulated Memory Decay**
```yaml
Modulation:
  - Brownian Motion → Memory Decay (Depth: 0.5)
  - AudioFollower → Brownian.Rate (Cross-Mod, Depth: 0.8)

Behavior:
  - Brownian slowly drifts memory filtering
  - Audio follower speeds up drift rate during loud passages
  - Creates organic, breathing filter movements
```

**Example #4: Gesture-Controlled Memories**
```yaml
Modulation:
  - Recorded Gesture → Memory Amount (Depth: 1.0, Speed: 0.5x)

Behavior:
  - User records manual Memory knob movements
  - Gesture plays back at half speed, looping
  - Creates personalized, rhythmic memory patterns
```

---

## Enhancement #4: Preset Examples with Memory

### Preset #1: "Ghostly Cathedral"
```yaml
character: 0.5
spaceType: 0.3 (Hall)
energy: 0.6 (Grow)
motion: 0.3 (Drift)
color: 0.4 (Balanced)
dimension: 0.7 (Cathedral)

routing: TraditionalCathedral
memoryPlacement: After Chambers (default)

modulation:
  - AudioFollower → Memory Amount (Depth: 0.6)
    → Memories surface during loud passages

result: Cathedral reverb with ghostly echoes that respond to input dynamics
```

### Preset #2: "Fragmented Reality"
```yaml
character: 0.8
spaceType: 0.7 (Granular)
energy: 0.9 (Chaos)
motion: 0.9 (Random)
color: 0.6 (Bright)
dimension: 0.9 (Infinite)

routing: MetallicGranular
memoryPlacement: Before Chambers (early reflection memories)

modulation:
  - ChaosAttractor.X → Memory Drift (Depth: 0.8, Quantized: 12)
  - Brownian → Memory Decay (Depth: 0.5, Probability: 70%)

result: Granular texture with fragmented, pitch-shifting memories
```

### Preset #3: "Recursive Haunt"
```yaml
character: 0.7
spaceType: 0.5 (Shimmer)
energy: 0.8 (Grow)
motion: 0.5 (Drift)
color: 0.8 (Bright)
dimension: 0.85 (Infinite)

routing: ElasticFeedbackDream
memoryPlacement: In feedback loop (memories feed back into Pillars)

modulation:
  - AudioFollower → Memory Amount (Depth: 0.9)
  - Chaos → Memory Drift (Depth: 0.6)

result: Shimmer reverb with recursive memories that build and evolve
```

### Preset #4: "Metallic Haunt"
```yaml
character: 0.9
spaceType: 0.95 (Metallic)
energy: 0.7 (Grow)
motion: 0.8 (Pulse)
color: 0.9 (Spectral)
dimension: 1.0 (Infinite)

routing: ImpossibleChaos
memoryPlacement: After TubeRayTracer (tube resonance memories)

modulation:
  - Chaos → Tube Count (Depth: 0.5)
  - Audio → Memory Amount (Depth: 0.8, Probability: 50%)
  - Chaos → Memory Drift (Depth: 0.9, Quantized: 5)

result: Metallic reverb with ringing, spectral memories (impossible acoustic space)
```

---

## Implementation Steps

### Step 1: Add MemoryEchoes to Routing Graph

Update `dsp/DspRoutingGraph.cpp`:

```cpp
void DspRoutingGraph::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    // ... existing module preparation

    memoryEchoes = std::make_unique<MemoryEchoes>();
    memoryEchoes->prepare(sampleRate, maxBlockSize, numChannels);
}

void DspRoutingGraph::processModule(ModuleType module, juce::AudioBuffer<float>& buffer)
{
    switch (module)
    {
        // ... existing modules

        case ModuleType::MemoryEchoes:
            memoryEchoes->process(buffer);
            break;
    }
}

void DspRoutingGraph::setMemoryEchoesParams(float memory, float depth,
                                             float decay, float drift)
{
    memoryEchoes->setMemory(memory);
    memoryEchoes->setDepth(depth);
    memoryEchoes->setDecay(decay);
    memoryEchoes->setDrift(drift);
}
```

### Step 2: Add Memory Parameters to ExpressiveMacroMapper

Update `dsp/ExpressiveMacroMapper.h`:

```cpp
struct ParameterTargets
{
    // ... existing parameters

    // Memory Echoes parameters
    float memoryAmount{0.0f};
    float memoryDepth{0.5f};
    float memoryDecay{0.4f};
    float memoryDrift{0.3f};
};
```

### Step 3: Add Memory Destinations to ModulationMatrix

Update `dsp/ModulationMatrix.h` and `.cpp` to include memory parameters as modulation destinations.

### Step 4: Update UI (Advanced Panel)

Add memory controls to the advanced panel (hidden by default):

```cpp
// In MonumentAudioProcessorEditor.h
HeroKnob memoryAmountKnob;
HeroKnob memoryDepthKnob;
HeroKnob memoryDecayKnob;
HeroKnob memoryDriftKnob;

// Only visible in Advanced mode
memoryAmountKnob.setVisible(currentView == ViewMode::Advanced);
```

### Step 5: Create Preset Variations

Create 5 presets that showcase MemoryEchoes in different routing positions:
1. "Ghostly Cathedral" (after Chambers)
2. "Fragmented Reality" (before Chambers)
3. "Recursive Haunt" (feedback loop)
4. "Metallic Haunt" (after TubeRayTracer)
5. "Elastic Memories" (after ElasticHallway)

---

## Enhanced Features (Optional Phase 6)

### Feature #1: Memory Capture Triggers

Allow manual or modulated triggers for memory capture:

```cpp
class MemoryEchoes
{
public:
    void triggerCapture();  // Manual capture
    void setAutoCaptureThreshold(float threshold);  // Capture when RMS > threshold
};

// Usage in modulation:
// AudioFollower → Memory Capture Trigger (when input > -6 dBFS)
```

### Feature #2: Stereo Memory Panning

Pan surfaced memories across the stereo field:

```cpp
void MemoryEchoes::setStereoSpread(float spread);  // 0.0 = mono, 1.0 = wide

// Modulation: Chaos.Y → Memory Stereo Spread (Depth: 0.7)
// Result: Memories jump around the stereo field chaotically
```

### Feature #3: Memory Grain Size

Control the duration of surfaced memory fragments:

```cpp
void MemoryEchoes::setGrainSize(float size);  // 0.0 = short (50ms), 1.0 = long (2000ms)

// Macro: Dimension → Memory Grain Size
// Intimate = short grains (percussive), Infinite = long grains (sustained)
```

---

## Benefits of Integration

### Sonic Diversity
- **5 new routing presets** using MemoryEchoes in different positions
- **Unpredictable results** from random memory surfacing
- **Dynamic response** to input via audio follower modulation

### Experimental Exploration
- **Probability gates** make memories intermittent (30% chance to surface)
- **Quantized drift** creates melodic, stepped pitch shifts
- **Cross-modulation** allows chaos to control memory behavior
- **Gesture recording** lets users create personalized memory patterns

### Musical Utility
- **Subtle ghosting** for ambient/cinematic music (low Memory Amount)
- **Rhythmic fragments** for electronic/experimental music (high Memory Amount + quantization)
- **Evolving textures** for sound design (chaos + cross-modulation)

---

## Testing Strategy

### Test 1: Routing Positions
1. Load "Ghostly Cathedral" (memory after Chambers)
2. Load "Fragmented Reality" (memory before Chambers)
3. **Verify**: Memories sound fundamentally different (smooth tail vs percussive fragments)

### Test 2: Macro Integration
1. Set Energy = 0.1 (Decay) → Memory Amount should be 0.0
2. Set Energy = 0.9 (Chaos) → Memory Amount should be 1.0 (frequent surfacing)
3. **Verify**: Energy macro controls memory behavior coherently

### Test 3: Experimental Modulation
1. Add: AudioFollower → Memory Amount (Depth: 0.8)
2. Play loud passage → Memories should surface frequently
3. Play quiet passage → Memories should stop
4. **Verify**: Dynamic, responsive behavior

### Test 4: Probability Gates
1. Add: Chaos → Memory Drift (Depth: 0.7, Probability: 40%)
2. Listen for 30 seconds
3. **Verify**: Drift only occurs ~40% of the time (intermittent)

---

## Rollback Plan

If MemoryEchoes integration causes issues:
1. **Keep routing flexibility** (MemoryEchoes as module)
2. **Disable modulation targets** (revert to manual control only)
3. **Simplify macro mappings** (Energy → Memory only, remove others)

---

## Summary

**MemoryEchoes is a perfect fit for the experimental redesign.**

- **Already experimental** → Aligns with playful exploration theme
- **Routable module** → Creates dramatic sonic diversity
- **Macro-compatible** → Energy/Motion naturally control memory behavior
- **Modulation-friendly** → Audio follower, chaos, probability gates all enhance it
- **5 new presets** → Ghostly Cathedral, Fragmented Reality, Recursive Haunt, etc.

**Recommendation**: Integrate MemoryEchoes in Phase 2 (alongside ExpressiveMacros) for maximum impact.
