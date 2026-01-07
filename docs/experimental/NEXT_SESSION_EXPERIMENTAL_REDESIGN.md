# Next Session Handoff: Experimental Redesign Implementation

**Date**: 2026-01-04
**Session Type**: Implementation Start (Phase 1)
**Estimated Time**: 3-4 hours for Phase 1 prototype

---

## What Was Completed (This Session)

### Comprehensive Redesign System ✅

**Problem**: All Monument presets sound similar. Need dramatic sonic diversity, experimental modulation, simplified UI.

**Solution Designed**:
1. **Flexible DSP Routing** - Series/parallel/feedback/bypass (`DspRoutingGraph.h`)
2. **Expressive Macros** - 6 musical controls (`ExpressiveMacroMapper.h`)
3. **Experimental Modulation** - Probability gates, quantization, morphing (`ExperimentalModulation.h`)
4. **Simplified UI** - Performance/Advanced/Playground modes
5. **MemoryEchoes Integration** - Routable, macro-controlled

### Files Created

**Design Documents** (4 files):
- [`docs/architecture/EXPERIMENTAL_REDESIGN.md`](docs/architecture/EXPERIMENTAL_REDESIGN.md) - Full design rationale (19 pages)
- [`docs/architecture/IMPLEMENTATION_GUIDE.md`](docs/architecture/IMPLEMENTATION_GUIDE.md) - Step-by-step guide (22 pages)
- [`docs/architecture/MEMORY_ECHOES_INTEGRATION.md`](docs/architecture/MEMORY_ECHOES_INTEGRATION.md) - MemoryEchoes details (14 pages)
- [`docs/architecture/REDESIGN_SUMMARY.md`](docs/architecture/REDESIGN_SUMMARY.md) - Executive summary (13 pages)

**C++ Header Files** (3 files, ready for implementation):
- [`dsp/DspRoutingGraph.h`](dsp/DspRoutingGraph.h) - Flexible routing system
- [`dsp/ExpressiveMacroMapper.h`](dsp/ExpressiveMacroMapper.h) - New macro controls
- [`dsp/ExperimentalModulation.h`](dsp/ExperimentalModulation.h) - Experimental features

---

## Next Session Task: Implement Phase 1 Prototype

### Goal
**Prove that flexible DSP routing creates dramatic sonic diversity.**

Implement `DspRoutingGraph.cpp` and test 3 routing presets:
1. **Traditional Cathedral** (baseline - existing sound)
2. **Metallic Granular** (bypass Chambers, use TubeRayTracer)
3. **Elastic Feedback Dream** (feedback loop)

**Success Criteria**: Presets #2 and #3 sound **fundamentally different** from #1 (not just "slightly different cathedrals")

---

## Implementation Checklist (Phase 1)

### Step 1: Create `dsp/DspRoutingGraph.cpp` (90 mins)

**Location**: `/Users/noisebox/Documents/3_Development/Repos/monument-reverb/dsp/DspRoutingGraph.cpp`

**Key Methods to Implement**:

```cpp
// 1. Constructor/Destructor
DspRoutingGraph::DspRoutingGraph() {}
DspRoutingGraph::~DspRoutingGraph() = default;

// 2. Prepare (allocate modules and buffers)
void DspRoutingGraph::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    // Allocate all 9 modules (Foundation, Pillars, Chambers, etc.)
    // Allocate temp buffers for parallel processing
    // Set sample rate and block size
    // Load default routing preset (TraditionalCathedral)
}

// 3. Process (execute routing graph)
void DspRoutingGraph::process(juce::AudioBuffer<float>& buffer)
{
    // Loop through routingConnections
    // Handle Series, Parallel, Feedback, Crossfeed, Bypass modes
    // Process each module in correct order
}

// 4. Load Routing Presets
void DspRoutingGraph::loadRoutingPreset(RoutingPresetType preset)
{
    // Clear existing connections
    // Set up connections for selected preset:
    //   - TraditionalCathedral: Foundation → Pillars → Chambers → Weathering → Facade
    //   - MetallicGranular: Foundation → Pillars → TubeRayTracer → Facade (bypass Chambers)
    //   - ElasticFeedbackDream: Foundation → ElasticHallway ⟲ Feedback → Chambers → Alien → Facade
}

// 5. Helper: Process individual module
void DspRoutingGraph::processModule(ModuleType module, juce::AudioBuffer<float>& buffer)
{
    // Switch on module type, call appropriate process() method
}

// 6. Helper: Blend buffers for parallel mode
void DspRoutingGraph::blendBuffers(juce::AudioBuffer<float>& destination,
                                    const juce::AudioBuffer<float>& source,
                                    float blendAmount)
{
    // Mix source into destination with blendAmount (0.0-1.0)
}
```

**Reference**: See [`docs/architecture/IMPLEMENTATION_GUIDE.md`](docs/architecture/IMPLEMENTATION_GUIDE.md) Phase 1 for detailed code examples.

