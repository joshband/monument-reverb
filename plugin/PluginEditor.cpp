#include "PluginEditor.h"
#include "dsp/ModulationMatrix.h"
#include "dsp/SequencePresets.h"

namespace
{
constexpr int kHeaderHeight = 86;
constexpr int kMargin = 16;
constexpr int kGroupPadding = 12;
constexpr int kGroupSpacing = 18;
constexpr int kLabelHeight = 18;
constexpr int kControlHeight = 80;
constexpr int kSmallControlHeight = 24;
constexpr int kRowSpacing = 12;
constexpr int kColSpacing = 12;
constexpr int kMeterUpdateHz = 20;
constexpr int kTabRowHeight = 26;
constexpr int kEditorWidth = 1100;
constexpr int kEditorHeight = 820;
constexpr int kSectionButtonGroup = 2001;
constexpr int kModulationRefreshTicks = 5;

juce::String modulationSourceToString(monument::dsp::ModulationMatrix::SourceType source)
{
    using Source = monument::dsp::ModulationMatrix::SourceType;
    switch (source)
    {
        case Source::ChaosAttractor: return "Chaos";
        case Source::AudioFollower: return "Audio Follower";
        case Source::BrownianMotion: return "Brownian";
        case Source::EnvelopeTracker: return "Envelope";
        case Source::Count: break;
    }
    return "Unknown";
}

juce::String modulationDestinationToString(monument::dsp::ModulationMatrix::DestinationType destination)
{
    using Dest = monument::dsp::ModulationMatrix::DestinationType;
    switch (destination)
    {
        case Dest::Time: return "Time";
        case Dest::Mass: return "Mass";
        case Dest::Density: return "Density";
        case Dest::Bloom: return "Bloom";
        case Dest::Air: return "Air";
        case Dest::Width: return "Width";
        case Dest::Mix: return "Mix";
        case Dest::Warp: return "Warp";
        case Dest::Drift: return "Drift";
        case Dest::Gravity: return "Gravity";
        case Dest::PillarShape: return "Pillar Shape";
        case Dest::TubeCount: return "Tube Count";
        case Dest::RadiusVariation: return "Radius Variation";
        case Dest::MetallicResonance: return "Metallic Resonance";
        case Dest::CouplingStrength: return "Coupling Strength";
        case Dest::Elasticity: return "Elasticity";
        case Dest::RecoveryTime: return "Recovery Time";
        case Dest::AbsorptionDrift: return "Absorption Drift";
        case Dest::Nonlinearity: return "Nonlinearity";
        case Dest::ImpossibilityDegree: return "Impossibility";
        case Dest::PitchEvolutionRate: return "Pitch Evolution";
        case Dest::ParadoxResonanceFreq: return "Paradox Freq";
        case Dest::ParadoxGain: return "Paradox Gain";
        case Dest::PositionX: return "Position X";
        case Dest::PositionY: return "Position Y";
        case Dest::PositionZ: return "Position Z";
        case Dest::Distance: return "Distance";
        case Dest::VelocityX: return "Velocity X";
        case Dest::Count: break;
    }
    return "Unknown";
}
}

