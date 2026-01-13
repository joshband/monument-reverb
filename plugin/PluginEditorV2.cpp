#include "PluginEditorV2.h"
#include "dsp/ModulationMatrix.h"
#include "dsp/SequencePresets.h"

#include <cstdlib>
#include <optional>
#include <vector>

namespace
{
constexpr int kHeaderHeight = 92;
constexpr int kMargin = 16;
constexpr int kGroupPadding = 12;
constexpr int kGroupSpacing = 18;
constexpr int kLabelHeight = 18;
constexpr int kControlHeight = 90;
constexpr int kSmallControlHeight = 24;
constexpr int kRowSpacing = 12;
constexpr int kColSpacing = 12;
constexpr int kMeterUpdateHz = 20;
constexpr int kTabRowHeight = 26;
constexpr int kGroupHeaderHeight = 30;
constexpr int kGroupToggleWidth = 36;
constexpr int kGroupToggleHeight = 18;
constexpr int kEditorWidth = 1100;
constexpr int kEditorHeight = 820;
constexpr int kSectionButtonGroup = 2401;
constexpr int kModulationRefreshTicks = 5;

const auto kBackgroundTop = juce::Colour(0xff111315);
const auto kBackgroundBottom = juce::Colour(0xff1a1d20);
const auto kBorder = juce::Colour(0xff2d3034);
const auto kAccentCore = juce::Colour(0xff6db7ff);
const auto kAccentMacro = juce::Colour(0xffd0a36b);
const auto kAccentExpressive = juce::Colour(0xff7ed0ff);
const auto kAccentPhysical = juce::Colour(0xff7bd8b8);
const auto kAccentSafety = juce::Colour(0xffef7c7c);

struct LayeredKnobAssets
{
    juce::Image plate;
    juce::Image plateShadow;
    juce::Image knob;
    juce::Image highlight;
    juce::Image shadow;
    juce::Image indicator;
    juce::String variant;
    bool indicatorOnly{false};
};

juce::File findKnobRootFromBase(juce::File base)
{
    const juce::StringArray candidates{"assets/ui/archive", "assets/ui/line6"};
    for (int depth = 0; depth < 8; ++depth)
    {
        for (const auto& candidatePath : candidates)
        {
            auto candidate = base.getChildFile(candidatePath);
            if (candidate.exists())
                return candidate;
        }

        auto parent = base.getParentDirectory();
        if (parent == base)
            break;
        base = parent;
    }
    return {};
}

juce::File findKnobRoot()
{
    if (const char* env = std::getenv("MONUMENT_KNOB_DIR"))
    {
        auto envString = juce::String::fromUTF8(env);
        juce::File direct(envString);
        if (direct.exists())
            return direct;
    }

    if (const char* env = std::getenv("MONUMENT_LINE6_DIR"))
    {
        auto envString = juce::String::fromUTF8(env);
        juce::File direct(envString);
        if (direct.exists())
            return direct;
    }

    if (const char* env = std::getenv("MONUMENT_ASSETS_DIR"))
    {
        auto envString = juce::String::fromUTF8(env);
        juce::File base(envString);
        if (base.exists())
        {
            auto candidate = base.getChildFile("assets/ui/archive");
            if (candidate.exists())
                return candidate;
            candidate = base.getChildFile("assets/ui/line6");
            if (candidate.exists())
                return candidate;
            candidate = base.getChildFile("ui/archive");
            if (candidate.exists())
                return candidate;
            candidate = base.getChildFile("ui/line6");
            if (candidate.exists())
                return candidate;
        }
    }

    if (auto fromCwd = findKnobRootFromBase(juce::File::getCurrentWorkingDirectory());
        fromCwd.exists())
    {
        return fromCwd;
    }

    auto execBase = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();
    if (auto fromExec = findKnobRootFromBase(execBase); fromExec.exists())
        return fromExec;

    auto appBase = juce::File::getSpecialLocation(juce::File::currentApplicationFile).getParentDirectory();
    return findKnobRootFromBase(appBase);
}

std::optional<LayeredKnobAssets> loadLayeredKnobAssets()
{
    auto root = findKnobRoot();
    if (!root.exists())
        return std::nullopt;

    juce::String variant;
    if (const char* env = std::getenv("MONUMENT_KNOB_VARIANT"))
        if (*env != '\0')
            variant = juce::String::fromUTF8(env);
    if (variant.isEmpty())
    {
        if (const char* env = std::getenv("MONUMENT_LINE6_KNOB"))
            if (*env != '\0')
                variant = juce::String::fromUTF8(env);
    }
    if (variant.isEmpty())
    {
        if (root.getChildFile("archive_brass_precision").isDirectory())
            variant = "archive_brass_precision";
        else
            variant = "line6_brass_alt";
    }

    auto folder = root.getChildFile(variant);
    if (!folder.isDirectory())
        return std::nullopt;

    const auto plateFile = folder.getChildFile(variant + "_plate.png");
    const auto plateShadowFile = folder.getChildFile(variant + "_plate_shadow.png");
    const auto knobFile = folder.getChildFile(variant + "_knob.png");
    const auto highlightFile = folder.getChildFile(variant + "_highlight.png");
    const auto shadowFile = folder.getChildFile(variant + "_shadow.png");
    const auto indicatorFile = folder.getChildFile(variant + "_indicator.png");
    if (!plateFile.existsAsFile() || !knobFile.existsAsFile())
        return std::nullopt;

    auto plateImage = juce::ImageFileFormat::loadFrom(plateFile);
    juce::Image plateShadowImage;
    auto knobImage = juce::ImageFileFormat::loadFrom(knobFile);
    if (!plateImage.isValid() || !knobImage.isValid())
        return std::nullopt;

    if (plateShadowFile.existsAsFile())
        plateShadowImage = juce::ImageFileFormat::loadFrom(plateShadowFile);

    juce::Image highlightImage;
    juce::Image shadowImage;
    juce::Image indicatorImage;
    if (highlightFile.existsAsFile())
        highlightImage = juce::ImageFileFormat::loadFrom(highlightFile);
    if (shadowFile.existsAsFile())
        shadowImage = juce::ImageFileFormat::loadFrom(shadowFile);
    if (indicatorFile.existsAsFile())
        indicatorImage = juce::ImageFileFormat::loadFrom(indicatorFile);

    bool indicatorOnly = variant.startsWithIgnoreCase("archive_");
    if (const char* env = std::getenv("MONUMENT_KNOB_ROTATION"))
    {
        juce::String mode = juce::String::fromUTF8(env).toLowerCase();
        if (mode == "indicator" || mode == "indicator_only")
            indicatorOnly = true;
        if (mode == "knob" || mode == "knob_and_indicator")
            indicatorOnly = false;
    }

    return LayeredKnobAssets{
        plateImage,
        plateShadowImage,
        knobImage,
        highlightImage,
        shadowImage,
        indicatorImage,
        variant,
        indicatorOnly,
    };
}

const LayeredKnobAssets* getLayeredKnobAssets()
{
    static const std::optional<LayeredKnobAssets> cached = loadLayeredKnobAssets();
    return cached ? &(*cached) : nullptr;
}

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
} // namespace

