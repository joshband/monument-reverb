#!/bin/bash
# run_ci_tests.sh - CI/CD wrapper for Monument Reverb automated testing
#
# This script runs all automated tests and returns proper exit codes for CI.
# Designed to run on GitHub Actions, GitLab CI, or any CI/CD system.
#
# Exit codes:
#   0 = All tests passed
#   1 = Tests failed
#   2 = Setup/dependency error

set -e  # Exit on any error

# Color output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TEST_RESULTS_DIR="$PROJECT_ROOT/test-results"
BASELINE_DIR="${BASELINE_DIR:-$TEST_RESULTS_DIR/baseline-ci}"
CURRENT_DIR="$TEST_RESULTS_DIR/preset-baseline"
THRESHOLD="${THRESHOLD:-0.01}"  # 1% tolerance default

echo -e "${GREEN}===== Monument Reverb CI Test Suite =====${NC}"
echo "Project Root: $PROJECT_ROOT"
echo "Baseline: $BASELINE_DIR"
echo "Threshold: ${THRESHOLD}%"
echo ""

# Check dependencies
echo "Checking dependencies..."
command -v python3 >/dev/null 2>&1 || {
    echo -e "${RED}ERROR: python3 not found${NC}"
    exit 2
}

command -v cmake >/dev/null 2>&1 || {
    echo -e "${RED}ERROR: cmake not found${NC}"
    exit 2
}

# Check Python packages
python3 -c "import pyroomacoustics" 2>/dev/null || {
    echo -e "${YELLOW}WARNING: pyroomacoustics not installed, installing...${NC}"
    pip3 install pyroomacoustics numpy scipy matplotlib
}

# Check UI testing dependencies if enabled
if [ "${ENABLE_UI_TESTS:-0}" = "1" ]; then
    python3 -c "import pyautogui" 2>/dev/null || {
        echo -e "${YELLOW}WARNING: pyautogui not installed for UI tests, installing...${NC}"
        pip3 install pyautogui pillow numpy
    }
fi

# Check if plugin analyzer is built
ANALYZER_PATH="$PROJECT_ROOT/build/monument_plugin_analyzer_artefacts/Debug/monument_plugin_analyzer"
if [ ! -f "$ANALYZER_PATH" ]; then
    ANALYZER_PATH="$PROJECT_ROOT/build/monument_plugin_analyzer_artefacts/monument_plugin_analyzer"
fi

if [ ! -f "$ANALYZER_PATH" ]; then
    echo -e "${RED}ERROR: monument_plugin_analyzer not found${NC}"
    echo "Please build it first: cmake --build build --target monument_plugin_analyzer"
    exit 2
fi

echo -e "${GREEN}✓ All dependencies found${NC}"
echo ""

# Run C++ unit tests if available
if [ -d "$PROJECT_ROOT/build" ]; then
    echo "Running C++ unit tests..."
    cd "$PROJECT_ROOT"

    if ctest --test-dir build --output-on-failure 2>&1 | tee /tmp/ctest-output.txt; then
        echo -e "${GREEN}✓ C++ unit tests passed${NC}"
    else
        echo -e "${RED}✗ C++ unit tests failed${NC}"
        cat /tmp/ctest-output.txt
        exit 1
    fi
    echo ""
fi

# Capture all presets
echo "Capturing all 37 presets..."
cd "$PROJECT_ROOT"
if ./scripts/capture_all_presets.sh; then
    echo -e "${GREEN}✓ Preset capture complete${NC}"
else
    echo -e "${RED}✗ Preset capture failed${NC}"
    exit 1
fi
echo ""

# Analyze audio quality
echo "Analyzing audio quality..."
if ./scripts/analyze_all_presets.sh; then
    echo -e "${GREEN}✓ Audio analysis complete${NC}"
else
    echo -e "${RED}✗ Audio analysis failed${NC}"
    exit 1
fi
echo ""

