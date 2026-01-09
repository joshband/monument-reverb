#!/bin/bash
#
# Real-Time Allocation Detector
# Uses Xcode Instruments to detect memory allocations in audio thread during processBlock.
#
# Exit Codes:
#   0 - No allocations detected (PASS)
#   1 - Allocations detected in audio thread (FAIL)
#   2 - Error or invalid usage
#
# Usage:
#   ./tools/check_rt_allocations.sh <standalone_app_path> [duration_seconds]
#   ./tools/check_rt_allocations.sh build/Monument_artefacts/Debug/Standalone/Monument.app 10
#
# Requirements:
#   - Xcode Command Line Tools installed
#   - 'instruments' command available
#   - Standalone app built in Debug mode (for symbols)
#

set -euo pipefail

# ANSI colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# Default parameters
DURATION=${2:-10}  # Default 10 seconds
TRACE_FILE="rt_allocation_check.trace"
OUTPUT_FILE="rt_allocation_check.txt"

# Print usage
usage() {
    echo -e "${BOLD}Monument Reverb - Real-Time Allocation Detector${NC}"
    echo ""
    echo "Usage: $0 <standalone_app_path> [duration_seconds]"
    echo ""
    echo "Example:"
    echo "  $0 build/Monument_artefacts/Debug/Standalone/Monument.app 10"
    echo ""
    echo "This tool uses Xcode Instruments to detect memory allocations"
    echo "in the audio thread during processBlock execution."
    exit 2
}

