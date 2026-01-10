// Monument Reverb - Photorealistic Knob Component
// Filmstrip-based knob rendering with LED ring visualization

#pragma once

#include <JuceHeader.h>

namespace monument {

/**
 * Photorealistic knob using pre-rendered filmstrip
 *
 * Renders knobs from vertical filmstrip images (PNG with multiple frames)
 * Supports parameter binding, LED ring overlay, and smooth animation
 */
class PhotorealisticKnob : public juce::Slider
{
public:
    PhotorealisticKnob()
    {
        setSliderStyle(juce::Slider::RotaryVerticalDrag);
        setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        setRotaryParameters(juce::MathConstants<float>::pi * 1.2f,
                           juce::MathConstants<float>::pi * 2.8f,
                           true);
    }

    /** Load filmstrip asset
     *
     * @param filmstripImage Vertical strip of knob frames
     * @param numFrames Number of frames in the strip (typically 128)
     */
    void loadFilmstrip(const juce::Image& filmstripImage, int numFrames)
    {
        filmstrip = filmstripImage;
        this->numFrames = numFrames;
        frameHeight = filmstrip.getHeight() / numFrames;
    }

    /** Enable/disable LED ring overlay */
    void setLEDRingEnabled(bool enabled)
    {
        ledRingEnabled = enabled;
        repaint();
    }

    /** Set LED ring color */
    void setLEDRingColor(juce::Colour color)
    {
        ledColor = color;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        if (!filmstrip.isValid())
        {
            // Fallback rendering if no filmstrip loaded
            paintFallback(g);
            return;
        }

        // Calculate frame index from value (0 to numFrames-1)
        const float normalizedValue = static_cast<float>(valueToProportionOfLength(getValue()));
        const int frameIndex = juce::jlimit(0, numFrames - 1,
                                           static_cast<int>(normalizedValue * (numFrames - 1)));

        // Draw filmstrip frame
        const juce::Rectangle<int> sourceRect(0, frameIndex * frameHeight,
                                              filmstrip.getWidth(), frameHeight);

        g.drawImage(filmstrip,
                   getLocalBounds().toFloat(),
                   sourceRect.toFloat(),
                   false);  // Don't use high quality (faster rendering)

        // Draw LED ring overlay if enabled
        if (ledRingEnabled)
        {
            paintLEDRing(g, normalizedValue);
        }

        // Draw label if set
        if (label.isNotEmpty())
        {
            paintLabel(g);
        }
    }

    /** Set parameter label displayed below knob */
    void setLabel(const juce::String& labelText)
    {
        label = labelText;
        repaint();
    }

private:
    void paintFallback(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat().reduced(10.0f);

        // Draw circular knob background
        g.setColour(juce::Colours::darkgrey);
        g.fillEllipse(bounds);

        // Draw knob outline
        g.setColour(juce::Colours::lightgrey);
        g.drawEllipse(bounds, 2.0f);

        // Draw indicator line
        const float angle = static_cast<float>(getRotaryParameters().startAngleRadians +
            (getRotaryParameters().endAngleRadians - getRotaryParameters().startAngleRadians) *
            valueToProportionOfLength(getValue()));

        const float centerX = bounds.getCentreX();
        const float centerY = bounds.getCentreY();
        const float radius = bounds.getWidth() * 0.4f;

        juce::Path indicator;
        indicator.addLineSegment(
            juce::Line<float>(centerX, centerY,
                             centerX + radius * std::sin(angle),
                             centerY - radius * std::cos(angle)),
            3.0f);

        g.setColour(juce::Colours::white);
        g.fillPath(indicator);
    }

