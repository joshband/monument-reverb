#pragma once

#include <JuceHeader.h>
#include <array>
#include <initializer_list>

#include "PluginProcessor.h"
#include "ui/PhotorealisticKnob.h"

/**
 * Photorealistic-inspired UI for Monument Reverb.
 * Keeps full DSP access with tabbed sections for Base Params, Modulation, and Timeline.
 */
class MonumentAudioProcessorEditorV2 : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    explicit MonumentAudioProcessorEditorV2(MonumentAudioProcessor&);
    ~MonumentAudioProcessorEditorV2() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    enum class SectionView
    {
        BaseParams,
        Modulation,
        Timeline
    };

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    static constexpr int kModulationRowCount = 8;

    struct LabeledKnob
    {
        monument::PhotorealisticKnob knob;
        juce::Label label;
        std::unique_ptr<SliderAttachment> attachment;
    };

    struct LabeledCombo
    {
        juce::ComboBox combo;
        juce::Label label;
        std::unique_ptr<ComboBoxAttachment> attachment;
    };

    struct LabeledToggle
    {
        juce::ToggleButton toggle;
        juce::Label label;
        std::unique_ptr<ButtonAttachment> attachment;
    };

    struct ControlEntry
    {
        juce::Component* control{nullptr};
        juce::Label* label{nullptr};
        int controlHeight{0};
        int controlWidth{0};
    };

    MonumentAudioProcessor& processorRef;

    juce::Label titleLabel;
    juce::ToggleButton debugToggle;
    juce::TextButton baseParamsButton;
    juce::TextButton modulationButton;
    juce::TextButton timelineButton;

    juce::Viewport controlsViewport;
    juce::Component controlsContent;

    juce::GroupComponent macroModeGroup;
    juce::GroupComponent ancientMacroGroup;
    juce::GroupComponent expressiveMacroGroup;
    juce::GroupComponent coreGroup;
    juce::GroupComponent routingGroup;
    juce::GroupComponent modulationGroup;
    juce::GroupComponent memoryGroup;
    juce::GroupComponent physicalGroup;
    juce::GroupComponent timelineGroup;
    juce::GroupComponent safetyGroup;
    juce::GroupComponent diagnosticsGroup;

    juce::TextButton macroModeToggle;
    juce::TextButton ancientMacroToggle;
    juce::TextButton expressiveMacroToggle;
    juce::TextButton coreToggle;
    juce::TextButton routingToggle;
    juce::TextButton modulationToggle;
    juce::TextButton memoryToggle;
    juce::TextButton physicalToggle;
    juce::TextButton timelineToggle;
    juce::TextButton safetyToggle;
    juce::TextButton diagnosticsToggle;

    LabeledCombo macroModeControl;
    LabeledCombo routingPresetControl;
    LabeledCombo pillarModeControl;
    LabeledCombo timelinePresetControl;

    LabeledToggle freezeControl;
    LabeledToggle timelineEnabledControl;
    LabeledToggle safetyClipControl;

    LabeledKnob mixControl;
    LabeledKnob timeControl;
    LabeledKnob massControl;
    LabeledKnob densityControl;
    LabeledKnob bloomControl;
    LabeledKnob airControl;
    LabeledKnob widthControl;
    LabeledKnob warpControl;
    LabeledKnob driftControl;
    LabeledKnob gravityControl;
    LabeledKnob pillarShapeControl;

    LabeledKnob materialControl;
    LabeledKnob topologyControl;
    LabeledKnob viscosityControl;
    LabeledKnob evolutionControl;
    LabeledKnob chaosControl;
    LabeledKnob elasticityDecayControl;
    LabeledKnob patinaControl;
    LabeledKnob abyssControl;
    LabeledKnob coronaControl;
    LabeledKnob breathControl;

    LabeledKnob characterControl;
    LabeledKnob spaceTypeControl;
    LabeledKnob energyControl;
    LabeledKnob motionControl;
    LabeledKnob colorControl;
    LabeledKnob dimensionControl;

    LabeledKnob memoryControl;
    LabeledKnob memoryDepthControl;
    LabeledKnob memoryDecayControl;
    LabeledKnob memoryDriftControl;

    LabeledKnob tubeCountControl;
    LabeledKnob radiusVariationControl;
    LabeledKnob metallicResonanceControl;
    LabeledKnob couplingStrengthControl;
    LabeledKnob elasticityControl;
    LabeledKnob recoveryTimeControl;
    LabeledKnob absorptionDriftControl;
    LabeledKnob nonlinearityControl;
    LabeledKnob impossibilityDegreeControl;
    LabeledKnob pitchEvolutionRateControl;
    LabeledKnob paradoxResonanceFreqControl;
    LabeledKnob paradoxGainControl;

    LabeledKnob safetyClipDriveControl;

    juce::Label inputLevelLabel;
    juce::Label outputLevelLabel;

    juce::Label modulationSummaryLabel;
    std::array<juce::Label, kModulationRowCount> modulationConnectionLabels;
    juce::TextButton modulationSparseButton;
    juce::TextButton modulationDenseButton;
    juce::TextButton modulationClearButton;

    bool debugMode{false};
    bool macroModeExpanded{false};
    bool ancientMacroExpanded{true};
    bool expressiveMacroExpanded{true};
    bool coreExpanded{true};
    bool routingExpanded{false};
    bool modulationExpanded{true};
    bool memoryExpanded{false};
    bool physicalExpanded{false};
    bool timelineExpanded{true};
    bool safetyExpanded{false};
    bool diagnosticsExpanded{false};
    SectionView activeSection{SectionView::BaseParams};
    int modulationLabelTick{0};

    void timerCallback() override;

    void setupKnob(LabeledKnob& control,
                   const juce::String& labelText,
                   const juce::String& paramId,
                   juce::Colour ledColor);
    void setupCombo(LabeledCombo& control,
                    const juce::String& labelText,
                    const juce::StringArray& items,
                    const juce::String& paramId);
    void setupToggle(LabeledToggle& control, const juce::String& labelText, const juce::String& paramId);

    ControlEntry entry(LabeledKnob& control) const;
    ControlEntry entry(LabeledCombo& control) const;
    ControlEntry entry(LabeledToggle& control) const;

    void setupGroupToggle(juce::TextButton& toggle, bool& expanded);
    void updateGroupToggle(juce::TextButton& toggle, bool expanded);
    int layoutCollapsedGroup(juce::GroupComponent& group, juce::TextButton& toggle, int y);
    void positionGroupToggle(juce::GroupComponent& group, juce::TextButton& toggle);
    void loadUiState();
    void persistUiState();

    int layoutGroup(juce::GroupComponent& group,
                    int y,
                    int columns,
                    std::initializer_list<ControlEntry> controls);
    void setControlsVisible(std::initializer_list<ControlEntry> controls, bool visible);
    void setComponentsVisible(std::initializer_list<juce::Component*> components, bool visible);

    void setActiveSection(SectionView view);
    void layoutControls();
    void updateSectionVisibility();
    void updateMacroModeVisibility(bool allowShow);
    void updateModulationLabels();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MonumentAudioProcessorEditorV2)
};
