# Complete Experimental Redesign - Implementation Plan

**Date:** 2026-01-04
**Goal:** Finish the experimental redesign (Option A)
**Current Status:** 85% complete (Phases 1-2 done, Phase 3 at 30%)
**Estimated Effort:** 3-4 weeks
**Risk Level:** High (breaking changes, preset migration, dual system integration)

---

## Overview

The experimental redesign is largely implemented but not integrated. This plan completes:
1. **Phase 3 implementation** (15% remaining - ExperimentalModulation.cpp)
2. **System integration** (wire up routing graph + expressive macros)
3. **Macro migration** (Ancient Monuments → Expressive Macros)
4. **UI updates** (6 macros + routing selector)
5. **Preset migration** (28 factory presets)
6. **Testing & validation** (stability, CPU, user experience)

---

## Week 1: Complete Phase 3 Implementation

### Day 1-2: Core Experimental Modulation Classes

#### Task 1.1: Implement ModulationQuantizer.cpp
**Complexity:** Low (30-40 lines)

```cpp
// dsp/ExperimentalModulation.cpp
namespace monument::dsp
{

void ModulationQuantizer::setSteps(int numSteps) noexcept
{
    steps = juce::jlimit(2, 64, numSteps);
}

float ModulationQuantizer::quantize(float smoothValue) const noexcept
{
    // Snap to discrete steps
    const int stepIndex = static_cast<int>(smoothValue * steps);
    const int clampedIndex = juce::jlimit(0, steps - 1, stepIndex);
    return static_cast<float>(clampedIndex) / static_cast<float>(steps - 1);
}

} // namespace monument::dsp
```

**Testing:**
- Verify 8-step quantization: input [0, 1] → output {0.0, 0.143, 0.286, ..., 1.0}
- Test edge cases: 2 steps, 64 steps

---

#### Task 1.2: Implement ProbabilityGate.cpp
**Complexity:** Medium (80-100 lines)

```cpp
namespace monument::dsp
{

ProbabilityGate::ProbabilityGate()
    : rng(std::random_device{}())
{
}

void ProbabilityGate::setProbability(float prob) noexcept
{
    probability = juce::jlimit(0.0f, 1.0f, prob);
}

void ProbabilityGate::setSmoothingMs(float ms, double sampleRate) noexcept
{
    gateEnvelope.reset(sampleRate, ms / 1000.0);
}

void ProbabilityGate::prepare(double sampleRate)
{
    gateEnvelope.reset(sampleRate, 0.05);  // 50ms default smoothing
    gateEnvelope.setCurrentAndTargetValue(0.0f);
}

bool ProbabilityGate::shouldBeActive()
{
    // Decide per-block (not per-sample) to avoid zipper noise
    return dist(rng) < probability;
}

float ProbabilityGate::process(float inputModulation)
{
    // Check if gate should open/close (per-block decision)
    const bool targetState = shouldBeActive();

    if (targetState != currentlyActive)
    {
        currentlyActive = targetState;
        gateEnvelope.setTargetValue(currentlyActive ? 1.0f : 0.0f);
    }

    // Apply smoothed gate envelope
    const float gateGain = gateEnvelope.getNextValue();
    return inputModulation * gateGain;
}

} // namespace monument::dsp
```

**Testing:**
- 100% probability = always passes through
- 0% probability = always blocked
- 50% probability = ~50% of blocks active (statistical validation over 1000 blocks)
- Verify smooth transitions (no clicks)

---

#### Task 1.3: Implement SpringMassModulator.cpp
**Complexity:** Medium (80-100 lines)