    void paintLEDRing(juce::Graphics& g, float normalizedValue)
    {
        auto bounds = getLocalBounds().toFloat().reduced(5.0f);
        const float centerX = bounds.getCentreX();
        const float centerY = bounds.getCentreY();
        const float radius = bounds.getWidth() * 0.5f + 10.0f;  // Outside knob

        // Draw LED arc from start to current position
        const auto& rotaryParams = getRotaryParameters();
        const float startAngle = rotaryParams.startAngleRadians;
        const float endAngle = startAngle + normalizedValue *
            (rotaryParams.endAngleRadians - rotaryParams.startAngleRadians);

        // Create arc path
        juce::Path arc;
        arc.addCentredArc(centerX, centerY, radius, radius,
                         0.0f,  // rotation
                         startAngle,
                         endAngle,
                         true);

        // Draw LED ring with glow effect
        g.setColour(ledColor.withAlpha(0.3f));
        g.strokePath(arc, juce::PathStrokeType(8.0f));

        g.setColour(ledColor);
        g.strokePath(arc, juce::PathStrokeType(3.0f));

        // Draw LED dots along the arc (for retro LED look)
        const int numDots = 32;
        const int litDots = static_cast<int>(normalizedValue * numDots);

        for (int i = 0; i < litDots; ++i)
        {
            const float dotAngle = startAngle + (endAngle - startAngle) * i / numDots;
            const float dotX = centerX + radius * std::sin(dotAngle);
            const float dotY = centerY - radius * std::cos(dotAngle);

            g.setColour(ledColor);
            g.fillEllipse(dotX - 2.0f, dotY - 2.0f, 4.0f, 4.0f);

            // Glow effect
            g.setColour(ledColor.withAlpha(0.4f));
            g.fillEllipse(dotX - 4.0f, dotY - 4.0f, 8.0f, 8.0f);
        }
    }

    void paintLabel(juce::Graphics& g)
    {
        const int labelHeight = 20;
        auto labelBounds = getLocalBounds().removeFromBottom(labelHeight);

        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.setFont(juce::Font(14.0f, juce::Font::bold));
        g.drawText(label, labelBounds, juce::Justification::centred);
    }

    juce::Image filmstrip;
    int numFrames{128};
    int frameHeight{0};

    bool ledRingEnabled{true};
    juce::Colour ledColor{juce::Colours::cyan};
    juce::String label;
};

/**
 * Knob group component - displays multiple related knobs
 *
 * Example: Time, Mass, Density, Gravity in a row
 */
class KnobGroup : public juce::Component
{
public:
    KnobGroup(const juce::String& groupTitle) : title(groupTitle)
    {
    }

    void addKnob(PhotorealisticKnob* knob, const juce::String& label)
    {
        knob->setLabel(label);
        knobs.add(knob);
        addAndMakeVisible(knob);
        resized();
    }

    void paint(juce::Graphics& g) override
    {
        // Draw group background
        g.setColour(juce::Colour(0x22ffffff));
        g.fillRoundedRectangle(getLocalBounds().toFloat(), 10.0f);

        // Draw title
        g.setColour(juce::Colours::white);
        g.setFont(juce::Font(18.0f, juce::Font::bold));
        g.drawText(title, getLocalBounds().removeFromTop(30),
                  juce::Justification::centred);
    }

    void resized() override
    {
        auto bounds = getLocalBounds().reduced(10);
        bounds.removeFromTop(40);  // Space for title

        const int knobSize = 100;
        const int spacing = 20;
        const int numKnobs = knobs.size();

        if (numKnobs == 0)
            return;

        const int totalWidth = numKnobs * knobSize + (numKnobs - 1) * spacing;
        auto knobArea = bounds.withSizeKeepingCentre(totalWidth, knobSize);

        for (auto* knob : knobs)
        {
            knob->setBounds(knobArea.removeFromLeft(knobSize));
            knobArea.removeFromLeft(spacing);
        }
    }

private:
    juce::String title;
    juce::OwnedArray<PhotorealisticKnob> knobs;
};

/**
 * Parameter display with value readout
 *
 * Shows numeric value and units below knob
 */
class ParameterDisplay : public juce::Component,
                         private juce::Slider::Listener
{
public:
    ParameterDisplay(PhotorealisticKnob& knob, const juce::String& units)
        : attachedKnob(knob), unitString(units)
    {
        addAndMakeVisible(knob);
        knob.addListener(this);

        addAndMakeVisible(valueLabel);
        valueLabel.setJustificationType(juce::Justification::centred);
        valueLabel.setColour(juce::Label::textColourId, juce::Colours::white);

        updateDisplay();
    }

    void resized() override
    {
        auto bounds = getLocalBounds();
        attachedKnob.setBounds(bounds.removeFromTop(bounds.getHeight() - 30));
        valueLabel.setBounds(bounds);
    }

private:
    void sliderValueChanged(juce::Slider*) override
    {
        updateDisplay();
    }

    void updateDisplay()
    {
        const double value = attachedKnob.getValue();
        const juce::String text = juce::String(value, 2) + " " + unitString;
        valueLabel.setText(text, juce::dontSendNotification);
    }

    PhotorealisticKnob& attachedKnob;
    juce::String unitString;
    juce::Label valueLabel;
};

} // namespace monument
