# Experimental Presets Enhancement Patches

**Date:** 2026-01-09
**Purpose:** Enable Memory and Modulation systems in experimental presets (4-8)
**Rationale:** Documentation claims features that aren't implemented; these patches fix the gaps

---

## PATCH 1: Enable Memory in "Infinite Abyss" (Preset 4)

### File: `dsp/SequencePresets.cpp`

**Location:** Function `createInfiniteAbyss()` starting at line 169

**Issue:** Documentation claims "eternal memory feedback" but Memory parameters are not set

**Fix:** Add Memory parameters to keyframes

```cpp
// BEFORE (Current Implementation - Lines 182-188)
Keyframe kf0(0.0, Interpolation::SCurve);
kf0.setParameter(ParamId::Time, 1.0f);       // Maximum decay
kf0.setParameter(ParamId::Mass, 0.9f);       // Ultra-heavy
kf0.setParameter(ParamId::Density, 0.85f);   // Dense reflections
kf0.setParameter(ParamId::Bloom, 0.8f);      // High diffusion
kf0.setParameter(ParamId::Gravity, 0.3f);    // Light damping (eternal tail)
sequence.addKeyframe(kf0);

// AFTER (Enhanced Implementation)
Keyframe kf0(0.0, Interpolation::SCurve);
kf0.setParameter(ParamId::Time, 1.0f);        // Maximum decay
kf0.setParameter(ParamId::Mass, 0.9f);        // Ultra-heavy
kf0.setParameter(ParamId::Density, 0.85f);    // Dense reflections
kf0.setParameter(ParamId::Bloom, 0.8f);       // High diffusion
kf0.setParameter(ParamId::Gravity, 0.3f);     // Light damping (eternal tail)
// NEW: Memory system for eternal feedback
kf0.setParameter(ParamId::Memory, 0.8f);      // High memory amount
kf0.setParameter(ParamId::MemoryDepth, 0.7f); // Strong feedback injection
kf0.setParameter(ParamId::MemoryDecay, 0.9f); // Very slow decay (near-infinite)
kf0.setParameter(ParamId::MemoryDrift, 0.3f); // Moderate drift for organic feel
sequence.addKeyframe(kf0);
```

**Complete Enhanced Function:**

```cpp
SequenceScheduler::Sequence SequencePresets::createInfiniteAbyss()
{
    using Keyframe = SequenceScheduler::Keyframe;
    using ParamId = SequenceScheduler::ParameterId;
    using Interpolation = SequenceScheduler::InterpolationType;

    SequenceScheduler::Sequence sequence("Infinite Abyss");
    sequence.timingMode = SequenceScheduler::TimingMode::Beats;
    sequence.playbackMode = SequenceScheduler::PlaybackMode::Loop;
    sequence.durationBeats = 64.0;
    sequence.enabled = false;

    // Keyframe 0 (0 beats): Deep pit begins with eternal memory feedback
    Keyframe kf0(0.0, Interpolation::SCurve);
    kf0.setParameter(ParamId::Time, 1.0f);        // Maximum decay
    kf0.setParameter(ParamId::Mass, 0.9f);        // Ultra-heavy
    kf0.setParameter(ParamId::Density, 0.85f);    // Dense reflections
    kf0.setParameter(ParamId::Bloom, 0.8f);       // High diffusion
    kf0.setParameter(ParamId::Gravity, 0.3f);     // Light damping (eternal tail)
    // Memory system for eternal feedback (NEW)
    kf0.setParameter(ParamId::Memory, 0.8f);      // High memory amount
    kf0.setParameter(ParamId::MemoryDepth, 0.7f); // Strong feedback injection
    kf0.setParameter(ParamId::MemoryDecay, 0.9f); // Very slow decay (near-infinite)
    kf0.setParameter(ParamId::MemoryDrift, 0.3f); // Moderate drift for organic aging
    sequence.addKeyframe(kf0);

    // Keyframe 1 (16 beats): Gravity destabilizes, memory intensifies
    Keyframe kf1(16.0, Interpolation::SCurve);
    kf1.setParameter(ParamId::Gravity, 0.1f);      // Even lighter (chaos begins)
    kf1.setParameter(ParamId::MemoryDepth, 0.85f); // Peak feedback injection (NEW)
    sequence.addKeyframe(kf1);

    // Keyframe 2 (32 beats): Gravity oscillates, memory stabilizes
    Keyframe kf2(32.0, Interpolation::SCurve);
    kf2.setParameter(ParamId::Gravity, 0.5f);      // Heavier
    kf2.setParameter(ParamId::MemoryDepth, 0.65f); // Slightly reduced feedback (NEW)
    sequence.addKeyframe(kf2);

    // Keyframe 3 (48 beats): Return to light, memory drifts
    Keyframe kf3(48.0, Interpolation::SCurve);
    kf3.setParameter(ParamId::Gravity, 0.2f);
    kf3.setParameter(ParamId::MemoryDrift, 0.5f);  // Increased drift for variation (NEW)
    sequence.addKeyframe(kf3);

    // Keyframe 4 (64 beats): Loop point, return to initial memory state
    Keyframe kf4(64.0, Interpolation::SCurve);
    kf4.setParameter(ParamId::Gravity, 0.3f);
    kf4.setParameter(ParamId::MemoryDepth, 0.7f);  // Back to initial (NEW)
    kf4.setParameter(ParamId::MemoryDrift, 0.3f);  // Back to initial (NEW)
    sequence.addKeyframe(kf4);

    return sequence;
}
```