MonumentAudioProcessorEditor::MonumentAudioProcessorEditor(MonumentAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p)
{
    setSize(kEditorWidth, kEditorHeight);
#if defined(MONUMENT_TESTING)
    setResizable(false, false);
    setSize(kEditorWidth, kEditorHeight);
#endif

    titleLabel.setText("MONUMENT REVERB", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(22.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);

    debugToggle.setButtonText("Debug");
    debugToggle.setToggleState(false, juce::dontSendNotification);
    debugToggle.onClick = [this]
    {
        debugMode = debugToggle.getToggleState();
        updateSectionVisibility();
        resized();
    };
    addAndMakeVisible(debugToggle);

    baseParamsButton.setButtonText("BASE PARAMS");
    baseParamsButton.setClickingTogglesState(true);
    baseParamsButton.setRadioGroupId(kSectionButtonGroup);
    baseParamsButton.setAccessible(true);
    baseParamsButton.setTitle("BASE PARAMS");
    baseParamsButton.setTooltip("BASE PARAMS");
    baseParamsButton.onClick = [this]
    {
        setActiveSection(SectionView::BaseParams);
    };
    addAndMakeVisible(baseParamsButton);

    modulationButton.setButtonText("MODULATION");
    modulationButton.setClickingTogglesState(true);
    modulationButton.setRadioGroupId(kSectionButtonGroup);
    modulationButton.setAccessible(true);
    modulationButton.setTitle("MODULATION");
    modulationButton.setTooltip("MODULATION");
    modulationButton.onClick = [this]
    {
        setActiveSection(SectionView::Modulation);
    };
    addAndMakeVisible(modulationButton);

    timelineButton.setButtonText("TIMELINE");
    timelineButton.setClickingTogglesState(true);
    timelineButton.setRadioGroupId(kSectionButtonGroup);
    timelineButton.setAccessible(true);
    timelineButton.setTitle("TIMELINE");
    timelineButton.setTooltip("TIMELINE");
    timelineButton.onClick = [this]
    {
        setActiveSection(SectionView::Timeline);
    };
    addAndMakeVisible(timelineButton);

    for (auto* button : {&baseParamsButton, &modulationButton, &timelineButton})
    {
        button->setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));
        button->setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff3a3a3a));
        button->setColour(juce::TextButton::textColourOffId, juce::Colours::lightgrey);
        button->setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    }
    baseParamsButton.setToggleState(true, juce::dontSendNotification);

    controlsViewport.setViewedComponent(&controlsContent, false);
    controlsViewport.setScrollBarsShown(true, false);
    addAndMakeVisible(controlsViewport);

    macroModeGroup.setText("Macro System");
    ancientMacroGroup.setText("Ancient Macros");
    expressiveMacroGroup.setText("Expressive Macros");
    coreGroup.setText("Core DSP");
    routingGroup.setText("Routing + Modes");
    modulationGroup.setText("Modulation Nexus");
    memoryGroup.setText("Memory System");
    physicalGroup.setText("Physical Modeling");
    timelineGroup.setText("Timeline");
    safetyGroup.setText("Output Safety");
    diagnosticsGroup.setText("Diagnostics");

    controlsContent.addAndMakeVisible(macroModeGroup);
    controlsContent.addAndMakeVisible(ancientMacroGroup);
    controlsContent.addAndMakeVisible(expressiveMacroGroup);
    controlsContent.addAndMakeVisible(coreGroup);
    controlsContent.addAndMakeVisible(routingGroup);
    controlsContent.addAndMakeVisible(modulationGroup);
    controlsContent.addAndMakeVisible(memoryGroup);
    controlsContent.addAndMakeVisible(physicalGroup);
    controlsContent.addAndMakeVisible(timelineGroup);
    controlsContent.addAndMakeVisible(safetyGroup);
    controlsContent.addAndMakeVisible(diagnosticsGroup);

    juce::StringArray macroModeItems;
    macroModeItems.add("Ancient Monuments");
    macroModeItems.add("Expressive");
    setupCombo(macroModeControl, "Macro Mode", macroModeItems, "macroMode");
    macroModeControl.combo.onChange = [this]
    {
        updateSectionVisibility();
        resized();
    };

    juce::StringArray routingItems;
    routingItems.add("Traditional Cathedral");
    routingItems.add("Metallic Granular");
    routingItems.add("Elastic Feedback Dream");
    routingItems.add("Parallel Worlds");
    routingItems.add("Shimmer Infinity");
    routingItems.add("Impossible Chaos");
    routingItems.add("Organic Breathing");
    routingItems.add("Minimal Sparse");
    setupCombo(routingPresetControl, "Architecture", routingItems, "routingPreset");

    juce::StringArray pillarModeItems;
    pillarModeItems.add("Glass");
    pillarModeItems.add("Stone");
    pillarModeItems.add("Fog");
    setupCombo(pillarModeControl, "Pillar Mode", pillarModeItems, "pillarMode");

    juce::StringArray timelinePresets;
    for (int i = 0; i < monument::dsp::SequencePresets::getNumPresets(); ++i)
        timelinePresets.add(monument::dsp::SequencePresets::getPresetName(i));
    setupCombo(timelinePresetControl, "Sequence", timelinePresets, "timelinePreset");

    setupToggle(freezeControl, "Freeze", "freeze");
    setupToggle(timelineEnabledControl, "Timeline Enabled", "timelineEnabled");
    setupToggle(safetyClipControl, "Safety Clip", "safetyClip");

    setupSlider(mixControl, "Mix", "mix");
    setupSlider(timeControl, "Time", "time");
    setupSlider(massControl, "Mass", "mass");
    setupSlider(densityControl, "Density", "density");
    setupSlider(bloomControl, "Bloom", "bloom");
    setupSlider(airControl, "Air", "air");
    setupSlider(widthControl, "Width", "width");
    setupSlider(warpControl, "Warp", "warp");
    setupSlider(driftControl, "Drift", "drift");
    setupSlider(gravityControl, "Gravity", "gravity");
    setupSlider(pillarShapeControl, "Pillar Shape", "pillarShape");

    setupSlider(materialControl, "Material", "material");
    setupSlider(topologyControl, "Topology", "topology");
    setupSlider(viscosityControl, "Viscosity", "viscosity");
    setupSlider(evolutionControl, "Evolution", "evolution");
    setupSlider(chaosControl, "Chaos", "chaosIntensity");
    setupSlider(elasticityDecayControl, "Elasticity", "elasticityDecay");
    setupSlider(patinaControl, "Patina", "patina");
    setupSlider(abyssControl, "Abyss", "abyss");
    setupSlider(coronaControl, "Corona", "corona");
    setupSlider(breathControl, "Breath", "breath");

    setupSlider(characterControl, "Character", "character");
    setupSlider(spaceTypeControl, "Space Type", "spaceType");
    setupSlider(energyControl, "Energy", "energy");
    setupSlider(motionControl, "Motion", "motion");
    setupSlider(colorControl, "Color", "color");
    setupSlider(dimensionControl, "Dimension", "dimension");

    setupSlider(memoryControl, "Memory", "memory");
    setupSlider(memoryDepthControl, "Memory Depth", "memoryDepth");
    setupSlider(memoryDecayControl, "Memory Decay", "memoryDecay");
    setupSlider(memoryDriftControl, "Memory Drift", "memoryDrift");

    setupSlider(tubeCountControl, "Tube Count", "tubeCount");
    setupSlider(radiusVariationControl, "Radius Variation", "radiusVariation");
    setupSlider(metallicResonanceControl, "Metallic Resonance", "metallicResonance");
    setupSlider(couplingStrengthControl, "Coupling Strength", "couplingStrength");
    setupSlider(elasticityControl, "Elasticity", "elasticity");
    setupSlider(recoveryTimeControl, "Recovery Time", "recoveryTime");
    setupSlider(absorptionDriftControl, "Absorption Drift", "absorptionDrift");
    setupSlider(nonlinearityControl, "Nonlinearity", "nonlinearity");
    setupSlider(impossibilityDegreeControl, "Impossibility", "impossibilityDegree");
    setupSlider(pitchEvolutionRateControl, "Pitch Evolution", "pitchEvolutionRate");
    setupSlider(paradoxResonanceFreqControl, "Paradox Freq", "paradoxResonanceFreq");
    setupSlider(paradoxGainControl, "Paradox Gain", "paradoxGain");

    setupSlider(safetyClipDriveControl, "Safety Drive", "safetyClipDrive");

    modulationSummaryLabel.setText("Active connections: 0", juce::dontSendNotification);
    modulationSummaryLabel.setJustificationType(juce::Justification::centredLeft);
    modulationSummaryLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    controlsContent.addAndMakeVisible(modulationSummaryLabel);

    modulationSparseButton.setButtonText("Randomize Sparse");
    modulationSparseButton.onClick = [this]
    {
        processorRef.getModulationMatrix().randomizeSparse();
        updateModulationLabels();
    };
    controlsContent.addAndMakeVisible(modulationSparseButton);

    modulationDenseButton.setButtonText("Randomize Dense");
    modulationDenseButton.onClick = [this]
    {
        processorRef.getModulationMatrix().randomizeDense();
        updateModulationLabels();
    };
    controlsContent.addAndMakeVisible(modulationDenseButton);

    modulationClearButton.setButtonText("Clear");
    modulationClearButton.onClick = [this]
    {
        processorRef.getModulationMatrix().clearConnections();
        updateModulationLabels();
    };
    controlsContent.addAndMakeVisible(modulationClearButton);

    for (auto& label : modulationConnectionLabels)
    {
        label.setText("", juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centredLeft);
        label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        label.setFont(juce::Font(13.0f));
        controlsContent.addAndMakeVisible(label);
    }

    inputLevelLabel.setText("Input: -- dB", juce::dontSendNotification);
    inputLevelLabel.setFont(juce::Font(13.0f, juce::Font::bold));
    inputLevelLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    controlsContent.addAndMakeVisible(inputLevelLabel);

    outputLevelLabel.setText("Output: -- dB", juce::dontSendNotification);
    outputLevelLabel.setFont(juce::Font(13.0f, juce::Font::bold));
    outputLevelLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    controlsContent.addAndMakeVisible(outputLevelLabel);

    updateSectionVisibility();
    updateModulationLabels();
    resized();
    startTimerHz(kMeterUpdateHz);
}

