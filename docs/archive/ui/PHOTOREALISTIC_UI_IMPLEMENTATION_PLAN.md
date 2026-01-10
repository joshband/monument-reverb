# Monument Reverb - Photorealistic UI Implementation Plan

**Date:** 2026-01-05
**Architecture:** FlexBox + Layered PBR Rendering + Real-time Animation
**Theme:** Stone + Time + Architecture

---

## Layer Stack Architecture

### Rendering Pipeline (Back to Front)

```
Layer 0: Stone Texture Base (PBR albedo)
    ↓
Layer 1: Normal Map (surface detail, grooves, weathering)
    ↓
Layer 2: Roughness Map (matte stone vs polished metal)
    ↓
Layer 3: Ambient Occlusion (shadows in crevices)
    ↓
Layer 4: Edge/Rim Lighting (highlight raised surfaces)
    ↓
Layer 5: Blueprint Grid Overlay (architectural guides)
    ↓
Layer 6: UI Controls (photorealistic knob sprites)
    ↓
Layer 7: Glow/Emission (active modulation, playhead)
    ↓
Layer 8: Particle Effects (dust motes, echo visualization)
    ↓
Layer 9: Audio Reactivity (level meters, waveform)
```

---

## Component Classes with PBR Materials

### 1. PhotorealisticKnob - Full PBR Implementation

