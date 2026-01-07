# Knob Asset Generation Prompts - Production Grade

**Purpose:** Generate layered PBR knob assets for Monument's photorealistic UI system.

This document provides **exact prompts** for AI image generation (Midjourney/DALL-E/Stable Diffusion) plus instructions for using the existing Blender pipeline for procedural generation.

---

## GLOBAL CONSTRAINTS (APPLY TO ALL PROMPTS)

**Always include these lines verbatim:**

> photorealistic, material-accurate, industrial macro photography
> not illustration, not concept art, not fantasy art
> physically-based rendering, studio-controlled lighting
> no text, no symbols, no UI elements, no icons
> neutral gray background (RGB 128,128,128)
> ultra high detail, 4K macro surface fidelity
> orthographic or near-orthographic camera
> perfectly centered, no perspective distortion
> clean alpha channel for transparency

---

## ASSET TAXONOMY

Monument knobs use a **4-layer compositing system**:

```
Knob Visual Stack (bottom to top):
├─ 1. Stone Base (albedo + surface detail)
├─ 2. Crystal Glow (RGBA with alpha for blending)
├─ 3. Metal Core (specular center cap)
└─ 4. Rotation Indicator (pointer line or mark)
```

Each layer is rendered separately at **512×512px or 1024×1024px** with proper RGBA.

---

# LAYER 1: STONE BASE KNOBS

**Purpose:** Primary tactile surface with geological authenticity.

## Stone Knob Variant 01 - Polished Granite

### Prompt

```
A photorealistic circular control knob carved from polished dark granite,
perfectly centered, viewed from directly above (orthographic).

The knob is cylindrical with a subtle dome top,
diameter approximately 40mm, height visible from side profile.

Surface shows fine mineral flecks (quartz, mica, feldspar),
subtle blue-gray veining beneath the surface,
polished to a satin finish with micro-scratches from use.

Edges are slightly beveled with gentle wear,
no sharp corners, feels hand-machined.

Lighting is soft studio setup from 45° above,
minimal shadow, controlled highlights only,
no rim lighting, no dramatic effects.

The object feels ancient, heavy, engineered,
like a precision instrument carved from stone.

photorealistic, material-accurate, industrial macro photography
not illustration, not concept art
physically-based rendering, studio-controlled lighting
no text, no symbols, no decorative patterns
neutral gray background (RGB 128,128,128)
ultra high detail, 4K macro surface fidelity
orthographic camera, perfectly centered
clean alpha channel
```

**Resolution:** 1024×1024px
**Format:** PNG with RGBA
**Alpha:** Knob shape opaque, background transparent

---

## Stone Knob Variant 02 - Weathered Basalt

### Prompt

```
A photorealistic circular control knob cut from dark weathered basalt,
perfectly centered, orthographic top view.

The knob shows more surface texture than polished granite:
fine pitting, microscopic cracks, geological age visible.
Surface is matte with subtle variations in darkness.

Edges are slightly worn, not perfectly circular,
giving an excavated/archaeological feeling.

No visible veining, more uniform density,
feels heavier and older than granite.

Lighting: soft overhead studio light,
no glow, no energy effects, minimal shadows.

photorealistic, material-accurate, industrial macro photography
not illustration, not fantasy art
studio-controlled lighting
no text, no symbols
neutral gray background (RGB 128,128,128)
orthographic camera, perfectly centered
clean alpha channel
```

**Resolution:** 1024×1024px
**Format:** PNG with RGBA

---

## Stone Knob Variant 03 - Pale Limestone

### Prompt

```
A photorealistic circular control knob carved from pale limestone,
cream to light gray coloration with subtle warm tone,
perfectly centered, orthographic top view.

Surface is semi-polished with visible sedimentary texture,
faint horizontal stratification lines,
softer appearance than granite or basalt.

Edges show slight chamfer with organic wear patterns,
feels refined but not over-engineered.

Lighting: neutral studio setup, soft and even,
no dramatic highlights, minimal shadow.

photorealistic, material-accurate, industrial macro photography
not illustration
studio-controlled lighting
no text, no patterns
neutral gray background (RGB 128,128,128)
orthographic camera, perfectly centered
clean alpha channel
```

