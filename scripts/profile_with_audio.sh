#!/bin/bash
# Profile Monument with active audio processing
# This script launches the standalone app and waits for user to start audio before profiling

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
APP_PATH="$PROJECT_ROOT/build/Monument_artefacts/RelWithDebInfo/Standalone/Monument.app"
TRACE_OUTPUT="$PROJECT_ROOT/monument_profile.trace"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${GREEN}Monument CPU Profiler (Interactive Mode)${NC}"
echo "==========================================="
echo ""

# Check if app exists
if [ ! -d "$APP_PATH" ]; then
    echo -e "${RED}Error: Monument.app not found at $APP_PATH${NC}"
    echo "Please build with RelWithDebInfo first:"
    echo "  cmake -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo"
    echo "  cmake --build build --config RelWithDebInfo"
    exit 1
fi

# Remove old trace if exists
if [ -f "$TRACE_OUTPUT" ]; then
    echo "Removing old trace file..."
    rm -rf "$TRACE_OUTPUT"
fi

echo -e "${BLUE}Step 1: Launching Monument standalone app...${NC}"
echo ""

# Launch the app in the background
open "$APP_PATH"

# Wait for app to launch
sleep 3

# Get the Monument process ID
MONUMENT_PID=$(pgrep -f "Monument.app" | head -1)

if [ -z "$MONUMENT_PID" ]; then
    echo -e "${RED}Error: Could not find Monument process${NC}"
    echo "Please make sure the standalone app launched successfully"
    exit 1
fi

echo -e "${GREEN}✓ Monument is running (PID: $MONUMENT_PID)${NC}"
echo ""
echo -e "${YELLOW}Step 2: Configure audio in Monument:${NC}"
echo "  1. Open Settings/Preferences in the app"
echo "  2. Enable audio input/output"
echo "  3. Start playing audio (file, live input, or signal generator)"
echo "  4. Adjust knobs (especially Tube Count, Metallic Resonance)"
echo "  5. Ensure the plugin is actively processing audio"
echo ""
echo -e "${BLUE}Step 3: Ready to profile?${NC}"
read -p "Press ENTER when audio is actively processing through Monument... " _

echo ""
echo -e "${GREEN}Starting CPU profiling for 30 seconds...${NC}"
echo "Keep the audio processing during this time!"
echo ""

# Profile the running process
xctrace record \
    --template "Time Profiler" \
    --attach "$MONUMENT_PID" \
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
    --output "${PROJECT_ROOT}/monument_profile_export.xml"

echo ""
echo -e "${GREEN}Analyzing results...${NC}"
echo ""
python3 "$SCRIPT_DIR/analyze_profile.py" "${PROJECT_ROOT}/monument_profile_export.xml"

echo ""
echo -e "${BLUE}To view the full trace in Instruments:${NC}"
echo "  open $TRACE_OUTPUT"
echo ""
