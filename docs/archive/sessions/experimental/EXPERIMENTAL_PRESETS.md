# Monument Reverb - Experimental Otherworldly Presets

**Created:** 2026-01-09
**Purpose:** Push Monument Reverb into impossible territory with long tails, wild spatial manipulation, and dynamic evolution

---

## Overview

These 5 experimental presets use Monument's **SequenceScheduler** timeline automation system to create evolving, otherworldly reverb spaces that are physically impossible but sonically fascinating. They leverage:

- **Extreme parameter ranges** (Time=1.0, Drift=1.0, ultra-sparse density)
- **Timeline-based automation** (parameters morph over 32-120 seconds)
- **Spatial manipulation** (3D position jumps, quantum tunneling effects)
- **Multi-dimensional modulation** (all parameters evolving simultaneously)

---

## Preset 4: Infinite Abyss 🕳️

**Concept:** Bottomless pit with eternal memory feedback

### Technical Characteristics

```
Timeline: 64 beats (tempo-synced), Loop
RT60: Expected >30s (infinite decay feel)
```

### Parameter Evolution

| Beat Range | Time | Mass | Density | Bloom | Gravity | Memory | MemoryDepth | MemoryDrift | Effect |
|------------|------|------|---------|-------|---------|--------|-------------|-------------|--------|
| 0-16 | 1.0 | 0.9 | 0.85 | 0.8 | 0.3→0.1 | 0.8 | 0.7→0.85 | 0.3 | Falling sensation with eternal feedback |
| 16-32 | — | — | — | — | 0.1→0.5 | — | 0.85→0.65 | — | Floor becomes unstable |
| 32-48 | — | — | — | — | 0.5→0.2 | — | 0.65 | 0.3→0.5 | Deeper descent, memory drifts |
| 48-64 | — | — | — | — | 0.2→0.3 | — | 0.65→0.7 | 0.5→0.3 | Loop point (eternal fall) |

### Key Features

- **Maximum decay** (Time=1.0) creates endless tail
- **Ultra-heavy mass** (Mass=0.9) for slow, ponderous response
- **Gravity oscillation** creates unstable floor sensation
- **Dense reflections** (Density=0.85, Bloom=0.8) fill the void
- **Memory System** (Memory=0.8, MemoryDecay=0.9) enables true eternal feedback
- **MemoryDepth modulation** (0.7→0.85→0.65→0.7) creates breathing feedback intensity
- **MemoryDrift evolution** (0.3→0.5→0.3) adds organic pitch aging

### Sonic Character

- Heavy, oppressive reverb that never truly ends
- **Cascading recursive echoes** from Memory system (near-infinite feedback)
- Gravity modulation creates breathing/pulsing sensation
- **Organic pitch aging** from MemoryDrift creates evolving tonal character
- Perfect for ambient, dark soundscapes, and horror scores
- **Expected RT60:** >30 seconds (measurement limited by capture duration)

### Use Cases

- Drone music and dark ambient
- Horror film sound design
- Building tension in cinematic contexts
- Deep, evolving pad sounds

---

## Preset 5: Quantum Tunneling  ⚛️

**Concept:** Sound teleports through impossible geometry

### Technical Characteristics

```
Timeline: 32 beats (tempo-synced), Loop
Spatial Movement: 8 instantaneous jumps per cycle
RT60: Expected ~15-20s with spatial artifacts
```

### Parameter Set

| Parameter | Value | Purpose |
|-----------|-------|---------|
| Time | 0.85 | Long decay for spatial trails |
| Density | 0.15 | **Ultra-sparse** (crystalline) |
| Bloom | 0.9 | Maximum diffusion for artifacts |
| Warp | 1.0 | **Maximum spatial distortion** |
| Drift | 0.8 | High pitch shifting |
| Mass | 0.3 | Light, ethereal |

### Spatial Path (8 Quantum Jumps)

Uses **Step interpolation** (instant jumps, no smoothing):

```
PositionX = 0.5 + 0.4·cos(phase × 3.0)
PositionY = 0.5 + 0.4·sin(phase × 2.0)
PositionZ = 0.5 + 0.3·sin(phase × 5.0)
VelocityX = 0.5 + 0.4·cos(phase × 7.0)
```

- **Phase** = (jump_index / 8) × 2π
- **Different frequencies** create complex Lissajous-like 3D path
- **Velocity spikes** create Doppler shifts at each jump

### Key Features