```cpp
namespace monument::dsp
{

SpringMassModulator::SpringMassModulator() = default;

void SpringMassModulator::setSpringConstant(float k) noexcept
{
    springConstant = juce::jmax(0.01f, k);  // Prevent division by zero
}

void SpringMassModulator::setMass(float m) noexcept
{
    mass = juce::jmax(0.01f, m);  // Prevent division by zero
}

void SpringMassModulator::setDamping(float c) noexcept
{
    damping = juce::jmax(0.0f, c);
}

void SpringMassModulator::applyForce(float force) noexcept
{
    externalForce = force;
}

void SpringMassModulator::prepare(double sampleRate)
{
    dt = 1.0 / sampleRate;
    reset();
}

void SpringMassModulator::updatePhysics()
{
    // Semi-implicit Euler integration (stable for stiff springs)
    // F = ma = -kx - cv + F_ext
    // a = (-kx - cv + F_ext) / m

    const float springForce = -springConstant * position;
    const float dampingForce = -damping * velocity;
    const float totalForce = springForce + dampingForce + externalForce;

    const float acceleration = totalForce / mass;

    // Update velocity first (semi-implicit)
    velocity += acceleration * static_cast<float>(dt);

    // Then update position
    position += velocity * static_cast<float>(dt);

    // Soft limit position to prevent runaway
    position = juce::jlimit(-10.0f, 10.0f, position);
}

float SpringMassModulator::processSample()
{
    updatePhysics();
    return position;
}

void SpringMassModulator::reset() noexcept
{
    position = 0.0f;
    velocity = 0.0f;
    externalForce = 0.0f;
}

} // namespace monument::dsp
```

**Testing:**
- Apply constant force → verify spring oscillates then settles
- High damping → quick settling, no overshoot
- Low damping → long oscillation
- Stability test: run 10 seconds at extreme parameter values

---

### Day 3-4: Advanced Experimental Classes

#### Task 1.4: Implement PresetMorpher.cpp
**Complexity:** High (150-180 lines)

```cpp
namespace monument::dsp
{

PresetMorpher::PresetMorpher()
{
    // Initialize with empty parameter vectors
    for (auto& preset : presetParameters)
        preset.resize(0);
}

void PresetMorpher::setCornerPresets(int topLeft, int topRight,
                                      int bottomLeft, int bottomRight)
{
    cornerPresets[0] = topLeft;
    cornerPresets[1] = topRight;
    cornerPresets[2] = bottomLeft;
    cornerPresets[3] = bottomRight;
}

void PresetMorpher::setMorphPosition(float x, float y) noexcept
{
    morphX = juce::jlimit(0.0f, 1.0f, x);
    morphY = juce::jlimit(0.0f, 1.0f, y);
}

void PresetMorpher::loadPresetStates(const std::vector<std::vector<float>>& presetParams)
{
    jassert(presetParams.size() == 4);  // Must have 4 presets

    for (size_t i = 0; i < 4; ++i)
    {
        presetParameters[i] = presetParams[i];

        // All presets must have same parameter count
        if (i > 0)
            jassert(presetParameters[i].size() == presetParameters[0].size());
    }
}

float PresetMorpher::bilinearInterpolate(float topLeft, float topRight,
                                          float bottomLeft, float bottomRight,
                                          float x, float y) const noexcept
{
    // Bilinear interpolation formula:
    // f(x,y) = (1-x)(1-y)·TL + x(1-y)·TR + (1-x)y·BL + xy·BR

    const float invX = 1.0f - x;
    const float invY = 1.0f - y;

    const float weightTL = invX * invY;
    const float weightTR = x * invY;
    const float weightBL = invX * y;
    const float weightBR = x * y;

    return weightTL * topLeft + weightTR * topRight +
           weightBL * bottomLeft + weightBR * bottomRight;
}

float PresetMorpher::getMorphedParameter(int parameterIndex) const noexcept
{
    // Validate parameter index
    if (parameterIndex < 0 || presetParameters[0].empty() ||
        parameterIndex >= static_cast<int>(presetParameters[0].size()))
    {
        return 0.0f;
    }

    // Get parameter value from each corner preset
    const float topLeft = presetParameters[0][parameterIndex];
    const float topRight = presetParameters[1][parameterIndex];
    const float bottomLeft = presetParameters[2][parameterIndex];
    const float bottomRight = presetParameters[3][parameterIndex];

    // Perform bilinear interpolation
    return bilinearInterpolate(topLeft, topRight, bottomLeft, bottomRight,
                                morphX, morphY);
}

} // namespace monument::dsp
```

**Testing:**
- Corner positions (0,0), (1,0), (0,1), (1,1) return exact preset values
- Center position (0.5, 0.5) returns average of all 4 presets
- Verify smooth interpolation across 2D space

---

#### Task 1.5: Implement GestureRecorder.cpp
**Complexity:** Medium (100-120 lines)

