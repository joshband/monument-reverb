# Next Session: Hero Knob Quality Implementation

**Created:** 2026-01-03
**Context:** Phase 4 Complete → Phase 5 Hero Knob Quality
**Session Goal:** Replace procedural knobs with photorealistic materials

---

## Session Context (Quick Recap)

### What We Discovered Today

**Problem Identified:**
- Procedural Blender knobs look "too CGI" - lack photographic realism
- Reference knob shown has exceptional quality: weathered stone, aged brass, machined threading
- Current procedural approach cannot match this quality level

**Assets Already Available:**
1. ✅ **1,300+ Midjourney UI element images** in `~/Desktop/midjourney/`
2. ✅ **Materialize pipeline** processes images → PBR maps (albedo, normal, roughness, metallic, AO)
3. ✅ **Studio HDRI downloaded** (`assets/textures/hdri/studio.hdr`) for realistic lighting
4. ✅ **10+ isolated knob images** created TODAY in `~/Desktop/midjourney_session/rotary_knob_isolated_on_pure_white*.png`

**Key Finding:**
- Complex multi-component images give poor materialize results (low confidence ~25%)
- **Isolated single-object images on white background** = IDEAL for extraction
- Your recent knobs are perfect candidates!

---

## Immediate Next Steps (Start Here)

### Step 1: Process Isolated Knobs Through Materialize (30 min)

**Goal:** Extract PBR maps from your best isolated knob images

**Commands:**
```bash
cd ~/Documents/3_Development/Repos/materialize

# Create input directory structure
mkdir -p input/hero_knobs

# Copy best isolated knobs (select 3-5 best ones)
cp ~/Desktop/midjourney_session/rotary_knob_isolated_on_pure_white_background_top_down*.png \
   input/hero_knobs/

# Run materialize pipeline
python -m materialize \
  --in input/hero_knobs \
  --out dist/hero_knobs \
  --verbose

# Check output quality
ls -lh dist/hero_knobs/*/
```

**What to Look For:**
- ✅ albedo.png should have clean color, no shadows baked in
- ✅ normal.png should show surface detail (varied colors, not flat)
- ✅ roughness.png should have material variation
- ✅ Higher confidence scores (>50%) in `components.juce.json`
- ✅ Larger primitive masks (>100KB, not 1-2KB)

**Quality Checkpoint:**
```bash
# View the results
open dist/hero_knobs/rotary_knob*/canonical.png
open dist/hero_knobs/rotary_knob*/albedo.png
open dist/hero_knobs/rotary_knob*/normal.png
```

If quality is good → Continue to Step 2
If quality is poor → Jump to Alternative Approaches below

---

### Step 2: Render Knob with Real PBR Materials (45 min)

**Goal:** Apply extracted materials to 3D knob in Blender, render filmstrip

**Script Already Created:** `scripts/render_with_real_materials.py`

**Usage:**
```bash
cd ~/Documents/3_Development/Repos/monument-reverb

# Edit script to point to your best materialize output
# Line 17: MATERIAL_NAME = "rotary_knob_isolated_[...your_best_one...]"

# Render filmstrip (60 frames, 256 samples)
blender --background --python scripts/render_with_real_materials.py

# Check output
open assets/ui/knobs_real_materials/rotary_knob_*/frame_000.png
open assets/ui/knobs_real_materials/rotary_knob_*/frame_030.png
```

**Expected Output:**
- 60 PNG frames (one per rotation degree)
- 1024×1024 resolution
- RGBA with transparency
- Real material textures applied (not procedural)

---

### Step 3: Stitch Frames into Filmstrip (15 min)

**Goal:** Combine 60 frames into single vertical filmstrip PNG

**Tool Options:**

**A. ImageMagick (Recommended):**
```bash
cd assets/ui/knobs_real_materials/rotary_knob_*/

# Vertical filmstrip (60 frames stacked)
convert frame_*.png -append filmstrip_vertical.png

# Check size (should be 1024×61440)
identify filmstrip_vertical.png
```

**B. Python Script (Alternative):**
```bash
# Create quick stitcher
python3 << 'EOF'
from PIL import Image
import glob

frames = sorted(glob.glob("frame_*.png"))
imgs = [Image.open(f) for f in frames]

# Vertical stack
width = imgs[0].width
height = imgs[0].height * len(imgs)
filmstrip = Image.new('RGBA', (width, height))

for i, img in enumerate(imgs):
    filmstrip.paste(img, (0, i * imgs[0].height))

filmstrip.save("filmstrip_vertical.png")
print(f"✓ Created {width}×{height} filmstrip")
EOF
```

**C. Use Wavesfactory Strip Generator:**
- Download: https://www.wavesfactory.com/blog/posts/strip-generator/
- Import frame folder
- Export filmstrip PNG

