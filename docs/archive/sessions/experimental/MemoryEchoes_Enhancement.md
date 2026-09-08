# MemoryEchoes Enhancement Plan (Implementation + Roadmap)

**Date:** 2026-01-10  
**Status:** Draft  
**Scope:** MemoryEchoes (Strata) integration + enhancement roadmap

---

## Purpose

This plan operationalizes MemoryEchoes (Strata) as a routable, macro-aware, and modulation-ready module while preserving the realtime-safe architecture defined in:

- `docs/architecture/MEMORY_ECHOES_INTEGRATION.md`
- `docs/architecture/dsp/memory-system/11-strata.md`

It also captures the product-facing "Memory Echoes" summary so the system can be described consistently in codex, README, or design docs.

---

## Product Summary (codex/README ready)

**Memory Echoes** is a software-centric audio effect system that combines temporal modulation, sample memory, and evolving feedback structures to create textured, organic echoes and soundscapes. It treats incoming audio as a mutable memory field rather than a fixed delay line, allowing past material to resurface, morph, and blend into the present in evolving ways.

**What it does**
- Captures audio into dynamic memory buffers with variable read and write behavior.
- Replays and transforms stored audio using pitch drift, filtering, and time warping.
- Crossfades between memory regions to create evolving textures instead of static repeats.
- Uses modulation (LFO, random, envelope-followed) to introduce organic variation.

**Inspirations (and how we differ)**
- **Soma Cosmos**: shares an interest in non-linear, unpredictable echoes but Memory Echoes uses programmable memory buffers instead of analog chaos circuitry.
- **Chase Bliss Audio Habit**: shares a focus on evolving delay textures, but Memory Echoes is built on a mutable memory core with broader spectral and granular manipulation options.

**Positioning**
Memory Echoes sits between experimental modular effects and deeply programmable digital processors, aimed at ambient sound design, organic delay textures, and evolving feedback-rich spaces.

---

## Alignment with Existing Architecture

**From `11-strata.md` (Strata / MemoryEchoes baseline)**
- Dual buffers: 24s stereo (short) + 180s mono (long) with exponential decay.
- State machine: Idle -> FadeIn -> Hold -> FadeOut -> Cooldown.
- Age-based processing: lowpass + saturation + drift modulation.
- Default injection path: recall is mixed back into the processing buffer with a gain scalar.
- CPU/memory envelope: ~0.5% capture, ~1-2% recall active, ~39 MB total.

**From `MEMORY_ECHOES_INTEGRATION.md` (integration expectations)**
- Make MemoryEchoes routable in `DspRoutingGraph`.
- Map macros to memory parameters.
- Add modulation destinations for memory controls.
- Add UI controls in the Advanced panel.
- Provide routing presets that showcase placement differences.

**Naming**
The code remains `MemoryEchoes`, while documentation and UX may label the module as **Strata**. A later refactor can rename the class while preserving parameter IDs.

---

## Implementation Plan

### Phase 0: Baseline Verification (no behavior change)

- Confirm MemoryEchoes passes existing tests: `tests/MemoryEchoesTest.cpp`, `tests/MemoryEchoesHarness.cpp`.
- Confirm realtime safety: no logging in audio process paths.
- Verify parameter smoothing targets match 11-strata (300-450ms range).

**Exit criteria**
- MemoryEchoes unit tests pass.
- No allocations or logging in `MemoryEchoes::process` or `captureWet`.

---

### Phase 1: Routing Graph Integration

**Goal:** Make MemoryEchoes a first-class routable module.

**File touchpoints**
- `dsp/DspRoutingGraph.h`
- `dsp/DspRoutingGraph.cpp`
- `plugin/PluginProcessor.cpp` (re-enable integration)

**Key changes**
- Add `ModuleType::MemoryEchoes` to routing enums.
- Store `std::unique_ptr<MemoryEchoes> memoryEchoes`.
- During processing, the MemoryEchoes module should:
  1. `memoryEchoes->captureWet(buffer)` using the buffer at the module position.
  2. `memoryEchoes->process(buffer)` to surface recall into that same buffer.
- Add `setMemoryEchoesParams(float memory, float depth, float decay, float drift)`.
- Update routing presets to include memory placements (Ghostly Cathedral, Fragmented Reality, Recursive Haunt, Metallic Memories, Elastic Memories).

**Routing-specific caveat**
If MemoryEchoes appears in parallel branches, avoid double-capture in the same block. Guard capture with a per-block flag inside the routing graph or inside MemoryEchoes (eg. `captureOncePerBlock`).