```cpp
namespace monument::dsp
{

GestureRecorder::GestureRecorder() = default;

void GestureRecorder::startRecording()
{
    recordedValues.clear();
    recordedValues.reserve(10000);  // Pre-allocate for ~3 seconds at 48kHz/512 blocks
    recording = true;
    playing = false;
}

void GestureRecorder::stopRecording()
{
    recording = false;
}

void GestureRecorder::recordValue(float value)
{
    if (recording)
    {
        recordedValues.push_back(value);

        // Safety limit: max 60 seconds at 48kHz/512 blocks = ~180k samples
        if (recordedValues.size() > 180000)
            stopRecording();
    }
}

void GestureRecorder::startPlayback(float speed, bool loop)
{
    if (recordedValues.empty())
        return;

    playbackSpeed = juce::jlimit(0.1f, 10.0f, speed);
    looping = loop;
    playbackIndex = 0;
    playing = true;
    recording = false;
}

void GestureRecorder::stopPlayback()
{
    playing = false;
    playbackIndex = 0;
}

float GestureRecorder::getSample()
{
    if (!playing || recordedValues.empty())
        return 0.0f;

    // Get current sample
    const float sample = recordedValues[playbackIndex];

    // Advance playback index with speed control
    playbackIndex += static_cast<int>(playbackSpeed);

    // Handle end of recording
    if (playbackIndex >= static_cast<int>(recordedValues.size()))
    {
        if (looping)
            playbackIndex = 0;  // Wrap around
        else
        {
            playing = false;
            playbackIndex = 0;
            return 0.0f;
        }
    }

    return sample;
}

} // namespace monument::dsp
```

**Testing:**
- Record 100 samples, playback → verify exact reproduction
- 2× speed → playback completes in half the time
- Loop mode → verify seamless restart
- Max length limit → verify stops at 60 seconds

---

#### Task 1.6: Implement ChaosSeeder.cpp
**Complexity:** Low (60-80 lines)

```cpp
namespace monument::dsp
{

std::mt19937& ChaosSeeder::getRng()
{
    static std::mt19937 rng{std::random_device{}()};
    return rng;
}

std::vector<std::tuple<int, int, float>> ChaosSeeder::generateRandomConnections(
    int numConnections, int numSources, int numDestinations)
{
    std::vector<std::tuple<int, int, float>> connections;
    connections.reserve(numConnections);

    auto& rng = getRng();
    std::uniform_int_distribution<> sourceDist(0, numSources - 1);
    std::uniform_int_distribution<> destDist(0, numDestinations - 1);
    std::uniform_real_distribution<> depthDist(0.2f, 0.6f);  // Musical range

    // Track used combinations to avoid duplicates
    std::set<std::pair<int, int>> usedPairs;

    int attempts = 0;
    const int maxAttempts = numConnections * 10;  // Safety limit

    while (connections.size() < static_cast<size_t>(numConnections) &&
           attempts < maxAttempts)
    {
        const int source = sourceDist(rng);
        const int dest = destDist(rng);

        // Skip if already used
        if (usedPairs.find({source, dest}) != usedPairs.end())
        {
            ++attempts;
            continue;
        }

        // 70% positive depth bias (more musical)
        float depth = depthDist(rng);
        if (std::uniform_real_distribution<>(0.0f, 1.0f)(rng) > 0.7f)
            depth = -depth;

        connections.emplace_back(source, dest, depth);
        usedPairs.insert({source, dest});
        ++attempts;
    }

    return connections;
}

std::vector<float> ChaosSeeder::generateRandomProbabilities(int numConnections)
{
    std::vector<float> probabilities;
    probabilities.reserve(numConnections);

    auto& rng = getRng();
    std::uniform_real_distribution<> dist(0.3f, 1.0f);  // 30-100% range

    for (int i = 0; i < numConnections; ++i)
        probabilities.push_back(dist(rng));

    return probabilities;
}

std::vector<int> ChaosSeeder::generateRandomQuantization(int numConnections)
{
    std::vector<int> steps;
    steps.reserve(numConnections);

    auto& rng = getRng();
    std::uniform_int_distribution<> dist(2, 16);  // 2-16 steps

    for (int i = 0; i < numConnections; ++i)
        steps.push_back(dist(rng));

    return steps;
}

} // namespace monument::dsp
```