MonumentAudioProcessorEditorV2::MonumentAudioProcessorEditorV2(MonumentAudioProcessor& p)
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

    loadUiState();
    setupGroupToggle(macroModeToggle, macroModeExpanded);
    setupGroupToggle(ancientMacroToggle, ancientMacroExpanded);
    setupGroupToggle(expressiveMacroToggle, expressiveMacroExpanded);
    setupGroupToggle(coreToggle, coreExpanded);
    setupGroupToggle(routingToggle, routingExpanded);
    setupGroupToggle(modulationToggle, modulationExpanded);
    setupGroupToggle(memoryToggle, memoryExpanded);
    setupGroupToggle(physicalToggle, physicalExpanded);
    setupGroupToggle(timelineToggle, timelineExpanded);
    setupGroupToggle(safetyToggle, safetyExpanded);
    setupGroupToggle(diagnosticsToggle, diagnosticsExpanded);

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

    setupKnob(mixControl, "Mix", "mix", kAccentCore);
    setupKnob(timeControl, "Time", "time", kAccentCore);
    setupKnob(massControl, "Mass", "mass", kAccentCore);
    setupKnob(densityControl, "Density", "density", kAccentCore);
    setupKnob(bloomControl, "Bloom", "bloom", kAccentCore);
    setupKnob(airControl, "Air", "air", kAccentCore);
    setupKnob(widthControl, "Width", "width", kAccentCore);
    setupKnob(warpControl, "Warp", "warp", kAccentCore);
    setupKnob(driftControl, "Drift", "drift", kAccentCore);
    setupKnob(gravityControl, "Gravity", "gravity", kAccentCore);
    setupKnob(pillarShapeControl, "Pillar Shape", "pillarShape", kAccentCore);

    setupKnob(materialControl, "Material", "material", kAccentMacro);
    setupKnob(topologyControl, "Topology", "topology", kAccentMacro);
    setupKnob(viscosityControl, "Viscosity", "viscosity", kAccentMacro);
    setupKnob(evolutionControl, "Evolution", "evolution", kAccentMacro);
    setupKnob(chaosControl, "Chaos", "chaosIntensity", kAccentMacro);
    setupKnob(elasticityDecayControl, "Elasticity", "elasticityDecay", kAccentMacro);
    setupKnob(patinaControl, "Patina", "patina", kAccentMacro);
    setupKnob(abyssControl, "Abyss", "abyss", kAccentMacro);
    setupKnob(coronaControl, "Corona", "corona", kAccentMacro);
    setupKnob(breathControl, "Breath", "breath", kAccentMacro);

    setupKnob(characterControl, "Character", "character", kAccentExpressive);
    setupKnob(spaceTypeControl, "Space Type", "spaceType", kAccentExpressive);
    setupKnob(energyControl, "Energy", "energy", kAccentExpressive);
    setupKnob(motionControl, "Motion", "motion", kAccentExpressive);
    setupKnob(colorControl, "Color", "color", kAccentExpressive);
    setupKnob(dimensionControl, "Dimension", "dimension", kAccentExpressive);

    setupKnob(memoryControl, "Memory", "memory", kAccentPhysical);
    setupKnob(memoryDepthControl, "Memory Depth", "memoryDepth", kAccentPhysical);
    setupKnob(memoryDecayControl, "Memory Decay", "memoryDecay", kAccentPhysical);
    setupKnob(memoryDriftControl, "Memory Drift", "memoryDrift", kAccentPhysical);

    setupKnob(tubeCountControl, "Tube Count", "tubeCount", kAccentPhysical);
    setupKnob(radiusVariationControl, "Radius Variation", "radiusVariation", kAccentPhysical);
    setupKnob(metallicResonanceControl, "Metallic Resonance", "metallicResonance", kAccentPhysical);
    setupKnob(couplingStrengthControl, "Coupling Strength", "couplingStrength", kAccentPhysical);
    setupKnob(elasticityControl, "Elasticity", "elasticity", kAccentPhysical);
    setupKnob(recoveryTimeControl, "Recovery Time", "recoveryTime", kAccentPhysical);
    setupKnob(absorptionDriftControl, "Absorption Drift", "absorptionDrift", kAccentPhysical);
    setupKnob(nonlinearityControl, "Nonlinearity", "nonlinearity", kAccentPhysical);
    setupKnob(impossibilityDegreeControl, "Impossibility", "impossibilityDegree", kAccentPhysical);
    setupKnob(pitchEvolutionRateControl, "Pitch Evolution", "pitchEvolutionRate", kAccentPhysical);
    setupKnob(paradoxResonanceFreqControl, "Paradox Freq", "paradoxResonanceFreq", kAccentPhysical);
    setupKnob(paradoxGainControl, "Paradox Gain", "paradoxGain", kAccentPhysical);

    setupKnob(safetyClipDriveControl, "Safety Drive", "safetyClipDrive", kAccentSafety);

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
    persistUiState();
    startTimerHz(kMeterUpdateHz);
}

