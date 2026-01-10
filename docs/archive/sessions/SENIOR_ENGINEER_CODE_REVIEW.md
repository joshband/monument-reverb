# Monument Reverb - Senior DSP Engineer Architecture & Code Review

**Date:** 2026-01-06
**Reviewer:** Senior Audio DSP / JUCE / VST3 / AU Engineer
**Scope:** Full codebase architecture, DSP implementation, real-time safety, and documentation hygiene

---

## Executive Summary

Monument is a well-architected JUCE C++17 audio plugin implementing an abstract, architectural reverb with modular DSP processing. The codebase demonstrates solid engineering practices including proper real-time safety, modular architecture, and comprehensive parameter management.

**Overall Assessment: 🟢 Good (85%)**

### Key Strengths
- ✅ Real-time safe DSP processing with proper memory pre-allocation
- ✅ Clean FDN (Feedback Delay Network) implementation with stability safeguards
- ✅ Modular DSP architecture with flexible routing graph
- ✅ Comprehensive parameter smoothing preventing zipper noise
- ✅ Thread-safe modulation matrix with proper atomic operations
- ✅ Well-documented macro system mapping high-level controls to low-level DSP

### Critical Issues Found
- 🔴 **Missing HeroKnob.h** - PluginEditor.h includes `ui/HeroKnob.h` which doesn't exist
- 🟡 **Smoke test inadequate** - Only tests a trivial math operation, not DSP
- 🟡 **Documentation sprawl** - Multiple handoff/session files need consolidation

---

## 1. DSP Architecture Review

### 1.1 Signal Flow (Current Implementation)

```
Input
  │
  ▼
┌─────────────────────────────────────────────────────────────────────┐
│ DspRoutingGraph (flexible routing container)                        │
│ ┌─────────────┐  ┌─────────┐  ┌──────────┐  ┌───────────┐          │
│ │ Foundation  │→│ Pillars │→│ Chambers │→│ Weathering│          │
│ │ (DC block,  │  │ (early  │  │ (8-line  │  │ (mod/drift)│          │
│ │  gain stage)│  │ reflect)│  │ FDN)     │  │           │          │
│ └─────────────┘  └─────────┘  └──────────┘  └───────────┘          │
│       │                            │              │                 │
│       ▼                            ▼              ▼                 │
│ ┌──────────────┐  ┌──────────────┐  ┌──────────────┐               │
│ │TubeRayTracer │  │ElasticHallway│  │AlienAmplify  │               │
│ │(modal tubes) │  │(elastic walls)│  │(physics bend)│               │
│ └──────────────┘  └──────────────┘  └──────────────┘               │
│       │                                   │                         │
│       ▼                                   ▼                         │
│ ┌─────────────┐  ┌─────────────┐                                   │
│ │ Buttress    │→│ Facade      │→ Output                           │
│ │ (limiter)   │  │ (stereo/mix)│                                   │
│ └─────────────┘  └─────────────┘                                   │
└─────────────────────────────────────────────────────────────────────┘
```

### 1.2 Chambers (FDN Core) - **Excellent Implementation**

**Location:** `dsp/Chambers.h/cpp`

**Strengths:**
- 8-line FDN with prime-length delays (2411, 4201, 7001... samples)
- Hadamard/Householder matrix morphing via Warp parameter
- Per-line damping with per-sample smoothing (prevents clicks)
- Input diffusion (2 allpass) + Late diffusion (8 allpass) for density
- Freeze mode with proper crossfading and topology capture
- Bloom envelope shaping (exponential decay + plateau blend)
- Gravity high-pass in feedback loop (low-frequency containment)

**Code Quality:**
```cpp
// From Chambers.cpp:211-325 - Proper prepare() with full pre-allocation
void Chambers::prepare(double sampleRate, int blockSize, int numChannels)
{
    // All delay lines allocated here, not in process()
    delayLines.setSize(kNumLines, delayBufferLength);
    // Per-parameter smoothing with different time constants
    timeSmoother.setSmoothingTimeMs(40.0f);  // Responsive but safe
    massSmoother.setSmoothingTimeMs(60.0f);  // Slower for damping
}
```

