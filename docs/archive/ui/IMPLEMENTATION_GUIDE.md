# Monument Experimental Redesign: Implementation Guide

**Date**: 2026-01-04
**Status**: Design Complete, Implementation Pending
**Estimated Effort**: 4-6 weeks (phased approach)

---

## Overview

This guide provides step-by-step instructions for implementing the experimental redesign that transforms Monument into a dramatically more diverse, expressive, and playful reverb instrument.

**Created Files**:
- [`docs/architecture/EXPERIMENTAL_REDESIGN.md`](EXPERIMENTAL_REDESIGN.md) - Full design rationale
- [`dsp/DspRoutingGraph.h`](../../dsp/DspRoutingGraph.h) - Flexible routing system
- [`dsp/ExpressiveMacroMapper.h`](../../dsp/ExpressiveMacroMapper.h) - New macro controls
- [`dsp/ExperimentalModulation.h`](../../dsp/ExperimentalModulation.h) - Probability, quantization, morphing

---

## Phase 1: DSP Routing System (Week 1-2)

### Goal
Replace fixed serial chain with flexible routing graph that enables parallel processing, feedback loops, and module bypass.

### Step 1.1: Implement DspRoutingGraph.cpp

Create `dsp/DspRoutingGraph.cpp` with these key methods:

```cpp
void DspRoutingGraph::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    sampleRateHz = sampleRate;
    maxBlockSizeInternal = maxBlockSize;
    numChannelsInternal = numChannels;

    // Allocate all modules
    foundation = std::make_unique<Foundation>();
    pillars = std::make_unique<Pillars>();
    chambers = std::make_unique<Chambers>();
    weathering = std::make_unique<Weathering>();
    tubeRayTracer = std::make_unique<TubeRayTracer>();
    elasticHallway = std::make_unique<ElasticHallway>();
    alienAmplification = std::make_unique<AlienAmplification>();
    buttress = std::make_unique<Buttress>();
    facade = std::make_unique<Facade>();

    // Prepare all modules
    foundation->prepare(sampleRate, maxBlockSize);
    pillars->prepare(sampleRate, maxBlockSize);
    // ... prepare others

    // Allocate temp buffers for parallel processing
    for (auto& buffer : tempBuffers)
        buffer.setSize(numChannels, maxBlockSize);

    feedbackBuffer.setSize(numChannels, maxBlockSize);
    feedbackBuffer.clear();

    // Load default routing preset
    loadRoutingPreset(RoutingPresetType::TraditionalCathedral);
}

void DspRoutingGraph::process(juce::AudioBuffer<float>& buffer)
{
    // Save dry signal for parallel modes
    tempBuffers[0].makeCopyOf(buffer);

    // Process each connection in order
    for (const auto& conn : routingConnections)
    {
        if (!conn.enabled || moduleBypassed[static_cast<size_t>(conn.source)])
            continue;

        switch (conn.mode)
        {
            case RoutingMode::Series:
                processModule(conn.destination, buffer);
                break;

            case RoutingMode::Parallel:
            {
                auto& parallelBuf = tempBuffers[static_cast<size_t>(conn.destination)];
                parallelBuf.makeCopyOf(tempBuffers[0]);  // Start with dry
                processModule(conn.destination, parallelBuf);
                blendBuffers(buffer, parallelBuf, conn.blendAmount);
                break;
            }

            case RoutingMode::Feedback:
            {
                // Mix feedback buffer into input
                for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                {
                    buffer.addFrom(ch, 0, feedbackBuffer, ch, 0,
                                   buffer.getNumSamples(), conn.feedbackGain);
                }

                processModule(conn.destination, buffer);

                // Save output for next block's feedback
                feedbackBuffer.makeCopyOf(buffer);
                break;
            }

            case RoutingMode::Crossfeed:
            {
                if (buffer.getNumChannels() >= 2)
                {
                    auto* L = buffer.getWritePointer(0);
                    auto* R = buffer.getWritePointer(1);
                    for (int i = 0; i < buffer.getNumSamples(); ++i)
                    {
                        float crossfeed = (L[i] + R[i]) * 0.5f * conn.crossfeedAmount;
                        L[i] = L[i] * (1.0f - conn.crossfeedAmount) + crossfeed;
                        R[i] = R[i] * (1.0f - conn.crossfeedAmount) + crossfeed;
                    }
                }
                break;
            }

            case RoutingMode::Bypass:
                // Skip this module
                break;
        }
    }
}

void DspRoutingGraph::loadRoutingPreset(RoutingPresetType preset)
{
    currentPreset = preset;
    routingConnections.clear();

    switch (preset)
    {
        case RoutingPresetType::TraditionalCathedral:
            // Foundation → Pillars → Chambers → Weathering → Facade
            routingConnections.push_back({ModuleType::Foundation, ModuleType::Pillars});
            routingConnections.push_back({ModuleType::Pillars, ModuleType::Chambers});
            routingConnections.push_back({ModuleType::Chambers, ModuleType::Weathering});
            routingConnections.push_back({ModuleType::Weathering, ModuleType::Facade});
            break;

        case RoutingPresetType::MetallicGranular:
            // Foundation → Pillars → TubeRayTracer → Facade (bypass Chambers)
            routingConnections.push_back({ModuleType::Foundation, ModuleType::Pillars});
            routingConnections.push_back({ModuleType::Pillars, ModuleType::TubeRayTracer});
            routingConnections.push_back({ModuleType::TubeRayTracer, ModuleType::Facade});
            setModuleBypass(ModuleType::Chambers, true);
            break;

        case RoutingPresetType::ElasticFeedbackDream:
        {
            // Foundation → ElasticHallway → Chambers → AlienAmplification → Facade
            // ElasticHallway ⟲ Feedback to Pillars
            routingConnections.push_back({ModuleType::Foundation, ModuleType::Pillars});
            routingConnections.push_back({ModuleType::Pillars, ModuleType::ElasticHallway});
            routingConnections.push_back({ModuleType::ElasticHallway, ModuleType::Chambers});
            routingConnections.push_back({ModuleType::Chambers, ModuleType::AlienAmplification});
            routingConnections.push_back({ModuleType::AlienAmplification, ModuleType::Facade});

            // Feedback connection
            RoutingConnection feedback{ModuleType::ElasticHallway, ModuleType::Pillars,
                                        RoutingMode::Feedback};
            feedback.feedbackGain = 0.3f;
            routingConnections.push_back(feedback);
            break;
        }

        case RoutingPresetType::ParallelWorlds:
        {
            // Foundation → [Chambers + TubeRayTracer + ElasticHallway] parallel → Facade
            routingConnections.push_back({ModuleType::Foundation, ModuleType::Pillars});

            RoutingConnection parallel1{ModuleType::Pillars, ModuleType::Chambers,
                                         RoutingMode::Parallel};
            parallel1.blendAmount = 0.33f;
            routingConnections.push_back(parallel1);

            RoutingConnection parallel2{ModuleType::Pillars, ModuleType::TubeRayTracer,
                                         RoutingMode::Parallel};
            parallel2.blendAmount = 0.33f;
            routingConnections.push_back(parallel2);

            RoutingConnection parallel3{ModuleType::Pillars, ModuleType::ElasticHallway,
                                         RoutingMode::Parallel};
            parallel3.blendAmount = 0.34f;
            routingConnections.push_back(parallel3);

            routingConnections.push_back({ModuleType::Chambers, ModuleType::Facade});
            break;
        }

        // ... implement other presets
    }
}
```

