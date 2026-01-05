# Next Session - Hero Knob Integration
**Date:** 2026-01-04+
**Previous Session:** Hero knob PBR texture generation (2026-01-03)

---

## 🎯 Quick Context

### What We Just Completed
✅ **Hero Knobs Processing** (2026-01-03)
- Selected 3 stone knob series from 222 Midjourney images
- Developed GrabCut masking pipeline for clean background removal
- Generated 36+ PBR texture maps (albedo, normal, roughness, height, metallic, AO)
- Created 11 masking/processing scripts

**Full details:** [HERO_KNOBS_SESSION_SUMMARY.md](HERO_KNOBS_SESSION_SUMMARY.md)

### Generated Assets (Not in Git)
```
~/Documents/3_Development/Repos/materialize/dist/hero_knobs/
├── series_1/  (Best quality: 57-67% coverage)
│   ├── albedo.png
│   ├── normal.png
│   ├── roughness.png
│   ├── height.png
│   └── ... (13 more textures)
├── series_2/  (Good quality: 52-60% coverage)
└── series_3/  (Challenging: 29-49% coverage)
```

---

## 📋 This Session Options

### Option 1: Integrate Hero Knob into JUCE Plugin ⭐ RECOMMENDED
**Goal:** Use Series 1 PBR textures in Monument Reverb knob UI

**Tasks:**
1. Copy PBR textures to plugin assets
   ```bash
   mkdir -p assets/ui/hero_knob/
   cp ~/Documents/3_Development/Repos/materialize/dist/hero_knobs/series_1/*.png \
      assets/ui/hero_knob/
   ```

2. Update JUCE component to load textures
   - File: `src/ui/KnobComponent.cpp` or similar
   - Load albedo as base texture
   - Apply normal map for lighting
   - Use roughness for material properties

3. Render in plugin UI and verify quality

4. Compare with current Blender-generated knobs

**Estimated Time:** 1-2 hours

---

### Option 2: Process More Knob Variations
**Goal:** Generate PBR textures for additional knob styles

**Available in `~/Desktop/knobs/`:**
- Brass metallic knobs (8 images)
- Top-down rotary knobs (10+ images)
- Toggle switches (20 images)

**Tasks:**
1. Select best candidates using `scripts/analyze_knob_images.py`
2. Run `scripts/batch_mask_hero_knobs_grabcut.py` with new series
3. Process through materialize pipeline
4. Compare quality with stone series

**Estimated Time:** 30-45 minutes

---

### Option 3: Refine Existing Textures
**Goal:** Improve PBR texture quality through parameter tuning

**Tasks:**
1. Review materialize config: `~/Documents/3_Development/Repos/materialize/config/default.yaml`
2. Adjust parameters:
   - Normal map strength
   - Height map depth
   - Roughness variance
3. Reprocess with new settings
4. Side-by-side comparison

**Estimated Time:** 45-60 minutes

---

### Option 4: Create Composite Hero Knob
**Goal:** Manually combine best elements from multiple variations

**Tasks:**
1. Select best base texture (Series 1, variation 1 or 2)
2. Enhance LED glow using Series 2 elements
3. Add detail from Series 3 where needed
4. Process composite through materialize
5. Generate final PBR set

**Tools Needed:** Photoshop/GIMP for compositing
**Estimated Time:** 1-2 hours

---

### Option 5: Continue with Preset System
**Goal:** Return to previous task from [NEXT_SESSION_HANDOFF.md](NEXT_SESSION_HANDOFF.md)

**Task:** Modulation connection serialization (format v3)
- Serialize modulation connections in preset save/load
- Update preset browser to show modulation state

**Estimated Time:** 2-3 hours

---

## 🛠️ Available Scripts

### Knob Processing Pipeline
```bash
# Analyze images for quality
python3 scripts/analyze_knob_images.py

# Setup new series
python3 scripts/setup_hero_knobs.py

# Batch mask with GrabCut
python3 scripts/batch_mask_hero_knobs_grabcut.py

# Single image masking (for testing)
python3 scripts/mask_with_grabcut.py <image_path>
```

### Alternative Masking Methods (if GrabCut fails)
```bash
python3 scripts/mask_with_color_distance.py <image>
python3 scripts/mask_with_edge_detection.py <image>
python3 scripts/mask_with_ellipse_fit.py <image>
```

### Materialize Pipeline
```bash
cd ~/Documents/3_Development/Repos/materialize
python -m materialize --in input/hero_knobs --out dist/hero_knobs
```

---

## 📊 Quality Reference

### Series 1 Characteristics (BEST)
- **Texture:** Rich stone detail, varied surface
- **LED Glow:** Prominent warm amber center
- **Coverage:** 57-67% (excellent alpha masking)
- **Use Case:** Primary hero knob for plugin UI

### Series 2 Characteristics (GOOD)
- **Texture:** Different stone pattern, more uniform
- **LED Glow:** Strong LED presence
- **Coverage:** 52-60% (clean masking)
- **Use Case:** Alternate style or backup

### Series 3 Characteristics (CHALLENGING)
- **Texture:** Good detail but harder to mask
- **LED Glow:** Subtle, less prominent
- **Coverage:** 29-49% (more background artifacts)
- **Use Case:** Parts extraction or compositing source

---

## 🔧 Quick Commands

```bash
# View generated PBR textures
open ~/Documents/3_Development/Repos/materialize/dist/hero_knobs/series_1/

# Preview specific textures
open ~/Documents/3_Development/Repos/materialize/dist/hero_knobs/series_1/albedo.png
open ~/Documents/3_Development/Repos/materialize/dist/hero_knobs/series_1/normal.png

# Copy to plugin assets (if integrating)
mkdir -p assets/ui/hero_knob/
cp ~/Documents/3_Development/Repos/materialize/dist/hero_knobs/series_1/*.png \
   assets/ui/hero_knob/

# Source knobs location
open ~/Desktop/knobs/
```

---

## 📝 Notes

### Project Status
- **Phase 4:** UI enhancements ✅ COMPLETE
- **Current Focus:** Hero knob integration OR preset system v2
- **Branch:** `main` (up to date)

### Token Budget Reminder
- Use `/clear` after this session completes
- Target: <$6/day (~100K tokens)
- Handoff to Codex if >$10/day

### Documentation
- Session summary: [HERO_KNOBS_SESSION_SUMMARY.md](HERO_KNOBS_SESSION_SUMMARY.md)
- Phase 4 complete: [docs/PHASE_4_COMPLETE_SUMMARY.md](docs/PHASE_4_COMPLETE_SUMMARY.md)
- Token optimization: [TOKEN_OPTIMIZATION_STRATEGIES.md](TOKEN_OPTIMIZATION_STRATEGIES.md)

---

## 🎯 Recommended Starting Point

**Option 1: Integrate Hero Knob** is the most natural next step since we just generated the textures. However, if you prefer to continue with the preset system (Option 5), that's also a solid choice.

**Quick Start for Option 1:**
```bash
# 1. Preview the best textures
open ~/Documents/3_Development/Repos/materialize/dist/hero_knobs/series_1/

# 2. Copy to plugin assets
mkdir -p assets/ui/hero_knob/
cp ~/Documents/3_Development/Repos/materialize/dist/hero_knobs/series_1/*.png assets/ui/hero_knob/

# 3. Find the knob component code
grep -r "KnobComponent" src/ --include="*.cpp" --include="*.h"

# 4. Start integration
```

---

**Ready to start? Let me know which option you'd like to pursue!**
