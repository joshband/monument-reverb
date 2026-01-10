# Monument-Reverb: Architecture Quick Reference
## Fast Navigation & Visual Summary

**Status:** Updated 2026-01-07. Modules listed below are implemented in the current codebase.

---

## THREE PILLARS OF INNOVATION

### 1️⃣ MACRO CONTROL SYSTEM
**Purpose:** High-level creative controls that morph multiple parameters musically

```
6 Macro Controls:
┌──────────────────────────────────────────┐
│ MATERIAL      (Soft ↔ Hard)              │
│ TOPOLOGY      (Regular ↔ Non-Euclidean)  │
│ VISCOSITY     (Airy ↔ Thick)             │
│ EVOLUTION     (Static ↔ Blooming)        │
│ CHAOS         (Stable ↔ Unstable)        │
│ ELASTICITY    (Instant ↔ Slow Recovery)  │
└──────────────────────────────────────────┘

Each macro drives multiple underlying parameters:
MATERIAL 0.7 → {Time: 0.69, Mass: 0.69, Density: 0.79, ...}
```

**Primary Files:**
- `dsp/MacroMapper.h/cpp`

**Integration Point:**
- `plugin/PluginProcessor.cpp` - Once per block before smoothing

---

### 2️⃣ MODULATION MATRIX
**Purpose:** Multiple modulation sources → any parameter destination (64+ connections)

```
4 Modulation Sources:
┌─────────────────────────────────────────┐
│ • Chaos Attractor (Lorenz/Rössler)      │
│ • Audio Follower (input-reactive)       │
│ • Brownian Motion (smooth random)       │
│ • Envelope Tracker (multi-stage)        │
└─────────────────────────────────────────┘

16 Parameter Destinations:
{Time, Mass, Density, Bloom, Air, Width, Warp, Drift,
 Gravity, Tube params, Elastic params, ...}

Example Connections:
Chaos (axis 0) → Warp (depth 0.3)
Audio Follower → Mass (depth 0.15)
Brownian → Bloom (depth 0.2)
```

**Primary Files:**
- `dsp/ModulationMatrix.h/cpp`
- `dsp/ChaosAttractor.h/cpp`
- `dsp/AudioFollower.h/cpp`
- `dsp/BrownianMotion.h/cpp`
- `dsp/EnvelopeTracker.h/cpp`

**Integration Point:**
- `plugin/PluginProcessor.cpp` - Once per block after macros

---

### 3️⃣ PHYSICAL/ALGORITHMIC MODULES
**Purpose:** New DSP modules for tube modeling, elastic spaces, and alien acoustics

#### A. TubeRayTracer
```
Input → [Ray Propagation] → Metal Tube Coloration → Output
         (8-16 virtual tubes with modal resonances)

Features:
✓ Distance-based absorption (high-freq rolloff)
✓ Tube modal frequencies (Helmholtz resonance)
✓ Inter-tube coupling (energy transfer)
```

**Position in Chain:** Between Chambers & Weathering
**Files:** `dsp/TubeRayTracer.h/cpp`

---

#### B. ElasticHallway
```
Chambers Output → [Elastic Deformation] → Modulated Chambers State
                   (walls respond to pressure)

Features:
✓ Room geometry (width, height, depth)
✓ Elastic walls (deform under acoustic pressure)
✓ Slow recovery (creates evolving timbre)
✓ Non-linear reflections (energy-dependent)
```

**Position in Chain:** Wraps/couples Chambers (modulates delays & matrix)
**Files:** `dsp/ElasticHallway.h/cpp`

---

#### C. AlienAmplification
```
[FDN Output] → [Pitch Evolution] → [Paradox Resonance] → [Non-Local Absorption]
                (frequency shifts)   (impossible rule-breaking)  (custom curves)

Features:
✓ Pitch evolution (frequency morphs with age)
✓ Paradox resonance (frequency that amplifies, not decays)
✓ Impossible geometry (violation of normal acoustics)
```

**Position in Chain:** Post-Chambers (wraps output)
**Files:** `dsp/AlienAmplification.h/cpp`

---

## SIGNAL FLOW (WITH NEW ADDITIONS)

```
INPUT
  ↓
[Foundation]
  ↓
[Pillars]
  ↓
[Chambers (FDN)]  ← ElasticHallway couples to this
  ↓
[TubeRayTracer]  ← NEW
  ↓
[AlienAmplification]  ← NEW
  ↓
[Weathering]
  ↓
[Buttress]
  ↓
[Facade]
  ↓
[MemoryEchoes]
  ↓
[Mix] → OUTPUT

CONTROL LAYERS (run once per block):
┌─────────────────────────────────────┐
│ Host Automation / APVTS             │ ← User moves sliders
│        ↓                            │
│ MacroMapper                         │ ← Macros drive parameters
│        ↓                            │
│ ModulationMatrix                    │ ← Modulation adds motion
│        ↓                            │
│ ParameterSmoother                   │ ← Smoothing prevents zipper noise
│        ↓                            │
│ DSP Modules (read smoothed values)  │ ← Audio processing
└─────────────────────────────────────┘
```