### Step 1.2: Integrate into PluginProcessor

Update `plugin/PluginProcessor.cpp`:

```cpp
class MonumentAudioProcessor : public juce::AudioProcessor
{
public:
    // Replace individual module instances with routing graph
    monument::dsp::DspRoutingGraph routingGraph;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override
    {
        routingGraph.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    }

    void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override
    {
        // Update routing graph parameters from APVTS
        updateRoutingGraphParams();

        // Process entire graph
        routingGraph.process(buffer);
    }

private:
    void updateRoutingGraphParams()
    {
        routingGraph.setChambersParams(
            paramCache.time,
            paramCache.mass,
            paramCache.density,
            paramCache.bloom,
            paramCache.gravity
        );

        routingGraph.setTubeRayTracerParams(
            paramCache.tubeCount,
            paramCache.radiusVariation,
            paramCache.metallicResonance,
            paramCache.couplingStrength
        );

        // ... set other module params
    }
};
```

### Step 1.3: Test Routing Presets

Create a test preset that uses each routing type:

```cpp
// Test: Traditional Cathedral (default)
processor.routingGraph.loadRoutingPreset(RoutingPresetType::TraditionalCathedral);
// Expected: Familiar Monument reverb sound

// Test: Metallic Granular (bypass Chambers)
processor.routingGraph.loadRoutingPreset(RoutingPresetType::MetallicGranular);
// Expected: Bright, textured, tube resonances (no smooth tail)

// Test: Parallel Worlds (3 modules in parallel)
processor.routingGraph.loadRoutingPreset(RoutingPresetType::ParallelWorlds);
// Expected: Rich, complex blend of 3 different acoustic spaces
```

