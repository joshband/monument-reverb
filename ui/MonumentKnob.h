#pragma once

#include <JuceHeader.h>

class MonumentKnob : public juce::Component
{
public:
    MonumentKnob(juce::AudioProcessorValueTreeState& state,
                 const juce::String& parameterId,
                 const juce::String& labelText);

    void resized() override;

private:
    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    juce::Slider slider;
    juce::Label label;
    Attachment attachment;
};