**Real-Time Safety:** ✅ No allocations in `process()`, uses `juce::ScopedNoDenormals`

### 1.3 Pillars (Early Reflections) - **Good with Minor Issues**

**Location:** `dsp/DspModules.cpp:62-456`

**Strengths:**
- Fractal tap clustering with procedural randomization
- Mode-based tuning (Glass/Stone/Fog) with distinct frequency profiles
- Impulse response loading capability
- Per-tap allpass diffusion with smoothed coefficients

**Issue Found:**
```cpp
// Line 149: Deferred tap updates based on signal level
if (tapsDirty && inputPeakMagnitude < kTapUpdateThreshold)
{
    updateTapLayout();  // Safe: only when signal is quiet
}
```
This is good defensive programming, but the `kTapUpdateThreshold` (1.0e-3f = -60dB) may be too high for very quiet passages. Consider lowering to 1.0e-4f (-80dB).

### 1.4 Parameter Management - **Excellent**

**Location:** `plugin/PluginProcessor.cpp:180-420`

**Strengths:**
- Batched atomic parameter reads into `ParameterCache` struct
- Block-rate smoothing via `juce::SmoothedValue` (50ms default)
- Conditional smoother advancement (skip if not ramping)
- Macro system blending with influence-based crossfade

**Pattern to Highlight:**
```cpp
// Lines 206-247: Batched atomic loads reduce cache thrashing
paramCache.mix = parameters.getRawParameterValue("mix")->load();
paramCache.time = parameters.getRawParameterValue("time")->load();
// ... all parameters loaded in one batch
```

### 1.5 Modulation Matrix - **Well Designed**

**Location:** `dsp/ModulationMatrix.h/cpp`

**Features:**
- 4 modulation sources (Chaos, AudioFollower, Brownian, Envelope)
- 15+ destinations (all reverb + physical modeling parameters)
- 64+ simultaneous connections possible
- Block-rate processing for efficiency
- Thread-safe connection management with `juce::SpinLock`

**Real-Time Safety:** ✅ Uses lock-free reads in audio thread, spin lock only for connection updates

---

## 2. JUCE Integration & Plugin Formats

### 2.1 CMakeLists.txt Analysis

**Location:** `CMakeLists.txt`

**Configuration:**
- JUCE 8.0.12 via FetchContent (or local checkout option)
- Plugin formats: AU + VST3
- C++17 required
- macOS deployment target: 12.0 (arm64 default)

**Issues Found:**
1. **No AAX format** - Consider adding for Pro Tools market
2. **No AUv3 format** - Listed in requirements but not in CMake (iOS)

```cmake
# Line 47: Only AU and VST3
FORMATS AU VST3
# Should be: FORMATS AU AUv3 VST3 [AAX if licensed]
```

### 2.2 Binary Data Assets

**Current assets in CMake:**
```cmake
juce_add_binary_data(MonumentAssets
    SOURCES
    assets/ui/chamber_wall_time_states.png
    assets/ui/knobs_test/base_body_concrete.png
    assets/ui/knobs_test/indicator_metal.png
    assets/ui/knobs_test/detail_ring_engraved.png
    assets/ui/knobs_test/center_cap_brushed_metal.png
)
```

**Issue:** No hero knob assets referenced, yet PluginEditor uses `HeroKnob` class extensively.

### 2.3 Plugin Processor Compliance

**Proper Implementation:**
- ✅ `isBusesLayoutSupported()` - Handles mono and stereo correctly
- ✅ `getTailLengthSeconds()` - Returns 0.0 (could be improved to return actual reverb tail)
- ✅ `getStateInformation()/setStateInformation()` - XML serialization
- ✅ `acceptsMidi()/producesMidi()` - Correctly returns false for effect

