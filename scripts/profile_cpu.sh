#!/bin/bash
# Profile Monument standalone app CPU usage

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
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

find_standalone_app() {
    local base="$BUILD_DIR/Monument_artefacts"
    local candidates=(
        "$base/$PROFILE_CONFIG/Standalone/Monument.app"
        "$base/RelWithDebInfo/Standalone/Monument.app"
        "$base/Release/Standalone/Monument.app"
        "$base/Debug/Standalone/Monument.app"
        "$base/Standalone/Monument.app"
    )

    for candidate in "${candidates[@]}"; do
        if [ -d "$candidate" ]; then
            echo "$candidate"
            return 0
        fi
    done

    return 1
}

APP_PATH=""
if APP_PATH=$(find_standalone_app); then
    true
else
    APP_PATH="$BUILD_DIR/Monument_artefacts/$PROFILE_CONFIG/Standalone/Monument.app"
fi
TRACE_OUTPUT="$PROJECT_ROOT/monument_profile.trace"

# Colors
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m'

echo -e "${GREEN}Monument CPU Profiler${NC}"
echo "================================"
echo ""

# Check if app exists
if [ ! -d "$APP_PATH" ]; then
    echo -e "${RED}Error: Monument.app not found at $APP_PATH${NC}"
    echo "Please build with RelWithDebInfo first:"
    echo "  cmake -B \"$BUILD_DIR\" -DCMAKE_BUILD_TYPE=RelWithDebInfo"
    echo "  cmake --build \"$BUILD_DIR\" --config RelWithDebInfo"
    exit 1
fi

# Remove old trace if exists
if [ -f "$TRACE_OUTPUT" ]; then
    echo "Removing old trace file..."
    rm -rf "$TRACE_OUTPUT"
fi

echo -e "${YELLOW}Instructions:${NC}"
echo "1. The standalone app will launch shortly"
echo "2. Start playing audio through Monument (process some input)"
echo "3. Let it run for ~30 seconds while profiling"
echo "4. Press Ctrl+C in this terminal to stop profiling"
echo ""
echo "Recording CPU profile for 30 seconds..."
echo ""

# Launch app with Time Profiler
# Note: The app will launch and profile for 30 seconds automatically
xctrace record \
    --template "Time Profiler" \
    --launch "$APP_PATH" \
    --output "$TRACE_OUTPUT" \
    --time-limit 30s

echo ""
echo -e "${GREEN}Profile recording complete!${NC}"
echo "Trace saved to: $TRACE_OUTPUT"
echo ""
echo -e "${YELLOW}To view the trace:${NC}"
echo "  open $TRACE_OUTPUT"
echo ""
echo -e "${YELLOW}To export call tree data:${NC}"
echo "  xctrace export --input $TRACE_OUTPUT --xpath '/trace-toc/run[@number=\"1\"]/data/table[@schema=\"time-profile\"]'"
echo ""