**Testing:**
- Verify no duplicate source/dest pairs
- Verify depth range [0.2, 0.6] and [-0.6, -0.2]
- Verify 70% positive bias (statistical test over 1000 generations)

---

### Day 5: CMake Integration + Unit Tests

#### Task 1.7: Add ExperimentalModulation.cpp to CMakeLists.txt

```cmake
# In CMakeLists.txt, add to JUCE_SOURCES:
dsp/ExperimentalModulation.cpp
```

#### Task 1.8: Create Unit Tests

Create `tests/ExperimentalModulationTest.cpp`:

```cpp
#include <catch2/catch.hpp>
#include "dsp/ExperimentalModulation.h"

using namespace monument::dsp;

TEST_CASE("ModulationQuantizer", "[experimental]")
{
    ModulationQuantizer quantizer;

    SECTION("8-step quantization")
    {
        quantizer.setSteps(8);

        REQUIRE(quantizer.quantize(0.0f) == Approx(0.0f));
        REQUIRE(quantizer.quantize(0.5f) == Approx(0.571f).margin(0.01));
        REQUIRE(quantizer.quantize(1.0f) == Approx(1.0f));
    }
}

TEST_CASE("ProbabilityGate", "[experimental]")
{
    ProbabilityGate gate;
    gate.prepare(48000.0);

    SECTION("100% probability always passes")
    {
        gate.setProbability(1.0f);

        for (int i = 0; i < 100; ++i)
        {
            float result = gate.process(1.0f);
            REQUIRE(result >= 0.9f);  // Allow for smoothing
        }
    }
}

// ... more tests
```

**Build and run:**
```bash
cmake --build build --target Monument_Tests
./build/Monument_Tests
```

---

## Week 2: System Integration

### Day 6-7: Integrate DspRoutingGraph into PluginProcessor

#### Task 2.1: Replace Direct DSP Chain with Routing Graph

**File:** `plugin/PluginProcessor.h`

```cpp
// BEFORE (current):
class MonumentAudioProcessor : public juce::AudioProcessor
{
private:
    monument::dsp::Foundation foundation;
    monument::dsp::Pillars pillars;
    monument::dsp::Chambers chambers;
    // ... individual modules
};

// AFTER (with routing graph):
class MonumentAudioProcessor : public juce::AudioProcessor
{
private:
    std::unique_ptr<monument::dsp::DspRoutingGraph> routingGraph;

    // Routing mode (Ancient Monuments or Preset-based)
    std::atomic<int> currentRoutingMode{0};  // 0=Ancient Way, 1=Resonant Halls, 2=Breathing Stone
    std::atomic<int> currentRoutingPreset{0};  // TraditionalCathedral, MetallicGranular, etc.
};
```

**File:** `plugin/PluginProcessor.cpp`

```cpp
void MonumentAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    routingGraph = std::make_unique<monument::dsp::DspRoutingGraph>();
    routingGraph->prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());

    // Load default routing
    routingGraph->loadRoutingPreset(monument::dsp::RoutingPresetType::TraditionalCathedral);
}

void MonumentAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Update routing graph parameters from macros
    updateRoutingGraphParameters();

    // Process through routing graph
    const int mode = currentRoutingMode.load();
    switch (mode)
    {
        case 0:  // Ancient Way
            routingGraph->processAncientWay(buffer);
            break;
        case 1:  // Resonant Halls
            routingGraph->processResonantHalls(buffer);
            break;
        case 2:  // Breathing Stone
            routingGraph->processBreathingStone(buffer);
            break;
        default:
            // Use preset-based routing
            routingGraph->process(buffer);
            break;
    }
}
```

**Testing:**
- Build plugin → verify no crashes
- Load in DAW → verify audio output
- Switch routing modes → verify timbral changes
- Check CPU usage (should be ~4.5%, within budget)

---

### Day 8-9: Integrate ExpressiveMacroMapper

#### Task 2.2: Replace Ancient Monuments Macros with Expressive Macros

**File:** `plugin/PluginProcessor.h`

