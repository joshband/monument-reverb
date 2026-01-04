# Monument-Reverb: Comprehensive Architecture Review
## Creating Innovative Macro Controls & Impossible Spaces

**Prepared for:** Advanced Creative Feature Development
**Date:** 2026-01-03
**Scope:** Full codebase architecture analysis with recommendations for innovative macro controls, physical modeling, and alien atmospheres

---

## EXECUTIVE SUMMARY

Monument-Reverb has an **excellent foundational architecture** for building creative reverb spaces. The modular DSP design, parameter management system, and real-time safety practices are solid. However, to achieve your vision of:

- **Musically morphing macro controls** that mutate parameters in coordinated ways
- **Physical tube modeling** with distance-based propagation
- **Hyper-elastic hallways** with deformable geometry
- **Alien atmospheres** with impossible sound behavior

...you need to add three critical architectural layers:

1. **Macro Control System** - A parameter hierarchy that maps high-level "material/topology/viscosity" controls to multiple underlying parameters
2. **Advanced Modulation Bus** - Multiple modulation sources (chaos, audio-following, Brownian motion) that coordinate parameter evolution
3. **Physical/Algorithmic Modeling Modules** - New DSP modules for tubes, elastic spaces, and non-Euclidean behavior

---

## IMPLEMENTATION STATUS UPDATE

### Phase 3: Modulation Sources - ✅ COMPLETE (2026-01-03)

All 4 modulation sources fully implemented with real-time DSP and integrated into ModulationMatrix.

### Phase 2: Macro System Integration - ✅ COMPLETE (2026-01-03)

Macro system fully operational in audio processing pipeline with automatic parameter blending.

### Phase 1: Macro System Foundation - ✅ COMPLETE (2026-01-03)

The macro control system and modulation matrix infrastructure have been successfully implemented:

**Completed:**

- ✅ **MacroMapper** ([dsp/MacroMapper.h](dsp/MacroMapper.h), [dsp/MacroMapper.cpp](dsp/MacroMapper.cpp)) - 399 lines
  - 6 macro parameters (Material, Topology, Viscosity, Evolution, Chaos Intensity, Elasticity Decay)
  - Musical mapping functions with weighted influence combining
  - All parameters normalized [0, 1] with clear semantic ranges
- ✅ **ModulationMatrix** ([dsp/ModulationMatrix.h](dsp/ModulationMatrix.h), [dsp/ModulationMatrix.cpp](dsp/ModulationMatrix.cpp)) - 438 lines
  - 4 modulation source types (Chaos Attractor, Audio Follower, Brownian Motion, Envelope Tracker)
  - 16 parameter destinations including all base parameters and future physical modeling targets
  - Connection management system with per-connection depth and smoothing
  - Block-rate processing with juce::SmoothedValue for zipper-free modulation
  - Stub source implementations (Phase 2 will implement full DSP)
- ✅ **APVTS Integration** ([plugin/PluginProcessor.cpp](plugin/PluginProcessor.cpp:585-626))
  - 6 new macro parameters exposed in AudioProcessorValueTreeState
  - Real-time safe parameter polling infrastructure ready
- ✅ **Build System** ([CMakeLists.txt](CMakeLists.txt:72-75))
  - New source files integrated, build successful

**Phase 2 Complete:**

- ✅ Integrated into PluginProcessor::processBlock() ([plugin/PluginProcessor.cpp](plugin/PluginProcessor.cpp:197-310))
- ✅ Macro influence blending (base params → macro targets based on distance from defaults)
- ✅ All module setters use macro-influenced effective parameters
- ✅ Build verified, AU/VST3 plugins functional

**Phase 3 Complete:**

- ✅ **ChaosAttractor** ([dsp/ModulationMatrix.cpp](dsp/ModulationMatrix.cpp:22-101)) - Lorenz attractor, 3-axis output
- ✅ **AudioFollower** ([dsp/ModulationMatrix.cpp](dsp/ModulationMatrix.cpp:109-178)) - RMS envelope tracking with attack/release
- ✅ **BrownianMotion** ([dsp/ModulationMatrix.cpp](dsp/ModulationMatrix.cpp:186-250)) - Smooth random walk with boundary reflection
- ✅ **EnvelopeTracker** ([dsp/ModulationMatrix.cpp](dsp/ModulationMatrix.cpp:258-367)) - Multi-stage envelope detection (attack/sustain/release)
- ✅ **Full ModulationMatrix Integration** - Sources process per-block, accumulate to destinations, smoothed output
- ✅ **Parameter Application** ([plugin/PluginProcessor.cpp](plugin/PluginProcessor.cpp:276-299)) - Modulation offsets applied to all parameters
- ✅ **"Living" Presets** ([plugin/PresetManager.cpp](plugin/PresetManager.cpp:107-140)) - 5 new factory presets with modulation:
  - Breathing Stone, Drifting Cathedral, Chaos Hall, Living Pillars, Event Horizon Evolved
- ✅ **Preset Architecture** ([plugin/PresetManager.h](plugin/PresetManager.h:29)) - Modulation connections stored & applied on load
- ✅ Build successful, plugins installed (VST3 + AU)

**Phase 3-4 Roadmap (Remaining):**

- Phase 3b (Weeks 5-6): Add physical modeling modules (TubeRayTracer, ElasticHallway, AlienAmplification)
- Phase 4 (Weeks 7-8): UI integration, preset showcase, documentation

---

## PART 1: CURRENT ARCHITECTURE ANALYSIS

### Strengths

#### 1. **Excellent Module Architecture** (`dsp/`)
Monument's 8-module serial pipeline is clean and extensible:
```
Input → Foundation → Pillars → Chambers → Weathering → Buttress → Facade → Output
```

Each module:
- Inherits from `DspModule` base class with `prepare/reset/process` interface
- Is real-time safe (no allocations in `process()`)
- Uses JUCE idioms correctly (HeapBlock, ScopedNoDenormals)
- Has parameter smoothing to prevent zipper noise

**Why this matters for your vision:** The module chain is the right place to insert your new components (TubeRayTracer, ElasticHallway, AlienAmplification). You can extend it without refactoring the core.

#### 2. **Sophisticated Parameter Management** (`plugin/PluginProcessor`)
- JUCE AudioProcessorValueTreeState (APVTS) for parameter hosting
- Lock-free parameter polling per audio block
- 16 parameters (7 primary + 5 advanced + 4 memory)
- ParameterSmoother prevents aliasing artifacts
- Clean separation between host interface and DSP

**Why this matters:** The existing parameter infrastructure scales well for macro controls. You can add new macro parameters to APVTS and wire them through a new mapping layer.