MonumentAudioProcessorEditor::~MonumentAudioProcessorEditor() = default;

void MonumentAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a1a));
    g.setColour(juce::Colours::darkgrey);
    g.drawRect(getLocalBounds(), 2);
}

void MonumentAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    auto header = bounds.removeFromTop(kHeaderHeight).reduced(kMargin, 8);
    auto topRow = header.removeFromTop(kLabelHeight + 4);
    titleLabel.setBounds(topRow.removeFromLeft(360));
    debugToggle.setBounds(topRow.removeFromRight(100));
    header.removeFromTop(6);
    auto tabRow = header.removeFromTop(kTabRowHeight);
    const int buttonWidth = (tabRow.getWidth() - 2 * kColSpacing) / 3;
    baseParamsButton.setBounds(tabRow.removeFromLeft(buttonWidth));
    tabRow.removeFromLeft(kColSpacing);
    modulationButton.setBounds(tabRow.removeFromLeft(buttonWidth));
    tabRow.removeFromLeft(kColSpacing);
    timelineButton.setBounds(tabRow.removeFromLeft(buttonWidth));

    controlsViewport.setBounds(bounds);
    layoutControls();
}

void MonumentAudioProcessorEditor::timerCallback()
{
    const auto inputDb = juce::Decibels::gainToDecibels(processorRef.getInputLevel(), -80.0f);
    const auto outputDb = juce::Decibels::gainToDecibels(processorRef.getOutputLevel(), -80.0f);
    inputLevelLabel.setText("Input: " + juce::String(inputDb, 1) + " dB", juce::dontSendNotification);
    outputLevelLabel.setText("Output: " + juce::String(outputDb, 1) + " dB", juce::dontSendNotification);

    if (modulationGroup.isVisible())
    {
        if (++modulationLabelTick >= kModulationRefreshTicks)
        {
            modulationLabelTick = 0;
            updateModulationLabels();
        }
    }
    else
    {
        modulationLabelTick = 0;
    }
}

