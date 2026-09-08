/**
 * Facade Gain Smoothing Test
 *
 * Regression test for zipper noise when Facade output gain changes rapidly
 * (e.g. fast mix automation). Verifies SmoothedValue interpolation keeps
 * sample-to-sample deltas below an audible threshold.
 *
 * Historical note: this file previously also carried
 * testFeedbackAt100PercentMix, a regression test for feedback runaway in
 * DspRoutingGraph's generic series/parallel/feedback/crossfeed graph
 * executor (DspRoutingGraph::process()). That executor had no caller
 * anywhere in the plugin and has been removed as dead code (see
 * ARCHITECTURE.md), so the test for its feedback-runaway bug was removed
 * with it — the bug's subject no longer exists.
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

//==============================================================================
// Test: Facade Gain Smoothing (Zipper Noise Prevention), via the live signal chain
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

            // Process a few blocks through the live chain to let smoother catch up
            for (int block = 0; block < 5; ++block)
            {
                auto bufferCopy = buffer;
                graph.processAncientWay(bufferCopy);

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
    std::cout << "Facade Gain Smoothing Regression Test" << std::endl;
    std::cout << "===============================================\n" << std::endl;

    std::vector<TestResult> results;

    std::cout << "Test: Facade Gain Smoothing" << std::endl;
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
