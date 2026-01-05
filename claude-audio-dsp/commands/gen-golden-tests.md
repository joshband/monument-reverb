# /gen-golden-tests Command

Generate golden image render tests for Visual DNA RGBA components.

## Usage

```
/gen-golden-tests <componentId>
/gen-golden-tests <componentId> --size 512
```

## Purpose

Given a component manifest + layers, render offscreen and compare to golden reference. Catches:
- Layer ordering bugs
- Opacity regressions
- Blend mode changes
- Indicator transform mistakes
- Asset drift

## Output Files

```
tests/
├── RenderGolden.h
├── ImageDiff.h
├── TestGolden<Component>.cpp
├── golden/<componentId>/
│   ├── v0_default.png
│   ├── v05_default.png
│   ├── v1_default.png
│   ├── v05_pressed.png
│   └── v05_disabled.png
└── out/<componentId>/
```

## RenderGolden.h

```cpp
#pragma once
#include <JuceHeader.h>

inline juce::Image renderComponentToImage(juce::Component& c, int w, int h) {
    c.setSize(w, h);
    juce::Image img(juce::Image::ARGB, w, h, true);
    juce::Graphics g(img);
    c.paintEntireComponent(g, true);
    return img;
}

inline bool writePNG(const juce::File& f, const juce::Image& img) {
    juce::PNGImageFormat png;
    if (auto stream = f.createOutputStream())
        return png.writeImageToStream(img, *stream);
    return false;
}

inline juce::Image loadPNG(const juce::File& f) {
    juce::PNGImageFormat png;
    if (auto in = f.createInputStream())
        return png.decodeImage(*in);
    return {};
}
```

## ImageDiff.h

```cpp
#pragma once
#include <JuceHeader.h>
#include <cmath>

struct DiffResult {
    double maxAbs = 0.0;       // max per-channel absolute difference (0-255)
    double meanAbs = 0.0;      // mean per-channel absolute difference
    double badPixelRatio = 0.0; // pixels over threshold ratio
};

inline DiffResult diffImages(const juce::Image& a, const juce::Image& b,
                             int perChannelThreshold = 2)
{
    jassert(a.getWidth() == b.getWidth() && a.getHeight() == b.getHeight());
    
    const int w = a.getWidth(), h = a.getHeight();
    long double sum = 0.0;
    long long bad = 0;
    double maxAbs = 0.0;
    
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            auto pa = a.getPixelAt(x, y);
            auto pb = b.getPixelAt(x, y);
            
            int da = std::abs((int)pa.getAlpha() - (int)pb.getAlpha());
            int dr = std::abs((int)pa.getRed()   - (int)pb.getRed());
            int dg = std::abs((int)pa.getGreen() - (int)pb.getGreen());
            int db = std::abs((int)pa.getBlue()  - (int)pb.getBlue());
            
            maxAbs = std::max({maxAbs, (double)da, (double)dr, (double)dg, (double)db});
            sum += da + dr + dg + db;
            
            if (da > perChannelThreshold || dr > perChannelThreshold ||
                dg > perChannelThreshold || db > perChannelThreshold)
                bad++;
        }
    }
    
    DiffResult r;
    r.maxAbs = maxAbs;
    r.meanAbs = (double)(sum / (4.0 * w * h));
    r.badPixelRatio = (double)bad / (double)(w * h);
    return r;
}
```

## TestGoldenKnob.cpp (Template)

```cpp
#include <JuceHeader.h>
#include "RenderGolden.h"
#include "ImageDiff.h"
#include "LayeredRGBAComponent.h"
#include "ComponentManifestLoader.h"

int main() {
    const juce::String componentId = "{{componentId}}";
    const int W = {{size}}, H = {{size}};
    
    struct Case { float value; juce::String state; juce::String name; };
    Case cases[] = {
        { 0.0f, "default",  "v0_default" },
        { 0.5f, "default",  "v05_default" },
        { 1.0f, "default",  "v1_default" },
        { 0.5f, "pressed",  "v05_pressed" },
        { 0.5f, "disabled", "v05_disabled" }
    };
    
    auto cwd = juce::File::getCurrentWorkingDirectory();
    auto goldenDir = cwd.getChildFile("tests/golden").getChildFile(componentId);
    auto outDir = cwd.getChildFile("tests/out").getChildFile(componentId);
    outDir.createDirectory();
    
    // Load manifest
    auto manifest = ComponentManifestLoader::load(componentId);
    LayeredRGBAComponent comp(manifest, nullptr, "");
    
    int passed = 0, failed = 0;
    
    for (auto& tc : cases) {
        comp.setNormalizedValue(tc.value);
        comp.setState(tc.state);
        
        auto img = renderComponentToImage(comp, W, H);
        auto outFile = outDir.getChildFile(tc.name + ".png");
        writePNG(outFile, img);
        
        auto goldenFile = goldenDir.getChildFile(tc.name + ".png");
        
        if (!goldenFile.existsAsFile()) {
            std::cerr << "MISSING GOLDEN: " << goldenFile.getFullPathName() << "\n";
            std::cerr << "  Wrote candidate: " << outFile.getFullPathName() << "\n";
            failed++;
            continue;
        }
        
        auto golden = loadPNG(goldenFile);
        if (!golden.isValid()) {
            std::cerr << "FAILED TO LOAD: " << goldenFile.getFullPathName() << "\n";
            failed++;
            continue;
        }
        
        auto diff = diffImages(img, golden, 2);
        
        // Tolerance policy
        const double maxAbsLimit = 8.0;       // max per-channel delta
        const double badRatioLimit = 0.002;   // 0.2% pixels can differ
        
        if (diff.maxAbs > maxAbsLimit || diff.badPixelRatio > badRatioLimit) {
            std::cerr << "FAIL " << tc.name
                      << " maxAbs=" << diff.maxAbs
                      << " badRatio=" << diff.badPixelRatio << "\n";
            failed++;
        } else {
            std::cout << "PASS " << tc.name
                      << " maxAbs=" << diff.maxAbs
                      << " meanAbs=" << diff.meanAbs << "\n";
            passed++;
        }
    }
    
    std::cout << "\n" << passed << " passed, " << failed << " failed\n";
    return failed > 0 ? 1 : 0;
}
```

## Workflow

### Initial Setup
```bash
# First run - generates candidates
./TestGoldenKnob
# Review outputs in tests/out/knob_industrial_01/
# Copy approved renders to tests/golden/knob_industrial_01/
```

### CI Integration
```bash
# Subsequent runs - compare to golden
./TestGoldenKnob
# Exit code 0 = all passed
# Exit code 1 = visual regression detected
```

## CMake Integration

```cmake
add_executable(TestGoldenKnob 
    tests/TestGoldenKnob.cpp
    src/LayeredRGBAComponent.cpp
    src/ComponentManifestLoader.cpp
)
target_link_libraries(TestGoldenKnob PRIVATE 
    juce::juce_graphics
    juce::juce_gui_basics
)

# Add to CI
add_test(NAME GoldenKnob COMMAND TestGoldenKnob)
```

## Tolerance Tuning

| Parameter | Default | Use |
|-----------|---------|-----|
| `perChannelThreshold` | 2 | Per-channel delta to count as "bad" |
| `maxAbsLimit` | 8.0 | Fail if any channel differs by more |
| `badRatioLimit` | 0.002 | Fail if >0.2% of pixels are "bad" |

Adjust for anti-aliasing, gamma, and platform differences.

## Determinism Requirements

For reliable golden tests:
- Fixed render size
- No time-based effects
- Consistent gamma/color space
- Premultiplied alpha
- No random elements
