#include "StoneBackgroundComponent.h"

StoneBackgroundComponent::StoneBackgroundComponent()
{
}

void StoneBackgroundComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Layer 1: Base stone gradient (subtle top-to-bottom lighting)
    juce::ColourGradient baseGradient(
        baseStone.brighter(0.05f), 0.0f, 0.0f,
        baseStone.darker(0.03f), 0.0f, static_cast<float>(bounds.getHeight()),
        false);

    g.setGradientFill(baseGradient);
    g.fillRect(bounds);

    // Layer 2: Subtle stone texture (noise pattern)
    paintStoneTexture(g, bounds);

    // Layer 3: Panel dividers (embossed lines)
    for (int yPos : dividerPositions)
    {
        paintPanelDivider(g, yPos);
    }
}

void StoneBackgroundComponent::resized()
{
}

void StoneBackgroundComponent::setPanelDividers(const std::vector<int>& yPositions)
{
    dividerPositions = yPositions;
    repaint();
}

void StoneBackgroundComponent::paintStoneTexture(juce::Graphics& g, juce::Rectangle<int> area)
{
    // Create subtle noise texture for stone appearance
    juce::Random random(42); // Fixed seed for consistent texture

    g.setOpacity(0.03f); // Very subtle

    for (int i = 0; i < area.getWidth() * area.getHeight() / 100; ++i)
    {
        int x = area.getX() + random.nextInt(area.getWidth());
        int y = area.getY() + random.nextInt(area.getHeight());

        // Random brightness variation
        float brightness = random.nextFloat() * 0.5f + 0.5f;

        g.setColour(juce::Colours::white.withAlpha(brightness));
        g.fillRect(x, y, 1, 1);
    }

    g.setOpacity(1.0f);
}

void StoneBackgroundComponent::paintPanelDivider(juce::Graphics& g, int yPosition)
{
    auto width = getWidth();

    // Embossed effect (dark top line, light bottom line)

    // Dark shadow line (top)
    g.setColour(dividerDark);
    g.drawLine(0.0f, static_cast<float>(yPosition),
               static_cast<float>(width), static_cast<float>(yPosition),
               1.0f);

    // Light highlight line (bottom)
    g.setColour(dividerLight);
    g.drawLine(0.0f, static_cast<float>(yPosition + 1),
               static_cast<float>(width), static_cast<float>(yPosition + 1),
               1.0f);
}