---

### Step 4: Integrate into JUCE (30 min)

**Goal:** Replace basic JUCE slider with filmstrip-based knob

**Reference Implementations:**
- `ui/LayeredKnob.h/.cpp` (already exists in Monument)
- [martinpenberthy/JUCE_CustomRotaryKnobPNG](https://github.com/martinpenberthy/JUCE_CustomRotaryKnobPNG) (example)
- [audioplastic/Juce-look-and-feel-examples](https://github.com/audioplastic/Juce-look-and-feel-examples) (production code)

**Quick Integration Test:**

1. **Add filmstrip to CMakeLists.txt:**
```cmake
juce_add_binary_data(MonumentAssets
  # ... existing assets ...
  assets/ui/knobs_real_materials/rotary_knob_hero/filmstrip_vertical.png
)
```

2. **Update MonumentKnob.cpp** (use filmstrip):
```cpp
// Replace in MonumentKnob::paint()
auto filmstrip = juce::ImageCache::getFromMemory(
    BinaryData::filmstrip_vertical_png,
    BinaryData::filmstrip_vertical_pngSize
);

const int frameHeight = 1024;  // Single frame size
const int numFrames = 60;
const int frameIndex = static_cast<int>(slider.getValue() * (numFrames - 1));

g.drawImage(filmstrip,
    0, 0, getWidth(), getHeight(),
    0, frameIndex * frameHeight, 1024, frameHeight
);
```

3. **Build and test:**
```bash
cmake --build build
open build/Monument_artefacts/Debug/Standalone/Monument.app
```

---

## Alternative Approaches (If Materialize Quality Poor)

### Option A: Use Midjourney Renders Directly (Fastest)

**Skip materialize entirely** - use raw images as sprite sheets:

```bash
# Select best single knob render
cp ~/Desktop/midjourney_session/rotary_knob_*_1.png \
   assets/ui/knobs_raw/hero_knob.png

# Use directly in JUCE (no PBR extraction needed)
# Just composite the flat image with rotation
```

**Pros:** Photographic quality preserved, zero processing
**Cons:** Fixed lighting, can't adjust materials

---

### Option B: Purchase Professional 3D Model (Highest Quality)

**Find model matching your reference:**

**Free Resources:**
- [TurboSquid Free Knobs](https://www.turbosquid.com/Search/3D-Models/free/knob) - 10+ free models
- [CGTrader Free](https://www.cgtrader.com/free-3d-models/knobs) - Filtered search

**Premium (Best Match):**
- [CGTrader Knobs](https://www.cgtrader.com/3d-models/knobs) - 1,863 models ($5-50)
- Search: "vintage brass knob", "industrial control", "aged metal knob"

**Workflow:**
1. Purchase/download `.blend`, `.fbx`, or `.obj` model
2. Import to Blender
3. Apply HDRI lighting (already have `studio.hdr`)
4. Render 60-frame rotation
5. Stitch filmstrip
6. Integrate into JUCE

**Time:** 2-3 hours (includes finding right model)

---

### Option C: Use Community Filmstrips (Ready-Made)

**Download production-ready knob filmstrips:**

1. **HISE Community (Free):**
   - [Analog Knob Kit by Noisehead](https://forum.hise.audio/topic/6427/free-hise-filmstrips-analog-knob-kit-01-by-noisehead)
   - Professional 3D renders, multiple styles
   - Drop directly into JUCE

2. **Audio-UI.com (Premium):**
   - [Professional GUI Elements](https://www.audio-ui.com/)
   - 128-frame filmstrips, HD PNG
   - $10-30 per knob set

3. **UI Mother (Premium Templates):**
   - [Audio UI Templates](https://uimother.com/)
   - Complete UI kits with matching knobs

**Pros:** Production-ready, immediate use
**Cons:** May not match your exact aesthetic

---

## Files Created This Session (Reference)

### Scripts
- `scripts/generate_knob_hero.py` - Advanced shaders (subsurface, anisotropic, HDRI)
- `scripts/run_hero_knob.sh` - Runner for hero knob test
- `scripts/download_textures.sh` - Download PBR textures + HDRI (already run)
- `scripts/render_with_real_materials.py` - **USE THIS** - renders with materialize PBR maps

### Assets Downloaded
- `assets/textures/hdri/studio.hdr` (6.4MB) - ✅ Ready to use
- Stone textures (pending - download failed, but optional)

### Documentation
- `docs/PHASE_4_COMPLETE_SUMMARY.md` - Full Phase 4 achievements
- `NEXT_SESSION_HERO_KNOBS.md` - **This document**

---

## Key Resources (Bookmarks)

### Texture/Material Sources
- **Poly Haven:** https://polyhaven.com/ (Free PBR textures + HDRIs)
- **FreePBR:** https://freepbr.com/ (600+ free 2K materials)
- **3D Textures:** https://3dtextures.me/ (Free seamless PBR)

### 3D Models
- **CGTrader Knobs:** https://www.cgtrader.com/3d-models/knobs
- **TurboSquid Free:** https://www.turbosquid.com/Search/3D-Models/free/knob

### JUCE Filmstrip Examples
- **JUCE_CustomRotaryKnobPNG:** https://github.com/martinpenberthy/JUCE_CustomRotaryKnobPNG
- **Juce-look-and-feel-examples:** https://github.com/audioplastic/Juce-look-and-feel-examples
- **TeragonGuiComponents:** https://github.com/teragonaudio/TeragonGuiComponents

### Community Filmstrips
- **HISE Free Knobs:** https://forum.hise.audio/topic/6427/free-hise-filmstrips-analog-knob-kit-01-by-noisehead
- **Audio-UI:** https://www.audio-ui.com/
- **UI Mother:** https://uimother.com/

---

## Success Criteria

**Minimum Viable (1-2 hours):**
- ✅ One hero knob rendered with real materials
- ✅ Integrated into Monument UI
- ✅ Visible quality improvement over procedural

**Ideal (3-4 hours):**
- ✅ 3-5 material variants (stone, brass, glass, etc.)
- ✅ All 18 Monument parameters use enhanced knobs
- ✅ Filmstrip pipeline automated
- ✅ Documentation updated

**Stretch Goal (Full day):**
- ✅ Custom 3D models for each parameter type
- ✅ Hover/active state variations
- ✅ Parameter-specific materials (Time=granite, Mass=marble, etc.)
- ✅ Export presets for other projects

---

## Troubleshooting

### Issue: Materialize gives low confidence (<30%)

**Cause:** Image has complex background or multiple objects
**Fix:** Use isolated knobs on pure white background (you already have these!)

### Issue: PBR maps look blurry/low detail

**Cause:** Source Midjourney image was low resolution
**Fix:** Regenerate Midjourney with `--ar 1:1 --q 2` for max quality

### Issue: Filmstrip knob doesn't rotate smoothly

**Cause:** Not enough frames (60 may be too few for slow parameter changes)
**Fix:** Render 90 or 120 frames for smoother animation

### Issue: Blender render is too slow (>10min per frame)

**Cause:** High sample count (512) on CPU rendering
**Fix:**
```python
# In render script, reduce samples:
RENDER_SAMPLES = 128  # Fast iteration
# Or enable GPU:
scene.cycles.device = 'GPU'
```

---

## Context for Claude (Next Session)

**What was accomplished in Phase 4:**
- ✅ ModMatrixPanel UI (4×15 grid, color-coded sources)
- ✅ Enhanced knob rendering pipeline (Blender + PBR)
- ✅ User preset system (format v3 with modulation)
- ✅ Performance optimizations (parameter cache, smoothing)
- ✅ Thread safety hardening (SpinLock, safe async)

**Current limitation:**
- Procedural Blender knobs look "too CGI"
- Need photorealistic materials from real images

**Assets ready:**
- 10+ isolated knob Midjourney renders (`~/Desktop/midjourney_session/`)
- Materialize pipeline for PBR extraction
- HDRI lighting downloaded
- Scripts created for rendering with real materials

**Next session goal:**
- Process isolated knobs → Extract PBR maps
- Render filmstrip with real materials
- Integrate into JUCE
- Replace all 18 parameters with hero knobs

**Estimated time:** 2-4 hours for MVP, full day for complete implementation

---

## Quick Start Command (Next Session)

```bash
# Start here:
cd ~/Documents/3_Development/Repos/monument-reverb

# 1. Process knobs through materialize
cd ~/Documents/3_Development/Repos/materialize
mkdir -p input/hero_knobs
cp ~/Desktop/midjourney_session/rotary_knob_isolated_*top_down*.png input/hero_knobs/
python -m materialize --in input/hero_knobs --out dist/hero_knobs

# 2. Check quality
open dist/hero_knobs/rotary_knob*/albedo.png

# 3. Render filmstrip (if quality good)
cd ~/Documents/3_Development/Repos/monument-reverb
# Edit: scripts/render_with_real_materials.py (set MATERIAL_NAME)
blender --background --python scripts/render_with_real_materials.py

# 4. Stitch frames
cd assets/ui/knobs_real_materials/rotary_knob_*/
convert frame_*.png -append filmstrip_vertical.png

# 5. Integrate into JUCE
# See Step 4 above for CMakeLists.txt + code changes
```

---

**Ready to start! Clear context, specific assets identified, multiple approaches outlined.**

**Safe to `/clear` context after this session. 🚀**