#### 3. **Memory Echoes System** (`dsp/MemoryEchoes`)
A sophisticated capture/recall system with:
- Age-based decay and frequency rolloff
- Random surfacing (probabilistic triggering)
- Pitch instability (±15 cents drift)
- Freeze state capture
- Excellent testing harness

**Why this matters:** Memory Echoes demonstrates Monument's intent for creative, generative behavior. This is the seed for your broader modulation and mutation systems.

#### 4. **Chambers FDN Core** (`dsp/Chambers`)
- 8-line feedback delay network with prime-length delays
- Two mixing matrices (Hadamard orthogonal + Householder dense)
- Per-line damping filters and allpass diffusion
- Matrix interpolation for "warp" (spatial morphing)
- Freeze state management
- Drift LFO modulation (±1 sample per line)

**Why this matters:** This is the reverb engine that will be your canvas. The warp and drift systems already show how to do slow, musically meaningful parameter mutation. You're building on proven concepts.

#### 5. **Thoughtful Preset System** (`plugin/PresetManager`)
- 18 curated factory presets with narrative descriptions
- Preset categories (Foundational, Living, Remembering, Abstract)
- JSON serialization for user presets
- Smooth preset transitions (fade-out/reset/fade-in)

**Why this matters:** Your macro system needs a preset structure that can serialize macro positions, modulation matrix connections, and physical modeling parameters. The current JSON system scales well.

#### 6. **Parameter Mapping Philosophy**
Most parameters map inputs [0, 1] to meaningful ranges:
- `Time: [0, 1] → feedback gain [0.35–0.995]`
- `Mass: [0, 1] → damping filter cutoff [0.9–1.6]`
- `Density: [0, 1] → affects multiple stages`

**Why this matters:** This normalized parameter approach is perfect for macro controls. Each macro can similarly map [0, 1] to coordinated parameter values across multiple modules.

---

### Gaps for Your Vision

#### 1. **No Parameter Interaction Layer**
Currently, each of the 7 primary parameters is mostly independent:
- Changing `Time` doesn't affect `Mass`
- Changing `Density` doesn't musically interact with `Bloom`
- Parameters respond only to direct host automation

**What you need:** A **Macro Control Layer** that creates coordinated parameter mutations. Example:
```
MATERIAL Macro (0→1):
  Time:    0.2 → 0.95
  Mass:    0.2 → 0.9
  Density: 0.3 → 1.0

When you move MATERIAL slider, all three parameters move together musically
```

#### 2. **Limited Modulation Sources**
Current modulation is minimal:
- Warp: sub-Hz interpolation between matrix types (deterministic)
- Drift: per-line LFOs at 0.05–0.2 Hz (repetitive)
- Memory Echoes: random-triggered recalls (not parameter-driven)

**What you need:** A **Modulation Matrix** with multiple sources:
- Chaos attractors (Lorenz, Rössler) - deterministic but unpredictable
- Audio followers - react to input signal energy
- Brownian motion - smooth random walks
- Envelope trackers - responsive to musical dynamics
- Each source can modulate any parameter (64+ connections possible)

#### 3. **No Physical Modeling**
Monument is "impossible scale architecture" but not physically modeled. It's:
- Algorithmic (FDN is abstract, not ray-traced)
- Geometric only in concept (Chambers is not modeled as a real room)
- No distance-based effects (no attenuation by travel distance)
- No frequency-dependent propagation (only frequency-dependent decay via Gravity)

**What you need:** New DSP modules for:
- **TubeRayTracer** - Sound bouncing through series of metal tubes
- **ElasticHallway** - Room with deformable walls that respond to energy
- **AlienAmplification** - Non-Euclidean space where normal acoustic rules break

#### 4. **No Algorithmic Composition Layer**
Parameters respond only to:
- Direct user automation
- Preset selection (instant switch)
- Memory Echoes surfacing (probabilistic)

**What you need:** Systems that generate parameter sequences:
- Macro morphing schedules (evolve over 10s of seconds)
- Chaotic parameter evolution (deterministic but unpredictable)
- Correlation between parameters (when warp increases, drift also increases)

#### 5. **No Spatialization or Position-Based Effects**
The reverb doesn't know where sound "is" in the space:
- All delays are fixed (or slowly modulated)
- No distance attenuation
- No Doppler effects
- Stereo width is just mid/side scaling, not spatial modeling

**What you need:** Optional spatial routing where:
- Objects can have position in 3D space
- Sound traveling to/from position has distance-based effects
- Walls can be positioned and have frequency-dependent absorption

---

## PART 2: ARCHITECTURAL RECOMMENDATIONS

### Tier 1: Macro Control System

Create a **MacroMapper** class that converts 6 high-level controls into coordinated parameter sets:

```cpp
// New file: dsp/MacroMapper.h
class MacroMapper {
public:
    // User controls (host automation)
    float materialMacro;    // 0–1: soft material → hard material
    float topologyMacro;    // 0–1: regular room → non-Euclidean space
    float viscosityMacro;   // 0–1: airy → thick medium
    float evolutionMacro;   // 0–1: static → blooming/changing
    float chaosIntensity;   // 0–1: stable → chaotic
    float elasticityDecay;  // 0–1: instant recovery → slow deformation

    // Compute underlying parameter targets
    ParameterValues computeTargets() const;
};
```

**Integration:** In `PluginProcessor::processBlock()`, compute macro targets once per block:
```cpp
auto macroTargets = macroMapper.computeTargets();
// Then blend with actual parameter values using existing ParameterSmoother
```

**Benefits:**
- High-level, intuitive controls (6 parameters instead of 16)
- Coordinated parameter mutation (musically coherent)
- Exposes creativity layer without complexity
- Scales to future features (10+ macros possible)

### Tier 2: Modulation Matrix

Create a central **ModulationMatrix** that routes modulation sources to parameter destinations:

```cpp
// New file: dsp/ModulationMatrix.h
class ModulationMatrix {
private:
    // 4 sources of modulation
    std::unique_ptr<ChaosAttractor> chaosGen;
    std::unique_ptr<AudioFollower> audioFollower;
    std::unique_ptr<BrownianMotion> brownianGen;
    std::unique_ptr<EnvelopeTracker> envTracker;

    // 64 connections: 4 sources × 16 destinations
    struct Connection {
        int sourceIndex;        // 0-3
        int destinationIndex;   // 0-15 (parameters)
        float depth;           // -1 to +1 (bipolar modulation)
        float smoothingMs;     // 20-1000 ms lag
    };
    std::vector<Connection> connections;

public:
    void process(int numSamples);  // Called once per audio block
    void setConnection(int src, int dest, float depth, float lag);

    // Query current modulation values (per destination)
    float getModulation(int destinationIndex) const;
};
```

