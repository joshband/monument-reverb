# Monument Reverb - Stress Testing Guide

How to run and extend Monument's stress/performance test coverage: CPU
profiling, parameter-extremity stress, and concurrency hazards.

**Scope note:** An earlier version of this document was a forward-looking
test plan for six proposed test files (`PerformanceBenchmarkTest.cpp`,
`ParameterStressTest.cpp`, `LongDurationTest.cpp`, `EdgeCaseTest.cpp`,
`RealTimeSafetyTest.cpp`, `NumericalStabilityTest.cpp`) and several
supporting scripts, all marked "done" with checkmarks. In the current
source tree, only the first two of those test files exist and are wired
into CTest; `LongDurationTest.cpp`, `EdgeCaseTest.cpp`,
`RealTimeSafetyTest.cpp`, `NumericalStabilityTest.cpp`,
`scripts/run_full_stress_tests.sh`, and `scripts/run_long_tests.sh` do not
exist in this repository. This rewrite describes what actually exists and
how to use it, and keeps the original category catalogue further down as a
backlog/reference rather than a status report.

---

## What Actually Exists Today

| CTest target | Source | What it does |
|---|---|---|
| `monument_performance_benchmark` | [tests/PerformanceBenchmarkTest.cpp](../../tests/PerformanceBenchmarkTest.cpp) | CPU/memory/SIMD profiling across DSP modules. Registered with `--quick` for CTest. |
| `monument_parameter_stress_test` | [tests/ParameterStressTest.cpp](../../tests/ParameterStressTest.cpp) | Extreme parameter values, rapid automation, zipper-noise and click detection. Registered with `--quick` for CTest. |
| `monument_modulation_matrix_concurrency_stress_test` | [tests/ModulationMatrixConcurrencyStressTest.cpp](../../tests/ModulationMatrixConcurrencyStressTest.cpp) | Audio-thread-vs-message-thread race on `ModulationMatrix`'s connection double-buffer. See [MODULATION_TESTING_GUIDE.md](MODULATION_TESTING_GUIDE.md#concurrency-stress-test-in-detail). |
| `monument_realtime_allocation_characterization_test` | [tests/RealtimeAllocationCharacterizationTest.cpp](../../tests/RealtimeAllocationCharacterizationTest.cpp) | Characterizes real allocations in specific `processBlock` paths. **Deliberately failing by design** — see [TESTING.md](../../TESTING.md)'s CTest Inventory section for the current status of this known-red test; do not "fix" it without reading that note first. |

### Running them

```sh
# Build must be configured with -DMONUMENT_ENABLE_TESTS=ON -DBUILD_TESTING=ON
ctest --test-dir build -C Release -R "performance_benchmark|parameter_stress|concurrency_stress"
```

Or run a binary directly for full (non `--quick`) output:

```sh
./build/monument_performance_benchmark_artefacts/Debug/monument_performance_benchmark
./build/monument_parameter_stress_test_artefacts/Debug/monument_parameter_stress_test
```

`monument_performance_benchmark` supports `--quick`, `--cpu-only`, and
`--mem-only`. `monument_parameter_stress_test` supports `--quick` for a
30-second subset; omit it for the full suite.

### Concurrency stress test under ThreadSanitizer

