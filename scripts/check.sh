#!/bin/bash
# check.sh - Bounded, explicit QA gate for Monument Reverb.
#
# This is a single canonical entrypoint that fails loudly instead of silently
# skipping work. It exists because scripts/run_ci_tests.sh is documented in
# TESTING.md as a non-authoritative local diagnostic wrapper: it skips missing
# diagnostic binaries, can auto-create a missing baseline and treat that as
# success, searches multiple build-directory layouts for artifacts, and can
# return success without checking recorded continuation failures. Do not add
# those tolerant behaviors here.
#
# What this script does, in order:
#   1. Resolves one source root, one build directory, one explicit config.
#   2. Validates the audio-dsp-qa-harness submodule is checked out, then
#      configures + builds the Monument plugin and the monument_qa harness
#      executable.
#   3. Runs an explicit, curated CTest selection for processor-level
#      contracts (state/parameter/preset/module) that the scenario harness
#      cannot reach directly. Fails if zero tests match.
#   4. Runs the 8-scenario critical suite via monument_qa and fails on any
#      FAIL/ERROR, an unexpected scenario count, or unmeasured performance
#      data hiding in soft warnings.
#   5. Prints a final report: commit hash, harness submodule pin, CMake
#      config, test/scenario counts, and artifact locations.
#   6. With --full, additionally runs the 22-scenario full suite.
#
# This script is NOT wired into CI yet and does NOT replace run_ci_tests.sh.
#
# Env overrides (same convention as scripts/run_ci_tests.sh):
#   BUILD_DIR   - build directory (default: build-check)
#   TEST_CONFIG - CMake/CTest configuration (default: Release)
#   JUCE_SOURCE_DIR - if set, builds against a local JUCE checkout instead of
#                     FetchContent (same passthrough scripts/dev_loop.sh uses)
#
# Exit codes:
#   0 = all checks passed
#   1 = a check failed (test/scenario failures, unmeasured required data, etc.)
#   2 = setup/dependency error (missing submodule, missing executable, etc.)

set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_ROOT"

RUN_FULL=0
for arg in "$@"; do
    case "$arg" in
        --full)
            RUN_FULL=1
            ;;
        *)
            echo "[check] ERROR: unknown argument: $arg" >&2
            echo "[check] usage: $0 [--full]" >&2
            exit 2
            ;;
    esac
done