**Resolution:** 1024×1024px
**Format:** PNG with RGBA

---

## Stone Knob Variant 04 - Obsidian Glass

### Prompt

```
A photorealistic circular control knob carved from black obsidian,
volcanic glass with deep dark surface,
perfectly centered, orthographic top view.

Surface is highly polished with subtle conchoidal fracture patterns,
almost mirror-like but with natural stone imperfections.
Minimal texture, mostly smooth with rare flow lines.

Edges are sharp but chamfered for safety,
feels precise and engineered despite organic material.

Lighting: controlled to show subtle surface reflections
without overwhelming the dark material,
soft key light from 45° above.

photorealistic, material-accurate, industrial macro photography
physically-based rendering
no text, no symbols
neutral gray background (RGB 128,128,128)
orthographic camera, perfectly centered
clean alpha channel
```

**Resolution:** 1024×1024px
**Format:** PNG with RGBA

---

# LAYER 2: CRYSTAL GLOW OVERLAYS

**Purpose:** Energy state visualization, RGBA with variable alpha for blending.

## Crystal Glow 01 - Deep Blue Energy

### Prompt

```
A photorealistic circular crystal glow effect,
deep blue luminescence emanating from within translucent crystal,
perfectly centered, orthographic top view.

The glow is soft and volumetric,
suggesting depth and internal structure.
Light diffuses through crystal facets with subtle caustics.

Edge of glow is soft gradual falloff (not hard edge),
core is brighter, outer regions fade naturally.

Color: deep blue (RGB ~60, 100, 180) with cool white highlights.
Intensity: medium, suggesting active but not maximum energy.

No visible stone or solid material,
this layer is pure light and crystal effect.

Render with transparent background (RGBA),
alpha channel represents glow intensity (strong in center, fade to edges).

photorealistic, material-accurate, crystal volumetric lighting
not illustration, not fantasy art
soft studio lighting to enhance translucency
no text, no symbols
transparent background with proper alpha channel
orthographic camera, perfectly centered
ultra high detail crystal internal structure
```

**Resolution:** 1024×1024px
**Format:** PNG with RGBA
**Alpha:** Gradient from opaque center (255) to transparent edges (0)

---

## Crystal Glow 02 - Warm Gold Energy

### Prompt

```
A photorealistic circular crystal glow effect,
warm golden-orange luminescence from within crystal,
perfectly centered, orthographic top view.

Glow suggests heat and vitality,
warm color temperature with subtle amber undertones.
Internal crystal structure shows prismatic refraction.

Edge falloff is gradual and organic,
no sharp cutoff, natural light dispersion.

Color: warm gold (RGB ~200, 140, 60) with white-hot core.
Intensity: high but controlled, suggesting peak energy state.

No solid material visible,
pure glow and crystal transparency effect only.

Render with transparent background (RGBA),
alpha channel maps glow intensity falloff.

photorealistic, crystal volumetric lighting
soft studio enhancement
no text, no symbols
transparent background with proper alpha channel
orthographic camera, perfectly centered
```

**Resolution:** 1024×1024px
**Format:** PNG with RGBA
**Alpha:** Gradient from opaque center to transparent edges

---

## Crystal Glow 03 - Cool White Energy

### Prompt

```
A photorealistic circular crystal glow effect,
cool white luminescence with slight blue tint,
perfectly centered, orthographic top view.

Glow is clean and precise,
suggesting clarity and control rather than heat.
Crystal structure is more uniform, less chaotic than colored glows.

Edge falloff is very gradual,
creates soft halo effect.

Color: cool white (RGB ~220, 230, 245) with ice-blue highlights.
Intensity: medium-high, suggesting stable high energy.

No solid material,
pure volumetric crystal glow only.

Render with transparent background (RGBA),
alpha channel for blending.

photorealistic, crystal volumetric lighting
studio-controlled
transparent background with proper alpha channel
orthographic camera, perfectly centered
```

