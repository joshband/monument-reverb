# Monument UI Asset Generation Workflow

**Status:** 📊 Existing assets analyzed, ready for generation/processing
**Date:** 2026-01-05

---

## Quick Summary

You have **36 existing AI-generated assets** from Midjourney:
- ✅ 12 stone knob variants (3 unique designs)
- ✅ 27 LED/crystal glow overlays
- ✅ 4 stone switches
- ✅ 5 PBR map examples

**Still needed:**
- ⭕ 3 metal core caps (brushed aluminum, brass, copper)
- ⭕ 2 rotation indicators (line pointer, dot marker)

---

## Step 1: Review Existing Assets (Visual Contact Sheets)

✅ **Contact sheets created** in `UI Mockup/`:

- [contact_sheet_stone_knobs.png](contact_sheet_stone_knobs.png) - 12 stone variants
- [contact_sheet_crystal_glows.png](contact_sheet_crystal_glows.png) - 27 glow overlays
- [contact_sheet_switches.png](contact_sheet_switches.png) - 4 switches
- [knob_assets_contact_sheet.png](knob_assets_contact_sheet.png) - Complete overview

**Action:** Open these files and identify which assets to use:
- Pick **4 best stone knob variants** (different materials/textures)
- Pick **3-4 best crystal glow overlays** (different colors/intensities)

Note the **#number** shown in the contact sheet for selection.

---

## Step 2: Generate Missing Assets via OpenAI DALL-E 3

### Prerequisites

```bash
# Install dependencies
pip3 install openai pillow requests

# Set API key
export OPENAI_API_KEY="your-openai-api-key-here"
```

### Generate Metal Cores (3 variants)

```bash
python3 tools/generate_knob_assets_api.py \
    --asset-type metal_cores \
    --output "MonumentUI_Demo/Assets/knobs" \
    --quality hd \
    --delay 10
```

This generates:
- `core/core_metal_brushed_generated.png` (aluminum)
- `core/core_metal_brass_generated.png` (brass)
- `core/core_metal_copper_generated.png` (copper)

**Cost:** ~$0.12 (3 images × $0.040 per HD image)

### Generate Rotation Indicators (2 variants)

```bash
python3 tools/generate_knob_assets_api.py \
    --asset-type indicators \
    --output "MonumentUI_Demo/Assets/knobs" \
    --quality standard \
    --delay 10
```

This generates:
- `indicator/indicator_line_generated.png`
- `indicator/indicator_dot_generated.png`

**Cost:** ~$0.04 (2 images × $0.020 per standard image)

**Total generation cost:** ~$0.16 USD

---

## Step 3: Process Existing Assets

### Analyze Current Assets

```bash
python3 tools/process_knob_assets.py \
    --input "UI Mockup/images/knobs" \
    --output "MonumentUI_Demo/Assets/knobs" \
    --analyze-only
```

### Process Selected Stone Knobs

After reviewing contact sheets, process selected variants:

```bash
# Example: Process stone knobs #1, #5, #9, #12 from contact sheet
python3 tools/process_knob_assets.py \
    --input "UI Mockup/images/knobs" \
    --output "MonumentUI_Demo/Assets/knobs" \
    --stone-variants "315a7246" "855f2c8b" "df2ebe2a"
```

**Note:** Use the UUID portion from the filename visible in contact sheet.

### Process Selected Crystal Glows

```bash
# Process 3 best glow variants
python3 tools/process_knob_assets.py \
    --input "UI Mockup/images/knobs" \
    --output "MonumentUI_Demo/Assets/knobs" \
    --glow-variants "1bdbf161" "2ff5e3dc" "9300e561"
```

This creates:
- `crystal/crystal_glow_warm.png`
- `crystal/crystal_glow_gold.png`
- `crystal/crystal_glow_amber.png`

---

## Step 4: Verify Asset Organization

Check the final structure:

```bash
tree MonumentUI_Demo/Assets/knobs/
```

**Expected structure:**

```
knobs/
├── stone/
│   ├── knob_stone_01.png (1024×1024, RGBA)
│   ├── knob_stone_02.png
│   ├── knob_stone_03.png
│   └── knob_stone_04.png
│
├── crystal/
│   ├── crystal_glow_warm.png (1024×1024, RGBA with alpha)
│   ├── crystal_glow_gold.png
│   └── crystal_glow_amber.png
│
├── core/
│   ├── core_metal_brushed_generated.png (512×512, RGBA)
│   ├── core_metal_brass_generated.png
│   └── core_metal_copper_generated.png
│
└── indicator/
    ├── indicator_line_generated.png (512×512, RGBA)
    └── indicator_dot_generated.png
```

