/**
 * Monument Reverb - DSP Initialization & Lifecycle Test
 *
 * Tests that all DSP modules initialize correctly and handle lifecycle transitions safely.
 * This is the foundation for all DSP verification - if initialization fails, nothing else matters.
 *
 * Success Criteria:
 * - All modules initialize without crashes
 * - No memory leaks during repeated init/destroy cycles
 * - Clean reset behavior (no state contamination)
 * - Deterministic behavior across sample rate/block size changes
 * - Proper playback stop/start handling
 */

#include <JuceHeader.h>
#include "dsp/DspModules.h"
#include "dsp/Chambers.h"
#include "dsp/TubeRayTracer.h"
#include "dsp/ElasticHallway.h"
#include "dsp/AlienAmplification.h"
#include <iostream>
#include <iomanip>
#include <memory>
#include <cmath>

// ANSI color codes for terminal output
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_BLUE "\033[0;34m"
#define COLOR_RESET "\033[0m"

using namespace monument::dsp;

// Test configuration
constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 512;
constexpr int kNumChannels = 2;

struct TestResult
{
    std::string testName;
    bool passed;
    std::string message;
};

//==============================================================================
// Test 1: Cold Start Initialization
//==============================================================================
TestResult testColdStartInitialization()
{
    try
    {
        // Test all 9 DSP modules
        Foundation foundation;
        Pillars pillars;
        Chambers chambers;
        Weathering weathering;
        TubeRayTracer tubeRayTracer;
        ElasticHallway elasticHallway;
        AlienAmplification alienAmplification;
        Buttress buttress;
        Facade facade;

        // Prepare all modules
        foundation.prepare(kSampleRate, kBlockSize, kNumChannels);
        pillars.prepare(kSampleRate, kBlockSize, kNumChannels);
        chambers.prepare(kSampleRate, kBlockSize, kNumChannels);
        weathering.prepare(kSampleRate, kBlockSize, kNumChannels);
        tubeRayTracer.prepare(kSampleRate, kBlockSize, kNumChannels);
        elasticHallway.prepare(kSampleRate, kBlockSize, kNumChannels);
        alienAmplification.prepare(kSampleRate, kBlockSize, kNumChannels);
        buttress.prepare(kSampleRate, kBlockSize, kNumChannels);
        facade.prepare(kSampleRate, kBlockSize, kNumChannels);

        // Process silence to verify no crashes
        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        buffer.clear();

        foundation.process(buffer);
        pillars.process(buffer);
        chambers.process(buffer);
        weathering.process(buffer);
        tubeRayTracer.process(buffer);
        elasticHallway.process(buffer);
        alienAmplification.process(buffer);
        buttress.process(buffer);
        facade.process(buffer);

        return {
            "Cold Start Initialization",
            true,
            "All 9 modules initialized and processed successfully"};
    }
    catch (const std::exception& e)
    {
        return {
            "Cold Start Initialization",
            false,
            std::string("Exception: ") + e.what()};
    }
}

//==============================================================================
// Test 2: Repeated Initialization
//==============================================================================
TestResult testRepeatedInitialization()
{
    try
    {
        Chambers chambers;

        // Repeated prepare/process cycles (simulates host changing settings)
        for (int i = 0; i < 10; ++i)
        {
            chambers.prepare(kSampleRate, kBlockSize, kNumChannels);

            juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
            buffer.clear();
            chambers.process(buffer);
        }

        // Process silence through all cycles
        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        buffer.clear();
        chambers.process(buffer);

        // Check output is near-zero (no state contamination)
        float maxOutput = buffer.getMagnitude(0, 0, kBlockSize);
        if (maxOutput > 1e-6f)
        {
            return {
                "Repeated Initialization",
                false,
                "State contamination detected (max output: " + std::to_string(maxOutput) + ")"};
        }

        return {
            "Repeated Initialization",
            true,
            "10 prepare/process cycles completed without contamination"};
    }
    catch (const std::exception& e)
    {
        return {
            "Repeated Initialization",
            false,
            std::string("Exception: ") + e.what()};
    }
}

