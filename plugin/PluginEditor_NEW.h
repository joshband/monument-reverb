#pragma once

#include <JuceHeader.h>

#include "PluginProcessor.h"
#include "ui/EnhancedBackgroundComponent.h"
#include "ui/HeaderBar.h"
#include "ui/CollapsiblePanel.h"
#include "ui/PhotorealisticKnob.h"
#include "ui/ModMatrixPanel.h"
#include "ui/TimelineComponent.h"

/**
 * Monument Reverb Plugin Editor - Photorealistic Brutalist UI
 *
 * Layout Structure (matches mockup design):
 * ┌─────────────────────────────────────────────────┐
 * │ HeaderBar (60px)                                 │
 * │ - MONUMENT logo, preset selector, meters        │
 * ├─────────────────────────────────────────────────┤
 * │ ▶ THE MACRO CONTROL                              │
 * │   [12 stone knobs in 2 rows of 6]               │
 * ├─────────────────────────────────────────────────┤
 * │ ▶ THE FOUNDATION                                 │
 * │   [11 base parameter knobs]                     │
 * ├─────────────────────────────────────────────────┤
 * │ ▶ THE MODULATION NEXUS                           │
 * │   [Timeline editor with orange keyframes]       │
 * └─────────────────────────────────────────────────┘
 *
 * Visual Theme:
 * - Dark stone background (#0d0d0d - #1a1a1a)
 * - Blue ethereal wisps (animated fog effects)
 * - Stone knobs with bright blue LED centers
 * - Orange timeline keyframes (#ff8844)
 * - Collapsible panels with smooth animations
 */
class MonumentAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    explicit MonumentAudioProcessorEditor(MonumentAudioProcessor&);
    ~MonumentAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // Timer callback for level meters
    void timerCallback() override;

    MonumentAudioProcessor& processorRef;

    // Background layer (z-order: bottom)
    std::unique_ptr<EnhancedBackgroundComponent> background;

    // Header bar (z-order: top)
    std::unique_ptr<HeaderBar> headerBar;

    // Three collapsible panels
    std::unique_ptr<CollapsiblePanel> macroControlPanel;
    std::unique_ptr<CollapsiblePanel> foundationPanel;
    std::unique_ptr<CollapsiblePanel> modulationNexusPanel;

    // Content components for each panel
    std::unique_ptr<juce::Component> macroControlContent;
    std::unique_ptr<juce::Component> foundationContent;

    // === THE MACRO CONTROL (12 knobs) ===
    // Row 1: Material, Topology, Viscosity, Evolution, Chaos, Elasticity
    std::unique_ptr<PhotorealisticKnob> materialKnob;
    std::unique_ptr<PhotorealisticKnob> topologyKnob;
    std::unique_ptr<PhotorealisticKnob> viscosityKnob;
    std::unique_ptr<PhotorealisticKnob> evolutionKnob;
    std::unique_ptr<PhotorealisticKnob> chaosKnob;
    std::unique_ptr<PhotorealisticKnob> elasticityKnob;

    // Row 2: Time, Bloom, Density, 10.5x Suns (Mass?), Mass, Empire (?)
    std::unique_ptr<PhotorealisticKnob> timeKnob;
    std::unique_ptr<PhotorealisticKnob> bloomKnob;
    std::unique_ptr<PhotorealisticKnob> densityKnob;
    std::unique_ptr<PhotorealisticKnob> massKnob;
    std::unique_ptr<PhotorealisticKnob> patinaKnob;
    std::unique_ptr<PhotorealisticKnob> abyssKnob;

    // === THE FOUNDATION (11 knobs) ===
    std::unique_ptr<PhotorealisticKnob> mixKnob;
    std::unique_ptr<PhotorealisticKnob> sizeKnob;
    std::unique_ptr<PhotorealisticKnob> gravityKnob;
    std::unique_ptr<PhotorealisticKnob> velocityKnob;
    std::unique_ptr<PhotorealisticKnob> filterKnob;
    std::unique_ptr<PhotorealisticKnob> rateKnob;
    std::unique_ptr<PhotorealisticKnob> coronaKnob;
    std::unique_ptr<PhotorealisticKnob> breathKnob;
    std::unique_ptr<PhotorealisticKnob> airKnob;
    std::unique_ptr<PhotorealisticKnob> widthKnob;
    std::unique_ptr<PhotorealisticKnob> warpKnob;

    // === THE MODULATION NEXUS ===
    std::unique_ptr<monument::ui::ModMatrixPanel> modMatrixPanel;
    std::unique_ptr<monument::ui::TimelineComponent> timelinePanel;

    // Layout constants
    static constexpr int WINDOW_WIDTH = 900;
    static constexpr int HEADER_HEIGHT = 60;
    static constexpr int KNOB_SIZE = 100;
    static constexpr int KNOB_SPACING = 20;

    // Helper methods
    void createMacroControlContent();
    void createFoundationContent();
    void layoutKnobGrid(juce::Component* parent,
                        std::vector<PhotorealisticKnob*> knobs,
                        int columns, int rows);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MonumentAudioProcessorEditor)
};
