#pragma once

#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "ui/MonumentKnob.h"
#include "ui/MonumentToggle.h"
#include "ui/MonumentTimeKnob.h"
#include "ui/HeroKnob.h"
#include "ui/PhotorealisticKnob.h"
#include "ui/EnhancedBackgroundComponent.h"
#include "ui/HeaderBar.h"
#include "ui/CollapsiblePanel.h"
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

    // Background (bottom-most layer)
    EnhancedBackgroundComponent enhancedBackground;

    // Header bar (top-most UI layer)
    std::unique_ptr<HeaderBar> headerBar;

    // Collapsible Panels
    std::unique_ptr<CollapsiblePanel> macroControlPanel;
    std::unique_ptr<CollapsiblePanel> foundationPanel;
    std::unique_ptr<CollapsiblePanel> modulationNexusPanel;

    // Macro Controls (Primary Interface) - All PhotorealisticKnob
    PhotorealisticKnob materialKnob;
    PhotorealisticKnob topologyKnob;
    PhotorealisticKnob viscosityKnob;
    PhotorealisticKnob evolutionKnob;
    PhotorealisticKnob chaosKnob;
    PhotorealisticKnob elasticityKnob;
    PhotorealisticKnob patinaKnob;      // Ancient Monuments Phase 5
    PhotorealisticKnob abyssKnob;       // Ancient Monuments Phase 5
    PhotorealisticKnob coronaKnob;      // Ancient Monuments Phase 5
    PhotorealisticKnob breathKnob;      // Ancient Monuments Phase 5

    // Base Parameters - All PhotorealisticKnob with stone variants
    PhotorealisticKnob mixKnob;
    PhotorealisticKnob timeKnob;
    PhotorealisticKnob sizeHeroKnob;
    PhotorealisticKnob massKnob;
    PhotorealisticKnob densityKnob;
    PhotorealisticKnob bloomKnob;
    PhotorealisticKnob airKnob;
    PhotorealisticKnob widthKnob;
    PhotorealisticKnob warpKnob;
    PhotorealisticKnob driftKnob;
    PhotorealisticKnob gravityKnob;
    MonumentToggle freezeToggle;
    juce::ComboBox presetBox;
    juce::TextButton savePresetButton;

    // Modulation Matrix Panel (separate from collapsible panels)
    std::unique_ptr<monument::ui::ModMatrixPanel> modMatrixPanel;
    juce::TextButton modMatrixToggleButton;
    bool modMatrixVisible{false};

    // User preset management
    void refreshPresetList();
    void scanUserPresets();
    void showSavePresetDialog();
    std::vector<juce::File> userPresetFiles;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MonumentAudioProcessorEditor)
};
