#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace Monument
{

/**
 * Demo component displaying a photorealistic stone knob with PBR layer compositing.
 *
 * Loads and composites 11 PBR layers: albedo, AO, roughness, normal,
 * glow_core, glow_crystal, bloom, light_wrap, highlight, indicator, contact_shadow.
 */
class StoneKnobDemo : public juce::Component,
                      private juce::Slider::Listener
{
public:
    enum class KnobType
    {
        Geode,      // Dark crystal with blue interior
        Obsidian,   // Polished black volcanic glass
        Marble,     // Pale marble with veining
        Weathered   // Ancient weathered basalt
    };

    /**
     * Create a demo knob.
     *
     * @param labelText Text displayed below the knob
     * @param type Which knob variant to display
     */
    explicit StoneKnobDemo(const juce::String& labelText,
                          KnobType type = KnobType::Geode);

    ~StoneKnobDemo() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    /**
     * Get current knob value (0.0 to 1.0)
     */
    double getValue() const { return slider.getValue(); }

    /**
     * Set knob value (0.0 to 1.0)
     */
    void setValue(double value, juce::NotificationType notification = juce::sendNotificationAsync)
    {
        slider.setValue(value, notification);
    }

private:
    void sliderValueChanged(juce::Slider* sliderThatChanged) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

    bool loadPBRLayers(KnobType type);
    void compositeLayers(juce::Graphics& g, const juce::Rectangle<float>& bounds);
    juce::Image rotateImage(const juce::Image& source, float angleRadians);

    // Blend mode functions
    void blendImageMultiply(juce::Image& destination, const juce::Image& source, float opacity);
    void blendImageAdditive(juce::Image& destination, const juce::Image& source, float opacity);
    void blendImageScreen(juce::Image& destination, const juce::Image& source, float opacity);

    juce::Slider slider;
    juce::Label label;

    // PBR layer images
    juce::Image albedo;
    juce::Image ao;
    juce::Image roughness;
    juce::Image normal;
    juce::Image glowCore;
    juce::Image glowCrystal;
    juce::Image bloom;
    juce::Image lightWrap;
    juce::Image highlight;
    juce::Image indicator;
    juce::Image contactShadow;

    KnobType knobType;
    bool isHovered = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StoneKnobDemo)
};

} // namespace Monument