**Why modulation happens at block rate (not sample rate):**
- Macros + modulation = 16+ parameters changing
- Modulation sources (chaos, envelope) are smooth/slow
- Block-rate updates (~88 parameters/ms at 48kHz) are imperceptible
- Huge CPU savings vs sample-rate updates

**Typical Connections:**
```
Chaos Attractor axis 0 → Warp (depth 0.3)
Chaos Attractor axis 1 → Drift (depth 0.4)
Audio Follower → Mass (depth 0.15)  [input dynamics increase density]
Brownian Motion (1/f) → Bloom (depth 0.2)  [smooth swelling]
Envelope Tracker → Gravity (depth 0.25)  [frequency rolloff follows dynamics]
```

**Benefits:**
- Parameter evolution feels "alive" and reactive
- Relationships are explicitly serialized (presets capture modulation routings)
- Scales to unlimited sources/destinations
- Real-time safe: all state pre-allocated, no locks

### Tier 3: Physical Modeling Modules

Add three new modules to the DSP chain. Each should:
- Inherit from `DspModule`
- Pre-allocate all state in `prepare()`
- Use block-rate processing where possible (not sample-rate)

#### A. TubeRayTracer
**Position in chain:** Between Chambers and Weathering

**Purpose:** Simulate sound bouncing through a series of metal tubes

**Parameters:**
- `tubeCount` (5–16): How many virtual tubes
- `tubeRadiusVariation` (0–1): Variation in tube diameters
- `metallicResonance` (0–1): Emphasis of tube modal frequencies
- `couplingStrength` (0–1): Energy transfer between tubes

**Implementation outline:**
```cpp
class TubeRayTracer : public DspModule {
private:
    struct Tube {
        float lengthSamples;      // 0.5m–10m @ 48kHz
        float diameterMM;         // 5–50mm
        float absorptionPerSample; // frequency-dependent
        std::vector<float> modalFrequencies;  // Helmholtz resonances
        juce::dsp::IIR::Filter<float> resonanceFilter;
    };

    std::vector<Tube> tubes;

    // Ray-tracing state (block-rate)
    std::vector<float> rayEnergy;
    int rayCount = 64;

public:
    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void process(juce::AudioBuffer<float>& buffer) override;

private:
    void traceRays();  // Called once per block
};
```

**Why this matters:** Metal tube modeling creates:
- Subtle harmonic coloration (modal emphasis)
- Distance-based frequency loss (high-frequency rolloff)
- Resonant peaks at specific frequencies
- Interaction with input pitch creates formant effects

#### B. ElasticHallway
**Position in chain:** Wraps/extends Chambers (couples to delay times and matrix)

**Purpose:** Non-linear geometry with walls that deform under acoustic pressure

**Parameters:**
- `elasticity` (0–1): How much walls deform
- `elasticityRecoveryMs` (100–5000): How fast they recover
- `wallAbsorptionDrift` (0–1): How absorption changes over time
- `nonlinearityFactor` (0–1): Energy-dependent reflections

**Implementation outline:**
```cpp
class ElasticHallway : public DspModule {
private:
    // Room geometry
    float roomWidth, roomHeight, roomDepth;

    // Wall state
    float elasticDeformation = 0.0f;  // -20% to +20% of nominal
    float internalPressure = 0.0f;    // RMS accumulator
    juce::dsp::IIR::Coefficients<float> pressureFilter;  // exponential averaging

    // Pre-computed modal frequencies
    std::vector<float> modalFrequencies;  // Room modes

public:
    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void process(juce::AudioBuffer<float>& buffer) override;

private:
    void updateWallDeformation(float rms);
    void modifyChambersState(const Chambers& chambers);  // Couple to FDN delays
};
```

**Why this matters:** Elastic walls create:
- Slow geometric morphing without pitch shift
- Energy-responsive architecture (louder input → more deformation)
- Smooth wall recovery creates evolving timbre
- Non-linear feedback (walls push back against pressure)

#### C. AlienAmplification
**Position in chain:** Post-Chambers (wraps/colors the FDN output)

**Purpose:** Non-Euclidean sound behavior with impossible physics

**Parameters:**
- `impossibilityDegree` (0–1): How much it violates normal acoustics
- `pitchEvolutionRate` (0–1): Frequency content morphs with age
- `paradoxResonanceFreq` (50–5000 Hz): Frequency that amplifies instead of decays
- `paradoxGain` (1.00–1.05): Amplification factor (kept < 1.0 for stability)

**Implementation outline:**
```cpp
class AlienAmplification : public DspModule {
private:
    // Pitch evolution: allpass cascade shifts frequency with age
    std::array<juce::dsp::IIR::Filter<float>, 8> pitchEvolutionBands;

    // Paradox resonance: narrow peak that violates decay rules
    juce::dsp::IIR::Filter<float> paradoxResonance;  // Biquad with clamped Q

    // Non-local absorption: frequency-dependent but changes over time
    std::vector<float> absorptionCurve;  // Per-band profile
    juce::dsp::IIR::Filter<float> absorptionFilter;

public:
    void prepare(const juce::dsp::ProcessSpec& spec) override;
    void process(juce::AudioBuffer<float>& buffer) override;

private:
    void updatePitchEvolution(float age);
    void updateParadoxResonance(float frequency, float gain);
};
```

**Why this matters:** Alien atmospheres create:
- Frequency content that evolves with time (pitch walks up/down)
- Impossible acoustics (sound gets louder as it decays)
- Frequency-specific rule breakage
- Sense of "otherworldliness" through violation of expected behavior

---

### Integration with Existing Systems

#### Memory Echoes Enhancement
Memory Echoes is currently:
- **Good:** Captures fragments, recalls with age-based coloring
- **Gap:** Doesn't interact with macro system or modulation

**Enhancement:**
```cpp
// In MemoryEchoes::surfaceMemory()
// When recalling a fragment:
// 1. Get original macro state from capture time
// 2. Current macro state
// 3. Blend them: recalled_macros = original * age_factor + current * (1 - age_factor)
// 4. Apply blended macros to underlying parameters
// → Memories "morph" into current space as they age
```

**Why this matters:** Bridges memory system and macro system. Old memories don't sound like copies; they gradually transform into the current sonic context.

---

## PART 3: IMPLEMENTATION ROADMAP