---

## Phase 2: Expressive Macros (Week 2-3)

### Goal
Replace conceptual macros (Material, Topology) with immediately musical controls (Character, Space Type, Energy, Motion, Color, Dimension).

### Step 2.1: Implement ExpressiveMacroMapper.cpp

Create `dsp/ExpressiveMacroMapper.cpp`:

```cpp
ParameterTargets ExpressiveMacroMapper::computeTargets(const MacroInputs& macros) const
{
    ParameterTargets targets;

    // 1. Character: Global intensity scaling
    float intensityScale = mapCharacterToIntensity(macros.character);

    // 2. Space Type: Select routing preset
    targets.routingPreset = mapSpaceTypeToRouting(macros.spaceType);
    applySpaceTypeModifiers(targets, macros.spaceType);

    // 3. Energy: Decay behavior (exclusive control, no conflicts)
    targets.time = mapEnergyToFeedback(macros.energy);
    targets.bloom = mapEnergyToBloom(macros.energy);
    targets.paradoxGain = mapEnergyToParadoxGain(macros.energy);

    // 4. Motion: Temporal evolution (exclusive control)
    targets.drift = mapMotionToDrift(macros.motion);
    targets.warp = mapMotionToWarp(macros.motion);

    // 5. Color: Spectral character (exclusive control)
    targets.mass = mapColorToMass(macros.color);
    targets.air = mapColorToAir(macros.color);
    targets.gravity = mapColorToGravity(macros.color);
    targets.metallicResonance = mapColorToMetallicResonance(macros.color);

    // 6. Dimension: Space size (exclusive control)
    targets.time = mapDimensionToTime(macros.dimension);  // Note: Energy also sets time, blend them
    targets.density = mapDimensionToDensity(macros.dimension);
    targets.width = mapDimensionToWidth(macros.dimension);
    targets.impossibilityDegree = mapDimensionToImpossibility(macros.dimension);

    // Apply character scaling to all intensity-dependent params
    targets.density = applyCharacterScaling(targets.density, intensityScale);
    targets.metallicResonance = applyCharacterScaling(targets.metallicResonance, intensityScale);
    targets.warp = applyCharacterScaling(targets.warp, intensityScale);

    return targets;
}

float ExpressiveMacroMapper::mapCharacterToIntensity(float character) const noexcept
{
    // 0.0 = subtle (0.3x intensity), 1.0 = extreme (2.0x intensity)
    return juce::jmap(character, 0.0f, 1.0f, 0.3f, 2.0f);
}

RoutingPresetType ExpressiveMacroMapper::mapSpaceTypeToRouting(float spaceType) const noexcept
{
    // Discrete mode selection with hysteresis
    if (spaceType < 0.2f)
        return RoutingPresetType::TraditionalCathedral;  // Chamber mode
    else if (spaceType < 0.4f)
        return RoutingPresetType::TraditionalCathedral;  // Hall mode (same routing, different params)
    else if (spaceType < 0.6f)
        return RoutingPresetType::ShimmerInfinity;       // Shimmer mode
    else if (spaceType < 0.8f)
        return RoutingPresetType::MetallicGranular;      // Granular mode
    else
        return RoutingPresetType::ImpossibleChaos;       // Metallic mode
}

float ExpressiveMacroMapper::mapEnergyToFeedback(float energy) const noexcept
{
    // Decay (0.0-0.2) → Sustain (0.3-0.5) → Grow (0.6-0.8) → Chaos (0.9-1.0)
    std::vector<float> breakpoints = {0.0f, 0.2f, 0.5f, 0.8f, 1.0f};
    std::vector<float> feedbackValues = {0.4f, 0.6f, 0.8f, 0.9f, 0.95f};
    return piecewiseLinear(energy, breakpoints, feedbackValues);
}

float ExpressiveMacroMapper::mapMotionToDrift(float motion) const noexcept
{
    // Still (0.0-0.2) → Drift (0.3-0.5) → Pulse (0.6-0.8) → Random (0.9-1.0)
    std::vector<float> breakpoints = {0.0f, 0.2f, 0.5f, 0.8f, 1.0f};
    std::vector<float> driftValues = {0.0f, 0.3f, 0.6f, 0.8f, 1.0f};
    return piecewiseLinear(motion, breakpoints, driftValues);
}

// ... implement other mapping functions
```

