#include "MonumentBodyComponent.h"

namespace Monument
{

MonumentBodyComponent::MonumentBodyComponent()
    : shadow(juce::Colours::black.withAlpha(0.6f), 30, {0, 8})
{
}

void MonumentBodyComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Fill outside the mask with pure black
    g.fillAll(juce::Colours::black);

    // Draw shadow first (outside the mask)
    shadow.drawForPath(g, bodyMask);

    // Clip all subsequent drawing to the body mask
    g.reduceClipRegion(bodyMask);

    // Body fill - dark stone color
    // In full implementation, this could be a textured background
    g.setColour(juce::Colour(0xff0f0f10));
    g.fillPath(bodyMask);

    // Optional: Subtle vignette for depth
    juce::ColourGradient vignette(
        juce::Colours::transparentBlack, bounds.getCentre(),
        juce::Colours::black.withAlpha(0.3f), bounds.getTopLeft(),
        true);

    g.setGradientFill(vignette);
    g.fillPath(bodyMask);
}

void MonumentBodyComponent::resized()
{
    rebuildMask();
}

void MonumentBodyComponent::rebuildMask()
{
    auto bounds = getLocalBounds().toFloat();
    auto w = bounds.getWidth();
    auto h = bounds.getHeight();

    // Create asymmetric sculptural silhouette
    // This creates a non-rectangular shape that feels like carved stone
    bodyMask.clear();

    // Top-left corner (irregular)
    bodyMask.startNewSubPath(20.f, 10.f);

    // Top edge (curved upward in the middle)
    bodyMask.quadraticTo(w * 0.3f, -10.f,
                          w * 0.6f, -5.f);
    bodyMask.quadraticTo(w * 0.8f, 0.f,
                          w - 20.f, 30.f);

    // Right edge (slight outward bow)
    bodyMask.quadraticTo(w + 5.f, h * 0.3f,
                          w - 10.f, h * 0.6f);
    bodyMask.lineTo(w - 10.f, h - 40.f);

    // Bottom-right corner (more rounded)
    bodyMask.quadraticTo(w * 0.8f, h + 10.f,
                          w * 0.6f, h - 10.f);

    // Bottom edge (subtle dip)
    bodyMask.quadraticTo(w * 0.4f, h - 5.f,
                          30.f, h - 20.f);

    // Left edge (relatively straight)
    bodyMask.lineTo(10.f, h * 0.3f);
    bodyMask.quadraticTo(5.f, h * 0.15f,
                          20.f, 10.f);

    bodyMask.closeSubPath();
}

bool MonumentBodyComponent::hitTest(int x, int y)
{
    // Only respond to mouse events inside the sculptural shape
    return bodyMask.contains((float)x, (float)y);
}

} // namespace Monument
