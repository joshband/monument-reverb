#include "PluginEditor.h"

MonumentAudioProcessorEditor::MonumentAudioProcessorEditor(MonumentAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // Set editor size
    setSize(800, 600);

    // Configure title label
    titleLabel.setText("MONUMENT REVERB", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(24.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);

    // Helper lambda to configure slider and label
    auto setupControl = [this](juce::Slider& slider, juce::Label& label,
                               const juce::String& labelText,
                               const juce::String& paramID) {
        // Configure slider
        slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
        slider.setColour(juce::Slider::thumbColourId, juce::Colours::white);
        slider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colours::lightblue);
        slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colours::darkgrey);
        addAndMakeVisible(slider);

        // Configure label
        label.setText(labelText, juce::dontSendNotification);
        label.setFont(juce::Font(14.0f));
        label.setJustificationType(juce::Justification::centred);
        label.setColour(juce::Label::textColourId, juce::Colours::white);
        addAndMakeVisible(label);
    };

    // Setup all macro controls
    setupControl(materialSlider, materialLabel, "Material", "material");
    setupControl(topologySlider, topologyLabel, "Topology", "topology");
    setupControl(viscositySlider, viscosityLabel, "Viscosity", "viscosity");
    setupControl(evolutionSlider, evolutionLabel, "Evolution", "evolution");
    setupControl(chaosSlider, chaosLabel, "Chaos", "chaosIntensity");
    setupControl(elasticitySlider, elasticityLabel, "Elasticity", "elasticityDecay");
    setupControl(patinaSlider, patinaLabel, "Patina", "patina");
    setupControl(abyssSlider, abyssLabel, "Abyss", "abyss");
    setupControl(coronaSlider, coronaLabel, "Corona", "corona");
    setupControl(breathSlider, breathLabel, "Breath", "breath");

    // Create parameter attachments
    auto& apvts = processorRef.getAPVTS();
    materialAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "material", materialSlider);
    topologyAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "topology", topologySlider);
    viscosityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "viscosity", viscositySlider);
    evolutionAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "evolution", evolutionSlider);
    chaosAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "chaosIntensity", chaosSlider);
    elasticityAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "elasticityDecay", elasticitySlider);
    patinaAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "patina", patinaSlider);
    abyssAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "abyss", abyssSlider);
    coronaAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "corona", coronaSlider);
    breathAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(apvts, "breath", breathSlider);
}

MonumentAudioProcessorEditor::~MonumentAudioProcessorEditor()
{
}

void MonumentAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Dark background
    g.fillAll(juce::Colour(0xff1a1a1a));

    // Draw subtle border
    g.setColour(juce::Colours::darkgrey);
    g.drawRect(getLocalBounds(), 2);
}

void MonumentAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();

    // Title area at top
    auto titleArea = bounds.removeFromTop(60);
    titleLabel.setBounds(titleArea.reduced(10));

    // Remaining area for controls
    auto controlArea = bounds.reduced(20);

    // Create 2 rows of 5 controls each
    const int numRows = 2;
    const int numCols = 5;
    const int knobSize = 100;
    const int knobSpacing = 20;
    const int labelHeight = 20;

    // Calculate starting position to center the grid
    const int totalWidth = numCols * knobSize + (numCols - 1) * knobSpacing;
    const int totalHeight = numRows * (knobSize + labelHeight + knobSpacing);
    const int startX = (controlArea.getWidth() - totalWidth) / 2;
    const int startY = (controlArea.getHeight() - totalHeight) / 2;

    // Helper to position control
    auto positionControl = [&](juce::Slider& slider, juce::Label& label, int row, int col) {
        int x = controlArea.getX() + startX + col * (knobSize + knobSpacing);
        int y = controlArea.getY() + startY + row * (knobSize + labelHeight + knobSpacing);

        label.setBounds(x, y, knobSize, labelHeight);
        slider.setBounds(x, y + labelHeight, knobSize, knobSize);
    };

    // Position all controls in grid (2 rows × 5 columns)
    // Row 1
    positionControl(materialSlider, materialLabel, 0, 0);
    positionControl(topologySlider, topologyLabel, 0, 1);
    positionControl(viscositySlider, viscosityLabel, 0, 2);
    positionControl(evolutionSlider, evolutionLabel, 0, 3);
    positionControl(chaosSlider, chaosLabel, 0, 4);

    // Row 2
    positionControl(elasticitySlider, elasticityLabel, 1, 0);
    positionControl(patinaSlider, patinaLabel, 1, 1);
    positionControl(abyssSlider, abyssLabel, 1, 2);
    positionControl(coronaSlider, coronaLabel, 1, 3);
    positionControl(breathSlider, breathLabel, 1, 4);
}