MonumentAudioProcessorEditorV2::~MonumentAudioProcessorEditorV2() = default;

void MonumentAudioProcessorEditorV2::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    juce::ColourGradient gradient(kBackgroundTop, 0.0f, 0.0f,
                                  kBackgroundBottom, 0.0f, bounds.getBottom(),
                                  false);
    g.setGradientFill(gradient);
    g.fillAll();

    juce::ColourGradient glow(juce::Colour(0x2200b0ff), bounds.getCentreX(), bounds.getCentreY(),
                              juce::Colours::transparentBlack, bounds.getCentreX(),
                              bounds.getCentreY() + bounds.getHeight() * 0.6f, true);
    g.setGradientFill(glow);
    g.fillRect(bounds);

    g.setColour(kBorder);
    g.drawRect(getLocalBounds(), 2);
}

void MonumentAudioProcessorEditorV2::resized()
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

void MonumentAudioProcessorEditorV2::timerCallback()
{
    const auto inputDb = juce::Decibels::gainToDecibels(processorRef.getInputLevel(), -80.0f);
    const auto outputDb = juce::Decibels::gainToDecibels(processorRef.getOutputLevel(), -80.0f);
    inputLevelLabel.setText("Input: " + juce::String(inputDb, 1) + " dB", juce::dontSendNotification);
    outputLevelLabel.setText("Output: " + juce::String(outputDb, 1) + " dB", juce::dontSendNotification);

    if (modulationGroup.isVisible() && modulationExpanded)
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

void MonumentAudioProcessorEditorV2::setupKnob(LabeledKnob& control,
                                               const juce::String& labelText,
                                               const juce::String& paramId,
                                               juce::Colour ledColor)
{
    control.knob.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
    control.knob.setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    control.knob.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff404040));
    control.knob.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff1b1e21));
    if (const auto* assets = getLayeredKnobAssets())
    {
        control.knob.setLayerImages(assets->plate, assets->knob);
        if (assets->plateShadow.isValid())
            control.knob.setPlateShadowImage(assets->plateShadow);
        control.knob.setOverlayImages(assets->highlight, assets->shadow);
        if (assets->indicator.isValid())
            control.knob.setIndicatorImage(assets->indicator);
        control.knob.setRotationMode(
            assets->indicatorOnly
                ? monument::PhotorealisticKnob::RotationMode::IndicatorOnly
                : monument::PhotorealisticKnob::RotationMode::KnobAndIndicator);
        control.knob.setLEDRingEnabled(false);
    }
    else
    {
        control.knob.setLEDRingEnabled(true);
        control.knob.setLEDRingColor(ledColor);
    }
    controlsContent.addAndMakeVisible(control.knob);

    control.label.setText(labelText, juce::dontSendNotification);
    control.label.setJustificationType(juce::Justification::centred);
    control.label.setColour(juce::Label::textColourId, juce::Colours::white);
    control.label.setInterceptsMouseClicks(false, false);
    controlsContent.addAndMakeVisible(control.label);

    control.attachment = std::make_unique<SliderAttachment>(processorRef.getAPVTS(), paramId, control.knob);
}

