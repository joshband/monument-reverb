# P1-01: Ambient Reverb Quality Implementation

**Status:** ✅ Complete
**Domain:** `[DSP]`
**Effort:** 8h (original estimate) + a substantial fix/redesign pass (see History)
**Reference:** Valhalla Supermassive, BigSky MX, NightSky (inspiration for the feature set; no numeric claims below are measured against those plugins)

---

## Summary

Extended Chambers' FDN reverb core from 8 to 12 delay lines and added three
ambient-shaping controls: warp-clustering mode (how the 12 lines relate to
each other), density evolution (echo density drift over the decay), and
slow attack (a gradual swell instead of an instant onset). All three are
wired to host-automatable parameters.

The initial implementation (see History below) did not compile, and three
of its features were silent no-ops. This document describes the final,
verified state after fixing that work — see `git log` on
`dsp/p1-01-ambient-reverb-quality` for the full sequence of fixes.

---

## Changes Implemented

### 1. Extended FDN Delay Network

**Before:** 8 delay lines, max ~1.23s at 48kHz
**After:** 12 delay lines, max ~6.5s at 48kHz (the base/Incommensurate case)

```cpp
constexpr std::array<int, 12> kDelaySamples48k{
    2411, 4201, 7001, 11003, 17011, 26003, 39019, 59009,
    89003, 135007, 205003, 310019
};
```

Extending the feedback matrix from 8×8 to 12×12 required real matrix work,
not a resize: the order-8 Hadamard matrix uses a Sylvester construction
that only works for powers of 2, so order 12 needed a different,
independently-verified construction (Paley, over GF(11)) — verified
numerically that `H·Hᵀ = 12·I`. The Householder matrix generalizes to any
N algebraically, so that one just needed the formula updated.

### 2. Warp-Clustering Modes

```cpp
enum class WarpClusteringMode
{
    Incommensurate = 0,  // Prime-based, non-repeating (default) -- each
                          // line keeps its own base delay, unrelated to
                          // the others
    Harmonic2x,           // Each line is a small-integer ratio (1-32x)
                          // of one shared fundamental delay
    Harmonic3x,           // Each line is an odd ratio (1-23x) of the
                          // fundamental
    OctaveStack           // Each line is a power-of-2 ratio (0.25x-512x,
                          // i.e. -2 to +9 octaves) of the fundamental
};

void setWarpClusteringMode(WarpClusteringMode mode);
```

**Design note (this deviated from the initial implementation):** ratios
apply to a single shared fundamental (the shortest base delay), not to
each line's own already-different base delay. The initial version
multiplied each line's own base by the ratio, which compounds two wide
ranges (base delays span ~129x across lines, ratios span up to 2048x) into
an unbounded worst case — the longest base times the largest ratio came
out to ~635M samples, no realistic buffer holds that, and a buffer-safety
clamp made most lines collapse onto the identical clamped maximum instead
of the intended distinct spacing. A shared fundamental is also what
"harmonic"/"octave" relationships actually mean musically (multiples of
one reference, not of twelve unrelated ones), and it bounds the result:
`kOctaveRatios` tops out at 512x (not the original 2048x) so the worst
case fits a deliberate delay-buffer budget instead of an unbounded one.

**Click safety:** recomputing all 12 delay lengths on a mode switch is a
discontinuous jump in a live feedback delay network. `setWarpClusteringMode`
schedules the change; the actual swap happens at the midpoint of a 10ms
mute-and-crossfade window (5ms out, swap, 5ms back in), verified directly
via a test-only accessor rather than inferred from audio level (the FDN's
own inter-line interference makes a level comparison an unreliable proxy
for this).

### 3. Density Evolution

```cpp
// evolution = -1: decreasing density (smooth -> grainy) over the decay
// evolution =  0: constant density (traditional reverb, the default)
// evolution = +1: increasing density (grainy -> smooth)
void setDensityEvolution(float evolution);
```

Applies a factor (derived from elapsed envelope time) to the density value
that feeds the diffusion-strength calculations, with a one-sample lag
(the factor for sample N is computed from sample N's envelope time and
applied starting sample N+1, since the diffusion calculations for a given
sample already consume `densityNorm` before that sample's envelope time is
known). Verified with a test comparing the reverb tail 6 seconds into the
decay under `evolution=-1` vs `evolution=0` (5.85% RMS difference,
comfortably above the 1% no-effect threshold).

### 4. Slow-Attack Reverb

```cpp
// attackTimeNorm = 0: instant attack (traditional, the default)
// attackTimeNorm = 1: ~10 second slow attack (ambient swell)
void setAttackTime(float attackTimeNorm);
```

An envelope multiplier that starts at 0 on each new transient and ramps to
1 over the configured time, applied to the wet signal only (a real dry
passthrough must not duck on every note in continuous use, so it's
deliberately excluded). Verified with a test comparing the tail's level a
few hundred milliseconds after an impulse under `attackTime=1` vs `=0`
(the slow-attack case measured ~3.4% of the instant-attack level).

### 5. Extended Parameter Arrays

All FDN-related arrays extended from 8 to 12 elements (`kLateDiffuserSamples48k`,
`kFeedbackDiffuserSamples48k`, `kDampingOffsets`, `kLateDiffuserCoeffOffsets`,
`kFeedbackDiffuserCoeffOffsets`, `kInputMid`, `kInputSide`, `kOutputLeft`,
`kOutputRight`); `kOutputGain` renormalized from 0.5 to 0.408 for the new
line count.