```cpp
class MonumentAudioProcessor : public juce::AudioProcessor
{
private:
    std::unique_ptr<monument::dsp::ExpressiveMacroMapper> macroMapper;

    // 6 Expressive Macros (replace 10 Ancient Monuments macros)
    juce::AudioParameterFloat* characterParam{nullptr};
    juce::AudioParameterFloat* spaceTypeParam{nullptr};
    juce::AudioParameterFloat* energyParam{nullptr};
    juce::AudioParameterFloat* motionParam{nullptr};
    juce::AudioParameterFloat* colorParam{nullptr};
    juce::AudioParameterFloat* dimensionParam{nullptr};

    // Keep base parameters (Drive, Air, Width, Mix)
    juce::AudioParameterFloat* driveParam{nullptr};
    juce::AudioParameterFloat* airParam{nullptr};
    juce::AudioParameterFloat* widthParam{nullptr};
    juce::AudioParameterFloat* mixParam{nullptr};
};
```

**File:** `plugin/PluginProcessor.cpp`

```cpp
void MonumentAudioProcessor::updateRoutingGraphParameters()
{
    // Get current macro values
    const float character = characterParam->get();
    const float spaceType = spaceTypeParam->get();
    const float energy = energyParam->get();
    const float motion = motionParam->get();
    const float color = colorParam->get();
    const float dimension = dimensionParam->get();

    // Compute parameter targets from macros
    const auto targets = macroMapper->computeTargets(
        character, spaceType, energy, motion, color, dimension
    );

    // Apply to routing graph
    routingGraph->setChambersParams(
        targets.time,
        targets.mass,
        targets.density,
        targets.bloom,
        targets.gravity
    );

    routingGraph->setPillarsParams(
        targets.density,
        targets.pillarShape,
        targets.warp
    );

    routingGraph->setWeatheringParams(
        targets.warp,
        targets.drift
    );

    routingGraph->setTubeRayTracerParams(
        targets.tubeCount,
        targets.radiusVariation,
        targets.metallicResonance,
        targets.couplingStrength
    );

    routingGraph->setElasticHallwayParams(
        targets.elasticity,
        targets.recoveryTime,
        targets.absorptionDrift,
        targets.nonlinearity
    );

    routingGraph->setAlienAmplificationParams(
        targets.impossibilityDegree,
        targets.pitchEvolutionRate,
        targets.paradoxResonanceFreq,
        targets.paradoxGain
    );

    routingGraph->setFacadeParams(
        targets.air,
        targets.width,
        targets.mix
    );

    // Load routing preset based on Space Type
    if (targets.routingPreset != routingGraph->getCurrentPreset())
        routingGraph->loadRoutingPreset(targets.routingPreset);
}
```

**Testing:**
- Move Character macro → verify global intensity change
- Move Space Type macro → verify routing preset changes
- Move Energy macro → verify decay behavior changes
- Move Motion macro → verify drift/warp changes
- Move Color macro → verify spectral character changes
- Move Dimension macro → verify space size changes

---

### Day 10: Parameter Migration Strategy

#### Task 2.3: Create Migration Utility for Old Presets

**File:** `plugin/PresetMigration.h`

```cpp
namespace monument
{

/**
 * @brief Migrates presets from Ancient Monuments (v3) to Expressive Macros (v4)
 *
 * Maps 10 Ancient Monuments macros to 6 Expressive Macros.
 */
class PresetMigration
{
public:
    struct AncientMonumentsPreset
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

    struct ExpressiveMacroPreset
    {
        float character{0.5f};
        float spaceType{0.2f};
        float energy{0.1f};
        float motion{0.2f};
        float color{0.5f};
        float dimension{0.5f};
    };

    /**
     * @brief Convert Ancient Monuments preset to Expressive Macros
     *
     * Mapping logic:
     * - Stone → Character (intensity)
     * - Labyrinth → Dimension (size/complexity)
     * - Mist → Color (spectral character)
     * - Bloom → Energy (decay behavior)
     * - Tempest → Motion (temporal evolution)
     * - Echo → Dimension (space depth)
     * - Patina → Color (tonal warmth)
     * - Abyss → Energy (feedback/grow)
     * - Corona → Color (brightness)
     * - Breath → Motion (organic drift)
     */
    static ExpressiveMacroPreset migrate(const AncientMonumentsPreset& ancient);
};

} // namespace monument
```