void MonumentAudioProcessorEditorV2::setupCombo(LabeledCombo& control,
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
    control.label.setInterceptsMouseClicks(false, false);
    controlsContent.addAndMakeVisible(control.label);

    control.attachment = std::make_unique<ComboBoxAttachment>(processorRef.getAPVTS(), paramId, control.combo);
}

void MonumentAudioProcessorEditorV2::setupToggle(LabeledToggle& control,
                                                 const juce::String& labelText,
                                                 const juce::String& paramId)
{
    control.toggle.setClickingTogglesState(true);
    controlsContent.addAndMakeVisible(control.toggle);

    control.label.setText(labelText, juce::dontSendNotification);
    control.label.setJustificationType(juce::Justification::centred);
    control.label.setColour(juce::Label::textColourId, juce::Colours::white);
    control.label.setInterceptsMouseClicks(false, false);
    controlsContent.addAndMakeVisible(control.label);

    control.attachment = std::make_unique<ButtonAttachment>(processorRef.getAPVTS(), paramId, control.toggle);
}

void MonumentAudioProcessorEditorV2::setupGroupToggle(juce::TextButton& toggle, bool& expanded)
{
    toggle.setClickingTogglesState(false);
    toggle.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2a));
    toggle.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff3a3a3a));
    toggle.setColour(juce::TextButton::textColourOffId, juce::Colours::lightgrey);
    toggle.setColour(juce::TextButton::textColourOnId, juce::Colours::white);
    updateGroupToggle(toggle, expanded);
    toggle.onClick = [this, &expanded, &toggle]
    {
        expanded = !expanded;
        updateGroupToggle(toggle, expanded);
        persistUiState();
        layoutControls();
    };
    controlsContent.addAndMakeVisible(toggle);
}