**Resolution:** 1024×1024px
**Format:** PNG with RGBA

---

# LAYER 3: METAL CORE CAPS

**Purpose:** Central touch point with specular reflections.

## Metal Core 01 - Brushed Aluminum

### Prompt

```
A photorealistic circular metal cap for a knob center,
brushed aluminum with linear grain pattern,
perfectly centered, orthographic top view.

Diameter approximately 15mm (smaller than full knob),
slight dome profile, edges chamfered smoothly.

Surface shows fine circular brushing marks radiating from center,
creating anisotropic reflections (light streaks rotate with viewing angle).

Color: neutral aluminum (RGB ~180, 180, 185),
satin finish, not mirror polished but not matte.

Edges catch light with subtle highlight,
minimal shadow, controlled studio lighting.

No text, no markings, pure geometric form.

Render on transparent background (RGBA),
alpha channel defines circular cap boundary.

photorealistic, material-accurate, industrial macro photography
physically-based metal rendering
studio-controlled lighting for anisotropic highlights
no text, no symbols
transparent background with proper alpha channel
orthographic camera, perfectly centered
```

**Resolution:** 512×512px (smaller than full knob)
**Format:** PNG with RGBA
**Alpha:** Circular cap opaque, background transparent

---

## Metal Core 02 - Polished Brass

### Prompt

```
A photorealistic circular metal cap for a knob center,
polished brass with warm golden color,
perfectly centered, orthographic top view.

Diameter approximately 15mm,
slight dome with smooth chamfered edges.

Surface is highly polished with subtle wear,
shows specular reflections and minor aging patina (not heavy oxidation).

Color: warm brass (RGB ~195, 165, 105),
high gloss but not mirror-perfect.

Lighting shows material reflectivity,
soft key light creates smooth gradient highlight.

Render on transparent background (RGBA).

photorealistic, material-accurate, industrial macro photography
physically-based metal rendering
studio lighting
transparent background with proper alpha channel
orthographic camera, perfectly centered
```

**Resolution:** 512×512px
**Format:** PNG with RGBA

---

## Metal Core 03 - Copper with Patina

### Prompt

```
A photorealistic circular metal cap for a knob center,
copper with subtle green-blue patina forming,
perfectly centered, orthographic top view.

Diameter approximately 15mm,
dome profile with chamfered edges.

Surface shows aged copper with irregular patina patterns,
mix of copper color (warm) and verdigris (cool green-blue).
Not fully oxidized, partial aging only.

Finish: semi-matte with areas of remaining shine.

Color: aged copper blend (base RGB ~180, 120, 100,
patina spots RGB ~100, 140, 130).

Render on transparent background (RGBA).

photorealistic, material-accurate, aged metal photography
physically-based rendering
studio lighting
transparent background with proper alpha channel
orthographic camera, perfectly centered
```

**Resolution:** 512×512px
**Format:** PNG with RGBA

---

# LAYER 4: ROTATION INDICATORS

**Purpose:** Visual pointer for knob angle, simple geometry.

## Rotation Indicator 01 - Linear Pointer

### Prompt

```
A simple linear pointer indicator for a rotary knob,
thin rectangular bar or line extending from center,
perfectly centered, orthographic top view.

Dimensions: 2-3mm wide, 20mm long,
positioned vertically (pointing to 12 o'clock).

Material: brushed aluminum or white painted metal,
very simple, minimal detail, functional not decorative.

Color: light neutral (RGB ~200, 200, 205) OR pure white.

Render on transparent background (RGBA),
alpha channel defines pointer shape only.

No shadows, no complex lighting,
this is a simple geometric overlay.

minimal photorealistic style
clean geometric form
no embellishments
transparent background with proper alpha channel
orthographic camera, perfectly centered
```

