# Monument Reverb: Experimental Redesign Proposal

**Date**: 2026-01-04
**Goal**: Transform Monument into a dramatically diverse, expressive, and experimental reverb instrument

---

## Problem Analysis: Why Presets Sound Similar

### Current Limitations

1. **Fixed Serial Chain**: All audio flows through the same path (Foundation → Pillars → Chambers → Weathering → Physical → Buttress → Facade)
   - Every preset uses the same DSP topology
   - Sonic character is constrained by this single architecture
   - No way to create radically different spaces (granular cloud vs metallic chamber vs elastic hall)

2. **Macro Abstractions Are Too Similar**: Current macros (Material, Topology, Viscosity, Evolution, Chaos, Elasticity) map to overlapping parameter sets
   - Material affects Time, Mass, Density
   - Viscosity also affects Time and Mass (conflicts!)
   - User moves multiple macros → parameters fight each other
   - Result: narrow range of "slightly different cathedral" sounds

3. **Traditional Modulation**: ModulationMatrix is powerful but conventional
   - Source → Destination with Depth
   - No randomness, probability, or chaos in routing itself
   - No cross-modulation or feedback loops
   - Missing "happy accident" features that inspire exploration

4. **UI Exposes Complexity First**: 20+ parameters visible by default
   - Overwhelming for sound design
   - Tempts users to tweak individual params (breaks macro coherence)
   - Doesn't encourage preset morphing or experimentation

---

## Solution 1: Flexible DSP Routing System

### Architecture: Modular Routing Graph

Replace fixed serial chain with a **flexible routing graph** where modules can be:
- Series (current default)
- Parallel (multiple paths blended)
- Feedback loops (module output → earlier module input)
- Crossfeed (L→R, R→L channel routing)
- Bypassed entirely

### Implementation: `DspRoutingGraph.h`

```cpp
namespace monument::dsp
{

enum class ModuleType
{
    Foundation,
    Pillars,
    Chambers,
    Weathering,
    TubeRayTracer,
    ElasticHallway,
    AlienAmplification,
    Buttress,
    Facade
};

enum class RoutingMode
{
    Series,        // A → B (current default)
    Parallel,      // A + B (50/50 blend)
    ParallelWet,   // Dry + (A + B) (blend with dry signal)
    Feedback,      // B → A (with gain control)
    Crossfeed,     // L→R, R→L
    Bypass         // Skip module entirely
};

struct RoutingConnection
{
    ModuleType source;
    ModuleType destination;
    RoutingMode mode;
    float blendAmount{0.5f};     // For parallel modes
    float feedbackGain{0.3f};    // For feedback mode
    bool enabled{true};
};

class DspRoutingGraph
{
public:
    void setRouting(const std::vector<RoutingConnection>& connections);
    void process(AudioBuffer<float>& buffer);

    // Preset routing templates
    void loadRoutingPreset(RoutingPresetType preset);

private:
    std::vector<RoutingConnection> routingGraph;
    std::unordered_map<ModuleType, std::unique_ptr<DspModule>> modules;
    AudioBuffer<float> tempBuffers[8];  // For parallel processing
};

} // namespace monument::dsp
```

### Routing Preset Examples

**Preset #1: "Traditional Cathedral"** (Default)
```
Foundation → Pillars → Chambers → Weathering → Buttress → Facade
```

**Preset #2: "Metallic Granular"**
```
Foundation → Pillars (BYPASS Chambers) → TubeRayTracer → Granular Diffusion → Facade
```

**Preset #3: "Elastic Feedback Dream"**
```
Foundation → ElasticHallway ⟲ (Feedback to Pillars) → Chambers → AlienAmplification → Facade
```

**Preset #4: "Parallel Worlds"**
```
Foundation → [Chambers (dry) + TubeRayTracer (metallic) + ElasticHallway (organic)] → Blend → Facade
```

**Preset #5: "Shimmer Infinity"**
```
Foundation → Chambers → PitchShifter (+7 semitones) ⟲ Feedback → Facade
```

### Benefits

- **Dramatic Sonic Diversity**: Each routing creates a fundamentally different instrument
- **Preset Differentiation**: "Metallic Granular" sounds nothing like "Elastic Feedback Dream"
- **Creative Exploration**: Users can design their own routing graphs
- **CPU Efficiency**: Bypass unused modules (save CPU)

---

## Solution 2: Redesigned Macro System

### Problem with Current Macros