void MonumentAudioProcessorEditorV2::updateGroupToggle(juce::TextButton& toggle, bool expanded)
{
    toggle.setButtonText(expanded ? "[-]" : "[+]");
    toggle.setTooltip(expanded ? "Collapse" : "Expand");
}

int MonumentAudioProcessorEditorV2::layoutCollapsedGroup(juce::GroupComponent& group,
                                                         juce::TextButton& toggle,
                                                         int y)
{
    const int contentWidth = controlsContent.getWidth() - kMargin * 2;
    group.setBounds(kMargin, y, contentWidth, kGroupHeaderHeight);
    positionGroupToggle(group, toggle);
    return group.getBottom() + kGroupSpacing;
}

void MonumentAudioProcessorEditorV2::positionGroupToggle(juce::GroupComponent& group, juce::TextButton& toggle)
{
    if (!group.isVisible())
    {
        toggle.setVisible(false);
        return;
    }

    toggle.setVisible(true);
    const int toggleX = group.getRight() - kGroupToggleWidth - kGroupPadding;
    const int toggleY = group.getY() + (kGroupHeaderHeight - kGroupToggleHeight) / 2;
    toggle.setBounds(toggleX, toggleY, kGroupToggleWidth, kGroupToggleHeight);
    toggle.toFront(false);
}

void MonumentAudioProcessorEditorV2::loadUiState()
{
    auto& root = processorRef.getAPVTS().state;
    auto uiState = root.getChildWithName("ui");
    if (!uiState.isValid())
        return;

    auto readBool = [&uiState](const juce::Identifier& key, bool fallback)
    {
        if (!uiState.hasProperty(key))
            return fallback;
        return static_cast<bool>(uiState.getProperty(key));
    };

    macroModeExpanded = readBool("macroModeExpanded", macroModeExpanded);
    ancientMacroExpanded = readBool("ancientMacroExpanded", ancientMacroExpanded);
    expressiveMacroExpanded = readBool("expressiveMacroExpanded", expressiveMacroExpanded);
    coreExpanded = readBool("coreExpanded", coreExpanded);
    routingExpanded = readBool("routingExpanded", routingExpanded);
    modulationExpanded = readBool("modulationExpanded", modulationExpanded);
    memoryExpanded = readBool("memoryExpanded", memoryExpanded);
    physicalExpanded = readBool("physicalExpanded", physicalExpanded);
    timelineExpanded = readBool("timelineExpanded", timelineExpanded);
    safetyExpanded = readBool("safetyExpanded", safetyExpanded);
    diagnosticsExpanded = readBool("diagnosticsExpanded", diagnosticsExpanded);
}