**Improvement Needed:**
```cpp
// Line 69-72: getTailLengthSeconds should return actual tail estimate
double MonumentAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;  // Should return ~5-15 seconds based on Time parameter
}
```

---

## 3. Critical Issues

### 3.1 🔴 Missing HeroKnob.h File

**Severity:** CRITICAL - Build will fail

**Location:** `plugin/PluginEditor.h:9`
```cpp
#include "ui/HeroKnob.h"  // FILE DOES NOT EXIST
```

**Evidence:**
- `PluginEditor.h` declares 21 `HeroKnob` members
- No `HeroKnob.h` or `HeroKnob.cpp` in `ui/` directory
- Glob search shows only: LayeredKnob, MonumentKnob, MonumentControl, etc.

**Resolution Required:**
1. Create `ui/HeroKnob.h/cpp` (likely extends LayeredKnob with hero texture loading)
2. Or rename existing knob class to HeroKnob
3. Add to CMakeLists.txt target_sources

### 3.2 🟡 Inadequate Test Coverage

**Location:** `tests/smoke-test.cpp`
```cpp
int main()
{
    const double input = 0.5;
    const double expected = 0.25;
    const double actual = input * input;
    return std::abs(actual - expected) < 1.0e-12 ? 0 : 1;
}
```

**Problem:** This tests basic math, not plugin functionality.

**Recommended Tests:**
1. Plugin instantiation without crash
2. `prepareToPlay()` + `processBlock()` with silence
3. Parameter range validation
4. Preset save/load round-trip
5. FDN stability under extreme parameters

---

## 4. UI Component Analysis

### 4.1 Current UI Components

| Component | Status | Purpose |
|-----------|--------|---------|
| `LayeredKnob.h/cpp` | ✅ Active | Multi-layer PNG rendering with rotation |
| `ModMatrixPanel.h/cpp` | ✅ Active | 4×15 modulation matrix grid |
| `MonumentKnob.h/cpp` | ✅ Active | Base knob wrapper |
| `MonumentToggle.h/cpp` | ✅ Active | Toggle button |
| `MonumentTimeKnob.h` | ✅ Active | Time-specific formatting |
| `ChamberWallControl.h/cpp` | 🟡 Deprecated | Old MVP system, superseded |
| `MonumentControl.h/cpp` | 🟡 Deprecated | Old MVP system, superseded |
| `HeroKnob.h` | 🔴 Missing | Referenced but doesn't exist |

### 4.2 UI Architecture

The editor uses a toggleable panel system:
- Macro controls (10 knobs) always visible
- Base parameters hidden by default (toggle button)
- Modulation matrix as expandable panel
- Processing mode and architecture selectors in toolbar

**Good Practice:** Dynamic window sizing based on visible panels (260px compact, 580px with base params, 1080px with mod matrix).

---

## 5. Documentation Inventory

### 5.1 Root Documentation (Keep)

| File | Status | Action |
|------|--------|--------|
| `README.md` | ✅ Current | Keep |
| `ARCHITECTURE.md` | ✅ Current | Keep |
| `ARCHITECTURE_QUICK_REFERENCE.md` | ✅ Current | Keep |
| `CHANGELOG.md` | ✅ Current | Keep |
| `MANIFEST.md` | ✅ Current | Keep (timeless) |
| `STANDARD_BUILD_WORKFLOW.md` | ✅ Current | Keep |
| `CONTRIBUTING.md` | ✅ Current | Keep |
| `AGENTS.md` | ✅ Current | Keep |
| `CMakeLists.txt` | ✅ Current | Keep |

### 5.2 Session/Handoff Files (Consolidate or Archive)

