/**
 * Monument Reverb - Macro Mode Integration Test
 *
 * Verifies that macro mode selection drives routing preset selection in the processor.
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

void setFloatParam(juce::AudioProcessorValueTreeState& apvts,
                   const juce::String& paramId,
                   float normalized)
{
    if (auto* param = dynamic_cast<juce::RangedAudioParameter*>(apvts.getParameter(paramId)))
        param->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, normalized));
}

int main()
{
    std::cout << COLOR_BLUE << "\nMonument - Macro Mode Integration Test" << COLOR_RESET << "\n\n";

    MonumentAudioProcessor processor;
    processor.prepareToPlay(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    buffer.clear();
    juce::MidiBuffer midi;

    auto& apvts = processor.getAPVTS();

    // Expressive macro mode: SpaceType -> routing preset mapping
    setChoiceParam(apvts, "macroMode", 1); // Expressive
    setFloatParam(apvts, "spaceType", 0.9f); // Metallic

    processor.processBlock(buffer, midi);

#if defined(MONUMENT_TESTING)
    const int routingPreset = processor.getLastRoutingPresetForTesting();
    const int expectedPreset = static_cast<int>(monument::dsp::RoutingPresetType::MetallicGranular);
    if (routingPreset != expectedPreset)
    {
        std::cout << COLOR_RED << "✗ Expressive macro routing preset mismatch" << COLOR_RESET
                  << " (expected " << expectedPreset << ", got " << routingPreset << ")\n";
        return 1;
    }
#endif

    // Ancient macro mode: routing preset parameter should drive routing
    setChoiceParam(apvts, "macroMode", 0); // Ancient
    setChoiceParam(apvts, "routingPreset", 4); // Shimmer Infinity

    processor.processBlock(buffer, midi);

#if defined(MONUMENT_TESTING)
    const int routingPresetAncient = processor.getLastRoutingPresetForTesting();
    const int expectedAncient = 4;
    if (routingPresetAncient != expectedAncient)
    {
        std::cout << COLOR_RED << "✗ Ancient macro routing preset mismatch" << COLOR_RESET
                  << " (expected " << expectedAncient << ", got " << routingPresetAncient << ")\n";
        return 1;
    }
#endif

    std::cout << COLOR_GREEN << "✓ Macro mode routing integration OK" << COLOR_RESET << "\n";
    return 0;
}