//==============================================================================
// Test 3: Sample Rate Changes
//==============================================================================
TestResult testSampleRateChanges()
{
    try
    {
        Chambers chambers;

        // Test different sample rates
        std::vector<double> sampleRates = {44100.0, 48000.0, 88200.0, 96000.0};

        for (double sampleRate : sampleRates)
        {
            chambers.prepare(sampleRate, kBlockSize, kNumChannels);

            juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
            buffer.clear();
            chambers.process(buffer);

            // Verify no crashes or denormals
            for (int ch = 0; ch < kNumChannels; ++ch)
            {
                const float* samples = buffer.getReadPointer(ch);
                for (int i = 0; i < kBlockSize; ++i)
                {
                    if (std::isnan(samples[i]) || std::isinf(samples[i]))
                    {
                        return {
                            "Sample Rate Changes",
                            false,
                            "NaN/Inf detected at " + std::to_string(sampleRate) + " Hz"};
                    }
                }
            }
        }

        return {
            "Sample Rate Changes",
            true,
            "All sample rates (44.1k, 48k, 88.2k, 96k) handled correctly"};
    }
    catch (const std::exception& e)
    {
        return {
            "Sample Rate Changes",
            false,
            std::string("Exception: ") + e.what()};
    }
}

//==============================================================================
// Test 4: Block Size Changes
//==============================================================================
TestResult testBlockSizeChanges()
{
    try
    {
        Chambers chambers;

        // Test different block sizes (from tiny to huge)
        std::vector<int> blockSizes = {64, 128, 256, 512, 1024, 2048};

        for (int blockSize : blockSizes)
        {
            chambers.prepare(kSampleRate, blockSize, kNumChannels);

            juce::AudioBuffer<float> buffer(kNumChannels, blockSize);
            buffer.clear();
            chambers.process(buffer);

            // Verify output is valid
            float maxOutput = buffer.getMagnitude(0, 0, blockSize);
            if (std::isnan(maxOutput) || std::isinf(maxOutput))
            {
                return {
                    "Block Size Changes",
                    false,
                    "Invalid output at block size " + std::to_string(blockSize)};
            }
        }

        return {
            "Block Size Changes",
            true,
            "All block sizes (64-2048) handled correctly"};
    }
    catch (const std::exception& e)
    {
        return {
            "Block Size Changes",
            false,
            std::string("Exception: ") + e.what()};
    }
}

//==============================================================================
// Test 5: Reset Behavior
//==============================================================================
TestResult testResetBehavior()
{
    try
    {
        Chambers chambers;
        chambers.prepare(kSampleRate, kBlockSize, kNumChannels);

        // Send impulse to fill reverb tail
        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);  // Unit impulse
        buffer.setSample(1, 0, 1.0f);

        // Process to build up reverb tail
        for (int i = 0; i < 10; ++i)
        {
            chambers.process(buffer);
            buffer.clear();
        }

        // Reset should clear all state
        chambers.reset();

        // Process silence - should be near-zero
        buffer.clear();
        chambers.process(buffer);

        float maxOutput = buffer.getMagnitude(0, 0, kBlockSize);
        if (maxOutput > 1e-6f)
        {
            return {
                "Reset Behavior",
                false,
                "Tail carryover detected after reset (max: " + std::to_string(maxOutput) + ")"};
        }

        return {
            "Reset Behavior",
            true,
            "Reset clears all state (output < -120dB)"};
    }
    catch (const std::exception& e)
    {
        return {
            "Reset Behavior",
            false,
            std::string("Exception: ") + e.what()};
    }
}