```cpp
/**
 * Knob with 128-frame filmstrip + real-time lighting
 *
 * Assets per knob:
 * - knob_albedo_128frames.png (diffuse color)
 * - knob_normal_128frames.png (surface bumps)
 * - knob_roughness_128frames.png (matte/glossy)
 * - knob_ao_128frames.png (ambient occlusion)
 */
class PhotorealisticKnob : public juce::Slider
{
public:
    PhotorealisticKnob()
    {
        setTextBoxStyle(NoTextBox, false, 0, 0);
        startTimerHz(60); // 60 FPS for smooth animation
    }

    void loadAssets(const KnobAssets& assets)
    {
        albedoFilmStrip = assets.albedo;
        normalFilmStrip = assets.normal;
        roughnessFilmStrip = assets.roughness;
        aoFilmStrip = assets.ao;

        numFrames = 128;
        frameHeight = albedoFilmStrip.getHeight() / numFrames;
    }

    void paint(juce::Graphics& g) override
    {
        // Calculate current frame (with smooth interpolation)
        float value = valueToProportionOfLength(getValue());
        float frameFloat = value * (numFrames - 1);
        int frameIndex = static_cast<int>(frameFloat);
        float frameFrac = frameFloat - frameIndex;

        // Target rotation with easing
        float targetRotation = value * 270.0f - 135.0f; // ±135° range
        currentRotation = juce::jmap(0.15f, currentRotation, targetRotation); // Smooth ease

        auto bounds = getLocalBounds().toFloat();
        auto center = bounds.getCentre();

        // Layer 1: Ambient Occlusion (shadow base)
        drawFrame(g, aoFilmStrip, frameIndex, bounds, 1.0f);

        // Layer 2: Albedo (diffuse color)
        drawFrame(g, albedoFilmStrip, frameIndex, bounds, 1.0f);

        // Layer 3: Normal map (lighting simulation)
        if (enableDynamicLighting)
        {
            drawNormalLitFrame(g, normalFilmStrip, frameIndex, bounds,
                              lightPosition, lightIntensity);
        }

        // Layer 4: Roughness/Specular highlights
        drawFrame(g, roughnessFilmStrip, frameIndex, bounds, specularity);

        // Layer 5: Indicator line (glowing when active)
        drawIndicatorLine(g, center, currentRotation);

        // Layer 6: Glow if modulated
        if (isModulated)
        {
            drawGlowHalo(g, bounds, modulationColor, modulationAmount);
        }
    }

    void timerCallback() override
    {
        // Animate hover glow
        if (isMouseOver())
            hoverGlow = juce::jmin(1.0f, hoverGlow + 0.1f);
        else
            hoverGlow = juce::jmax(0.0f, hoverGlow - 0.1f);

        // Pulse active parameters
        if (isBeingControlled || isModulated)
            pulsePhase += 0.05f;

        repaint();
    }

private:
    void drawFrame(juce::Graphics& g, const juce::Image& filmStrip,
                   int frameIndex, juce::Rectangle<float> bounds, float alpha)
    {
        g.setOpacity(alpha);
        g.drawImage(filmStrip, bounds,
            juce::RectanglePlacement::centred, false,
            {0, frameIndex * frameHeight,
             filmStrip.getWidth(), frameHeight});
    }

    void drawNormalLitFrame(juce::Graphics& g, const juce::Image& normalMap,
                           int frameIndex, juce::Rectangle<float> bounds,
                           juce::Point<float> lightPos, float intensity)
    {
        // Use normal map to calculate per-pixel lighting
        // This creates the illusion of 3D surface with dynamic lighting

        juce::Image normalFrame = normalMap.getClippedImage(
            {0, frameIndex * frameHeight, normalMap.getWidth(), frameHeight});

        // Sample normal vectors and calculate lighting
        auto center = bounds.getCentre();
        juce::Point<float> lightVector = (lightPos - center).normalised();

        // Apply lighting based on normal map (simplified for real-time)
        g.setOpacity(intensity);
        g.drawImage(normalFrame, bounds, juce::RectanglePlacement::centred);
    }

    void drawIndicatorLine(juce::Graphics& g, juce::Point<float> center,
                          float rotation)
    {
        // Glowing indicator line
        float lineLength = getWidth() * 0.35f;
        float lineWidth = 3.0f;

        // Calculate line end point
        float rad = juce::degreesToRadians(rotation);
        juce::Point<float> lineEnd = center +
            juce::Point<float>(std::sin(rad), -std::cos(rad)) * lineLength;

        // Draw glow
        g.setGradientFill(juce::ColourGradient(
            juce::Colour(0xffff8844).withAlpha(0.8f), center,
            juce::Colour(0xffff8844).withAlpha(0.0f), lineEnd,
            true));
        g.drawLine(center.x, center.y, lineEnd.x, lineEnd.y,
                   lineWidth + hoverGlow * 2.0f);

        // Draw solid core
        g.setColour(juce::Colour(0xffff8844));
        g.drawLine(center.x, center.y, lineEnd.x, lineEnd.y, lineWidth);
    }

    void drawGlowHalo(juce::Graphics& g, juce::Rectangle<float> bounds,
                     juce::Colour color, float amount)
    {
        // Pulsing glow around knob when modulated
        float pulse = std::sin(pulsePhase) * 0.3f + 0.7f;
        float glowSize = getWidth() * 0.6f * amount * pulse;

        juce::ColourGradient gradient(
            color.withAlpha(0.6f * amount), bounds.getCentre(),
            color.withAlpha(0.0f), bounds.getCentre(),
            true);
        gradient.addColour(0.5, color.withAlpha(0.3f * amount));

        g.setGradientFill(gradient);
        g.fillEllipse(bounds.expanded(glowSize));
    }

    // Asset images
    juce::Image albedoFilmStrip;
    juce::Image normalFilmStrip;
    juce::Image roughnessFilmStrip;
    juce::Image aoFilmStrip;

    int numFrames{128};
    int frameHeight{0};

    // Animation state
    float currentRotation{0.0f};
    float hoverGlow{0.0f};
    float pulsePhase{0.0f};

    // Lighting
    juce::Point<float> lightPosition{-50.0f, -50.0f}; // Top-left light source
    float lightIntensity{0.8f};
    float specularity{0.4f};
    bool enableDynamicLighting{true};

    // Modulation state
    bool isModulated{false};
    juce::Colour modulationColor{0xffff8844};
    float modulationAmount{0.0f};
};
```

---

## 2. State Indicators System

### Visual Feedback for All States

