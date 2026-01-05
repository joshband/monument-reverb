#!/bin/bash
# Manual UI screenshot capture for Monument
# Captures screenshots using keyboard shortcuts

echo "📸 Monument Manual UI Capture"
echo "=============================="
echo ""
echo "This script will guide you through capturing baseline screenshots manually."
echo ""
echo "1. Make sure Monument standalone is open"
echo "2. Position the window where you want it"
echo "3. Follow the prompts to capture each state"
echo ""
read -p "Press Enter when Monument is ready..."

OUTDIR="test-results/ui-baseline"
mkdir -p "$OUTDIR"

echo ""
echo "📷 Capture 1: Default view"
echo "   - Make sure Monument shows the default state (no panels expanded)"
read -p "   - Press Cmd+Shift+4, then Space, then click Monument window"
read -p "   - Save as: $OUTDIR/01_default.png"
echo "   ✓ Captured"

echo ""
echo "📷 Capture 2: BASE PARAMS expanded"
echo "   - Click the 'BASE PARAMS' button in Monument"
read -p "   - Press Cmd+Shift+4, then Space, then click Monument window"
read -p "   - Save as: $OUTDIR/02_base_params.png"
echo "   ✓ Captured"

echo ""
echo "📷 Capture 3: MODULATION panel"
echo "   - Click 'BASE PARAMS' again to close it"
echo "   - Click the 'MODULATION' button"
read -p "   - Press Cmd+Shift+4, then Space, then click Monument window"
read -p "   - Save as: $OUTDIR/03_modulation.png"
echo "   ✓ Captured"

echo ""
echo "📷 Capture 4: TIMELINE panel"
echo "   - Click 'MODULATION' to close it"
echo "   - Click the 'TIMELINE' button"
read -p "   - Press Cmd+Shift+4, then Space, then click Monument window"
read -p "   - Save as: $OUTDIR/04_timeline.png"
echo "   ✓ Captured"

echo ""
echo "✅ All screenshots captured!"
echo "   Location: $OUTDIR/"
echo ""
echo "Next: Run visual regression tests with:"
echo "   python3 tools/test_ui_visual.py"