### Step 2.2: Update UI with New Macro Names

Update `plugin/PluginEditor.cpp`:

```cpp
MonumentAudioProcessorEditor::MonumentAudioProcessorEditor(MonumentAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // Replace old macro names with expressive names
    characterKnob.setName("CHARACTER");
    characterKnob.setRange(0.0, 1.0, 0.01);
    characterKnob.setTooltip("Subtle → Extreme");

    spaceTypeKnob.setName("SPACE");
    spaceTypeKnob.setRange(0.0, 1.0, 0.01);
    spaceTypeKnob.setTooltip("Chamber / Hall / Shimmer / Granular / Metallic");

    energyKnob.setName("ENERGY");
    energyKnob.setRange(0.0, 1.0, 0.01);
    energyKnob.setTooltip("Decay / Sustain / Grow / Chaos");

    motionKnob.setName("MOTION");
    motionKnob.setRange(0.0, 1.0, 0.01);
    motionKnob.setTooltip("Still / Drift / Pulse / Random");

    colorKnob.setName("COLOR");
    colorKnob.setRange(0.0, 1.0, 0.01);
    colorKnob.setTooltip("Dark / Balanced / Bright / Spectral");

    dimensionKnob.setName("DIMENSION");
    dimensionKnob.setRange(0.0, 1.0, 0.01);
    dimensionKnob.setTooltip("Intimate / Room / Cathedral / Infinite");
}
```

### Step 2.3: Create Migration Presets

Migrate existing presets to new macro system:

```yaml
# Old Preset: "Cathedral"
material: 0.7
topology: 0.5
viscosity: 0.4
evolution: 0.5
chaos: 0.2
elasticity: 0.3

# New Preset: "Cathedral"
character: 0.5  (Balanced)
spaceType: 0.3  (Hall)
energy: 0.1     (Decay)
motion: 0.2     (Still)
color: 0.5      (Balanced)
dimension: 0.7  (Cathedral)
```

---

## Phase 3: Experimental Modulation (Week 3-4)

### Goal
Add probability gates, quantization, cross-modulation, preset morphing, and gesture recording.

### Step 3.1: Implement ExperimentalModulation.cpp

Create `dsp/ExperimentalModulation.cpp` with implementations for all classes.

### Step 3.2: Extend ModulationMatrix

Update `dsp/ModulationMatrix.h` to use `ExperimentalModConnection`:

```cpp
class ModulationMatrix
{
public:
    void setConnection(SourceType source, DestinationType destination,
                       int sourceAxis, float depth, float smoothingMs,
                       bool enableProbability = false, float probability = 1.0f,
                       bool enableQuantization = false, int steps = 8);

    float getModulation(DestinationType destination) const;

private:
    std::vector<ExperimentalModConnection> connections;
};
```

### Step 3.3: Add Randomization Tools

```cpp
void ModulationMatrix::randomizeAll()
{
    clearConnections();

    auto randomConnections = ChaosSeeder::generateRandomConnections(
        6,  // 6 random connections
        static_cast<int>(SourceType::Count),
        static_cast<int>(DestinationType::Count)
    );

    auto probabilities = ChaosSeeder::generateRandomProbabilities(6);
    auto quantizations = ChaosSeeder::generateRandomQuantization(6);

    for (size_t i = 0; i < randomConnections.size(); ++i)
    {
        auto [src, dst, depth] = randomConnections[i];
        bool enableProb = probabilities[i] < 0.8f;
        bool enableQuant = quantizations[i] > 1;

        setConnection(
            static_cast<SourceType>(src),
            static_cast<DestinationType>(dst),
            0, depth, 200.0f,
            enableProb, probabilities[i],
            enableQuant, quantizations[i]
        );
    }
}
```

---

## Phase 4: UI Simplification (Week 5)

### Goal
Hide primary parameters by default, show only macros + presets.

### Step 4.1: Create ViewMode System

```cpp
enum class ViewMode
{
    Performance,    // 6 macros + presets (default)
    Advanced,       // All 20+ parameters
    Playground      // Experimental modulation tools
};

void MonumentAudioProcessorEditor::setViewMode(ViewMode mode)
{
    currentView = mode;

    // Hide/show components based on mode
    bool showAdvanced = (mode == ViewMode::Advanced);
    bool showPlayground = (mode == ViewMode::Playground);

    mixKnob.setVisible(showAdvanced);
    timeKnob.setVisible(showAdvanced);
    massKnob.setVisible(showAdvanced);
    // ... hide all primary params

    modMatrixPanel->setVisible(showPlayground);
    // ... show playground tools
}
```

