#!/bin/bash
# Monument Reverb - Batch Preset Analysis Script
# Analyzes RT60, frequency response, and spatial metrics for all captured presets
# Supports parallel execution for faster processing

set -e  # Exit on error

# Configuration
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
INPUT_BASE="${INPUT_BASE:-$PROJECT_ROOT/test-results/preset-baseline}"
if [[ "$INPUT_BASE" != /* ]]; then
    INPUT_BASE="$PROJECT_ROOT/$INPUT_BASE"
fi
PYTHON_RT60="$PROJECT_ROOT/tools/plugin-analyzer/python/rt60_analysis_robust.py"
PYTHON_FREQ="$PROJECT_ROOT/tools/plugin-analyzer/python/frequency_response.py"
PYTHON_SPATIAL="$PROJECT_ROOT/tools/plugin-analyzer/python/spatial_metrics.py"
NUM_PRESETS=37

PYTHON_BIN="${PYTHON_BIN:-}"
if [ -z "$PYTHON_BIN" ]; then
    if [ -n "${VENV_PATH:-}" ] && [ -x "$VENV_PATH/bin/python" ]; then
        PYTHON_BIN="$VENV_PATH/bin/python"
    elif [ -x "$PROJECT_ROOT/.venv/bin/python" ]; then
        PYTHON_BIN="$PROJECT_ROOT/.venv/bin/python"
    else
        PYTHON_BIN="python3"
    fi
fi

PIP_CMD=("$PYTHON_BIN" -m pip)

# Parallel execution (default: use all CPU cores)
# Set PARALLEL_JOBS=1 to disable parallel processing
detect_parallel_jobs() {
    if command -v sysctl >/dev/null 2>&1; then
        sysctl -n hw.ncpu 2>/dev/null && return 0
    fi
    if command -v getconf >/dev/null 2>&1; then
        getconf _NPROCESSORS_ONLN 2>/dev/null && return 0
    fi
    if command -v nproc >/dev/null 2>&1; then
        nproc 2>/dev/null && return 0
    fi
    echo 1
}

PARALLEL_JOBS=${PARALLEL_JOBS:-$(detect_parallel_jobs)}
MAX_PARALLEL=8  # Cap at 8 to avoid overwhelming the system
PARALLEL_JOBS=$((PARALLEL_JOBS > MAX_PARALLEL ? MAX_PARALLEL : PARALLEL_JOBS))

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Check if Python scripts exist
if [ ! -f "$PYTHON_RT60" ]; then
    echo -e "${RED}Error: RT60 analysis script not found at $PYTHON_RT60${NC}"
    exit 1
fi

if [ ! -f "$PYTHON_FREQ" ]; then
    echo -e "${RED}Error: Frequency response script not found at $PYTHON_FREQ${NC}"
    exit 1
fi

if [ ! -f "$PYTHON_SPATIAL" ]; then
    echo -e "${RED}Error: Spatial metrics script not found at $PYTHON_SPATIAL${NC}"
    exit 1
fi

# Check if input directory exists
if [ ! -d "$INPUT_BASE" ]; then
    echo -e "${RED}Error: Input directory not found: $INPUT_BASE${NC}"
    echo "Run: ./scripts/capture_all_presets.sh"
    exit 1
fi

# Check for Python dependencies
if ! "$PYTHON_BIN" -c "import pyroomacoustics, numpy, scipy, matplotlib" 2>/dev/null; then
    echo -e "${YELLOW}Installing Python dependencies...${NC}"
    "${PIP_CMD[@]}" install -q pyroomacoustics numpy scipy matplotlib
fi

# Print header
echo ""
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}  Monument Reverb - Batch Preset Analysis${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""
echo -e "Input:    ${GREEN}${INPUT_BASE}${NC}"
echo -e "Presets:  ${GREEN}0-36 (${NUM_PRESETS} total)${NC}"
echo -e "Parallel: ${GREEN}${PARALLEL_JOBS} workers${NC}"
echo ""

# Count valid captures
VALID_COUNT=0
for i in $(seq 0 36); do
    PRESET_DIR="${INPUT_BASE}/preset_$(printf "%02d" $i)"
    if [ -f "${PRESET_DIR}/wet.wav" ]; then
        VALID_COUNT=$((VALID_COUNT + 1))
    fi
done

echo -e "${YELLOW}Found ${VALID_COUNT}/${NUM_PRESETS} valid captures${NC}"
echo ""
if [ -z "${NON_INTERACTIVE:-}" ] && [ -z "${CI:-}" ]; then
    read -p "Press Enter to continue or Ctrl+C to cancel..."
    echo ""
else
    echo "Non-interactive mode: skipping prompt."
    echo ""
fi

# Function to analyze a single preset (will be called in parallel)
analyze_preset() {
    local i=$1
    local PRESET_DIR="${INPUT_BASE}/preset_$(printf "%02d" $i)"
    local WET_WAV="${PRESET_DIR}/wet.wav"

    # Check if capture exists
    if [ ! -f "$WET_WAV" ]; then
        echo "[Preset ${i}] ✗ Skipped - no capture file"
        return 1
    fi

    echo "[Preset ${i}] Analyzing..."

    # Run RT60 analysis
    if "$PYTHON_BIN" "$PYTHON_RT60" "$WET_WAV" \
        --output "${PRESET_DIR}/rt60_metrics.json" \
        > "${PRESET_DIR}/rt60_analysis.log" 2>&1; then
        echo "[Preset ${i}] ✓ RT60 analysis complete"
    else
        echo "[Preset ${i}] ⚠ RT60 analysis failed (see rt60_analysis.log)"
    fi

    # Run frequency response analysis
    if "$PYTHON_BIN" "$PYTHON_FREQ" "$WET_WAV" \
        --impulse \
        --output "${PRESET_DIR}/freq_metrics.json" \
        > "${PRESET_DIR}/freq_analysis.log" 2>&1; then
        echo "[Preset ${i}] ✓ Frequency response complete"

        # Move auto-generated plot to standard location
        AUTO_PLOT="${PRESET_DIR}/wet_frequency_response.png"
        if [ -f "$AUTO_PLOT" ]; then
            mv "$AUTO_PLOT" "${PRESET_DIR}/frequency_response.png"
        fi
    else
        echo "[Preset ${i}] ⚠ Frequency response failed (see freq_analysis.log)"
    fi

    # Run spatial metrics analysis
    if "$PYTHON_BIN" "$PYTHON_SPATIAL" "$WET_WAV" \
        --output "${PRESET_DIR}/spatial_metrics.json" \
        > "${PRESET_DIR}/spatial_analysis.log" 2>&1; then
        echo "[Preset ${i}] ✓ Spatial metrics complete"
    else
        echo "[Preset ${i}] ⚠ Spatial metrics failed (see spatial_analysis.log)"
    fi

    return 0
}

# Export function and variables for parallel execution
export -f analyze_preset
export INPUT_BASE PYTHON_RT60 PYTHON_FREQ PYTHON_SPATIAL PYTHON_BIN

# Track statistics
START_TIME=$(date +%s)

# Generate list of preset indices and process in parallel
echo "Starting parallel analysis with ${PARALLEL_JOBS} workers..."
echo ""

# Use xargs for parallel execution
if seq 0 36 | xargs -P "$PARALLEL_JOBS" -I {} bash -c 'analyze_preset "$@"' _ {}; then
    echo ""
    echo "All analysis jobs completed"
else
    echo ""
    echo "Some analysis jobs failed (see individual logs)"
fi

# Calculate elapsed time
END_TIME=$(date +%s)
ELAPSED=$((END_TIME - START_TIME))
MINUTES=$((ELAPSED / 60))
SECONDS=$((ELAPSED % 60))

# Count successes
RT60_SUCCESS=0
FREQ_SUCCESS=0
SPATIAL_SUCCESS=0
for i in $(seq 0 36); do
    PRESET_DIR="${INPUT_BASE}/preset_$(printf "%02d" $i)"
    [ -f "${PRESET_DIR}/rt60_metrics.json" ] && RT60_SUCCESS=$((RT60_SUCCESS + 1))
    [ -f "${PRESET_DIR}/freq_metrics.json" ] && FREQ_SUCCESS=$((FREQ_SUCCESS + 1))
    [ -f "${PRESET_DIR}/spatial_metrics.json" ] && SPATIAL_SUCCESS=$((SPATIAL_SUCCESS + 1))
done

# Print summary
echo ""
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}  Analysis Complete!${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""
echo -e "RT60 Analysis:       ${GREEN}${RT60_SUCCESS}/${VALID_COUNT}${NC}"
echo -e "Freq Analysis:       ${GREEN}${FREQ_SUCCESS}/${VALID_COUNT}${NC}"
echo -e "Spatial Analysis:    ${GREEN}${SPATIAL_SUCCESS}/${VALID_COUNT}${NC}"
echo -e "Time:                ${YELLOW}${MINUTES}m ${SECONDS}s${NC}"
echo ""

if [ $RT60_SUCCESS -eq $VALID_COUNT ] && [ $FREQ_SUCCESS -eq $VALID_COUNT ] && [ $SPATIAL_SUCCESS -eq $VALID_COUNT ]; then
    echo -e "${GREEN}✓ All presets analyzed successfully!${NC}"
    echo ""
    echo -e "Results saved to:"
    echo -e "  ${BLUE}${INPUT_BASE}/preset_*/rt60_metrics.json${NC}"
    echo -e "  ${BLUE}${INPUT_BASE}/preset_*/freq_metrics.json${NC}"
    echo -e "  ${BLUE}${INPUT_BASE}/preset_*/frequency_response.png${NC}"
    echo ""
    echo -e "Next steps:"
    echo -e "  1. Generate summary report: ${BLUE}python3 tools/generate_preset_report.py${NC}"
    echo -e "  2. Create comparison plots: ${BLUE}python3 tools/plot_preset_comparison.py${NC}"
    echo ""
    exit 0
else
    echo -e "${YELLOW}⚠ Some analyses failed${NC}"
    echo -e "Check individual logs in ${INPUT_BASE}/preset_*/*_analysis.log"
    echo ""
    exit 1
fi
