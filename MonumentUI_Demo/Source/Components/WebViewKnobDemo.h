#pragma once

#include <juce_gui_extra/juce_gui_extra.h>

namespace Monument
{

/**
 * WebView-based knob demo using CSS blend modes.
 *
 * This demonstrates JUCE 8's WebView capability for building UIs with
 * web technologies. The knob is rendered using HTML/CSS with native
 * CSS mix-blend-mode for layer compositing.
 *
 * Comparison to StoneKnobDemo (CPU blending):
 * - Pros: Hardware-accelerated blend modes, easier to iterate
 * - Cons: Larger binary size, web/C++ bridge complexity
 */
class WebViewKnobDemo : public juce::Component
{
public:
    WebViewKnobDemo();
    ~WebViewKnobDemo() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    /**
     * Get current knob value (0.0 to 1.0)
     */
    double getValue() const { return currentValue; }

    /**
     * Set knob value (0.0 to 1.0)
     */
    void setValue(double value);

private:
    std::unique_ptr<juce::WebBrowserComponent> webView;
    juce::Label label;
    double currentValue = 0.5;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WebViewKnobDemo)
};

} // namespace Monument
