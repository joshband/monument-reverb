# Monument Reverb - Project Context

## About This Project
**Type:** JUCE C++ Audio Plugin (VST3/AU/AUv3)
**Current Focus:** Enhanced knob UI with photorealistic 3D geometry
**Main Feature:** High-quality reverb effect with professional UI controls

## Current Session Task
**Date:** 2026-01-03
**Task:** ✅ Preset System v2 - Macro Parameters + UI Browser (Complete)
**Handoff Doc:** [NEXT_SESSION_HANDOFF.md](NEXT_SESSION_HANDOFF.md)
**Next Task:** Modulation connection serialization (format v3)

## Quick Start Commands

### UI Development (Blender-based knob generation)
```bash
# Generate enhanced knobs with current 10-layer geometry
./scripts/run_blender_enhanced.sh

# Preview composite layering
python3 scripts/preview_knob_composite_enhanced.py

# View output
open assets/ui/knobs_enhanced/
```

### Plugin Build (macOS)
```bash
# Full build
./scripts/build_macos.sh

# CMake build only
cmake --build build

# Run in standalone mode (after build)
open build/monument-reverb_artefacts/Debug/Standalone/monument-reverb.app
```

### Development Tools
```bash
# Check dependencies
brew install blender python3

# Verify Blender installation
blender --version

# Kill Audio Unit host (if needed for testing)
killall -9 AudioComponentRegistrar
```

## Key Files & Directories

### UI/Knob Generation
- **Main Script:** [scripts/generate_knob_blender_enhanced.py](scripts/generate_knob_blender_enhanced.py) (10-layer geometry)
- **Runner:** [scripts/run_blender_enhanced.sh](scripts/run_blender_enhanced.sh)
- **Preview:** [scripts/preview_knob_composite_enhanced.py](scripts/preview_knob_composite_enhanced.py)
- **Output:** `assets/ui/knobs_enhanced/` (generated PNGs, 1024×1024)

### Documentation
- **UI Summary:** [docs/ui/ENHANCED_UI_SUMMARY.md](docs/ui/ENHANCED_UI_SUMMARY.md)
- **Design References:** [docs/ui/design-references/VINTAGE_CONTROL_PANEL_REFERENCES.md](docs/ui/design-references/VINTAGE_CONTROL_PANEL_REFERENCES.md)
- **Testing Docs:** [docs/testing/](docs/testing/)

### C++ Plugin Code (JUCE)
- **Source:** `src/` (DSP, UI components)
- **Headers:** `include/`
- **Build:** `build/` (CMake output, git-ignored)

## Current Enhancement: LED Ring Layer

### Technical Specs
```python
# Layer 11 (NEW): LED Ring beneath knob cap
layer_name = "Layer_11_LED_Ring"
geometry = {
    'type': 'torus',
    'major_radius': 25.875,  # knob_radius * 1.15
    'minor_radius': 1.0,
    'segments': 64
}

material = {
    'emission_color': (1.0, 0.6, 0.2),  # Warm amber
    'emission_strength': 3.0,
    'transparency_mix': 0.3
}

position = {
    'z_offset': 1.0  # Below cap, above base
}
```

### Implementation Steps
1. Open [scripts/generate_knob_blender_enhanced.py](scripts/generate_knob_blender_enhanced.py)
2. Add `create_led_ring_layer()` function after `create_tick_marks_layer()`
3. Call in main generation loop (around line 450)
4. Test: `./scripts/run_blender_enhanced.sh`
5. Verify: `open assets/ui/knobs_enhanced/`
6. Preview composite: `python3 scripts/preview_knob_composite_enhanced.py`

## Upcoming Enhancements (After LED Ring)
1. ✅ LED ring (current task)
2. ⏳ Concave cap geometry (ergonomic)
3. ⏳ Enhanced tick marks (variable height)
4. ⏳ 3-level depth architecture (premium look)

## Development Rules
- **Test Before Commit:** Verify Blender script runs without errors
- **Visual Verification:** Always check output PNGs for correctness
- **Incremental Changes:** One layer/feature at a time
- **Documentation:** Update [docs/ui/ENHANCED_UI_SUMMARY.md](docs/ui/ENHANCED_UI_SUMMARY.md) after changes

## Git Workflow
```bash
# Current branch
git branch  # Should be 'main'

# Check status
git status

# Stage UI changes
git add assets/ui/knobs_enhanced/
git add scripts/generate_knob_blender_enhanced.py
git add docs/ui/

# Commit (with approval)
git commit -m "feat: add LED ring layer to enhanced knob geometry"
git push origin main
```

## Token Optimization
**See:** [TOKEN_OPTIMIZATION_STRATEGIES.md](TOKEN_OPTIMIZATION_STRATEGIES.md)
- Use `/clear` after completing this task
- Archive detailed history to `~/Documents/session-history/`
- Keep this CLAUDE.md lean (under 200 lines)

## External References
- **JUCE Framework:** https://juce.com/
- **Blender Python API:** https://docs.blender.org/api/current/
- **Design Inspiration:** Vintage industrial control panels (see design references)

---

**Quick Nav:** [Handoff](NEXT_SESSION_HANDOFF.md) | [UI Docs](docs/ui/ENHANCED_UI_SUMMARY.md) | [Design Refs](docs/ui/design-references/) | [Token Tips](TOKEN_OPTIMIZATION_STRATEGIES.md)
