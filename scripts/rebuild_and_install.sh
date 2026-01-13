#!/bin/bash
# Monument - Rebuild and Install Script
# Rebuilds the VST3 plugin and installs it to the system location
# Usage: ./scripts/rebuild_and_install.sh [target]
#        target: Monument (default) | monument_plugin_analyzer | all

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Script directory and project root
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
TEST_CONFIG="${TEST_CONFIG:-Debug}"

# System plugin directories
VST3_INSTALL_DIR="$HOME/Library/Audio/Plug-Ins/VST3"
AU_INSTALL_DIR="$HOME/Library/Audio/Plug-Ins/Components"

# Determine target
TARGET="${1:-Monument}"

echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${BLUE}Monument Rebuild and Install${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

# Change to project root
cd "$PROJECT_ROOT"

find_bundle() {
    local subpath="$1"
    local base="$BUILD_DIR/Monument_artefacts"
    local candidates=(
        "$base/$TEST_CONFIG/$subpath"
        "$base/Debug/$subpath"
        "$base/Release/$subpath"
        "$base/RelWithDebInfo/$subpath"
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

# Step 1: Clean build (optional, only for Monument)
if [[ "$TARGET" == "Monument" ]] || [[ "$TARGET" == "all" ]]; then
    echo -e "${YELLOW}▸ Cleaning previous build...${NC}"
    if [ -d "$BUILD_DIR" ]; then
        cmake --build "$BUILD_DIR" --target clean 2>/dev/null || echo "  (No artifacts to clean)"
    else
        echo -e "  ${RED}✗ Build directory not found. Run ./scripts/build_macos.sh first${NC}"
        exit 1
    fi
    echo -e "${GREEN}  ✓ Clean complete${NC}"
    echo ""
fi

# Step 2: Build the target(s)
if [[ "$TARGET" == "all" ]]; then
    echo -e "${YELLOW}▸ Building Monument (all formats)...${NC}"
    cmake --build "$BUILD_DIR" --target Monument_All
    echo -e "${GREEN}  ✓ Monument built${NC}"
    echo ""

    echo -e "${YELLOW}▸ Building Plugin Analyzer...${NC}"
    cmake --build "$BUILD_DIR" --target monument_plugin_analyzer
    echo -e "${GREEN}  ✓ Plugin Analyzer built${NC}"
    echo ""
else
    echo -e "${YELLOW}▸ Building $TARGET...${NC}"
    if [[ "$TARGET" == "Monument" ]]; then
        cmake --build "$BUILD_DIR" --target Monument_All
    else
        cmake --build "$BUILD_DIR" --target "$TARGET"
    fi
    echo -e "${GREEN}  ✓ $TARGET built successfully${NC}"
    echo ""
fi

# Step 3: Install VST3 (only for Monument or all)
if [[ "$TARGET" == "Monument" ]] || [[ "$TARGET" == "all" ]]; then
    echo -e "${YELLOW}▸ Installing VST3 plugin...${NC}"

    if VST3_PATH=$(find_bundle "VST3/Monument.vst3"); then
        true
    else
        echo -e "${RED}  ✗ VST3 bundle not found in $BUILD_DIR/Monument_artefacts${NC}"
        exit 1
    fi

    # Create plugin directory if needed
    mkdir -p "$VST3_INSTALL_DIR"

    # Remove old version to avoid caching issues
    if [ -d "$VST3_INSTALL_DIR/Monument.vst3" ]; then
        rm -rf "$VST3_INSTALL_DIR/Monument.vst3"
    fi

    # Install (copy) the VST3 bundle
    echo -e "  Source: $VST3_PATH"
    echo -e "  Destination: $VST3_INSTALL_DIR/"
    cp -R "$VST3_PATH" "$VST3_INSTALL_DIR/"

    echo -e "${GREEN}  ✓ VST3 installed${NC}"
    echo ""

    # Install AU component
    echo -e "${YELLOW}▸ Installing AU plugin...${NC}"

    if AU_PATH=$(find_bundle "AU/Monument.component"); then
        true
    else
        echo -e "${YELLOW}  ⚠ AU component not found (may not be built)${NC}"
        AU_PATH=""
    fi

    if [ -n "$AU_PATH" ]; then
        # Create plugin directory if needed
        mkdir -p "$AU_INSTALL_DIR"

        # Remove old version to avoid caching issues
        if [ -d "$AU_INSTALL_DIR/Monument.component" ]; then
            rm -rf "$AU_INSTALL_DIR/Monument.component"
        fi

        # Install (copy) the AU component
        echo -e "  Source: $AU_PATH"
        echo -e "  Destination: $AU_INSTALL_DIR/"
        cp -R "$AU_PATH" "$AU_INSTALL_DIR/"

        echo -e "${GREEN}  ✓ AU installed${NC}"
    fi
    echo ""

    # Step 4: Refresh plugin cache
    echo -e "${YELLOW}▸ Refreshing plugin cache...${NC}"
    if killall AudioComponentRegistrar 2>/dev/null; then
        echo -e "${GREEN}  ✓ Plugin cache refreshed${NC}"
    else
        echo -e "  (Cache process not running - will auto-refresh on next DAW launch)"
    fi
    echo ""
fi

# Step 5: Show installed analyzer path (if built)
if [[ "$TARGET" == "monument_plugin_analyzer" ]] || [[ "$TARGET" == "all" ]]; then
    ANALYZER_CANDIDATES=(
        "$BUILD_DIR/monument_plugin_analyzer_artefacts/$TEST_CONFIG/monument_plugin_analyzer"
        "$BUILD_DIR/monument_plugin_analyzer_artefacts/Debug/monument_plugin_analyzer"
        "$BUILD_DIR/monument_plugin_analyzer_artefacts/Release/monument_plugin_analyzer"
        "$BUILD_DIR/monument_plugin_analyzer_artefacts/RelWithDebInfo/monument_plugin_analyzer"
        "$BUILD_DIR/monument_plugin_analyzer_artefacts/monument_plugin_analyzer"
    )
    ANALYZER_PATH=""
    for candidate in "${ANALYZER_CANDIDATES[@]}"; do
        if [ -f "$candidate" ]; then
            ANALYZER_PATH="$candidate"
            break
        fi
    done

    if [ -n "$ANALYZER_PATH" ]; then
        echo -e "${GREEN}▸ Plugin Analyzer location:${NC}"
        echo -e "  $ANALYZER_PATH"
        echo ""
    fi
fi

# Done
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo -e "${GREEN}✓ Build and install complete!${NC}"
echo -e "${BLUE}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${NC}"
echo ""

# Show next steps
if [[ "$TARGET" == "Monument" ]] || [[ "$TARGET" == "all" ]]; then
    echo -e "${YELLOW}Next steps:${NC}"
    echo -e "  • Test in DAW (restart if needed)"
    echo -e "  • Run analyzer: ./build/monument_plugin_analyzer_artefacts/Debug/monument_plugin_analyzer --plugin Monument.vst3"
    echo -e "  • Run validator: pluginval --validate Monument.vst3"
fi