Current 6 macros are **conceptual abstractions** (Material, Topology, Viscosity) that require mental translation:
- "What does Viscosity=0.7 sound like?" → User doesn't know
- Multiple macros fight over the same parameters (Time, Mass, Density)
- Doesn't map to musical genres or use cases

### New Macro System: **Expressive Performance Controls**

Replace with 6 **immediately musical** macros that create dramatic diversity:

#### 1. **Character** (0 = Subtle → 1 = Extreme)
- **Low (0.0-0.3)**: Gentle, transparent reverb (mixing/mastering)
- **High (0.7-1.0)**: Dramatic, saturated, effect (sound design)
- **Maps to**: Drive, Saturation, Density, Feedback Intensity

#### 2. **Space Type** (Discrete Modes + Morph)
- **Chamber** (0.0-0.2): Small, resonant, focused
- **Hall** (0.2-0.4): Large, smooth, musical
- **Shimmer** (0.4-0.6): Pitched, bright, ethereal
- **Granular** (0.6-0.8): Textured, diffuse, cloud
- **Metallic** (0.8-1.0): TubeRayTracer + resonances
- **Maps to**: Routing Graph Preset + Module Enables

#### 3. **Energy** (Decay Behavior)
- **Decay** (0.0-0.2): Traditional fade-out
- **Sustain** (0.3-0.5): Stable, freeze-like hold
- **Grow** (0.6-0.8): Bloom, building swell
- **Chaos** (0.9-1.0): Unpredictable, oscillating
- **Maps to**: Feedback, Bloom, Freeze, Paradox Gain

#### 4. **Motion** (Temporal Evolution)
- **Still** (0.0-0.2): Static, frozen, architectural
- **Drift** (0.3-0.5): Slow Brownian wander
- **Pulse** (0.6-0.8): Rhythmic LFO modulation
- **Random** (0.9-1.0): Chaotic attractor jumps
- **Maps to**: Drift, Warp, Modulation Depth, LFO Rate

#### 5. **Color** (Spectral Character)
- **Dark** (0.0-0.2): Lo-fi, vintage, muffled
- **Balanced** (0.3-0.6): Neutral, transparent
- **Bright** (0.7-0.8): Air, shimmer, clarity
- **Spectral** (0.9-1.0): Harmonic distortion, ringing
- **Maps to**: Mass, Air, Gravity, Metallic Resonance

#### 6. **Dimension** (Perceived Space Size)
- **Intimate** (0.0-0.2): Close, personal, booth
- **Room** (0.3-0.5): Standard studio space
- **Cathedral** (0.6-0.8): Large, vast, deep
- **Infinite** (0.9-1.0): Impossible, endless, alien
- **Maps to**: Time, Density, Width, Impossibility Degree

### Macro Mapping Strategy

```cpp
namespace monument::dsp
{

struct ExpressiveMacros
{
    float character{0.5f};      // Subtle → Extreme
    float spaceType{0.2f};      // Chamber/Hall/Shimmer/Granular/Metallic
    float energy{0.1f};         // Decay/Sustain/Grow/Chaos
    float motion{0.2f};         // Still/Drift/Pulse/Random
    float color{0.5f};          // Dark/Balanced/Bright/Spectral
    float dimension{0.5f};      // Intimate/Room/Cathedral/Infinite
};

class ExpressiveMacroMapper
{
public:
    ParameterTargets computeTargets(const ExpressiveMacros& macros) const;

    // Each macro has minimal overlap with others
    // "Character" scales intensity of all effects
    // "Space Type" selects routing graph + module enables
    // "Energy" controls decay behavior (exclusive control)
    // "Motion" controls modulation (exclusive control)
    // "Color" controls spectral balance (exclusive control)
    // "Dimension" controls size/time (exclusive control)

private:
    float mapCharacterToIntensity(float character) const;
    RoutingPresetType mapSpaceTypeToRouting(float spaceType) const;
    float mapEnergyToDecay(float energy) const;
    float mapMotionToModulation(float motion) const;
    float mapColorToSpectrum(float color) const;
    float mapDimensionToSize(float dimension) const;
};

} // namespace monument::dsp
```

### Benefits

- **Immediate Musical Meaning**: "Granular + Grow + Bright + Infinite" = obvious sound
- **No Parameter Conflicts**: Each macro controls orthogonal aspects
- **Dramatic Diversity**: Combinations create vastly different instruments
- **Genre-Specific Workflows**: "Shimmer + Sustain + Bright" = ambient; "Chamber + Decay + Dark" = vintage

---

## Solution 3: Experimental Modulation Features

### Current Mod Matrix: Powerful But Predictable