void MonumentAudioProcessorEditorV2::persistUiState()
{
    auto& root = processorRef.getAPVTS().state;
    auto uiState = root.getChildWithName("ui");
    if (!uiState.isValid())
    {
        uiState = juce::ValueTree("ui");
        root.addChild(uiState, -1, nullptr);
    }

    uiState.setProperty("macroModeExpanded", macroModeExpanded, nullptr);
    uiState.setProperty("ancientMacroExpanded", ancientMacroExpanded, nullptr);
    uiState.setProperty("expressiveMacroExpanded", expressiveMacroExpanded, nullptr);
    uiState.setProperty("coreExpanded", coreExpanded, nullptr);
    uiState.setProperty("routingExpanded", routingExpanded, nullptr);
    uiState.setProperty("modulationExpanded", modulationExpanded, nullptr);
    uiState.setProperty("memoryExpanded", memoryExpanded, nullptr);
    uiState.setProperty("physicalExpanded", physicalExpanded, nullptr);
    uiState.setProperty("timelineExpanded", timelineExpanded, nullptr);
    uiState.setProperty("safetyExpanded", safetyExpanded, nullptr);
    uiState.setProperty("diagnosticsExpanded", diagnosticsExpanded, nullptr);
}

MonumentAudioProcessorEditorV2::ControlEntry MonumentAudioProcessorEditorV2::entry(LabeledKnob& control) const
{
    return {&control.knob, &control.label, kControlHeight, kControlHeight};
}

MonumentAudioProcessorEditorV2::ControlEntry MonumentAudioProcessorEditorV2::entry(LabeledCombo& control) const
{
    return {&control.combo, &control.label, kSmallControlHeight, 0};
}

MonumentAudioProcessorEditorV2::ControlEntry MonumentAudioProcessorEditorV2::entry(LabeledToggle& control) const
{
    return {&control.toggle, &control.label, kSmallControlHeight, 0};
}