**Exit criteria**
- Routing presets can place MemoryEchoes in multiple positions.
- Audio still flows in all three processing modes.

---

### Phase 2: Macro + Modulation Integration

**Goal:** Expose MemoryEchoes to expressive macro mapping and modulation matrix.

**Macro mapping (from integration doc)**
- Energy -> Memory Amount
- Motion -> Memory Drift
- Character -> Memory Depth
- Color -> Memory Decay

**File touchpoints**
- `dsp/ExpressiveMacroMapper.h`
- `dsp/ExpressiveMacroMapper.cpp`
- `dsp/ModulationMatrix.h`
- `dsp/ModulationMatrix.cpp`

**Notes**
- Keep modulation destinations block-rate (not per-sample) to match existing MemoryEchoes smoother design.
- Ensure modulation targets clamp to [0, 1] and are smoothed before hitting MemoryEchoes setters.

**Exit criteria**
- Macros change memory behavior audibly.
- Modulation sources (audio follower, chaos) can drive memory parameters.

---

### Phase 3: UI Controls + Visual Feedback

**Goal:** Provide user access and visual feedback for memory behavior.

**UI additions (Advanced panel)**
- Memory Amount, Depth, Decay, Drift knobs
- Optional "Memory Active" LED or meter driven by recall RMS

**UI data bridge**
- Avoid reading `recallBuffer` on the UI thread.
- Use a lock-free RMS meter or FIFO to share recall activity.

**Exit criteria**
- Advanced panel exposes memory controls with automation support.
- Visual feedback is stable and realtime-safe.

---

### Phase 4: Preset and Documentation Updates

**Goal:** Showcase routing placement differences and provide reference use cases.

**Preset additions**
- Ghostly Cathedral: Memory after Chambers.
- Fragmented Reality: Memory before Chambers.
- Recursive Haunt: Memory in feedback loop.
- Metallic Memories: Memory after TubeRayTracer.
- Elastic Memories: Memory after ElasticHallway.

**Documentation**
- Update routing preset docs.
- Add MemoryEchoes examples to experimental preset notes.

---

### Phase 5: Validation and Performance

**Smoke tests**
- Verify memory surfacing changes by placement (before vs after Chambers).
- Confirm macro mappings hit expected ranges.
- Validate modulation gating and probability controls.

**Performance checks**
- Capture path stays near ~0.5% CPU.
- Recall active stays under ~2% CPU at 48kHz/512 samples.
- Memory footprint remains ~39 MB unless expanded by enhancements.

---

## Enhancement Roadmap

### Near-term (Phase 6)

1. **Memory Capture Triggers**
   - Manual trigger and RMS-based auto-capture.
2. **Stereo Memory Panning**
   - Parameterized stereo spread, modulated by chaos or LFO.
3. **Memory Grain Size**
   - Surface window length control (50ms to 2000ms).

### Mid-term (Phase 7)

1. **Energy-Age Tracking**
   - Preferential recall of high-energy events.
2. **Multi-Resolution Buffers**
   - 3-4 memory layers for long-term texture.
3. **SIMD Capture**
   - Vectorized buffer writes for lower CPU overhead.

### Long-term / Research (Phase 8)

1. **Spectral Freezing**
   - FFT-based memory layers with phase/magnitude morphing.
2. **User-Configurable Memory Durations**
   - UI-exposed buffer lengths with safe reallocation strategy.
3. **Rename MemoryEchoes -> Strata**
   - Preserve parameter IDs and preset compatibility.

---

## Risks and Mitigations

| Risk | Mitigation |
| --- | --- |
| Double-capture in parallel routing | Per-block capture guard in routing graph |
| Memory usage growth with new layers | Add budget guardrails + optional downsampling |
| UI thread access to recall buffer | Use lock-free RMS meter or FIFO |
| Routing feedback runaway | Clamp feedback gain, retain safety clipping stages |

---

## Success Criteria

- MemoryEchoes is routable in the graph and usable in multiple placements.
- Macros and modulation control memory in a musical, predictable way.
- Advanced UI exposes memory parameters and activity feedback.
- Performance stays within the documented CPU/memory budgets.

---

## Open Questions

1. Should MemoryEchoes capture the buffer before or after any local feedback loop in custom routing?
2. Should recall always inject into the local buffer, or optionally feed a separate return bus?
3. When renaming to Strata, do we preserve `MemoryEchoes` as a type alias for plugin state compatibility?

