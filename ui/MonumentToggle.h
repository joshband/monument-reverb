#pragma once

#include <JuceHeader.h>

class MonumentToggle : public juce::Component
{
public:
    MonumentToggle(juce::AudioProcessorValueTreeState& state,
                   const juce::String& parameterId,
                   const juce::String& labelText);

    void resized() override;

private:
    using Attachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    juce::ToggleButton toggle;
    juce::Label label;
    Attachment attachment;
};
