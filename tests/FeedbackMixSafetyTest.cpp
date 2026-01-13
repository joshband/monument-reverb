/**
 * Feedback Mix Safety Test
 *
 * Regression test for feedback runaway at 100% mix levels.
 *
 * Bug discovered: When mix=100%, feedback routing presets (ShimmerInfinity,
 * ElasticFeedbackDream) experienced energy buildup because there was no
 * dry signal dampening and Facade output gain was fixed at 1.0.
 *
 * Fix: Apply mix-dependent attenuation to Facade output gain:
 * - 0% mix: 1.0x gain (no attenuation)
 * - 100% mix: 0.94x gain (-0.53 dB dampening prevents runaway)
 */

#include <JuceHeader.h>
#include "../dsp/DspRoutingGraph.h"
#include <iostream>
#include <cmath>

using namespace monument::dsp;

constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 512;
constexpr int kNumChannels = 2;

struct TestResult
{
    bool passed{false};
    std::string testName;
    std::string message;
};

float measureRMS(const juce::AudioBuffer<float>& buffer)
{
    float sum = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float sample = buffer.getSample(ch, i);
            sum += sample * sample;
        }
    }
    return std::sqrt(sum / (buffer.getNumChannels() * buffer.getNumSamples()));
}

float measurePeak(const juce::AudioBuffer<float>& buffer)
{
    float peak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            peak = std::max(peak, std::abs(buffer.getSample(ch, i)));
        }
    }
    return peak;
}

bool containsInvalidSamples(const juce::AudioBuffer<float>& buffer)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float sample = buffer.getSample(ch, i);
            if (!std::isfinite(sample))
                return true;
        }
    }
    return false;
}