# Validate baseline data integrity
echo "Validating baseline data integrity..."
if [ -d "$CURRENT_DIR" ]; then
    if python3 "$PROJECT_ROOT/tools/validate_baseline.py" "$CURRENT_DIR"; then
        echo -e "${GREEN}✓ Baseline validation passed${NC}"
    else
        echo -e "${RED}✗ Baseline validation failed${NC}"
        echo ""
        echo "The baseline data has integrity issues."
        echo "Please review the validation report above and fix the issues."
        exit 1
    fi
else
    echo -e "${YELLOW}WARNING: Current test results directory not found: $CURRENT_DIR${NC}"
    echo "Skipping baseline validation."
fi
echo ""

# Validate JSON schemas (schema v1.0.0 format)
echo "Validating JSON schemas against formal specifications..."
if [ -d "$CURRENT_DIR" ]; then
    # Note: This validates against JSON Schema v1.0.0 format
    # Legacy data without 'version' field will fail validation (expected)
    if python3 "$PROJECT_ROOT/tools/validate_schemas.py" "$CURRENT_DIR" 2>&1 | tail -20; then
        echo -e "${GREEN}✓ Schema validation passed${NC}"
    else
        echo -e "${YELLOW}⚠ Schema validation failed (non-blocking)${NC}"
        echo "Note: Existing baseline uses legacy format without 'version' field."
        echo "This will be enforced once data is migrated to schema v1.0.0 format."
        echo "See docs/schemas/README.md for schema details."
    fi
else
    echo -e "${YELLOW}WARNING: Current test results directory not found: $CURRENT_DIR${NC}"
    echo "Skipping schema validation."
fi
echo ""

# Check if baseline exists
if [ ! -d "$BASELINE_DIR" ]; then
    echo -e "${YELLOW}No baseline found. Creating initial baseline...${NC}"
    mkdir -p "$(dirname "$BASELINE_DIR")"
    cp -r "$CURRENT_DIR" "$BASELINE_DIR"
    echo -e "${GREEN}✓ Baseline created at $BASELINE_DIR${NC}"
    echo ""
    echo -e "${GREEN}===== All Tests Passed (Baseline Created) =====${NC}"
    exit 0
fi

# Compare against baseline (regression test)
echo "Running regression tests..."
if python3 "$PROJECT_ROOT/tools/compare_baseline.py" \
    "$BASELINE_DIR" \
    "$CURRENT_DIR" \
    --threshold "$THRESHOLD" \
    --output "$TEST_RESULTS_DIR/regression-report.json"; then
    echo -e "${GREEN}✓ No regressions detected${NC}"
else
    echo -e "${RED}✗ Regression detected!${NC}"
    echo ""
    echo "Regression report:"
    cat "$TEST_RESULTS_DIR/regression-report.json" 2>/dev/null || echo "(Report not generated)"
    exit 1
fi

# Run UI visual regression tests (optional)
if [ "${ENABLE_UI_TESTS:-0}" = "1" ]; then
    echo ""
    echo "Running UI visual regression tests..."

    # Check if standalone app is built
    STANDALONE_PATH="$PROJECT_ROOT/build/Monument_artefacts/Debug/Standalone/Monument.app"
    if [ ! -d "$STANDALONE_PATH" ]; then
        echo -e "${YELLOW}WARNING: Standalone app not found, skipping UI tests${NC}"
        echo "Build it with: cmake --build build --target Monument_Standalone"
    else
        if python3 "$PROJECT_ROOT/tools/test_ui_visual.py" \
            --baseline-dir "$TEST_RESULTS_DIR/ui-baseline" \
            --output-dir "$TEST_RESULTS_DIR/ui-current" \
            --threshold 0.02; then
            echo -e "${GREEN}✓ UI visual tests passed${NC}"
        else
            echo -e "${RED}✗ UI visual tests failed${NC}"
            echo "View report: open $TEST_RESULTS_DIR/ui-current/report.html"
            exit 1
        fi
    fi
fi

echo ""
echo -e "${GREEN}===== All Tests Passed =====${NC}"
echo "Results: $TEST_RESULTS_DIR"
echo "Report: $TEST_RESULTS_DIR/regression-report.json"
if [ "${ENABLE_UI_TESTS:-0}" = "1" ]; then
    echo "UI Report: $TEST_RESULTS_DIR/ui-current/report.html"
fi

exit 0
