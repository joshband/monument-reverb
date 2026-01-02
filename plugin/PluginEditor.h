#pragma once

#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "ui/MonumentKnob.h"
#include "ui/MonumentToggle.h"

class MonumentAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MonumentAudioProcessorEditor(MonumentAudioProcessor&);
    ~MonumentAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    MonumentAudioProcessor& processorRef;

    MonumentKnob mixKnob;
    MonumentKnob timeKnob;
    MonumentKnob massKnob;
    MonumentKnob densityKnob;
    MonumentKnob bloomKnob;
    MonumentKnob airKnob;
    MonumentKnob widthKnob;
    MonumentKnob warpKnob;
    MonumentKnob driftKnob;
    MonumentKnob gravityKnob;
    MonumentToggle freezeToggle;
    juce::ComboBox presetBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MonumentAudioProcessorEditor)
};