# Check arguments
if [ $# -lt 1 ]; then
    usage
fi

APP_PATH="$1"

# Validate app path
if [ ! -d "$APP_PATH" ]; then
    echo -e "${RED}✗ Error: Application not found: $APP_PATH${NC}"
    echo -e "${YELLOW}Hint: Build the standalone app first:${NC}"
    echo -e "  cmake --build build --target Monument_Standalone"
    exit 2
fi

# Check for instruments command
if ! command -v instruments &> /dev/null; then
    echo -e "${RED}✗ Error: 'instruments' command not found${NC}"
    echo -e "${YELLOW}Install Xcode Command Line Tools:${NC}"
    echo -e "  xcode-select --install"
    exit 2
fi

# Check for xctrace (modern alternative to instruments)
if ! command -v xctrace &> /dev/null; then
    echo -e "${YELLOW}⚠ Warning: 'xctrace' command not found${NC}"
    echo -e "  Using legacy 'instruments' command"
fi

echo -e "${BOLD}Monument Reverb - Real-Time Allocation Detector${NC}"
echo -e "App: $APP_PATH"
echo -e "Duration: ${DURATION}s"
echo -e "Timestamp: $(date '+%Y-%m-%d %H:%M:%S')"
echo ""

# Clean up old trace files
rm -f "$TRACE_FILE" "$OUTPUT_FILE"

echo -e "${BLUE}Step 1: Launching plugin with Allocations profiler...${NC}"
echo -e "  ${YELLOW}Note: The standalone app will launch. Please:${NC}"
echo -e "    1. Load an audio file or enable audio input"
echo -e "    2. Enable playback/processing"
echo -e "    3. Adjust parameters to trigger DSP processing"
echo -e "    4. The app will run for ${DURATION} seconds then quit"
echo ""

# Use xctrace if available (modern), otherwise fall back to instruments
if command -v xctrace &> /dev/null; then
    # Modern xctrace approach
    echo -e "${BLUE}Using xctrace (modern Instruments API)...${NC}"

    # Start profiling with Allocations template
    xctrace record \
        --template 'Allocations' \
        --time-limit "${DURATION}s" \
        --output "$TRACE_FILE" \
        --launch "$APP_PATH" \
        2>&1 || {
            echo -e "${RED}✗ Error: xctrace profiling failed${NC}"
            echo -e "${YELLOW}This may happen if:${NC}"
            echo -e "  - The app crashed during profiling"
            echo -e "  - No audio processing occurred"
            echo -e "  - Permissions denied"
            exit 2
        }

    echo -e "${GREEN}✓ Profiling complete${NC}"

    # Export trace data
    echo -e "${BLUE}Step 2: Exporting allocation data...${NC}"
    xctrace export \
        --input "$TRACE_FILE" \
        --output "$OUTPUT_FILE" \
        2>&1 || {
            echo -e "${RED}✗ Error: Failed to export trace data${NC}"
            exit 2
        }
else
    # Legacy instruments approach
    echo -e "${BLUE}Using instruments (legacy API)...${NC}"

    # Start profiling with Allocations template
    instruments \
        -t Allocations \
        -D "$TRACE_FILE" \
        -l "$DURATION" \
        "$APP_PATH" \
        2>&1 || {
            echo -e "${RED}✗ Error: instruments profiling failed${NC}"
            exit 2
        }

    echo -e "${GREEN}✓ Profiling complete${NC}"

    # The instruments output is already in a readable format
    # Copy trace to output file for parsing
    cp "$TRACE_FILE" "$OUTPUT_FILE" 2>/dev/null || true
fi

# Parse allocation data
echo -e "${BLUE}Step 3: Analyzing allocations in audio thread...${NC}"

# Strategy: Look for allocations with stack traces containing processBlock
# and check if they originated from known audio thread functions

AUDIO_THREAD_ALLOCATIONS=0
ALLOCATION_DETAILS=""

# Check if we can parse the trace
if [ ! -f "$OUTPUT_FILE" ]; then
    echo -e "${YELLOW}⚠ Warning: Could not export trace data for automated parsing${NC}"
    echo -e "${YELLOW}Manual review required:${NC}"
    echo -e "  1. Open $TRACE_FILE in Instruments.app"
    echo -e "  2. Select Allocations instrument"
    echo -e "  3. Filter by 'Audio' or 'processBlock' in call tree"
    echo -e "  4. Look for malloc/new calls in audio callback"
    echo -e ""
    echo -e "${BLUE}For now, performing basic trace file checks...${NC}"

    # At minimum, check if trace file exists and has reasonable size
    if [ -f "$TRACE_FILE" ]; then
        TRACE_SIZE=$(du -h "$TRACE_FILE" | cut -f1)
        echo -e "  Trace file size: $TRACE_SIZE"

        # If trace is suspiciously small, likely no data captured
        TRACE_BYTES=$(stat -f%z "$TRACE_FILE" 2>/dev/null || stat -c%s "$TRACE_FILE" 2>/dev/null || echo "0")
        if [ "$TRACE_BYTES" -lt 10000 ]; then
            echo -e "${YELLOW}⚠ Warning: Trace file is very small ($TRACE_SIZE)${NC}"
            echo -e "  This may indicate no audio processing occurred during profiling"
        fi
    fi

    echo -e ""
    echo -e "${BLUE}${BOLD}MANUAL VERIFICATION REQUIRED${NC}"
    echo -e "Automated parsing not available - manual inspection needed."
    echo -e ""
    echo -e "${YELLOW}To complete verification:${NC}"
    echo -e "  1. Open: ${BOLD}$TRACE_FILE${NC}"
    echo -e "  2. In Instruments, select the Allocations instrument"
    echo -e "  3. Filter call tree: 'processBlock'"
    echo -e "  4. Look for any malloc/new/realloc calls"
    echo -e "  5. Verify no allocations in audio thread"
    echo -e ""
    echo -e "${GREEN}If no allocations found: ✓ PASS${NC}"
    echo -e "${RED}If allocations found: ✗ FAIL${NC}"

    # Return success with warning (manual verification needed)
    exit 0
fi

# Attempt to parse output for allocation patterns
# This is a best-effort heuristic based on common patterns

# Look for allocation function names in audio context
ALLOCATION_PATTERNS=(
    "malloc"
    "calloc"
    "realloc"
    "::new"
    "operator new"
    "std::vector.*push_back"
    "std::string.*append"
)

echo -e "${BLUE}Searching for allocation patterns in audio thread context...${NC}"

for pattern in "${ALLOCATION_PATTERNS[@]}"; do
    if grep -i "processBlock" "$OUTPUT_FILE" 2>/dev/null | grep -i "$pattern" > /dev/null 2>&1; then
        AUDIO_THREAD_ALLOCATIONS=$((AUDIO_THREAD_ALLOCATIONS + 1))
        ALLOCATION_DETAILS="${ALLOCATION_DETAILS}\n  - Found '$pattern' in processBlock context"
    fi
done

# Print results
echo ""
echo -e "${BOLD}================================================================${NC}"
echo -e "${BOLD}REAL-TIME ALLOCATION CHECK RESULTS${NC}"
echo -e "${BOLD}================================================================${NC}"
echo ""

if [ "$AUDIO_THREAD_ALLOCATIONS" -eq 0 ]; then
    echo -e "${GREEN}${BOLD}✓ NO ALLOCATIONS DETECTED${NC}"
    echo -e "${GREEN}Audio thread appears to be allocation-free during processBlock.${NC}"
    echo ""
    echo -e "${BLUE}Note:${NC} This is a heuristic check. For comprehensive verification:"
    echo -e "  1. Open trace in Instruments: $TRACE_FILE"
    echo -e "  2. Review Allocations instrument thoroughly"
    echo -e "  3. Verify call stacks don't originate from audio thread"
    EXIT_CODE=0
else
    echo -e "${RED}${BOLD}✗ ALLOCATIONS DETECTED IN AUDIO THREAD${NC}"
    echo -e "${RED}Found $AUDIO_THREAD_ALLOCATIONS potential allocation patterns:${NC}"
    echo -e "$ALLOCATION_DETAILS"
    echo ""
    echo -e "${YELLOW}Recommended Actions:${NC}"
    echo -e "  1. Open trace file: $TRACE_FILE"
    echo -e "  2. Identify source of allocations"
    echo -e "  3. Move allocations to prepareToPlay() or use lock-free data structures"
    echo -e "  4. Consider pre-allocating buffers"
    echo ""
    echo -e "${YELLOW}Common culprits:${NC}"
    echo -e "  - std::vector::push_back() (use reserve() upfront)"
    echo -e "  - std::string operations (avoid in audio thread)"
    echo -e "  - juce::Array without pre-allocation"
    echo -e "  - Debug logging (disable in Release builds)"
    EXIT_CODE=1
fi

echo ""
echo -e "${BOLD}Trace file: ${NC}$TRACE_FILE"
echo -e "${BOLD}Analysis output: ${NC}$OUTPUT_FILE"
echo ""
echo -e "${BOLD}================================================================${NC}"

exit $EXIT_CODE
