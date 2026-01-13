/**
 * Monument Reverb - Timeline Integration Test
 *
 * Ensures timeline parameters drive the SequenceScheduler in the processor.
 */

#include <JuceHeader.h>
#include "plugin/PluginProcessor.h"
#include "dsp/SequenceScheduler.h"
#include <iostream>

#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_BLUE "\033[0;34m"
#define COLOR_RESET "\033[0m"

constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 512;

void setChoiceParam(juce::AudioProcessorValueTreeState& apvts,
                    const juce::String& paramId,
                    int index)
{
    auto* param = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(paramId));
    if (param == nullptr)
        return;

    const float normalized = param->convertTo0to1(static_cast<float>(index));
    param->setValueNotifyingHost(normalized);
}

void setBoolParam(juce::AudioProcessorValueTreeState& apvts,
                  const juce::String& paramId,
                  bool value)
{
    if (auto* param = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter(paramId)))
        param->setValueNotifyingHost(value ? 1.0f : 0.0f);
}

int main()
{
    std::cout << COLOR_BLUE << "\nMonument - Timeline Integration Test" << COLOR_RESET << "\n\n";

    MonumentAudioProcessor processor;
    processor.prepareToPlay(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    buffer.clear();
    juce::MidiBuffer midi;

    auto& apvts = processor.getAPVTS();

    setChoiceParam(apvts, "timelinePreset", 2); // Living Space
    setBoolParam(apvts, "timelineEnabled", true);

    processor.processBlock(buffer, midi);

    auto& scheduler = processor.getSequenceScheduler();
    if (!scheduler.isEnabled())
    {
        std::cout << COLOR_RED << "✗ Timeline not enabled" << COLOR_RESET << "\n";
        return 1;
    }

    if (scheduler.getSequence().name != "Living Space")
    {
        std::cout << COLOR_RED << "✗ Timeline preset mismatch" << COLOR_RESET
                  << " (got '" << scheduler.getSequence().name << "')\n";
        return 1;
    }

    auto warpValue = scheduler.getParameterValue(monument::dsp::SequenceScheduler::ParameterId::Warp);
    if (!warpValue.has_value())
    {
        std::cout << COLOR_RED << "✗ Timeline did not publish parameter values" << COLOR_RESET << "\n";
        return 1;
    }

    if (*warpValue < 0.0f || *warpValue > 1.0f)
    {
        std::cout << COLOR_RED << "✗ Timeline warp out of range" << COLOR_RESET << "\n";
        return 1;
    }

    setBoolParam(apvts, "timelineEnabled", false);
    processor.processBlock(buffer, midi);

    warpValue = scheduler.getParameterValue(monument::dsp::SequenceScheduler::ParameterId::Warp);
    if (warpValue.has_value())
    {
        std::cout << COLOR_RED << "✗ Timeline values not cleared when disabled" << COLOR_RESET << "\n";
        return 1;
    }

    std::cout << COLOR_GREEN << "✓ Timeline integration OK" << COLOR_RESET << "\n";
    return 0;
}
