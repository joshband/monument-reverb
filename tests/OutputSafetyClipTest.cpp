/**
 * Monument Reverb - Output Safety Clip Test
 *
 * Verifies that the safety clip limiter bounds output when enabled.
 */

#include <JuceHeader.h>
#include "plugin/PluginProcessor.h"
#include <iostream>

#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_BLUE "\033[0;34m"
#define COLOR_RESET "\033[0m"

constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 512;

void setFloatParam(juce::AudioProcessorValueTreeState& apvts,
                   const juce::String& paramId,
                   float normalized)
{
    if (auto* param = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter(paramId)))
        param->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, normalized));
}

void setBoolParam(juce::AudioProcessorValueTreeState& apvts,
                  const juce::String& paramId,
                  bool value)
{
    if (auto* param = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter(paramId)))
        param->setValueNotifyingHost(value ? 1.0f : 0.0f);
}

float measurePeak(const juce::AudioBuffer<float>& buffer)
{
    float peak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            peak = std::max(peak, std::abs(data[i]));
    }
    return peak;
}

int main()
{
    std::cout << COLOR_BLUE << "\nMonument - Output Safety Clip Test" << COLOR_RESET << "\n\n";

    MonumentAudioProcessor processor;
    processor.prepareToPlay(kSampleRate, kBlockSize);

    auto& apvts = processor.getAPVTS();
    setFloatParam(apvts, "mix", 0.0f); // 0% wet to preserve dry input

    juce::MidiBuffer midi;

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    buffer.clear();

    // High amplitude dry signal
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            buffer.setSample(ch, i, 2.0f);
    }

    setBoolParam(apvts, "safetyClip", true);
    setFloatParam(apvts, "safetyClipDrive", 1.0f);

    processor.processBlock(buffer, midi);
    const float clippedPeak = measurePeak(buffer);

    if (clippedPeak > 1.01f)
    {
        std::cout << COLOR_RED << "✗ Safety clip failed to bound output (peak=" << clippedPeak << ")" << COLOR_RESET << "\n";
        return 1;
    }

    // Disable safety clip, output should remain above unity
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            buffer.setSample(ch, i, 2.0f);
    }

    setBoolParam(apvts, "safetyClip", false);
    processor.processBlock(buffer, midi);
    const float unclippedPeak = measurePeak(buffer);

    if (unclippedPeak < 1.5f)
    {
        std::cout << COLOR_RED << "✗ Safety clip disabled but output was still limited (peak=" << unclippedPeak << ")" << COLOR_RESET << "\n";
        return 1;
    }

    std::cout << COLOR_GREEN << "✓ Safety clip bounds output" << COLOR_RESET << "\n";
    return 0;
}