Current system: `Source (Chaos/Follower/Brownian) → Destination (Time/Mass/Warp) with Depth`

**Missing**: Randomness, surprise, exploration, playfulness

### New Experimental Features

#### Feature 1: **Probability Gates**

Modulation only applies a certain percentage of the time (dice roll per block)

```cpp
struct ProbabilityGate
{
    float probability{1.0f};        // 0.0-1.0 (1.0 = always, 0.5 = 50% of blocks)
    float smoothingMs{50.0f};       // How quickly it fades in/out
    std::mt19937 rng;

    bool shouldModulate()
    {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        return dist(rng) < probability;
    }
};

// Usage: ChaosAttractor → Warp (Depth: 0.5, Probability: 30%)
// Result: Chaos warps space intermittently (unpredictable)
```

#### Feature 2: **Quantized Modulation**

Stepped values instead of smooth (e.g., 5 discrete positions)

```cpp
float quantizeModulation(float smoothValue, int steps)
{
    return std::floor(smoothValue * steps) / steps;
}

// Usage: AudioFollower → Time (Quantized: 8 steps)
// Result: Time jumps between 8 discrete values (rhythmic gating)
```

#### Feature 3: **Cross-Modulation**

One modulation source controls another source's depth/rate

```cpp
struct CrossModConnection
{
    SourceType modulator;       // E.g., AudioFollower
    SourceType target;          // E.g., ChaosAttractor
    ModulationParameter param;  // E.g., Rate or Depth
    float depth{0.5f};
};

// Usage: AudioFollower → ChaosAttractor.Rate (Depth: 0.8)
// Result: Chaos speed increases with input volume (dynamic chaos)
```

#### Feature 4: **Chaos Seeds / Randomize**

One-click randomization of all modulation routings

```cpp
void ModulationMatrix::randomizeAll()
{
    clearConnections();

    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<> sourceDist(0, static_cast<int>(SourceType::Count) - 1);
    std::uniform_int_distribution<> destDist(0, static_cast<int>(DestinationType::Count) - 1);
    std::uniform_real_distribution<> depthDist(-1.0, 1.0);

    // Create 4-8 random connections
    int numConnections = rng() % 5 + 4;
    for (int i = 0; i < numConnections; ++i)
    {
        auto source = static_cast<SourceType>(sourceDist(rng));
        auto dest = static_cast<DestinationType>(destDist(rng));
        float depth = depthDist(rng);
        setConnection(source, dest, 0, depth, 200.0f);
    }
}
```

#### Feature 5: **Preset Morphing**

Blend between 2-4 presets with modulation control

```cpp
class PresetMorpher
{
public:
    void setPresets(int presetA, int presetB, int presetC, int presetD);
    void setMorphPosition(float x, float y);  // 2D morph space

    ParameterTargets computeMorphedParameters() const;

private:
    std::array<ParameterTargets, 4> presets;
    float morphX{0.5f};
    float morphY{0.5f};

    // Bilinear interpolation in 2D preset space
    ParameterTargets interpolatePresets() const;
};

// Usage: Load 4 presets (TL, TR, BL, BR), modulate X/Y with LFO or chaos
// Result: Continuously morphing between 4 sonic worlds
```

#### Feature 6: **Gesture Recording**

Record parameter movements as custom modulation sources

```cpp
class GestureRecorder
{
public:
    void startRecording(DestinationType param);
    void stopRecording();

    void playback(float speed = 1.0f, bool loop = true);

    float getSample() const;  // Use as modulation source

private:
    std::vector<float> recordedValues;
    int playbackIndex{0};
    bool isRecording{false};
};

// Usage: Record manual knob movements, play back as LFO
// Result: Humanized, musical modulation patterns
```

#### Feature 7: **Physics-Based Modulators**

Spring/Mass/Damper systems as chaotic sources

```cpp
class SpringMassModulator
{
public:
    void setSpringConstant(float k);
    void setMass(float m);
    void setDamping(float c);

    void applyForce(float force);  // Driven by audio follower

    float processSample();

private:
    float position{0.0f};
    float velocity{0.0f};
    float k{1.0f}, m{1.0f}, c{0.1f};

    // Differential equation solver (Euler or RK4)
    void updatePhysics(float dt);
};

// Usage: Audio hits drive spring system, position modulates Warp
// Result: Organic, physical response to input dynamics
```

### UI: Modulation "Playground"

Instead of traditional mod matrix grid, create a **playful exploration interface**:

