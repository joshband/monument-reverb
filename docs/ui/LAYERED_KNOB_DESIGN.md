# Layered Knob System - Design Document

**Date:** 2026-01-03
**Goal:** Photorealistic round knobs with proper alpha channels and layered construction

---

## Architecture Overview

### Layer-Based Rendering System

```
┌─────────────────────────────────────┐
│  Layer 4: Center Cap (static)       │  ← Top layer
│  Layer 3: Indicator (rotates)       │  ← Pointer/marker
│  Layer 2: Detail Ring (static)      │  ← Scale markings
│  Layer 1: Base Body (rotates/static)│  ← Main knob body
└─────────────────────────────────────┘
        ↓
    Composited Result
```

**Current packs use 8-10 layers** (albedo, AO, roughness, specular/highlights, glow, indicator, contact shadow, and optional normal). The system supports arbitrary layer counts.

### Rotation System

**Parameter Value (0.0 - 1.0) → Rotation Angle**

```cpp
// Example: Map 0.0-1.0 to 270° sweep (standard audio knob)
float angleRadians = juce::MathConstants<float>::pi * 1.5f * normalizedValue;
// Result: 0.0 → 0°, 0.5 → 135°, 1.0 → 270°
```

---

## Asset Requirements

### File Format
- **Format:** PNG with 32-bit RGBA (alpha channel required)
- **Dimensions:** 512×512px (power of 2 for GPU efficiency)
- **Background:** Fully transparent (alpha = 0)
- **Shape:** Circular, centered in 512×512 canvas

### Layer Specifications

#### Layer 1: Base Body
- **Purpose:** Main knob body with primary material/texture
- **Rotation:** Typically rotates with parameter value
- **Style:** Brutalist concrete/stone with geometric patterns
- **Details:**
  - Carved radial lines or geometric segments
  - Depth and shadow baked into texture
  - Weathered/aged appearance
  - Example: Concentric circles carved into stone

