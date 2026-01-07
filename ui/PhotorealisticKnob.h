#pragma once

#include <JuceHeader.h>

/**
 * Photorealistic stone knob using filmstrip rendering.
 *
 * Uses pre-rendered 128-frame filmstrips generated from stone knob images.
 * Provides smooth rotation with authentic stone texture and glowing LED center.
 *
 * Design Philosophy:
 * - "Stone & Aether" aesthetic - brutalist architecture meets cosmic dimension
 * - Each knob shows weathered stone with blue ethereal glow center
 * - Filmstrip approach ensures consistent lighting across rotation
 *
 * Technical Details:
 * - 128 frames from -135° to +135° (270° sweep, standard audio knob)
 * - 1024×1024px per frame, horizontal filmstrip (131072×1024 total)
 * - RGBA with alpha channel for transparent background compositing
 * - JUCE filmstrip Slider rendering for GPU-accelerated performance
 * - Animated blue LED center with breathing effect
 */
class PhotorealisticKnob : public juce::Component,
                           private juce::Slider::Listener,
                           private juce::Timer
{
public:
    /**
     * Knob style variants.
     * Each represents a different stone knob design from the generated filmstrips.
     */
    enum class Style
    {
        // Three unique stone knob designs, 4 variations each
        StoneType1_Variant0,  // Irregular stone with blue cosmic center
        StoneType1_Variant1,
        StoneType1_Variant2,
        StoneType1_Variant3,

        StoneType2_Variant0,  // Smooth polished stone with LED glow
        StoneType2_Variant1,
        StoneType2_Variant2,
        StoneType2_Variant3,

        StoneType3_Variant0,  // Rough weathered stone with bright center
        StoneType3_Variant1,
        StoneType3_Variant2,
        StoneType3_Variant3
    };

    /**
     * Create a photorealistic knob bound to a parameter.
     *
     * @param state AudioProcessorValueTreeState for parameter binding
     * @param parameterId Parameter ID to bind to (e.g., "time", "material")
     * @param labelText Label text displayed below knob
     * @param style Which knob filmstrip design to use
     */
    PhotorealisticKnob(juce::AudioProcessorValueTreeState& state,
                       const juce::String& parameterId,
                       const juce::String& labelText,
                       Style style = Style::StoneType1_Variant0);

    ~PhotorealisticKnob() override;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    /**
     * Set modulation state (activates blue glow effect).
     * When modulated, an additional blue overlay is composited for visual feedback.
     */
    void setModulated(bool isModulated);

    /**
     * Enable/disable LED glow animation (default: enabled)
     */
    void setLEDGlowEnabled(bool enabled);

    /**
     * Get the underlying slider for custom styling if needed.
     */
    juce::Slider& getSlider() { return slider; }

private:
    // Slider::Listener
    void sliderValueChanged(juce::Slider* sliderThatChanged) override;

    // Timer callback for LED animation
    void timerCallback() override;

    // Load filmstrip for given style
    juce::Image loadFilmstripForStyle(Style style);

    // Get filmstrip filename for style
    static juce::String getFilmstripFilename(Style style);

    // Draw animated LED glow center
    void drawLEDGlow(juce::Graphics& g);

    // UI components
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;

    // State
    Style currentStyle;
    bool modulated = false;
    bool ledGlowEnabled = true;

    // Animation state
    float ledGlowPhase = 0.0f;
    float hoverGlow = 0.0f;
    bool isMouseOver = false;

    // Filmstrip properties
    static constexpr int FRAME_COUNT = 128;
    static constexpr int FRAME_WIDTH = 1024;
    static constexpr int FRAME_HEIGHT = 1024;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PhotorealisticKnob)
};
