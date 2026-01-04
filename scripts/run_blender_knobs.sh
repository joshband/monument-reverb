#!/bin/bash
# Wrapper script to run Blender knob generation

set -e

# Find Blender executable
BLENDER=""
if command -v blender &> /dev/null; then
    BLENDER="blender"
elif [ -f "/Applications/Blender.app/Contents/MacOS/Blender" ]; then
    BLENDER="/Applications/Blender.app/Contents/MacOS/Blender"
else
    echo "❌ Blender not found. Please install from https://www.blender.org/download/"
    echo "   Or set BLENDER environment variable to the Blender executable"
    exit 1
fi

echo "🎨 Found Blender: $BLENDER"
echo ""

# Get script directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
REPO_ROOT="$( cd "$SCRIPT_DIR/.." && pwd )"

# Default output directory
OUTPUT_DIR="$REPO_ROOT/assets/ui/knobs_test"
SIZE=512

# Parse arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --out)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        --size)
            SIZE="$2"
            shift 2
            ;;
        *)
            echo "Unknown option: $1"
            echo "Usage: $0 [--out OUTPUT_DIR] [--size SIZE]"
            exit 1
            ;;
    esac
done

echo "📁 Output directory: $OUTPUT_DIR"
echo "📐 Resolution: ${SIZE}x${SIZE}"
echo ""
echo "⏳ Rendering knob layers (this may take 2-3 minutes)..."
echo ""

# Run Blender in background
"$BLENDER" --background --python "$SCRIPT_DIR/generate_knob_blender.py" -- \
    --out "$OUTPUT_DIR" \
    --size "$SIZE"

echo ""
echo "✅ Done! Check your knob layers:"
echo "   ls -lh $OUTPUT_DIR"