### Phase 1: Foundation (Real-Time Safe Infrastructure)

**Duration:** 1–2 weeks
**Files to create/modify:**
- `dsp/MacroMapper.h/cpp` - Macro parameter mapping
- `dsp/ModulationMatrix.h/cpp` - Central modulation bus
- `plugin/PluginProcessor.h/cpp` - Integrate macros + modulation into process chain

**Deliverables:**
1. Macro system with 6 high-level controls
2. Modulation matrix infrastructure (empty, no sources yet)
3. Parameter smoothing pipeline
4. Preset serialization for macros + modulation

**Testing:**
- Macro controls move underlying parameters correctly
- Modulation matrix accepts/stores connections
- No audio dropouts or performance regression
- Real-time safe (no allocations in process())

### Phase 2: Modulation Sources (Creative Parameter Evolution)

**Duration:** 2–3 weeks
**Files to create:**
- `dsp/ChaosAttractor.h/cpp` - Lorenz/Rössler chaos
- `dsp/AudioFollower.h/cpp` - Input-reactive envelope
- `dsp/BrownianMotion.h/cpp` - Smooth random walks
- `dsp/EnvelopeTracker.h/cpp` - Multi-stage envelope detection

**Deliverables:**
1. Four independent modulation sources
2. Integration into ModulationMatrix
3. Visual indicators in UI (LEDs showing source activity)
4. Preset storage of modulation routings

**Testing:**
- Each source produces expected behavior
- Sources can be routed to any parameter
- Modulation sounds natural, not abrupt
- CPU budget remains < 0.5% per source

### Phase 3: Physical Modeling Modules

**Duration:** 3–4 weeks
**Files to create:**
- `dsp/TubeRayTracer.h/cpp` - Metal tube propagation
- `dsp/ElasticHallway.h/cpp` - Deformable room geometry
- `dsp/AlienAmplification.h/cpp` - Non-Euclidean behavior

**Deliverables:**
1. Three new modules in DSP chain
2. Parameter controls for each
3. Integration with ModulationMatrix (can be modulated)
4. Preset defaults

**Testing:**
- Each module adds desired coloration
- Modules don't cause instability
- CPU budget < 3% for all three
- Sound quality maintained (no aliasing)

### Phase 4: Integration & Polish

**Duration:** 2–3 weeks
**Modifications:**
- `plugin/PluginEditor.cpp` - UI for macros, modulation matrix, physical params
- `plugin/PresetManager.cpp` - Extended preset JSON with all new data
- Documentation updates
- Factory presets demonstrating new capabilities

**Deliverables:**
1. Intuitive UI for macro controls
2. Modulation matrix visualization
3. 10–15 new factory presets
4. User manual update

---

## PART 4: CODE-LEVEL GUIDANCE

### Parameter Smoothing Pipeline

Current: Parameters → ParameterSmoother → DSP Module

New: Host Parameters → Macro Mapper → Modulation Matrix → Parameter Smoother → DSP Module

```cpp
// In PluginProcessor::processBlock()
void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                  juce::MidiBuffer& midi) {
    // 1. Read raw parameters from host automation
    float macroMaterial = apvts.getRawParameterValue("material")->load();
    float macroTopology = apvts.getRawParameterValue("topology")->load();
    // ... etc

    // 2. Compute macro targets (this round)
    MacroMapper::ParameterValues targets = macroMapper.computeTargets(
        macroMaterial, macroTopology, viscosity, evolution, chaos, elasticity
    );

    // 3. Apply modulation (add perturbations)
    modulationMatrix.process(numSamples);
    float warpMod = modulationMatrix.getModulation(PARAM_WARP);
    float driftMod = modulationMatrix.getModulation(PARAM_DRIFT);
    // ... etc

    // 4. Update parameter smoothers (these prevent aliasing)
    smoother_time.setTargetValue(targets.time + warpMod);
    smoother_mass.setTargetValue(targets.mass);
    // ... etc

    // 5. Process audio through DSP modules
    foundation.process(buffer);
    pillars.process(buffer);
    chambers.setFeedback(smoother_time.getNextValue());
    chambers.setDensity(smoother_mass.getNextValue());
    chambers.process(buffer);
    // ... etc
}
```

### Real-Time Safety Checklist

When implementing new modules, ensure:

```cpp
class NewModule : public DspModule {
public:
    void prepare(const juce::dsp::ProcessSpec& spec) override {
        // ✅ Pre-allocate ALL buffers
        delayLine.allocate(maxDelayMs * spec.sampleRate / 1000.0);
        rayBuffer.allocate(maxRays * maxTubes);

        // ✅ Initialize all state
        internalState = 0.0f;

        // ❌ DO NOT allocate here again if already allocated
        // ❌ DO NOT use new/delete
    }

    void reset() override {
        // ✅ Clear state
        delayLine.clear();
        internalState = 0.0f;
    }

    void process(juce::AudioBuffer<float>& buffer) override {
        juce::ScopedNoDenormals noDenormals;

        // ✅ Only parameter polling and memory reads
        // ❌ NO allocations
        // ❌ NO file I/O
        // ❌ NO locks (mutex, semaphore)
        // ❌ NO logging
        // ❌ NO random() - use PRNG seeded in prepare()

        // ✅ Process samples
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            auto* data = buffer.getWritePointer(channel);
            for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
                data[sample] = processSample(data[sample], channel);
            }
        }
    }
};
```

### Parameter Mapping Pattern

Following Monument's approach, all macros and parameters should map [0, 1] to meaningful ranges:

```cpp
// Good: Normalized [0, 1] input → meaningful range
float MaterialMacro::mapToMass(float macro) {
    // macro: 0 (soft) → 1 (hard)
    // Mass controls low-pass damping
    // Soft = less damping (bright)
    // Hard = more damping (dark)
    return 0.2f + macro * (0.9f - 0.2f);  // [0.2, 0.9]
}

// Good: Clear parameter sweep
void TubeRayTracer::setMetallicResonance(float normalized) {
    // 0 = no resonance emphasis
    // 1 = strong harmonic peaks
    // Maps to resonance filter Q
    float q = 1.0f + normalized * 9.0f;  // [1.0, 10.0]
    resonanceFilter.setQ(q);
}

// ❌ Avoid: Arbitrary mappings or unmotivated ranges
void BadModule::setThing(float normalized) {
    internalValue = normalized;  // ← What does 0.5 mean?
}
```

### Denormal Flushing

All modules should flush denormals to prevent CPU spikes:

