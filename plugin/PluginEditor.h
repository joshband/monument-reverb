#pragma once

#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "ui/MonumentKnob.h"
#include "ui/MonumentToggle.h"
#include "ui/MonumentTimeKnob.h"

class MonumentAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MonumentAudioProcessorEditor(MonumentAudioProcessor&);
    ~MonumentAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    MonumentAudioProcessor& processorRef;

    // Macro Controls (Primary Interface)
    MonumentKnob materialKnob;
    MonumentKnob topologyKnob;
    MonumentKnob viscosityKnob;
    MonumentKnob evolutionKnob;
    MonumentKnob chaosKnob;
    MonumentKnob elasticityKnob;

    // Base Parameters
    MonumentKnob mixKnob;
    MonumentTimeKnob timeKnob;  // Layered knob with industrial aesthetic
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
    juce::TextButton savePresetButton;

    // User preset management
    void refreshPresetList();
    void scanUserPresets();
    void showSavePresetDialog();
    std::vector<juce::File> userPresetFiles;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MonumentAudioProcessorEditor)
};