| File | Status | Recommendation |
|------|--------|----------------|
| `NEXT_SESSION_HANDOFF.md` | 🟡 Stale | Archive after extracting tasks |
| `NEXT_SESSION_START.md` | 🟡 Stale | Archive |
| `NEXT_SESSION_HERO_KNOBS.md` | 🟡 Stale | Archive |
| `HERO_KNOBS_SESSION_SUMMARY.md` | 🟡 Stale | Archive or merge into UI docs |
| `TOKEN_OPTIMIZATION_STRATEGIES.md` | 🟡 Meta | Archive (AI tooling specific) |
| `CLAUDE_MD_OPTIMIZATION_RESULTS.md` | 🟡 Meta | Archive (AI tooling specific) |
| `CLAUDE.md` | ⚠️ Keep | Project context for AI tools |

**Recommendation:** Create a single `docs/session-archive/` directory and move all `NEXT_SESSION_*.md` files there after each sprint.

### 5.3 Documentation in `docs/`

| Directory | Files | Status |
|-----------|-------|--------|
| `docs/ui/` | 5 files | 🟡 Mixed - some describe old MVP |
| `docs/architecture/` | 5 files | ✅ Current |
| `docs/testing/` | 5 files | 🟡 Phase 2/3 historical |
| `docs/development/` | 2 files | ✅ Current |
| `docs/presets/` | 1 file | ✅ Current |

### 5.4 Files to Archive/Remove

**Archive to `docs/archive/` or separate repo:**

1. **Session files** (move to `docs/session-archive/`):
   - `NEXT_SESSION_HANDOFF.md`
   - `NEXT_SESSION_START.md`
   - `NEXT_SESSION_HERO_KNOBS.md`
   - `HERO_KNOBS_SESSION_SUMMARY.md`

2. **AI tooling meta-docs** (move to `docs/ai-tooling/`):
   - `TOKEN_OPTIMIZATION_STRATEGIES.md`
   - `CLAUDE_MD_OPTIMIZATION_RESULTS.md`

3. **Deprecated UI docs** (mark as historical):
   - `docs/ui/MVP_UI_HANDOFF_2026_01_03.md`
   - `docs/ui/MONUMENT_UI_STRATEGIC_DESIGN_PLAN.md` (future vision, not current)

### 5.5 Missing Documentation

| Needed Document | Priority |
|-----------------|----------|
| `docs/ui/MOD_MATRIX_PANEL_GUIDE.md` | HIGH |
| `docs/testing/PHASE_4_COMPLETE_SUMMARY.md` | MEDIUM |
| `docs/ui/COMPONENT_REFERENCE.md` | MEDIUM |
| `docs/presets/PRESET_AUTHORING_GUIDE.md` | LOW |

---

## 6. Deprecated/Stale Code

### 6.1 UI Components to Consider Removing

| File | Lines | Reason | Recommendation |
|------|-------|--------|----------------|
| `ui/ChamberWallControl.h/cpp` | 1026 | Old MVP system, not used | Archive or remove |
| `ui/MonumentControl.h/cpp` | 8104 | Old MVP system, partially used | Keep if MonumentKnob depends on it |

**Note:** `MonumentControl` may still be the base class for `MonumentKnob`. Verify inheritance before removing.

### 6.2 Scripts Inventory

| Script | Status | Purpose |
|--------|--------|---------|
| `scripts/build_macos.sh` | ✅ Active | Build script |
| `scripts/dev_loop.sh` | ✅ Active | Watch/rebuild loop |
| `scripts/install_macos.sh` | ✅ Active | Install plugins |
| `scripts/open_xcode.sh` | ✅ Active | Xcode project gen |
| `scripts/run_pluginval.sh` | ✅ Active | Plugin validation |
| `scripts/run_blender_enhanced.sh` | ✅ Active | Blender knob gen |
| `scripts/generate_knob_*.py` | 🟡 Multiple | Many knob scripts - consolidate |
| `scripts/mask_*.py` | 🟡 Multiple | Image masking experiments |

