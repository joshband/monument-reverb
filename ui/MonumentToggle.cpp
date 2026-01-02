#include "MonumentToggle.h"

MonumentToggle::MonumentToggle(juce::AudioProcessorValueTreeState& state,
                               const juce::String& parameterId,
                               const juce::String& labelText)
    : attachment(state, parameterId, toggle)
{
    toggle.setButtonText({});
    toggle.setColour(juce::ToggleButton::tickColourId, juce::Colour(0xffe6e1d6));
    toggle.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colour(0xff5a5a5a));
    addAndMakeVisible(toggle);

    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);
}

void MonumentToggle::resized()
{
    auto area = getLocalBounds();
    label.setBounds(area.removeFromTop(20));
    toggle.setBounds(area.reduced(6));
}
