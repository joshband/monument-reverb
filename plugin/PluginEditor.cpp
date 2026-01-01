#include "PluginEditor.h"

MonumentAudioProcessorEditor::MonumentAudioProcessorEditor(MonumentAudioProcessor& p)
    : juce::AudioProcessorEditor(&p),
      processor(p),
      timeAttachment(processor.getAPVTS(), "time", timeSlider),
      mixAttachment(processor.getAPVTS(), "mix", mixSlider)
{
    auto setupSlider = [this](juce::Slider& slider, juce::Label& label, const juce::String& text) {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 20);
        addAndMakeVisible(slider);

        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(label);
    };

    setupSlider(timeSlider, timeLabel, "Time");
    setupSlider(mixSlider, mixLabel, "Mix");

    setSize(420, 240);
}

MonumentAudioProcessorEditor::~MonumentAudioProcessorEditor() = default;

void MonumentAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0d0f12));
    g.setColour(juce::Colour(0xffe6e1d6));
    g.setFont(juce::Font(20.0f, juce::Font::bold));
    g.drawFittedText("Monument", getLocalBounds().removeFromTop(30), juce::Justification::centred, 1);
}

void MonumentAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(24);
    area.removeFromTop(30);

    auto sliderArea = area.reduced(10);
    const auto halfWidth = sliderArea.getWidth() / 2;

    auto timeArea = sliderArea.removeFromLeft(halfWidth);
    timeLabel.setBounds(timeArea.removeFromTop(20));
    timeSlider.setBounds(timeArea.reduced(10));

    auto mixArea = sliderArea;
    mixLabel.setBounds(mixArea.removeFromTop(20));
    mixSlider.setBounds(mixArea.reduced(10));
}
