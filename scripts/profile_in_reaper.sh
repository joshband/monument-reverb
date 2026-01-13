#!/bin/bash
# Profile Monument plugin in REAPER for accurate CPU measurements

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
TRACE_OUTPUT="$PROJECT_ROOT/monument_reaper_profile.trace"
if [ -z "${BUILD_DIR:-}" ]; then
    if [ -d "$PROJECT_ROOT/build" ]; then
        BUILD_DIR="$PROJECT_ROOT/build"
    elif [ -d "$PROJECT_ROOT/build-ninja" ]; then
        BUILD_DIR="$PROJECT_ROOT/build-ninja"
    else
        BUILD_DIR="$PROJECT_ROOT/build"
    fi
elif [[ "$BUILD_DIR" != /* ]]; then
    BUILD_DIR="$PROJECT_ROOT/$BUILD_DIR"
fi
PROFILE_CONFIG="${PROFILE_CONFIG:-RelWithDebInfo}"

find_bundle() {
    local subpath="$1"
    local base="$BUILD_DIR/Monument_artefacts"
    local candidates=(
        "$base/$PROFILE_CONFIG/$subpath"
        "$base/RelWithDebInfo/$subpath"
        "$base/Release/$subpath"
        "$base/Debug/$subpath"
        "$base/$subpath"
    )

    for candidate in "${candidates[@]}"; do
        if [ -d "$candidate" ]; then
            echo "$candidate"
            return 0
        fi
    done

    return 1
}

AU_PATH="$(find_bundle "AU/Monument.component" || true)"
VST3_PATH="$(find_bundle "VST3/Monument.vst3" || true)"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${GREEN}Monument REAPER CPU Profiler${NC}"
echo "=============================="
echo ""

# Check if plugin exists
if [ ! -d "$AU_PATH" ] && [ ! -d "$VST3_PATH" ]; then
    echo -e "${RED}Error: Monument plugin not found${NC}"
    echo "Please build with RelWithDebInfo first:"
    echo "  cmake -B \"$BUILD_DIR\" -DCMAKE_BUILD_TYPE=RelWithDebInfo"
    echo "  cmake --build \"$BUILD_DIR\" --config RelWithDebInfo"
    exit 1
fi

# Determine which format to use
PLUGIN_FORMAT="AU"
PLUGIN_PATH="$AU_PATH"
if [ ! -d "$AU_PATH" ] && [ -d "$VST3_PATH" ]; then
    PLUGIN_FORMAT="VST3"
    PLUGIN_PATH="$VST3_PATH"
fi

echo -e "${GREEN}✓ Found Monument $PLUGIN_FORMAT plugin${NC}"
echo "  Path: $PLUGIN_PATH"
echo ""

# Remove old trace if exists
if [ -f "$TRACE_OUTPUT" ]; then
    echo "Removing old trace file..."
    rm -rf "$TRACE_OUTPUT"
fi

echo -e "${BLUE}Setup Instructions for REAPER:${NC}"
echo "================================"
echo ""
echo -e "${YELLOW}Step 1: Open REAPER${NC}"
echo "  • Launch REAPER (if not already open)"
echo ""
echo -e "${YELLOW}Step 2: Create a test project${NC}"
echo "  • Create a new track (Cmd+T)"
echo "  • Add a signal generator:"
echo "    → Insert > JS > Synthesis/tone_generator"
echo "    → Set frequency: 220 Hz (A3)"
echo "    → Set volume: -12 dB"
echo ""
echo -e "${YELLOW}Step 3: Load Monument plugin${NC}"
echo "  • Add Monument to the track FX chain"
echo "  • Set parameters to stress TubeRayTracer:"
echo "    → Time: 2-3 seconds (longer reverb)"
echo "    → Density: 70-80% (more complexity)"
echo "    → Tube Count: 8-12 tubes (maximum complexity)"
echo "    → Metallic Resonance: 50-70% (active filters)"
echo "    → Mix: 100% (hear only wet signal)"
echo ""
echo -e "${YELLOW}Step 4: Start playback${NC}"
echo "  • Click Play (Spacebar)"
echo "  • Verify audio is processing (meters moving, reverb tail audible)"
echo "  • Let it run for a few seconds to ensure everything is stable"
echo ""
echo -e "${BLUE}Ready to profile?${NC}"
read -p "Press ENTER when REAPER is playing audio through Monument... " _

echo ""

# Find REAPER process
REAPER_PID=$(pgrep -i "reaper" | head -1)

if [ -z "$REAPER_PID" ]; then
    echo -e "${RED}Error: Could not find REAPER process${NC}"
    echo "Please make sure REAPER is running"
    exit 1
fi

echo -e "${GREEN}✓ Found REAPER (PID: $REAPER_PID)${NC}"
echo ""
echo -e "${GREEN}Starting CPU profiling for 30 seconds...${NC}"
echo "Keep REAPER playing audio through Monument during this time!"
echo ""

# Profile REAPER process (which includes Monument)
xctrace record \
    --template "Time Profiler" \
    --attach "$REAPER_PID" \
    --output "$TRACE_OUTPUT" \
    --time-limit 30s

echo ""
echo -e "${GREEN}Profile recording complete!${NC}"
echo "Trace saved to: $TRACE_OUTPUT"
echo ""

# Export and analyze
echo "Exporting trace data..."
xctrace export --input "$TRACE_OUTPUT" \
    --xpath '/trace-toc/run[@number="1"]/data/table[@schema="time-profile"]' \
    --output "${PROJECT_ROOT}/monument_reaper_profile_export.xml"

echo ""
echo -e "${GREEN}Analyzing results...${NC}"
echo ""
python3 "$SCRIPT_DIR/analyze_profile.py" "${PROJECT_ROOT}/monument_reaper_profile_export.xml"

echo ""
echo -e "${BLUE}Additional Analysis:${NC}"
echo "To view the full trace in Instruments:"
echo "  open $TRACE_OUTPUT"
echo ""
echo "To export specific functions:"
echo "  xctrace export --input $TRACE_OUTPUT --xpath '...' --output custom_export.xml"
echo ""