int MonumentAudioProcessorEditorV2::layoutGroup(juce::GroupComponent& group,
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

void MonumentAudioProcessorEditorV2::setControlsVisible(std::initializer_list<ControlEntry> controls, bool visible)
{
    for (const auto& entry : controls)
    {
        if (entry.control)
            entry.control->setVisible(visible);
        if (entry.label)
            entry.label->setVisible(visible);
    }
}

void MonumentAudioProcessorEditorV2::setComponentsVisible(std::initializer_list<juce::Component*> components,
                                                         bool visible)
{
    for (auto* component : components)
    {
        if (component != nullptr)
            component->setVisible(visible);
    }
}

void MonumentAudioProcessorEditorV2::setActiveSection(SectionView view)
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

void MonumentAudioProcessorEditorV2::layoutControls()
{
    const int contentWidth = controlsViewport.getWidth();
    if (contentWidth <= 0)
        return;

    updateSectionVisibility();

    controlsContent.setBounds(0, 0, contentWidth, 0);

    int y = kMargin;

    if (macroModeGroup.isVisible())
    {
        if (macroModeExpanded)
            y = layoutGroup(macroModeGroup, y, 1, {entry(macroModeControl)});
        else
            y = layoutCollapsedGroup(macroModeGroup, macroModeToggle, y);
        if (macroModeExpanded)
            positionGroupToggle(macroModeGroup, macroModeToggle);
    }

    if (ancientMacroGroup.isVisible())
    {
        if (ancientMacroExpanded)
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
            positionGroupToggle(ancientMacroGroup, ancientMacroToggle);
        }
        else
        {
            y = layoutCollapsedGroup(ancientMacroGroup, ancientMacroToggle, y);
        }
    }

    if (expressiveMacroGroup.isVisible())
    {
        if (expressiveMacroExpanded)
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
            positionGroupToggle(expressiveMacroGroup, expressiveMacroToggle);
        }
        else
        {
            y = layoutCollapsedGroup(expressiveMacroGroup, expressiveMacroToggle, y);
        }
    }

    if (coreGroup.isVisible())
    {
        if (coreExpanded)
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
            positionGroupToggle(coreGroup, coreToggle);
        }
        else
        {
            y = layoutCollapsedGroup(coreGroup, coreToggle, y);
        }
    }

    if (routingGroup.isVisible())
    {
        if (routingExpanded)
        {
            y = layoutGroup(routingGroup, y, 3,
            {
                entry(routingPresetControl),
                entry(pillarModeControl),
                entry(freezeControl)
            });
            positionGroupToggle(routingGroup, routingToggle);
        }
        else
        {
            y = layoutCollapsedGroup(routingGroup, routingToggle, y);
        }
    }

    if (modulationGroup.isVisible())
    {
        if (modulationExpanded)
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

            positionGroupToggle(modulationGroup, modulationToggle);
            y = modulationGroup.getBottom() + kGroupSpacing;
        }
        else
        {
            y = layoutCollapsedGroup(modulationGroup, modulationToggle, y);
        }
    }

    if (memoryGroup.isVisible())
    {
        if (memoryExpanded)
        {
            y = layoutGroup(memoryGroup, y, 4,
            {
                entry(memoryControl),
                entry(memoryDepthControl),
                entry(memoryDecayControl),
                entry(memoryDriftControl)
            });
            positionGroupToggle(memoryGroup, memoryToggle);
        }
        else
        {
            y = layoutCollapsedGroup(memoryGroup, memoryToggle, y);
        }
    }

    if (physicalGroup.isVisible())
    {
        if (physicalExpanded)
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
            positionGroupToggle(physicalGroup, physicalToggle);
        }
        else
        {
            y = layoutCollapsedGroup(physicalGroup, physicalToggle, y);
        }
    }

    if (timelineGroup.isVisible())
    {
        if (timelineExpanded)
        {
            y = layoutGroup(timelineGroup, y, 2,
            {
                entry(timelineEnabledControl),
                entry(timelinePresetControl)
            });
            positionGroupToggle(timelineGroup, timelineToggle);
        }
        else
        {
            y = layoutCollapsedGroup(timelineGroup, timelineToggle, y);
        }
    }

    if (safetyGroup.isVisible())
    {
        if (safetyExpanded)
        {
            y = layoutGroup(safetyGroup, y, 2,
            {
                entry(safetyClipControl),
                entry(safetyClipDriveControl)
            });
            positionGroupToggle(safetyGroup, safetyToggle);
        }
        else
        {
            y = layoutCollapsedGroup(safetyGroup, safetyToggle, y);
        }
    }

    if (diagnosticsGroup.isVisible())
    {
        if (diagnosticsExpanded)
        {
            const int groupHeight = kGroupPadding * 2 + kSmallControlHeight;
            diagnosticsGroup.setBounds(kMargin, y, contentWidth - kMargin * 2, groupHeight);
            auto area = diagnosticsGroup.getBounds().reduced(kGroupPadding);
            const int cellWidth = (area.getWidth() - kColSpacing) / 2;
            inputLevelLabel.setBounds(area.getX(), area.getY(), cellWidth, kSmallControlHeight);
            outputLevelLabel.setBounds(area.getX() + cellWidth + kColSpacing, area.getY(), cellWidth, kSmallControlHeight);
            positionGroupToggle(diagnosticsGroup, diagnosticsToggle);
            y = diagnosticsGroup.getBottom() + kGroupSpacing;
        }
        else
        {
            y = layoutCollapsedGroup(diagnosticsGroup, diagnosticsToggle, y);
        }
    }

    controlsContent.setSize(contentWidth, y + kMargin);
}