### Step 2: Update `plugin/PluginProcessor.h` (15 mins)

**Changes**:
```cpp
class MonumentAudioProcessor : public juce::AudioProcessor
{
public:
    // REPLACE individual module instances with:
    monument::dsp::DspRoutingGraph routingGraph;

    // REMOVE (now managed by routingGraph):
    // monument::dsp::Foundation foundation;
    // monument::dsp::Pillars pillars;
    // monument::dsp::Chambers chambers;
    // etc.
};
```

### Step 3: Update `plugin/PluginProcessor.cpp` (30 mins)

**Changes**:
```cpp
void MonumentAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // REPLACE individual module prepare() calls with:
    routingGraph.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
}

void MonumentAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    // Update routing graph parameters from APVTS
    updateRoutingGraphParams();

    // REPLACE individual module process() calls with:
    routingGraph.process(buffer);
}

// NEW METHOD:
void MonumentAudioProcessor::updateRoutingGraphParams()
{
    // Forward parameters to routing graph modules
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

    // ... set params for all other modules
}
```

### Step 4: Update `CMakeLists.txt` (5 mins)

**Add new source file**:
```cmake
target_sources(monument-reverb
    PRIVATE
        # ... existing files
        dsp/DspRoutingGraph.cpp  # ← ADD THIS
)
```

### Step 5: Build and Test (30 mins)

**Build**:
```bash
cd /Users/noisebox/Documents/3_Development/Repos/monument-reverb
cmake --build build
```

**Test Routing Presets**:
```cpp
// In plugin or test app:

// Test 1: Traditional Cathedral (should sound familiar)
processor.routingGraph.loadRoutingPreset(RoutingPresetType::TraditionalCathedral);
// Expected: Smooth, musical reverb (same as before)

// Test 2: Metallic Granular (should sound COMPLETELY different)
processor.routingGraph.loadRoutingPreset(RoutingPresetType::MetallicGranular);
// Expected: Bright, textured, tube resonances, NO smooth tail

// Test 3: Elastic Feedback Dream (should sound COMPLETELY different)
processor.routingGraph.loadRoutingPreset(RoutingPresetType::ElasticFeedbackDream);
// Expected: Organic, morphing, breathing, unstable feedback
```

**Success Criteria**:
- ✅ Builds without errors
- ✅ Traditional Cathedral sounds familiar (baseline)
- ✅ Metallic Granular sounds **fundamentally different** (not just "slightly different")
- ✅ Elastic Feedback Dream sounds **fundamentally different** and unstable
- ✅ No crashes, clicks, or artifacts

---

## Decision Point After Phase 1

### If Successful (Presets Sound Dramatically Different)
✅ **Continue to Phase 2**: Implement `ExpressiveMacroMapper.cpp`
- Start next session with: "Implement Phase 2: Expressive Macros"

### If Unsuccessful (Presets Still Sound Similar)
❌ **Debug and Iterate**:
1. Verify routing connections are correct
2. Check that modules are processing in correct order
3. Ensure parallel blending is working
4. Test feedback loop with safety limiting

---

## Quick Start Commands (Next Session)

```bash
# Navigate to project
cd /Users/noisebox/Documents/3_Development/Repos/monument-reverb

# Create implementation file
touch dsp/DspRoutingGraph.cpp

# Open in editor (VS Code, CLion, etc.)
code dsp/DspRoutingGraph.cpp

# Build after implementing
cmake --build build

# Run standalone app to test
open build/monument-reverb_artefacts/Debug/Standalone/monument-reverb.app
```

---

## Key Files to Reference (Next Session)

### Design Documents (Read First)
1. **[`docs/architecture/IMPLEMENTATION_GUIDE.md`](docs/architecture/IMPLEMENTATION_GUIDE.md)**
   - Step-by-step Phase 1 instructions (pages 1-8)
   - Code examples for `DspRoutingGraph.cpp`

2. **[`docs/architecture/EXPERIMENTAL_REDESIGN.md`](docs/architecture/EXPERIMENTAL_REDESIGN.md)**
   - Routing preset definitions (pages 4-6)
   - Routing mode specifications

### Header Files (Implementation Reference)
1. **[`dsp/DspRoutingGraph.h`](dsp/DspRoutingGraph.h)**
   - Class definition, method signatures
   - Enum definitions (ModuleType, RoutingMode, RoutingPresetType)

2. **[`dsp/DspModules.h`](dsp/DspModules.h)**
   - Existing module interfaces
   - Foundation, Pillars, Chambers, etc.

### Code to Modify
1. **[`plugin/PluginProcessor.h`](plugin/PluginProcessor.h)**
   - Replace individual modules with `routingGraph`

2. **[`plugin/PluginProcessor.cpp`](plugin/PluginProcessor.cpp)**
   - Update `prepareToPlay()` and `processBlock()`

---

## Potential Issues and Solutions