`monument_modulation_matrix_concurrency_stress_test` is a best-effort race
detector under a plain build; build it with `-DMONUMENT_TSAN=ON` in a
dedicated build directory to get a verified characterization instead of an
inferred one — see
[MODULATION_TESTING_GUIDE.md](MODULATION_TESTING_GUIDE.md#concurrency-stress-test-in-detail)
for the exact commands.

---

## Extending: Adding a New Stress Test

1. Decide whether the new test belongs in an existing file
   (`PerformanceBenchmarkTest.cpp` for CPU/memory/SIMD questions,
   `ParameterStressTest.cpp` for parameter-extremity/automation questions)
   or needs a new file — a new file needs its own `juce_add_console_app` +
   `add_test` block in `CMakeLists.txt` (see the existing blocks for these
   two targets as a template).
2. Follow the existing pattern in both files: a `TestResult` struct, ANSI
   color-coded pass/fail output, and a `main()` that runs a list of named
   checks and returns nonzero on any failure (so CTest sees it as a
   failure).
3. If the new test can run in a bounded time (a few seconds), register it
   with `add_test` so it runs under plain `ctest`. If it needs minutes or
   hours (e.g. a long-duration stability soak), gate it behind an
   environment variable check inside the test binary (the pattern proposed
   for `LONG-*` tests below was `MONUMENT_RUN_LONG_TESTS=1`) rather than
   registering it as a normal CTest target — a bounded local QA gate
   (`scripts/check.sh`) and CI both need every registered test to complete
   quickly.
4. Update the table above once the target is wired into CTest.

---

## Backlog: Stress Test Categories Not Yet Implemented

The following categories were proposed in the original version of this
plan and have **not** been implemented as of this rewrite. They are kept
here as a reference for anyone picking this work up — treat every row as
"not built" rather than assuming partial coverage exists under a similar
name.

### Category 1: Performance Benchmarks

Substantially covered today by `monument_performance_benchmark`. Specific
proposed sub-checks not yet present: cache-line/L1 miss measurement
(`CACHE-1`), explicit worst-case-execution-time budget check (`WCET-1`),
and lock-contention/priority-inversion detection (`THREAD-1`, `THREAD-2`).

### Category 2: Parameter Stress Tests

Substantially covered today by `monument_parameter_stress_test`. Specific
proposed sub-checks not yet present: an explicit "47 params changing
simultaneously" automation-storm test (`PARAM-6`) and an
invalid-parameter-value clamping test (`PARAM-14`).

### Category 3: Long-Duration Stability (not implemented)

1-hour and 24-hour soak tests (silence, continuous tone, random noise,
freeze mode, automation loop, denormal accumulation, DC offset drift). No
`LongDurationTest.cpp` exists. If implemented, follow the
`MONUMENT_RUN_LONG_TESTS=1` gating pattern above so CI/`check.sh` aren't
stuck running hour-long tests by default.

### Category 4: Edge Case Scenarios (not implemented)

Zero-length buffers, mono/multichannel input, live sample-rate/block-size
changes, denormal/DC/Nyquist/Inf/NaN input handling, bypass transitions. No
`EdgeCaseTest.cpp` exists.

### Category 5: Real-Time Safety (partially covered elsewhere)

Allocation detection has a real, if narrow, implementation today in
`monument_realtime_allocation_characterization_test` (see table above) and
in `scripts/check_rt_allocations.sh` (Instruments-based, macOS-only, opt-in
via `ENABLE_RT_ALLOCATION_CHECK=1` in `run_ci_tests.sh`). System-call
detection, lock/mutex detection, virtual-call overhead, exception-safety
auditing, unbounded-loop detection, and stack-usage/thread-affinity checks
are not implemented as automated tests — they are covered only by the
"Absolute Rules" code-review discipline in the project's `CLAUDE.md`.

### Category 6: Numerical Stability (not implemented)

Denormal performance, float-precision, filter-coefficient stability,
fixed-point overflow, trig precision, exponential monotonicity, and
feedback-matrix spectral-radius checks. No `NumericalStabilityTest.cpp`
exists. `scripts/check_audio_stability.py` covers a related but different
concern (NaN/Inf/denormal/DC-offset detection on *captured preset audio*,
not on internal DSP state).

---

## Success Criteria (target, not currently enforced everywhere)

These were the original targets for a complete stress-test suite. Only the
subset actually measured by `monument_performance_benchmark` and
`monument_parameter_stress_test` today is enforced by CTest; the rest are
aspirational until the corresponding tests above are implemented.

- **CPU Usage:** < 30% full chain at 48kHz, 512 samples
- **Memory:** Zero allocations in the audio callback (partially
  characterized — see `monument_realtime_allocation_characterization_test`,
  which is currently a known-red/deliberately-failing test)
- **Zipper noise:** > -40dB rejection during rapid parameter sweeps
- **Click detection:** > -30dB rejection on instant parameter jumps
- **Energy growth:** < ±6dB over any 1-minute window (long-duration,
  unimplemented)
- **DC offset:** < 0.001 absolute value

---

## See Also

- [TESTING.md](../../TESTING.md) - canonical testing hub, CTest inventory, DSP QA authority policy
- [MODULATION_TESTING_GUIDE.md](MODULATION_TESTING_GUIDE.md) - modulation-specific testing (including the concurrency stress test)
- [scripts/README.md](../../scripts/README.md) - `profile_cpu.sh`, `profile_with_audio.sh`, `check_rt_allocations.sh`, and other supporting scripts that exist today
