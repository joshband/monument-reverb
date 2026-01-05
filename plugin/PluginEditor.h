#pragma once

#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "ui/MonumentKnob.h"
#include "ui/MonumentToggle.h"
#include "ui/MonumentTimeKnob.h"
#include "ui/HeroKnob.h"
#include "ui/ModMatrixPanel.h"
#include "ui/TimelineComponent.h"

class MonumentAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MonumentAudioProcessorEditor(MonumentAudioProcessor&);
    ~MonumentAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    MonumentAudioProcessor& processorRef;

    // Macro Controls (Primary Interface) - All using HeroKnob (codex brushed aluminum)
    HeroKnob materialKnob;
    HeroKnob topologyKnob;
    HeroKnob viscosityKnob;
    HeroKnob evolutionKnob;
    HeroKnob chaosKnob;
    HeroKnob elasticityKnob;
    HeroKnob patinaKnob;      // Ancient Monuments Phase 5
    HeroKnob abyssKnob;       // Ancient Monuments Phase 5
    HeroKnob coronaKnob;      // Ancient Monuments Phase 5
    HeroKnob breathKnob;      // Ancient Monuments Phase 5

    // Base Parameters - All using HeroKnob (codex brushed aluminum)
    HeroKnob mixKnob;
    HeroKnob timeKnob;
    HeroKnob sizeHeroKnob;
    HeroKnob massKnob;
    HeroKnob densityKnob;
    HeroKnob bloomKnob;
    HeroKnob airKnob;
    HeroKnob widthKnob;
    HeroKnob warpKnob;
    HeroKnob driftKnob;
    HeroKnob gravityKnob;
    MonumentToggle freezeToggle;
    juce::ComboBox presetBox;
    juce::TextButton savePresetButton;

    // Routing Architecture Selector (Phase 1.5)
    juce::Label routingPresetLabel;
    juce::ComboBox routingPresetBox;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> routingPresetAttachment;

    // Processing Mode Selector (Ancient Monuments Routing)
    juce::Label processingModeLabel;
    juce::ComboBox processingModeBox;

    // Modulation Matrix Panel
    juce::TextButton modMatrixToggleButton;
    std::unique_ptr<monument::ui::ModMatrixPanel> modMatrixPanel;
    bool modMatrixVisible{false};

    // Timeline Editor Panel (Phase 5)
    juce::TextButton timelineToggleButton;
    std::unique_ptr<monument::ui::TimelineComponent> timelinePanel;
    bool timelineVisible{false};

    // Base Parameters visibility toggle
    juce::TextButton baseParamsToggleButton;
    bool baseParamsVisible{false};  // Hidden by default - macros are primary interface

    // User preset management
    void refreshPresetList();
    void scanUserPresets();
    void showSavePresetDialog();
    std::vector<juce::File> userPresetFiles;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MonumentAudioProcessorEditor)
};
