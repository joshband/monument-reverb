# Photorealistic UI Design

Advanced techniques for professional plugin interfaces with layered sprites, materials, lighting, and animation.

## Asset Pipeline

```
Design Tool (Figma/Blender/Substance)
    ↓
Export layered assets (PNG with alpha)
    ↓
TexturePacker → Sprite atlas (JSON + PNG)
    ↓
JUCE BinaryData or runtime loading
    ↓
Layered compositing in paint()
```

## Layer Architecture

```cpp
enum class LayerType {
    Background,    // Static background
    Shadow,        // Drop/contact shadows
    Reflection,    // Surface reflections
    Diffuse,       // Base color/texture
    Specular,      // Highlights
    Emission,      // Glow, LEDs
    Overlay        // Scratches, dust
};

struct SpriteLayer {
    juce::Image image;
    LayerType type;
    float opacity{1.0f};
    juce::AffineTransform transform;
};
```

## Film Strip Knob (Most Common)

```cpp
class FilmStripKnob : public juce::Slider {
public:
    void loadFilmStrip(const juce::Image& strip, int numFrames) {
        filmStrip = strip;
        this->numFrames = numFrames;
        frameHeight = strip.getHeight() / numFrames;
        setTextBoxStyle(NoTextBox, false, 0, 0);
    }
    
    void paint(juce::Graphics& g) override {
        int frame = (int)(valueToProportionOfLength(getValue()) * (numFrames - 1));
        
        g.drawImage(filmStrip, getLocalBounds().toFloat(),
            juce::RectanglePlacement::centred, false,
            {0, frame * frameHeight, filmStrip.getWidth(), frameHeight});
    }
    
private:
    juce::Image filmStrip;
    int numFrames{128}, frameHeight{0};
};
```

## Dynamic Specular Highlights

```cpp
juce::Image generateSpecular(int size, float angle, float glossiness) {
    juce::Image img(juce::Image::ARGB, size, size, true);
    juce::Graphics g(img);
    
    float cx = size / 2.0f, cy = size / 2.0f;
    float radius = size * 0.4f;
    
    // Highlight position based on knob angle
    float hx = cx + std::cos(angle + pi) * radius * 0.3f;
    float hy = cy + std::sin(angle + pi) * radius * 0.3f;
    float specSize = radius * (1.0f - glossiness) * 0.5f;
    
    juce::ColourGradient grad(
        juce::Colours::white.withAlpha(0.8f * glossiness), hx, hy,
        juce::Colours::transparentWhite, hx + specSize, hy + specSize, true);
    
    g.setGradientFill(grad);
    g.fillEllipse(hx - specSize, hy - specSize, specSize * 2, specSize * 2);
    
    return img;
}
```

## Shadow Generation

```cpp
// Use melatonin_blur library (recommended)
#include <melatonin_blur/melatonin_blur.h>

class KnobWithShadow : public juce::Component {
    void paint(juce::Graphics& g) override {
        // Drop shadow
        melatonin::DropShadow shadow{{
            juce::Colours::black.withAlpha(0.5f),
            10,      // blur radius
            {4, 4}   // offset
        }};
        shadow.render(g, knobPath);
        
        // Draw knob
        g.drawImage(knobImage, getLocalBounds().toFloat());
    }
};
```

## Glow Effects (LEDs, VU Meters)

```cpp
void drawLEDGlow(juce::Graphics& g, juce::Point<float> pos, 
                 juce::Colour color, float intensity) {
    // Multi-layer for realism
    for (int i = 3; i >= 0; --i) {
        float radius = 5.0f * (1.0f + i * 0.5f);
        float alpha = intensity * (1.0f / (i + 1));
        
        juce::ColourGradient grad(
            color.withAlpha(alpha), pos.x, pos.y,
            color.withAlpha(0.0f), pos.x + radius, pos.y, true);
        
        g.setGradientFill(grad);
        g.fillEllipse(pos.x - radius, pos.y - radius, radius * 2, radius * 2);
    }
    
    // Bright core
    g.setColour(color.brighter().withAlpha(intensity));
    g.fillEllipse(pos.x - 2, pos.y - 2, 4, 4);
}
```

## Texture Atlas Loading

```cpp
class TextureAtlas {
public:
    bool load(const juce::String& jsonPath, const juce::String& imagePath) {
        atlas = juce::ImageFileFormat::loadFrom(juce::File(imagePath));
        
        auto json = juce::JSON::parse(juce::File(jsonPath));
        if (auto* frames = json["frames"].getDynamicObject()) {
            for (const auto& prop : frames->getProperties()) {
                auto& f = prop.value["frame"];
                sprites[prop.name.toString()] = {
                    f["x"], f["y"], f["w"], f["h"]
                };
            }
        }
        return atlas.isValid();
    }
    
    juce::Image getSprite(const juce::String& name) {
        if (sprites.count(name))
            return atlas.getClippedImage(sprites[name]);
        return {};
    }
    
private:
    juce::Image atlas;
    std::map<juce::String, juce::Rectangle<int>> sprites;
};
```

## Animation (Sprite Sheet)

```cpp
class AnimatedSprite : public juce::Component, private juce::Timer {
public:
    void load(const juce::Image& sheet, int cols, int rows, float fps) {
        spriteSheet = sheet;
        frameW = sheet.getWidth() / cols;
        frameH = sheet.getHeight() / rows;
        totalFrames = cols * rows;
        this->cols = cols;
        startTimerHz((int)fps);
    }
    
    void paint(juce::Graphics& g) override {
        int col = currentFrame % cols;
        int row = currentFrame / cols;
        
        g.drawImage(spriteSheet, getLocalBounds().toFloat(),
            juce::RectanglePlacement::centred, false,
            {col * frameW, row * frameH, frameW, frameH});
    }
    
private:
    void timerCallback() override {
        currentFrame = (currentFrame + 1) % totalFrames;
        repaint();
    }
    
    juce::Image spriteSheet;
    int frameW{0}, frameH{0}, cols{1}, totalFrames{1}, currentFrame{0};
};
```

## OpenGL Acceleration (Optional)

```cpp
class OpenGLUI : public juce::Component, private juce::OpenGLRenderer {
public:
    OpenGLUI() {
        context.setRenderer(this);
        context.attachTo(*this);
    }
    
    void newOpenGLContextCreated() override {
        // Load shaders, textures
    }
    
    void renderOpenGL() override {
        juce::OpenGLHelpers::clear(juce::Colours::black);
        // GPU-accelerated rendering
    }
    
private:
    juce::OpenGLContext context;
};
```

## Recommended Libraries

```cmake
# melatonin_blur - Fast blur/shadows
FetchContent_Declare(melatonin_blur
    GIT_REPOSITORY https://github.com/sudara/melatonin_blur.git)
FetchContent_MakeAvailable(melatonin_blur)
target_link_libraries(MyPlugin PRIVATE melatonin_blur)

# gin - Extended JUCE utilities
FetchContent_Declare(gin
    GIT_REPOSITORY https://github.com/FigBug/Gin.git)
```

## Asset Creation Workflow

1. **Blender** - Model 3D knob with materials/lighting
2. **Render** - 128 frames at rotation angles
3. **TexturePacker** - Combine into vertical strip
4. **JUCE BinaryData** - Embed in plugin
5. **FilmStripKnob** - Display correct frame

## Performance Tips

- Cache rendered images when possible
- Use `setBufferedToImage(true)` for static components
- Minimize `repaint()` calls
- Profile with JUCE performance overlay
