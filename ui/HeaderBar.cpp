#include "HeaderBar.h"

HeaderBar::HeaderBar(juce::AudioProcessorValueTreeState& apvts)
    : valueTreeState(apvts)
{
    // Title label - MONUMENT logo
    titleLabel.setText("▶ MONUMENT ◀", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(18.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(TEXT_COLOR));
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(titleLabel);

    // Preset selector
    presetSelector.setTextWhenNothingSelected("Select Preset...");
    presetSelector.addItem("Ambient Way", 1);
    presetSelector.addItem("Distant Echo", 2);
    presetSelector.addItem("Grand Hall", 3);
    presetSelector.addItem("Shimmer Space", 4);
    presetSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff242428));
    presetSelector.setColour(juce::ComboBox::textColourId, juce::Colour(TEXT_COLOR));
    presetSelector.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff0d0d0d));
    presetSelector.setColour(juce::ComboBox::arrowColourId, juce::Colour(ACCENT_COLOR));
    addAndMakeVisible(presetSelector);

    // Hall/Wall selector
    hallWallSelector.setTextWhenNothingSelected("Hall 02");
    hallWallSelector.addItem("Hall 01", 1);
    hallWallSelector.addItem("Hall 02", 2);
    hallWallSelector.addItem("Hall 03", 3);
    hallWallSelector.setSelectedId(2);
    hallWallSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff242428));
    hallWallSelector.setColour(juce::ComboBox::textColourId, juce::Colour(TEXT_COLOR));
    hallWallSelector.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff0d0d0d));
    hallWallSelector.setColour(juce::ComboBox::arrowColourId, juce::Colour(ACCENT_COLOR));
    addAndMakeVisible(hallWallSelector);

    // Architecture selector
    architectureSelector.setTextWhenNothingSelected("192x Architecture");
    architectureSelector.addItem("64x Architecture", 1);
    architectureSelector.addItem("128x Architecture", 2);
    architectureSelector.addItem("192x Architecture", 3);
    architectureSelector.addItem("256x Architecture", 4);
    architectureSelector.setSelectedId(3);
    architectureSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff242428));
    architectureSelector.setColour(juce::ComboBox::textColourId, juce::Colour(TEXT_COLOR));
    architectureSelector.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff0d0d0d));
    architectureSelector.setColour(juce::ComboBox::arrowColourId, juce::Colour(ACCENT_COLOR));
    addAndMakeVisible(architectureSelector);
}

HeaderBar::~HeaderBar()
{
}

void HeaderBar::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Dark background
    g.setColour(juce::Colour(BG_COLOR));
    g.fillRect(bounds);

    // Bottom border line
    g.setColour(juce::Colour(0xff0d0d0d));
    g.drawHorizontalLine(bounds.getBottom() - 1, 0.0f, static_cast<float>(bounds.getWidth()));

    // Draw level meters (right side)
    auto meterBounds = bounds.removeFromRight(120).reduced(10, 15).toFloat();

    // Input meter label
    g.setColour(juce::Colour(TEXT_COLOR));
    g.setFont(juce::Font(10.0f));
    g.drawText("IN", meterBounds.removeFromLeft(18), juce::Justification::centredLeft);

    // Input meter
    auto inputMeterBounds = meterBounds.removeFromLeft(25).reduced(0, 2);
    drawLevelMeter(g, inputMeterBounds, inputLevel);

    meterBounds.removeFromLeft(5); // Spacing

    // Output meter label
    g.drawText("OUT", meterBounds.removeFromLeft(20), juce::Justification::centredLeft);

    // Output meter
    auto outputMeterBounds = meterBounds.removeFromLeft(25).reduced(0, 2);
    drawLevelMeter(g, outputMeterBounds, outputLevel);
}

void HeaderBar::resized()
{
    auto bounds = getLocalBounds().reduced(10, 10);

    // Title on far left
    titleLabel.setBounds(bounds.removeFromLeft(150));

    // Preset selector
    bounds.removeFromLeft(10); // Spacing
    presetSelector.setBounds(bounds.removeFromLeft(150).reduced(0, 5));

    // Hall/Wall selector
    bounds.removeFromLeft(10);
    hallWallSelector.setBounds(bounds.removeFromLeft(100).reduced(0, 5));

    // Architecture selector
    bounds.removeFromLeft(10);
    architectureSelector.setBounds(bounds.removeFromLeft(150).reduced(0, 5));

    // Level meters drawn in paint()
}

void HeaderBar::setInputLevel(float level)
{
    inputLevel = level;
    repaint();
}

void HeaderBar::setOutputLevel(float level)
{
    outputLevel = level;
    repaint();
}

void HeaderBar::drawLevelMeter(juce::Graphics& g, juce::Rectangle<float> bounds, float level)
{
    // Background
    g.setColour(juce::Colour(0xff0d0d0d));
    g.fillRoundedRectangle(bounds, 2.0f);

    // Calculate filled height
    float filledHeight = bounds.getHeight() * juce::jlimit(0.0f, 1.0f, level);
    auto filledBounds = bounds.withTop(bounds.getBottom() - filledHeight);

    // Gradient: green → yellow → red
    juce::ColourGradient gradient;
    if (level < 0.7f)
    {
        // Green to yellow
        gradient = juce::ColourGradient(
            juce::Colour(0xff44ff44), filledBounds.getX(), filledBounds.getBottom(),
            juce::Colour(0xffffff44), filledBounds.getX(), filledBounds.getY(),
            false);
    }
    else
    {
        // Yellow to red (clipping warning)
        gradient = juce::ColourGradient(
            juce::Colour(0xffffff44), filledBounds.getX(), filledBounds.getBottom(),
            juce::Colour(0xffff4444), filledBounds.getX(), filledBounds.getY(),
            false);
    }

    g.setGradientFill(gradient);
    g.fillRoundedRectangle(filledBounds, 2.0f);

    // Glow effect
    g.setColour(juce::Colour(0xffffff88).withAlpha(0.4f * level));
    g.fillRoundedRectangle(filledBounds.expanded(1.0f), 3.0f);
}