---

## Acceptance Criteria

| Criterion | Target | Status |
|-----------|--------|--------|
| FDN delay lines | 12 lines | ✅ |
| Max delay time | >2 seconds | ✅ (~6.5s at the base setting) |
| RT60 range | 4s - 120s | ❌ Not delivered — see below |
| Harmonic clustering modes | 4 modes, all 12 lines musically distinct | ✅ |
| Density evolution | -1 to +1 range, measurable effect | ✅ |
| Attack time | 0-10 seconds, measurable effect | ✅ |
| Backward compatible | Existing parameters unchanged | ✅ |
| Host-automatable | Wired to APVTS | ✅ |

**RT60 range (4s-120s) was not delivered.** The initial implementation
added a `kMaxFeedbackAmbient` constant intended to extend the feedback
coefficient's ceiling, but never actually used it anywhere — the feedback
calculation still uses the pre-existing `kMaxFeedback` (0.995) cap. This
was dead code (confirmed via the compiler's own unused-variable warning)
rather than a working feature, so it was removed rather than half-wired.
RT60 today is unchanged from before this task: 2-35s, verified by
`monument_reverb_dsp_test`'s existing impulse-response test. Extending it
would mean deciding a new feedback ceiling and re-verifying stability at
the higher value — a follow-up task, not something bundled into this one.

---

## Testing

Four new tests in `tests/ReverbDspTest.cpp` (not `ChambersAmbientTest.cpp`,
which never existed), each verified with a genuine failing-then-passing
cycle against the bug it targets, not written after the fact:

- `testDensityEvolutionAffectsOutput` — catches the density-evolution dead
  code (failed with 0.000000% difference before the fix)
- `testAttackTimeProducesSlowSwell` — catches the attack-time dead code
- `testWarpClusteringModeSwitchIsClickFree` — verifies the mute-guard
  envelope directly via `getWarpClusteringMuteGainForTesting()`
- `testHarmonicClusteringModesKeepLinesDistinct` — catches the
  fundamental-vs-per-line-base regression (failed with lines collapsing to
  an identical clamped delay before the redesign)

Plus the pre-existing `monument_reverb_dsp_test` suite (RT60, late-tail
decay, DC offset, stereo decorrelation, freeze stability, parameter-jump
smoothness) and `monument_parameter_smoothing_test` (sweeps every
registered APVTS parameter, including the three new ones, for clicks).

No preset-comparison or audio-regression baseline was captured for this
task — the acceptance criteria never specified one beyond RT60, and RT60
itself didn't change from this task's DSP work (see above).

---

## Usage Examples

### Ambient Pad Texture

```cpp
chambers.setAttackTime(0.7f);           // ~7 second attack
chambers.setDensityEvolution(-0.5f);    // Smooth -> grainy
chambers.setWarpClusteringMode(Chambers::WarpClusteringMode::Harmonic2x);
chambers.setTime(0.9f);                 // Long decay
chambers.setDensity(0.3f);              // Sparse initial density
```

### Resonant Metallic Space

```cpp
chambers.setWarpClusteringMode(Chambers::WarpClusteringMode::OctaveStack);
chambers.setDensityEvolution(0.3f);     // Grainy -> smooth
chambers.setDrift(0.4f);                // Detune effect
chambers.setWarp(0.5f);                 // Mixed topology
```

### Traditional Cathedral (Backward Compatible)

```cpp
chambers.setWarpClusteringMode(Chambers::WarpClusteringMode::Incommensurate);
chambers.setDensityEvolution(0.0f);     // Constant density (traditional)
chambers.setAttackTime(0.0f);           // Instant attack
// Matches pre-P1-01 behavior exactly (these are all the defaults)
```

---

## Performance Impact

Not independently measured for this task. The feedback-matrix multiply
grew from 64 to 144 scalar multiplies per sample (8×8 to 12×12); SIMD
vectorization was dropped rather than generalized (12 doesn't divide
evenly into an 8-wide register, though it would into three 4-wide groups
— left as a documented follow-up, not implemented, since correctness took
priority and the scalar cost wasn't shown to be a problem). No CPU
percentage or memory-footprint measurement was taken before/after; treat
any such numbers from earlier drafts of this document as unverified
guesses, not data.

---

## History

The version of this feature originally merged into this document (and
into `dsp/Chambers.{h,cpp}` as uncommitted work) did not compile, and had
three features that were pure no-ops despite looking complete in code
review (a computed value that was never applied, an envelope that never
left its initial value, and a `reset()` that silently discarded
user-set targets). Fixing that work — compile errors, the dead code
above, the harmonic-ratio collapse, and several smaller bugs found in an
independent code-review pass — is fully described in the commit messages
on the `dsp/p1-01-ambient-reverb-quality` branch. This document describes
the end state after those fixes, not the original draft.

---

## Files Modified

- `dsp/Chambers.h` — `WarpClusteringMode` enum, new parameters, mute-guard
  and test-only accessor state
- `dsp/Chambers.cpp` — extended arrays, order-12 Hadamard/Householder
  matrices, new parameter implementations, click-safe mode-switch logic
- `plugin/PluginProcessor.{h,cpp}` — `warpClustering`/`densityEvolution`/
  `attackTime` APVTS parameters, wired once per block
- `tests/ReverbDspTest.cpp` — the six new tests described above under Testing