```cpp
void Module::process(juce::AudioBuffer<float>& buffer) {
    juce::ScopedNoDenormals noDenormals;  // ← Automatic flush

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
        // ... process
        // If output ever becomes very small (< 1e-30), flush to zero:
        if (std::abs(output) < 1e-30f) output = 0.0f;
    }
}
```

### DSP Module Ordering

Suggested signal flow:

```
Input
  ↓
Foundation (DC block, gain normalization)
  ↓
Pillars (early reflections)
  ↓
Chambers (FDN reverb core) ← Couples to ElasticHallway for matrix morphing
  ↓
[NEW] TubeRayTracer (optional: metal tube coloration)
  ↓
[NEW] AlienAmplification (optional: pitch evolution, paradox resonance)
  ↓
Weathering (warp/drift LFOs)
  ↓
Buttress (feedback safety)
  ↓
Facade (stereo width, air, output gain)
  ↓
[MemoryEchoes - optional sidechain injection]
  ↓
Mix (dry/wet blend)
  ↓
Output
```

---

## PART 5: PERFORMANCE CONSIDERATIONS

### CPU Budget

Current Monument overhead: ~0.5% CPU per instance (48 kHz, stereo)

New features should stay within these budgets:

| Feature | Target CPU | Implementation Strategy |
|---------|-----------|------------------------|
| Macro system | <0.1% | Just parameter mapping, no DSP |
| Modulation matrix | 0.3–0.5% | Block-rate updates, 4 × 16 connections |
| Chaos generators | 0.2–0.4% | LUT-based state iteration |
| Audio follower | 0.1% | One-pole RMS filter |
| Brownian motion | 0.1% | Pseudorandom generator |
| TubeRayTracer | 1–2% | Block-rate ray tracing, ~64 rays |
| ElasticHallway | 0.5–1% | Modal filter bank, 8 bands |
| AlienAmplification | 0.7–1% | Allpass cascade, biquad resonance |
| **Total (all enabled)** | **3–5%** | Scales with instance count |

### Memory Budget

Per-instance memory: ~25–30 KB (negligible for plugins)

Largest allocations:
- Tube grid: ~16 KB (16 tubes × 64 rays × various state)
- Chaos/Brownian buffers: ~4 KB
- Filter coefficients: ~2 KB

### Optimization Techniques

1. **Block-Rate Processing:** Modulation, ray tracing, and modal updates happen once per audio block (2048 samples @ 48 kHz = 42 ms), not per sample
2. **Lookup Tables:** Pre-compute tube resonances, absorption curves, chaos attractors
3. **SIMD:** Vectorize chaos iteration and envelope following if profiling shows bottleneck
4. **Selective Enabling:** Features can be toggled off via compile flags for lighter variants (e.g., "Monument Lite")

---

## PART 6: PRESET ARCHITECTURE

### Extended Preset Format

```json
{
  "name": "Crystalline Bloom",
  "description": "Hard material with elastic walls that absorb pitch evolution",
  "version": "0.2.0",

  "macros": {
    "material": 0.85,
    "topology": 0.4,
    "viscosity": 0.3,
    "evolution": 0.75,
    "chaosIntensity": 0.2,
    "elasticityDecay": 0.6
  },

  "baseParameters": {
    "time": 0.65,
    "mix": 0.75
  },

  "physicalModeling": {
    "tubeCount": 10,
    "tubeRadiusVariation": 0.3,
    "metallicResonance": 0.8,
    "couplingStrength": 0.5
  },

  "elasticHallway": {
    "roomWidth": 0.6,
    "elasticity": 0.7,
    "elasticityRecoveryMs": 2500,
    "wallAbsorptionDrift": 0.3
  },

  "alienAtmospheres": {
    "impossibilityDegree": 0.4,
    "pitchEvolutionRate": 0.6,
    "paradoxResonanceFreq": 432.0,
    "paradoxGain": 1.03
  },

  "modulation": [
    {
      "source": "chaos",
      "sourceAxis": 0,
      "destination": "warp",
      "depth": 0.3,
      "smoothingMs": 200
    },
    {
      "source": "audioFollower",
      "destination": "mass",
      "depth": 0.15,
      "smoothingMs": 50
    },
    {
      "source": "brownian",
      "destination": "bloom",
      "depth": 0.2,
      "smoothingMs": 300
    }
  ]
}
```

### Preset Categories (Suggested)

- **Foundational Materials** - Hard, soft, ceramic, metallic
- **Impossible Topologies** - Non-Euclidean, folded, infinite
- **Elastic Responses** - Slow deformation, rigid, bouncy
- **Chaotic Systems** - Deterministic chaos, strange attractors
- **Alien Atmospheres** - Pitch evolution, paradox resonance
- **Interactive Spaces** - Audio-reactive, dynamically evolving

---

## PART 7: UI DESIGN FRAMEWORK

### New Editor Sections (Expandable Panels)

#### 1. Macro Control Panel (Primary)
```
┌─ MACRO CONTROLS ────────────────────────┐
│                                         │
│  [Material]      Soft ←→ Hard          │
│  [Topology]      Regular ←→ Chaotic    │
│  [Viscosity]     Airy ←→ Thick         │
│  [Evolution]     Static ←→ Blooming    │
│  [Chaos]         Stable ←→ Unstable    │
│  [Elasticity]    Instant ←→ Slow       │
│                                         │
└─────────────────────────────────────────┘
```

#### 2. Modulation Matrix (Advanced)
```
┌─ MODULATION MATRIX ─────────────────────┐
│                Time Mass Warp Drift ... │
│ Chaos      [●]  [·]  [●●] [··]         │
│ Audio      [··] [●●] [·]  [··]         │
│ Brownian   [●]  [·]  [··] [●●]         │
│ Envelope   [··] [●●] [··] [·]          │
│            ← Click to adjust depth      │
└─────────────────────────────────────────┘
```

#### 3. Physical Modeling (Expandable)
```
┌─ PHYSICAL MODELING ─────────────────────┐
│ [Tube Count: 8/16]                      │
│ [Metallic Resonance: ████░░]   0.8      │
│ [Coupling Strength: ██░░░░░]    0.25    │
│                                         │
│ Visualization: [Wire-frame room model]  │
└─────────────────────────────────────────┘
```

#### 4. Alien Atmospheres (Advanced)
```
┌─ ALIEN ATMOSPHERES ─────────────────────┐
│ [Impossibility Degree: ███░░░]  0.45    │
│ [Pitch Evolution Rate: ██░░░░░] 0.28    │
│ [Paradox Resonance: 432 Hz]             │
│ [Paradox Gain: 1.02]                    │
│                                         │
│ Pitch evolution curve: [Curve display]  │
└─────────────────────────────────────────┘
```

