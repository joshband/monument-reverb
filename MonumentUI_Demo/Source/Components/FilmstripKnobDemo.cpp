#include "FilmstripKnobDemo.h"
#include <juce_gui_basics/juce_gui_basics.h>

namespace Monument
{

FilmstripKnobDemo::FilmstripKnobDemo(const juce::String& labelText, KnobType type)
    : knobType(type)
{
    // Setup slider (invisible, used for interaction only)
    slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    slider.setRange(0.0, 1.0, 0.001);
    slider.setValue(0.5);
    slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    slider.addListener(this);
    addAndMakeVisible(slider);

    // Setup label
    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(0xffcccccc));
    addAndMakeVisible(label);

    // Load filmstrip
    loadFilmstrip(type);

    setSize(200, 240);
}

FilmstripKnobDemo::~FilmstripKnobDemo()
{
    slider.removeListener(this);
}

void FilmstripKnobDemo::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Draw knob area background (for debugging/contrast)
    auto knobBounds = bounds.removeFromTop(200.0f).reduced(10.0f);
    g.setColour(juce::Colour(0xff1a1a1a));
    g.fillRoundedRectangle(knobBounds, 8.0f);

    // Calculate which frame to display based on slider value
    const int frameIndex = juce::jlimit(0, numFrames - 1,
        static_cast<int>(slider.getValue() * (numFrames - 1)));

    // Calculate source rectangle in filmstrip
    juce::Rectangle<int> sourceRect(0, frameIndex * frameHeight,
                                     filmstrip.getWidth(), frameHeight);

    // Draw the appropriate frame
    if (filmstrip.isValid())
    {
        auto targetBounds = knobBounds.reduced(20.0f);
        g.drawImage(filmstrip,
                    static_cast<int>(targetBounds.getX()),
                    static_cast<int>(targetBounds.getY()),
                    static_cast<int>(targetBounds.getWidth()),
                    static_cast<int>(targetBounds.getHeight()),
                    sourceRect.getX(),
                    sourceRect.getY(),
                    sourceRect.getWidth(),
                    sourceRect.getHeight(),
                    false);
    }

    // Draw hover state indicator
    if (isHovered)
    {
        g.setColour(juce::Colours::white.withAlpha(0.1f));
        g.drawRoundedRectangle(knobBounds, 8.0f, 2.0f);
    }
}

void FilmstripKnobDemo::resized()
{
    auto bounds = getLocalBounds();

    // Knob area (top 200px)
    auto knobArea = bounds.removeFromTop(200);
    slider.setBounds(knobArea);

    // Label area (remaining height)
    bounds.removeFromTop(5); // Small gap
    label.setBounds(bounds);
}

void FilmstripKnobDemo::sliderValueChanged(juce::Slider* /*sliderThatChanged*/)
{
    repaint();
}

void FilmstripKnobDemo::mouseEnter(const juce::MouseEvent& /*event*/)
{
    isHovered = true;
    repaint();
}

void FilmstripKnobDemo::mouseExit(const juce::MouseEvent& /*event*/)
{
    isHovered = false;
    repaint();
}

bool FilmstripKnobDemo::loadFilmstrip(KnobType type)
{
    // Determine filmstrip filename
    juce::String filename;
    switch (type)
    {
        case KnobType::Geode:
            filename = "knob_geode_filmstrip.png";
            break;
        case KnobType::Obsidian:
            filename = "knob_obsidian_filmstrip.png";
            break;
        case KnobType::Marble:
            filename = "knob_marble_filmstrip.png";
            break;
        case KnobType::Weathered:
            filename = "knob_weathered_filmstrip.png";
            break;
        default:
            filename = "knob_geode_filmstrip.png";
    }

    // Construct path relative to executable
    auto assetsDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile)
        .getParentDirectory()
        .getChildFile("Assets")
        .getChildFile("knobs_filmstrip");

    auto filmstripFile = assetsDir.getChildFile(filename);

    // Try absolute path if relative doesn't work
    if (!filmstripFile.existsAsFile())
    {
        filmstripFile = juce::File("/Users/noisebox/Documents/3_Development/Repos/monument-reverb/MonumentUI_Demo/Assets/knobs_filmstrip")
            .getChildFile(filename);
    }

    if (!filmstripFile.existsAsFile())
    {
        DBG("Filmstrip not found: " + filmstripFile.getFullPathName());
        return false;
    }

    // Load filmstrip
    filmstrip = juce::ImageFileFormat::loadFrom(filmstripFile);

    if (!filmstrip.isValid())
    {
        DBG("Failed to load filmstrip: " + filmstripFile.getFullPathName());
        return false;
    }

    // Calculate frame dimensions
    frameHeight = filmstrip.getWidth(); // Assuming square frames
    numFrames = filmstrip.getHeight() / frameHeight;

    DBG("Loaded filmstrip: " + juce::String(numFrames) + " frames of " +
        juce::String(frameHeight) + "x" + juce::String(frameHeight) + "px");

    return true;
}

} // namespace Monument