```cpp
struct StateIndicator
{
    enum class State
    {
        Idle,           // Default grey, subtle glow
        Hover,          // Brighter, highlight edges
        Active,         // Orange glow, pulsing
        Modulated,      // Blue connection lines
        Recording,      // Red pulse
        Frozen,         // Ice blue, crystallized
        Muted           // Desaturated, dim
    };

    static juce::Colour getStateColor(State state)
    {
        switch (state)
        {
            case State::Idle:       return juce::Colour(0xff4a4e54);
            case State::Hover:      return juce::Colour(0xff6a6e74);
            case State::Active:     return juce::Colour(0xffff8844);
            case State::Modulated:  return juce::Colour(0xff4488ff);
            case State::Recording:  return juce::Colour(0xffff4444);
            case State::Frozen:     return juce::Colour(0xff88ccff);
            case State::Muted:      return juce::Colour(0xff2a2e34);
        }
    }

    static void paintStateGlow(juce::Graphics& g,
                               juce::Rectangle<float> bounds,
                               State state, float intensity)
    {
        auto color = getStateColor(state);

        // Outer glow
        juce::ColourGradient gradient(
            color.withAlpha(0.6f * intensity), bounds.getCentre(),
            color.withAlpha(0.0f), bounds.getCentre(),
            true);

        g.setGradientFill(gradient);
        g.fillEllipse(bounds.expanded(20.0f * intensity));

        // Inner core
        g.setColour(color.withAlpha(0.8f * intensity));
        g.fillEllipse(bounds.reduced(2.0f));
    }
};
```

---

## 3. DSP Signal Visualization

### Real-Time Audio Reactivity

