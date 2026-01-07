#include "StoneKnobDemo.h"
#include "BinaryData.h"

namespace Monument
{

StoneKnobDemo::StoneKnobDemo(const juce::String& labelText, KnobType type)
    : knobType(type)
{
    // Configure slider as invisible rotary knob (we'll draw manually)
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters(
        -135.0f * juce::MathConstants<float>::pi / 180.0f,  // -135° start
        +135.0f * juce::MathConstants<float>::pi / 180.0f,  // +135° end
        true  // Stop at end
    );
    slider.setRange(0.0, 1.0, 0.01);
    slider.setValue(0.5);  // Default to center position

    // Make slider transparent (we'll paint over it)
    slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour(juce::Slider::thumbColourId, juce::Colours::transparentBlack);

    slider.addListener(this);
    addAndMakeVisible(slider);

    // Configure label
    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colours::white);
    label.setFont(juce::FontOptions(14.0f));
    addAndMakeVisible(label);

    // Load PBR layers
    if (!loadPBRLayers(type))
    {
        DBG("Warning: Failed to load PBR layers for knob");
    }
}

StoneKnobDemo::~StoneKnobDemo()
{
    slider.removeListener(this);
}

void StoneKnobDemo::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Dark background panel for knob area
    auto knobBounds = bounds.removeFromTop(bounds.getHeight() - 20);
    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillRoundedRectangle(knobBounds, 8.0f);

    // Draw hover indicator
    if (isHovered)
    {
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.drawRoundedRectangle(knobBounds, 8.0f, 2.0f);
    }

    // Composite all PBR layers
    compositeLayers(g, knobBounds);
}

void StoneKnobDemo::resized()
{
    auto bounds = getLocalBounds();

    // Reserve space for label at bottom
    auto labelHeight = 20;
    auto labelBounds = bounds.removeFromBottom(labelHeight);
    label.setBounds(labelBounds);

    // Slider takes up remaining space (invisible, just for interaction)
    slider.setBounds(bounds);
}

void StoneKnobDemo::sliderValueChanged(juce::Slider* sliderThatChanged)
{
    if (sliderThatChanged == &slider)
    {
        repaint();  // Redraw to update indicator rotation
    }
}

void StoneKnobDemo::mouseEnter(const juce::MouseEvent&)
{
    isHovered = true;
    repaint();
}

void StoneKnobDemo::mouseExit(const juce::MouseEvent&)
{
    isHovered = false;
    repaint();
}

bool StoneKnobDemo::loadPBRLayers(KnobType type)
{
    // JUCE BinaryData strips directory structure and adds numbers for duplicate filenames
    // The assets are named: layername_png, layername_png2, layername_png3, layername_png4
    // for Geode, Obsidian, Marble, Weathered respectively

    juce::String typeSuffix;
    switch (type)
    {
        case KnobType::Geode:    typeSuffix = ""; break;       // first variant has no suffix
        case KnobType::Obsidian: typeSuffix = "2"; break;      // _png2
        case KnobType::Marble:   typeSuffix = "3"; break;      // _png3
        case KnobType::Weathered: typeSuffix = "4"; break;     // _png4
    }

    // Load each layer from BinaryData
    auto loadLayer = [&](const juce::String& layerName) -> juce::Image
    {
        juce::String resourceName = layerName + "_png" + typeSuffix;

        int dataSize = 0;
        const char* data = BinaryData::getNamedResource(resourceName.toRawUTF8(), dataSize);

        if (data != nullptr && dataSize > 0)
        {
            auto image = juce::ImageFileFormat::loadFrom(data, (size_t)dataSize);
            if (image.isValid())
            {
                DBG("Loaded layer: " << resourceName);
                return image;
            }
        }

        DBG("Failed to load layer: " << resourceName);
        return juce::Image();
    };

    albedo = loadLayer("albedo");
    ao = loadLayer("ao");
    roughness = loadLayer("roughness");
    normal = loadLayer("normal");
    glowCore = loadLayer("glow_core");
    glowCrystal = loadLayer("glow_crystal");
    bloom = loadLayer("bloom");
    lightWrap = loadLayer("light_wrap");
    highlight = loadLayer("highlight");
    indicator = loadLayer("indicator");
    contactShadow = loadLayer("contact_shadow");

    return albedo.isValid();  // At minimum, albedo must be present
}

