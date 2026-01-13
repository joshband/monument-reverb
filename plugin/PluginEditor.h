#pragma once

#include <JuceHeader.h>
#include <array>
#include <initializer_list>

#include "PluginProcessor.h"

/**
 * Expanded JUCE-based editor for Monument Reverb.
 * Exposes full DSP controls with an optional debug mode for advanced sections.
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

    struct LabeledSlider
    {
        juce::Slider slider;
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

    LabeledCombo macroModeControl;
    LabeledCombo routingPresetControl;
    LabeledCombo pillarModeControl;
    LabeledCombo timelinePresetControl;

    LabeledToggle freezeControl;
    LabeledToggle timelineEnabledControl;
    LabeledToggle safetyClipControl;

    LabeledSlider mixControl;
    LabeledSlider timeControl;
    LabeledSlider massControl;
    LabeledSlider densityControl;
    LabeledSlider bloomControl;
    LabeledSlider airControl;
    LabeledSlider widthControl;
    LabeledSlider warpControl;
    LabeledSlider driftControl;
    LabeledSlider gravityControl;
    LabeledSlider pillarShapeControl;

    LabeledSlider materialControl;
    LabeledSlider topologyControl;
    LabeledSlider viscosityControl;
    LabeledSlider evolutionControl;
    LabeledSlider chaosControl;
    LabeledSlider elasticityDecayControl;
    LabeledSlider patinaControl;
    LabeledSlider abyssControl;
    LabeledSlider coronaControl;
    LabeledSlider breathControl;

    LabeledSlider characterControl;
    LabeledSlider spaceTypeControl;
    LabeledSlider energyControl;
    LabeledSlider motionControl;
    LabeledSlider colorControl;
    LabeledSlider dimensionControl;

    LabeledSlider memoryControl;
    LabeledSlider memoryDepthControl;
    LabeledSlider memoryDecayControl;
    LabeledSlider memoryDriftControl;

    LabeledSlider tubeCountControl;
    LabeledSlider radiusVariationControl;
    LabeledSlider metallicResonanceControl;
    LabeledSlider couplingStrengthControl;
    LabeledSlider elasticityControl;
    LabeledSlider recoveryTimeControl;
    LabeledSlider absorptionDriftControl;
    LabeledSlider nonlinearityControl;
    LabeledSlider impossibilityDegreeControl;
    LabeledSlider pitchEvolutionRateControl;
    LabeledSlider paradoxResonanceFreqControl;
    LabeledSlider paradoxGainControl;

    LabeledSlider safetyClipDriveControl;

    juce::Label inputLevelLabel;
    juce::Label outputLevelLabel;

    juce::Label modulationSummaryLabel;
    std::array<juce::Label, kModulationRowCount> modulationConnectionLabels;
    juce::TextButton modulationSparseButton;
    juce::TextButton modulationDenseButton;
    juce::TextButton modulationClearButton;

    bool debugMode{false};
    SectionView activeSection{SectionView::BaseParams};
    int modulationLabelTick{0};

    void timerCallback() override;

    void setupSlider(LabeledSlider& control, const juce::String& labelText, const juce::String& paramId);
    void setupCombo(LabeledCombo& control,
                    const juce::String& labelText,
                    const juce::StringArray& items,
                    const juce::String& paramId);
    void setupToggle(LabeledToggle& control, const juce::String& labelText, const juce::String& paramId);

    ControlEntry entry(LabeledSlider& control) const;
    ControlEntry entry(LabeledCombo& control) const;
    ControlEntry entry(LabeledToggle& control) const;

    int layoutGroup(juce::GroupComponent& group,
                    int y,
                    int columns,
                    std::initializer_list<ControlEntry> controls);
    void setControlsVisible(std::initializer_list<ControlEntry> controls, bool visible);
    void setComponentsVisible(std::initializer_list<juce::Component*> components, bool visible);

    void setActiveSection(SectionView view);
    void layoutControls();
    void updateDebugVisibility();
    void updateSectionVisibility();
    void updateMacroModeVisibility(bool allowShow);
    void updateModulationLabels();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MonumentAudioProcessorEditor)
};