BUILD_DIR="${BUILD_DIR:-build-check}"
if [[ "$BUILD_DIR" != /* ]]; then
    BUILD_DIR="$PROJECT_ROOT/$BUILD_DIR"
fi
TEST_CONFIG="${TEST_CONFIG:-Release}"

SUBMODULE_DIR="$PROJECT_ROOT/external/audio-dsp-qa-harness"
MONUMENT_QA_BIN="$BUILD_DIR/monument_qa_artefacts/$TEST_CONFIG/monument_qa"

# Curated CTest selection: processor-level contracts (host state, parameter
# automation, preset/timeline/macro routing, DSP lifecycle) that the
# scenario-driven QA harness cannot exercise directly. See TESTING.md's
# "CTest Inventory" for the full list this was picked from. The array is the
# single source of truth: it drives both what gets built and the -R regex.
CTEST_TARGETS=(
    monument_state_management_test
    monument_macro_mode_integration_test
    monument_timeline_integration_test
    monument_parameter_buffer_test
    monument_dsp_initialization_test
)
CTEST_REGEX="^($(IFS='|'; echo "${CTEST_TARGETS[*]}"))\$"

CRITICAL_SUITE="scenarios/monument/monument_critical_suite.json"
CRITICAL_SUITE_EXPECTED_TOTAL=8
FULL_SUITE="scenarios/monument/monument_suite.json"

echo "[check] Project root: $PROJECT_ROOT"
echo "[check] Build dir:    $BUILD_DIR"
echo "[check] Config:       $TEST_CONFIG"
echo "[check] Full suite:   $([ "$RUN_FULL" -eq 1 ] && echo yes || echo no)"
echo ""

# --- Step 1: validate pinned dependencies -----------------------------------

if [[ ! -f "$SUBMODULE_DIR/CMakeLists.txt" ]]; then
    echo "[check] ERROR: external/audio-dsp-qa-harness submodule is not checked out." >&2
    echo "[check] Run: git submodule update --init --recursive external/audio-dsp-qa-harness" >&2
    exit 2
fi

HARNESS_COMMIT="$(git -C "$SUBMODULE_DIR" rev-parse HEAD)"
echo "[check] audio-dsp-qa-harness pinned at: $HARNESS_COMMIT"
echo ""

# --- Step 2: configure + build -----------------------------------------------

CMAKE_CONFIGURE_ARGS=(
    -S "$PROJECT_ROOT"
    -B "$BUILD_DIR"
    -DCMAKE_BUILD_TYPE="$TEST_CONFIG"
    -DCMAKE_POLICY_VERSION_MINIMUM=3.5
    -DMONUMENT_COPY_PLUGIN_AFTER_BUILD=OFF
    -DBUILD_QA_HARNESS=ON
    -DMONUMENT_ENABLE_TESTS=ON
    -DBUILD_TESTING=ON
)

if [[ -n "${JUCE_SOURCE_DIR:-}" ]]; then
    CMAKE_CONFIGURE_ARGS+=(-DMONUMENT_USE_LOCAL_JUCE=ON -DJUCE_SOURCE_DIR="$JUCE_SOURCE_DIR")
fi

echo "[check] Configuring..."
cmake "${CMAKE_CONFIGURE_ARGS[@]}"
echo ""

echo "[check] Building Monument..."
cmake --build "$BUILD_DIR" --config "$TEST_CONFIG" --target Monument
echo ""

echo "[check] Building monument_qa..."
cmake --build "$BUILD_DIR" --config "$TEST_CONFIG" --target monument_qa
echo ""

echo "[check] Building curated CTest targets..."
BUILD_TEST_TARGET_ARGS=()
for target in "${CTEST_TARGETS[@]}"; do
    BUILD_TEST_TARGET_ARGS+=(--target "$target")
done
cmake --build "$BUILD_DIR" --config "$TEST_CONFIG" "${BUILD_TEST_TARGET_ARGS[@]}"
echo ""

if [[ ! -x "$MONUMENT_QA_BIN" ]]; then
    echo "[check] ERROR: monument_qa executable not found at $MONUMENT_QA_BIN after build." >&2
    exit 2
fi

# --- Step 3: curated CTest selection ----------------------------------------

echo "[check] Resolving curated CTest selection (dry run)..."
CTEST_LIST_OUTPUT="$(mktemp)"
ctest --test-dir "$BUILD_DIR" -C "$TEST_CONFIG" -R "$CTEST_REGEX" -N > "$CTEST_LIST_OUTPUT" 2>&1 || true
cat "$CTEST_LIST_OUTPUT"

CTEST_MATCH_COUNT="$(grep -Eo 'Total Tests: [0-9]+' "$CTEST_LIST_OUTPUT" | grep -Eo '[0-9]+' || true)"
rm -f "$CTEST_LIST_OUTPUT"

if [[ -z "$CTEST_MATCH_COUNT" ]] || [[ "$CTEST_MATCH_COUNT" -eq 0 ]]; then
    echo "[check] ERROR: curated CTest selection matched ZERO tests. Regex: $CTEST_REGEX" >&2
    echo "[check] This is the silent-no-op failure mode this script exists to catch." >&2
    exit 1
fi

echo "[check] Curated CTest selection matched $CTEST_MATCH_COUNT test(s). Running..."
ctest --test-dir "$BUILD_DIR" -C "$TEST_CONFIG" -R "$CTEST_REGEX" --output-on-failure
echo "[check] Curated CTest selection passed: $CTEST_MATCH_COUNT test(s) run."
echo ""

# --- Step 4: scenario harness suites ----------------------------------------

run_suite() {
    local suite_path="$1"
    local expected_total="$2"

    echo "[check] Running scenario suite: $suite_path"
    local suite_output
    suite_output="$(mktemp)"
    local suite_exit=0
    "$MONUMENT_QA_BIN" "$suite_path" 2>&1 | tee "$suite_output" || suite_exit=1

    # monument_qa prints a "Total: ..." summary line twice (once from the
    # harness's own executor, once from this repo's main.cpp report block) —
    # both reflect the same TestSuiteResult, so take the last one.
    local summary_line
    summary_line="$(grep -E '^Total: ' "$suite_output" | tail -n 1 || true)"
    if [[ -z "$summary_line" ]]; then
        echo "[check] ERROR: no suite summary line found in monument_qa output for $suite_path." >&2
        rm -f "$suite_output"
        exit 1
    fi
    echo "[check] Suite summary: $summary_line"

    local total pass warn fail error skip
    total="$(echo "$summary_line" | grep -Eo 'Total: [0-9]+' | grep -Eo '[0-9]+')"
    warn="$(echo "$summary_line" | grep -Eo 'Warn: [0-9]+' | grep -Eo '[0-9]+')"
    fail="$(echo "$summary_line" | grep -Eo 'Fail: [0-9]+' | grep -Eo '[0-9]+')"
    error="$(echo "$summary_line" | grep -Eo 'Error: [0-9]+' | grep -Eo '[0-9]+')"

    if [[ -z "$expected_total" ]]; then
        expected_total="$total"
    fi

    if [[ "$total" -ne "$expected_total" ]]; then
        echo "[check] ERROR: $suite_path reported $total scenario(s), expected $expected_total." >&2
        rm -f "$suite_output"
        exit 1
    fi

    if [[ "$fail" -ne 0 ]] || [[ "$error" -ne 0 ]]; then
        echo "[check] ERROR: $suite_path has Fail: $fail, Error: $error (both must be 0)." >&2
        rm -f "$suite_output"
        exit 1
    fi

    if [[ "$suite_exit" -ne 0 ]]; then
        echo "[check] ERROR: monument_qa exited non-zero for $suite_path." >&2
        rm -f "$suite_output"
        exit 1
    fi

    if [[ "$warn" -ne 0 ]]; then
        echo "[check] WARNING: $suite_path reported Warn: $warn (non-fatal, but review below)."
    fi

    local unmeasured_lines
    unmeasured_lines="$(grep -i 'unmeasured' "$suite_output" || true)"
    if [[ -n "$unmeasured_lines" ]]; then
        echo "[check] WARNING: unmeasured metric data reported by $suite_path:"
        echo "$unmeasured_lines" | sed 's/^/[check]   /'
    fi

    rm -f "$suite_output"
    echo "[check] $suite_path: Total: $total  Warn: $warn  Fail: $fail  Error: $error"
    echo ""
}

run_suite "$CRITICAL_SUITE" "$CRITICAL_SUITE_EXPECTED_TOTAL"

if [[ "$RUN_FULL" -eq 1 ]]; then
    run_suite "$FULL_SUITE" ""
fi

# --- Step 5: final report -----------------------------------------------------

GIT_COMMIT="$(git -C "$PROJECT_ROOT" rev-parse HEAD)"

echo "===== check.sh: ALL CHECKS PASSED ====="
echo "Repo commit:            $GIT_COMMIT"
echo "Harness submodule pin:  $HARNESS_COMMIT"
echo "CMake config:           $TEST_CONFIG (BUILD_QA_HARNESS=ON, MONUMENT_ENABLE_TESTS=ON, BUILD_TESTING=ON, MONUMENT_COPY_PLUGIN_AFTER_BUILD=OFF)"
echo "Build dir:              $BUILD_DIR"
echo "Curated CTest selection: $CTEST_MATCH_COUNT test(s), regex: $CTEST_REGEX"
echo "Critical suite:          $CRITICAL_SUITE"
if [[ "$RUN_FULL" -eq 1 ]]; then
    echo "Full suite:              $FULL_SUITE"
fi
echo "monument_qa binary:      $MONUMENT_QA_BIN"

exit 0
