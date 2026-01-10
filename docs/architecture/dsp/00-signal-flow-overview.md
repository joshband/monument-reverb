# Signal Flow Overview: Monument Architecture

**Document Version:** 1.0
**Last Updated:** 2026-01-09

---

## Introduction

This document provides a **high-level overview** of Monument Reverb's DSP architecture. Think of it as the **architectural blueprint** - showing how all modules connect, where signal flows, and how routing modes create sonic diversity.

**For detailed module documentation, see:**
- [Core Modules](core-modules/) - Individual module deep-dives
- [Physical Modeling](physical-modeling/) - Novel algorithms
- [Memory System](memory-system/11-strata.md) - Strata (temporal feedback)

---

## The Complete Monument

Monument consists of **11 DSP modules** organized into 4 categories:

### Module Categories

| Category | Modules | Count | Purpose |
|----------|---------|-------|---------|
| **Core** | Foundation, Pillars, Chambers, Weathering, Buttress, Facade | 6 | Main signal path |
| **Orchestration** | Routing Graph | 1 | Signal routing + coordination |
| **Physical Modeling** | Resonance, Living Stone, Impossible Geometry | 3 | Novel acoustics |
| **Memory** | Strata | 1 | Temporal feedback |

**Total:** 11 modules (10 processing + 1 orchestration)

---

## Default Signal Flow: AncientWay Mode

Monument's **default routing mode** (AncientWay) follows a traditional serial chain:

```
Audio Input
    ↓
┌─────────────────────────────────────────────────────────────┐
│  1. FOUNDATION                                              │
│     DC blocker + input gain                                 │
│     "Sound enters through heavy stone doors"                │
└────────────────────────────┬────────────────────────────────┘
                             ↓
┌─────────────────────────────────────────────────────────────┐
│  2. PILLARS                                                 │
│     32-tap early reflections (Glass/Stone/Fog modes)        │
│     "Sound scatters off vertical columns"                   │
└────────────────────────────┬────────────────────────────────┘
                             ↓
┌─────────────────────────────────────────────────────────────┐
│  3. CHAMBERS                                                │
│     8×8 FDN reverb core (time, mass, density, bloom)        │
│     "The heart - 8 vaulted stone rooms"                     │
│     ┌──────────────────────────────────────────┐           │
│     │  Wet Output → Strata Capture (optional)  │           │
│     └──────────────────────────────────────────┘           │
└────────────────────────────┬────────────────────────────────┘
                             ↓
┌─────────────────────────────────────────────────────────────┐
│  4. WEATHERING                                              │
│     LFO modulation (warp, drift)                            │
│     "Erosion over geological time"                          │
└────────────────────────────┬────────────────────────────────┘
                             ↓
┌─────────────────────────────────────────────────────────────┐
│  5. RESONANCE (TubeRayTracer)                               │
│     Metallic tube resonances (16 tubes, modal filtering)    │
│     "Metal pipes embedded in stone"                         │
└────────────────────────────┬────────────────────────────────┘
                             ↓
┌─────────────────────────────────────────────────────────────┐
│  6. LIVING STONE (ElasticHallway)                           │
│     Deformable walls (elastic response to pressure)         │
│     "Stone that breathes with sound"                        │
└────────────────────────────┬────────────────────────────────┘
                             ↓
┌─────────────────────────────────────────────────────────────┐
│  7. IMPOSSIBLE GEOMETRY (AlienAmplification)                │
│     Non-Euclidean acoustics (pitch evolution, paradox)      │
│     "Space where physics breaks down"                       │
└────────────────────────────┬────────────────────────────────┘
                             ↓
┌─────────────────────────────────────────────────────────────┐
│  8. BUTTRESS                                                │
│     Drive saturation + freeze (prevents feedback collapse)  │
│     "Structural support holds the monument together"        │
└────────────────────────────┬────────────────────────────────┘
                             ↓
┌─────────────────────────────────────────────────────────────┐
│  9. FACADE                                                  │
│     Stereo width, air (brightness), wet/dry mix             │
│     "The external face of the monument"                     │
└────────────────────────────┬────────────────────────────────┘
                             ↓
Audio Output
```

