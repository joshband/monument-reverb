#!/bin/bash
# Monument Reverb - Batch Preset Analysis Script
# Analyzes RT60 and frequency response for all captured presets
# Supports parallel execution for faster processing

set -e  # Exit on error

# Configuration
INPUT_BASE="./test-results/preset-baseline"
PYTHON_RT60="./tools/plugin-analyzer/python/rt60_analysis_robust.py"
PYTHON_FREQ="./tools/plugin-analyzer/python/frequency_response.py"
NUM_PRESETS=37

# Parallel execution (default: use all CPU cores)
PARALLEL_JOBS=${PARALLEL_JOBS:-$(sysctl -n hw.ncpu)}
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

# Check if input directory exists
if [ ! -d "$INPUT_BASE" ]; then
    echo -e "${RED}Error: Input directory not found: $INPUT_BASE${NC}"
    echo "Run: ./scripts/capture_all_presets.sh"
    exit 1
fi

# Check for Python dependencies
if ! python3 -c "import pyroomacoustics, numpy, scipy, matplotlib" 2>/dev/null; then
    echo -e "${YELLOW}Installing Python dependencies...${NC}"
    pip3 install -q pyroomacoustics numpy scipy matplotlib
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
read -p "Press Enter to continue or Ctrl+C to cancel..."
echo ""

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
    if python3 "$PYTHON_RT60" "$WET_WAV" \
        --output "${PRESET_DIR}/rt60_metrics.json" \
        > "${PRESET_DIR}/rt60_analysis.log" 2>&1; then
        echo "[Preset ${i}] ✓ RT60 analysis complete"
    else
        echo "[Preset ${i}] ⚠ RT60 analysis failed (see rt60_analysis.log)"
    fi

    # Run frequency response analysis
    if python3 "$PYTHON_FREQ" "$WET_WAV" \
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

    return 0
}

# Export function and variables for parallel execution
export -f analyze_preset
export INPUT_BASE PYTHON_RT60 PYTHON_FREQ

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
for i in $(seq 0 36); do
    PRESET_DIR="${INPUT_BASE}/preset_$(printf "%02d" $i)"
    [ -f "${PRESET_DIR}/rt60_metrics.json" ] && RT60_SUCCESS=$((RT60_SUCCESS + 1))
    [ -f "${PRESET_DIR}/freq_metrics.json" ] && FREQ_SUCCESS=$((FREQ_SUCCESS + 1))
done

# Print summary
echo ""
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}  Analysis Complete!${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""
echo -e "RT60 Analysis:       ${GREEN}${RT60_SUCCESS}/${VALID_COUNT}${NC}"
echo -e "Freq Analysis:       ${GREEN}${FREQ_SUCCESS}/${VALID_COUNT}${NC}"
echo -e "Time:                ${YELLOW}${MINUTES}m ${SECONDS}s${NC}"
echo ""

if [ $RT60_SUCCESS -eq $VALID_COUNT ] && [ $FREQ_SUCCESS -eq $VALID_COUNT ]; then
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
