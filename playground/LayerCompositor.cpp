// PHASE 3: Layer compositor implementation with straight-alpha blending
#include "LayerCompositor.h"
#include <algorithm>

namespace monument::playground {

LayerCompositor::LayerCompositor()
{
}

// === Layer Management ===

bool LayerCompositor::loadImage(const juce::File& file,
                                  const juce::String& name,
                                  BlendMode mode,
                                  float opacity)
{
    // Attempt to load the image
    juce::Image image = juce::ImageFileFormat::loadFrom(file);

    if (!image.isValid())
    {
        DBG("LayerCompositor: Failed to load image from " + file.getFullPathName());
        return false;
    }

    // Ensure image has alpha channel for proper compositing
    if (image.getFormat() != juce::Image::ARGB)
    {
        juce::Image rgbaImage(juce::Image::ARGB, image.getWidth(), image.getHeight(), true);
        juce::Graphics g(rgbaImage);
        g.drawImageAt(image, 0, 0);
        image = rgbaImage;
    }

    // Use filename as name if not provided
    juce::String layerName = name.isEmpty() ? file.getFileName() : name;

    addLayer(image, layerName, mode, opacity);

    DBG("LayerCompositor: Loaded layer '" + layerName + "' (" +
        juce::String(image.getWidth()) + "x" + juce::String(image.getHeight()) + ", " +
        "mode=" + juce::String((int)mode) + ", opacity=" + juce::String(opacity, 2) + ")");

    return true;
}

void LayerCompositor::addLayer(const juce::Image& image,
                                const juce::String& name,
                                BlendMode mode,
                                float opacity)
{
    // Clamp opacity to valid range
    float clampedOpacity = juce::jlimit(0.0f, 1.0f, opacity);

    layers.emplace_back(image, name, mode, clampedOpacity);

    // Invalidate cache
    cachedComposite = juce::Image();
}

void LayerCompositor::clear()
{
    layers.clear();
    cachedComposite = juce::Image();
}

// === Compositing ===

juce::Image LayerCompositor::composite()
{
    if (layers.empty())
    {
        DBG("LayerCompositor: No layers to composite");
        return juce::Image();
    }

    // Determine output dimensions from first layer
    const auto& baseLayer = layers[0];
    int width = baseLayer.image.getWidth();
    int height = baseLayer.image.getHeight();

    // Create output image with alpha channel (straight-alpha format)
    juce::Image result(juce::Image::ARGB, width, height, true);

    // Start with transparent background
    result.clear(result.getBounds(), juce::Colours::transparentBlack);

    // Composite each layer in order (bottom to top)
    for (const auto& layer : layers)
    {
        if (!layer.image.isValid())
            continue;

        // Process each pixel
        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                // Get source pixel from layer (may need to sample if different size)
                juce::Colour topPixel;
                if (x < layer.image.getWidth() && y < layer.image.getHeight())
                {
                    topPixel = layer.image.getPixelAt(x, y);
                }
                else
                {
                    topPixel = juce::Colours::transparentBlack; // Out of bounds
                }

                // Get destination pixel
                juce::Colour bottomPixel = result.getPixelAt(x, y);

                // Blend pixels using specified mode
                juce::Colour blended = blendPixels(bottomPixel, topPixel, layer.blendMode, layer.opacity);

                // Write back to result
                result.setPixelAt(x, y, blended);
            }
        }
    }

    // Apply debug overlay if enabled
    if (debugMode)
    {
        // Add a subtle red tint to the border to indicate debug mode
        juce::Graphics g(result);
        g.setColour(juce::Colours::red.withAlpha(0.5f));
        g.drawRect(0, 0, width, height, 2);

        // Add debug text in corner
        g.setColour(juce::Colours::red);
        g.setFont(12.0f);
        g.drawText("DEBUG MODE", 5, 5, 100, 20, juce::Justification::topLeft);
    }

    // Cache result
    cachedComposite = result;

    return result;
}

// === Debug Visualization ===

juce::Image LayerCompositor::getAlphaVisualization() const
{
    if (!cachedComposite.isValid())
    {
        DBG("LayerCompositor: No composite to visualize");
        return juce::Image();
    }

    int width = cachedComposite.getWidth();
    int height = cachedComposite.getHeight();

    // Create grayscale image showing alpha values
    juce::Image alphaVis(juce::Image::RGB, width, height, true);

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            juce::Colour pixel = cachedComposite.getPixelAt(x, y);
            uint8 alpha = pixel.getAlpha();

            // Convert alpha to grayscale (white = opaque, black = transparent)
            juce::Colour gray = juce::Colour(alpha, alpha, alpha);
            alphaVis.setPixelAt(x, y, gray);
        }
    }

    return alphaVis;
}

// === Blending Implementation ===

