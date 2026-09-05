# Task 1 report: expose unavailable QA performance measurements

## Result

Performance invariants now report unavailable or invalid profiling data as a
non-passing `soft_warn` with an `unmeasured:` message. Valid metrics continue
through the existing threshold evaluator.

## Red/green evidence

The focused harness build was configured with:

```text
cmake -S . -B /tmp/monument-task1-harness-build -DBUILD_QA_TESTS=ON -DCMAKE_BUILD_TYPE=Debug -DCMAKE_POLICY_VERSION_MINIMUM=3.5
```

The policy flag was required because the fetched yaml-cpp dependency uses a
pre-3.5 `cmake_minimum_required` declaration rejected by the installed CMake.

After adding the null and invalid metric cases, before the evaluator change:

```text
cmake --build /tmp/monument-task1-harness-build --target performance_invariant_test -j2
/tmp/monument-task1-harness-build/performance_invariant_test "[perf_invariant]"
```

```text
test cases: 14 | 12 passed | 2 failed
assertions: 68 | 66 passed | 2 failed

InvariantEvaluator: perf is unmeasured when profiling data is absent
REQUIRE_FALSE(result.invariantResults[0].passed)
with expansion: !true

InvariantEvaluator: perf is unmeasured when profiling data is invalid
REQUIRE_FALSE(result.invariantResults[0].passed)
with expansion: !true
```

After the evaluator change and updating the mixed audio/performance
characterization:

```text
cmake --build /tmp/monument-task1-harness-build --target performance_invariant_test -j2
/tmp/monument-task1-harness-build/performance_invariant_test "[perf_invariant]"
```

```text
All tests passed (75 assertions in 14 test cases)
```

`git diff --check` also passed before commit.

## Commits

- Harness: `e7c0a23bd3e62ec976074654b5dc7035629afca2` (`fix: report unmeasured performance invariants`)
- Monument parent gitlink: `b9e183f1fd6b67a380535f9502952923af8f2b56` (`chore: update QA harness performance reporting`)

## Changed files

- `external/audio-dsp-qa-harness/scenario_engine/invariant_evaluator.cpp`
  - Treats null or `valid == false` performance metrics as `passed == false`,
    `severity == "soft_warn"`, with a reason-specific `unmeasured:` message.
- `external/audio-dsp-qa-harness/tests/performance_invariant_test.cpp`
  - Adds `evaluateInto` coverage for null, invalid, and valid zero-allocation
    metrics and updates the mixed audio/performance expectation.
- `external/audio-dsp-qa-harness` gitlink
  - Updated by the parent commit above.

## Concerns

- The focused build emits existing third-party/dependency warnings; they did
  not affect the test result.
- The test writes a small fixed-name temporary WAV under the system temporary
  directory and overwrites it on each case; this is local test data only.
- No DSP code, CI policy, or performance thresholds were changed.