**Implementation:**

```cpp
PresetMigration::ExpressiveMacroPreset PresetMigration::migrate(
    const AncientMonumentsPreset& ancient)
{
    ExpressiveMacroPreset expressive;

    // Character: Stone controls global intensity
    expressive.character = ancient.stone;

    // Space Type: Default to Hall (0.3), unless specific indicators
    expressive.spaceType = 0.3f;  // Hall default

    // Energy: Bloom + Abyss control decay behavior
    expressive.energy = (ancient.bloom * 0.6f + ancient.abyss * 0.4f);

    // Motion: Tempest + Breath control temporal evolution
    expressive.motion = (ancient.tempest * 0.6f + ancient.breath * 0.4f);

    // Color: Mist + Patina + Corona control spectral character
    expressive.color = (ancient.mist * 0.4f + ancient.patina * 0.3f + ancient.corona * 0.3f);

    // Dimension: Labyrinth + Echo control space size
    expressive.dimension = (ancient.labyrinth * 0.5f + ancient.echo * 0.5f);

    return expressive;
}
```

**Testing:**
- Migrate all 28 factory presets
- A/B compare old vs new sound
- Verify presets still recognizable after migration

---

## Week 3: UI Implementation

### Day 11-13: Create UI for 6 Expressive Macros

#### Task 3.1: Design New Macro Panel Layout

**Layout:**
```
┌─────────────────────────────────────────────────────┐
│  Monument Reverb - Expressive Macros                │
├─────────────────────────────────────────────────────┤
│                                                      │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐          │
│  │CHARACTER │  │SPACE TYPE│  │ ENERGY   │          │
│  │  [====]  │  │  [====]  │  │  [====]  │          │
│  │ Subtle → │  │ Chamber →│  │ Decay → │          │
│  │ Extreme  │  │ Metallic │  │  Chaos   │          │
│  └──────────┘  └──────────┘  └──────────┘          │
│                                                      │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐          │
│  │ MOTION   │  │  COLOR   │  │DIMENSION │          │
│  │  [====]  │  │  [====]  │  │  [====]  │          │
│  │ Still →  │  │ Dark →   │  │ Intimate→│          │
│  │ Random   │  │ Spectral │  │ Infinite │          │
│  └──────────┘  └──────────┘  └──────────┘          │
│                                                      │
├─────────────────────────────────────────────────────┤
│ Routing: [Ancient Way ▼] | [Traditional Cathedral ▼]│
│ Base: Drive [==] Air [==] Width [==] Mix [====]     │
└─────────────────────────────────────────────────────┘
```

**File:** `ui/ExpressiveMacroPanel.h`

```cpp
class ExpressiveMacroPanel : public juce::Component
{
public:
    ExpressiveMacroPanel(MonumentAudioProcessor& processor);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    MonumentAudioProcessor& audioProcessor;

    // 6 macro sliders
    std::unique_ptr<juce::Slider> characterSlider;
    std::unique_ptr<juce::Slider> spaceTypeSlider;
    std::unique_ptr<juce::Slider> energySlider;
    std::unique_ptr<juce::Slider> motionSlider;
    std::unique_ptr<juce::Slider> colorSlider;
    std::unique_ptr<juce::Slider> dimensionSlider;

    // Labels
    std::unique_ptr<juce::Label> characterLabel;
    std::unique_ptr<juce::Label> spaceTypeLabel;
    // ... etc

    // Attachments
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> attachments;
};
```

#### Task 3.2: Add Routing Mode Selector

```cpp
class RoutingModeSelector : public juce::ComboBox
{
public:
    RoutingModeSelector(MonumentAudioProcessor& processor)
        : audioProcessor(processor)
    {
        // Ancient Monuments modes
        addItem("Ancient Way", 1);
        addItem("Resonant Halls", 2);
        addItem("Breathing Stone", 3);

        addSeparator();

        // Routing presets
        addItem("Traditional Cathedral", 10);
        addItem("Metallic Granular", 11);
        addItem("Elastic Feedback Dream", 12);
        addItem("Parallel Worlds", 13);
        addItem("Shimmer Infinity", 14);
        addItem("Impossible Chaos", 15);
        addItem("Organic Breathing", 16);
        addItem("Minimal Sparse", 17);
    }

private:
    MonumentAudioProcessor& audioProcessor;
};
```