**Expected Behavior After Patch:**
- Reverb tail truly never ends (Memory captures and re-injects signal)
- Cascading recursive echoes emerge from long memory buffer
- Organic pitch drift creates evolving, tape-like character
- Matches documented behavior: "bottomless pit with eternal memory feedback"

---

## PATCH 2: Add Modulation to "Hyperdimensional Fold" (Preset 8)

### File: `plugin/PluginProcessor.cpp`

**Location:** Function `loadFactoryPreset()` or preset initialization

**Issue:** ModulationMatrix connections not defined for experimental presets

**Enhancement Pattern:**

```cpp
// Add to PluginProcessor::loadFactoryPreset() for preset index 7 (Hyperdimensional Fold)
// This would go in the preset loading logic

if (presetIndex == 7) // Hyperdimensional Fold
{
    // Configure modulation matrix for never-repeating evolution

    // Connection 1: Chaos Attractor X → Time (polyrhythmic decay variation)
    monument::dsp::ModulationMatrix::Connection conn1;
    conn1.source = monument::dsp::ModulationMatrix::Source::ChaosAttractor;
    conn1.sourceAxis = monument::dsp::ModulationMatrix::SourceAxis::X;
    conn1.destination = monument::dsp::ModulationMatrix::Destination::Time;
    conn1.depth = 0.2f;           // 20% modulation depth
    conn1.smoothingMs = 500.0f;   // 500ms smoothing for organic transitions
    conn1.enabled = true;
    modulationMatrix.addConnection(conn1);

    // Connection 2: Chaos Attractor Y → Mass (multi-dimensional character shifts)
    monument::dsp::ModulationMatrix::Connection conn2;
    conn2.source = monument::dsp::ModulationMatrix::Source::ChaosAttractor;
    conn2.sourceAxis = monument::dsp::ModulationMatrix::SourceAxis::Y;
    conn2.destination = monument::dsp::ModulationMatrix::Destination::Mass;
    conn2.depth = 0.15f;
    conn2.smoothingMs = 700.0f;   // Slower for weight changes
    conn2.enabled = true;
    modulationMatrix.addConnection(conn2);

    // Connection 3: Chaos Attractor Z → Warp (spatial distortion)
    monument::dsp::ModulationMatrix::Connection conn3;
    conn3.source = monument::dsp::ModulationMatrix::Source::ChaosAttractor;
    conn3.sourceAxis = monument::dsp::ModulationMatrix::SourceAxis::Z;
    conn3.destination = monument::dsp::ModulationMatrix::Destination::Warp;
    conn3.depth = 0.25f;          // Higher depth for shimmer emphasis
    conn3.smoothingMs = 300.0f;
    conn3.enabled = true;
    modulationMatrix.addConnection(conn3);

    // Connection 4: Brownian Motion → Drift (organic pitch wandering)
    monument::dsp::ModulationMatrix::Connection conn4;
    conn4.source = monument::dsp::ModulationMatrix::Source::BrownianMotion;
    conn4.sourceAxis = monument::dsp::ModulationMatrix::SourceAxis::Value;
    conn4.destination = monument::dsp::ModulationMatrix::Destination::Drift;
    conn4.depth = 0.18f;
    conn4.smoothingMs = 1000.0f;  // Very slow for subtle evolution
    conn4.enabled = true;
    modulationMatrix.addConnection(conn4);

    // Connection 5: Brownian Motion → Gravity (floor oscillation)
    monument::dsp::ModulationMatrix::Connection conn5;
    conn5.source = monument::dsp::ModulationMatrix::Source::BrownianMotion;
    conn5.sourceAxis = monument::dsp::ModulationMatrix::SourceAxis::Value;
    conn5.destination = monument::dsp::ModulationMatrix::Destination::Gravity;
    conn5.depth = 0.12f;
    conn5.smoothingMs = 800.0f;
    conn5.enabled = true;
    modulationMatrix.addConnection(conn5);
}
```

