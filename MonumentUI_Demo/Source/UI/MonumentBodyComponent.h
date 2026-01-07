#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>

namespace Monument
{

/**
 * MonumentBodyComponent - The main sculptural UI container
 *
 * Creates an asymmetric, non-rectangular visual presence while maintaining
 * a rectangular window for host compatibility.
 *
 * Key features:
 * - Path-based masking for asymmetric silhouette
 * - Drop shadow and ambient occlusion
 * - All child components clip to the mask
 * - Hit-testing respects the sculptural shape
 */
class MonumentBodyComponent : public juce::Component
{
public:
    MonumentBodyComponent();
    ~MonumentBodyComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    /**
     * Hit-testing respects the asymmetric mask
     * Returns true only if point is inside the sculptural shape
     */
    bool hitTest(int x, int y) override;

    /**
     * Get the body mask path (useful for debugging or advanced rendering)
     */
    const juce::Path& getBodyMask() const { return bodyMask; }

private:
    void rebuildMask();

    juce::Path bodyMask;
    juce::DropShadow shadow;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MonumentBodyComponent)
};

} // namespace Monument