juce::Colour LayerCompositor::blendPixels(const juce::Colour& bottom,
                                           const juce::Colour& top,
                                           BlendMode mode,
                                           float opacity) const
{
    switch (mode)
    {
        case BlendMode::Normal:
            return normalBlend(bottom, top, opacity);
        case BlendMode::Multiply:
            return multiplyBlend(bottom, top, opacity);
        case BlendMode::Screen:
            return screenBlend(bottom, top, opacity);
        case BlendMode::Additive:
            return additiveBlend(bottom, top, opacity);
        default:
            return normalBlend(bottom, top, opacity);
    }
}

juce::Colour LayerCompositor::normalBlend(const juce::Colour& bottom,
                                           const juce::Colour& top,
                                           float opacity) const
{
    // Standard "over" operator using straight-alpha blending
    // Formula: C = (Ct * αt * opacity) + (Cb * αb * (1 - αt * opacity))
    //          α = αt * opacity + αb * (1 - αt * opacity)
    //
    // This preserves proper alpha without premultiplication artifacts

    float topAlpha = top.getFloatAlpha() * opacity;
    float bottomAlpha = bottom.getFloatAlpha();

    // Early out for fully transparent top layer
    if (topAlpha < 0.001f)
        return bottom;

    // Calculate result alpha
    float resultAlpha = topAlpha + bottomAlpha * (1.0f - topAlpha);

    // Early out for fully transparent result
    if (resultAlpha < 0.001f)
        return juce::Colours::transparentBlack;

    // Blend RGB channels using straight-alpha formula
    float topWeight = topAlpha / resultAlpha;
    float bottomWeight = (bottomAlpha * (1.0f - topAlpha)) / resultAlpha;

    float r = top.getFloatRed() * topWeight + bottom.getFloatRed() * bottomWeight;
    float g = top.getFloatGreen() * topWeight + bottom.getFloatGreen() * bottomWeight;
    float b = top.getFloatBlue() * topWeight + bottom.getFloatBlue() * bottomWeight;

    return juce::Colour::fromFloatRGBA(r, g, b, resultAlpha);
}

juce::Colour LayerCompositor::multiplyBlend(const juce::Colour& bottom,
                                             const juce::Colour& top,
                                             float opacity) const
{
    // Multiply blend: Darkens the image (useful for shadows, AO)
    // Formula: C = Cb * Ct (component-wise)
    // Alpha blending still uses normal "over" operator

    float topAlpha = top.getFloatAlpha() * opacity;

    if (topAlpha < 0.001f)
        return bottom;

    // Multiply RGB
    float r = bottom.getFloatRed() * top.getFloatRed();
    float g = bottom.getFloatGreen() * top.getFloatGreen();
    float b = bottom.getFloatBlue() * top.getFloatBlue();

    // Create multiplied color
    juce::Colour multiplied = juce::Colour::fromFloatRGBA(r, g, b, topAlpha);

    // Blend multiplied result over bottom using normal blend
    return normalBlend(bottom, multiplied, 1.0f);
}

juce::Colour LayerCompositor::screenBlend(const juce::Colour& bottom,
                                           const juce::Colour& top,
                                           float opacity) const
{
    // Screen blend: Lightens the image (useful for highlights, bloom)
    // Formula: C = 1 - (1 - Cb) * (1 - Ct)
    // Equivalent to inverting both colors, multiplying, and inverting result

    float topAlpha = top.getFloatAlpha() * opacity;

    if (topAlpha < 0.001f)
        return bottom;

    // Screen blend RGB
    float r = 1.0f - (1.0f - bottom.getFloatRed()) * (1.0f - top.getFloatRed());
    float g = 1.0f - (1.0f - bottom.getFloatGreen()) * (1.0f - top.getFloatGreen());
    float b = 1.0f - (1.0f - bottom.getFloatBlue()) * (1.0f - top.getFloatBlue());

    // Create screened color
    juce::Colour screened = juce::Colour::fromFloatRGBA(r, g, b, topAlpha);

    // Blend screened result over bottom using normal blend
    return normalBlend(bottom, screened, 1.0f);
}

juce::Colour LayerCompositor::additiveBlend(const juce::Colour& bottom,
                                             const juce::Colour& top,
                                             float opacity) const
{
    // Additive blend: Adds light (useful for glow, emission)
    // Formula: C = Cb + (Ct * opacity)
    // Clamped to [0, 1] to prevent overflow

    float topAlpha = top.getFloatAlpha() * opacity;

    if (topAlpha < 0.001f)
        return bottom;

    // Add RGB, weighted by top alpha
    float r = juce::jlimit(0.0f, 1.0f, bottom.getFloatRed() + top.getFloatRed() * topAlpha);
    float g = juce::jlimit(0.0f, 1.0f, bottom.getFloatGreen() + top.getFloatGreen() * topAlpha);
    float b = juce::jlimit(0.0f, 1.0f, bottom.getFloatBlue() + top.getFloatBlue() * topAlpha);

    // Alpha uses normal "over" operator
    float resultAlpha = topAlpha + bottom.getFloatAlpha() * (1.0f - topAlpha);

    return juce::Colour::fromFloatRGBA(r, g, b, resultAlpha);
}

} // namespace monument::playground