```cpp
class DSPVisualizationLayer : public juce::Component,
                              private juce::Timer
{
public:
    DSPVisualizationLayer(MonumentProcessor& p) : processor(p)
    {
        startTimerHz(30); // 30 FPS for smooth visualization
    }

    void paint(juce::Graphics& g) override
    {
        // Layer 1: Input level meter
        paintInputLevelMeter(g);

        // Layer 2: Reverb density visualization (particle field)
        paintDensityField(g);

        // Layer 3: Frequency spectrum analyzer
        paintSpectrumAnalyzer(g);

        // Layer 4: Spatial position indicators
        paintSpatialPositionMarkers(g);

        // Layer 5: Timeline playhead with audio reactive pulse
        paintAudioReactivePlayhead(g);
    }

    void timerCallback() override
    {
        // Update from processor
        inputLevel = processor.getCurrentInputLevel();
        outputLevel = processor.getCurrentOutputLevel();
        densityValue = processor.getCurrentDensity();
        spatialPositions = processor.getSpatialPositions();

        // Get FFT data for spectrum
        spectrumData = processor.getSpectrumData();

        repaint();
    }

private:
    void paintInputLevelMeter(juce::Graphics& g)
    {
        auto bounds = getLevelMeterBounds();

        // Background stone texture
        g.setColour(juce::Colour(0xff2a2e34));
        g.fillRoundedRectangle(bounds, 4.0f);

        // Level bar with gradient
        float levelHeight = bounds.getHeight() * inputLevel;
        auto levelBounds = bounds.removeFromBottom(levelHeight);

        juce::ColourGradient gradient(
            juce::Colour(0xff44ff44), levelBounds.getBottom(),
            juce::Colour(0xffffff44), levelBounds.getY() + levelBounds.getHeight() * 0.7f,
            false);
        gradient.addColour(0.9, juce::Colour(0xffff4444)); // Clip warning

        g.setGradientFill(gradient);
        g.fillRoundedRectangle(levelBounds, 4.0f);

        // Glow effect
        g.setColour(juce::Colour(0xffffff88).withAlpha(0.4f * inputLevel));
        g.fillRoundedRectangle(levelBounds.expanded(2.0f), 6.0f);
    }

    void paintDensityField(juce::Graphics& g)
    {
        // Particle system showing reverb density
        int numParticles = static_cast<int>(densityValue * 200.0f);

        juce::Random random(juce::Time::currentTimeMillis());

        for (int i = 0; i < numParticles; ++i)
        {
            float x = random.nextFloat() * getWidth();
            float y = random.nextFloat() * getHeight();
            float size = random.nextFloat() * 2.0f + 1.0f;
            float alpha = random.nextFloat() * 0.3f + 0.1f;

            // Glowing dust motes
            g.setColour(juce::Colour(0xffffff88).withAlpha(alpha));
            g.fillEllipse(x - size/2, y - size/2, size, size);

            // Glow halo
            g.setColour(juce::Colour(0xffffff44).withAlpha(alpha * 0.3f));
            g.fillEllipse(x - size, y - size, size * 2, size * 2);
        }
    }

    void paintSpectrumAnalyzer(juce::Graphics& g)
    {
        if (spectrumData.empty()) return;

        auto bounds = getSpectrumBounds();
        float barWidth = bounds.getWidth() / spectrumData.size();

        for (size_t i = 0; i < spectrumData.size(); ++i)
        {
            float magnitude = spectrumData[i];
            float barHeight = magnitude * bounds.getHeight();

            juce::Rectangle<float> barBounds(
                bounds.getX() + i * barWidth,
                bounds.getBottom() - barHeight,
                barWidth - 1.0f,
                barHeight);

            // Frequency-dependent color
            juce::Colour barColor = juce::Colour::fromHSV(
                i / static_cast<float>(spectrumData.size()) * 0.6f, // Hue
                0.8f,                                                // Saturation
                magnitude,                                           // Brightness
                1.0f);

            g.setColour(barColor);
            g.fillRect(barBounds);

            // Glow
            g.setColour(barColor.withAlpha(0.4f));
            g.fillRect(barBounds.expanded(1.0f));
        }
    }

    void paintSpatialPositionMarkers(juce::Graphics& g)
    {
        // Visualize 3D spatial positions in 2D top-down view
        auto bounds = getSpatialViewBounds();
        auto center = bounds.getCentre();

        for (const auto& pos : spatialPositions)
        {
            // Map 3D position to 2D screen space
            float screenX = center.x + pos.x * bounds.getWidth() * 0.4f;
            float screenY = center.y + pos.y * bounds.getHeight() * 0.4f;

            // Draw glowing marker
            float markerSize = 8.0f;
            juce::Rectangle<float> marker(
                screenX - markerSize/2,
                screenY - markerSize/2,
                markerSize, markerSize);

            // Pulsing glow based on audio level
            float pulse = std::sin(juce::Time::currentTimeMillis() * 0.003f) * 0.3f + 0.7f;

            g.setColour(juce::Colour(0xff4488ff).withAlpha(0.6f * pulse));
            g.fillEllipse(marker.expanded(4.0f));

            g.setColour(juce::Colour(0xff4488ff));
            g.fillEllipse(marker);
        }
    }

    void paintAudioReactivePlayhead(juce::Graphics& g)
    {
        if (!processor.isTimelinePlaying()) return;

        float playheadX = getPlayheadPosition();
        auto bounds = getTimelineBounds();

        // Pulse width based on current audio level
        float pulseWidth = 2.0f + inputLevel * 8.0f;

        // Draw glowing beam
        juce::ColourGradient gradient(
            juce::Colour(0xff4488ff).withAlpha(0.8f), playheadX, bounds.getY(),
            juce::Colour(0xff4488ff).withAlpha(0.0f), playheadX, bounds.getBottom(),
            false);

        g.setGradientFill(gradient);
        g.fillRect(playheadX - pulseWidth/2, bounds.getY(),
                   pulseWidth, bounds.getHeight());

        // Solid core
        g.setColour(juce::Colour(0xff88ccff));
        g.fillRect(playheadX - 1.0f, bounds.getY(), 2.0f, bounds.getHeight());
    }

    MonumentProcessor& processor;
    float inputLevel{0.0f};
    float outputLevel{0.0f};
    float densityValue{0.0f};
    std::vector<juce::Point<float>> spatialPositions;
    std::vector<float> spectrumData;
};
```