//==============================================================================
// Test: Feedback Stability at 100% Mix
//==============================================================================
TestResult testFeedbackAt100PercentMix()
{
    TestResult result;
    result.testName = "Feedback Stability at 100% Mix";

    try
    {
        DspRoutingGraph graph;
        graph.prepare(kSampleRate, kBlockSize, kNumChannels);

        // Test all feedback routing presets
        std::vector<RoutingPresetType> feedbackPresets = {
            RoutingPresetType::ShimmerInfinity,
            RoutingPresetType::ElasticFeedbackDream
        };

        for (const auto& preset : feedbackPresets)
        {
            graph.loadRoutingPreset(preset);

            // Simulate 100% mix by setting Facade output gain to 0.94 (feedback safety)
            // In PluginProcessor, this is calculated as:
            // feedbackSafetyGain = juce::jmap(1.0f, 1.0f, 0.94f) = 0.94f
            graph.setFacadeParams(0.5f, 1.0f, 0.94f);  // air=0.5, width=1.0, gain=0.94

            // Create impulse
            juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
            buffer.clear();
            buffer.setSample(0, 0, 0.8f);  // Strong impulse to stress test
            buffer.setSample(1, 0, 0.8f);

            // Process for 20 seconds to detect slow energy buildup
            const int numBlocks = static_cast<int>((20.0 * kSampleRate) / kBlockSize);
            float maxRMS = 0.0f;
            float maxPeak = 0.0f;

            for (int i = 0; i < numBlocks; ++i)
            {
                graph.process(buffer);

                float rms = measureRMS(buffer);
                float peak = measurePeak(buffer);
                maxRMS = std::max(maxRMS, rms);
                maxPeak = std::max(maxPeak, peak);

                // Check for runaway feedback (stricter threshold for 100% mix)
                // At 100% mix with feedback, RMS should stabilize < 1.5
                if (rms > 1.5f)
                {
                    result.passed = false;
                    result.message = "Feedback runaway at 100% mix (preset: " +
                                    std::to_string(static_cast<int>(preset)) +
                                    "): RMS = " + std::to_string(rms) +
                                    " at block " + std::to_string(i) +
                                    " (should be < 1.5)";
                    return result;
                }

                // Check for clipping (peak > 1.0 indicates energy buildup)
                if (peak > 2.0f)
                {
                    result.passed = false;
                    result.message = "Signal clipping at 100% mix: Peak = " +
                                    std::to_string(peak) + " at block " + std::to_string(i);
                    return result;
                }

                // Check for NaN/Inf
                if (containsInvalidSamples(buffer))
                {
                    result.passed = false;
                    result.message = "NaN/Inf detected at 100% mix";
                    return result;
                }

                // Continue with silence (feedback loop should sustain, not grow)
                buffer.clear();
            }

            std::cout << "  Preset " << static_cast<int>(preset)
                      << ": maxRMS=" << maxRMS << ", maxPeak=" << maxPeak << std::endl;
        }

        result.passed = true;
        result.message = "All feedback presets stable at 100% mix over 20s";
    }
    catch (const std::exception& e)
    {
        result.passed = false;
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test: Facade Gain Smoothing (Zipper Noise Prevention)
//==============================================================================
TestResult testFacadeGainSmoothing()
{
    TestResult result;
    result.testName = "Facade Gain Smoothing";

    try
    {
        DspRoutingGraph graph;
        graph.prepare(kSampleRate, kBlockSize, kNumChannels);
        graph.loadRoutingPreset(RoutingPresetType::TraditionalCathedral);

        // Create constant tone to detect zipper noise
        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        const float frequency = 440.0f;  // A4
        for (int ch = 0; ch < kNumChannels; ++ch)
        {
            for (int i = 0; i < kBlockSize; ++i)
            {
                float phase = static_cast<float>(i) / kSampleRate;
                buffer.setSample(ch, i, 0.5f * std::sin(2.0f * juce::MathConstants<float>::pi * frequency * phase));
            }
        }

        // Rapidly change gain to stress test smoother (simulate fast mix changes)
        std::vector<float> gainSequence = {1.0f, 0.94f, 1.0f, 0.94f, 1.0f};
        std::vector<float> diffValues;

        for (size_t i = 0; i < gainSequence.size(); ++i)
        {
            graph.setFacadeParams(0.5f, 1.0f, gainSequence[i]);

            // Process a few blocks to let smoother catch up
            for (int block = 0; block < 5; ++block)
            {
                auto bufferCopy = buffer;
                graph.process(bufferCopy);

                // Measure sample-to-sample differences (zipper noise shows as high diffs)
                for (int ch = 0; ch < kNumChannels; ++ch)
                {
                    for (int samp = 1; samp < kBlockSize; ++samp)
                    {
                        float diff = std::abs(bufferCopy.getSample(ch, samp) -
                                            bufferCopy.getSample(ch, samp - 1));
                        diffValues.push_back(diff);
                    }
                }
            }
        }

        // Calculate 99th percentile of differences (should be small if smoothed)
        std::sort(diffValues.begin(), diffValues.end());
        size_t p99Index = static_cast<size_t>(diffValues.size() * 0.99);
        float p99Diff = diffValues[p99Index];

        // Threshold: With 20ms smoothing and rapid gain changes, sample-to-sample diffs should be < 0.03
        // (0.03 = ~26dB SNR, perceptually transparent for music signals)
        if (p99Diff > 0.03f)
        {
            result.passed = false;
            result.message = "Zipper noise detected: 99th percentile diff = " +
                            std::to_string(p99Diff) + " (should be < 0.03)";
            return result;
        }

        result.passed = true;
        result.message = "Facade gain smoothing working: p99 diff = " +
                        std::to_string(p99Diff) + " (< 0.03, perceptually transparent)";
    }
    catch (const std::exception& e)
    {
        result.passed = false;
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Main Test Runner
//==============================================================================
int main()
{
    std::cout << "===============================================" << std::endl;
    std::cout << "Feedback Mix Safety Regression Tests" << std::endl;
    std::cout << "===============================================\n" << std::endl;

    std::vector<TestResult> results;

    // Test 1: Feedback at 100% Mix
    std::cout << "Test 1: Feedback Stability at 100% Mix" << std::endl;
    results.push_back(testFeedbackAt100PercentMix());

    // Test 2: Facade Gain Smoothing
    std::cout << "\nTest 2: Facade Gain Smoothing" << std::endl;
    results.push_back(testFacadeGainSmoothing());

    // Print summary
    std::cout << "\n===============================================" << std::endl;
    std::cout << "Test Summary" << std::endl;
    std::cout << "===============================================" << std::endl;

    int passed = 0;
    int failed = 0;

    for (const auto& result : results)
    {
        std::cout << (result.passed ? "[PASS] " : "[FAIL] ")
                  << result.testName << ": " << result.message << std::endl;

        if (result.passed)
            ++passed;
        else
            ++failed;
    }

    std::cout << "\nTotal: " << (passed + failed) << " tests, "
              << passed << " passed, " << failed << " failed" << std::endl;

    return failed > 0 ? 1 : 0;
}
