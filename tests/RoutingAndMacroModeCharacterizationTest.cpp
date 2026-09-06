/**
 * Monument Reverb - Routing Preset x Macro Mode Characterization Test
 *
 * Report Step 5 characterization: proves that BOTH macro-mode systems
 * (macroMode: 0 = Ancient Monuments / MacroMapper, 1 = Expressive /
 * ExpressiveMacroMapper) combined with ALL 8 host-visible routingPreset
 * indices (0..7, see monument::dsp::RoutingPresetType / loadRoutingPreset()
 * in plugin/PluginProcessor.cpp and dsp/DspRoutingGraph.h/.cpp) are
 * reachable through the real MonumentAudioProcessor without crashing or
 * producing non-finite output, and that an impulse actually produces sound.
 *
 * This is explicitly NOT a claim about the musical correctness of any given
 * routing preset's topology. Per the architectural assessment report,
 * routingPreset only toggles a bypass mask on the fixed AncientWay
 * processing chain -- it does NOT select a different DSP topology. This
 * test does not contradict or attempt to "fix" that; it only proves
 * reachability/stability of each (macroMode, routingPreset) combination.
 */

#include <JuceHeader.h>
#include "plugin/PluginProcessor.h"
#include <cmath>
#include <iostream>

#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_BLUE "\033[0;34m"
#define COLOR_RESET "\033[0m"

namespace
{
constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 512;
constexpr int kNumSilentBlocksAfterImpulse = 4;

void setChoiceParam(juce::AudioProcessorValueTreeState& apvts, const juce::String& paramId, int index)
{
    if (auto* param = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter(paramId)))
    {
        const float normalized = param->convertTo0to1(static_cast<float>(index));
        param->setValueNotifyingHost(normalized);
    }
}

bool bufferIsFinite(const juce::AudioBuffer<float>& buffer)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const float* samples = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            if (!std::isfinite(samples[i]))
                return false;
    }
    return true;
}

bool bufferHasNonZeroSample(const juce::AudioBuffer<float>& buffer, float threshold = 1.0e-9f)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const float* samples = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            if (std::abs(samples[i]) > threshold)
                return true;
    }
    return false;
}
}  // namespace

int main()
{
    std::cout << COLOR_BLUE << "\nMonument - Routing Preset x Macro Mode Characterization Test" << COLOR_RESET << "\n\n";

    juce::ScopedJuceInitialiser_GUI juceInit;

    int totalCombinations = 0;
    int passedCombinations = 0;
    bool anyFailed = false;

    for (int macroMode = 0; macroMode <= 1; ++macroMode)
    {
        for (int routingPreset = 0; routingPreset <= 7; ++routingPreset)
        {
            ++totalCombinations;

            MonumentAudioProcessor processor;
            processor.prepareToPlay(kSampleRate, kBlockSize);

            auto& apvts = processor.getAPVTS();
            setChoiceParam(apvts, "macroMode", macroMode);
            setChoiceParam(apvts, "routingPreset", routingPreset);

            juce::MidiBuffer midi;

            // Impulse block: single-sample impulse on both channels at t=0.
            juce::AudioBuffer<float> impulseBuffer(2, kBlockSize);
            impulseBuffer.clear();
            impulseBuffer.setSample(0, 0, 1.0f);
            impulseBuffer.setSample(1, 0, 1.0f);

            processor.processBlock(impulseBuffer, midi);

            bool ok = true;
            juce::String failureReason;

            if (!bufferIsFinite(impulseBuffer))
            {
                ok = false;
                failureReason = "non-finite output on impulse block";
            }
            else if (!bufferHasNonZeroSample(impulseBuffer))
            {
                ok = false;
                failureReason = "impulse block was all-zero (no signal reached output)";
            }

            // A few subsequent silent blocks -- proves the routing/mode
            // combination doesn't blow up or produce non-finite garbage
            // while the reverb tail decays.
            for (int b = 0; b < kNumSilentBlocksAfterImpulse && ok; ++b)
            {
                juce::AudioBuffer<float> silentBuffer(2, kBlockSize);
                silentBuffer.clear();
                processor.processBlock(silentBuffer, midi);

                if (!bufferIsFinite(silentBuffer))
                {
                    ok = false;
                    failureReason = "non-finite output during silent tail block " + juce::String(b);
                }
            }

            processor.releaseResources();

            const juce::String macroModeName = (macroMode == 0 ? "Ancient" : "Expressive");
            std::cout << "  macroMode=" << macroModeName << " routingPreset=" << routingPreset << ": ";

            if (ok)
            {
                std::cout << COLOR_GREEN << "OK" << COLOR_RESET << "\n";
                ++passedCombinations;
            }
            else
            {
                std::cout << COLOR_RED << "FAIL (" << failureReason << ")" << COLOR_RESET << "\n";
                anyFailed = true;
            }
        }
    }

    std::cout << "\n" << totalCombinations << " combinations tested, " << passedCombinations << " passed.\n";

    if (!anyFailed)
    {
        std::cout << COLOR_GREEN << "\n✓ All macroMode x routingPreset combinations reachable, finite, and non-silent on impulse\n" << COLOR_RESET;
        return 0;
    }

    std::cout << COLOR_RED << "\n✗ One or more macroMode x routingPreset combinations failed characterization\n" << COLOR_RESET;
    return 1;
}