**Total Latency:** Variable (depends on Chambers time parameter: 10-1000ms)
**Total CPU:** ~22% (p99, 48kHz, 512 samples)

---

## Strata (Memory System) Integration

**Strata** operates **outside the main signal flow** as a parallel capture/recall system:

```
Main Signal Flow:
Foundation → Pillars → Chambers → Weathering → [Physical Modeling] → Buttress → Facade
                          ↓
                    [Wet Output]
                          ↓
              ┌─────────────────────┐
              │  STRATA (Capture)   │
              │  Short: 2-4s        │
              │  Long: 20-60s       │
              └──────────┬──────────┘
                         ↓
                    [Recall]
                         ↓
              ┌─────────────────────┐
              │  Inject back into   │
              │  pre-Chambers buffer│
              └─────────────────────┘
                         ↑
                         └──────────────────[Feedback Loop]
```

**Key Points:**
- Strata captures **post-Chambers wet output**
- Stores in dual buffers (short-term + long-term memory)
- Recalls randomly with fade-in/hold/fade-out envelopes
- Optionally injects recalled audio back **before Chambers** (temporal feedback)

**Result:** Sound from the past can re-enter the reverb, creating cascading echoes and infinite memory loops.

---

## The Three Routing Modes

Monument's **Routing Graph** supports **3 core topologies** for dramatic sonic diversity:

### Mode 1: AncientWay (Traditional - Default)

**Signal Flow:**
```
Foundation → Pillars → Chambers → Weathering → [Physical] → Buttress → Facade
```

**Character:**
- Classic reverb with physical modeling enhancements
- Smooth, predictable progression
- Best for traditional reverb tails

**Use Cases:**
- Mixing (standard reverb sends)
- Ambient pads (lush, evolving tails)
- Vocal reverb (clear, focused)

**CPU:** ~22% (p99)

---

### Mode 2: ResonantHalls (Metallic First)

**Signal Flow:**
```
Foundation → Pillars → Resonance → Chambers → Weathering → [Elastic/Alien] → Buttress → Facade
```

**Key Difference:** **Resonance BEFORE Chambers**

**Effect:**
- Bright metallic tube resonances **feed into** reverb diffusion
- Modal frequencies become part of reverb character
- Tighter, more focused coloration

**Character:**
- Bright, metallic early reflections
- Tube resonances preserved through reverb tail
- More articulate, less diffuse

**Use Cases:**
- Drums (metallic punch)
- Synths (bell-like tones)
- Sound design (sci-fi environments)

**CPU:** ~22% (similar to AncientWay)

---

### Mode 3: BreathingStone (Elastic Core)

**Signal Flow:**
```
Foundation → Pillars → Living Stone → Chambers → Living Stone → Weathering → [Alien/Resonance] → Buttress → Facade
```

**Key Difference:** **Chambers sandwiched between Living Stone (elastic walls)**

**Effect:**
- First elastic pass: Walls deform from dry signal
- Chambers: Reverb develops inside deformed space
- Second elastic pass: Walls respond to reverb energy
- Feedback creates **organic breathing** motion

**Safety:**
- Soft clip (tanh 0.7) **before** Chambers
- 0.95× gain reduction **after** first elastic pass
- Prevents runaway energy buildup

**Character:**
- Slow, breathing reverb (inhale/exhale rhythm)
- Walls "push back" against sound
- Impossible physics (walls responding to acoustics)

**Use Cases:**
- Ambient drones (organic evolution)
- Experimental textures (unstable reverb)
- Cinematic sound design (breathing spaces)

**CPU:** ~25% (elastic modal filtering adds ~3%)

---

## Advanced Routing: Parallel and Feedback Modes

Beyond the 3 core presets, Monument supports **custom routing** with advanced topologies:

### Parallel Processing

```
                    ┌─→ Chambers ─→┐
Foundation → Pillars ├─→ Resonance ─→├→ Blend → Facade
                    └─→ Living Stone→┘
```

