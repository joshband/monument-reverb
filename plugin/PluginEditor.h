#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

/**
 * Simple JUCE-based editor for Monument Reverb
 * Uses basic Slider components for clean, functional UI
 */
class MonumentAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MonumentAudioProcessorEditor(MonumentAudioProcessor&);
    ~MonumentAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    MonumentAudioProcessor& processorRef;

    // Header label
    juce::Label titleLabel;

    // Macro parameter sliders (10 controls)
    juce::Slider materialSlider;
    juce::Slider topologySlider;
    juce::Slider viscositySlider;
    juce::Slider evolutionSlider;
    juce::Slider chaosSlider;
    juce::Slider elasticitySlider;
    juce::Slider patinaSlider;
    juce::Slider abyssSlider;
    juce::Slider coronaSlider;
    juce::Slider breathSlider;

    // Labels for sliders
    juce::Label materialLabel;
    juce::Label topologyLabel;
    juce::Label viscosityLabel;
    juce::Label evolutionLabel;
    juce::Label chaosLabel;
    juce::Label elasticityLabel;
    juce::Label patinaLabel;
    juce::Label abyssLabel;
    juce::Label coronaLabel;
    juce::Label breathLabel;

    // Slider attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> materialAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> topologyAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> viscosityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> evolutionAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> chaosAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> elasticityAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> patinaAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> abyssAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> coronaAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> breathAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MonumentAudioProcessorEditor)
};