**Resolution:** 512×512px
**Format:** PNG with RGBA
**Alpha:** Pointer opaque, background fully transparent

---

## Rotation Indicator 02 - Dot Marker

### Prompt

```
A small circular dot marker for a rotary knob position indicator,
simple geometric circle near the edge of knob radius,
perfectly centered composition, orthographic top view.

Dot diameter: 4-5mm,
positioned at approximately 80% of knob radius from center,
at 12 o'clock position.

Color: pure white (RGB 255, 255, 255) OR light gray (RGB 220, 220, 220).

Render on transparent background (RGBA),
alpha channel defines dot only.

No shadows, no effects,
pure geometric marker.

minimal geometric style
clean simple form
transparent background with proper alpha channel
orthographic camera, perfectly centered
```

**Resolution:** 512×512px
**Format:** PNG with RGBA

---

# BLENDER PIPELINE (PROCEDURAL GENERATION)

**Existing scripts** in [scripts/generate_knob_blender_enhanced.py](../scripts/generate_knob_blender_enhanced.py) can generate:

## Supported Materials

- `granite` - Dark with mineral flecks
- `marble` - Light with veining
- `basalt` - Heavy uniform dark
- `glass` - Transparent with refractions
- `brushed_metal` - Anisotropic aluminum
- `brass` - Warm metal with aging
- `oxidized_copper` - Verdigris patina

## Usage

### Generate Single Material

```bash
cd monument-reverb
blender --background --python scripts/generate_knob_blender_enhanced.py -- --material granite
```

### Generate All Materials

```bash
./scripts/run_blender_enhanced.sh
```

### Output Location

Generated assets go to:
```
assets/ui/knobs_enhanced/
├── granite_body.png
├── granite_indicator.png
├── granite_ring.png
└── ...
```

## Advantages of Blender Pipeline

✅ **Pros:**
- Physically-accurate PBR materials
- Consistent lighting across all variants
- Procedural aging/weathering control
- Can generate normal/roughness/AO maps
- Fully reproducible renders

⚠️ **Cons:**
- Requires Blender installed
- Procedural look may be less unique than AI generation
- Requires Python/Blender knowledge to modify

---

# ASSET ORGANIZATION STRATEGY

## Recommended Folder Structure

```
MonumentUI_Demo/Assets/knobs/
├─ stone/
│  ├─ knob_stone_01.png (polished granite)
│  ├─ knob_stone_02.png (weathered basalt)
│  ├─ knob_stone_03.png (pale limestone)
│  └─ knob_stone_04.png (obsidian glass)
│
├─ crystal/
│  ├─ crystal_glow_blue.png (RGBA)
│  ├─ crystal_glow_gold.png (RGBA)
│  └─ crystal_glow_white.png (RGBA)
│
├─ core/
│  ├─ core_metal_brushed.png (aluminum)
│  ├─ core_metal_brass.png (polished brass)
│  └─ core_metal_copper.png (aged copper)
│
└─ indicator/
   ├─ indicator_line.png (linear pointer)
   └─ indicator_dot.png (dot marker)
```

## File Specifications

| Layer | Size | Format | Alpha Channel |
|-------|------|--------|---------------|
| Stone Base | 1024×1024 | PNG | Yes (circular mask) |
| Crystal Glow | 1024×1024 | PNG | Yes (gradient falloff) |
| Metal Core | 512×512 | PNG | Yes (circular) |
| Indicator | 512×512 | PNG | Yes (shape only) |

---

# GENERATION WORKFLOW

## Option A: AI Generation (Recommended for Unique Look)

1. **Copy prompts** from sections above
2. **Generate in Midjourney/DALL-E/Stable Diffusion**:
   - Use `--ar 1:1` for square format
   - Request highest resolution available
   - Generate multiple variations, pick best
3. **Post-process in Photoshop/GIMP**:
   - Ensure exactly 1024×1024px (stone/crystal) or 512×512px (core/indicator)
   - Add proper alpha channel (use Magic Wand to select background)
   - Export as PNG with transparency