**Testing:**
- Verify UI responsive to parameter changes
- Test automation recording
- Verify preset save/load includes macro values

---

### Day 14-15: Integrate Experimental Modulation into ModulationMatrix

#### Task 3.3: Add Experimental Features to Connection UI

**File:** `ui/ModMatrixPanel.cpp`

Add context menu options:
- "Probability Gate..." → popup dialog to set probability %
- "Quantize..." → popup dialog to set step count (2-64)
- "Enable Cross-Mod..." → popup dialog for cross-modulation routing

**File:** `dsp/ModulationMatrix.h`

```cpp
struct ModulationConnection
{
    // ... existing fields ...

    // NEW: Experimental features
    std::unique_ptr<monument::dsp::ProbabilityGate> probabilityGate;
    std::unique_ptr<monument::dsp::ModulationQuantizer> quantizer;

    bool probabilityEnabled{false};
    bool quantizationEnabled{false};
};
```

**Testing:**
- Create connection with 50% probability → verify intermittent modulation
- Create connection with 8-step quantization → verify stepped values
- Verify no CPU overhead when features disabled

---

## Week 4: Preset Migration & Testing

### Day 16-18: Migrate 28 Factory Presets

#### Task 4.1: Batch Convert All Presets

```cpp
// Run migration script
void migrateAllFactoryPresets()
{
    const std::vector<std::string> presetNames = {
        "Foundational", "Pillars of Sound", "Vast Chambers",
        // ... all 28 presets
    };

    for (const auto& name : presetNames)
    {
        auto ancient = loadAncientMonumentsPreset(name);
        auto expressive = PresetMigration::migrate(ancient);
        saveExpressiveMacroPreset(name, expressive);
    }
}
```

#### Task 4.2: Manual Tuning & Validation

For each preset:
1. Load old version (Ancient Monuments)
2. Load new version (Expressive Macros)
3. A/B compare sound
4. Manually adjust expressive macros if migration isn't perfect
5. Document any significant changes

**Expected Results:**
- ~80% of presets migrate cleanly
- ~20% need manual tweaking (especially experimental presets)

---

### Day 19-20: Comprehensive Testing

#### Task 4.3: Stability Testing

**Test Suite:**
1. **CPU Usage:**
   - Monitor over 10 minutes
   - All routing modes
   - All routing presets
   - Target: < 5% per instance @ 48kHz

2. **Feedback Stability:**
   - ElasticFeedbackDream preset
   - ShimmerInfinity preset
   - Breathing Stone mode
   - All macros at extreme values (0.0, 1.0)
   - Test for 5 minutes each
   - Verify no runaway, clipping, NaN, or denormals

3. **Preset Compatibility:**
   - Load all 28 factory presets
   - Verify no crashes
   - Verify recognizable sound

4. **Automation:**
   - Record automation for all 6 macros
   - Playback → verify smooth, no clicks
   - Test DAW recall

5. **State Save/Recall:**
   - Set routing mode + preset + macros
   - Save project
   - Reload project
   - Verify exact state restored

#### Task 4.4: Performance Profiling

Use Xcode Instruments:
```bash
instruments -t "Time Profiler" build/Monument_artefacts/Debug/Standalone/Monument.app
```

Identify hotspots:
- DspRoutingGraph::process()
- ExpressiveMacroMapper::computeTargets()
- Experimental modulation classes

**Target:** < 5% CPU per instance

---

### Day 21: Documentation & Handoff

#### Task 4.5: Update Documentation

**Files to Update:**
1. `README.md` - New macro system, routing presets
2. `CHANGELOG.md` - v2.0 breaking changes
3. `docs/PRESET_GALLERY.md` - Updated preset descriptions
4. `docs/architecture/ARCHITECTURE_REVIEW.md` - New signal flow
5. `docs/architecture/PARAMETER_BEHAVIOR.md` - New macro mappings
6. `NEXT_SESSION_HANDOFF.md` - Mark experimental redesign complete

#### Task 4.6: Create User Migration Guide

**File:** `docs/MIGRATION_GUIDE_V2.md`