**Effect:** All 3 processors run on **dry signal**, outputs blended

**Use Case:** Dense, layered textures with multiple reverb characters

---

### Feedback Loops

```
Foundation → Chambers ⟲ [Strata Feedback] → Facade
```

**Effect:** Strata recalls memory and injects back into Chambers input

**Use Case:** Infinite regression, cascading echoes, self-reinforcing reverb

**Safety:** Strata gain limited to prevent runaway feedback

---

### Crossfeed

```
Foundation → Pillars → [L→R, R→L Swap] → Chambers → Facade
```

**Effect:** Left channel processes through right, vice versa

**Use Case:** Stereo widening, spatial confusion, psychoacoustic effects

---

## Module Interconnections

### Data Flow Between Modules

**Parameter Automation:**
- **Per-Sample:** Chambers (time, mass, density, bloom, gravity), Pillars (shape)
- **Block-Rate:** Facade (air, width), Physical Modeling (all params)
- **Smoothing:** ParameterSmoother (block-rate), ParameterBuffer (per-sample)

**Spatial Processing:**
- Chambers → SpatialProcessor → 3D positioning (azimuth/elevation)
- Facade → 3D panning mode (constant-power law)

**Modulation:**
- ModulationMatrix → Routes 4 sources to 25 destinations
- Weathering LFO → Can modulate any parameter
- AudioFollower → Envelope-driven modulation

**Memory:**
- Chambers wet output → Strata capture
- Strata recall → Pre-Chambers injection (optional)

---

## CPU Budget Breakdown

### Per-Module CPU Usage (p99, 48kHz, 512 samples)

| Module | CPU % | Category | Notes |
|--------|-------|----------|-------|
| Chambers | 7.22% | Core | **31% improvement** from Phase 4 |
| Pillars | 5.38% | Core | 4% improvement from Phase 4 |
| Impossible Geometry | 4.05% | Physical | Allpass cascade (8 bands × 8 filters) |
| Living Stone | 3.38% | Physical | Modal filtering (8 room modes) |
| Facade | 0.80% | Core | M/S processing + EQ |
| Weathering | 0.50% | Core | LFO + delay modulation |
| Buttress | 0.20% | Core | Soft clipping |
| Strata | 0.15% | Memory | Dual-buffer capture/recall |
| Foundation | 0.10% | Core | DC blocker + gain |
| Resonance | 0.03% | Physical | **Block-rate** ray tracing |
| Routing Graph | 0.50% | Orchestration | Module coordination |

**Total:** ~22.3% (73% of 30% budget)
**Headroom:** ~7.7% (27% remaining)

---

## Memory Footprint

### Per-Module Memory Usage

| Module | Memory | Components |
|--------|--------|------------|
| Chambers | 128 KB | 8 delay lines + 8 allpass state vectors |
| Strata | 4-60 MB | Dual buffers (configurable: 2-4s + 20-60s) |
| Pillars | 32 KB | 32-tap delay buffer + allpass state |
| Weathering | 16 KB | Modulation delay buffer |
| ParameterBuffers | 64 KB | Per-sample automation pool |
| Physical Modeling | 20 KB | Modal filter state (all 3 modules) |
| ModulationMatrix | 20 KB | 4 sources × 25 destinations |
| Other | 50 KB | Misc state, routing graph, smoothers |

**Total (excluding Strata):** ~350 KB
**Total (with Strata):** ~5-60 MB (depends on buffer size configuration)

---

## Latency Considerations

### Module-Level Latency

**Zero-Latency Modules:**
- Foundation (DC blocker is IIR, no group delay)
- Buttress (soft clipper is instantaneous)
- Facade (EQ is IIR, no group delay)

**Variable-Latency Modules:**
- **Pillars:** 0-50ms (depends on tap layout)
- **Chambers:** 10-1000ms (depends on time parameter)
- **Weathering:** 5-20ms (modulation delay)
- **Strata:** 2-60 seconds (memory buffers)