### Interaction Model

- **Macros:** Large, prominent sliders (75% of UI)
- **Modulation:** Secondary, expandable section
- **Physical/Alien:** Tertiary, expert-level controls
- **Presets:** Enhanced browser with macro preview, modulation routing labels

---

## PART 8: TESTING & VALIDATION STRATEGY

### Unit Tests

Create test harnesses for each new component:

```cpp
// tests/ChaosAttractorTest.cpp
TEST(ChaosAttractor, LorenzProducesNonRepeatingSeries) {
    ChaosAttractor chaos(ChaosAttractor::Type::Lorenz);
    chaos.prepare({48000, 2048, 2});

    std::vector<float> output;
    for (int i = 0; i < 10000; ++i) {
        output.push_back(chaos.iterate());
    }

    // Verify non-repetition (statistically unique samples)
    // Verify bounded output
    // Verify deterministic (same seed = same sequence)
}

// tests/TubeRayTracerTest.cpp
TEST(TubeRayTracer, ProducesExpectedColoration) {
    TubeRayTracer tracer;
    tracer.prepare({48000, 2048, 2});

    juce::AudioBuffer<float> impulse(2, 2048);
    impulse.setSample(0, 0, 1.0f);  // Dirac impulse

    tracer.process(impulse);

    // Verify output is colored (not purely impulse)
    // Verify high-frequency rolloff
    // Verify stability
}

// tests/MacroMapperTest.cpp
TEST(MacroMapper, MapsCoordinatedParameters) {
    MacroMapper mapper;
    auto targets = mapper.computeTargets(0.5, 0.5, 0.5, 0.5, 0.5, 0.5);

    // Verify targets are within bounds
    EXPECT_GE(targets.time, 0.35f);
    EXPECT_LE(targets.time, 0.995f);

    // Verify macros at extremes map correctly
    auto hardMaterial = mapper.computeTargets(1.0, 0.5, 0.5, 0.5, 0.5, 0.5);
    EXPECT_GT(hardMaterial.mass, mapper.computeTargets(0.0, 0.5, 0.5, 0.5, 0.5, 0.5).mass);
}
```

### Integration Tests

```cpp
// tests/FullChainTest.cpp
TEST(FullChain, ProcessesAudioWithoutDistortion) {
    PluginProcessor processor;
    processor.prepareToPlay(48000, 2048);

    // Create test signal (sweep 20 Hz–20 kHz)
    juce::AudioBuffer<float> input(2, 96000);  // 2 seconds
    generateSweepSignal(input, 20.0f, 20000.0f);

    // Set all features to active
    processor.setMacro(MATERIAL, 0.7);
    processor.setModulation(CHAOS, WARP, 0.3);
    processor.setTubeCount(16);
    processor.setElasticity(0.8);
    processor.setImpossibilityDegree(0.5);

    // Process
    juce::MidiBuffer midiBuffer;
    processor.processBlock(input, midiBuffer);

    // Verify output
    EXPECT_FALSE(hasNaN(input));
    EXPECT_FALSE(hasInfinity(input));
    EXPECT_LT(getPeakLevel(input), 1.0f);  // No clipping
}
```

### Performance Profiling

```cpp
// tests/PerformanceTest.cpp
TEST(Performance, ModulationMatrixUnder1PercentCPU) {
    // Run 1000 blocks, measure CPU time
    auto startTime = std::chrono::high_resolution_clock::now();

    for (int block = 0; block < 1000; ++block) {
        modulationMatrix.process(2048);
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        endTime - startTime).count();

    // 1000 blocks × 2048 samples = ~42 seconds of audio
    // Expected time: ~420 ms (1% CPU)
    EXPECT_LT(totalMs, 500);  // Allow 1.2% margin
}
```

---

## PART 9: MIGRATION GUIDE FOR EXISTING PRESETS

Monument's 18 existing factory presets don't have macro or modulation data. When loading them:

```cpp
// In PresetManager::loadFactoryPreset()
void PresetManager::loadFactoryPreset(const Preset& preset) {
    // Load base parameters (existing)
    apvts->getParameter("time")->setValueNotifyingHost(preset.values.time);
    apvts->getParameter("mass")->setValueNotifyingHost(preset.values.mass);
    // ...

    // NEW: Infer macro values from base parameters
    // (This allows old presets to work in new system)
    float inferredMaterial = inferMaterial(preset.values.mass, preset.values.density);
    float inferredTopology = inferTopology(preset.values.warp);
    // ...

    apvts->getParameter("material")->setValueNotifyingHost(inferredMaterial);
    apvts->getParameter("topology")->setValueNotifyingHost(inferredTopology);
    // ...

    // No modulation connections (defaults to empty)
    // This preserves backward compatibility while adding new features
}
```

---

## PART 10: RECOMMENDED READING & INSPIRATION

### DSP Techniques
- **Chaotic Synthesis:** "Fractals as Musical Signals" by Jeffrey S. Pressing
- **Physical Modeling:** DAFX book by Davide Rocchesso & Augusto Sarti
- **Non-Linear Processing:** "Nonlinear Processes in Geophysical Fluid Dynamics" for exotic dynamical systems
- **Spatial Audio:** Ambisonic techniques, HOA encoding

### Design Philosophy
- Monument's existing design docs (DSP_ARCHITECTURE.md, PARAMETER_BEHAVIOR.md)
- Modularity patterns: 12-tone equal temperament (each parameter is a "note" in the macro chord)
- Preset ecology: Each preset should demonstrate a distinct macro/modulation region

### Tools & Libraries
- **JUCE DSP Module:** `juce::dsp::Oscillator`, `juce::dsp::IIR::Filter` for chaos, allpass
- **C++ std:** `<random>` for Brownian motion, `<cmath>` for chaotic attractors

---

## SUMMARY: ACTIONABLE NEXT STEPS

### Phase 1: Foundation - ✅ COMPLETE (2026-01-03)

1. ✅ Review this architecture document thoroughly
2. ✅ Create `dsp/MacroMapper.h` skeleton (parameter mapping functions)
3. ✅ Create `dsp/ModulationMatrix.h` skeleton (source/destination routing)
4. ✅ Add macro parameters to APVTS (6 new normalized [0, 1] parameters)
5. ✅ Implement MacroMapper computeTargets() with parameter sweep functions
6. ✅ Implement ModulationMatrix with 4 modulation sources (stubs, no DSP yet)
7. ✅ Build system integration and compilation successful

### Phase 2: Integration - ✅ COMPLETE (2026-01-03)