- **Instantaneous position jumps** every 4 beats (quantum tunneling effect)
- **Maximum warp + drift** creates extreme spatial distortion
- **Sparse density** with high bloom creates metallic, glassy artifacts
- **Doppler shifts** from velocity changes add motion realism

### Sonic Character

- Glitchy, teleporting reverb trails
- Metallic/crystalline artifacts from sparse density
- Unpredictable spatial movement
- Wild pitch shifting from extreme drift

### Use Cases

- Sci-fi sound design
- Glitch music and experimental electronic
- Transition effects
- Creating sense of impossible space

---

## Preset 6: Time Dissolution ⏳

**Concept:** Time itself becomes unstable

### Technical Characteristics

```
Timeline: 120 seconds (free-running), Loop
RT60: Highly variable (6-30s depending on time point)
```

### Parameter Evolution

| Time (s) | Time | Mass | Drift | Warp | Effect |
|----------|------|------|-------|------|--------|
| 0-30 | 0.9 | 0.1 | 0.5→1.0 | 0.0 | Time accelerates, pitch instability begins |
| 30-60 | 0.6 | 0.1 | 1.0 | 0.0 | **Time speeds up**, maximum drift chaos |
| 60-90 | 1.0 | 0.1 | 0.8 | 0.0→0.5 | Time slows to crawl, shimmer appears |
| 90-120 | 0.8 | 0.1 | 0.4 | 0.2→0.0 | Returning to stability (loop) |

### Key Features

- **Weightless** (Mass=0.1) for ethereal, unstable quality
- **Extreme drift modulation** (0.5→1.0→0.8) creates wildly shifting pitch
- **Time parameter varies** (0.6→1.0) making decay rate unstable
- **2-minute slow evolution** for organic, unpredictable changes

### Sonic Character

- Reverb decay rate constantly changing
- Extreme pitch instability (like tape running at variable speed)
- Shimmer effects appear and disappear
- Never sounds the same twice in a cycle

### Use Cases

- Experimental ambient music
- Psychedelic/trippy sound design
- Creating sense of temporal distortion
- Sci-fi time manipulation effects

---

## Preset 7: Crystalline Void 💎

**Concept:** Ultra-sparse delay taps create metallic resonances

### Technical Characteristics

```
Timeline: 48 beats (tempo-synced), Loop
Topology Modulation: 12 keyframes per cycle
RT60: Expected ~20s with crystalline ringing
```

### Parameter Set

| Parameter | Value | Purpose |
|-----------|-------|---------|
| Time | 0.9 | Long decay |
| Mass | 0.85 | **Heavy, metallic** |
| Density | 0.05 | **Ultra-sparse** (glass-like) |
| Bloom | 0.95 | Maximum diffusion |
| Gravity | 0.6 | Moderate damping |

### Topology Evolution (12-Step Wave)

```
Topology = 0.7 + 0.25·sin(phase)
Phase = (step / 12) × 2π
```

- **Topology modulates room shape** every 4 beats
- Creates shifting crystalline resonance patterns
- Combined with ultra-sparse density for glass-like quality

### Key Features

- **Density=0.05** creates only ~4-5 early reflections (vs 80-100 normally)
- **High mass + bloom** creates heavy, metallic ringing
- **Topology automation** shifts room shape continuously
- **Sparse + high bloom** = crystalline, glass-like artifacts

### Sonic Character

- Metallic, bell-like resonances
- Glass/crystal breaking quality
- Sparse, open space with long metallic tails
- Room geometry constantly morphing

### Use Cases

- Percussive instruments (bells, glass, metal)
- Sci-fi crystalline environments
- Creating sense of vast, empty metallic space
- Experimental percussion processing

---

## Preset 8: Hyperdimensional Fold 🌀

**Concept:** All dimensions modulate simultaneously

### Technical Characteristics

```
Timeline: 64 beats (tempo-synced), Loop
Keyframes: 16 (every 4 beats)
RT60: Highly variable, never repeats the same way
```

### Multi-Parameter Evolution

**ALL parameters evolve with different periods:**

```cpp
Time      = 0.5 + 0.4·sin(phase × 1.0)    // 64-beat cycle
Mass      = 0.5 + 0.3·cos(phase × 1.5)    // 42.7-beat cycle
Density   = 0.5 + 0.4·sin(phase × 2.0)    // 32-beat cycle
Bloom     = 0.5 + 0.4·cos(phase × 0.7)    // 91.4-beat cycle
Gravity   = 0.5 + 0.3·sin(phase × 1.3)    // 49.2-beat cycle
Warp      = 0.3 + 0.5·sin(phase × 3.0)    // 21.3-beat cycle
Drift     = 0.2 + 0.4·cos(phase × 2.5)    // 25.6-beat cycle

phase = (keyframe / 16) × 2π
```

