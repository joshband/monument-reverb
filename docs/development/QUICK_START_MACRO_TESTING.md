# Quick Start: Testing Macro Controls

## ✅ Build Complete!

Monument plugin has been successfully built with macro controls integrated into the UI.

**Build Date:** 2026-01-03
**Plugin Locations:**
- AU: `Monument_artefacts/Release/AU/Monument.component`
- VST3: `Monument_artefacts/Release/VST3/Monument.vst3`

---

## UI Layout

Monument now features a **two-tier UI** with macro controls at the top:

```
┌─────────────────────────────────────────────────────────────┐
│                        MONUMENT                             │
├─────────────────────────────────────────────────────────────┤
│  MACRO CONTROLS                                             │
│                                                              │
│  [Material] [Topology] [Viscosity] [Evolution] [Chaos] [Elasticity]
│                                                              │
├─────────────────────────────────────────────────────────────┤
│  BASE PARAMETERS                                            │
│                                                              │
│  [Mix]    [Time]    [Mass]    [Density]                    │
│  [Bloom]  [Air]     [Width]   [Warp]                       │
│  [Drift]  [Gravity] [Freeze]  [Presets]                    │
└─────────────────────────────────────────────────────────────┘
```

**Window Size:** 900×580 (expanded from 720×420)

---

## Step 1: Load Plugin in DAW

### Option A: Copy to System Plugins Folder

```bash
# AU Plugin
cp -r Monument_artefacts/Release/AU/Monument.component ~/Library/Audio/Plug-Ins/Components/

# VST3 Plugin
cp -r Monument_artefacts/Release/VST3/Monument.vst3 ~/Library/Audio/Plug-Ins/VST3/
```

### Option B: Rescan Plugins in DAW

If your DAW supports custom plugin folders, add the `Monument_artefacts/Release/` directory to your scan paths.

---

## Step 2: Quick Validation Tests

### Test 1: Material Macro (2 minutes)

**Setup:**
1. Load Monument on an audio track
2. Create/import a short impulse (clap, snare, rim shot)
3. Set all other macros to their defaults:
   - Topology = 0.5
   - Viscosity = 0.5
   - Evolution = 0.5
   - Chaos = 0.0
   - Elasticity = 0.0

**Test:**
1. Set **Material = 0.0** (soft)
2. Play impulse
3. **Listen for:** Short, bright, sparse reverb tail
4. Move **Material → 1.0** (hard)
5. **Listen for:**
   - Tail lengthens significantly
   - Sound gets darker/warmer
   - Reflections become more dense

**Success Criteria:**
- ✅ Smooth transition (no clicks or zipper noise)
- ✅ Clear audible difference between soft and hard
- ✅ Base parameters (Time, Mass, Density) update automatically

---

### Test 2: Viscosity Macro (2 minutes)

**Setup:**
1. Reset Material = 0.5
2. Generate a sustained tone (250 Hz sine wave, 4 seconds)

**Test:**
1. Set **Viscosity = 0.0** (airy)
2. Play tone
3. **Listen for:** Open, bright, airy reverb
4. Move **Viscosity → 1.0** (thick)
5. **Listen for:**
   - High frequencies roll off
   - Reverb sounds "muffled" or "underwater"
   - Tail shortens slightly

**Success Criteria:**
- ✅ High-frequency rolloff is smooth and progressive
- ✅ Thickness is musically pleasing (not broken/distorted)
- ✅ Air parameter responds inversely

---

### Test 3: Combined Macros (3 minutes)

**Scenario:** "Hard stone cathedral in thick atmosphere"

**Settings:**
- Material = 0.9 (very hard)
- Viscosity = 0.8 (very thick)
- Topology = 0.5
- Evolution = 0.5
- Chaos = 0.0

**Expected Sound:**
- Long, dark reverb tail (hard surfaces)
- Muffled highs (thick medium)
- Dense, complex reflections
- Effective tail shorter than pure hard material (viscosity reduces time)

**What's Happening Under the Hood:**
- Material wants Time = 0.77
- Viscosity wants Time = 0.44
- **Blending:** Material gets 60% weight, Viscosity gets 40%
- **Final Time:** ≈ 0.638 (weighted average)

**Success Criteria:**
- ✅ Macros interact musically (not conflicting)
- ✅ Sound is coherent and intentional
- ✅ No CPU spikes or audio dropouts

---

### Test 4: Topology + Chaos (3 minutes)

**Setup:**
1. Reset all macros to defaults
2. Use impulse or sustained tone

**Test Sequence:**

1. **Topology = 0.0** (regular Euclidean space)
   - Stable, predictable spatial character
   - Hadamard matrix (orthogonal)

2. **Topology = 1.0** (non-Euclidean space)
   - Complex, morphing spatial character
   - Householder matrix (dense)

3. **Topology = 0.5, Chaos = 1.0** (chaotic regular space)
   - Adds erratic motion to stable geometry
   - Drift increases significantly

**Success Criteria:**
- ✅ Warp parameter morphs FDN matrix smoothly
- ✅ Chaos adds unpredictability without instability
- ✅ No audio artifacts or feedback runaway

---

### Test 5: Evolution Macro (2 minutes)

