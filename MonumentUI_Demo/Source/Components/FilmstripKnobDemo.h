#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace Monument
{

/**
 * Zero-cost filmstrip knob with perfect PBR layer compositing.
 *
 * Uses pre-rendered filmstrip (64 frames) with all blend modes applied offline.
 * Runtime cost: single image blit per frame (no CPU compositing).
 *
 * This solves the alpha masking and blend mode issues by doing all compositing
 * in Python with PIL/Pillow, which has proper blend mode implementations.
 */
class FilmstripKnobDemo : public juce::Component,
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
     * Create a filmstrip knob demo.
     *
     * @param labelText Text displayed below the knob
     * @param type Which knob variant to display
     */
    explicit FilmstripKnobDemo(const juce::String& labelText,
                               KnobType type = KnobType::Geode);

    ~FilmstripKnobDemo() override;

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

    bool loadFilmstrip(KnobType type);

    juce::Slider slider;
    juce::Label label;

    juce::Image filmstrip;
    int numFrames = 64;
    int frameHeight = 512;
    KnobType knobType;
    bool isHovered = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilmstripKnobDemo)
};

} // namespace Monument
