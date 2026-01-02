#include "PluginEditor.h"

MonumentAudioProcessorEditor::MonumentAudioProcessorEditor(MonumentAudioProcessor& p)
    : juce::AudioProcessorEditor(&p),
      processorRef(p),
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

    const int presetCount = processorRef.getNumPresets();
    for (int index = 0; index < presetCount; ++index)
        presetBox.addItem(juce::String(processorRef.getPresetName(index)), index + 1);

    presetBox.onChange = [this]()
    {
        const int presetIndex = presetBox.getSelectedId() - 1;
        if (presetIndex >= 0)
            processorRef.loadPreset(presetIndex);
    };

    setSize(720, 420);
}

MonumentAudioProcessorEditor::~MonumentAudioProcessorEditor() = default;

void MonumentAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0d0f12));
    g.setColour(juce::Colour(0xffe6e1d6));
    g.setFont(juce::Font(juce::FontOptions(20.0f, juce::Font::bold)));
    g.drawFittedText("Monument", getLocalBounds().removeFromTop(30), juce::Justification::centred, 1);
}

void MonumentAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(24);
    area.removeFromTop(30);

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
