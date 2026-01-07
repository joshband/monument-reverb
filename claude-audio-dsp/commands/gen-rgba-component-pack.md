# /gen-rgba-component-pack Command

Generate a photorealistic layered RGBA component pack.

## Usage

```
/gen-rgba-component-pack <type> [style]
```

## Supported Types

- `knob` - Rotary knob
- `toggle` - Toggle switch
- `slider` - Linear slider

## Output

```
ComponentName/
├── component.manifest.json
├── layers/
│   ├── layer_01_shadow__default.png
│   ├── layer_02_base__default.png
│   ├── layer_03_material__default.png
│   ├── layer_04_ao__default.png
│   ├── layer_05_spec__default.png
│   ├── layer_06_gloss__default.png
│   ├── layer_07_indicator__default.png
│   ├── layer_08_focusRing__focus.png
│   └── layer_09_disabledOverlay__disabled.png
└── thumbs/
    └── preview_default.png
```

## Knob Manifest Template

```json
{
  "schemaVersion": "1.0.0",
  "component": {
    "id": "knob_{{style}}_01",
    "type": "knob.rotary",
    "displayName": "{{Style}} Knob 01",
    "authoring": {
      "unit": "px",
      "canvas": {"width": 1024, "height": 1024},
      "dpiHint": 144,
      "exportFormat": "png",
      "premultipliedAlpha": true
    },
    "geometry": {
      "anchor": {"x": 0.5, "y": 0.5},
      "hitRegion": {"type": "circle", "cx": 0.5, "cy": 0.5, "r": 0.46}
    },
    "behavior": {
      "valueDomain": {"min": 0.0, "max": 1.0},
      "rotation": {
        "enabled": true,
        "pivot": {"x": 0.5, "y": 0.5},
        "startDegrees": -135.0,
        "sweepDegrees": 270.0,
        "appliesToLayerId": "indicator"
      },
      "states": ["default", "focus", "pressed", "disabled"]
    }
  },
  "layers": [
    {"id": "pressedShadow", "role": "shadow.pressed", "file": "layers/layer_10_pressedShadow__pressed.png", "z": 10, "blend": "multiply", "defaultOpacity": 0.0, "stateOpacity": {"pressed": 0.55}},
    {"id": "shadow", "role": "shadow.contact", "file": "layers/layer_01_shadow__default.png", "z": 20, "blend": "multiply", "defaultOpacity": 0.85},
    {"id": "base", "role": "base.diffuse", "file": "layers/layer_02_base__default.png", "z": 30, "blend": "normal", "defaultOpacity": 1.0},
    {"id": "material", "role": "material.detail", "file": "layers/layer_03_material__default.png", "z": 40, "blend": "normal", "defaultOpacity": 1.0},
    {"id": "ao", "role": "lighting.ao", "file": "layers/layer_04_ao__default.png", "z": 50, "blend": "multiply", "defaultOpacity": 0.65},
    {"id": "spec", "role": "lighting.specular", "file": "layers/layer_05_spec__default.png", "z": 60, "blend": "screen", "defaultOpacity": 0.55},
    {"id": "gloss", "role": "lighting.gloss", "file": "layers/layer_06_gloss__default.png", "z": 70, "blend": "screen", "defaultOpacity": 0.45},
    {"id": "indicator", "role": "indicator.rotary", "file": "layers/layer_07_indicator__default.png", "z": 80, "blend": "normal", "defaultOpacity": 1.0, "transform": {"type": "rotateByValue", "pivot": {"x": 0.5, "y": 0.5}, "startDegrees": -135.0, "sweepDegrees": 270.0}},
    {"id": "focusRing", "role": "state.focusRing", "file": "layers/layer_08_focusRing__focus.png", "z": 90, "blend": "screen", "defaultOpacity": 0.0, "stateOpacity": {"focus": 0.8}},
    {"id": "disabledOverlay", "role": "state.disabledOverlay", "file": "layers/layer_09_disabledOverlay__disabled.png", "z": 100, "blend": "normal", "defaultOpacity": 0.0, "stateOpacity": {"disabled": 0.75}}
  ],
  "stateLogic": {
    "default": {"visible": ["shadow", "base", "material", "ao", "spec", "gloss", "indicator"]},
    "focus": {"visible": ["shadow", "base", "material", "ao", "spec", "gloss", "indicator", "focusRing"], "opacityOverrides": {"focusRing": 0.8}},
    "pressed": {"visible": ["pressedShadow", "shadow", "base", "material", "ao", "spec", "gloss", "indicator"], "opacityOverrides": {"pressedShadow": 0.55}},
    "disabled": {"visible": ["shadow", "base", "material", "ao", "spec", "gloss", "indicator", "disabledOverlay"], "opacityOverrides": {"disabledOverlay": 0.75, "spec": 0.25, "gloss": 0.2}}
  }
}
```