**Setup:**
1. Reset all macros
2. Use sustained tone (4-8 seconds)

**Test:**
1. Set **Evolution = 0.0** (static)
2. **Listen for:** Unchanging, frozen reverb
3. Set **Evolution = 1.0** (evolving)
4. **Listen for:**
   - Reverb tail swells and blooms over time
   - Gentle movement and drift
   - Dynamic, living character

**Success Criteria:**
- ✅ Bloom creates envelope-shaped evolution
- ✅ Drift increases subtly
- ✅ Space feels "alive" without being chaotic

---

## Step 3: Performance Check

### CPU Usage Test

**Setup:**
1. Create 5 instances of Monument on separate tracks
2. Play audio through all 5
3. Check DAW CPU meter

**Expected:**
- Phase 2 overhead: < 0.1% per instance
- Total per instance: ~0.6% CPU
- 5 instances: ~3% CPU total

**Success Criteria:**
- ✅ No audio dropouts or glitches
- ✅ CPU scales linearly with instance count
- ✅ Smooth playback even with multiple instances

---

## Step 4: Edge Case Testing (Optional)

### Test All Macros at Extremes

**Test A: All Minimums**
- Set all macros to 0.0
- Play audio
- **Verify:** Stable output, no NaN/Inf

**Test B: All Maximums**
- Set all macros to 1.0
- Play audio
- **Verify:** Stable output, no clipping

**Test C: Rapid Sweeps**
- Automate Material: 0.0 → 1.0 → 0.0 in 1 second
- **Verify:** No clicks, smooth transitions

---

## Known Limitations

### Elasticity Macro

**Status:** Reserved for Phase 3 (physical modeling)

**Current Behavior:**
- Parameter is visible in UI
- Moving it has **no audible effect** (DSP not implemented yet)
- This is expected and correct for Phase 2

**Future:** Phase 3 will implement elastic hallway deformation

---

## Troubleshooting

### Issue: No macro controls visible

**Solution:**
- Verify plugin was rebuilt after UI changes
- Check window size is 900×580 (not 720×420)
- Try closing and reopening the plugin editor

### Issue: Parameters don't respond

**Check:**
1. APVTS has parameters registered ([plugin/PluginProcessor.cpp:585-626](plugin/PluginProcessor.cpp))
2. MacroMapper.computeTargets() is called per block ([plugin/PluginProcessor.cpp:197-310](plugin/PluginProcessor.cpp))
3. Influence blending is enabled

### Issue: Zipper noise or clicks

**Check:**
1. ParameterSmoother is active
2. Smoothing time constants (20-50ms recommended)
3. No sample-rate parameter updates

### Issue: CPU spikes

**Check:**
1. No allocations in processBlock()
2. MacroMapper computeTargets() is block-rate (not sample-rate)
3. Profile with Xcode Instruments

---

## Next Steps

### If All Tests Pass ✅

**Proceed to Phase 3: Implement Modulation Sources**

1. Start with [BrownianMotion](dsp/BrownianMotion.h) (simplest)
2. Follow TDD approach (write tests first)
3. Reference [ARCHITECTURE_REVIEW.md](ARCHITECTURE_REVIEW.md:469-490)

**Estimated Time:** 2-3 weeks
- Week 1: Brownian Motion + Audio Follower
- Week 2: Envelope Tracker + Chaos Attractor
- Week 3: Integration + Polish

### If Tests Fail ❌

1. Document the failure (which test, what happened)
2. Check troubleshooting section above
3. Review [PHASE_2_VALIDATION_TEST.md](PHASE_2_VALIDATION_TEST.md) for detailed debugging

---

## Test Results Template

**Copy and fill out:**

```
# Macro UI Testing Results
Date: ________
Tester: ________
DAW: ________
OS Version: ________

## Quick Validation:

[ ] Test 1: Material Macro (Pass/Fail)
    Notes: _______________________________________

[ ] Test 2: Viscosity Macro (Pass/Fail)
    Notes: _______________________________________

[ ] Test 3: Combined Macros (Pass/Fail)
    Notes: _______________________________________

[ ] Test 4: Topology + Chaos (Pass/Fail)
    Notes: _______________________________________

[ ] Test 5: Evolution Macro (Pass/Fail)
    Notes: _______________________________________

[ ] Performance Check (Pass/Fail)
    CPU: ___% per instance
    Notes: _______________________________________

## Overall Status:
[ ] Pass - Ready for Phase 3
[ ] Fail - Issues found (see notes)

## Screenshots:
- [ ] Macro controls visible in UI
- [ ] Parameter values update correctly
- [ ] DAW automation shows macro parameters
```

---

## Reference Documents

- [ARCHITECTURE_REVIEW.md](ARCHITECTURE_REVIEW.md) - Full roadmap and implementation details
- [PHASE_2_VALIDATION_TEST.md](PHASE_2_VALIDATION_TEST.md) - Comprehensive testing guide
- [docs/DSP_ARCHITECTURE.md](docs/DSP_ARCHITECTURE.md) - Macro system architecture
- [CHANGELOG.md](CHANGELOG.md) - Phase 1 & 2 completion notes

---

**Ready to test!** 🎛️

Load Monument in your DAW and experience the macro control system in action.