void StoneKnobDemo::compositeLayers(juce::Graphics& g, const juce::Rectangle<float>& bounds)
{
    if (!albedo.isValid())
    {
        // Fallback: draw error indicator
        g.setColour(juce::Colours::darkgrey);
        g.fillEllipse(bounds);
        g.setColour(juce::Colours::red);
        g.drawEllipse(bounds, 2.0f);
        return;
    }

    // Create compositing buffer (RGBA with transparency)
    int size = 512;  // Render at 512x512 for performance
    juce::Image composite(juce::Image::ARGB, size, size, true);  // true = clear to transparent

    juce::Graphics gc(composite);
    gc.setImageResamplingQuality(juce::Graphics::highResamplingQuality);

    auto compositeBounds = composite.getBounds().toFloat();

    // Helper to draw with alpha (drawImageAt preserves alpha better than drawImage)
    auto drawLayerWithAlpha = [&](const juce::Image& layer, float opacity) {
        if (!layer.isValid()) return;

        // Scale and center the layer
        float scale = compositeBounds.getWidth() / layer.getWidth();
        int scaledW = (int)(layer.getWidth() * scale);
        int scaledH = (int)(layer.getHeight() * scale);
        int x = (int)((compositeBounds.getWidth() - scaledW) / 2);
        int y = (int)((compositeBounds.getHeight() - scaledH) / 2);

        gc.setOpacity(opacity);
        gc.drawImage(layer, x, y, scaledW, scaledH,
                     0, 0, layer.getWidth(), layer.getHeight(), false);
    };

    // Layer compositing order (bottom to top):

    // 1. Contact shadow (beneath knob) - with alpha
    drawLayerWithAlpha(contactShadow, 0.6f);

    // 2. Base albedo (color) - full opacity with alpha channel
    drawLayerWithAlpha(albedo, 1.0f);

    // 3. Ambient occlusion (multiply blend - darkens crevices)
    if (ao.isValid())
    {
        blendImageMultiply(composite, ao, 0.5f);
    }

    // 4. Glow core (additive - center LED)
    if (glowCore.isValid())
    {
        blendImageAdditive(composite, glowCore, 0.7f);
    }

    // 5. Glow crystal (additive - crystal shine)
    if (glowCrystal.isValid())
    {
        blendImageAdditive(composite, glowCrystal, 0.6f);
    }

    // 6. Highlight (screen blend - specular reflections)
    if (highlight.isValid())
    {
        float highlightOpacity = isHovered ? 0.6f : 0.4f;
        blendImageScreen(composite, highlight, highlightOpacity);
    }

    // 7. Indicator (rotation pointer) - rotate based on slider value
    if (indicator.isValid())
    {
        // Calculate rotation angle (-135° to +135°)
        float rotation = juce::jmap(
            (float)slider.getValue(),
            0.0f, 1.0f,
            -135.0f, +135.0f
        ) * juce::MathConstants<float>::pi / 180.0f;

        // Rotate indicator and draw with alpha
        auto rotatedIndicator = rotateImage(indicator, rotation);
        drawLayerWithAlpha(rotatedIndicator, 0.9f);
    }

    // 8. Apply circular alpha mask for clean edges
    {
        juce::Image::BitmapData compData(composite, juce::Image::BitmapData::readWrite);

        float centerX = compData.width / 2.0f;
        float centerY = compData.height / 2.0f;
        float radius = compData.width * 0.48f;  // Slightly smaller than image
        float feather = 4.0f;  // Soft edge

        for (int y = 0; y < compData.height; ++y)
        {
            for (int x = 0; x < compData.width; ++x)
            {
                float dx = x - centerX;
                float dy = y - centerY;
                float dist = std::sqrt(dx * dx + dy * dy);

                // Calculate circular mask with soft edge
                float maskAlpha = 1.0f;
                if (dist > radius - feather)
                {
                    if (dist > radius)
                        maskAlpha = 0.0f;
                    else
                        maskAlpha = (radius - dist) / feather;
                }

                // Apply mask to pixel alpha
                auto pixel = compData.getPixelColour(x, y);
                float newAlpha = pixel.getFloatAlpha() * maskAlpha;

                compData.setPixelColour(x, y, juce::Colour::fromFloatRGBA(
                    pixel.getFloatRed(),
                    pixel.getFloatGreen(),
                    pixel.getFloatBlue(),
                    newAlpha
                ));
            }
        }
    }

    // Draw final composite to screen with full alpha preservation
    g.setImageResamplingQuality(juce::Graphics::highResamplingQuality);
    g.drawImage(composite, bounds, juce::RectanglePlacement::centred);
}

juce::Image StoneKnobDemo::rotateImage(const juce::Image& source, float angleRadians)
{
    if (!source.isValid())
        return juce::Image();

    // Create output image with same format and size
    juce::Image rotated(source.getFormat(), source.getWidth(), source.getHeight(), true);

    juce::Graphics g(rotated);

    // Translate to center, rotate, translate back
    auto transform = juce::AffineTransform::rotation(
        angleRadians,
        source.getWidth() / 2.0f,
        source.getHeight() / 2.0f
    );

    g.drawImageTransformed(source, transform, false);

    return rotated;
}