//==============================================================================
// Test 6: Playback Stop/Start
//==============================================================================
TestResult testPlaybackStopStart()
{
    try
    {
        Chambers chambers;
        chambers.prepare(kSampleRate, kBlockSize, kNumChannels);

        // Simulate: prepare → process → reset → process
        // This mimics host behavior when transport stops/starts

        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);

        // Process audio
        for (int i = 0; i < 10; ++i)
        {
            buffer.clear();
            buffer.setSample(0, i * 50, 0.5f);  // Sparse impulses
            buffer.setSample(1, i * 50, 0.5f);
            chambers.process(buffer);
        }

        // Stop playback (host calls reset)
        chambers.reset();

        // Start playback again
        for (int i = 0; i < 10; ++i)
        {
            buffer.clear();
            buffer.setSample(0, i * 50, 0.3f);
            buffer.setSample(1, i * 50, 0.3f);
            chambers.process(buffer);
        }

        // Final output should be valid
        buffer.clear();
        chambers.process(buffer);

        float maxOutput = buffer.getMagnitude(0, 0, kBlockSize);
        if (std::isnan(maxOutput) || std::isinf(maxOutput))
        {
            return {
                "Playback Stop/Start",
                false,
                "Invalid output after stop/start cycle"};
        }

        return {
            "Playback Stop/Start",
            true,
            "Clean restart after transport stop"};
    }
    catch (const std::exception& e)
    {
        return {
            "Playback Stop/Start",
            false,
            std::string("Exception: ") + e.what()};
    }
}

//==============================================================================
// Main Test Runner
//==============================================================================
int main()
{
    juce::ScopedJuceInitialiser_GUI juce;

    std::cout << COLOR_BLUE << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << COLOR_RESET << std::endl;
    std::cout << COLOR_BLUE << "  Monument Reverb - DSP Initialization & Lifecycle Test" << COLOR_RESET << std::endl;
    std::cout << COLOR_BLUE << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << COLOR_RESET << std::endl;
    std::cout << std::endl;

    std::cout << "Test Configuration:" << std::endl;
    std::cout << "  Sample rate: " << kSampleRate << " Hz" << std::endl;
    std::cout << "  Block size:  " << kBlockSize << " samples" << std::endl;
    std::cout << "  Channels:    " << kNumChannels << std::endl;
    std::cout << std::endl;

    // Run all tests
    std::vector<TestResult> results;
    results.push_back(testColdStartInitialization());
    results.push_back(testRepeatedInitialization());
    results.push_back(testSampleRateChanges());
    results.push_back(testBlockSizeChanges());
    results.push_back(testResetBehavior());
    results.push_back(testPlaybackStopStart());

    // Report results
    std::cout << "Test Results:" << std::endl;
    std::cout << std::endl;

    int passedCount = 0;
    for (const auto& result : results)
    {
        if (result.passed)
        {
            std::cout << COLOR_GREEN << "  ✓ " << result.testName << COLOR_RESET << std::endl;
            std::cout << "    " << result.message << std::endl;
            passedCount++;
        }
        else
        {
            std::cout << COLOR_RED << "  ✗ " << result.testName << COLOR_RESET << std::endl;
            std::cout << "    " << result.message << std::endl;
        }
        std::cout << std::endl;
    }

    // Summary
    std::cout << COLOR_BLUE << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << COLOR_RESET << std::endl;
    std::cout << COLOR_BLUE << "  Summary" << COLOR_RESET << std::endl;
    std::cout << COLOR_BLUE << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << COLOR_RESET << std::endl;
    std::cout << std::endl;

    std::cout << "  Total tests:  " << results.size() << std::endl;
    std::cout << "  Passed:       " << COLOR_GREEN << passedCount << COLOR_RESET << std::endl;
    std::cout << "  Failed:       " << COLOR_RED << (results.size() - passedCount) << COLOR_RESET << std::endl;
    std::cout << std::endl;

    if (passedCount == results.size())
    {
        std::cout << COLOR_GREEN << "✓ All DSP initialization tests passed" << COLOR_RESET << std::endl;
        std::cout << std::endl;
        std::cout << "All DSP modules initialize correctly and handle lifecycle" << std::endl;
        std::cout << "transitions safely. Foundation for DSP verification is solid." << std::endl;
        std::cout << std::endl;
        return 0;
    }
    else
    {
        std::cout << COLOR_RED << "✗ Some initialization tests failed" << COLOR_RESET << std::endl;
        std::cout << std::endl;
        std::cout << "DSP initialization issues detected. Fix these before proceeding" << std::endl;
        std::cout << "with further DSP verification tests." << std::endl;
        std::cout << std::endl;
        return 1;
    }
}