```
┌─────────────────────────────────────────────────┐
│  MODULATION PLAYGROUND                          │
│                                                 │
│  [🎲 Randomize All]  [🔄 Morph Mode]           │
│  [📼 Record Gesture] [🌀 Chaos Seed]           │
│                                                 │
│  Active Connections:                            │
│  ┌──────────────────────────────────────────┐  │
│  │ Chaos.X → Warp (0.6) 🎲 Probability: 40% │  │
│  │ Audio → Time (0.8)   🎚️ Quantized: 8    │  │
│  │ Brownian → Drift (0.5) ⏱️ Cross-Mod     │  │
│  └──────────────────────────────────────────┘  │
│                                                 │
│  [+ Add Random]  [Clear All]                   │
└─────────────────────────────────────────────────┘
```

---

## Solution 4: Simplified UI Architecture

### Current Problem: Information Overload

Current editor shows **20+ parameters** by default:
- 6 macros (Material, Topology, Viscosity, Evolution, Chaos, Elasticity)
- 7 primary params (Mix, Time, Mass, Density, Bloom, Air, Width)
- 5 advanced params (Warp, Drift, Gravity, Pillar Shape, Freeze)
- 12 physical modeling params (Tube Count, Metallic Resonance, etc.)
- Modulation matrix toggle

**Result**: Overwhelming, tempts micro-tweaking, breaks macro coherence

### New UI: Progressive Disclosure

#### Default View: "Performance Mode" (6 controls only)

```
┌─────────────────────────────────────────────────────────┐
│                    MONUMENT REVERB                      │
│                                                         │
│  ┌───────┐  ┌───────┐  ┌───────┐  ┌───────┐           │
│  │CHARACTER│  │ SPACE  │  │ENERGY │  │MOTION │           │
│  │  ███   │  │ Hall  │  │ Grow  │  │ Drift │           │
│  └───────┘  └───────┘  └───────┘  └───────┘           │
│                                                         │
│  ┌───────┐  ┌───────┐                                  │
│  │ COLOR  │  │DIMENSION│                                │
│  │Balanced│  │Cathedral│                                │
│  └───────┘  └───────┘                                  │
│                                                         │
│  ┌─────────────────────────────────────────────────┐   │
│  │  Preset: Cathedral Shimmer ▼                    │   │
│  │  [Previous] [Next] [Save] [Morph Mode]         │   │
│  └─────────────────────────────────────────────────┘   │
│                                                         │
│  [🎨 Advanced] [🌀 Playground] [💾 Presets]           │
└─────────────────────────────────────────────────────────┘
```

#### Advanced View: Full Parameter Access (Hidden by Default)

Click "🎨 Advanced" → Expands to show:
- All 20+ individual parameters
- Routing graph editor
- Modulation matrix (traditional view)

#### Playground View: Experimental Modulation

Click "🌀 Playground" → Shows:
- Randomization tools
- Probability gates
- Preset morphing
- Gesture recording

### Implementation: `PluginEditor.h` Changes

```cpp
class MonumentAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    enum class ViewMode
    {
        Performance,    // Default: 6 macros + presets
        Advanced,       // Full parameter access
        Playground      // Experimental modulation
    };

    void setViewMode(ViewMode mode);

private:
    ViewMode currentView{ViewMode::Performance};

    // Performance Mode (always visible)
    HeroKnob characterKnob;
    HeroKnob spaceTypeKnob;
    HeroKnob energyKnob;
    HeroKnob motionKnob;
    HeroKnob colorKnob;
    HeroKnob dimensionKnob;

    // Advanced Mode (hidden by default)
    std::unique_ptr<juce::Component> advancedPanel;

    // Playground Mode (hidden by default)
    std::unique_ptr<juce::Component> playgroundPanel;
};
```

---

## Solution 5: Preset Diversity Strategy

### Design Presets That Use Dramatically Different Routings

#### Preset #1: "Traditional Cathedral"
```yaml
Routing: Foundation → Pillars → Chambers → Weathering → Facade
Macros:
  Character: 0.3 (Subtle)
  Space Type: 0.3 (Hall)
  Energy: 0.1 (Decay)
  Motion: 0.1 (Still)
  Color: 0.5 (Balanced)
  Dimension: 0.7 (Cathedral)
```

#### Preset #2: "Metallic Granular Dream"
```yaml
Routing: Foundation → Pillars (Bypass Chambers) → TubeRayTracer → Granular Diffusion → Facade
Macros:
  Character: 0.8 (Extreme)
  Space Type: 0.9 (Metallic)
  Energy: 0.7 (Grow)
  Motion: 0.9 (Random)
  Color: 0.8 (Bright)
  Dimension: 0.9 (Infinite)
Modulation:
  Chaos.X → Tube Count (Depth: 0.6, Probability: 40%)
  Audio → Metallic Resonance (Depth: 0.7, Quantized: 5 steps)
```