#### Layer 2: Detail Ring (Optional)
- **Purpose:** Scale markings, graduations, or decorative ring
- **Rotation:** Usually static (doesn't rotate)
- **Style:** Subtle markings at key positions (0°, 90°, 180°, 270°)
- **Details:**
  - Could be engraved tick marks
  - Or geometric patterns around the perimeter
  - Light/shadow to create depth

#### Layer 3: Indicator
- **Purpose:** Shows current parameter value
- **Rotation:** Always rotates with parameter
- **Style:** Clear, visible pointer or marker
- **Details:**
  - Could be a raised line/ridge
  - Or a cutout/carved notch
  - Or a contrasting material (metal on stone)
  - Should be clearly visible at a glance

#### Layer 4: Center Cap (Optional)
- **Purpose:** Visual anchor, hides rotation pivot point
- **Rotation:** Always static
- **Style:** Contrasting material (e.g., metal cap on stone body)
- **Details:**
  - Could have embossed logo or pattern
  - Adds realism and craftsmanship feel

---

## Rendering Pipeline

### 1. Layer Loading
```cpp
void setLayer(int layerIndex, const void* imageData, size_t dataSize,
              bool rotates = false, float rotationOffset = 0.0f);
```

### 2. Transform Calculation
```cpp
// For each rotating layer:
juce::AffineTransform transform =
    juce::AffineTransform::rotation(currentAngle, centerX, centerY);
```

### 3. Compositing
```cpp
// Render layers bottom-to-top with alpha blending:
for (auto& layer : layers) {
    if (layer.rotates) {
        g.addTransform(layer.transform);
        g.drawImageAt(layer.image, 0, 0);
        g.resetToDefaultState();
    } else {
        g.drawImageAt(layer.image, 0, 0);
    }
}
```

---

## Asset Creation Workflow

### Option 1: Midjourney + Photoshop
1. Generate brutalist knob renders in Midjourney
2. Extract layers in Photoshop:
   - Use layer masks to isolate components
   - Ensure clean alpha channels
   - Export each layer as separate PNG

### Option 2: Blender 3D
1. Model knob in Blender (cylinder + boolean operations)
2. Apply materials (concrete, metal, stone)
3. Render each layer separately with alpha:
   - Base body
   - Indicator
   - Details
4. Export as 512×512 PNG with transparency

### Option 3: Materialize-Style Tool
1. Create base knob design (flat image)
2. Generate height/normal maps
3. Use height maps to create depth layers
4. Export layered PNGs

---

## Example: Monument Time Knob

### Layer Breakdown (Industrial/Architectural Style)

**Layer 1: Base Body (Rotates)**
- Carved concrete/stone disc
- Geometric segments (like a gear or cog)
- Weathered texture with cracks
- Subtle ambient occlusion in crevices
- 512×512 PNG with alpha

**Layer 2: Scale Ring (Static)**
- Thin ring with 12 tick marks (like clock positions)
- Engraved into stone or metal
- Positioned around perimeter
- 512×512 PNG with alpha (most pixels transparent)

**Layer 3: Indicator (Rotates)**
- Raised metal bar or carved notch
- Extends from center to outer edge
- High contrast with base (metal on stone)
- 512×512 PNG with alpha (just the indicator visible)

**Layer 4: Center Cap (Static)**
- Brushed metal or polished stone disc
- 80×80px in center of 512×512 canvas
- Could have subtle Monument logo or pattern
- 512×512 PNG with alpha

### Rotation Mapping
```cpp
// Monument reverb times: 0.1s - 10.0s
// Map to 270° sweep (standard knob range)
float angleMin = -135.0f * (pi / 180.0f);  // -135° (7:30 position)
float angleMax = +135.0f * (pi / 180.0f);  // +135° (4:30 position)
float angle = angleMin + (angleMax - angleMin) * normalizedValue;
```

---

## Component API

```cpp
class LayeredKnob : public juce::Component,
                    private juce::AudioProcessorValueTreeState::Listener
{
public:
    LayeredKnob(juce::AudioProcessorValueTreeState& state,
                const juce::String& parameterId,
                const juce::String& labelText);

    // Layer management
    void setLayer(int index, const void* imageData, size_t dataSize,
                  bool rotates = false);

    // Rotation configuration
    void setRotationRange(float startAngleDegrees, float endAngleDegrees);

    // Standard component methods
    void paint(juce::Graphics& g) override;
    void resized() override;

    // Mouse interaction (same as MonumentControl)
    void mouseDown(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;

private:
    struct Layer {
        juce::Image image;
        bool rotates;
        float rotationOffset;  // Optional fixed rotation offset
    };

    std::vector<Layer> layers;
    float currentAngle = 0.0f;
    float angleMin = -135.0f * juce::MathConstants<float>::pi / 180.0f;
    float angleMax = +135.0f * juce::MathConstants<float>::pi / 180.0f;

    // Parameter binding (same as MonumentControl)
    juce::AudioProcessorValueTreeState& apvts;
    juce::RangedAudioParameter* parameter = nullptr;

    void updateAngleFromParameter();
};
```

---

## Next Steps

1. **Create LayeredKnob component** (header + implementation)
2. **Generate brutalist knob assets** (Midjourney or Blender)
3. **Extract layers** (Photoshop or GIMP)
4. **Test with Monument Time parameter**
5. **Refine assets based on visual feedback**
6. **Replicate for remaining 17 parameters**

---

## Benefits Over Sprite Sheet Approach

| Sprite Sheets | Layered Knobs |
|---------------|---------------|
| Rectangular frames | Round with alpha |
| Linear crossfade | Proper rotation |
| Large file size | Smaller (4 layers vs 64+ frames) |
| Fixed animation | Infinite resolution (smooth rotation) |
| No depth separation | True layer compositing |

**File Size Example:**
- Sprite Sheet: 64 frames × 512×512 = ~15 MB
- Layered: 4 layers × 512×512 = ~2 MB (87% smaller)

---

## References

- JUCE Graphics::addTransform() for rotation
- JUCE AffineTransform::rotation() for angle calculation
- PNG alpha channel best practices
- Audio plugin knob UX standards (270° sweep)
