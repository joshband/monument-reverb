#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "OpenGLVisualizer.h"
#include "LayeredPanel.h"

class SpectrumComponent : public juce::Component
{
public:
    static constexpr int fftSize = @PLUGIN_NAME@AudioProcessor::FftAnalyzer::fftSize;

    void setMagnitudes(const std::array<float, fftSize>& newMagnitudes)
    {
        magnitudes = newMagnitudes;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(juce::Colours::black);
        g.setColour(juce::Colours::cyan);

        auto bounds = getLocalBounds().toFloat();
        const int bins = fftSize / 2;
        juce::Path path;

        for (int i = 0; i < bins; ++i)
        {
            const float norm = juce::jlimit(0.0f, 1.0f, magnitudes[static_cast<size_t>(i)] * 0.02f);
            const float x = bounds.getX() + bounds.getWidth() * (static_cast<float>(i) / static_cast<float>(bins - 1));
            const float y = bounds.getBottom() - norm * bounds.getHeight();

            if (i == 0)
                path.startNewSubPath(x, y);
            else
                path.lineTo(x, y);
        }

        g.strokePath(path, juce::PathStrokeType(2.0f));
    }

private:
    std::array<float, fftSize> magnitudes {};
};

class @PLUGIN_NAME@AudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit @PLUGIN_NAME@AudioProcessorEditor(@PLUGIN_NAME@AudioProcessor&);
    ~@PLUGIN_NAME@AudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    @PLUGIN_NAME@AudioProcessor& processor;

    juce::Slider mixSlider;
    juce::Slider delayTimeSlider;
    juce::Slider delayFeedbackSlider;
    juce::Slider reverbSizeSlider;
    juce::Slider reverbDampingSlider;
    juce::Slider reverbMixSlider;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;
    std::unique_ptr<SliderAttachment> delayTimeAttachment;
    std::unique_ptr<SliderAttachment> delayFeedbackAttachment;
    std::unique_ptr<SliderAttachment> reverbSizeAttachment;
    std::unique_ptr<SliderAttachment> reverbDampingAttachment;
    std::unique_ptr<SliderAttachment> reverbMixAttachment;

    SpectrumComponent spectrum;
    OpenGLVisualizer glVisualizer;
    LayeredPanel overlayPanel;

    std::array<float, SpectrumComponent::fftSize> magnitudes {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(@PLUGIN_NAME@AudioProcessorEditor)
};
