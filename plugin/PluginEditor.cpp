#include "PluginEditor.h"

MonumentAudioProcessorEditor::MonumentAudioProcessorEditor(MonumentAudioProcessor& p)
    : juce::AudioProcessorEditor(&p),
      processorRef(p),
      // Macro Controls
      materialKnob(processorRef.getAPVTS(), "material", "Material"),
      topologyKnob(processorRef.getAPVTS(), "topology", "Topology"),
      viscosityKnob(processorRef.getAPVTS(), "viscosity", "Viscosity"),
      evolutionKnob(processorRef.getAPVTS(), "evolution", "Evolution"),
      chaosKnob(processorRef.getAPVTS(), "chaos", "Chaos"),
      elasticityKnob(processorRef.getAPVTS(), "elasticity", "Elasticity"),
      // Base Parameters
      mixKnob(processorRef.getAPVTS(), "mix", "Mix"),
      timeKnob(processorRef.getAPVTS(), "time", "Time"),
      massKnob(processorRef.getAPVTS(), "mass", "Mass"),
      densityKnob(processorRef.getAPVTS(), "density", "Density"),
      bloomKnob(processorRef.getAPVTS(), "bloom", "Bloom"),
      airKnob(processorRef.getAPVTS(), "air", "Air"),
      widthKnob(processorRef.getAPVTS(), "width", "Width"),
      warpKnob(processorRef.getAPVTS(), "warp", "Warp"),
      driftKnob(processorRef.getAPVTS(), "drift", "Drift"),
      gravityKnob(processorRef.getAPVTS(), "gravity", "Gravity"),
      freezeToggle(processorRef.getAPVTS(), "freeze", "Freeze")
{
    // Add macro controls (primary interface)
    addAndMakeVisible(materialKnob);
    addAndMakeVisible(topologyKnob);
    addAndMakeVisible(viscosityKnob);
    addAndMakeVisible(evolutionKnob);
    addAndMakeVisible(chaosKnob);
    addAndMakeVisible(elasticityKnob);

    // Add base parameters
    addAndMakeVisible(mixKnob);
    addAndMakeVisible(timeKnob);
    addAndMakeVisible(massKnob);
    addAndMakeVisible(densityKnob);
    addAndMakeVisible(bloomKnob);
    addAndMakeVisible(airKnob);
    addAndMakeVisible(widthKnob);
    addAndMakeVisible(warpKnob);
    addAndMakeVisible(driftKnob);
    addAndMakeVisible(gravityKnob);
    addAndMakeVisible(freezeToggle);
    addAndMakeVisible(presetBox);

    presetBox.setTextWhenNothingSelected("Presets");
    presetBox.setJustificationType(juce::Justification::centred);
    presetBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff14171b));
    presetBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xffe6e1d6));
    presetBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff3a3f46));
    presetBox.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xffe6e1d6));
    presetBox.setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff14171b));
    presetBox.setColour(juce::PopupMenu::textColourId, juce::Colour(0xffe6e1d6));
    presetBox.setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff242833));
    presetBox.setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(0xffe6e1d6));

    const int presetCount = processorRef.getNumFactoryPresets();
    auto addSection = [&](const juce::String& title, int start, int end)
    {
        if (start >= presetCount)
            return;
        const int clampedEnd = juce::jmin(end, presetCount - 1);
        if (clampedEnd < start)
            return;

        presetBox.addSectionHeading(title);
        for (int index = start; index <= clampedEnd; ++index)
            presetBox.addItem(processorRef.getFactoryPresetName(index), index + 1);
    };

    addSection("Foundational Spaces", 0, 5);
    addSection("Living Spaces", 6, 11);
    addSection("Remembering Spaces", 12, 14);
    addSection("Time-Bent / Abstract", 15, 17);
    addSection("Evolving Spaces", 18, presetCount - 1);

    presetBox.onChange = [this]()
    {
        const int presetIndex = presetBox.getSelectedId() - 1;
        if (presetIndex >= 0)
            processorRef.loadFactoryPreset(presetIndex);
    };

    setSize(900, 580);
}

MonumentAudioProcessorEditor::~MonumentAudioProcessorEditor() = default;

void MonumentAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0d0f12));
    g.setColour(juce::Colour(0xffe6e1d6));

    // Title
    g.setFont(juce::Font(juce::FontOptions(20.0f, juce::Font::bold)));
    g.drawFittedText("Monument", getLocalBounds().removeFromTop(35), juce::Justification::centred, 1);

    // Macro section label
    g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
    g.setColour(juce::Colour(0xffa8a49c));
    g.drawFittedText("MACRO CONTROLS", juce::Rectangle<int>(24, 45, getWidth() - 48, 20),
                     juce::Justification::centredLeft, 1);

    // Separator line after macros
    g.setColour(juce::Colour(0xff3a3f46));
    g.drawLine(24.0f, 185.0f, static_cast<float>(getWidth() - 24), 185.0f, 1.0f);

    // Base parameters label
    g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
    g.setColour(juce::Colour(0xffa8a49c));
    g.drawFittedText("BASE PARAMETERS", juce::Rectangle<int>(24, 195, getWidth() - 48, 20),
                     juce::Justification::centredLeft, 1);
}

void MonumentAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(24);
    area.removeFromTop(35);  // Title space

    // Macro Controls Section
    area.removeFromTop(25);  // Label space
    auto macroArea = area.removeFromTop(115);
    const auto macroWidth = macroArea.getWidth() / 6;

    materialKnob.setBounds(macroArea.removeFromLeft(macroWidth).reduced(6));
    topologyKnob.setBounds(macroArea.removeFromLeft(macroWidth).reduced(6));
    viscosityKnob.setBounds(macroArea.removeFromLeft(macroWidth).reduced(6));
    evolutionKnob.setBounds(macroArea.removeFromLeft(macroWidth).reduced(6));
    chaosKnob.setBounds(macroArea.removeFromLeft(macroWidth).reduced(6));
    elasticityKnob.setBounds(macroArea.removeFromLeft(macroWidth).reduced(6));

    area.removeFromTop(10);  // Separator space
    area.removeFromTop(25);  // Base params label space

    // Base Parameters Grid (4x3)
    auto gridArea = area.reduced(10);
    const auto columnWidth = gridArea.getWidth() / 4;
    const auto rowHeight = gridArea.getHeight() / 3;

    auto cell = [&](int row, int column)
    {
        return juce::Rectangle<int>(gridArea.getX() + column * columnWidth,
                                    gridArea.getY() + row * rowHeight,
                                    columnWidth,
                                    rowHeight)
            .reduced(6);
    };

    mixKnob.setBounds(cell(0, 0));
    timeKnob.setBounds(cell(0, 1));
    massKnob.setBounds(cell(0, 2));
    densityKnob.setBounds(cell(0, 3));

    bloomKnob.setBounds(cell(1, 0));
    airKnob.setBounds(cell(1, 1));
    widthKnob.setBounds(cell(1, 2));
    warpKnob.setBounds(cell(1, 3));

    driftKnob.setBounds(cell(2, 0));
    gravityKnob.setBounds(cell(2, 1));
    freezeToggle.setBounds(cell(2, 2));
    presetBox.setBounds(cell(2, 3));
}
