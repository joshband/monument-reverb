#include "MonumentKnob.h"

MonumentKnob::MonumentKnob(juce::AudioProcessorValueTreeState& state,
                           const juce::String& parameterId,
                           const juce::String& labelText)
    : attachment(state, parameterId, slider)
{
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 20);
    addAndMakeVisible(slider);

    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(label);
}

void MonumentKnob::resized()
{
    auto area = getLocalBounds();
    label.setBounds(area.removeFromTop(20));
    slider.setBounds(area.reduced(6));
}