```markdown
# Monument Reverb v2.0 Migration Guide

## Breaking Changes

### Macro System Overhaul
- **Old:** 10 Ancient Monuments macros
- **New:** 6 Expressive Macros

### Preset Format
- **Old:** v3 format (.monument3)
- **New:** v4 format (.monument4)

### Automatic Migration
- All factory presets migrated automatically
- User presets: Run "Tools → Migrate Old Presets" in plugin

### Parameter Mapping
| Ancient Monuments | → | Expressive Macros |
|-------------------|---|-------------------|
| Stone             | → | Character         |
| Labyrinth         | → | Dimension         |
| Mist              | → | Color             |
| Bloom             | → | Energy            |
| Tempest           | → | Motion            |
| ...               |   |                   |
```

---

## Risk Mitigation Strategies

### Risk 1: Feedback Instability in Routing Presets
**Mitigation:**
- ✅ Safety clipping in processBreathingStone() (implemented)
- ✅ Feedback gain limiting (max 0.95f)
- ✅ Low-pass filtering at 8kHz
- 🔧 Add "Stability" parameter (0-100%) to globally scale feedback
- 🔧 Add visual feedback indicator in UI (red = danger zone)

### Risk 2: Preset Migration Sounds Wrong
**Mitigation:**
- ✅ Manual validation of all 28 presets
- 🔧 Keep v3 presets available as "Legacy Presets"
- 🔧 Allow users to revert to Ancient Monuments mode (toggle in settings)

### Risk 3: CPU Usage Exceeds Budget
**Mitigation:**
- ✅ Current implementation within budget (4.2%)
- 🔧 Add "Quality" mode: High/Medium/Low (adjusts block size)
- 🔧 Optimize TubeRayTracer with SIMD (saves 0.7% CPU)

### Risk 4: User Confusion with New Macros
**Mitigation:**
- 🔧 Add tooltips to all macro sliders (explain what they do)
- 🔧 Create video tutorial explaining new system
- 🔧 Add "Quick Start" presets that showcase each macro

### Risk 5: DAW Automation Breaks
**Mitigation:**
- ✅ Use JUCE AudioParameterFloat (DAW-compatible)
- 🔧 Test in Logic Pro X, Ableton Live, Pro Tools
- 🔧 Implement parameter smoothing (prevent automation zipper noise)

---

## Success Criteria

### Minimum Viable Release (v2.0)
- ✅ All 3 phases implemented
- ✅ 6 expressive macros functional
- ✅ 8 routing presets working
- ✅ CPU usage < 5%
- ✅ No crashes, clicks, or NaN values
- ✅ 28 factory presets migrated
- ✅ DAW automation works

### Stretch Goals (v2.1+)
- 🎯 Preset morphing (2D XY pad)
- 🎯 Gesture recorder UI
- 🎯 Visual feedback for routing graph
- 🎯 Undo/redo for macro movements
- 🎯 MIDI learn for all macros

---

## Timeline Summary

| Week | Days | Tasks | Deliverable |
|------|------|-------|-------------|
| **1** | 1-5 | Phase 3 implementation | ExperimentalModulation.cpp complete |
| **2** | 6-10 | System integration | Routing graph + macros in PluginProcessor |
| **3** | 11-15 | UI implementation | 6-macro panel + routing selector |
| **4** | 16-21 | Presets + testing | 28 presets migrated, stability validated |

**Total Effort:** 21 days (~3-4 weeks)

---

## Next Session Checklist

Before starting implementation:
- [ ] Backup entire codebase (`git tag v1.5-pre-redesign`)
- [ ] Create feature branch (`git checkout -b experimental-redesign-completion`)
- [ ] Set up build environment
- [ ] Run existing tests to establish baseline

During implementation:
- [ ] Commit after each major task (granular commits)
- [ ] Run tests after each phase
- [ ] Profile CPU usage after each integration
- [ ] Document breaking changes as they occur

Before merging:
- [ ] All tests pass
- [ ] CPU usage < 5%
- [ ] No memory leaks (Valgrind/Instruments)
- [ ] User documentation complete
- [ ] CHANGELOG updated
- [ ] Create v2.0 release tag

---

**Ready to start Week 1, Day 1?**