---

## PARAMETER MAPPING PHILOSOPHY

**Example: MATERIAL Macro**

```cpp
MATERIAL = 0.0 (Soft)
  ├─ Mass: 0.2 (light damping)
  ├─ Density: 0.3 (sparse early reflections)
  ├─ Metallicness: 0.0 (no metallic coloration)
  └─ Tube Absorption: 0.3 (absorbs less)

MATERIAL = 0.5 (Neutral)
  ├─ Mass: 0.55 (moderate damping)
  ├─ Density: 0.65 (balanced density)
  ├─ Metallicness: 0.5 (mixed)
  └─ Tube Absorption: 0.65 (balanced)

MATERIAL = 1.0 (Hard)
  ├─ Mass: 0.9 (heavy damping)
  ├─ Density: 1.0 (maximum density)
  ├─ Metallicness: 1.0 (strong metallic peaks)
  └─ Tube Absorption: 1.0 (highly absorptive)
```

**Pattern:** All parameters normalized [0, 1] → meaningful ranges via mapping functions

---

## FILE STRUCTURE (MODIFIED & NEW)

### Existing Files (Modify)
```
plugin/
  ├─ PluginProcessor.h/cpp     ← Add macro params, modulation integration
  ├─ PluginEditor.h/cpp        ← Add macro UI panels
  └─ PresetManager.h/cpp       ← Extend JSON with macro/modulation data

dsp/
  ├─ Chambers.h/cpp            ← Couple to ElasticHallway (delay modulation)
  ├─ MemoryEchoes.h/cpp        ← Track macro state for memory evolution
  └─ ParameterSmoother.h/cpp   ← Add per-destination lag control
```

### New Files (Create)
```
dsp/
  ├─ MacroMapper.h/cpp         ← Macro → parameter mapping
  ├─ ModulationMatrix.h/cpp    ← Central modulation hub
  ├─ ChaosAttractor.h/cpp      ← Lorenz/Rössler chaos
  ├─ AudioFollower.h/cpp       ← Input envelope detection
  ├─ BrownianMotion.h/cpp      ← Smooth random walks
  ├─ EnvelopeTracker.h/cpp     ← Multi-stage envelope
  ├─ TubeRayTracer.h/cpp       ← Metal tube modeling
  ├─ ElasticHallway.h/cpp      ← Elastic room geometry
  └─ AlienAmplification.h/cpp  ← Non-Euclidean coloration
```

---

## REAL-TIME SAFETY CHECKLIST

✅ **DO:**
- Pre-allocate all buffers in `prepare()`
- Use `juce::ScopedNoDenormals` in `process()`
- Read parameters lock-free from APVTS
- Use exponential averaging (1-pole) filters
- Keep state updates per-block (not per-sample) for modulation
- Clamp all feedback coefficients < 1.0
- Use lookup tables for chaos, tube modes, absorption curves

❌ **DON'T:**
- Use `new`/`delete` in `process()`
- Lock mutexes in audio thread
- Log or print in `process()`
- Use `juce::Random` without seeding in `prepare()`
- Allow denormal numbers (use `std::abs(x) < 1e-30f ? 0.0f : x`)
- Modulate feedback > 0.995 (stability margin)

---

## CPU BUDGET

| Component | Target | Notes |
|-----------|--------|-------|
| Macro system | <0.1% | Just mapping |
| Modulation matrix | 0.3–0.5% | 4 sources × 16 dests |
| ChaosAttractor | 0.2–0.4% | LUT-based |
| AudioFollower | 0.1% | One-pole RMS |
| BrownianMotion | 0.1% | PRNG |
| EnvelopeTracker | 0.1% | Envelope detection |
| TubeRayTracer | 1–2% | Block-rate ray tracing |
| ElasticHallway | 0.5–1% | Modal filter bank |
| AlienAmplification | 0.7–1% | Allpass + biquad |
| **TOTAL** | **3–5%** | All features enabled |

---

## PRESET FORMAT (NEW STRUCTURE)

```json
{
  "name": "Crystalline Bloom",
  "macros": {
    "material": 0.85,
    "topology": 0.4,
    "viscosity": 0.3,
    "evolution": 0.75,
    "chaosIntensity": 0.2,
    "elasticityDecay": 0.6
  },
  "physicalModeling": {
    "tubeCount": 10,
    "metallicResonance": 0.8,
    "couplingStrength": 0.5
  },
  "elasticHallway": {
    "elasticity": 0.7,
    "elasticityRecoveryMs": 2500
  },
  "alienAtmospheres": {
    "impossibilityDegree": 0.4,
    "pitchEvolutionRate": 0.6,
    "paradoxGain": 1.03
  },
  "modulation": [
    {
      "source": "chaos",
      "destination": "warp",
      "depth": 0.3,
      "smoothingMs": 200
    }
  ]
}
```