void StoneKnobDemo::blendImageMultiply(juce::Image& destination, const juce::Image& source, float opacity)
{
    if (!destination.isValid() || !source.isValid())
        return;

    // Get pixel data from both images
    juce::Image::BitmapData destData(destination, juce::Image::BitmapData::readWrite);
    juce::Image::BitmapData srcData(source, juce::Image::BitmapData::readOnly);

    int width = juce::jmin(destData.width, srcData.width);
    int height = juce::jmin(destData.height, srcData.height);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            auto destPixel = destData.getPixelColour(x, y);
            auto srcPixel = srcData.getPixelColour(x, y);

            // Extract alpha channels
            float srcAlpha = srcPixel.getFloatAlpha() * opacity;
            float destAlpha = destPixel.getFloatAlpha();

            // Multiply blend: result = dest * src
            float r = destPixel.getFloatRed() * srcPixel.getFloatRed();
            float g = destPixel.getFloatGreen() * srcPixel.getFloatGreen();
            float b = destPixel.getFloatBlue() * srcPixel.getFloatBlue();

            // Mix blended result with original based on source alpha
            r = destPixel.getFloatRed() * (1.0f - srcAlpha) + r * srcAlpha;
            g = destPixel.getFloatGreen() * (1.0f - srcAlpha) + g * srcAlpha;
            b = destPixel.getFloatBlue() * (1.0f - srcAlpha) + b * srcAlpha;

            // Alpha compositing: result alpha = src + dst * (1 - src)
            float resultAlpha = srcAlpha + destAlpha * (1.0f - srcAlpha);

            destData.setPixelColour(x, y, juce::Colour::fromFloatRGBA(r, g, b, resultAlpha));
        }
    }
}

void StoneKnobDemo::blendImageAdditive(juce::Image& destination, const juce::Image& source, float opacity)
{
    if (!destination.isValid() || !source.isValid())
        return;

    juce::Image::BitmapData destData(destination, juce::Image::BitmapData::readWrite);
    juce::Image::BitmapData srcData(source, juce::Image::BitmapData::readOnly);

    int width = juce::jmin(destData.width, srcData.width);
    int height = juce::jmin(destData.height, srcData.height);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            auto destPixel = destData.getPixelColour(x, y);
            auto srcPixel = srcData.getPixelColour(x, y);

            // Extract alpha channels
            float srcAlpha = srcPixel.getFloatAlpha() * opacity;
            float destAlpha = destPixel.getFloatAlpha();

            // Additive blend: result = dest + src (clamped)
            float r = juce::jmin(1.0f, destPixel.getFloatRed() + srcPixel.getFloatRed() * srcAlpha);
            float g = juce::jmin(1.0f, destPixel.getFloatGreen() + srcPixel.getFloatGreen() * srcAlpha);
            float b = juce::jmin(1.0f, destPixel.getFloatBlue() + srcPixel.getFloatBlue() * srcAlpha);

            // Alpha compositing: result alpha = src + dst * (1 - src)
            float resultAlpha = srcAlpha + destAlpha * (1.0f - srcAlpha);

            destData.setPixelColour(x, y, juce::Colour::fromFloatRGBA(r, g, b, resultAlpha));
        }
    }
}

void StoneKnobDemo::blendImageScreen(juce::Image& destination, const juce::Image& source, float opacity)
{
    if (!destination.isValid() || !source.isValid())
        return;

    juce::Image::BitmapData destData(destination, juce::Image::BitmapData::readWrite);
    juce::Image::BitmapData srcData(source, juce::Image::BitmapData::readOnly);

    int width = juce::jmin(destData.width, srcData.width);
    int height = juce::jmin(destData.height, srcData.height);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            auto destPixel = destData.getPixelColour(x, y);
            auto srcPixel = srcData.getPixelColour(x, y);

            // Extract alpha channels
            float srcAlpha = srcPixel.getFloatAlpha() * opacity;
            float destAlpha = destPixel.getFloatAlpha();

            // Screen blend: result = 1 - (1 - dest) * (1 - src)
            float r = 1.0f - (1.0f - destPixel.getFloatRed()) * (1.0f - srcPixel.getFloatRed());
            float g = 1.0f - (1.0f - destPixel.getFloatGreen()) * (1.0f - srcPixel.getFloatGreen());
            float b = 1.0f - (1.0f - destPixel.getFloatBlue()) * (1.0f - srcPixel.getFloatBlue());

            // Mix blended result with original based on source alpha
            r = destPixel.getFloatRed() * (1.0f - srcAlpha) + r * srcAlpha;
            g = destPixel.getFloatGreen() * (1.0f - srcAlpha) + g * srcAlpha;
            b = destPixel.getFloatBlue() * (1.0f - srcAlpha) + b * srcAlpha;

            // Alpha compositing: result alpha = src + dst * (1 - src)
            float resultAlpha = srcAlpha + destAlpha * (1.0f - srcAlpha);

            destData.setPixelColour(x, y, juce::Colour::fromFloatRGBA(r, g, b, resultAlpha));
        }
    }
}

} // namespace Monument