void MonumentAudioProcessorEditor::setupSlider(LabeledSlider& control,
                                               const juce::String& labelText,
                                               const juce::String& paramId)
{
    control.slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    control.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
    control.slider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
    control.slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::lightskyblue);
    control.slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::dimgrey);
    controlsContent.addAndMakeVisible(control.slider);

    control.label.setText(labelText, juce::dontSendNotification);
    control.label.setJustificationType(juce::Justification::centred);
    control.label.setColour(juce::Label::textColourId, juce::Colours::white);
    controlsContent.addAndMakeVisible(control.label);

    control.attachment = std::make_unique<SliderAttachment>(processorRef.getAPVTS(), paramId, control.slider);
}

void MonumentAudioProcessorEditor::setupCombo(LabeledCombo& control,
                                              const juce::String& labelText,
                                              const juce::StringArray& items,
                                              const juce::String& paramId)
{
    control.combo.addItemList(items, 1);
    control.combo.setJustificationType(juce::Justification::centred);
    controlsContent.addAndMakeVisible(control.combo);

    control.label.setText(labelText, juce::dontSendNotification);
    control.label.setJustificationType(juce::Justification::centred);
    control.label.setColour(juce::Label::textColourId, juce::Colours::white);
    controlsContent.addAndMakeVisible(control.label);

    control.attachment = std::make_unique<ComboBoxAttachment>(processorRef.getAPVTS(), paramId, control.combo);
}

