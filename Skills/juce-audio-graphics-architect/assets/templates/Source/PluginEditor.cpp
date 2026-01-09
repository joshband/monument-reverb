#include "PluginEditor.h"

@PLUGIN_NAME@AudioProcessorEditor::@PLUGIN_NAME@AudioProcessorEditor(@PLUGIN_NAME@AudioProcessor& p)
    : AudioProcessorEditor(&p), processor(p)
{
    auto configureSlider = [] (juce::Slider& slider)
    {
        slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
    };

    configureSlider(mixSlider);
    configureSlider(delayTimeSlider);
    configureSlider(delayFeedbackSlider);
    configureSlider(reverbSizeSlider);
    configureSlider(reverbDampingSlider);
    configureSlider(reverbMixSlider);

    addAndMakeVisible(mixSlider);
    addAndMakeVisible(delayTimeSlider);
    addAndMakeVisible(delayFeedbackSlider);
    addAndMakeVisible(reverbSizeSlider);
    addAndMakeVisible(reverbDampingSlider);
    addAndMakeVisible(reverbMixSlider);

    mixAttachment = std::make_unique<SliderAttachment>(processor.apvts, "mix", mixSlider);
    delayTimeAttachment = std::make_unique<SliderAttachment>(processor.apvts, "delayTimeMs", delayTimeSlider);
    delayFeedbackAttachment = std::make_unique<SliderAttachment>(processor.apvts, "delayFeedback", delayFeedbackSlider);
    reverbSizeAttachment = std::make_unique<SliderAttachment>(processor.apvts, "reverbSize", reverbSizeSlider);
    reverbDampingAttachment = std::make_unique<SliderAttachment>(processor.apvts, "reverbDamping", reverbDampingSlider);
    reverbMixAttachment = std::make_unique<SliderAttachment>(processor.apvts, "reverbMix", reverbMixSlider);

    addAndMakeVisible(glVisualizer);
    addAndMakeVisible(spectrum);
    addAndMakeVisible(overlayPanel);

    overlayPanel.setTargetAlpha(0.85f);
    overlayPanel.toFront(false);

    startTimerHz(60);
    setSize(780, 480);
}

@PLUGIN_NAME@AudioProcessorEditor::~@PLUGIN_NAME@AudioProcessorEditor() = default;

void @PLUGIN_NAME@AudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void @PLUGIN_NAME@AudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(16);
    auto top = area.removeFromTop(area.getHeight() / 2);
    glVisualizer.setBounds(top);

    auto overlayBounds = top.reduced(12);
    overlayPanel.setBounds(overlayBounds);

    auto controlsArea = area.removeFromRight(220);
    spectrum.setBounds(area);

    auto knobArea = controlsArea.reduced(10);
    auto knobRowHeight = knobArea.getHeight() / 6;

    mixSlider.setBounds(knobArea.removeFromTop(knobRowHeight));
    delayTimeSlider.setBounds(knobArea.removeFromTop(knobRowHeight));
    delayFeedbackSlider.setBounds(knobArea.removeFromTop(knobRowHeight));
    reverbSizeSlider.setBounds(knobArea.removeFromTop(knobRowHeight));
    reverbDampingSlider.setBounds(knobArea.removeFromTop(knobRowHeight));
    reverbMixSlider.setBounds(knobArea.removeFromTop(knobRowHeight));
}

void @PLUGIN_NAME@AudioProcessorEditor::timerCallback()
{
    if (processor.getFftAnalyzer().popMagnitudes(magnitudes))
    {
        spectrum.setMagnitudes(magnitudes);
        const auto maxIt = std::max_element(magnitudes.begin(), magnitudes.begin() + (magnitudes.size() / 2));
        const float level = (maxIt != magnitudes.end()) ? *maxIt : 0.0f;
        glVisualizer.setLevel(level * 0.02f);
    }
}
