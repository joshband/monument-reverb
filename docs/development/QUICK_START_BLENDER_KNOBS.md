# Quick Start: Generate Knobs with Blender

**Goal:** Create photorealistic knob layers in 5 minutes using headless Blender (no Midjourney wait!)

---

## Current Status - Fixed LayeredKnob Rendering (2026-01-03)

✅ **Transform Fix Applied** ([ui/LayeredKnob.cpp:125-132](ui/LayeredKnob.cpp#L125-L132))
- Changed from `g.addTransform()` + `g.drawImage()` to `g.drawImageTransformed()`
- Proper transform chain: scale → translate to center → rotate around center
- All layers now aligned and scaled uniformly

✅ **Preview-First Workflow**
- **ALWAYS preview composites BEFORE building** to catch alignment issues early
- Iterate on Blender config until preview looks perfect
- Saves rebuild time and iteration cycles

✅ **Build System Standardized**
- Single `build/` directory (no more build-fetch, build-harness, build-ninja, etc.)
- Incremental builds: ~6 seconds (only changed files)
- Auto-install to ~/Library/Audio/Plug-Ins/{Components,VST3}/
- See [STANDARD_BUILD_WORKFLOW.md](STANDARD_BUILD_WORKFLOW.md) for details

---

## Prerequisites

### Install Blender (if not already installed)

```bash
# Check if Blender is installed
blender --version

# If not installed, download from:
# https://www.blender.org/download/
# Or install via Homebrew:
brew install --cask blender
```

---

## Generate Knob Layers (One Command!)

```bash
cd /Users/noisebox/Documents/3_Development/Repos/monument-reverb
./scripts/run_blender_knobs.sh
```

**What this does:**
- Runs Blender in headless mode (no GUI)
- Generates 4 knob layers procedurally:
  - `base_body_concrete.png` - Industrial concrete disc with radial segments
  - `indicator_metal.png` - Brushed metal pointer bar
  - `detail_ring_engraved.png` - Scale markings at 12 positions
  - `center_cap_brushed_metal.png` - Center cap disc
- Outputs to: `assets/ui/knobs_test/`
- Takes ~2-3 minutes

**Custom output:**
```bash
# Custom directory and resolution
./scripts/run_blender_knobs.sh --out ~/Desktop/my_knobs --size 1024
```

---

## Verify Output

```bash
ls -lh assets/ui/knobs_test/

# Should show 4 PNG files:
# layer_0_base.png         (~200-400 KB)
# layer_1_ring.png         (~150-300 KB)
# layer_2_indicator.png    (~50-100 KB)
# layer_3_cap.png          (~80-150 KB)
```

**Visual check:**
```bash
# macOS: Open in Preview
open assets/ui/knobs_test/layer_0_base.png

# Verify:
# ✅ Round shape with transparent background
# ✅ Concrete texture with radial segments
# ✅ Proper lighting and shadows
# ✅ 512×512 pixels
```

---

## Preview Composite (DO THIS BEFORE BUILDING!)

**CRITICAL: Always preview the composite to verify alignment BEFORE building the plugin!**

```bash
# Preview at multiple rotations to verify alignment
python3 scripts/preview_knob_composite.py --rotation 0
python3 scripts/preview_knob_composite.py --rotation 45
python3 scripts/preview_knob_composite.py --rotation 90

# Save to file for inspection
python3 scripts/preview_knob_composite.py --rotation 45 --out /tmp/knob_preview.png
open /tmp/knob_preview.png
```

**What to check:**
- ✅ All layers centered and aligned
- ✅ Base and indicator rotate together smoothly
- ✅ Ring and cap stay static
- ✅ No gaps or misalignment at any rotation angle
- ✅ Proper scale (all layers same size)

**If preview looks wrong:** Adjust Blender config and regenerate layers. Don't build yet!

**If preview looks perfect:** Safe to proceed with integration.

---

## Integrate with JUCE

### Step 1: Add Assets to CMakeLists.txt

Edit `CMakeLists.txt` (lines 86-95):

```cmake
juce_add_binary_data(MonumentAssets
  HEADER_NAME BinaryData.h
  NAMESPACE BinaryData
  SOURCES
    assets/ui/knobs_test/base_body_concrete.png
    assets/ui/knobs_test/indicator_metal.png
    assets/ui/knobs_test/detail_ring_engraved.png
    assets/ui/knobs_test/center_cap_brushed_metal.png
)
```

### Step 2: Add LayeredKnob Sources

Edit `CMakeLists.txt` (lines 80-85):

```cmake
# Add these lines after existing ui/ files
ui/LayeredKnob.h
ui/LayeredKnob.cpp
```

### Step 3: Create MonumentTimeKnob

Create `ui/MonumentTimeKnob.h`:

```cpp
#pragma once

#include "LayeredKnob.h"

class MonumentTimeKnob : public LayeredKnob
{
public:
    MonumentTimeKnob(juce::AudioProcessorValueTreeState& state)
        : LayeredKnob(state, "time", "TIME")
    {
        // Layer 0 (bottom): Base body - rotates
        addLayer(
            BinaryData::base_body_concrete_png,
            BinaryData::base_body_concrete_pngSize,
            true  // rotates with parameter
        );

        // Layer 1: Detail ring - static
        addLayer(
            BinaryData::detail_ring_engraved_png,
            BinaryData::detail_ring_engraved_pngSize,
            false  // stays fixed
        );

        // Layer 2: Indicator - rotates
        addLayer(
            BinaryData::indicator_metal_png,
            BinaryData::indicator_metal_pngSize,
            true  // rotates with parameter
        );

        // Layer 3 (top): Center cap - static
        addLayer(
            BinaryData::center_cap_brushed_metal_png,
            BinaryData::center_cap_brushed_metal_pngSize,
            false  // stays fixed
        );

        // Standard audio knob rotation: 270° sweep
        setRotationRange(-135.0f, +135.0f);
    }
};
```

### Step 4: Update PluginEditor

Edit `plugin/PluginEditor.h`:

```cpp
#include "ui/MonumentTimeKnob.h"

class MonumentAudioProcessorEditor : public juce::AudioProcessorEditor
{
private:
    std::unique_ptr<MonumentTimeKnob> timeKnob;  // Replace old knob
    // ...
};
```

Edit `plugin/PluginEditor.cpp`:

```cpp
MonumentAudioProcessorEditor::MonumentAudioProcessorEditor(MonumentAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Replace old chamberWallControl with layered knob
    timeKnob = std::make_unique<MonumentTimeKnob>(audioProcessor.apvts);
    addAndMakeVisible(*timeKnob);

    // Layout
    timeKnob->setBounds(20, 20, 120, 150);  // 120×120 knob + 30px label

    setSize(800, 600);
}
```

---

## Build and Test

**IMPORTANT:** Use the standardized build workflow. See [STANDARD_BUILD_WORKFLOW.md](STANDARD_BUILD_WORKFLOW.md) for details.

```bash
# Incremental build (6 seconds, auto-installs)
cmake --build build --target Monument_AU --config Release -j8
```

**No clean needed!** CMake tracks dependencies automatically:
- First build: ~40 seconds (full compile)
- Subsequent builds: ~6 seconds (only changed files)
- Auto-installs to ~/Library/Audio/Plug-Ins/Components/

**Clear DAW caches (if needed):**
```bash
killall -9 AudioComponentRegistrar
killall -9 coreaudiod
```

### Test in DAW

1. Open Logic Pro or Ableton Live
2. Rescan plugins if needed
3. Instantiate Monument on a track
4. **Verify knob behavior:**
   - ✅ Circular knob renders correctly
   - ✅ Concrete base rotates when dragging
   - ✅ Indicator bar rotates with base
   - ✅ Detail ring stays static
   - ✅ Center cap stays static
   - ✅ Smooth 270° rotation range
   - ✅ Syncs with DAW automation

---

## Customization

### Change Materials

Edit `generate_knob_blender.py` and adjust material properties:

**Make base darker (black concrete):**
```python
# Line ~103
bsdf.inputs['Base Color'].default_value = (0.15, 0.15, 0.17, 1.0)  # Darker
```

**Make indicator gold:**
```python
# Line ~149
bsdf.inputs['Base Color'].default_value = (0.83, 0.69, 0.22, 1.0)  # Gold
bsdf.inputs['Metallic'].default_value = 1.0
bsdf.inputs['Roughness'].default_value = 0.2  # Shinier
```

**More radial segments (gear-like):**
```python
# Line ~178
segment.modifiers["Array"].count = 64  # Double the segments
```

### Re-render After Changes

```bash
# After editing generate_knob_blender.py
./scripts/run_blender_knobs.sh

# Then rebuild JUCE plugin
cmake --build build --target Monument_AU --config Release
```

---

## Troubleshooting

### Blender Not Found

```bash
# Check Blender installation
which blender
ls /Applications/Blender.app/Contents/MacOS/Blender

# Set manually if needed
export BLENDER=/Applications/Blender.app/Contents/MacOS/Blender
./scripts/run_blender_knobs.sh
```

### Renders Look Wrong

**Issue:** Knob too bright/dark
**Fix:** Adjust lighting in `generate_knob_blender.py` (lines 43-66)

**Issue:** Low resolution/pixelated
**Fix:** Increase samples for better quality
```python
# Line ~81
scene.cycles.samples = 256  # Higher = better quality (but slower)
```

**Issue:** No alpha channel (square background)
**Fix:** Ensure transparent rendering is enabled
```python
# Line ~87
scene.render.film_transparent = True
```

### Build Errors

**BinaryData symbols not found:**
```bash
# Clean and rebuild
rm -rf build
cmake -B build -G Xcode
cmake --build build --target Monument_AU
```

**Header not found:**
```bash
# Verify LayeredKnob files exist
ls ui/LayeredKnob.h
ls ui/LayeredKnob.cpp
```

---

## Performance Notes

**Render Time:**
- 512×512, 128 samples: ~30-45 seconds per layer
- Total: ~2-3 minutes for all 4 layers
- Single-threaded (Cycles CPU renderer)

**Optimization:**
- Use GPU rendering if available (CUDA/OptiX/Metal)
- Lower samples for faster iterations (64 samples = ~15 seconds/layer)
- Higher samples for final quality (256+ samples)

---

## Next Steps

### Working Knob? ✅

Now replicate for remaining 17 parameters:

1. **Batch generate variations:**
   ```bash
   # Generate stone variant
   # (Edit script to change material, then re-run)
   ./scripts/run_blender_knobs.sh --out assets/ui/knobs_stone
   ```

2. **Create parameter-specific knobs:**
   ```cpp
   class MonumentMassKnob : public LayeredKnob { /* ... */ };
   class MonumentDensityKnob : public LayeredKnob { /* ... */ };
   ```

3. **Or switch to Midjourney for final assets:**
   - Use Blender knobs for MVP/testing
   - Generate high-quality Midjourney renders for production
   - Follow [LAYERED_KNOB_WORKFLOW.md](LAYERED_KNOB_WORKFLOW.md) for full pipeline

---

## Summary

✅ **5-minute workflow:**
1. Run `./scripts/run_blender_knobs.sh`
2. Update `CMakeLists.txt` to add PNGs
3. Create `MonumentTimeKnob.h`
4. Update `PluginEditor`
5. Build and test in DAW

✅ **No waiting for Midjourney**
✅ **Fully procedural and customizable**
✅ **Production-ready for testing**

**Ready to build?** Run the script now and see your first layered knob in action!