**Verify alpha channels:**

```bash
# Check if files have proper RGBA
for f in MonumentUI_Demo/Assets/knobs/**/*.png; do
    file "$f" | grep -q "RGBA" && echo "✓ $f" || echo "✗ $f (missing alpha)"
done
```

---

## Step 5: Update CMakeLists.txt

Add new assets to the BinaryData section in [MonumentUI_Demo/CMakeLists.txt](../MonumentUI_Demo/CMakeLists.txt):

```cmake
juce_add_binary_data(MonumentUI_BinaryData
    SOURCES
        # Panels (existing)
        Assets/panels/macro_cosmos_bg.png
        Assets/panels/foundation_bg.png
        Assets/panels/modulation_nexus_bg.png
        Assets/panels/temporal_vault_bg.png

        # Headers (existing)
        Assets/headers/macro_cosmos_header.png
        Assets/headers/foundation_header.png
        Assets/headers/modulation_nexus_header.png
        Assets/headers/temporal_vault_header.png

        # Stone knobs (NEW/UPDATED)
        Assets/knobs/stone/knob_stone_01.png
        Assets/knobs/stone/knob_stone_02.png
        Assets/knobs/stone/knob_stone_03.png
        Assets/knobs/stone/knob_stone_04.png

        # Crystal glows (NEW/UPDATED)
        Assets/knobs/crystal/crystal_glow_warm.png
        Assets/knobs/crystal/crystal_glow_gold.png
        Assets/knobs/crystal/crystal_glow_amber.png

        # Metal cores (NEW)
        Assets/knobs/core/core_metal_brushed_generated.png
        Assets/knobs/core/core_metal_brass_generated.png
        Assets/knobs/core/core_metal_copper_generated.png

        # Indicators (NEW)
        Assets/knobs/indicator/indicator_line_generated.png
        Assets/knobs/indicator/indicator_dot_generated.png
)
```

---

## Step 6: Update AssetManager

Edit [MonumentUI_Demo/Source/UI/AssetManager.cpp](../MonumentUI_Demo/Source/UI/AssetManager.cpp) to load new assets:

```cpp
void AssetManager::loadAllAssets()
{
    // ... existing panel/header loads ...

    // Stone knobs
    loadImage("knob.stone.01", BinaryData::knob_stone_01_png, BinaryData::knob_stone_01_pngSize);
    loadImage("knob.stone.02", BinaryData::knob_stone_02_png, BinaryData::knob_stone_02_pngSize);
    loadImage("knob.stone.03", BinaryData::knob_stone_03_png, BinaryData::knob_stone_03_pngSize);
    loadImage("knob.stone.04", BinaryData::knob_stone_04_png, BinaryData::knob_stone_04_pngSize);

    // Crystal glows
    loadImage("knob.crystal.warm", BinaryData::crystal_glow_warm_png, BinaryData::crystal_glow_warm_pngSize);
    loadImage("knob.crystal.gold", BinaryData::crystal_glow_gold_png, BinaryData::crystal_glow_gold_pngSize);
    loadImage("knob.crystal.amber", BinaryData::crystal_glow_amber_png, BinaryData::crystal_glow_amber_pngSize);

    // Metal cores
    loadImage("knob.core.brushed", BinaryData::core_metal_brushed_generated_png, BinaryData::core_metal_brushed_generated_pngSize);
    loadImage("knob.core.brass", BinaryData::core_metal_brass_generated_png, BinaryData::core_metal_brass_generated_pngSize);
    loadImage("knob.core.copper", BinaryData::core_metal_copper_generated_png, BinaryData::core_metal_copper_generated_pngSize);

    // Indicators
    loadImage("knob.indicator.line", BinaryData::indicator_line_generated_png, BinaryData::indicator_line_generated_pngSize);
    loadImage("knob.indicator.dot", BinaryData::indicator_dot_generated_png, BinaryData::indicator_dot_generated_pngSize);
}
```

---

## Step 7: Rebuild and Test

```bash
cd monument-reverb/MonumentUI_Demo

# Clean build
rm -rf build

# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Build
cmake --build build -j8

# Run
open build/MonumentUI_Demo_artefacts/Debug/Monument\ UI\ Demo.app
```

**Expected output:**

```
[  0%] Building BinaryData assets
[ 10%] Embedding: knob_stone_01.png
[ 15%] Embedding: crystal_glow_warm.png
...
[100%] Built target Monument UI Demo
```

**Test in app:**
- Open debug console (if configured)
- Verify no "Missing asset" warnings
- Check AssetManager has all keys loaded

---

## Alternative: Use Existing Blender Pipeline

If you prefer procedural generation over AI:

