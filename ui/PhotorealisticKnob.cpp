#include "PhotorealisticKnob.h"
#include "BinaryData.h"

/**
 * Custom LookAndFeel for filmstrip rendering.
 */
class FilmstripLookAndFeel : public juce::LookAndFeel_V4
{
public:
    FilmstripLookAndFeel(const juce::Image& filmstripImage, int numFrames)
        : filmstrip(filmstripImage), frameCount(numFrames)
    {
        frameWidth = filmstrip.getWidth() / numFrames;
        frameHeight = filmstrip.getHeight();
    }

    void drawRotarySlider(juce::Graphics& g,
                         int x, int y, int width, int height,
                         float sliderPosProportional,
                         float /*rotaryStartAngle*/,
                         float /*rotaryEndAngle*/,
                         juce::Slider& /*slider*/) override
    {
        // Calculate which frame to display (0 to frameCount-1)
        int frameIndex = juce::roundToInt(sliderPosProportional * (frameCount - 1));
        frameIndex = juce::jlimit(0, frameCount - 1, frameIndex);

        // Calculate source rectangle for this frame
        int srcX = frameIndex * frameWidth;

        // Draw the specific frame from the filmstrip, scaled to fit
        g.drawImage(filmstrip,
                   x, y, width, height,  // destination
                   srcX, 0, frameWidth, frameHeight,  // source
                   false);  // don't fill alpha channel
    }

private:
    juce::Image filmstrip;
    int frameCount;
    int frameWidth;
    int frameHeight;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(FilmstripLookAndFeel)
};


PhotorealisticKnob::PhotorealisticKnob(juce::AudioProcessorValueTreeState& state,
                                       const juce::String& parameterId,
                                       const juce::String& labelText,
                                       Style style)
    : currentStyle(style)
{
    // Configure slider as rotary knob
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.setRotaryParameters(
        -135.0f * juce::MathConstants<float>::pi / 180.0f,  // -135° start
        +135.0f * juce::MathConstants<float>::pi / 180.0f,  // +135° end
        true  // Stop at end
    );

    // Load filmstrip and set custom look and feel
    juce::Image filmstrip = loadFilmstripForStyle(style);

    if (filmstrip.isValid())
    {
        auto* laf = new FilmstripLookAndFeel(filmstrip, FRAME_COUNT);
        slider.setLookAndFeel(laf);
    }
    else
    {
        DBG("Warning: Failed to load filmstrip for PhotorealisticKnob");
    }

    slider.addListener(this);
    addAndMakeVisible(slider);

    // Configure label
    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(0xffb8b8b8));
    label.setFont(juce::FontOptions(12.0f));
    addAndMakeVisible(label);

    // Create parameter attachment
    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        state, parameterId, slider);

    // Start LED glow animation timer (30 FPS)
    startTimerHz(30);
}

PhotorealisticKnob::~PhotorealisticKnob()
{
    stopTimer();
    slider.removeListener(this);

    // Clean up custom LookAndFeel
    auto* laf = &slider.getLookAndFeel();
    slider.setLookAndFeel(nullptr);
    delete laf;
}

void PhotorealisticKnob::paint(juce::Graphics& g)
{
    // Draw animated LED glow center
    if (ledGlowEnabled)
    {
        drawLEDGlow(g);
    }

    // Optional: Draw modulation glow overlay (outer halo)
    if (modulated)
    {
        // Draw blue ethereal glow effect
        juce::Colour glowColor = juce::Colour(0x8000BFFF);  // Semi-transparent deep sky blue

        auto knobBounds = slider.getBounds().toFloat();

        // Draw expanding circles for glow effect
        for (int i = 0; i < 3; ++i)
        {
            float expansion = (i + 1) * 10.0f;
            auto glowBounds = knobBounds.expanded(expansion);

            float alpha = 0.3f - (i * 0.1f);
            g.setColour(glowColor.withAlpha(alpha));
            g.fillEllipse(glowBounds);
        }
    }
}

void PhotorealisticKnob::resized()
{
    auto bounds = getLocalBounds();

    // Label at bottom (20px height)
    auto labelBounds = bounds.removeFromBottom(20);
    label.setBounds(labelBounds);

    // Slider uses remaining space
    slider.setBounds(bounds);
}

void PhotorealisticKnob::setModulated(bool isModulated)
{
    if (modulated != isModulated)
    {
        modulated = isModulated;
        repaint();
    }
}

void PhotorealisticKnob::setLEDGlowEnabled(bool enabled)
{
    ledGlowEnabled = enabled;
    repaint();
}

void PhotorealisticKnob::sliderValueChanged(juce::Slider* /*sliderThatChanged*/)
{
    // Optional: Handle value changes (e.g., for parameter automation feedback)
}

void PhotorealisticKnob::timerCallback()
{
    // Update LED glow animation phase
    ledGlowPhase += 0.02f; // Slow breathing effect
    if (ledGlowPhase > juce::MathConstants<float>::twoPi)
        ledGlowPhase -= juce::MathConstants<float>::twoPi;

    // Update hover glow
    if (slider.isMouseOver())
        hoverGlow = juce::jmin(1.0f, hoverGlow + 0.1f);
    else
        hoverGlow = juce::jmax(0.0f, hoverGlow - 0.05f);

    repaint();
}

