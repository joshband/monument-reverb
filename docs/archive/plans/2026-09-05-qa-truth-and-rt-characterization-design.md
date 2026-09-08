# QA Truth and Realtime Characterization Design

## Goal

Make Monument's current validation limitations visible, then prove the two
known allocation paths with focused regression tests before changing DSP
behavior.

## Scope

This increment has two independently reviewable commits:

1. Change the pinned `audio-dsp-qa-harness` so a performance invariant with
   unavailable or invalid metrics is reported as a non-passing `soft_warn`,
   never as a pass.
2. Add an allocation-characterization executable that measures the next
   processor block after a timeline preset change and the next TubeRayTracer
   block after each count transition.

## Constraints

- Do not change the rendered DSP algorithms, routing mode, parameter IDs,
  factory program order, preset format, or CI policy.
- Do not claim that a global `new` counter covers every platform allocation.
- The allocation executable must compile without `MONUMENT_TESTING`, which
  enables logging in an audio path.
- The harness warning must preserve the declared invariant severity as
  `soft_warn` for unmeasured data, while valid measured data retains the
  scenario's configured severity and thresholds.
- The harness patch and Monument gitlink update remain a single reviewable
  Monument commit after the harness submodule has its own commit.

## Design

`InvariantEvaluator` currently treats a null performance-metrics pointer as a
successful skipped invariant. It will instead emit a failed result with
`soft_warn` severity and an `unmeasured:` message. The evaluator must apply
the same result to invalid performance data, because zero-valued invalid data
can otherwise satisfy a threshold. This makes suite and CLI status visibly
warn without falsely making platform-dependent timing a hard failure.

The allocation executable uses a scoped global `new`/`new[]` counter only
around the call under test. It warms initialization before each measured call.
The processor case changes `timelinePreset` through the APVTS and measures the
following `processBlock`; the direct DSP case crosses every integer
TubeRayTracer count boundary and measures the following `process`. These tests
are expected to fail on the pinned source and become the behavior contract for
the next runtime-remediation increment.

## Validation

- Harness unit tests cover null and invalid metrics, plus valid zero-allocation
  metrics.
- The harness performance-invariant suite is executed at its pinned commit.
- Monument's allocation executable is built with `MONUMENT_TESTING` absent and
  run once to record the current failure evidence. It is not weakened to pass.
- Existing focused processor/timeline tests are run as a regression check.

## Out of Scope

- Wiring actual scenario profiling, changing warning/failure CI policy, or
  adding performance scenarios to pull-request CI.
- Eliminating the timeline, TubeRayTracer, or oversized-block allocation paths.
- State/preset compatibility and processor-routing characterization.