**Expected Behavior After Patch:**
- 3-axis chaos creates truly never-repeating parameter evolution
- Brownian motion adds organic drift (1/f noise = natural feel)
- Multiple parameters modulated with different rates = polyrhythmic complexity
- Full cycle before exact repeat: >15 minutes (vs 1 minute without modulation)

---

## PATCH 3: Add Modulation to "Crystalline Void" (Preset 7)

### File: `plugin/PluginProcessor.cpp`

**Location:** Function `loadFactoryPreset()` for preset index 6

**Enhancement Pattern:**

```cpp
if (presetIndex == 6) // Crystalline Void
{
    // Audio-reactive crystals: input signal modulates room shape

    // Connection 1: Audio Follower → Topology (reactive room shape)
    monument::dsp::ModulationMatrix::Connection conn1;
    conn1.source = monument::dsp::ModulationMatrix::Source::AudioFollower;
    conn1.sourceAxis = monument::dsp::ModulationMatrix::SourceAxis::RMS;
    conn1.destination = monument::dsp::ModulationMatrix::Destination::Topology;
    conn1.depth = 0.3f;           // Strong response for reactive crystals
    conn1.smoothingMs = 50.0f;    // Fast response to transients
    conn1.enabled = true;
    modulationMatrix.addConnection(conn1);

    // Connection 2: Audio Follower → Density (sparse → less sparse on loud input)
    monument::dsp::ModulationMatrix::Connection conn2;
    conn2.source = monument::dsp::ModulationMatrix::Source::AudioFollower;
    conn2.sourceAxis = monument::dsp::ModulationMatrix::SourceAxis::RMS;
    conn2.destination = monument::dsp::ModulationMatrix::Destination::Density;
    conn2.depth = 0.08f;          // Subtle density increase (stays mostly sparse)
    conn2.smoothingMs = 100.0f;
    conn2.enabled = true;
    modulationMatrix.addConnection(conn2);

    // Connection 3: Envelope Tracker → Bloom (transient-responsive diffusion)
    monument::dsp::ModulationMatrix::Connection conn3;
    conn3.source = monument::dsp::ModulationMatrix::Source::EnvelopeTracker;
    conn3.sourceAxis = monument::dsp::ModulationMatrix::SourceAxis::Level;
    conn3.destination = monument::dsp::ModulationMatrix::Destination::Bloom;
    conn3.depth = 0.15f;
    conn3.smoothingMs = 20.0f;    // Very fast for transient tracking
    conn3.enabled = true;
    modulationMatrix.addConnection(conn3);
}
```

**Expected Behavior After Patch:**
- Crystals "respond" to input signal strength
- Room shape shifts with audio dynamics (topology modulation)
- Transients create crystalline blooming effect
- Truly reactive, not static - "dancing pillar positions" as documented

---

## PATCH 4: Add Modulation to "Infinite Abyss" (Preset 4)

### File: `plugin/PluginProcessor.cpp`

**Location:** Function `loadFactoryPreset()` for preset index 3

**Enhancement Pattern:**

```cpp
if (presetIndex == 3) // Infinite Abyss
{
    // Organic floor oscillation via chaos modulation

    // Connection 1: Chaos Attractor X → Gravity (unstable floor)
    monument::dsp::ModulationMatrix::Connection conn1;
    conn1.source = monument::dsp::ModulationMatrix::Source::ChaosAttractor;
    conn1.sourceAxis = monument::dsp::ModulationMatrix::SourceAxis::X;
    conn1.destination = monument::dsp::ModulationMatrix::Destination::Gravity;
    conn1.depth = 0.15f;          // Moderate oscillation (floor rises/falls)
    conn1.smoothingMs = 600.0f;   // Slow breathing sensation
    conn1.enabled = true;
    modulationMatrix.addConnection(conn1);

    // Connection 2: Brownian Motion → MemoryDrift (evolving recall character)
    monument::dsp::ModulationMatrix::Connection conn2;
    conn2.source = monument::dsp::ModulationMatrix::Source::BrownianMotion;
    conn2.sourceAxis = monument::dsp::ModulationMatrix::SourceAxis::Value;
    conn2.destination = monument::dsp::ModulationMatrix::Destination::MemoryDrift;
    conn2.depth = 0.2f;           // Adds organic pitch instability to memory
    conn2.smoothingMs = 1500.0f;  // Very slow evolution
    conn2.enabled = true;
    modulationMatrix.addConnection(conn2);

    // Connection 3: Chaos Attractor Y → Mass (weight fluctuation)
    monument::dsp::ModulationMatrix::Connection conn3;
    conn3.source = monument::dsp::ModulationMatrix::Source::ChaosAttractor;
    conn3.sourceAxis = monument::dsp::ModulationMatrix::SourceAxis::Y;
    conn3.destination = monument::dsp::ModulationMatrix::Destination::Mass;
    conn3.depth = 0.1f;           // Subtle weightlessness variations
    conn3.smoothingMs = 800.0f;
    conn3.enabled = true;
    modulationMatrix.addConnection(conn3);
}
```