```bash
# Generate all materials
./scripts/run_blender_enhanced.sh

# Copy to MonumentUI_Demo
cp assets/ui/knobs_enhanced/*.png MonumentUI_Demo/Assets/knobs/stone/

# Continue with Step 5 (update CMakeLists.txt)
```

**Blender advantages:**
- Free (no API costs)
- Physically accurate PBR
- Can generate normal/roughness/AO maps
- Reproducible

**AI generation advantages:**
- More unique/artistic results
- Faster iteration
- No Blender installation required

---

## Cost Summary

### AI Generation (OpenAI DALL-E 3)

| Asset Type | Count | Quality | Cost per | Total |
|------------|-------|---------|----------|-------|
| Metal cores | 3 | HD | $0.040 | $0.12 |
| Indicators | 2 | Standard | $0.020 | $0.04 |
| **Total** | **5** | - | - | **$0.16** |

### Blender Pipeline

| Asset Type | Count | Cost |
|------------|-------|------|
| Stone knobs | 7 materials | Free |
| Metal cores | 3 materials | Free |
| **Total** | **10+** | **$0.00** |

---

## Quality Checklist

Before marking assets as complete:

- [ ] All PNGs are 1024×1024 (stone/crystal) or 512×512 (core/indicator)
- [ ] All files have proper RGBA channels with alpha
- [ ] Background is transparent (neutral gray removed)
- [ ] Images are orthographic (no perspective distortion)
- [ ] Lighting is consistent across variants
- [ ] No text, watermarks, or UI elements
- [ ] File sizes are reasonable (<2MB each)
- [ ] Assets load correctly in AssetManager
- [ ] Visual compositing looks correct in JUCE

---

## Troubleshooting

### "Missing asset" warnings

Check asset keys match exactly:
```cpp
// AssetManager.cpp
loadImage("knob.stone.01", ...);  // Key must match

// Usage
auto img = AssetManager::instance().getImage("knob.stone.01");  // Exact match required
```

### Alpha channel not working

Verify PNG has alpha:
```bash
identify -verbose MonumentUI_Demo/Assets/knobs/stone/knob_stone_01.png | grep -i alpha
```

If missing, re-process with:
```bash
python3 tools/process_knob_assets.py --input "source" --output "dest"
```

### Generated images have wrong colors

OpenAI DALL-E 3 can be unpredictable. Try:
1. Regenerate with modified prompt
2. Add "neutral gray background (RGB 128,128,128)" explicitly
3. Use `--quality standard` for more literal interpretation
4. Post-process in Photoshop/GIMP to adjust colors

### Build errors with BinaryData

Ensure filenames match exactly:
- CMakeLists.txt: `Assets/knobs/stone/knob_stone_01.png`
- BinaryData usage: `BinaryData::knob_stone_01_png`
- Size constant: `BinaryData::knob_stone_01_pngSize`

CMake converts paths to C++ identifiers automatically.

---

## Next Steps After Assets Complete

1. **Implement StoneKnob component** (see [SESSION_HANDOFF.md](../MonumentUI_Demo/SESSION_HANDOFF.md))
2. **Test layered rendering** (stone + crystal + core compositing)
3. **Implement deterministic taxonomy** (param name → visual variant)
4. **Add animation system** (rotation curves, physics, glow)
5. **Build PanelComponent** with knobs integrated

---

## Reference Documents

- [knobPrompts.md](knobPrompts.md) - Detailed AI generation prompts
- [assetStrategy.md](assetStrategy.md) - Asset organization strategy
- [knobAnimation.md](knobAnimation.md) - Animation system design
- [SESSION_HANDOFF.md](../MonumentUI_Demo/SESSION_HANDOFF.md) - Implementation roadmap

---

## Tool Reference

### Create Contact Sheets
```bash
python3 tools/create_asset_contact_sheet.py \
    --input "UI Mockup/images/knobs" \
    --output "UI Mockup/contact_sheet.png" \
    --cols 4 \
    --thumb-size 256
```

### Generate Assets (OpenAI)
```bash
python3 tools/generate_knob_assets_api.py \
    --asset-type metal_cores \
    --count 3 \
    --quality hd \
    --delay 10
```

### Process Existing Assets
```bash
python3 tools/process_knob_assets.py \
    --input "UI Mockup/images/knobs" \
    --output "MonumentUI_Demo/Assets/knobs" \
    --stone-variants "uuid1" "uuid2" \
    --glow-variants "uuid3" "uuid4"
```

### Generate with Blender
```bash
./scripts/run_blender_enhanced.sh
```

---

**Status:** Ready to generate missing assets and process existing ones! 🚀