void MonumentAudioProcessorEditor::setupToggle(LabeledToggle& control,
                                               const juce::String& labelText,
                                               const juce::String& paramId)
{
    control.toggle.setClickingTogglesState(true);
    controlsContent.addAndMakeVisible(control.toggle);

    control.label.setText(labelText, juce::dontSendNotification);
    control.label.setJustificationType(juce::Justification::centred);
    control.label.setColour(juce::Label::textColourId, juce::Colours::white);
    controlsContent.addAndMakeVisible(control.label);

    control.attachment = std::make_unique<ButtonAttachment>(processorRef.getAPVTS(), paramId, control.toggle);
}

MonumentAudioProcessorEditor::ControlEntry MonumentAudioProcessorEditor::entry(LabeledSlider& control) const
{
    return {&control.slider, &control.label, kControlHeight, kControlHeight};
}

MonumentAudioProcessorEditor::ControlEntry MonumentAudioProcessorEditor::entry(LabeledCombo& control) const
{
    return {&control.combo, &control.label, kSmallControlHeight, 0};
}

MonumentAudioProcessorEditor::ControlEntry MonumentAudioProcessorEditor::entry(LabeledToggle& control) const
{
    return {&control.toggle, &control.label, kSmallControlHeight, 0};
}

int MonumentAudioProcessorEditor::layoutGroup(juce::GroupComponent& group,
                                              int y,
                                              int columns,
                                              std::initializer_list<ControlEntry> controls)
{
    const int contentWidth = controlsContent.getWidth() - kMargin * 2;
    const int rowHeight = kLabelHeight + kControlHeight + kRowSpacing;
    const int totalRows = static_cast<int>((controls.size() + columns - 1) / columns);
    const int groupHeight = kGroupPadding * 2
        + totalRows * (kLabelHeight + kControlHeight)
        + juce::jmax(0, totalRows - 1) * kRowSpacing;

    group.setBounds(kMargin, y, contentWidth, groupHeight);
    auto area = group.getBounds().reduced(kGroupPadding);
    const int cellWidth = (area.getWidth() - (columns - 1) * kColSpacing) / columns;

    int index = 0;
    for (const auto& entry : controls)
    {
        const int row = index / columns;
        const int col = index % columns;
        const int cellX = area.getX() + col * (cellWidth + kColSpacing);
        const int cellY = area.getY() + row * rowHeight;
        if (entry.label)
            entry.label->setBounds(cellX, cellY, cellWidth, kLabelHeight);
        if (entry.control)
        {
            const int targetWidth = entry.controlWidth > 0
                ? juce::jmin(entry.controlWidth, cellWidth)
                : cellWidth;
            const int controlX = cellX + (cellWidth - targetWidth) / 2;
            entry.control->setBounds(controlX,
                                     cellY + kLabelHeight,
                                     targetWidth,
                                     entry.controlHeight);
        }
        ++index;
    }

    return group.getBottom() + kGroupSpacing;
}

void MonumentAudioProcessorEditor::setControlsVisible(std::initializer_list<ControlEntry> controls, bool visible)
{
    for (const auto& entry : controls)
    {
        if (entry.control)
            entry.control->setVisible(visible);
        if (entry.label)
            entry.label->setVisible(visible);
    }
}