**Fixed-Latency Modules:**
- Resonance: <1ms (modal filters)
- Living Stone: <1ms (modal filters)
- Impossible Geometry: <1ms (allpass cascade)

**Total Plugin Latency:** Variable, **not compensated**

**Why?** Monument is designed for **creative reverb**, not transparent mixing. Users expect reverb tails to delay the signal.

---

## Real-Time Safety

All Monument modules follow **strict real-time audio rules**:

### ✅ Safe in processBlock()

- **No allocations** - All buffers pre-allocated in prepareToPlay()
- **No locks** - Lock-free atomics for parameter updates
- **No system calls** - No file I/O, logging, or network
- **Bounded operations** - No unbounded loops or recursion

### ⚠️ NOT Safe (Off-Audio-Thread Only)

- `Pillars::loadImpulseResponse()` - File I/O, heap allocation
- Preset loading - Parameter updates, state changes
- UI interactions - APVTS parameter changes (atomic, safe)

### Safety Verification

All modules tested with:
- **Thread sanitizer** (no data races)
- **Address sanitizer** (no memory leaks)
- **UBSan** (no undefined behavior)
- **Realtime checks** (no allocations in process())

---

## Modularity and Extension Points

### Adding New Modules

Monument's architecture supports easy extension:

**1. Create Module Class**
```cpp
class NewModule : public DSPModule
{
    void prepare(double sampleRate, int blockSize, int numChannels) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer) override;
};
```

**2. Add to Routing Graph**
```cpp
enum class ModuleType
{
    // ... existing ...
    NewModule,
    Count
};
```

**3. Register in PluginProcessor**
```cpp
newModule = std::make_unique<NewModule>();
routingGraph.addModule(ModuleType::NewModule, newModule.get());
```

**Extension Points:**
- Custom routing presets
- Additional physical modeling algorithms
- Alternative diffusion networks
- Novel modulation sources

---

## Future Architecture Evolution

### Phase 6: Strata Integration (Planned)

**Current State:** Strata hardcoded after Chambers

**Future State:** Strata as 11th routable module

**New Routing Presets:**
1. **Ghostly Cathedral** - Strata AFTER Chambers (current position)
2. **Fragmented Reality** - Strata BEFORE Chambers (memory-driven early reflections)
3. **Recursive Haunt** - Strata in feedback loop (infinite regression)
4. **Metallic Memory** - Strata after Resonance (tube resonance memories)

**Benefits:**
- Strata becomes core feature (not bolt-on)
- 4+ new routing presets
- Creative sound design possibilities

---

### SIMD Vectorization (Planned)

**Current State:** Scalar processing (single sample at a time)

**Future State:** SIMD-accelerated (4-8 samples in parallel)

**Target Modules:**
- Chambers (allpass filters, matrix multiply)
- Pillars (tap processing)
- Living Stone (modal filtering)

**Expected Improvement:** 2-4× speedup (from ~22% → ~8-12% CPU)

**Status:** SimdHelpers.h written but not yet applied

---

## Summary

Monument Reverb's architecture is:

**Modular** - 11 independent DSP modules
**Flexible** - 3+ routing modes for sonic diversity
**Performant** - 22% CPU, 57% headroom from 30% budget
**Real-time safe** - No allocations, locks, or system calls in processBlock()
**Thematically coherent** - Every module fits the stone/architecture narrative

**Next Steps:**
- Read individual [Module Documentation](core-modules/) for deep-dives
- Explore [Physical Modeling](physical-modeling/) algorithms
- Understand [Strata](memory-system/11-strata.md) memory system

---

## Further Reading

- [Monument Theme](00-monument-theme.md) - Architectural metaphor and design philosophy
- [Chambers](core-modules/03-chambers.md) - The heart of Monument (FDN reverb core)
- [Routing Graph](core-modules/07-routing-graph.md) - Signal routing and orchestration
- Project root `README.md` - Monument philosophy and goals

---

**Next:** [Chambers Documentation](core-modules/03-chambers.md) - Deep-dive into the FDN reverb core