void PhotorealisticKnob::drawLEDGlow(juce::Graphics& g)
{
    auto knobBounds = slider.getBounds().toFloat();
    auto center = knobBounds.getCentre();

    // Calculate breathing pulse (slow sine wave)
    float pulse = std::sin(ledGlowPhase) * 0.2f + 0.8f; // 0.6 to 1.0 range

    // Add hover intensity
    float totalIntensity = pulse + (hoverGlow * 0.3f);

    // LED center radius (smaller than knob, centered)
    float ledRadius = knobBounds.getWidth() * 0.15f; // 15% of knob size

    // Colors: bright cyan/blue LED
    juce::Colour ledCore = juce::Colour(0xff88ccff);    // Bright cyan
    juce::Colour ledGlow = juce::Colour(0xff4488ff);    // Deep blue

    // Outer glow (large, soft)
    juce::Point<float> centerPoint(center.x, center.y);
    juce::Point<float> outerEdge(center.x + ledRadius * 3.0f, center.y);

    juce::ColourGradient outerGlow(
        ledGlow.withAlpha(0.4f * totalIntensity), centerPoint,
        ledGlow.withAlpha(0.0f), outerEdge,
        true);

    g.setGradientFill(outerGlow);
    g.fillEllipse(
        center.x - ledRadius * 3.0f,
        center.y - ledRadius * 3.0f,
        ledRadius * 6.0f,
        ledRadius * 6.0f);

    // Middle glow (medium, brighter)
    juce::Point<float> middleEdge(center.x + ledRadius * 1.5f, center.y);

    juce::ColourGradient middleGlow(
        ledCore.withAlpha(0.6f * totalIntensity), centerPoint,
        ledGlow.withAlpha(0.0f), middleEdge,
        true);

    g.setGradientFill(middleGlow);
    g.fillEllipse(
        center.x - ledRadius * 1.5f,
        center.y - ledRadius * 1.5f,
        ledRadius * 3.0f,
        ledRadius * 3.0f);

    // Inner bright core
    juce::Point<float> coreEdge(center.x + ledRadius, center.y);

    juce::ColourGradient coreGlow(
        ledCore.withAlpha(0.9f * totalIntensity), centerPoint,
        ledCore.withAlpha(0.4f * totalIntensity), coreEdge,
        true);

    g.setGradientFill(coreGlow);
    g.fillEllipse(
        center.x - ledRadius,
        center.y - ledRadius,
        ledRadius * 2.0f,
        ledRadius * 2.0f);

    // Bright center dot (intense LED point)
    g.setColour(juce::Colour(0xffffffff).withAlpha(0.8f * totalIntensity));
    g.fillEllipse(
        center.x - ledRadius * 0.3f,
        center.y - ledRadius * 0.3f,
        ledRadius * 0.6f,
        ledRadius * 0.6f);
}

juce::Image PhotorealisticKnob::loadFilmstripForStyle(Style style)
{
    juce::String filename = getFilmstripFilename(style);
    juce::File filmstripFile = juce::File::getCurrentWorkingDirectory()
        .getChildFile("assets/ui/knobs_photorealistic")
        .getChildFile(filename);

    if (filmstripFile.existsAsFile())
    {
        return juce::ImageFileFormat::loadFrom(filmstripFile);
    }

    DBG("Warning: Filmstrip file not found: " + filmstripFile.getFullPathName());
    return juce::Image();
}

juce::String PhotorealisticKnob::getFilmstripFilename(Style style)
{
    // Map style enum to actual filmstrip filenames
    switch (style)
    {
        case Style::StoneType1_Variant0:
            return "stone_rotary_knob_glowing_led_center_decorative_isolated_on_p_315a7246-4392-4c23-8a9f-cbe7ad29dda5_0_filmstrip.png";
        case Style::StoneType1_Variant1:
            return "stone_rotary_knob_glowing_led_center_decorative_isolated_on_p_315a7246-4392-4c23-8a9f-cbe7ad29dda5_1_filmstrip.png";
        case Style::StoneType1_Variant2:
            return "stone_rotary_knob_glowing_led_center_decorative_isolated_on_p_315a7246-4392-4c23-8a9f-cbe7ad29dda5_2_filmstrip.png";
        case Style::StoneType1_Variant3:
            return "stone_rotary_knob_glowing_led_center_decorative_isolated_on_p_315a7246-4392-4c23-8a9f-cbe7ad29dda5_3_filmstrip.png";

        case Style::StoneType2_Variant0:
            return "stone_rotary_knob_glowing_led_center_decorative_isolated_on_p_855f2c8b-55f8-48c8-9a4e-be48d5e15d06_0_filmstrip.png";
        case Style::StoneType2_Variant1:
            return "stone_rotary_knob_glowing_led_center_decorative_isolated_on_p_855f2c8b-55f8-48c8-9a4e-be48d5e15d06_1_filmstrip.png";
        case Style::StoneType2_Variant2:
            return "stone_rotary_knob_glowing_led_center_decorative_isolated_on_p_855f2c8b-55f8-48c8-9a4e-be48d5e15d06_2_filmstrip.png";
        case Style::StoneType2_Variant3:
            return "stone_rotary_knob_glowing_led_center_decorative_isolated_on_p_855f2c8b-55f8-48c8-9a4e-be48d5e15d06_3_filmstrip.png";

        case Style::StoneType3_Variant0:
            return "stone_rotary_knob_glowing_led_center_decorative_isolated_on_p_df2ebe2a-8859-404c-a04e-877e14465667_0_filmstrip.png";
        case Style::StoneType3_Variant1:
            return "stone_rotary_knob_glowing_led_center_decorative_isolated_on_p_df2ebe2a-8859-404c-a04e-877e14465667_1_filmstrip.png";
        case Style::StoneType3_Variant2:
            return "stone_rotary_knob_glowing_led_center_decorative_isolated_on_p_df2ebe2a-8859-404c-a04e-877e14465667_2_filmstrip.png";
        case Style::StoneType3_Variant3:
            return "stone_rotary_knob_glowing_led_center_decorative_isolated_on_p_df2ebe2a-8859-404c-a04e-877e14465667_3_filmstrip.png";

        default:
            return "";
    }
}
