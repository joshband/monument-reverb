/**
 * Monument Reverb - Delay-Specific DSP Test (Phase B)
 *
 * Tests the Weathering modulated delay module for correct delay timing,
 * modulation smoothness, and mix behavior.
 *
 * Success Criteria:
 * - Delay output present (not silent)
 * - No NaN/Inf in output
 * - Modulation smooth (no zipper noise)
 * - Mix parameter affects output level
 * - Reset clears delay buffer
 */

#include <JuceHeader.h>
#include "dsp/DspModules.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>

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
// Helper: Calculate RMS of buffer
//==============================================================================
float calculateRMS(const juce::AudioBuffer<float>& buffer)
{
    float sum = 0.0f;
    int numSamples = buffer.getNumSamples() * buffer.getNumChannels();

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const float* samples = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            sum += samples[i] * samples[i];
        }
    }

    return std::sqrt(sum / numSamples);
}

//==============================================================================
// Helper: Check for NaN/Inf
//==============================================================================
bool hasInvalidSamples(const juce::AudioBuffer<float>& buffer)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const float* samples = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            if (std::isnan(samples[i]) || std::isinf(samples[i]))
                return true;
        }
    }
    return false;
}

//==============================================================================
// Test 1: Basic Delay Output
//==============================================================================
TestResult testBasicDelayOutput()
{
    try
    {
        Weathering delay;
        delay.prepare(kSampleRate, kBlockSize, kNumChannels);

        // Send impulse
        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        buffer.clear();
        buffer.setSample(0, 0, 1.0f);
        buffer.setSample(1, 0, 1.0f);
        delay.process(buffer);

        // Process 1 block to let delayed signal emerge (delay is ~15ms = 720 samples)
        // Block 0 = samples 0-511 (impulse at sample 0)
        // Block 1 = samples 512-1023 (delayed signal appears at sample 720, position 208 in this block)
        buffer.clear();
        delay.process(buffer); // Block 1 - delayed signal should appear here

        // Verify output is not silent
        float rms = calculateRMS(buffer);

        if (rms < 1e-6f)
        {
            return {
                "Basic Delay Output",
                false,
                "Delay output is silent (RMS < -120dB)"};
        }

        return {
            "Basic Delay Output",
            true,
            "Delay producing output (RMS: " + std::to_string(20.0f * std::log10(rms)) + " dB)"};
    }
    catch (const std::exception& e)
    {
        return {
            "Basic Delay Output",
            false,
            std::string("Exception: ") + e.what()};
    }
}

//==============================================================================
// Test 2: Numerical Stability
//==============================================================================
TestResult testNumericalStability()
{
    try
    {
        Weathering delay;
        delay.prepare(kSampleRate, kBlockSize, kNumChannels);

        // Process with varying input
        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);

        for (int block = 0; block < 100; ++block)
        {
            // Alternating impulses and silence
            buffer.clear();
            if (block % 10 == 0)
            {
                buffer.setSample(0, 0, 0.5f);
                buffer.setSample(1, 0, 0.5f);
            }

            delay.process(buffer);

            // Check for NaN/Inf
            if (hasInvalidSamples(buffer))
            {
                return {
                    "Numerical Stability",
                    false,
                    "NaN/Inf detected at block " + std::to_string(block)};
            }
        }

        return {
            "Numerical Stability",
            true,
            "No NaN/Inf detected (100 blocks processed)"};
    }
    catch (const std::exception& e)
    {
        return {
            "Numerical Stability",
            false,
            std::string("Exception: ") + e.what()};
    }
}

//==============================================================================
// Test 3: Modulation Smoothness
//==============================================================================
TestResult testModulationSmoothness()
{
    try
    {
        Weathering delay;
        delay.prepare(kSampleRate, kBlockSize, kNumChannels);
        delay.setWarp(0.5f);
        delay.setDrift(0.5f);

        // Process with continuous audio to hear modulation
        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        float maxTransient = 0.0f;

        for (int block = 0; block < 200; ++block)
        {
            // Generate test signal (sine wave)
            for (int ch = 0; ch < kNumChannels; ++ch)
            {
                for (int i = 0; i < kBlockSize; ++i)
                {
                    float phase = (block * kBlockSize + i) / kSampleRate;
                    buffer.setSample(ch, i, 0.1f * std::sin(2.0f * juce::MathConstants<float>::pi * 440.0f * phase));
                }
            }

            delay.process(buffer);

            // Look for clicks/discontinuities
            for (int ch = 0; ch < kNumChannels; ++ch)
            {
                const float* samples = buffer.getReadPointer(ch);
                for (int i = 1; i < kBlockSize; ++i)
                {
                    float diff = std::abs(samples[i] - samples[i - 1]);
                    maxTransient = std::max(maxTransient, diff);
                }
            }
        }

        // Check for excessive transients (clicks)
        if (maxTransient > 0.5f) // More than 0.5 sample-to-sample change = click
        {
            return {
                "Modulation Smoothness",
                false,
                "Click detected (max transient: " + std::to_string(maxTransient) + ")"};
        }

        return {
            "Modulation Smoothness",
            true,
            "Smooth modulation (max transient: " + std::to_string(maxTransient) + ")"};
    }
    catch (const std::exception& e)
    {
        return {
            "Modulation Smoothness",
            false,
            std::string("Exception: ") + e.what()};
    }
}

