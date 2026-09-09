# Monument Reverb - Project Roadmap

**Purpose:** Prioritized task queue for parallel async development
**Last Updated:** 2026-01-09
**Format:** Agent-ready task definitions with acceptance criteria

---

## Task Priority Legend

| Priority | Label | Definition | Max Parallel Agents |
|----------|-------|------------|---------------------|
| **P0** | Critical | Blocks release, safety issues, data loss | 1 (sequential) |
| **P1** | High | Core functionality, major quality improvements | 4-6 |
| **P2** | Medium | Quality of life, polish, documentation | 8-10 |
| **P3** | Low | Nice-to-have, experimental, future vision | Unlimited |

**Domain Tags:** `[DSP]` `[UI]` `[TEST]` `[DOC]` `[BUILD]` `[PERF]` `[PRESET]`

---

## P0: Critical Tasks (Sequential)

> ⚠️ **Do not parallelize** — These tasks require sequential completion and review

### P0-01: RT-Safety Verification Sweep
**Domain:** `[DSP] [PERF]` | **Effort:** 4h | **Status:** ✅ Complete

Verify all audio-thread code paths are allocation-free and lock-free.

**Acceptance Criteria:**
- [ ] `monument_realtime_allocation_characterization_test` passes green
- [ ] No `malloc`/`new` in `processBlock()` paths (verified via Instruments/ASan)
- [ ] All shared state uses atomic or lock-free patterns
- [ ] Denormal prevention active (`juce::ScopedNoDenormals`)

**Dependencies:** None

---

## P1: High Priority Tasks (Parallelizable: 4-6 agents)

### P1-01: Ambient Reverb Quality Alignment
**Domain:** `[DSP]` | **Effort:** 8h | **Status:** ✅ Complete (see [docs/development/P1-01_AMBIENT_REVERB_IMPLEMENTATION.md](docs/development/P1-01_AMBIENT_REVERB_IMPLEMENTATION.md))

Align Monument's ambient reverb characteristics with Valhalla Supermassive, BigSky MX, and NightSky.

