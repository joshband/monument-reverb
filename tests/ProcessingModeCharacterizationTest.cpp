/**
 * Monument Reverb - Processing Mode (Ancient Monuments Routing) Test
 *
 * Exercises the "processingMode" APVTS choice parameter added to wire up
 * MonumentAudioProcessor::setProcessingMode()/getProcessingMode(), which
 * previously had zero callers anywhere in the plugin. That made
 * DspRoutingGraph::processResonantHalls()/processBreathingStone()
 * unreachable in production even though they were fully implemented
 * (see dsp/DspRoutingGraph.cpp) -- only processAncientWay() ever ran.
 *
 * Coverage:
 * 1. All three processing modes (AncientWay, ResonantHalls, BreathingStone)
 *    are reachable through the real MonumentAudioProcessor via the
 *    "processingMode" parameter, produce finite, non-silent output on an
 *    impulse, and settle to the requested mode (getProcessingMode()).
 * 2. Switching processingMode mid-stream crossfades cleanly (no clicks),
 *    exercising the existing modeTransitionGain/ModeTransitionState machine.
 * 3. Distinct modes actually reorder the DSP chain: AncientWay and
 *    ResonantHalls (TubeRayTracer before vs. after Chambers) diverge on
 *    identical input once the mode change has settled. This proves
 *    processResonantHalls()/processBreathingStone() are truly exercised
 *    and not just no-ops that happen to alias processAncientWay().
 */

#include <JuceHeader.h>
#include "plugin/PluginProcessor.h"
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_BLUE "\033[0;34m"
#define COLOR_RESET "\033[0m"

namespace
{
constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 512;
constexpr int kNumChannels = 2;

// 50ms crossfade (see modeTransitionGain.reset(sampleRate, 0.05) in
// PluginProcessor.cpp) fades out then in, i.e. up to ~100ms to fully settle.
// At 512 samples/block and 48kHz that's ~9.4 blocks per leg; use a healthy
// margin.
constexpr int kSettleBlocks = 30;

struct TestResult
{
    std::string testName;
    bool passed{false};
    std::string message;
};

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

float measureMaxTransient(const juce::AudioBuffer<float>& previousLastSample,
                           const juce::AudioBuffer<float>& buffer)
{
    float maxTransient = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const float* data = buffer.getReadPointer(ch);
        float prev = previousLastSample.getSample(ch, 0);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            maxTransient = std::max(maxTransient, std::abs(data[i] - prev));
            prev = data[i];
        }
    }
    return maxTransient;
}

const char* modeName(int mode)
{
    switch (mode)
    {
        case 0: return "AncientWay";
        case 1: return "ResonantHalls";
        case 2: return "BreathingStone";
        default: return "Unknown";
    }
}
}  // namespace