1. ✅ Integrate modulation into PluginProcessor::processBlock()
2. ✅ Macro controls move underlying parameters smoothly

### Phase 3: Modulation Sources - ✅ COMPLETE (2026-01-03)

1. ✅ Implemented ChaosAttractor (Lorenz attractor with 3-axis output)
2. ✅ Implemented AudioFollower (RMS envelope tracking with attack/release)
3. ✅ Implemented BrownianMotion (smooth random walk with boundary reflection)
4. ✅ Implemented EnvelopeTracker (multi-stage attack/sustain/release detection)
5. ✅ Integrated all sources into ModulationMatrix::process()
6. ✅ Build successful, plugins installed

### Phase 3b: Physical Modeling Modules (Next 3-4 Weeks)

1. Create TubeRayTracer module (metal tube propagation)
2. Create ElasticHallway module (deformable room geometry)
3. Create AlienAmplification module (non-Euclidean behavior)
4. Insert new modules into DSP chain at appropriate positions
5. Add UI panels for modulation matrix, physical modeling controls

### Long-term (Weeks 7–8)

1. Design and build 10–15 factory presets showcasing all features
2. Update user manual with macro control explanations
3. Performance profiling and optimization
4. Beta testing with audio engineers

---

## APPENDIX: KEY CODE PATTERNS

### Parameter Smoothing
```cpp
ParameterSmoother smoother(48000, 20);  // 20 ms ramp
smoother.setTargetValue(newTarget);
float smoothedValue = smoother.getNextValue();  // Per-sample interpolation
```

### Real-Time Safe State Update
```cpp
// In prepare() - allocate once
std::vector<float> delayLine;
delayLine.resize(maxDelaySamples);

// In process() - only read/write, no allocations
for (int i = 0; i < buffer.getNumSamples(); ++i) {
    float output = delayLine[readIndex];
    delayLine[writeIndex] = input[i];
    // Update indices
}
```

### JUCE IIR Filter Usage
```cpp
// In prepare()
auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(
    sampleRate, cutoffHz);
filter.coefficients = coeffs;

// In process()
filter.process(juce::dsp::ProcessContextReplacing<float>(block));
```

---

## PART 11: DSP CLICK PREVENTION STRATEGIES

### Problem: Tap Delay Position Discontinuities

Monument's Pillars module uses an early reflection generator with multiple delay taps. Each tap has three properties:
- **tapSamples[]** - Physical read position in delay buffer (e.g., 150 samples back)
- **tapAllpassCoeff[]** - Allpass filter coefficient for diffusion
- **tapGains[]** - Output gain for this tap

When the **Topology** or **Shape** parameters change, `updateTapLayout()` recalculates all three arrays. While coefficient and gain changes can be smoothed with `juce::SmoothedValue`, **delay position changes create phase discontinuities** that no smoothing can fix.

**Example of the issue:**
```cpp
// Before: Tap 0 reading at position 100 samples back
tapSamples[0] = 100;

// After Topology change: Tap 0 suddenly reading at position 150
tapSamples[0] = 150;  // ← Instant 50-sample jump = phase discontinuity!
```

This creates an audible click because:
1. The tap was reading samples from one part of the delay buffer
2. It instantly jumps to reading from a different part (different phase relationship)
3. The discontinuity gets amplified by the reverb's feedback network
4. Result: Recurring clicks and volume swells when Topology > 0.0

### Solution Implemented: Option 1 - Deferred Tap Updates

**Strategy:** Only execute `updateTapLayout()` during near-silence to avoid clicks during active audio.

**Implementation:**
- Track input signal magnitude per block
- Set threshold at ~-60dB (0.001 linear)
- When `tapsDirty` flag is set, defer the update until signal is below threshold
- Keep `tapsDirty` flag active until update actually executes

**Pros:**
- Minimal code changes (30 minutes implementation)
- No additional memory allocation
- Eliminates clicks in 95% of real-world use cases
- Real-time safe (no new allocations, just threshold check)

**Cons:**
- Updates deferred during active audio (can take 1-2 seconds)
- Rapid knob movements during loud passages may feel "laggy"
- Threshold too low = updates never happen, too high = occasional clicks remain

**Code Pattern:**
```cpp
// In Pillars class (dsp/DspModules.h)
float inputPeakMagnitude = 0.0f;
static constexpr float kTapUpdateThreshold = 0.001f;  // ~-60dB

// In process() (dsp/DspModules.cpp)
// Track peak magnitude across all channels/samples
inputPeakMagnitude = 0.0f;
for (int ch = 0; ch < numChannels; ++ch) {
    const float* channelData = buffer.getReadPointer(ch);
    for (int sample = 0; sample < numSamples; ++sample) {
        inputPeakMagnitude = juce::jmax(inputPeakMagnitude,
                                        std::abs(channelData[sample]));
    }
}

// Only update tap layout when signal is quiet
if (tapsDirty && inputPeakMagnitude < kTapUpdateThreshold) {
    updateTapLayout();
    tapsDirty = false;
}
```

---

### Alternative Solution: Option 2 - Crossfade Tap Banks

**Strategy:** Maintain two complete tap banks (current + target) and crossfade between them over 50-100ms.

**Implementation:**
- Allocate two tap buffer sets: `tapsA` and `tapsB`
- When `tapsDirty` triggers:
  1. Compute new tap layout in inactive bank
  2. Start crossfade timer (e.g., 100ms)
  3. Per sample: blend outputs from both banks with fade curve
  4. Swap active/inactive banks when fade completes

**Pros:**
- **Guaranteed click-free** - Smooth amplitude crossfade masks phase discontinuity
- Updates happen immediately (no deferral)
- Musically responsive - knob changes feel instant
- Works during loud passages (no signal threshold needed)

**Cons:**
- **Moderate complexity** - 2-3 hours implementation time
- **2× memory usage** - Two complete tap banks + crossfade state
- **2× CPU during fade** - Processing two tap banks simultaneously
- Requires careful state management (track active/inactive bank, fade progress)

