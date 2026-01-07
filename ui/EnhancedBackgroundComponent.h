#pragma once

#include <JuceHeader.h>

/**
 * Enhanced dark stone background with atmospheric blue ethereal effects
 *
 * Features:
 * - Dark weathered stone texture (#0d0d0d - #1a1a1a)
 * - Animated blue fog/wisps (cyan atmospheric glow)
 * - Subtle noise texture for stone surface
 * - Panel dividers with embossed effect
 * - Performance optimized with cached rendering
 */
class EnhancedBackgroundComponent : public juce::Component,
                                     private juce::Timer
{
public:
    EnhancedBackgroundComponent();
    ~EnhancedBackgroundComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Panel divider positions (Y coordinates)
    void setPanelDividers(const std::vector<float>& dividerPositions);

    // Animation control
    void setAnimationEnabled(bool enabled);

private:
    void timerCallback() override;

    // Rendering methods
    void paintStoneTexture(juce::Graphics& g);
    void paintEtherealWisps(juce::Graphics& g);
    void paintPanelDividers(juce::Graphics& g);

    // Generate stone noise texture
    juce::Image generateStoneTexture(int width, int height);

    // Ethereal wisp animation
    struct Wisp
    {
        float x, y;
        float radius;
        float alpha;
        float vx, vy;
        juce::Colour color;
    };

    std::vector<Wisp> wisps;
    void initializeWisps();
    void updateWisps(float deltaTime);

    // Cached textures
    juce::Image stoneTexture;
    bool needsTextureRegen{true};

    // Animation state
    float animationTime{0.0f};
    bool animationEnabled{true};

    // Panel dividers
    std::vector<float> panelDividers;

    // Colors
    static constexpr juce::uint32 DARK_BASE = 0xff0d0d0d;
    static constexpr juce::uint32 DARK_MID = 0xff1a1a1a;
    static constexpr juce::uint32 DARK_HIGHLIGHT = 0xff242428;
    static constexpr juce::uint32 BLUE_WISP = 0xff4488ff;
    static constexpr juce::uint32 CYAN_WISP = 0xff88ccff;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnhancedBackgroundComponent)
};