void MonumentAudioProcessorEditor::setComponentsVisible(std::initializer_list<juce::Component*> components, bool visible)
{
    for (auto* component : components)
    {
        if (component != nullptr)
            component->setVisible(visible);
    }
}

void MonumentAudioProcessorEditor::setActiveSection(SectionView view)
{
    if (activeSection == view)
        return;

    activeSection = view;
    baseParamsButton.setToggleState(view == SectionView::BaseParams, juce::dontSendNotification);
    modulationButton.setToggleState(view == SectionView::Modulation, juce::dontSendNotification);
    timelineButton.setToggleState(view == SectionView::Timeline, juce::dontSendNotification);
    updateSectionVisibility();
    resized();
}

void MonumentAudioProcessorEditor::layoutControls()
{
    const int contentWidth = controlsViewport.getWidth();
    if (contentWidth <= 0)
        return;

    updateSectionVisibility();

    controlsContent.setBounds(0, 0, contentWidth, 0);

    int y = kMargin;

    if (macroModeGroup.isVisible())
        y = layoutGroup(macroModeGroup, y, 1, {entry(macroModeControl)});

    if (ancientMacroGroup.isVisible())
    {
        y = layoutGroup(ancientMacroGroup, y, 5,
        {
            entry(materialControl),
            entry(topologyControl),
            entry(viscosityControl),
            entry(evolutionControl),
            entry(chaosControl),
            entry(elasticityDecayControl),
            entry(patinaControl),
            entry(abyssControl),
            entry(coronaControl),
            entry(breathControl)
        });
    }

    if (expressiveMacroGroup.isVisible())
    {
        y = layoutGroup(expressiveMacroGroup, y, 3,
        {
            entry(characterControl),
            entry(spaceTypeControl),
            entry(energyControl),
            entry(motionControl),
            entry(colorControl),
            entry(dimensionControl)
        });
    }

    if (coreGroup.isVisible())
    {
        y = layoutGroup(coreGroup, y, 5,
        {
            entry(mixControl),
            entry(timeControl),
            entry(massControl),
            entry(densityControl),
            entry(bloomControl),
            entry(airControl),
            entry(widthControl),
            entry(warpControl),
            entry(driftControl),
            entry(gravityControl),
            entry(pillarShapeControl)
        });
    }

    if (routingGroup.isVisible())
    {
        y = layoutGroup(routingGroup, y, 3,
        {
            entry(routingPresetControl),
            entry(pillarModeControl),
            entry(freezeControl)
        });
    }

    if (modulationGroup.isVisible())
    {
        const int groupWidth = controlsContent.getWidth() - kMargin * 2;
        const int listRows = static_cast<int>(modulationConnectionLabels.size());
        const int rowHeight = kSmallControlHeight;
        const int groupHeight = kGroupPadding * 2
            + kTabRowHeight
            + kRowSpacing
            + kLabelHeight
            + kRowSpacing
            + listRows * rowHeight
            + (listRows - 1) * 4;

        modulationGroup.setBounds(kMargin, y, groupWidth, groupHeight);
        auto area = modulationGroup.getBounds().reduced(kGroupPadding);
        auto buttonRow = area.removeFromTop(kTabRowHeight);
        const int buttonWidth = (buttonRow.getWidth() - 2 * kColSpacing) / 3;
        modulationSparseButton.setBounds(buttonRow.removeFromLeft(buttonWidth));
        buttonRow.removeFromLeft(kColSpacing);
        modulationDenseButton.setBounds(buttonRow.removeFromLeft(buttonWidth));
        buttonRow.removeFromLeft(kColSpacing);
        modulationClearButton.setBounds(buttonRow.removeFromLeft(buttonWidth));

        area.removeFromTop(kRowSpacing);
        modulationSummaryLabel.setBounds(area.removeFromTop(kLabelHeight));
        area.removeFromTop(kRowSpacing);

        for (auto& label : modulationConnectionLabels)
        {
            label.setBounds(area.removeFromTop(rowHeight));
            area.removeFromTop(4);
        }

        y = modulationGroup.getBottom() + kGroupSpacing;
    }

    if (memoryGroup.isVisible())
    {
        y = layoutGroup(memoryGroup, y, 4,
        {
            entry(memoryControl),
            entry(memoryDepthControl),
            entry(memoryDecayControl),
            entry(memoryDriftControl)
        });
    }

    if (physicalGroup.isVisible())
    {
        y = layoutGroup(physicalGroup, y, 4,
        {
            entry(tubeCountControl),
            entry(radiusVariationControl),
            entry(metallicResonanceControl),
            entry(couplingStrengthControl),
            entry(elasticityControl),
            entry(recoveryTimeControl),
            entry(absorptionDriftControl),
            entry(nonlinearityControl),
            entry(impossibilityDegreeControl),
            entry(pitchEvolutionRateControl),
            entry(paradoxResonanceFreqControl),
            entry(paradoxGainControl)
        });
    }

    if (timelineGroup.isVisible())
    {
        y = layoutGroup(timelineGroup, y, 2,
        {
            entry(timelineEnabledControl),
            entry(timelinePresetControl)
        });
    }

    if (safetyGroup.isVisible())
    {
        y = layoutGroup(safetyGroup, y, 2,
        {
            entry(safetyClipControl),
            entry(safetyClipDriveControl)
        });
    }

    if (diagnosticsGroup.isVisible())
    {
        const int groupHeight = kGroupPadding * 2 + kSmallControlHeight;
        diagnosticsGroup.setBounds(kMargin, y, contentWidth - kMargin * 2, groupHeight);
        auto area = diagnosticsGroup.getBounds().reduced(kGroupPadding);
        const int cellWidth = (area.getWidth() - kColSpacing) / 2;
        inputLevelLabel.setBounds(area.getX(), area.getY(), cellWidth, kSmallControlHeight);
        outputLevelLabel.setBounds(area.getX() + cellWidth + kColSpacing, area.getY(), cellWidth, kSmallControlHeight);
        y = diagnosticsGroup.getBottom() + kGroupSpacing;
    }

    controlsContent.setSize(contentWidth, y + kMargin);
}

