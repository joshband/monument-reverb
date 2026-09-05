# QA Truth and Realtime Characterization Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make unavailable QA performance metrics visible and establish repeatable tests for the current timeline and tube-count allocation paths.

**Architecture:** The QA warning correction belongs in the pinned harness evaluator and its native unit tests. Monument consumes that change as a submodule gitlink update. A separate Monument console target records allocation behavior around real processor and TubeRayTracer calls without test-mode logging.

**Tech Stack:** C++17, JUCE, Catch2 (harness), CMake/CTest, Git submodules.

**Spec:** `docs/superpowers/specs/2026-09-05-qa-truth-and-rt-characterization-design.md`

## Global Constraints

- Preserve rendered DSP behavior, routing, parameter IDs, factory program order, preset formats, and CI policy.
- Keep unavailable or invalid performance data a visible `soft_warn`; do not make timing hard-fail until measurement is deterministic.
- Compile allocation characterization without `MONUMENT_TESTING`.
- Every production-code change follows a failing test first; allocation tests remain failing evidence until the later remediation scope is approved.
- Do not push, merge, publish, or alter the backup checkout.

---

## File Structure

- `external/audio-dsp-qa-harness/scenario_engine/invariant_evaluator.cpp` owns
  performance-invariant result construction.
- `external/audio-dsp-qa-harness/tests/performance_invariant_test.cpp` owns
  evaluator regressions for measured and unmeasured data.
- `external/audio-dsp-qa-harness` gitlink records the reviewed harness commit.
- `tests/RealtimeAllocationCharacterizationTest.cpp` owns scoped allocation
  counters and real processing probes.
- `CMakeLists.txt` owns the new no-test-mode console target and CTest entry.

### Task 1: Expose unavailable QA performance measurements

**Files:**
- Modify: `external/audio-dsp-qa-harness/scenario_engine/invariant_evaluator.cpp:260-289`
- Modify: `external/audio-dsp-qa-harness/tests/performance_invariant_test.cpp:276-292`
- Modify: `external/audio-dsp-qa-harness` gitlink

**Interfaces:**
- Consumes: `std::unique_ptr<PerformanceMetrics>` on `ScenarioResult`.
- Produces: an `InvariantResult` with `passed == false`, `severity == "soft_warn"`, and an `unmeasured:` message when metrics are absent or invalid.

- [ ] **Step 1: Write failing harness unit cases** for null and invalid metrics that require a non-passing soft warning; retain a valid zero-allocation pass case.
- [ ] **Step 2: Run the focused harness test** and verify it fails because null metrics still pass.
- [ ] **Step 3: Implement the minimal evaluator change** that emits the non-passing soft warning for null or invalid performance metrics.
- [ ] **Step 4: Run the focused harness tests** and verify the expected measured/unmeasured behavior.
- [ ] **Step 5: Commit the harness change** with its test evidence, then update Monument's gitlink and commit that update separately.

### Task 2: Characterize timeline and TubeRayTracer allocations

**Files:**
- Create: `tests/RealtimeAllocationCharacterizationTest.cpp`
- Modify: `CMakeLists.txt` beside the existing console test targets

**Interfaces:**
- Consumes: `MonumentAudioProcessor::prepareToPlay/processBlock`, APVTS
  `timelinePreset`, and `TubeRayTracer::prepare/setTubeCount/process`.
- Produces: a CTest executable which returns non-zero and names the allocation
  count when either measured call allocates.

- [ ] **Step 1: Write the executable with scoped global allocation counting** around the real post-warmup processing calls, one timeline case and one tube-count-boundary case.
- [ ] **Step 2: Add a CMake console target without `MONUMENT_TESTING`** and register it as `monument_realtime_allocation_characterization_test`.
- [ ] **Step 3: Configure and build only this target** with Monument tests enabled; fix compile/test-harness defects without changing DSP behavior.
- [ ] **Step 4: Run the executable** and record the expected non-zero allocation evidence for the pinned source.
- [ ] **Step 5: Commit the characterization test and CMake registration** even though it is red by design; document the exact expected failure in the commit and task report.

## Final Verification

- [ ] Confirm the two commits have no DSP, parameter, preset, routing, or CI-policy changes.
- [ ] Re-run the harness performance unit test and record its exit status.
- [ ] Re-run the allocation characterization executable and record the observed counts and exit status.
- [ ] Run existing timeline integration and focused state/parameter tests where their targets are available.
- [ ] Inspect `git diff` and `git status`, including submodule state.