**Expected Behavior After Patch:**
- Gravity modulation creates pulsing/breathing sensation (beyond static timeline)
- Memory echoes evolve with organic pitch drift
- Truly feels like "falling forever" with unstable physics

---

## IMPLEMENTATION NOTES

### Where to Add Modulation Code

**Option A: Extend SequencePresets with Modulation Configs** (Recommended)
- Add `configureModulation()` method to SequencePresets class
- Return modulation connection arrays alongside sequences
- Keep all preset data in one place

**Option B: Add to PluginProcessor::loadFactoryPreset()**
- Directly configure ModulationMatrix in preset loading
- Works with existing architecture
- More scattered code

### Data Structure for Modulation Presets

```cpp
// In SequencePresets.h (add new method)
class SequencePresets
{
public:
    // ... existing methods ...

    /**
     * @brief Get modulation connections for a preset
     *
     * @param index Preset index (0-7)
     * @return Vector of modulation connections to configure
     */
    static std::vector<monument::dsp::ModulationMatrix::Connection>
        getModulationConnections(int index);
};
```

### Testing Procedure

1. **Build with patches:**
   ```bash
   cmake --build build --target Monument_All -j8
   ./scripts/rebuild_and_install.sh Monument
   ```

2. **Load experimental presets in DAW** (Logic, Reaper, Ableton)

3. **Verify Memory behavior:**
   - "Infinite Abyss" should have infinite tail (>30s RT60)
   - Memory parameter knob should show 0.8 (not 0.0)

4. **Verify Modulation behavior:**
   - "Hyperdimensional Fold" parameters should oscillate visibly
   - "Crystalline Void" should respond to input transients
   - "Infinite Abyss" gravity should breathe/pulse

5. **Run automated tests:**
   ```bash
   ./scripts/capture_all_presets.sh
   ./scripts/analyze_all_presets.sh
   ```

6. **Check RT60 metrics:**
   - Preset 3 (Infinite Abyss): Should increase from ~20s → >30s
   - Preset 6 (Crystalline Void): Should vary dynamically (not static)
   - Preset 7 (Hyperdimensional Fold): Should show parameter variation

---

## DOCUMENTATION UPDATES REQUIRED

### File: `docs/EXPERIMENTAL_PRESETS.md`

**Add section after each preset:**

```markdown
### Modulation Configuration

| Connection | Source | Destination | Depth | Smoothing | Purpose |
|------------|--------|-------------|-------|-----------|---------|
| 1 | Chaos X | Time | 20% | 500ms | Polyrhythmic decay variation |
| 2 | Chaos Y | Mass | 15% | 700ms | Character shifts |
| ... | ... | ... | ... | ... | ... |
```

### File: `CHANGELOG.md`

```markdown
## [Unreleased]

### Fixed
- **Infinite Abyss preset** now includes Memory parameters matching documentation
  - Memory=0.8, MemoryDepth=0.7, MemoryDecay=0.9, MemoryDrift=0.3
  - True eternal feedback as documented

### Added
- **Modulation routing for experimental presets**
  - Hyperdimensional Fold: 5 modulation connections (Chaos + Brownian)
  - Crystalline Void: 3 modulation connections (AudioFollower + EnvelopeTracker)
  - Infinite Abyss: 3 modulation connections (Chaos + Brownian)
  - Creates truly never-repeating, reactive reverb spaces
```

---

## ESTIMATED EFFORT

| Task | Time | Priority |
|------|------|----------|
| Patch 1: Memory in Infinite Abyss | 15 min | **High** |
| Patch 2-4: Modulation in 3 presets | 1-2 hrs | High |
| Testing (DAW + automation) | 30 min | High |
| Documentation updates | 30 min | Medium |
| Code review + commit | 15 min | High |
| **Total** | **~3 hours** | - |

---

## ALTERNATIVE: MINIMAL FIX (15 minutes)

If time-constrained, implement **only Patch 1** (Memory in Infinite Abyss):
- Fixes documentation mismatch
- Delivers documented "eternal feedback" behavior
- Modulation can be added in Phase 6

---

**Next Steps:**
1. Apply Patch 1 to `dsp/SequencePresets.cpp`
2. Build and test "Infinite Abyss" preset
3. Optionally apply Patches 2-4 for full modulation integration
4. Update documentation to reflect changes
5. Commit with message: "fix: enable Memory system in Infinite Abyss preset"

---

**Generated by:** Claude Sonnet 4.5
**Date:** 2026-01-09
**Review Status:** Ready for implementation