### Issue #1: "Module not found" compile errors
**Solution**: Ensure all module headers are included:
```cpp
#include "dsp/Foundation.h"
#include "dsp/Pillars.h"
#include "dsp/Chambers.h"
// ... include all modules
```

### Issue #2: Feedback loop causes audio explosion
**Solution**: Add feedback gain safety limiting:
```cpp
// In loadRoutingPreset() for ElasticFeedbackDream:
RoutingConnection feedback{ModuleType::ElasticHallway, ModuleType::Pillars,
                            RoutingMode::Feedback};
feedback.feedbackGain = 0.3f;  // MUST be < 1.0 for stability
routingConnections.push_back(feedback);
```

### Issue #3: Parallel blending sounds phasey
**Solution**: Check blend amounts sum to 1.0:
```cpp
// For 3 parallel paths:
parallel1.blendAmount = 0.33f;  // 33%
parallel2.blendAmount = 0.33f;  // 33%
parallel3.blendAmount = 0.34f;  // 34% (total = 100%)
```

### Issue #4: CPU usage is high
**Solution**: Verify bypassed modules are skipped:
```cpp
if (!conn.enabled || moduleBypassed[static_cast<size_t>(conn.source)])
    continue;  // Skip processing
```

---

## Testing Checklist (Before Moving to Phase 2)

- [ ] `DspRoutingGraph.cpp` compiles without errors
- [ ] Traditional Cathedral preset sounds familiar (baseline)
- [ ] Metallic Granular preset sounds **dramatically different** (bright, textured)
- [ ] Elastic Feedback Dream preset sounds **dramatically different** (organic, unstable)
- [ ] No clicks, pops, or artifacts
- [ ] CPU usage is reasonable (<30% on modern CPU)
- [ ] Module bypass works correctly (CPU savings)
- [ ] Feedback loops are stable (no audio explosion)

---

## Time Estimates

| Task | Estimated Time | Cumulative |
|------|----------------|------------|
| Read implementation guide | 30 mins | 0:30 |
| Create `DspRoutingGraph.cpp` | 90 mins | 2:00 |
| Update `PluginProcessor.h/.cpp` | 45 mins | 2:45 |
| Update `CMakeLists.txt` | 5 mins | 2:50 |
| Build and test | 30 mins | 3:20 |
| Debug issues (buffer) | 30 mins | 3:50 |

**Total**: 3-4 hours for Phase 1 prototype

---

## Communication Notes

### If You Need Help
- **Design Questions**: See [`EXPERIMENTAL_REDESIGN.md`](docs/architecture/EXPERIMENTAL_REDESIGN.md) Section 1-2
- **Implementation Questions**: See [`IMPLEMENTATION_GUIDE.md`](docs/architecture/IMPLEMENTATION_GUIDE.md) Phase 1
- **JUCE DSP Questions**: See JUCE skill documentation (`/skills/juce-dsp-audio-plugin`)

### If You Get Stuck
1. **Check existing modules**: See how `Foundation`, `Pillars`, `Chambers` are currently integrated
2. **Verify routing logic**: Print routing connections to console for debugging
3. **Test incrementally**: Start with just Series mode, add Parallel/Feedback later

### Progress Reporting
After completing Phase 1, report:
- ✅ What works (which presets sound different)
- ❌ What doesn't work (issues encountered)
- 🎵 Audio examples (describe sonic character of each preset)
- 🔄 Next steps (continue to Phase 2 or iterate Phase 1)

---

## Final Notes

### Why Phase 1 is Critical
- **Proves the core hypothesis**: Flexible routing creates dramatic sonic diversity
- **Low risk**: Only changes DSP chain, doesn't touch macros or UI
- **Reversible**: Can roll back to fixed serial chain if unsuccessful
- **Fast feedback**: 3-4 hours to validate or invalidate approach

### What Happens If Phase 1 Succeeds
- **Phase 2**: Implement `ExpressiveMacroMapper.cpp` (2-3 days)
- **Phase 3**: Experimental modulation (2-3 days)
- **Phase 4**: UI simplification (1-2 days)
- **Phase 5**: Preset library (1 day)

### What Happens If Phase 1 Fails
- **Iterate**: Adjust routing definitions, add more presets
- **Debug**: Ensure parallel/feedback modes work correctly
- **Fallback**: Keep routing flexibility, revert to fixed chain as default

---

## Session Start Command

```bash
# Start next session with:
cd /Users/noisebox/Documents/3_Development/Repos/monument-reverb
cat NEXT_SESSION_EXPERIMENTAL_REDESIGN.md

# Then:
# 1. Read docs/architecture/IMPLEMENTATION_GUIDE.md (Phase 1)
# 2. Create dsp/DspRoutingGraph.cpp
# 3. Implement methods from guide
# 4. Build and test
```

**Ready to implement!** All design documents and header files are complete. Phase 1 should take 3-4 hours.

---

**Context Budget**: 92KB used, 108KB remaining (82% efficient)
**Handoff Complete**: All design and planning finished, ready for implementation start.