void MonumentAudioProcessorEditor::updateDebugVisibility()
{
    updateSectionVisibility();
}

void MonumentAudioProcessorEditor::updateSectionVisibility()
{
    const bool showBase = activeSection == SectionView::BaseParams;
    const bool showModulation = activeSection == SectionView::Modulation;
    const bool showTimeline = activeSection == SectionView::Timeline;

    macroModeGroup.setVisible(showBase);
    coreGroup.setVisible(showBase);
    routingGroup.setVisible(showBase);
    setControlsVisible({entry(macroModeControl)}, showBase);
    setControlsVisible(
        {
            entry(mixControl),
            entry(timeControl),
            entry(massControl),
            entry(densityControl),
            entry(bloomControl),
            entry(airControl),
            entry(widthControl),
            entry(warpControl),
            entry(driftControl),
            entry(gravityControl),
            entry(pillarShapeControl)
        },
        showBase);
    setControlsVisible(
        {
            entry(routingPresetControl),
            entry(pillarModeControl),
            entry(freezeControl)
        },
        showBase);

    updateMacroModeVisibility(showBase);

    modulationGroup.setVisible(showModulation);
    setComponentsVisible(
        {
            &modulationSummaryLabel,
            &modulationSparseButton,
            &modulationDenseButton,
            &modulationClearButton
        },
        showModulation);
    for (auto& label : modulationConnectionLabels)
        label.setVisible(showModulation);

    timelineGroup.setVisible(showTimeline);
    setControlsVisible(
        {
            entry(timelineEnabledControl),
            entry(timelinePresetControl)
        },
        showTimeline);

    const bool showDebugGroups = debugMode && showBase;
    memoryGroup.setVisible(showDebugGroups);
    physicalGroup.setVisible(showDebugGroups);
    safetyGroup.setVisible(showDebugGroups);
    diagnosticsGroup.setVisible(showDebugGroups);
    inputLevelLabel.setVisible(showDebugGroups);
    outputLevelLabel.setVisible(showDebugGroups);

    setControlsVisible(
        {
            entry(memoryControl),
            entry(memoryDepthControl),
            entry(memoryDecayControl),
            entry(memoryDriftControl)
        },
        showDebugGroups);

    setControlsVisible(
        {
            entry(tubeCountControl),
            entry(radiusVariationControl),
            entry(metallicResonanceControl),
            entry(couplingStrengthControl),
            entry(elasticityControl),
            entry(recoveryTimeControl),
            entry(absorptionDriftControl),
            entry(nonlinearityControl),
            entry(impossibilityDegreeControl),
            entry(pitchEvolutionRateControl),
            entry(paradoxResonanceFreqControl),
            entry(paradoxGainControl)
        },
        showDebugGroups);

    setControlsVisible(
        {
            entry(safetyClipControl),
            entry(safetyClipDriveControl)
        },
        showDebugGroups);

    if (showModulation)
        updateModulationLabels();
}