---

## 4. Animation System

### Smooth Transitions and Micro-interactions

```cpp
class AnimationSystem
{
public:
    struct Animation
    {
        enum class Type
        {
            PanelExpand,
            KnobRotation,
            GlowPulse,
            ParticleEmission,
            LevelMeter,
            ConnectionLine
        };

        Type type;
        float duration;       // seconds
        float elapsed{0.0f};
        std::function<float(float)> easingFunction;
        std::function<void(float)> updateCallback;
    };

    void addAnimation(Animation anim)
    {
        activeAnimations.push_back(std::move(anim));
    }

    void update(float deltaTime)
    {
        for (auto it = activeAnimations.begin();
             it != activeAnimations.end();)
        {
            it->elapsed += deltaTime;
            float progress = juce::jmin(1.0f, it->elapsed / it->duration);
            float easedProgress = it->easingFunction(progress);

            it->updateCallback(easedProgress);

            if (progress >= 1.0f)
                it = activeAnimations.erase(it);
            else
                ++it;
        }
    }

    // Easing functions
    static float easeOutCubic(float t)
    {
        return 1.0f - std::pow(1.0f - t, 3.0f);
    }

    static float easeInOutQuad(float t)
    {
        return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
    }

    static float elasticEase(float t)
    {
        const float c4 = (2.0f * juce::MathConstants<float>::pi) / 3.0f;
        return t == 0.0f ? 0.0f :
               t == 1.0f ? 1.0f :
               std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * c4) + 1.0f;
    }

private:
    std::vector<Animation> activeAnimations;
};
```

---

## Implementation Checklist

### Week 1: Core Infrastructure
- [ ] Create `PhotorealisticKnob` base class with filmstrip rendering
- [ ] Implement PBR layer stack (albedo, normal, roughness, AO)
- [ ] Add `StateIndicator` system for visual feedback
- [ ] Build `AnimationSystem` with easing functions

### Week 2: Asset Pipeline
- [ ] Generate knob renders in Blender (128 frames each)
- [ ] Create PBR maps (normal, roughness, AO) for each knob
- [ ] Export stone texture tiles (granite, limestone, basalt, marble)
- [ ] Create glow/emission overlays for active states

### Week 3: DSP Visualization
- [ ] Implement `DSPVisualizationLayer` component
- [ ] Add input/output level meters with audio reactivity
- [ ] Create particle system for density visualization
- [ ] Build spectrum analyzer with FFT data feed
- [ ] Add spatial position markers (top-down view)

### Week 4: Polish & Integration
- [ ] Integrate all layers into FlexBox layout system
- [ ] Add hover/click animations for all controls
- [ ] Implement modulation glow connections
- [ ] Test performance (60 FPS target)
- [ ] Optimize rendering (offscreen buffers, caching)

---

## Performance Targets

| Metric | Target | Critical |
|--------|--------|----------|
| Frame rate | 60 FPS | 30 FPS minimum |
| CPU usage (idle) | <2% | <5% |
| CPU usage (playing) | <8% | <15% |
| Memory footprint | <50 MB | <100 MB |
| Asset load time | <200ms | <500ms |

---

## File Structure

```
ui/
├── components/
│   ├── PhotorealisticKnob.h/cpp
│   ├── StateIndicator.h/cpp
│   ├── DSPVisualizationLayer.h/cpp
│   └── AnimationSystem.h/cpp
├── assets/
│   ├── knobs/
│   │   ├── material_knob_albedo_128.png
│   │   ├── material_knob_normal_128.png
│   │   ├── material_knob_roughness_128.png
│   │   └── material_knob_ao_128.png
│   └── textures/
│       ├── granite_tile_1k.png
│       ├── limestone_tile_1k.png
│       └── basalt_tile_1k.png
└── shaders/ (optional GLSL for advanced effects)
    ├── pbr_lighting.frag
    └── particle_system.frag
```