#### Preset #3: "Elastic Breathing Hall"
```yaml
Routing: Foundation → ElasticHallway → Chambers → Weathering → Facade
         ElasticHallway ⟲ Feedback to Pillars (Gain: 0.3)
Macros:
  Character: 0.5 (Balanced)
  Space Type: 0.3 (Hall)
  Energy: 0.5 (Sustain)
  Motion: 0.3 (Drift)
  Color: 0.4 (Balanced)
  Dimension: 0.6 (Cathedral)
Modulation:
  Audio → Wall Elasticity (Depth: 0.8)
  Brownian → Recovery Time (Depth: 0.5)
```

#### Preset #4: "Shimmer Infinity"
```yaml
Routing: Foundation → Chambers → PitchShifter (+7 semitones) ⟲ Feedback → Facade
Macros:
  Character: 0.7 (Dramatic)
  Space Type: 0.5 (Shimmer)
  Energy: 0.8 (Grow)
  Motion: 0.2 (Drift)
  Color: 0.9 (Bright)
  Dimension: 0.95 (Infinite)
Modulation:
  Chaos.Y → Pitch Shift Amount (Depth: 0.3, Smoothing: 500ms)
```

#### Preset #5: "Impossible Chaos Chamber"
```yaml
Routing: Foundation → Chambers → AlienAmplification → TubeRayTracer → Facade
Macros:
  Character: 0.95 (Extreme)
  Space Type: 0.95 (Metallic)
  Energy: 0.95 (Chaos)
  Motion: 0.95 (Random)
  Color: 0.9 (Spectral)
  Dimension: 1.0 (Infinite)
Modulation:
  Chaos Attractor → Impossibility Degree (Depth: 0.8)
  Chaos Attractor → Paradox Frequency (Depth: 0.6, Cross-Mod from Audio)
  Audio → Paradox Gain (Depth: 0.9, Probability: 60%)
```

---

## Implementation Roadmap

### Phase 1: DSP Routing System (2-3 weeks)
1. Implement `DspRoutingGraph.h` with series/parallel/feedback support
2. Create 5 routing preset templates
3. Update `PluginProcessor::processBlock()` to use routing graph
4. Test CPU efficiency (bypass unused modules)

### Phase 2: Expressive Macros (1-2 weeks)
1. Implement `ExpressiveMacroMapper.h` with 6 new macros
2. Define mapping functions (no parameter conflicts)
3. Update UI to show new macro names/ranges
4. Migrate existing presets to new macro system

### Phase 3: Experimental Modulation (2-3 weeks)
1. Add probability gates to `ModulationMatrix`
2. Implement quantization, cross-modulation
3. Create `PresetMorpher` for 2D morphing
4. Add `GestureRecorder` for custom modulation
5. Implement randomization tools

### Phase 4: UI Simplification (1-2 weeks)
1. Create Performance/Advanced/Playground view modes
2. Hide primary parameters by default
3. Design "Playground" panel UI
4. Add one-click randomization buttons

### Phase 5: Preset Library (1 week)
1. Design 20+ presets using diverse routings
2. Assign expressive macro positions
3. Add experimental modulation to 50% of presets
4. Write preset descriptions/use cases

---

## Expected Outcomes

### Before Redesign
- All presets sound like "slightly different cathedrals"
- Tweaking parameters creates subtle variations
- Modulation is predictable
- UI is overwhelming

### After Redesign
- **Dramatic Sonic Diversity**: "Metallic Granular" vs "Elastic Breathing Hall" vs "Shimmer Infinity" sound like different plugins
- **Expressive Performance**: 6 macros create immediate, musical results
- **Playful Exploration**: Randomization, probability gates, preset morphing inspire happy accidents
- **Simplified UI**: Default view is inviting and focused (6 controls + presets)
- **Genre-Specific**: Ambient, cinematic, sound design, mixing each have optimal presets

---

## Next Steps

1. **Validate Design**: Review this proposal, adjust macro names/ranges
2. **Prototype Routing**: Implement basic series/parallel/bypass in `DspRoutingGraph`
3. **Test One Preset**: Create "Metallic Granular Dream" with new routing
4. **Iterate**: Listen, adjust, refine
5. **Scale**: Apply architecture to all 20+ presets
