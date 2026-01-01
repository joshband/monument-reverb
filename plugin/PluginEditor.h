#pragma once

#include <JuceHeader.h>

#include "PluginProcessor.h"

class MonumentAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MonumentAudioProcessorEditor(MonumentAudioProcessor&);
    ~MonumentAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    MonumentAudioProcessor& processor;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    juce::Slider mixSlider;
    juce::Slider timeSlider;
    juce::Slider massSlider;

    juce::Label mixLabel;
    juce::Label timeLabel;
    juce::Label massLabel;

    Attachment mixAttachment;
    Attachment timeAttachment;
    Attachment massAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MonumentAudioProcessorEditor)
};