---

## IMPLEMENTATION PHASES

### Phase 1: Foundation (1–2 weeks)
✓ MacroMapper
✓ ModulationMatrix (empty)
✓ Add 6 macro parameters to APVTS
✓ Wire macros into processor block
**Goal:** Macros control underlying parameters

### Phase 2: Modulation Sources (2–3 weeks)
✓ ChaosAttractor
✓ AudioFollower, BrownianMotion, EnvelopeTracker
✓ Integrate into ModulationMatrix
**Goal:** Parameters evolve musically

### Phase 3: Physical Modules (3–4 weeks)
✓ TubeRayTracer
✓ ElasticHallway
✓ AlienAmplification
✓ Insert into DSP chain
**Goal:** New sonic possibilities

### Phase 4: Integration & Polish (2–3 weeks)
✓ UI panels for all features
✓ 10–15 factory presets
✓ Documentation & testing
**Goal:** Production-ready features

---

## KEY INTEGRATION POINTS

### 1. PluginProcessor::processBlock()
```cpp
// Order of operations (once per block):
macroMapper.setInput(getRawMacroValues());
auto targets = macroMapper.computeTargets();  // Step 1

modulationMatrix.process(numSamples);         // Step 2
auto mods = modulationMatrix.getOutputs();

smoothers.update(targets, mods);              // Step 3
// (smoothers interpolate per-sample in loop below)

// Then process audio:
for (int sample = 0; sample < numSamples; ++sample) {
    chambers.setFeedback(smoothers[TIME].getNextValue());
    chambers.process(...);
    // etc.
}
```

### 2. Chambers ↔ ElasticHallway Coupling
```cpp
// In Chambers::process():
float pressure = calculateInternalPressure();
elasticHallway.setPressure(pressure);

// ElasticHallway returns modified delay times:
auto modifiedDelays = elasticHallway.getModifiedDelays();
delayLine.setDelayTime(modifiedDelays[line]);
```

### 3. Memory Integration
```cpp
// In MemoryEchoes::surfaceMemory():
// When recalling fragment:
float macroBlend = 1.0f - (age / maxAge);  // Older = more current
float recalledMaterial =
    capturedMaterial * (1.0f - macroBlend) +
    currentMaterial * macroBlend;
// Memories morph into current space
```

---

## TESTING STRATEGY

### Unit Tests
- Each module processes without artifacts
- Parameters within bounds
- Chaos produces non-repeating sequences
- Macros map correctly
- Audio-follower responds to input

### Integration Tests
- Full chain processes without distortion
- All features enabled simultaneously work
- No parameter fighting (conflicts)
- Real-time safe (no allocations, locks)
- Performance meets budgets

### Listening Tests
- Macro morphing is musically coherent
- Modulation sources sound natural
- Physical models add expected coloration
- Alien features are creative, not broken

---

## REFERENCE: EXISTING MONUMENTS

**Current 18 Factory Presets:**
- Foundational Spaces (6): Init Patch, Stone Hall, High Vault, Cold Chamber, Night Atrium, Monumental Void
- Living Spaces (6): Stone Circles, Cathedral of Glass, Zero-G Garden, Weathered Nave, Dust in the Columns, Frozen Monument
- Remembering Spaces (3): Ruined Monument, What the Hall Kept, Event Horizon
- Time-Bent/Abstract (3): Folded Atrium, Hall of Mirrors, Tesseract Chamber

**New Preset Ideas** (leveraging macros):
- Crystalline Bloom (hard + elastic + pitch evolution)
- Chaotic Void (unstable + non-Euclidean)
- Audio-Reactive Cathedral (follower-driven parameter shifts)
- Slow Tectonic Plates (brownian + elasticity)
- Paradox Machine (paradox resonance + chaos)

---

## QUICK START: WHERE TO BEGIN

**If you want to understand the current code:**
1. Read `/home/user/monument-reverb/dsp/Chambers.cpp` (FDN core)
2. Review `/home/user/monument-reverb/plugin/PluginProcessor.cpp` (signal routing)
3. Explore `/home/user/monument-reverb/plugin/PresetManager.cpp` (parameter mapping)

**If you want to start implementing:**
1. Create `dsp/MacroMapper.h` with mapping functions
2. Add 6 macro parameters to APVTS in `PluginProcessor.h`
3. Wire macro computation into `processBlock()`
4. Test macro values move underlying parameters

**If you have questions:**
- See ARCHITECTURE_REVIEW.md (detailed, comprehensive)
- Check DSP_ARCHITECTURE.md (existing system documentation)
- Review PARAMETER_BEHAVIOR.md (current parameter mappings)

---

**Quick Ref Version:** 2026-01-03
**Full Review:** See ARCHITECTURE_REVIEW.md
**Current Status:** Ready for Phase 1 implementation
