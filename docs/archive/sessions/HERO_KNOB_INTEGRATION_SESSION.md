# Hero Knob Integration Session - 2026-01-04

## Summary

Successfully integrated AI-generated hero knob textures into Monument Reverb plugin, with fallback to codex pre-generated assets due to orthographic projection challenges with DALL-E 3.

---

## What Was Completed

### ✅ Created HeroKnob Component
- **File:** [ui/HeroKnob.h](ui/HeroKnob.h)
- **Inherits:** `LayeredKnob` (existing JUCE component)
- **Usage:** Single rotating RGBA albedo texture layer
- **Integration:** Added to [plugin/PluginEditor.h](plugin/PluginEditor.h) as `sizeHeroKnob`
- **Position:** Grid cell (0,2) - labeled "SIZE (HERO)"

### ✅ Generated PBR Texture Maps via OpenAI API
- **Script:** [scripts/generate_monument_knob_pbr.py](scripts/generate_monument_knob_pbr.py)
- **Maps Generated:**
  - `albedo.png` - Base color (brushed aluminum, dark charcoal anodized)
  - `roughness.png` - Surface roughness data
  - `metallic.png` - Metallic vs non-metallic
  - `normal.png` - Surface detail normal map
  - `ao.png` - Ambient occlusion for depth
- **Location:** `assets/ui/hero_knob_pbr/`

### ✅ Texture Packing Pipeline
- **Script:** [scripts/pack_hero_knob_pbr.py](scripts/pack_hero_knob_pbr.py)
- **Process:**
  1. Remove white background from albedo
  2. Create circular alpha mask (90% frame fill, soft edge)
  3. Pack R=roughness, G=metallic, B=AO into single RGBA
- **Output:**
  - `albedo_rgba.png` (used in plugin rendering)
  - `packed_rmao.png` (for future PBR enhancement)

### ✅ Vision-Based Verification System
- **Script:** [scripts/generate_verified_orthographic_knob.py](scripts/generate_verified_orthographic_knob.py)
- **Purpose:** Verify orthographic projection quality using GPT-4o vision
- **Outcome:** DALL-E 3 consistently generated 3D perspective views (failed 3/3 attempts)
- **Insight:** AI image models struggle with true orthographic projection

### ✅ Codex Fallback Solution
- **Script:** [scripts/process_codex_knob.py](scripts/process_codex_knob.py)
- **Source:** `/Users/noisebox/Documents/3_Development/Repos/monument-reverb/assets/codex/ui_assets/monument-reverb/assets/rotary_knobs/brushed_aluminum_neutral_indicator.png`
- **Status:** Currently used in plugin (proper alpha transparency)

---

## File Modifications

### CMakeLists.txt
```cmake
# Line 103
assets/ui/hero_knob_pbr/albedo_rgba.png
```

### ui/HeroKnob.h
- New component class
- Uses `BinaryData::albedo_rgba_png`
- 270° rotation range (-135° to +135°)

### plugin/PluginEditor.h
- Added `HeroKnob sizeHeroKnob` member
- Positioned at cell(0, 2)

### plugin/PluginEditor.cpp
- Constructor initialization: `sizeHeroKnob(processorRef.getAPVTS(), "size", "SIZE (HERO)")`
- `addAndMakeVisible(sizeHeroKnob)`
- Layout: `sizeHeroKnob.setBounds(cell(0, 2))`

---

## Known Issues & Limitations

### 🔴 Orthographic Projection Challenge
**Problem:** DALL-E 3 generates 3D perspective views despite strict prompts
**Evidence:** All 3 vision-verified attempts showed:
- Visible 3D cylinder sides
- Shadows/highlights indicating viewing angle
- Raised knurled edges (not flat pattern)
- Perspective distortion (elliptical, not circular)

**Workaround:** Using codex pre-generated knob (already orthographic)

### 🟡 Single Layer Approach
**Current:** Only albedo texture with alpha mask
**Missing:** Normal mapping, roughness variation, dynamic lighting
**Impact:** Knob appears flat/2D without depth cues

### 🟡 No Rotation Filmstrip
**Current:** Single texture rotates via transform
**Issue:** If texture has inherent directionality (e.g., brushed metal grain)
**Better:** Generate 64-frame filmstrip for pre-rendered rotation angles

---

## Assets Directory Structure

```
assets/ui/hero_knob_pbr/
├── albedo.png                    # AI-generated (has perspective issues)
├── albedo_original.png           # Backup of first attempt
├── albedo_orthographic.png       # Second attempt (still has perspective)
├── albedo_rgba.png               # ✅ Current in plugin (from codex)
├── albedo_attempt_1.png          # Vision verification attempts
├── albedo_attempt_2.png
├── albedo_attempt_3.png
├── roughness.png                 # AI-generated PBR maps (not used yet)
├── metallic.png
├── normal.png
├── ao.png
└── packed_rmao.png               # Packed texture (for future PBR)
```

---

## Codex Assets Reference

### Available Pre-Generated Knobs
Location: `/Users/noisebox/Documents/3_Development/Repos/monument-reverb/assets/codex/ui_assets/monument-reverb/assets/rotary_knobs/`

