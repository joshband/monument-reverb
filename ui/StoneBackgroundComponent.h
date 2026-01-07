#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
 * Dark stone background with subtle texture and panel dividers
 *
 * Theme: Deep grey stone (#1a1a1a to #2d2d2d)
 * Features:
 * - Subtle noise texture for stone appearance
 * - Panel dividers with embossed effect
 * - Gradient lighting for depth
 */
class StoneBackgroundComponent : public juce::Component
{
public:
    StoneBackgroundComponent();
    ~StoneBackgroundComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Panel divider positions (Y coordinates)
    void setPanelDividers(const std::vector<int>& yPositions);

private:
    void paintStoneTexture(juce::Graphics& g, juce::Rectangle<int> area);
    void paintPanelDivider(juce::Graphics& g, int yPosition);

    // Color palette
    juce::Colour baseStone{0xff1a1a1a};      // Dark grey base
    juce::Colour midStone{0xff242428};       // Mid grey
    juce::Colour lightStone{0xff2d2d2d};     // Lighter grey
    juce::Colour dividerDark{0xff0d0d0d};    // Dark edge
    juce::Colour dividerLight{0xff3a3a3a};   // Light edge

    std::vector<int> dividerPositions;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StoneBackgroundComponent)
};