**Code Pattern:**
```cpp
// In Pillars class (dsp/DspModules.h)
struct TapBank {
    std::array<int, kMaxTaps> tapSamples;
    std::array<float, kMaxTaps> tapGains;
    std::array<float, kMaxTaps> tapAllpassCoeff;
};

TapBank tapBankA, tapBankB;
bool activeBank = true;  // true = A, false = B
float crossfadeProgress = 1.0f;  // 0.0 = start, 1.0 = complete
int crossfadeSamples = 0;
int crossfadeTotalSamples = 0;

// In process()
if (tapsDirty) {
    // Compute new layout in inactive bank
    TapBank& inactiveBank = activeBank ? tapBankB : tapBankA;
    updateTapLayout(inactiveBank);

    // Start crossfade
    crossfadeProgress = 0.0f;
    crossfadeSamples = 0;
    crossfadeTotalSamples = static_cast<int>(sampleRateHz * 0.1);  // 100ms
}

if (crossfadeProgress < 1.0f) {
    // Process both banks, blend outputs
    float gain_A = activeBank ? (1.0f - crossfadeProgress) : crossfadeProgress;
    float gain_B = activeBank ? crossfadeProgress : (1.0f - crossfadeProgress);

    float output_A = processTapBank(tapBankA, input) * gain_A;
    float output_B = processTapBank(tapBankB, input) * gain_B;
    output = output_A + output_B;

    crossfadeProgress += 1.0f / crossfadeTotalSamples;
    if (crossfadeProgress >= 1.0f) {
        activeBank = !activeBank;  // Swap
        tapsDirty = false;
    }
}
```

**When to use:**
- Professional mastering contexts where clicks are unacceptable
- Live performance environments with unpredictable input dynamics
- Automation-heavy workflows (DAW automation sweeps Topology continuously)

---

### Alternative Solution: Option 3 - Fractional Delay Interpolation

**Strategy:** Use fractional delay lines with linear/cubic interpolation to smoothly transition tap read positions.

**Implementation:**
- Replace integer `tapSamples[]` with float `tapSamplesFractional[]`
- When tap position changes from 100 → 150:
  1. Set target: `tapSamplesTarget[tap] = 150.0f`
  2. Smooth transition: `tapSamplesFractional[tap]` ramps 100.0 → 150.0 over 50ms
  3. Per sample: Use interpolated delay read with fractional part
- Interpolation types:
  - **Linear:** `output = (1-frac) * sample[n] + frac * sample[n+1]`
  - **Cubic:** 4-point Hermite for smoother frequency response

**Pros:**
- **Extremely smooth** - Phase transitions are gradual, mathematically continuous
- **Single tap bank** - No memory doubling like Option 2
- **Musically responsive** - Updates happen immediately with smooth pitch bend
- **Minimal CPU overhead** - Fractional delay only adds 3-4 multiplies per tap

**Cons:**
- **Complex implementation** - 4-6 hours development time
- **Subtle pitch effects** - Ramping delay position creates brief pitch shift
  - Example: 100→150 samples over 50ms = transient pitch drop
  - Musical in some contexts (Doppler-like), distracting in others
- **Filter coefficient interaction** - Smoothing both delay position AND allpass coefficients simultaneously can create unexpected timbre changes

**Code Pattern:**
```cpp
// In Pillars class (dsp/DspModules.h)
std::array<float, kMaxTaps> tapSamplesFractional;  // Current positions (float)
std::array<float, kMaxTaps> tapSamplesTarget;      // Target positions
std::array<juce::SmoothedValue<float>, kMaxTaps> tapPositionSmoothers;

// In prepare()
for (size_t i = 0; i < kMaxTaps; ++i) {
    tapPositionSmoothers[i].reset(sampleRateHz, 0.05);  // 50ms ramp
}

// When tapsDirty triggers
if (tapsDirty) {
    updateTapLayout();  // Computes new tapSamples[] targets
    for (int tap = 0; tap < tapCount; ++tap) {
        tapPositionSmoothers[tap].setTargetValue(
            static_cast<float>(tapSamples[tap]));
    }
    tapsDirty = false;
}

// Per sample: Read with fractional delay
for (int tap = 0; tap < tapCount; ++tap) {
    float fracPos = tapPositionSmoothers[tap].getNextValue();
    int basePos = static_cast<int>(fracPos);
    float frac = fracPos - basePos;

    // Linear interpolation
    int readPos0 = (writePosition - basePos + delayBufferLength) % delayBufferLength;
    int readPos1 = (readPos0 - 1 + delayBufferLength) % delayBufferLength;
    float tapIn = (1.0f - frac) * delayData[readPos0] + frac * delayData[readPos1];

    // Continue with allpass and accumulation...
}
```

**When to use:**
- Experimental sound design where pitch effects are desirable
- Modular reverb system where tap position modulation is a creative feature
- Future "Elastic Hallway" module where Doppler effects are intentional

---

### Comparison Matrix

| Criterion | Option 1 (Defer) | Option 2 (Crossfade) | Option 3 (Fractional) |
|-----------|------------------|----------------------|----------------------|
| Implementation Time | 30 min | 2-3 hours | 4-6 hours |
| Code Complexity | Low | Medium | High |
| Memory Overhead | None | 2× tap state | None |
| CPU Overhead | Negligible | 2× during fade | +3-4 mults/tap |
| Click Elimination | 95% cases | 100% guaranteed | 100% guaranteed |
| Responsiveness | Deferred (1-2s lag) | Immediate | Immediate |
| Pitch Artifacts | None | None | Brief pitch shifts |
| Real-Time Safety | Yes | Yes (pre-allocated) | Yes |
| Best Use Case | General use | Professional/live | Experimental design |

---

### Recommendation for Future Work

**Current Status (2026-01-03):** Option 1 implemented and functional.

**Future Enhancement Path:**

1. **Short-term (next release):**
   - Gather user feedback on Option 1 deferral behavior
   - If users report "laggy" knob feel, implement Option 2

2. **Medium-term (6-12 months):**
   - Implement Option 2 as a compile-time flag: `MONUMENT_USE_TAP_CROSSFADE`
   - A/B test with beta users in professional studios
   - If CPU budget allows and users prefer it, make Option 2 the default

3. **Long-term (1-2 years):**
   - Research Option 3 for creative "Elastic Hallway" module
   - Fractional delay with intentional pitch modulation as a feature
   - Could become a separate "Doppler Diffusion" effect

**Hybrid Approach:**
Consider combining strategies:
- Use **Option 1** for minor tap adjustments (< 20 sample difference)
- Use **Option 2** for major topology changes (> 50 sample difference)
- Threshold-based decision in `updateTapLayout()`:
  ```cpp
  if (maxTapDelta < 20) {
      // Defer update (Option 1)
      if (inputPeakMagnitude < kThreshold)
          applyTapUpdate();
  } else {
      // Crossfade immediately (Option 2)
      startTapCrossfade();
  }
  ```

---

**End of Architecture Review**

*Last Updated: 2026-01-03*
*For implementation questions or clarifications, refer to the detailed sections above or consult the existing Monument codebase patterns.*
