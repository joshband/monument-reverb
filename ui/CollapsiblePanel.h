#pragma once

#include <JuceHeader.h>

/**
 * Collapsible panel with smooth expand/collapse animation
 *
 * Features:
 * - Expandable/collapsible with arrow indicator (▶/▼)
 * - Smooth animation (300ms with easing)
 * - Dark theme styling to match Monument aesthetic
 * - Header with title text
 * - Content area that resizes on expand/collapse
 */
class CollapsiblePanel : public juce::Component,
                         private juce::Timer
{
public:
    CollapsiblePanel(const juce::String& title);
    ~CollapsiblePanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;

    // Content management
    void setContentComponent(juce::Component* newContent);
    juce::Component* getContentComponent() const { return contentComponent.get(); }

    // Expand/collapse
    void setExpanded(bool shouldBeExpanded, bool animate = true);
    bool isExpanded() const { return expanded; }

    // Sizing
    void setCollapsedHeight(int height) { collapsedHeight = height; }
    void setExpandedHeight(int height) { expandedHeight = height; }
    int getCollapsedHeight() const { return collapsedHeight; }
    int getExpandedHeight() const { return expandedHeight; }

    // Animation callback
    std::function<void()> onExpandedChanged;

private:
    void timerCallback() override;
    void updateContentBounds();

    juce::String panelTitle;
    std::unique_ptr<juce::Component> contentComponent;

    bool expanded{false};
    bool animating{false};

    // Animation state
    float currentHeight{0.0f};
    float targetHeight{0.0f};
    float animationProgress{0.0f};

    // Sizing
    int collapsedHeight{40}; // Just header visible
    int expandedHeight{300}; // Full content visible
    int headerHeight{40};

    // Styling
    static constexpr juce::uint32 HEADER_BG = 0xff1a1a1a;
    static constexpr juce::uint32 HEADER_HOVER = 0xff242428;
    static constexpr juce::uint32 TEXT_COLOR = 0xffc0c0c0;
    static constexpr juce::uint32 ARROW_COLOR = 0xff888888;

    bool headerHovered{false};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CollapsiblePanel)
};