## State Table

| State | Layer Changes | Notes |
|-------|---------------|-------|
| default | Baseline stack | Indicator rotates with value |
| focus | +focusRing @0.8 | iPadOS focus ring |
| pressed | +pressedShadow @0.55 | Subtle depth cue |
| disabled | +disabledOverlay, reduce spec/gloss | Visually inactive |

## SwiftUI Compositing

```swift
struct LayeredKnob: View {
    let assets: KnobAssets
    @Binding var value: Double
    var state: String = "default"
    
    var angle: Angle { .degrees(-135 + value * 270) }
    
    func opacity(_ layer: String) -> Double {
        switch (state, layer) {
        case ("focus", "focusRing"): return 0.8
        case ("pressed", "pressedShadow"): return 0.55
        case ("disabled", "disabledOverlay"): return 0.75
        case ("disabled", "spec"): return 0.25
        case ("disabled", "gloss"): return 0.2
        case (_, "focusRing"), (_, "disabledOverlay"), (_, "pressedShadow"): return 0.0
        default: return 1.0
        }
    }
    
    var body: some View {
        ZStack {
            assets.pressedShadow.opacity(opacity("pressedShadow")).blendMode(.multiply)
            assets.shadow.opacity(0.85).blendMode(.multiply)
            assets.base
            assets.material
            assets.ao.opacity(0.65).blendMode(.multiply)
            assets.spec.opacity(opacity("spec") * 0.55).blendMode(.screen)
            assets.gloss.opacity(opacity("gloss") * 0.45).blendMode(.screen)
            assets.indicator.rotationEffect(angle)
            assets.focusRing.opacity(opacity("focusRing")).blendMode(.screen)
            assets.disabledOverlay.opacity(opacity("disabledOverlay"))
        }
        .compositingGroup()
        .drawingGroup(opaque: false, colorMode: .linear)
    }
}
```

## JUCE Compositing

```cpp
void KnobView::paint(juce::Graphics& g) {
    auto draw = [&](const juce::Image& img, float op = 1.0f) {
        g.setOpacity(op);
        g.drawImageWithin(img, 0, 0, getWidth(), getHeight(),
                          juce::RectanglePlacement::centred, false);
    };
    
    if (state == Pressed) draw(pressedShadow, 0.55f);
    draw(shadow, 0.85f);
    draw(base);
    draw(material);
    draw(ao, 0.65f);
    draw(spec, state == Disabled ? 0.14f : 0.55f);
    draw(gloss, state == Disabled ? 0.09f : 0.45f);
    
    // Rotate indicator
    float angle = juce::degreesToRadians(-135.0f + value * 270.0f);
    g.addTransform(juce::AffineTransform::rotation(angle, cx, cy));
    draw(indicator);
    g.addTransform(juce::AffineTransform::rotation(-angle, cx, cy));
    
    if (state == Focused) draw(focusRing, 0.8f);
    if (state == Disabled) draw(disabledOverlay, 0.75f);
}
```

## Web Canvas Compositing

```javascript
function drawKnob(ctx, imgs, x, y, size, value, state) {
  const ang = (-135 + value * 270) * Math.PI / 180;
  
  const draw = (img, op = 1, mode = "source-over") => {
    ctx.save();
    ctx.globalAlpha = op;
    ctx.globalCompositeOperation = mode;
    ctx.drawImage(img, x, y, size, size);
    ctx.restore();
  };
  
  if (state === "pressed") draw(imgs.pressedShadow, 0.55, "multiply");
  draw(imgs.shadow, 0.85, "multiply");
  draw(imgs.base);
  draw(imgs.material);
  draw(imgs.ao, 0.65, "multiply");
  draw(imgs.spec, state === "disabled" ? 0.14 : 0.55, "screen");
  draw(imgs.gloss, state === "disabled" ? 0.09 : 0.45, "screen");
  
  // Rotate indicator
  ctx.save();
  ctx.translate(x + size/2, y + size/2);
  ctx.rotate(ang);
  ctx.drawImage(imgs.indicator, -size/2, -size/2, size, size);
  ctx.restore();
  
  if (state === "focus") draw(imgs.focusRing, 0.8, "screen");
  if (state === "disabled") draw(imgs.disabledOverlay, 0.75);
}
```
