# Monument Reverb - Project Context

## About This Project
**Type:** JUCE C++ Audio Plugin (VST3/AU/AUv3)
**Current Focus:** Enhanced knob UI with photorealistic 3D geometry
**Main Feature:** High-quality reverb effect with professional UI controls

## Current Session Task
**Date:** 2026-01-03
**Task:** ✅ Hero Knob PBR Texture Generation (Complete)
**Handoff Doc:** [NEXT_SESSION_START.md](NEXT_SESSION_START.md)
**Next Options:** Hero knob integration OR Preset System v2 (modulation serialization)

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

## Latest Work: Hero Knob PBR Textures

### What Was Completed (2026-01-03)

✅ **Processed 222 Midjourney knob images**
✅ **Developed GrabCut masking pipeline** (5 algorithms tested)
✅ **Generated 36+ PBR texture maps** (albedo, normal, roughness, height, metallic, AO)
✅ **Created 11 processing scripts** (masking, analysis, batch workflows)

### Generated Assets (Not in Git)

```text
~/Documents/3_Development/Repos/materialize/dist/hero_knobs/
├── series_1/  (Best: 57-67% coverage) ⭐ RECOMMENDED
├── series_2/  (Good: 52-60% coverage)
└── series_3/  (Challenging: 29-49% coverage)
```

### Quick Access

```bash
# View PBR textures
open ~/Documents/3_Development/Repos/materialize/dist/hero_knobs/series_1/

# Full session details
cat HERO_KNOBS_SESSION_SUMMARY.md

# Integration guide
cat NEXT_SESSION_START.md
```

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