**Materials Available:**
- Brushed aluminum (neutral, charcoal, black, off-white, olive, navy, oxide red)
- Anodized aluminum matte/gloss (various colors)
- Powder-coated steel (various colors)
- Injection-molded polymer (various colors)
- Bakelite-style phenolic (black)
- Rubberized soft-touch
- Raw machined steel

**Variants:** With/without indicator line

### Codex Tools
- **generate_knobs.py** - OpenAI API generation script
- **generate_pbr_ui_components.py** - Generate with full PBR maps
- **pbr_rgba_packer.py** - Pack maps into efficient RGBA

---

## Next Session Recommendations

### Option A: Enhance Current Knob with Full PBR
**Goal:** Add dynamic lighting using normal map + packed RMAO

**Steps:**
1. Update `LayeredKnob` to support shader-based PBR rendering
2. Load `normal.png` and `packed_rmao.png` alongside albedo
3. Implement JUCE shader or OpenGL rendering for real-time lighting
4. Add subtle directional light source in plugin UI

**Effort:** 3-4 hours
**Impact:** Professional depth and realism

---

### Option B: Generate Filmstrip for Smooth Rotation
**Goal:** Pre-render 64 rotation angles for smoother animation

**Steps:**
1. Create script to generate 64 rotated versions of albedo
2. Stitch into vertical filmstrip (1024×65536)
3. Update `HeroKnob` to use filmstrip rendering
4. Calculate frame index from parameter value

**Effort:** 1-2 hours
**Impact:** Smoother rotation, better for directional textures

---

### Option C: Try Alternative Image Generation
**Goal:** Get true orthographic projection

**Options:**
- **Midjourney** - Better control over technical views
- **Stable Diffusion** - ControlNet for exact orthographic camera
- **Manual Creation** - Photoshop/Blender with orthographic camera
- **3D AI Studio** - Upload photo, export orthographic render

**Effort:** 1-2 hours testing
**Impact:** Perfect flat texture for rotation

---

### Option D: Replace All Knobs with Codex Assets
**Goal:** Unified visual style across entire UI

**Steps:**
1. Select knob style (brushed aluminum recommended)
2. Process all codex knobs with alpha transparency
3. Replace `MonumentTimeKnob` layers with single codex texture
4. Update macro knobs (material, topology, etc.)
5. Consistent indicator line orientation

**Effort:** 2-3 hours
**Impact:** Professional consistency, cleaner aesthetic

---

## Build Commands

```bash
# Rebuild plugin
cmake --build build --target Monument_Standalone --clean-first

# Launch standalone
open build/Monument_artefacts/Debug/Standalone/Monument.app

# Kill running instance
killall Monument
```

---

## Scripts Created This Session

1. **generate_monument_knob_pbr.py** - Generate 5 PBR maps via DALL-E 3
2. **pack_hero_knob_pbr.py** - Process and pack textures with alpha
3. **regenerate_albedo_orthographic.py** - Attempt stricter orthographic prompt
4. **generate_verified_orthographic_knob.py** - Vision-based verification loop
5. **process_codex_knob.py** - Process codex knob with alpha transparency

---

## Token Usage Summary

- **Session Start:** Fresh context
- **Session End:** ~114K/200K tokens (57% used)
- **Major Operations:**
  - 5 DALL-E 3 API calls (initial PBR generation)
  - 4 additional DALL-E 3 calls (orthographic attempts)
  - 3 GPT-4o vision calls (verification)
  - 3 full plugin rebuilds

**Recommendation:** Safe to `/clear` after documenting

---

## Key Learnings

1. **AI Image Generation Limitations:**
   - DALL-E 3 struggles with true orthographic projection
   - Vision verification helps detect issues automatically
   - Pre-generated assets (codex) may be more reliable

2. **JUCE Asset Pipeline:**
   - BinaryData embeds assets at compile time
   - CMakeLists.txt regenerates BinaryData on changes
   - Clean rebuild required when changing asset sources

3. **Alpha Transparency Critical:**
   - Circular mask essential for clean rendering
   - White background removal via threshold (RGB > 240)
   - Soft edge (gaussian blur) prevents aliasing

4. **LayeredKnob Architecture:**
   - Flexible multi-layer system
   - RGBA support built-in
   - Rotation transform applied per layer
   - Easy to extend for PBR in future

---

## Follow-Up Questions for Next Session

1. **Visual Quality:** Does the codex brushed aluminum knob meet aesthetic standards?
2. **Consistency:** Should all knobs be replaced with matching codex style?
3. **Enhancement:** Worth investing in full PBR rendering with normal maps?
4. **Alternative:** Try Midjourney/Stable Diffusion for better orthographic control?

---

## Contact & References

- **Session Date:** 2026-01-04
- **Branch:** `main`
- **Commit Status:** Uncommitted (assets + code changes ready)
- **Related Docs:**
  - [NEXT_SESSION_START.md](NEXT_SESSION_START.md) (prior session handoff)
  - [HERO_KNOBS_SESSION_SUMMARY.md](HERO_KNOBS_SESSION_SUMMARY.md) (Midjourney stone knobs)
  - [assets/codex/](assets/codex/) (AI generation tools & references)

---

**Status:** ✅ Working implementation with codex knob, ready for enhancement or iteration