**Delivered, with deviations from the original spec noted:**
- [x] FDN delay lines: 8 → **12** (spec said 12-16; 12 was the size actually implemented and is what all the matrix/buffer work below is verified against)
- [x] Max delay time: ~1.23s → **~6.5s** at the base (Incommensurate) setting
- [x] Warp-style harmonic delay clustering: **Harmonic2x/Harmonic3x/OctaveStack** modes, each line a ratio of one shared fundamental delay (not spec'd this way originally, but is what makes "harmonic"/"octave" relationships musically real rather than the initial per-line-base design, which collapsed most of the 12 lines onto an identical clamped delay)
- [x] Explicit density-evolution control: `densityEvolution` parameter, -1 (decreasing/grainy→smooth) to +1 (increasing/smooth→grainy)
- [ ] RT60 range 4s-120s: **not delivered as spec'd** — the WIP that attempted this (`kMaxFeedbackAmbient`) was dead code, never wired into the feedback calculation; removed rather than half-implemented. RT60 today is unchanged from pre-P1-01 (2-35s range, verified in `monument_reverb_dsp_test`)
- [ ] Density curves matching reference presets within 15%: not measured — no reference-preset comparison harness exists for this
- [x] (Not in original spec) Slow-attack reverb: `attackTime` parameter, 0 (instant) to 1 (~10s ambient swell)
- [x] (Not in original spec) All three new controls wired to host-automatable APVTS parameters (`warpClustering`, `densityEvolution`, `attackTime`)

**Dependencies:** P0-01 complete

---

### P1-02: Tail Decay & Modulation Enhancement
**Domain:** `[DSP]` | **Effort:** 6h | **Agents:** 2 (parallel)

**Subtasks (parallel):**
- **P1-02a: Extreme Decay Modes**
  - Add "Andromeda/Orion" style modes (decay >1000 seconds)
  - Implement slow-attack reverbs (Centaurus/Sagittarius-style)
  - Non-exponential decay curves (plateau → exponential)
  - Acceptance: 3+ new decay profiles in `Chambers.cpp`

- **P1-02b: Advanced Modulation**
  - Multi-phase sinusoidal modulation (Supermassive-style)
  - Warp-induced artifacts (harmonic delays, cascading shifts)
  - Sidechain modulation target (NightSky-style)
  - Acceptance: Modulation rates 0.06Hz-12Hz, depth 0-100%

**Dependencies:** P0-01 complete

---

### P1-03: Pitch-Shifted Reverb Core
**Domain:** `[DSP]` | **Effort:** 6h | **Agents:** 1

Implement NightSky-style variable process rate pitch manipulation.

**Acceptance Criteria:**
- [ ] Continuous SIZE/PITCH control (2.5 octave range)
- [ ] Quantize modes: Smooth, Half-Step, 11 scale types
- [ ] Glide/smoothing between pitch steps
- [ ] Regenerative shimmer (feedback pitch shift)
- [ ] No artifacts at extreme pitch values

**Dependencies:** P1-02 complete

---

### P1-04: Dual-Engine Layering System
**Domain:** `[DSP]` | **Effort:** 6h | **Agents:** 1

Implement BigSky MX-style parallel/series dual reverb engine routing.

**Acceptance Criteria:**
- [ ] Parallel mode: Input → [Engine A + Engine B] → Sum
- [ ] Series modes: A→B and B→A
- [ ] Split L|R mode: A→Left, B→Right
- [ ] Independent parameter sets per engine
- [ ] Cross-engine modulation (A modulates B's parameters)

**Dependencies:** P0-01 complete

---

### P1-05: Test Coverage Expansion
**Domain:** `[TEST]` | **Effort:** 8h | **Agents:** 2 (parallel)

**Subtasks (parallel):**
- **P1-05a: DSP Algorithm Tests**
  - FDN matrix orthogonality tests
  - Diffusion coefficient stability tests
  - Modulation phase coherence tests
  - Acceptance: 95%+ DSP function coverage

- **P1-05b: Audio Regression Suite**
  - Preset capture for all 37+ presets
  - RT60/frequency/spatial metrics comparison
  - Automated baseline drift detection (>3% change fails)
  - Acceptance: CI runs on every PR

**Dependencies:** None (can run parallel to DSP work)

---

### P1-06: Documentation Completion
**Domain:** `[DOC]` | **Effort:** 6h | **Agents:** 2 (parallel)

**Subtasks (parallel):**
- **P1-06a: Module Documentation**
  - Update `docs/architecture/dsp/00-index.md` with completion status
  - Add signal flow diagrams for all 9 modules
  - Document parameter ranges and DSP mappings
  - Acceptance: All modules have ≥500 word descriptions

- **P1-06b: User Documentation**
  - Quick start guide (setup, first sounds)
  - Preset gallery with audio examples
  - Macro mapping reference
  - Acceptance: README.md updated with current feature set

**Dependencies:** None (can run parallel to all work)

---

### P1-07: Fix pillarMode Parameter Gap (dead UI control)
**Domain:** `[DSP] [UI]` | **Effort:** 1h

Found via architecture review (2026-09-09). The `pillarMode` combo ("Glass/Stone/Fog") is loaded,
sanitized, and clamped every block in `PluginProcessor::processBlockCore`, but never passed to any
DSP module — `DspRoutingGraph::setPillarsParams()` has no slot for it. Moving this knob currently
does nothing audible; it's a real functional gap, not just a style issue.

**Acceptance Criteria:**
- [ ] Either wire `pillarMode` into `Pillars`/`DspRoutingGraph::setPillarsParams()` so it has an
      audible effect, or remove the control from the UI/APVTS if it's genuinely vestigial
- [ ] `monument_parameter_smoothing_test` still passes for this parameter either way

**Dependencies:** None

---

## P2: Medium Priority Tasks (Parallelizable: 8-10 agents)

### P2-01: UI Component System
**Domain:** `[UI]` | **Effort:** 12h | **Agents:** 3 (parallel)

**Subtasks (parallel):**
- **P2-01a: LayeredKnob Component**
  - Variants: geode/metal/industrial
  - States: default/hover/dragging/focus
  - Accessibility: keyboard navigation, screen reader
  - Acceptance: All knobs replace existing sliders

- **P2-01b: ParticleField Visualizer**
  - States: off/idle/reactive/audio-peak
  - Variants: embers/smoke/sparks
  - Performance: toggleable, <2% CPU at 60fps
  - Acceptance: Particles respond to RMS/peak levels

- **P2-01c: MacroCluster Layout**
  - 10-macro circular/linear arrangement
  - Visual profiles (colors from preset JSON)
  - Glyph hints and labels
  - Acceptance: Macros visible at 800×600 minimum

---

### P2-02: Preset Expansion
**Domain:** `[PRESET]` | **Effort:** 10h | **Agents:** 3 (parallel)

**Subtasks (parallel):**
- **P2-02a: Spaces Category (10 presets)**
  - Cathedral, Hall, Chamber, Room, Plate variants
  - Realistic architectural simulations
  - Acceptance: RT60 matches target space within 20%

- **P2-02b: Creative Category (10 presets)**
  - Shimmer, Reverse, Infinite, Freeze effects
  - Experimental/musical sound design
  - Acceptance: Each preset has unique macro mappings

- **P2-02c: Physical Category (8 presets)**
  - Spring, Tube, Elastic, Alien modeling
  - Showcase physical modules
  - Acceptance: Demonstrates module-specific features

---

### P2-03: Performance Optimization
**Domain:** `[PERF]` | **Effort:** 10h | **Agents:** 2 (parallel)

**Subtasks (parallel):**
- **P2-03a: SIMD Vectorization**
  - AVX-optimized 8×8 matrix multiplication
  - Vectorized fractional delay interpolation
  - Batch spatial calculations
  - Acceptance: 15-25% CPU reduction measured

- **P2-03b: Memory Optimization**
  - Cache-aligned buffer allocation
  - Relaxed memory ordering for independent params
  - Bitmask-based smoother tracking
  - Acceptance: <6% CPU @ 64 samples, zero dropouts

---

### P2-04: Advanced DSP Modules
**Domain:** `[DSP]` | **Effort:** 12h | **Agents:** 3 (parallel)

**Subtasks (parallel):**
- **P2-04a: Spectral Freeze Module**
  - FFT-based reverb tail manipulation
  - Harmonic freezing/morphing
  - Integration with MemoryEchoes
  - Acceptance: <3% CPU overhead at 44.1kHz

- **P2-04b: Convolution Layer**
  - Short IRs for early reflections
  - Algorithmic tail for infinite decay
  - IR morphing capabilities
  - Acceptance: IR load <100ms, memory <10MB

- **P2-04c: Multiband Processing**
  - 3-band crossover (user-configurable)
  - Independent decay per band
  - Tonal shaping controls
  - Acceptance: Crossover slopes 6/12/24 dB/oct

---

### P2-05: Testing Infrastructure
**Domain:** `[TEST]` | **Effort:** 8h | **Agents:** 2 (parallel)

**Subtasks (parallel):**
- **P2-05a: Visual Regression Testing**
  - UI screenshot capture (baseline/current)
  - Pixel-diff comparison with threshold
  - CI integration with failure reporting
  - Acceptance: Detects 1px changes, ignores anti-aliasing

- **P2-05b: Plugin Validation Suite**
  - pluginval integration (all strictness levels)
  - Parameter automation tests
  - State serialization tests
  - Acceptance: 100% pluginval tests pass

---

### P2-06: Dead/Unreachable Code Audit (processing modes, IR loading, knob utilities)
**Domain:** `[DSP] [UI]` | **Effort:** 4h | **Agents:** 2 (parallel)

Found via architecture review (2026-09-09). Several code paths have zero callers anywhere in the
repo (confirmed by full-repo grep, not just visual inspection). For each, decide whether to finish
wiring it up as an intended feature or remove it — don't leave it half-alive.

**Subtasks (parallel):**
- **P2-06a: Processing Mode Routing**
  - `MonumentAudioProcessor::setProcessingMode()`/`getProcessingMode()` (`plugin/PluginProcessor.h`)
    have zero callers anywhere, making `DspRoutingGraph::processResonantHalls()`/
    `processBreathingStone()` (~55 lines, `dsp/DspRoutingGraph.cpp`) unreachable in production
  - Decide: expose `ProcessingMode` selection somewhere in the UI/APVTS, or remove the unreachable
    code path
  - Acceptance: either a working UI control exercises all 3 modes, or the dead code is removed

- **P2-06b: Pillars IR Loading**
  - `Pillars::loadImpulseResponse()`/`clearImpulseResponse()` (`dsp/DspModules.cpp`) have zero
    callers anywhere; the `irLoaded` branch can never go true
  - Decide: wire up convolution-based early reflections as a real feature, or remove the dead branch
  - Acceptance: either a reachable, tested call site exists, or the code is removed

- **P2-06c: Orphaned PhotorealisticKnob Members**
  - `PhotorealisticKnob::loadFilmstrip()`, `setLabel()`, `clearLayerImages()`, `KnobGroup`,
    `ParameterDisplay` (`ui/PhotorealisticKnob.h`) — confirmed zero callers/instantiations across
    `plugin/`, `ui/`, `tests/`
  - Acceptance: removed, or a comment explains why they're kept for a specific planned use

**Dependencies:** None

---

### P2-07: Split dsp/DspModules.{h,cpp} by Concern
**Domain:** `[DSP]` | **Effort:** 2h

Found via architecture review (2026-09-09). `dsp/DspModules.h/cpp` (852 lines) bundles 5 unrelated
signal-chain stages (Foundation, Pillars, Weathering, Buttress, Facade) under a name that describes
none of them. `Pillars` alone is complex enough (IR loading, tap-based early reflections, mode
tuning) to read more clearly as its own file.

**Acceptance Criteria:**
- [ ] `Pillars` moved to its own `dsp/Pillars.{h,cpp}` (or similar)
- [ ] Foundation/Weathering/Buttress/Facade may stay bundled (small enough) or split further at the
      implementer's judgment
- [ ] `CMakeLists.txt`'s `DSP_SOURCES` updated; all existing tests pass unchanged

**Dependencies:** None

---

### P2-08: Real-Time-Safety Doc Contracts on Public Headers
**Domain:** `[DOC]` | **Effort:** 2h

Found via architecture review (2026-09-09). Public headers don't state real-time-safety contracts
where it matters most:
- `PluginProcessor.h`'s `getModulationMatrix()`/`getSequenceScheduler()` return live mutable
  references touched by the audio thread, with no concurrency note in the header itself (the
  actual safety mechanism — an announce/verify snapshot handshake — is documented only in
  `ModulationMatrix.cpp` and `ARCHITECTURE.md`)
- `PresetManager.h`'s `saveUserPreset`/`loadUserPreset` do file I/O with no "message-thread-only,
  not RT-safe" note

**Acceptance Criteria:**
- [ ] Each of the above methods has a doc comment stating its threading/RT-safety contract,
      discoverable from the header alone

**Dependencies:** None

---

### P2-09: processBlockCore Migration Scaffolding Cleanup
**Domain:** `[DSP] [PERF]` | **Effort:** 4h

Found via architecture review (2026-09-09). Leftover "Phase 4" migration scaffolding in
`plugin/PluginProcessor.cpp::processBlockCore` (per the code's own `// TEMPORARY (Step 3)` comment):
several parameter groups are filled into per-sample buffers via `SmoothedValue`, then immediately
re-averaged into a block-rate float that's either unused or duplicative of the per-sample buffer
already passed downstream.

**Subtasks:**
- **P2-09a: Remove unused computed values**
  - `timeModulated`, `gravityModulated`, `pillarShapeModulated`, `bloomEffective` are computed every
    block and never read (confirmed via full-file grep)
  - Acceptance: removed, or wired to their evident intended purpose (per-sample modulation, per the
    `TODO: Apply per-sample modulation in Step 8` comment on `makeModulatedView`)

- **P2-09b: Cache parameter pointers**
  - ~54 sequential `parameters.getRawParameterValue("id")` string-keyed atomic loads per block in
    `ParameterCache` population
  - Acceptance: cached `RangedAudioParameter*`/`std::atomic<float>*` pointers resolved once (e.g.
    in the constructor or `prepareToPlay`), avoiding per-block string lookups;
    `monument_parameter_smoothing_test` still passes

- **P2-09c: Deduplicate mix-gain cos/sin computation**
  - Equal-power dry/wet gain computed independently at 3 call sites in
    `plugin/PluginProcessor.cpp` (~line 1066-1069, ~1145-1150, ~1156-1166)
  - Acceptance: extracted to one small inline helper called from all 3 sites; no behavior change

**Dependencies:** None (independent of P2-06/07/08)

---

## P3: Low Priority / Experimental (Unlimited parallelization)

### P3-01: Machine Learning Features
**Domain:** `[DSP] [PERF]` | **Effort:** 20h+ | **Agents:** Unlimited

- Room IR prediction from parameters
- Preset recommendation system
- Adaptive processing based on input content
- ML-based denoising in reverb tail

**Status:** Research phase — no implementation started

---

### P3-02: Physical Modeling Expansion
**Domain:** `[DSP]` | **Effort:** 15h+ | **Agents:** Unlimited

- String/membrane resonators
- Waveguide networks
- Finite element room modeling
- Karplus-Strong integration

**Status:** Ideation — requires research

---

### P3-03: Cloud Ecosystem
**Domain:** `[BUILD]` | **Effort:** 25h+ | **Agents:** Unlimited

- Preset cloud sync
- Community preset sharing
- Collaborative sound design
- Versioned preset library

**Status:** Not started — depends on distribution strategy

---

### P3-04: Plugin Suite Expansion
**Domain:** `[DSP]` | **Effort:** 50h+ | **Agents:** Unlimited

**Monument Suite Family:**
- Monument Delay (advanced delay network)
- Monument Modulation (creative effects)
- Monument Spatial (3D audio processor)
- Monument Live (performance-optimized)
- Monument Creative (experimental, no stability guarantees)

**Status:** Long-term vision (2-5 years)

---

## Task Execution Model

### For Parallel Agents

1. **Pick a task** from the highest priority level with available agent slots
2. **Read dependencies** — ensure prerequisite tasks are complete
3. **Implement** with test coverage ≥80%
4. **Run local tests** before submitting
5. **Update task status** in this document

### Task Status Format

```markdown
### TASK-ID: Task Name
**Domain:** `[TAG]` | **Effort:** Xh | **Agents:** N (parallel)
**Status:** `pending` | `in_progress` | `review` | `complete`
**Assignee:** @agent-id (optional)
**PR:** #XXX (when created)
```

### Completion Checklist

For each task marked `complete`:

- [ ] Code changes committed with conventional commit message
- [ ] Tests added/updated (if applicable)
- [ ] Documentation updated (if applicable)
- [ ] No new compiler warnings
- [ ] No new audio-thread allocations (verified)
- [ ] PR reviewed and merged (if applicable)

---

## Current Session Context

**Active Tasks:** Reverb quality alignment with ambient references (Supermassive, BigSky MX, NightSky)

**Completed This Session:**
- ✅ Repository cleanup (merged branches, pruned stale refs)
- ✅ Local main synchronized with origin/main
- ✅ Reverb quality analysis report generated

**Next Actions:**
1. ✅ P1-01 (Ambient Reverb Quality Alignment) — complete, see task entry above
2. P1-07 (Fix pillarMode Parameter Gap) — small, do first: a real functional gap (dead UI control)
3. P2-06 through P2-09 (architecture-review cleanup: dead code audit, DspModules split, RT-safety
   doc contracts, processBlockCore scaffolding cleanup) — found 2026-09-09, no dependencies between
   them or on anything else, safe to pick up in any order
4. Execute P1-05 (Test Coverage Expansion) — 2 agents
5. Execute P1-06 (Documentation Completion) — 2 agents

---

## Metrics Dashboard

| Metric | Current | Target | Status |
|--------|---------|--------|--------|
| CTest Pass Rate | See TESTING.md | 100% | 🟡 In Progress |
| RT-Safety | ✅ Verified | ✅ Maintained | 🟢 Pass |
| CPU @ 64 samples | <10% | <6% | 🟡 In Progress |
| Preset Count | 37 | 50+ | 🟡 In Progress |
| Documentation Coverage | ~80% | 100% | 🟡 In Progress |
| UI Component Completion | ~60% | 100% | 🟡 In Progress |

---

**For session handoff, see:** [`docs/NEXT_SESSION_HANDOFF.md`](docs/NEXT_SESSION_HANDOFF.md)
**For DSP architecture, see:** [`docs/architecture/dsp/00-index.md`](docs/architecture/dsp/00-index.md)
**For test status, see:** [`TESTING.md`](TESTING.md)