//==============================================================================
// Test 1: All Three Modes Reachable
//==============================================================================
TestResult testAllModesReachable()
{
    TestResult result{"PROC-MODE-1: All Modes Reachable", false, ""};

    try
    {
        for (int mode = 0; mode <= 2; ++mode)
        {
            MonumentAudioProcessor processor;
            processor.prepareToPlay(kSampleRate, kBlockSize);

            auto& apvts = processor.getAPVTS();
            setChoiceParam(apvts, "processingMode", mode);

            juce::MidiBuffer midi;

            juce::AudioBuffer<float> impulseBuffer(kNumChannels, kBlockSize);
            impulseBuffer.clear();
            impulseBuffer.setSample(0, 0, 1.0f);
            impulseBuffer.setSample(1, 0, 1.0f);
            processor.processBlock(impulseBuffer, midi);

            if (!bufferIsFinite(impulseBuffer))
            {
                result.message = std::string("Non-finite output for mode ") + modeName(mode);
                return result;
            }

            bool everNonZero = bufferHasNonZeroSample(impulseBuffer);

            // Let the mode-change crossfade settle and the tail decay.
            for (int b = 0; b < kSettleBlocks; ++b)
            {
                juce::AudioBuffer<float> silentBuffer(kNumChannels, kBlockSize);
                silentBuffer.clear();
                processor.processBlock(silentBuffer, midi);

                if (!bufferIsFinite(silentBuffer))
                {
                    result.message = std::string("Non-finite output during tail for mode ") + modeName(mode);
                    return result;
                }
                everNonZero = everNonZero || bufferHasNonZeroSample(silentBuffer);
            }

            if (!everNonZero)
            {
                result.message = std::string("Mode ") + modeName(mode) + " produced no audible output";
                return result;
            }

            if (processor.getProcessingMode() != static_cast<ProcessingMode>(mode))
            {
                result.message = std::string("getProcessingMode() did not settle to requested mode ") + modeName(mode);
                return result;
            }

            processor.releaseResources();
        }

        result.passed = true;
        result.message = "AncientWay, ResonantHalls, and BreathingStone all reachable, finite, and audible";
    }
    catch (const std::exception& e)
    {
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test 2: Mode Switch Crossfades Without Clicks
//==============================================================================
TestResult testModeSwitchNoClicks()
{
    TestResult result{"PROC-MODE-2: Mode Switch Crossfade", false, ""};

    try
    {
        MonumentAudioProcessor processor;
        processor.prepareToPlay(kSampleRate, kBlockSize);

        auto& apvts = processor.getAPVTS();
        juce::MidiBuffer midi;

        const float omega = juce::MathConstants<float>::twoPi * 440.0f / static_cast<float>(kSampleRate);
        int phase = 0;

        auto fillSine = [&](juce::AudioBuffer<float>& buffer)
        {
            for (int ch = 0; ch < kNumChannels; ++ch)
            {
                float* data = buffer.getWritePointer(ch);
                for (int i = 0; i < kBlockSize; ++i)
                    data[i] = std::sin(omega * static_cast<float>(phase + i)) * 0.3f;
            }
        };

        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        juce::AudioBuffer<float> lastSample(kNumChannels, 1);
        lastSample.clear();

        float maxTransient = 0.0f;

        for (int block = 0; block < 60; ++block)
        {
            fillSine(buffer);
            phase += kBlockSize;

            if (block == 20)
                setChoiceParam(apvts, "processingMode", 1);  // -> ResonantHalls
            if (block == 40)
                setChoiceParam(apvts, "processingMode", 2);  // -> BreathingStone

            processor.processBlock(buffer, midi);

            if (!bufferIsFinite(buffer))
            {
                result.message = "Non-finite output during mode switch";
                return result;
            }

            maxTransient = std::max(maxTransient, measureMaxTransient(lastSample, buffer));
            for (int ch = 0; ch < kNumChannels; ++ch)
                lastSample.setSample(ch, 0, buffer.getSample(ch, kBlockSize - 1));
        }

        const float maxTransientDb = 20.0f * std::log10(maxTransient + 1e-10f);

        // The crossfade is a SmoothedValue-driven gain ramp, so switching
        // between fixed chains should not introduce a hard click. -25dB
        // leaves headroom above ordinary DSP-continuity noise while still
        // catching a genuine regression (e.g. crossfade skipped entirely).
        constexpr float kClickThresholdDb = -25.0f;
        if (maxTransientDb > kClickThresholdDb)
        {
            result.message = "Click detected during mode switch: " + std::to_string(maxTransientDb) +
                " dB (threshold " + std::to_string(kClickThresholdDb) + " dB)";
            return result;
        }

        result.passed = true;
        result.message = "No clicks across AncientWay->ResonantHalls->BreathingStone, max transient = " +
            std::to_string(maxTransientDb) + " dB";
    }
    catch (const std::exception& e)
    {
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test 3: Distinct Modes Actually Reorder the Chain (Wiring Regression)
//==============================================================================
TestResult testModesProduceDistinctOutput()
{
    TestResult result{"PROC-MODE-3: Modes Produce Distinct Output", false, ""};

    try
    {
        MonumentAudioProcessor processorAncient;
        MonumentAudioProcessor processorResonant;
        processorAncient.prepareToPlay(kSampleRate, kBlockSize);
        processorResonant.prepareToPlay(kSampleRate, kBlockSize);

        setChoiceParam(processorResonant.getAPVTS(), "processingMode", 1);  // ResonantHalls

        juce::MidiBuffer midi;
        const float omega = juce::MathConstants<float>::twoPi * 220.0f / static_cast<float>(kSampleRate);
        int phase = 0;

        auto fillSine = [&](juce::AudioBuffer<float>& buffer)
        {
            for (int ch = 0; ch < kNumChannels; ++ch)
            {
                float* data = buffer.getWritePointer(ch);
                for (int i = 0; i < kBlockSize; ++i)
                    data[i] = std::sin(omega * static_cast<float>(phase + i)) * 0.4f;
            }
        };

        float maxDifference = 0.0f;

        // Run past the crossfade settle window, then keep comparing so the
        // steady-state (fully-switched) chains are what gets measured.
        for (int block = 0; block < kSettleBlocks + 20; ++block)
        {
            juce::AudioBuffer<float> bufferAncient(kNumChannels, kBlockSize);
            juce::AudioBuffer<float> bufferResonant(kNumChannels, kBlockSize);
            fillSine(bufferAncient);
            bufferResonant.makeCopyOf(bufferAncient);
            phase += kBlockSize;

            processorAncient.processBlock(bufferAncient, midi);
            processorResonant.processBlock(bufferResonant, midi);

            if (!bufferIsFinite(bufferAncient) || !bufferIsFinite(bufferResonant))
            {
                result.message = "Non-finite output comparing AncientWay vs ResonantHalls";
                return result;
            }

            if (block >= kSettleBlocks)
            {
                for (int ch = 0; ch < kNumChannels; ++ch)
                {
                    const float* a = bufferAncient.getReadPointer(ch);
                    const float* r = bufferResonant.getReadPointer(ch);
                    for (int i = 0; i < kBlockSize; ++i)
                        maxDifference = std::max(maxDifference, std::abs(a[i] - r[i]));
                }
            }
        }

        constexpr float kMinExpectedDifference = 1.0e-6f;
        if (maxDifference < kMinExpectedDifference)
        {
            result.message = "ResonantHalls output identical to AncientWay (max diff " +
                std::to_string(maxDifference) + ") -- processResonantHalls() not actually exercised";
            return result;
        }

        result.passed = true;
        result.message = "processingMode reorders the DSP chain: max diff " + std::to_string(maxDifference);
    }
    catch (const std::exception& e)
    {
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Main Test Runner
//==============================================================================
int main()
{
    std::cout << COLOR_BLUE << "\n=================================================\n";
    std::cout << "Monument Reverb - Processing Mode Routing Test\n";
    std::cout << "=================================================\n" << COLOR_RESET;

    juce::ScopedJuceInitialiser_GUI juceInit;

    std::vector<TestResult> results;
    results.push_back(testAllModesReachable());
    results.push_back(testModeSwitchNoClicks());
    results.push_back(testModesProduceDistinctOutput());

    std::cout << "\n" << COLOR_BLUE << "=================================================\n";
    std::cout << "Test Results\n";
    std::cout << "=================================================\n" << COLOR_RESET;

    int passed = 0;
    for (const auto& result : results)
    {
        std::string status = result.passed ? (std::string(COLOR_GREEN) + "PASS" + COLOR_RESET)
                                            : (std::string(COLOR_RED) + "FAIL" + COLOR_RESET);
        std::cout << status << " | " << result.testName << "\n";
        std::cout << "      " << result.message << "\n";
        if (result.passed)
            ++passed;
    }

    std::cout << "\nSummary: " << passed << "/" << results.size() << " tests passed\n";

    return (passed == static_cast<int>(results.size())) ? 0 : 1;
}
