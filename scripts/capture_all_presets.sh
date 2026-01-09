#!/bin/bash
# Monument Reverb - Batch Preset Capture Script
# Captures impulse responses for all 37 factory presets with extended duration
# Supports parallel execution for faster processing

set -e  # Exit on error

# Configuration
PLUGIN_PATH="${HOME}/Library/Audio/Plug-Ins/VST3/Monument.vst3"
ANALYZER_PATH="./build/monument_plugin_analyzer_artefacts/Debug/monument_plugin_analyzer"
OUTPUT_BASE="./test-results/preset-baseline"
DURATION=30  # 30 seconds ensures complete reverb tail capture with margin (1.5x longest RT60)
NUM_PRESETS=37

# Parallel execution (default: use all CPU cores)
# Set PARALLEL_JOBS=1 to disable parallel processing
PARALLEL_JOBS=${PARALLEL_JOBS:-$(sysctl -n hw.ncpu)}
MAX_PARALLEL=8  # Cap at 8 to avoid overwhelming the system
PARALLEL_JOBS=$((PARALLEL_JOBS > MAX_PARALLEL ? MAX_PARALLEL : PARALLEL_JOBS))

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Check if analyzer exists
if [ ! -f "$ANALYZER_PATH" ]; then
    echo -e "${RED}Error: Plugin analyzer not found at $ANALYZER_PATH${NC}"
    echo "Run: ./scripts/rebuild_and_install.sh monument_plugin_analyzer"
    exit 1
fi

# Check if plugin exists
if [ ! -d "$PLUGIN_PATH" ]; then
    echo -e "${RED}Error: Monument plugin not found at $PLUGIN_PATH${NC}"
    echo "Run: ./scripts/rebuild_and_install.sh Monument"
    exit 1
fi

# Create output directory
mkdir -p "$OUTPUT_BASE"

# Print header
echo ""
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}  Monument Reverb - Batch Preset Capture${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""
echo -e "Plugin:    ${GREEN}${PLUGIN_PATH}${NC}"
echo -e "Presets:   ${GREEN}0-36 (${NUM_PRESETS} total)${NC}"
echo -e "Duration:  ${GREEN}${DURATION} seconds${NC}"
echo -e "Output:    ${GREEN}${OUTPUT_BASE}${NC}"
echo -e "Parallel:  ${GREEN}${PARALLEL_JOBS} workers${NC}"
echo ""

ESTIMATED_SERIAL=$(($DURATION * $NUM_PRESETS / 60))
ESTIMATED_PARALLEL=$(($DURATION * $NUM_PRESETS / $PARALLEL_JOBS / 60))
echo -e "${YELLOW}Estimated time: ~${ESTIMATED_PARALLEL} minutes (vs ${ESTIMATED_SERIAL}m sequential)${NC}"
echo ""
read -p "Press Enter to continue or Ctrl+C to cancel..."
echo ""

# Function to capture a single preset (will be called in parallel)
capture_preset() {
    local i=$1
    local PRESET_DIR="${OUTPUT_BASE}/preset_$(printf "%02d" $i)"

    echo "[Preset ${i}] Starting capture..."

    # Create preset directory
    mkdir -p "$PRESET_DIR"

    # Run analyzer with integrated analysis
    if "$ANALYZER_PATH" \
        --plugin "$PLUGIN_PATH" \
        --preset $i \
        --duration $DURATION \
        --output "$PRESET_DIR" \
        --analyze > "${PRESET_DIR}/capture.log" 2>&1; then

        # Verify output files exist (audio + analysis)
        if [ -f "${PRESET_DIR}/wet.wav" ] && [ -f "${PRESET_DIR}/dry.wav" ] && \
           [ -f "${PRESET_DIR}/rt60_metrics.json" ] && [ -f "${PRESET_DIR}/frequency_response.json" ]; then
            echo "[Preset ${i}] ✓ Captured and analyzed successfully"

            # Save metadata
            cat > "${PRESET_DIR}/metadata.json" <<EOF
{
  "preset_index": $i,
  "capture_date": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
  "duration_seconds": $DURATION,
  "sample_rate": 48000,
  "bit_depth": 24,
  "plugin_path": "$PLUGIN_PATH"
}
EOF
            return 0
        else
            echo "[Preset ${i}] ✗ Failed - output files missing"
            return 1
        fi
    else
        echo "[Preset ${i}] ✗ Failed - analyzer error"
        return 1
    fi
}

# Export function and variables for parallel execution
export -f capture_preset
export ANALYZER_PATH PLUGIN_PATH OUTPUT_BASE DURATION

# Track statistics
START_TIME=$(date +%s)

# Generate list of preset indices and process in parallel
echo "Starting parallel capture with ${PARALLEL_JOBS} workers..."
echo ""

# Use xargs for parallel execution (-P controls parallel jobs)
if seq 0 36 | xargs -P "$PARALLEL_JOBS" -I {} bash -c 'capture_preset "$@"' _ {}; then
    echo ""
    echo "All capture jobs completed"
else
    echo ""
    echo "Some capture jobs failed (see individual logs)"
fi

# Calculate elapsed time
END_TIME=$(date +%s)
ELAPSED=$((END_TIME - START_TIME))
MINUTES=$((ELAPSED / 60))
SECONDS=$((ELAPSED % 60))

# Count successes and failures
SUCCESS_COUNT=0
FAIL_COUNT=0
for i in $(seq 0 36); do
    PRESET_DIR="${OUTPUT_BASE}/preset_$(printf "%02d" $i)"
    if [ -f "${PRESET_DIR}/wet.wav" ] && [ -f "${PRESET_DIR}/dry.wav" ] && \
       [ -f "${PRESET_DIR}/rt60_metrics.json" ] && [ -f "${PRESET_DIR}/frequency_response.json" ]; then
        SUCCESS_COUNT=$((SUCCESS_COUNT + 1))
    else
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
done

# Print summary
echo ""
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}  Capture Complete!${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""
echo -e "Success: ${GREEN}${SUCCESS_COUNT}/${NUM_PRESETS}${NC}"
echo -e "Failed:  ${RED}${FAIL_COUNT}/${NUM_PRESETS}${NC}"
echo -e "Time:    ${YELLOW}${MINUTES}m ${SECONDS}s${NC}"
echo -e "Speedup: ${GREEN}$(($ESTIMATED_SERIAL / ($MINUTES + 1)))x faster than sequential${NC}"
echo ""

if [ $SUCCESS_COUNT -eq $NUM_PRESETS ]; then
    echo -e "${GREEN}✓ All presets captured and analyzed successfully!${NC}"
    echo ""
    echo -e "Generated for each preset:"
    echo -e "  • wet.wav, dry.wav (audio captures)"
    echo -e "  • rt60_metrics.json (reverb time analysis)"
    echo -e "  • frequency_response.json (spectral analysis)"
    echo -e "  • metadata.json (capture info)"
    echo ""
    echo -e "Next steps:"
    echo -e "  1. Compare with baseline: ${BLUE}python3 tools/compare_baseline.py test-results/preset-baseline${NC}"
    echo -e "  2. Generate report: ${BLUE}python3 tools/generate_preset_report.py${NC}"
    echo ""
    exit 0
else
    echo -e "${YELLOW}⚠ Some presets failed to capture or analyze${NC}"
    echo -e "Check individual logs in ${OUTPUT_BASE}/preset_*/capture.log"
    echo ""
    exit 1
fi