**Recommendation:** Consolidate the 10+ knob/masking Python scripts into a single `scripts/knob_generation/` directory with a master script.

### 6.3 Test Files

| Test | Status | Recommendation |
|------|--------|----------------|
| `tests/smoke-test.cpp` | ⚠️ Trivial | Replace with actual plugin test |
| `tests/MemoryEchoesTest.cpp` | ✅ Active | Keep |
| `tests/MemoryEchoesHarness.cpp` | ✅ Active | Keep |

---

## 7. Performance Considerations

### 7.1 Current CPU Budget (per NEXT_SESSION_HANDOFF.md)

| Component | CPU Usage | Status |
|-----------|-----------|--------|
| Core Reverb (Chambers) | 0.5% | ✅ |
| Macro System | 0.1% | ✅ |
| Modulation Matrix | 0.5% | ✅ |
| TubeRayTracer | 1.5% | ⚠️ Could optimize with SIMD |
| ElasticHallway | 0.75% | ✅ |
| AlienAmplification | 0.85% | ✅ |
| **TOTAL** | **4.2%** | ✅ (Target: 3-5%) |

### 7.2 Optimization Opportunities

1. **TubeRayTracer** (1.5% → 0.8%):
   - Use `juce::dsp::ProcessorChain` for SIMD batch processing
   - Current: per-sample biquad processing
   - Potential savings: 47%

2. **Parameter Smoothers**:
   - Current implementation already optimizes by skipping inactive smoothers
   - Good: `if (smoother.isSmoothing()) smoother.skip(blockSize);`

---

## 8. Recommendations Summary

### 8.1 Immediate Actions (Before Next Build)

1. **🔴 Create `ui/HeroKnob.h/cpp`** - Required for compilation
2. **Add HeroKnob to CMakeLists.txt** `target_sources()`
3. **Fix smoke-test.cpp** - Add minimal plugin instantiation test

### 8.2 Short-Term (1-2 Weeks)

1. Archive session handoff files to `docs/session-archive/`
2. Consolidate Python scripts into `scripts/knob_generation/`
3. Add historical headers to deprecated UI docs
4. Implement proper `getTailLengthSeconds()` return value
5. Add AUv3 format to CMake for iOS support

### 8.3 Medium-Term (1-2 Months)

1. Create comprehensive unit test suite
2. Add pluginval CI integration
3. Consolidate knob generation pipeline
4. Document ModMatrixPanel usage
5. Consider AAX format for Pro Tools

### 8.4 Documentation Restructure

```
docs/
├── INDEX.md                    # Keep - navigation hub
├── architecture/               # Keep - technical deep dives
├── development/                # Keep - quick start guides
├── testing/                    # Keep - test procedures
├── presets/                    # Keep - preset authoring
├── ui/                         # Keep - UI design
├── archive/                    # NEW - historical docs
│   ├── sessions/               # All NEXT_SESSION_*.md files
│   ├── ai-tooling/             # TOKEN_OPTIMIZATION, etc.
│   └── mvp-ui/                 # Old MVP UI docs
└── DOCUMENTATION_HYGIENE_REVIEW.md  # Keep - hygiene tracking
```

---

## 9. Conclusion

Monument Reverb demonstrates solid audio plugin engineering with a well-designed DSP architecture. The FDN implementation is stable and feature-rich, the macro system provides excellent high-level control, and the modulation matrix adds creative flexibility.

**Primary Blockers:**
- Missing HeroKnob implementation prevents compilation
- Test coverage is inadequate for production confidence

**Strengths to Preserve:**
- Real-time safe architecture
- Modular DSP routing graph
- Comprehensive parameter smoothing
- Clear separation of concerns

The codebase is in a good state for Phase 5 completion, pending resolution of the HeroKnob issue and documentation cleanup.

---

**Review Completed:** 2026-01-06
**Next Review:** After HeroKnob implementation and Phase 5 completion
