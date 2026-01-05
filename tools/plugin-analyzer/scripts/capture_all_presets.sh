#!/bin/bash
#
# Capture impulse responses for all Monument factory presets
# Generates RT60 plots and frequency response plots for each preset
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../../.." && pwd)"
ANALYZER="$PROJECT_ROOT/build/monument_plugin_analyzer_artefacts/Debug/monument_plugin_analyzer"
PLUGIN_PATH="$HOME/Library/Audio/Plug-Ins/VST3/Monument.vst3"
OUTPUT_DIR="$PROJECT_ROOT/test-results/presets"
PYTHON_DIR="$PROJECT_ROOT/tools/plugin-analyzer/python"

# Check if analyzer exists
if [ ! -f "$ANALYZER" ]; then
    echo "Error: Plugin analyzer not found at $ANALYZER"
    echo "Please build it first: cmake --build build --target monument_plugin_analyzer"
    exit 1
fi

# Check if plugin exists
if [ ! -d "$PLUGIN_PATH" ]; then
    echo "Error: Monument plugin not found at $PLUGIN_PATH"
    exit 1
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Monument has 37 factory presets (0-indexed)
# For now, we'll capture the default preset since the analyzer doesn't support preset loading yet
# TODO: Add preset loading functionality to analyzer

echo "============================================================"
echo "Monument Preset Analysis"
echo "============================================================"
echo ""
echo "NOTE: Plugin analyzer doesn't support preset loading yet."
echo "Capturing default preset (Init Patch) with 100% mix."
echo ""
echo "To capture all 37 presets, we need to add preset loading to the analyzer."
echo ""
echo "Preset names from Monument:"
echo "  0. Init Patch"
echo "  1. Stone Hall"
echo "  2. High Vault"
echo "  3. Cold Chamber"
echo "  4. Night Atrium"
echo "  5. Monumental Void"
echo "  6. Stone Circles"
echo "  7. Cathedral of Glass"
echo "  8. Zero-G Garden"
echo "  9. Weathered Nave"
echo "  ... (28 more presets)"
echo ""
echo "============================================================"
echo ""

# For now, capture only the default preset
PRESET_NAME="InitPatch"
PRESET_DIR="$OUTPUT_DIR/$PRESET_NAME"
mkdir -p "$PRESET_DIR"

echo "▸ Capturing impulse response for $PRESET_NAME..."
"$ANALYZER" --plugin "$PLUGIN_PATH" --duration 10 --output "$PRESET_DIR"

echo ""
echo "▸ Analyzing RT60..."
python3 "$PYTHON_DIR/rt60_analysis_robust.py" "$PRESET_DIR/wet.wav" \
    --output "$PRESET_DIR/metrics.json"

echo ""
echo "▸ Analyzing frequency response..."
python3 "$PYTHON_DIR/frequency_response.py" "$PRESET_DIR/wet.wav" --impulse \
    --output "$PRESET_DIR/frequency_metrics.json"

echo ""
echo "============================================================"
echo "✓ Analysis complete for $PRESET_NAME"
echo "============================================================"
echo ""
echo "Results saved to: $PRESET_DIR"
echo "  - wet.wav: Impulse response"
echo "  - wet.png: RT60 decay curve"
echo "  - wet_frequency_response.png: Frequency response"
echo "  - metrics.json: RT60 measurements"
echo "  - frequency_metrics.json: Frequency response data"
echo ""
echo "To capture all 37 presets, run:"
echo "  TODO: Implement preset loading in analyzer"
echo ""