### Step 4.2: Design Playground Panel

Create `ui/PlaygroundPanel.h`:

```cpp
class PlaygroundPanel : public juce::Component
{
public:
    PlaygroundPanel(MonumentAudioProcessor& proc);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::TextButton randomizeButton{"🎲 Randomize All"};
    juce::TextButton morphModeButton{"🔄 Morph Mode"};
    juce::TextButton recordGestureButton{"📼 Record Gesture"};
    juce::TextButton chaosSeedButton{"🌀 Chaos Seed"};

    juce::ListBox connectionsList;

    void onRandomizeClicked();
    void onMorphModeClicked();
    void onRecordGestureClicked();
    void onChaosSeedClicked();
};
```

---

## Phase 5: Preset Library (Week 6)

### Goal
Create 20+ presets using diverse routings and experimental modulation.

### Sample Presets

```yaml
# Preset 1: "Cathedral" (Traditional)
character: 0.3
spaceType: 0.3
energy: 0.1
motion: 0.1
color: 0.5
dimension: 0.7
routing: TraditionalCathedral

# Preset 2: "Metallic Dream" (Experimental)
character: 0.8
spaceType: 0.9
energy: 0.7
motion: 0.9
color: 0.8
dimension: 0.95
routing: MetallicGranular
modulation:
  - Chaos.X → Tube Count (Depth: 0.6, Probability: 40%)
  - Audio → Metallic Resonance (Depth: 0.7, Quantized: 5 steps)

# Preset 3: "Elastic Breathing" (Organic)
character: 0.5
spaceType: 0.3
energy: 0.5
motion: 0.3
color: 0.4
dimension: 0.6
routing: ElasticFeedbackDream
modulation:
  - Audio → Wall Elasticity (Depth: 0.8)
  - Brownian → Recovery Time (Depth: 0.5)

# ... create 17 more presets
```

---

## Testing Strategy

### Sonic Diversity Test

**Goal**: Verify that presets sound dramatically different.

1. Load "Cathedral" → Listen → Note character (smooth, traditional)
2. Load "Metallic Dream" → Listen → Should sound COMPLETELY different (bright, textured, ringing)
3. Load "Elastic Breathing" → Listen → Should sound COMPLETELY different (organic, morphing)

**Pass Criteria**: No two presets sound like "slightly different cathedrals"

### Macro Independence Test

**Goal**: Verify macros don't fight each other.

1. Set all macros to 0.5 (neutral)
2. Move Character to 0.0 → Should be subtle, transparent
3. Move Character to 1.0 → Should be extreme, saturated
4. Reset Character to 0.5, move Energy to 0.9 → Should be chaotic, no parameter conflicts
5. Repeat for all macros

**Pass Criteria**: Each macro creates independent, predictable changes

### Experimental Modulation Test

**Goal**: Verify playful, exploration-driven features work.

1. Click "🎲 Randomize All" → Should create instant random modulation
2. Listen → Should sound interesting, not broken
3. Click "🌀 Chaos Seed" → Should create different random modulation
4. Enable Probability Gate (30%) → Modulation should be intermittent
5. Enable Quantization (8 steps) → Modulation should be stepped

**Pass Criteria**: Features inspire exploration, not frustration

---

## Rollback Plan

If redesign doesn't meet goals:

1. **Keep DspRoutingGraph**: Routing flexibility is valuable even if macros revert
2. **Revert Macros**: Restore Material/Topology/Viscosity if expressive macros fail
3. **Optional Experimental**: Make probability/quantization opt-in features

---

## Success Metrics

- **Preset Diversity**: A/B listening tests show presets are clearly distinguishable
- **User Engagement**: Users explore presets more, tweak less
- **Sound Design Speed**: Creating new sounds is faster with 6 macros vs 20 params
- **CPU Efficiency**: Bypassed modules save 10-30% CPU

---

## Next Steps

1. **Review Design**: Get feedback on macro names, routing presets
2. **Prototype Phase 1**: Implement DspRoutingGraph, test 3 routing presets
3. **Prototype Phase 2**: Implement ExpressiveMacroMapper, migrate 5 presets
4. **User Testing**: A/B test old vs new macro systems
5. **Iterate**: Refine based on feedback
6. **Full Implementation**: Complete all 5 phases
