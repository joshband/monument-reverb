#!/bin/bash
# Hero Knob Test Renderer
# Tests advanced shader approach with high-quality materials

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
OUTPUT_DIR="$PROJECT_ROOT/assets/ui/knobs_hero"

echo "=== Hero Knob Test Render ==="
echo "Script: $SCRIPT_DIR/generate_knob_hero.py"
echo "Output: $OUTPUT_DIR"
echo ""

# Check Blender
if ! command -v blender &> /dev/null; then
    echo "❌ Error: Blender not found in PATH"
    echo "Install: brew install blender"
    exit 1
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Render
echo "Starting render (512 samples, ~5-7 minutes)..."
blender --background --python "$SCRIPT_DIR/generate_knob_hero.py" -- "$OUTPUT_DIR"

# Check output
if [ -f "$OUTPUT_DIR/hero_knob_test.png" ]; then
    echo "✅ Render complete!"
    echo "Output: $OUTPUT_DIR/hero_knob_test.png"
    echo ""
    echo "Open with: open $OUTPUT_DIR/hero_knob_test.png"
else
    echo "❌ Render failed - no output file"
    exit 1
fi