void MonumentAudioProcessorEditorV2::updateSectionVisibility()
{
    const bool showBase = activeSection == SectionView::BaseParams;
    const bool showModulation = activeSection == SectionView::Modulation;
    const bool showTimeline = activeSection == SectionView::Timeline;

    macroModeGroup.setVisible(showBase);
    macroModeToggle.setVisible(showBase);
    coreGroup.setVisible(showBase);
    coreToggle.setVisible(showBase);
    routingGroup.setVisible(showBase);
    routingToggle.setVisible(showBase);
    setControlsVisible({entry(macroModeControl)}, showBase && macroModeExpanded);
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
        showBase && coreExpanded);
    setControlsVisible(
        {
            entry(routingPresetControl),
            entry(pillarModeControl),
            entry(freezeControl)
        },
        showBase && routingExpanded);

    updateMacroModeVisibility(showBase);

    modulationGroup.setVisible(showModulation);
    modulationToggle.setVisible(showModulation);
    setComponentsVisible(
        {
            &modulationSummaryLabel,
            &modulationSparseButton,
            &modulationDenseButton,
            &modulationClearButton
        },
        showModulation && modulationExpanded);
    for (auto& label : modulationConnectionLabels)
        label.setVisible(showModulation && modulationExpanded);

    timelineGroup.setVisible(showTimeline);
    timelineToggle.setVisible(showTimeline);
    setControlsVisible(
        {
            entry(timelineEnabledControl),
            entry(timelinePresetControl)
        },
        showTimeline && timelineExpanded);

    const bool showDebugGroups = debugMode && showBase;
    memoryGroup.setVisible(showDebugGroups);
    physicalGroup.setVisible(showDebugGroups);
    safetyGroup.setVisible(showDebugGroups);
    diagnosticsGroup.setVisible(showDebugGroups);
    memoryToggle.setVisible(showDebugGroups);
    physicalToggle.setVisible(showDebugGroups);
    safetyToggle.setVisible(showDebugGroups);
    diagnosticsToggle.setVisible(showDebugGroups);
    inputLevelLabel.setVisible(showDebugGroups);
    outputLevelLabel.setVisible(showDebugGroups);

    setControlsVisible(
        {
            entry(memoryControl),
            entry(memoryDepthControl),
            entry(memoryDecayControl),
            entry(memoryDriftControl)
        },
        showDebugGroups && memoryExpanded);

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
        showDebugGroups && physicalExpanded);

    setControlsVisible(
        {
            entry(safetyClipControl),
            entry(safetyClipDriveControl)
        },
        showDebugGroups && safetyExpanded);

    if (showModulation && modulationExpanded)
        updateModulationLabels();
}

void MonumentAudioProcessorEditorV2::updateMacroModeVisibility(bool allowShow)
{
    const auto* macroModeParam = processorRef.getAPVTS().getRawParameterValue("macroMode");
    const float macroModeValue = macroModeParam ? macroModeParam->load(std::memory_order_relaxed) : 0.0f;
    const bool expressiveMode = macroModeValue >= 0.5f;

    const bool showAncient = allowShow && (debugMode || !expressiveMode);
    const bool showExpressive = allowShow && (debugMode || expressiveMode);

    ancientMacroGroup.setVisible(showAncient);
    ancientMacroToggle.setVisible(showAncient);
    expressiveMacroGroup.setVisible(showExpressive);
    expressiveMacroToggle.setVisible(showExpressive);

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
        showAncient && ancientMacroExpanded);

    setControlsVisible(
        {
            entry(characterControl),
            entry(spaceTypeControl),
            entry(energyControl),
            entry(motionControl),
            entry(colorControl),
            entry(dimensionControl)
        },
        showExpressive && expressiveMacroExpanded);
}

void MonumentAudioProcessorEditorV2::updateModulationLabels()
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
        modulationConnectionLabels[static_cast<size_t>(i)].setText("", juce::dontSendNotification);

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