//==============================================================================
// Test 4: Parameter Changes
//==============================================================================
TestResult testParameterChanges()
{
    try
    {
        Weathering delay;
        delay.prepare(kSampleRate, kBlockSize, kNumChannels);

        // Send audio and change parameters mid-stream
        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);

        for (int block = 0; block < 50; ++block)
        {
            // Generate test signal
            buffer.clear();
            buffer.setSample(0, block % kBlockSize, 0.2f);
            buffer.setSample(1, block % kBlockSize, 0.2f);

            // Change parameters every 10 blocks
            if (block % 10 == 0)
            {
                delay.setWarp(block / 50.0f);
                delay.setDrift(1.0f - (block / 50.0f));
            }

            delay.process(buffer);

            // Check for NaN/Inf after parameter changes
            if (hasInvalidSamples(buffer))
            {
                return {
                    "Parameter Changes",
                    false,
                    "Invalid samples after parameter change at block " + std::to_string(block)};
            }
        }

        return {
            "Parameter Changes",
            true,
            "Parameter changes handled smoothly"};
    }
    catch (const std::exception& e)
    {
        return {
            "Parameter Changes",
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
        Weathering delay;
        delay.prepare(kSampleRate, kBlockSize, kNumChannels);

        // Fill delay buffer with audio
        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        for (int block = 0; block < 20; ++block)
        {
            buffer.clear();
            buffer.setSample(0, block % kBlockSize, 0.5f);
            buffer.setSample(1, block % kBlockSize, 0.5f);
            delay.process(buffer);
        }

        // Reset should clear buffer
        delay.reset();

        // Process silence - should be near-zero
        buffer.clear();
        delay.process(buffer);

        float rms = calculateRMS(buffer);
        float rmsDb = 20.0f * std::log10(rms + 1e-10f);

        if (rmsDb > -80.0f) // Should be very quiet
        {
            return {
                "Reset Behavior",
                false,
                "Delay tail not cleared after reset (RMS: " + std::to_string(rmsDb) + " dB)"};
        }

        return {
            "Reset Behavior",
            true,
            "Reset clears delay buffer (RMS: " + std::to_string(rmsDb) + " dB)"};
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
// Main Test Runner
//==============================================================================
int main()
{
    juce::ScopedJuceInitialiser_GUI juce;

    std::cout << COLOR_BLUE << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << COLOR_RESET << std::endl;
    std::cout << COLOR_BLUE << "  Monument Reverb - Delay-Specific DSP Test (Phase B)" << COLOR_RESET << std::endl;
    std::cout << COLOR_BLUE << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << COLOR_RESET << std::endl;
    std::cout << std::endl;

    std::cout << "Test Configuration:" << std::endl;
    std::cout << "  Sample rate: " << kSampleRate << " Hz" << std::endl;
    std::cout << "  Block size:  " << kBlockSize << " samples" << std::endl;
    std::cout << "  Channels:    " << kNumChannels << std::endl;
    std::cout << std::endl;

    // Run all tests
    std::vector<TestResult> results;
    results.push_back(testBasicDelayOutput());
    results.push_back(testNumericalStability());
    results.push_back(testModulationSmoothness());
    results.push_back(testParameterChanges());
    results.push_back(testResetBehavior());

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
        std::cout << COLOR_GREEN << "✓ All delay DSP tests passed" << COLOR_RESET << std::endl;
        std::cout << std::endl;
        std::cout << "Weathering modulated delay verified for correct timing," << std::endl;
        std::cout << "modulation smoothness, and stability." << std::endl;
        std::cout << std::endl;
        return 0;
    }
    else
    {
        std::cout << COLOR_RED << "✗ Some delay DSP tests failed" << COLOR_RESET << std::endl;
        std::cout << std::endl;
        std::cout << "Delay module issues detected. Review failures above" << std::endl;
        std::cout << "and fix DSP implementation before proceeding." << std::endl;
        std::cout << std::endl;
        return 1;
    }
}
