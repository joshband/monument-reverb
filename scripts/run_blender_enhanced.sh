#!/bin/bash
# Generate enhanced photorealistic knobs with environmental effects for all materials

set -e

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Default output directory
OUTPUT_DIR="${PROJECT_ROOT}/assets/ui/knobs_enhanced"

# Blender executable
BLENDER="blender"
if [[ "$(uname)" == "Darwin" ]]; then
    # macOS: Check common install locations
    if [[ -f "/Applications/Blender.app/Contents/MacOS/Blender" ]]; then
        BLENDER="/Applications/Blender.app/Contents/MacOS/Blender"
    fi
fi

# Check if Blender is available
if ! command -v "$BLENDER" &> /dev/null; then
    echo -e "${YELLOW}⚠️  Blender not found!${NC}"
    echo "Please install Blender from: https://www.blender.org/download/"
    echo "Or install via Homebrew: brew install --cask blender"
    exit 1
fi

echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}🎨 Monument Reverb - Enhanced Knob Generation${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""
echo "Blender: $BLENDER"
echo "Output:  $OUTPUT_DIR"
echo ""

# Material variants to generate
MATERIALS=("granite" "marble" "basalt" "brushed_metal" "oxidized_copper")
INDICATORS=("brushed_aluminum" "gold" "copper")

# Parse command line arguments
SINGLE_MATERIAL=""
RESOLUTION=512
SAMPLES=256

while [[ $# -gt 0 ]]; do
    case $1 in
        --material)
            SINGLE_MATERIAL="$2"
            shift 2
            ;;
        --out)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        --size)
            RESOLUTION="$2"
            shift 2
            ;;
        --samples)
            SAMPLES="$2"
            shift 2
            ;;
        --quick)
            SAMPLES=64
            shift
            ;;
        --high-quality)
            SAMPLES=512
            shift
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--material NAME] [--out DIR] [--size 512] [--samples 256]"
            echo "       $0 --quick              # Fast preview (64 samples)"
            echo "       $0 --high-quality       # Production quality (512 samples)"
            exit 1
            ;;
    esac
done

# Generate function
generate_knob() {
    local material="$1"
    local indicator="$2"
    local mat_output="${OUTPUT_DIR}/${material}"

    echo -e "\n${GREEN}▶ Generating ${material} knob with ${indicator} indicator...${NC}"

    "$BLENDER" --background --python "${SCRIPT_DIR}/generate_knob_blender_enhanced.py" -- \
        --material "$material" \
        --indicator "$indicator" \
        --out "$mat_output" \
        --size "$RESOLUTION" \
        --samples "$SAMPLES" \
        2>&1 | grep -E "(Rendered|complete|Error)" || true
}

# Generate single material or all materials
if [[ -n "$SINGLE_MATERIAL" ]]; then
    echo -e "${BLUE}Generating single material: ${SINGLE_MATERIAL}${NC}"
    generate_knob "$SINGLE_MATERIAL" "brushed_aluminum"
else
    echo -e "${BLUE}Generating all material variants...${NC}"

    # Generate base materials
    for material in "${MATERIALS[@]}"; do
        # Use different indicator for each material to differentiate
        case $material in
            granite|basalt)
                indicator="brushed_aluminum"
                ;;
            marble)
                indicator="gold"
                ;;
            brushed_metal)
                indicator="copper"
                ;;
            oxidized_copper)
                indicator="gold"
                ;;
            *)
                indicator="brushed_aluminum"
                ;;
        esac

        generate_knob "$material" "$indicator"
    done
fi

echo ""
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo -e "${GREEN}✨ Generation complete!${NC}"
echo -e "${BLUE}═══════════════════════════════════════════════════════════════${NC}"
echo ""
echo "📦 Generated knobs:"
ls -lh "$OUTPUT_DIR"/*/*.png 2>/dev/null | awk '{print "   "$9" ("$5")"}' || echo "   (No files found)"
echo ""
echo -e "${YELLOW}Next steps:${NC}"
echo "  1. Preview composites:"
echo "     python3 scripts/preview_knob_composite_enhanced.py --material granite"
echo ""
echo "  2. Add to CMakeLists.txt:"
echo "     assets/ui/knobs_enhanced/granite/layer_*.png"
echo ""
echo "  3. Build and test:"
echo "     cmake --build build --target Monument_AU"
echo ""