4. **Test in JUCE**:
   - Load in AssetManager
   - Verify compositing looks correct
   - Check alpha blending

## Option B: Blender Pipeline (Faster, PBR-Correct)

1. **Run generation script**:
   ```bash
   ./scripts/run_blender_enhanced.sh
   ```
2. **Copy assets** to MonumentUI_Demo:
   ```bash
   cp assets/ui/knobs_enhanced/*.png MonumentUI_Demo/Assets/knobs/stone/
   ```
3. **Update CMakeLists.txt** BinaryData section if needed
4. **Rebuild project**

## Option C: Hybrid (Best of Both Worlds)

- Use **AI generation** for stone bases (unique photorealism)
- Use **Blender** for metal cores (perfect PBR materials)
- Use **simple vectors** for indicators (infinite scalability)

---

# QUALITY CHECKLIST

Before adding assets to project, verify:

- [ ] **Resolution correct** (1024×1024 or 512×512)
- [ ] **Format is PNG** with RGBA channels
- [ ] **Alpha channel exists** and is correct
- [ ] **Background is transparent** (neutral gray removed)
- [ ] **Orthographic view** (no perspective distortion)
- [ ] **Perfectly centered** in frame
- [ ] **Lighting consistent** across all variants
- [ ] **No text or UI elements** embedded
- [ ] **File size reasonable** (<2MB per asset)

---

# TESTING ASSETS IN JUCE

After generating assets:

1. **Add to CMakeLists.txt**:
```cmake
juce_add_binary_data(MonumentUI_BinaryData
    SOURCES
        Assets/knobs/stone/knob_stone_01.png
        Assets/knobs/crystal/crystal_glow_blue.png
        Assets/knobs/core/core_metal_brushed.png
        Assets/knobs/indicator/indicator_line.png
)
```

2. **Rebuild**:
```bash
cmake --build build -j8
```

3. **Test compositing** in StoneKnob component:
```cpp
// Layer order: stone → crystal → core → indicator
g.drawImageWithin(stoneLayer, bounds, RectanglePlacement::centred);
g.setOpacity(glowIntensity);
g.drawImageWithin(crystalLayer, bounds, RectanglePlacement::centred);
g.setOpacity(1.0f);
g.drawImageWithin(coreLayer, bounds, RectanglePlacement::centred);
```

4. **Verify performance**:
   - Check GPU usage with Instruments (macOS)
   - Ensure <2% CPU idle with all knobs rendered
   - Test with 21 knobs simultaneously

---

# ADVANCED: PBR TEXTURE MAPS (FUTURE)

For next-level realism, generate separate PBR maps:

## Stone Base Maps

- **Albedo** (base color, no lighting)
- **Normal** (surface bumps, tangent space)
- **Roughness** (matte vs glossy per-pixel)
- **Ambient Occlusion** (crevice shadows)
- **Height/Displacement** (optional, for parallax)

Use these in real-time shader for dynamic lighting response.

**Tools for PBR generation:**
- **Substance Designer** (procedural PBR)
- **Materialize** (photos → PBR maps)
- **NormalMap-Online** (free web tool)

---

# PROMPTS SUMMARY

This document provides:

✅ **4 stone base variants** (granite, basalt, limestone, obsidian)
✅ **3 crystal glow overlays** (blue, gold, white)
✅ **3 metal core caps** (aluminum, brass, copper)
✅ **2 rotation indicators** (line, dot)
✅ **Blender pipeline instructions**
✅ **Complete workflow guidance**

Total assets to generate: **12 PNG files** (or use existing + Blender pipeline).

---

**Next Steps:**

1. Choose generation method (AI, Blender, or hybrid)
2. Generate missing assets (primarily metal cores + extra crystal glows)
3. Test in JUCE MonumentUI_Demo project
4. Iterate on compositing and glow intensity

See [SESSION_HANDOFF.md](../MonumentUI_Demo/SESSION_HANDOFF.md) for integration into StoneKnob component.