### Key Features

- **7 parameters** evolving with **7 different periods**
- **Polyrhythmic evolution** - full cycle repeats only after LCM of all periods
- **16 keyframes** with S-curve interpolation for smooth transitions
- **Never truly repeats** - always discovering new sonic territory

### Mathematical Complexity

The full cycle before exact repetition:

```
LCM(64, 42.7, 32, 91.4, 49.2, 21.3, 25.6) ≈ 1,843 beats
At 120 BPM: ~15.4 minutes before true repeat
```

### Sonic Character

- Constantly evolving, never static
- Unpredictable parameter interactions
- Creates sense of infinite space
- Organic, living quality

### Use Cases

- Long-form ambient compositions
- Generative music systems
- Creating evolving soundscapes that never repeat
- Experimental sound design requiring constant variation

---

## Testing & Validation

### Expected Performance Characteristics

| Preset | Expected RT60 | CPU Load | Memory | Artifacts |
|--------|---------------|----------|--------|-----------|
| Infinite Abyss | >30s | High | Medium | Heavy, eternal tail |
| Quantum Tunneling | 15-20s | Medium | Low | Spatial glitches |
| Time Dissolution | 6-30s (variable) | Medium | Medium | Pitch instability |
| Crystalline Void | ~20s | Medium | Low | Metallic ringing |
| Hyperdimensional Fold | Variable | High | Medium | Never repeats |

### Automated Testing

Use Monument's capture and analysis tools:

```bash
# Build plugin with new presets
./scripts/rebuild_and_install.sh Monument

# Capture impulse responses (30s duration)
./scripts/capture_all_presets.sh

# Analyze RT60, frequency response
./scripts/analyze_all_presets.sh

# Check results
cat test-results/preset-baseline/preset_03/rt60_metrics.json  # Infinite Abyss
cat test-results/preset-baseline/preset_07/rt60_metrics.json  # Hyperdimensional Fold
```

### Key Metrics to Validate

1. **RT60 > 10s** for all presets (target: extreme long tails)
2. **Zipper noise < -40dB** during parameter automation
3. **CPU load < 30%** at 48kHz (within budget)
4. **No clicks/pops** during sequence transitions

---

## Design Philosophy

These presets embody Monument Reverb's **experimental ethos**:

1. **Push Physical Boundaries** - Create impossible spaces
2. **Use Testing Infrastructure** - Validate with objective measurements
3. **Timeline Automation** - Sequences create evolving complexity
4. **Multi-Dimensional Modulation** - Parameter interactions create emergence
5. **Long-Form Evolution** - Never static, always discovering

### Design Patterns Used

- **Extreme Values** - Max Time=1.0, ultra-sparse Density=0.05
- **Spatial Manipulation** - 3D position jumps, Doppler shifts
- **Polyrhythmic Evolution** - Different parameters with different periods
- **Interpolation Modes** - Step (instant), SCurve (smooth), Linear
- **Parameter Coupling** - Topology + Density, Warp + Drift, Time + Gravity

---

## Future Directions

### Potential Enhancements

1. **Modulation Integration**
   - Add Chaos Attractor → Gravity for Infinite Abyss (floor oscillation)
   - Brownian Motion → Time + Drift for Time Dissolution
   - Audio Follower → Topology for Crystalline Void (reactive crystals)

2. **Memory System Enhancement** ✅ **Partially Implemented**
   - ✅ **Infinite Abyss now uses Memory system** (Memory=0.8, MemoryDecay=0.9)
   - Potential: Extend to other presets (Crystalline Void, Time Dissolution)
   - Potential: Higher Memory values (0.95+) for even longer feedback

3. **Advanced Spatial Paths**
   - Lorenz attractor for chaotic 3D movement
   - Fibonacci spirals for golden ratio spatial evolution
   - Fractal patterns for self-similar spatial recursion

4. **Preset Morphing**
   - Blend between Infinite Abyss ↔ Crystalline Void
   - Morph Quantum Tunneling → Hyperdimensional Fold
   - Cross-fade timelines for smooth preset transitions

---

## Credits

**Design & Implementation:** Claude Sonnet 4.5 + Human Collaborator
**System:** Monument Reverb SequenceScheduler
**Testing Infrastructure:** Plugin Analyzer + RT60 measurement tools
**Date:** 2026-01-09

---

*"These presets don't simulate spaces that exist. They create spaces that can't exist."*
