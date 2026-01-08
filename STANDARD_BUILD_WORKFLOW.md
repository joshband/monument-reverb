# Standard Build Workflow for Monument

## Build Directory Standard

**Use ONE build directory:** `build/`

**Never create:** build-fetch, build-harness, build-ninja, build-ninja-debug, etc.

## Canonical Commands

```bash
./scripts/build_macos.sh
./scripts/open_xcode.sh
cmake -S . -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build --config Release
ctest --test-dir build -C Release
```

## Initial Setup (First Time Only)

```bash
# Configure CMake project (creates build/ directory)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# OR with Ninja for faster builds:
cmake -B build -G "Ninja Multi-Config"
```

## Incremental Builds (After Code Changes)

```bash
# Build only changed files (6-10 seconds)
cmake --build build --target Monument_AU --config Release -j8
```

**No clean needed!** CMake tracks dependencies automatically. Builds take:
- First build: ~40 seconds (full compile)
- Subsequent builds: ~6-10 seconds (only changed files)

## Auto-Install

Builds automatically install to:
- `~/Library/Audio/Plug-Ins/Components/Monument.component` (AU)
- `~/Library/Audio/Plug-Ins/VST3/Monument.vst3` (VST3)

Just reload your DAW to test.

## Knob Development Workflow

### 1. Generate Blender Layers
```bash
./scripts/run_blender_knobs.sh --out assets/ui/knobs_test
```

### 2. Preview Composite (BEFORE building!)
```bash
# Preview at any rotation to verify alignment
python3 scripts/preview_knob_composite.py --rotation 0
python3 scripts/preview_knob_composite.py --rotation 45
python3 scripts/preview_knob_composite.py --rotation 90

# Save to file for inspection
python3 scripts/preview_knob_composite.py --rotation 45 --out /tmp/knob.png
open /tmp/knob.png
```

**Iterate on Blender/layer configuration until preview looks perfect!**

### 3. Add Layers to CMakeLists.txt
```cmake
juce_add_binary_data(MonumentAssets SOURCES
    assets/ui/knobs_test/layer_0_base.png
    assets/ui/knobs_test/layer_1_ring.png
    assets/ui/knobs_test/layer_2_indicator.png
    assets/ui/knobs_test/layer_3_cap.png
)
```

### 4. Create Knob Wrapper Header
```cpp
// ui/MonumentTimeKnob.h
class MonumentTimeKnob : public LayeredKnob {
    addLayer(BinaryData::layer_0_base_png,
             BinaryData::layer_0_base_pngSize,
             true, 0.0f);  // rotates
    // ... etc
}
```

### 5. Incremental Build
```bash
cmake --build build --target Monument_AU --config Release -j8
```

### 6. Test in DAW
Reload your DAW - Monument.component was auto-installed!

## Troubleshooting

### Build fails with "command not found"
```bash
# Make sure you're in project root
cd /path/to/monument-reverb
cmake --build build --target Monument_AU --config Release -j8
```

### Layers look misaligned in DAW
```bash
# Use preview tool to verify layers BEFORE building
python3 scripts/preview_knob_composite.py --rotation 45
```

### Want to force full rebuild
```bash
rm -rf build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target Monument_AU --config Release -j8
```

## Build Targets

- `Monument_AU` - Audio Unit plugin
- `Monument_VST3` - VST3 plugin
- `Monument` - Shared code (auto-built as dependency)
- `MonumentAssets` - Binary data (auto-built as dependency)

## Tests

```bash
ctest --test-dir build -C Release
```