void MonumentAudioProcessorEditor::updateMacroModeVisibility(bool allowShow)
{
    const auto* macroModeParam = processorRef.getAPVTS().getRawParameterValue("macroMode");
    const float macroModeValue = macroModeParam ? macroModeParam->load(std::memory_order_relaxed) : 0.0f;
    const bool expressiveMode = macroModeValue >= 0.5f;

    const bool showAncient = allowShow && (debugMode || !expressiveMode);
    const bool showExpressive = allowShow && (debugMode || expressiveMode);

    ancientMacroGroup.setVisible(showAncient);
    expressiveMacroGroup.setVisible(showExpressive);

    ancientMacroGroup.setEnabled(allowShow && !expressiveMode);
    expressiveMacroGroup.setEnabled(allowShow && expressiveMode);

    setControlsVisible(
        {
            entry(materialControl),
            entry(topologyControl),
            entry(viscosityControl),
            entry(evolutionControl),
            entry(chaosControl),
            entry(elasticityDecayControl),
            entry(patinaControl),
            entry(abyssControl),
            entry(coronaControl),
            entry(breathControl)
        },
        showAncient);

    setControlsVisible(
        {
            entry(characterControl),
            entry(spaceTypeControl),
            entry(energyControl),
            entry(motionControl),
            entry(colorControl),
            entry(dimensionControl)
        },
        showExpressive);
}

void MonumentAudioProcessorEditor::updateModulationLabels()
{
    const auto connections = processorRef.getModulationMatrix().getConnections();
    std::vector<monument::dsp::ModulationMatrix::Connection> activeConnections;
    activeConnections.reserve(connections.size());

    for (const auto& connection : connections)
    {
        if (connection.enabled)
            activeConnections.push_back(connection);
    }

    const int total = static_cast<int>(activeConnections.size());
    const int maxRows = static_cast<int>(modulationConnectionLabels.size());
    const int rowsForConnections = total > maxRows ? maxRows - 1 : total;

    modulationSummaryLabel.setText("Active connections: " + juce::String(total),
                                   juce::dontSendNotification);

    for (int i = 0; i < maxRows; ++i)
        modulationConnectionLabels[i].setText("", juce::dontSendNotification);

    for (int i = 0; i < rowsForConnections; ++i)
    {
        const auto& connection = activeConnections[static_cast<size_t>(i)];
        const auto sourceLabel = modulationSourceToString(connection.source);
        const auto destLabel = modulationDestinationToString(connection.destination);
        const auto depth = juce::String(connection.depth, 2);
        const auto smoothMs = juce::String(connection.smoothingMs, 0);
        const juce::String line = sourceLabel + " -> " + destLabel
            + " depth " + depth + " smooth " + smoothMs + "ms";
        modulationConnectionLabels[static_cast<size_t>(i)].setText(line, juce::dontSendNotification);
    }

    if (total > maxRows)
    {
        const int remaining = total - rowsForConnections;
        modulationConnectionLabels[static_cast<size_t>(maxRows - 1)].setText(
            "... +" + juce::String(remaining) + " more",
            juce::dontSendNotification);
    }
}
