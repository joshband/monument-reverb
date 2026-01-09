/**
 * Monument Reverb - Spatial DSP Test (Phase S)
 *
 * Tests the SpatialProcessor for correct 3D positioning behavior including
 * distance attenuation, Doppler shift, and energy invariance.
 *
 * Success Criteria:
 * - Distance attenuation follows inverse square law
 * - Doppler shift bounded and correct
 * - Total energy preserved across positions
 * - No NaN/Inf in calculations
 * - Reset clears spatial state
 */

#include <JuceHeader.h>
#include "dsp/SpatialProcessor.h"
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
constexpr int kNumLines = 8;

struct TestResult
{
    std::string testName;
    bool passed;
    std::string message;
};

//==============================================================================
// Test 1: Distance Attenuation (Inverse Square Law)
//==============================================================================
TestResult testDistanceAttenuation()
{
    try
    {
        SpatialProcessor spatial;
        spatial.prepare(kSampleRate, kBlockSize, kNumLines);

        // Position line 0 at origin (0, 0, 0) - closest
        spatial.setPosition(0, 0.0f, 0.0f, 0.0f);

        // Position line 1 at distance 1.0
        spatial.setPosition(1, 1.0f, 0.0f, 0.0f);

        // Position line 2 at distance 2.0 (should be 1/4 the gain of line 1)
        spatial.setPosition(2, 2.0f, 0.0f, 0.0f);

        spatial.process();

        float gain0 = spatial.getAttenuationGain(0);
        float gain1 = spatial.getAttenuationGain(1);
        float gain2 = spatial.getAttenuationGain(2);

        // Verify inverse square law (approximately)
        // gain2 should be roughly 1/4 of gain1
        float ratio = gain2 / (gain1 + 1e-10f);

        if (std::isnan(gain0) || std::isnan(gain1) || std::isnan(gain2))
        {
            return {
                "Distance Attenuation",
                false,
                "NaN detected in attenuation gains"};
        }

        if (gain0 < gain1 || gain1 < gain2)
        {
            return {
                "Distance Attenuation",
                false,
                "Attenuation not decreasing with distance"};
        }

        // Verify rough inverse square behavior (ratio should be near 0.25)
        if (ratio < 0.1f || ratio > 0.4f)
        {
            return {
                "Distance Attenuation",
                false,
                "Inverse square law not followed (ratio: " + std::to_string(ratio) + ", expected ~0.25)"};
        }

        return {
            "Distance Attenuation",
            true,
            "Inverse square law verified (gain0=" + std::to_string(gain0) +
            ", gain1=" + std::to_string(gain1) +
            ", gain2=" + std::to_string(gain2) + ", ratio=" + std::to_string(ratio) + ")"};
    }
    catch (const std::exception& e)
    {
        return {
            "Distance Attenuation",
            false,
            std::string("Exception: ") + e.what()};
    }
}

//==============================================================================
// Test 2: Doppler Shift Calculation
//==============================================================================
TestResult testDopplerShiftCalculation()
{
    try
    {
        SpatialProcessor spatial;
        spatial.prepare(kSampleRate, kBlockSize, kNumLines);

        // Set velocity for line 0: moving away (+X direction)
        spatial.setPosition(0, 0.0f, 0.0f, 0.0f);
        spatial.setVelocity(0, 1.0f);

        // Set velocity for line 1: moving toward (-X direction)
        spatial.setPosition(1, 0.0f, 0.0f, 0.0f);
        spatial.setVelocity(1, -1.0f);

        spatial.process();

        float doppler0 = spatial.getDopplerShift(0);
        float doppler1 = spatial.getDopplerShift(1);

        // Check for NaN/Inf
        if (std::isnan(doppler0) || std::isinf(doppler0) ||
            std::isnan(doppler1) || std::isinf(doppler1))
        {
            return {
                "Doppler Shift Calculation",
                false,
                "NaN/Inf detected in Doppler shifts"};
        }

        // Doppler shift should be bounded (±2400 samples @ 48kHz = ±50ms)
        if (std::abs(doppler0) > 2400.0f || std::abs(doppler1) > 2400.0f)
        {
            return {
                "Doppler Shift Calculation",
                false,
                "Doppler shift out of bounds (doppler0=" + std::to_string(doppler0) +
                ", doppler1=" + std::to_string(doppler1) + ")"};
        }

        // Moving away should have positive shift, moving toward negative
        if (doppler0 < 0.0f || doppler1 > 0.0f)
        {
            return {
                "Doppler Shift Calculation",
                false,
                "Doppler shift direction incorrect"};
        }

        return {
            "Doppler Shift Calculation",
            true,
            "Doppler shift bounded and correct (doppler0=" + std::to_string(doppler0) +
            ", doppler1=" + std::to_string(doppler1) + ")"};
    }
    catch (const std::exception& e)
    {
        return {
            "Doppler Shift Calculation",
            false,
            std::string("Exception: ") + e.what()};
    }
}

//==============================================================================
// Test 3: Energy Invariance Across Positions
//==============================================================================
TestResult testEnergyInvariance()
{
    try
    {
        SpatialProcessor spatial;
        spatial.prepare(kSampleRate, kBlockSize, kNumLines);

        // Calculate total energy for configuration A
        for (int i = 0; i < kNumLines; ++i)
        {
            float angle = (i / (float)kNumLines) * 2.0f * juce::MathConstants<float>::pi;
            spatial.setPosition(i, std::cos(angle), std::sin(angle), 0.5f);
        }
        spatial.process();

        float totalEnergyA = 0.0f;
        for (int i = 0; i < kNumLines; ++i)
        {
            float gain = spatial.getAttenuationGain(i);
            totalEnergyA += gain * gain; // Energy is proportional to gain²
        }

        // Calculate total energy for configuration B (different positions)
        for (int i = 0; i < kNumLines; ++i)
        {
            spatial.setPosition(i, 0.0f, 0.0f, i / (float)kNumLines);
        }
        spatial.process();

        float totalEnergyB = 0.0f;
        for (int i = 0; i < kNumLines; ++i)
        {
            float gain = spatial.getAttenuationGain(i);
            totalEnergyB += gain * gain;
        }

        // Energy should be relatively stable (within ±6dB = 4x ratio)
        float energyRatio = totalEnergyB / (totalEnergyA + 1e-10f);

        if (energyRatio < 0.25f || energyRatio > 4.0f) // ±6dB tolerance
        {
            return {
                "Energy Invariance",
                false,
                "Total energy varies too much (ratio: " + std::to_string(energyRatio) + ")"};
        }

        return {
            "Energy Invariance",
            true,
            "Total energy stable (ratio: " + std::to_string(energyRatio) + ", within ±6dB)"};
    }
    catch (const std::exception& e)
    {
        return {
            "Energy Invariance",
            false,
            std::string("Exception: ") + e.what()};
    }
}

//==============================================================================
// Test 4: Numerical Stability
//==============================================================================
TestResult testNumericalStability()
{
    try
    {
        SpatialProcessor spatial;
        spatial.prepare(kSampleRate, kBlockSize, kNumLines);

        // Test with extreme positions and velocities
        for (int block = 0; block < 100; ++block)
        {
            for (int i = 0; i < kNumLines; ++i)
            {
                // Random positions
                float x = (block % 3 - 1.0f);
                float y = ((block + i) % 3 - 1.0f);
                float z = ((block * i) % 10) / 10.0f;
                spatial.setPosition(i, x, y, z);

                // Random velocities (X-axis only)
                float vx = (block % 2 == 0) ? 1.0f : -1.0f;
                spatial.setVelocity(i, vx);
            }

            spatial.process();

            // Check for NaN/Inf
            for (int i = 0; i < kNumLines; ++i)
            {
                float gain = spatial.getAttenuationGain(i);
                float doppler = spatial.getDopplerShift(i);

                if (std::isnan(gain) || std::isinf(gain) ||
                    std::isnan(doppler) || std::isinf(doppler))
                {
                    return {
                        "Numerical Stability",
                        false,
                        "NaN/Inf detected at block " + std::to_string(block) + ", line " + std::to_string(i)};
                }
            }
        }

        return {
            "Numerical Stability",
            true,
            "No NaN/Inf detected (100 blocks, varying positions/velocities)"};
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
// Test 5: Reset Behavior
//==============================================================================
TestResult testResetBehavior()
{
    try
    {
        SpatialProcessor spatial;
        spatial.prepare(kSampleRate, kBlockSize, kNumLines);

        // Set non-default positions and velocities
        for (int i = 0; i < kNumLines; ++i)
        {
            spatial.setPosition(i, 1.0f, 0.5f, 0.25f);
            spatial.setVelocity(i, 0.5f);
        }
        spatial.process();

        // Reset should restore defaults
        spatial.reset();
        spatial.process();

        // Check that all gains are at default (should be similar for centered positions)
        float firstGain = spatial.getAttenuationGain(0);
        bool allSimilar = true;

        for (int i = 1; i < kNumLines; ++i)
        {
            float gain = spatial.getAttenuationGain(i);
            if (std::abs(gain - firstGain) > 0.1f) // Allow 10% variation
            {
                allSimilar = false;
                break;
            }
        }

        if (!allSimilar)
        {
            return {
                "Reset Behavior",
                false,
                "Gains not uniform after reset (expected centered positions)"};
        }

        // Check Doppler shifts are near zero
        float maxDoppler = 0.0f;
        for (int i = 0; i < kNumLines; ++i)
        {
            float doppler = std::abs(spatial.getDopplerShift(i));
            maxDoppler = std::max(maxDoppler, doppler);
        }

        if (maxDoppler > 1.0f) // Should be very small
        {
            return {
                "Reset Behavior",
                false,
                "Doppler shifts not cleared after reset (max: " + std::to_string(maxDoppler) + ")"};
        }

        return {
            "Reset Behavior",
            true,
            "Reset cleared spatial state (uniform gains, zero Doppler)"};
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
    std::cout << COLOR_BLUE << "  Monument Reverb - Spatial DSP Test (Phase S)" << COLOR_RESET << std::endl;
    std::cout << COLOR_BLUE << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << COLOR_RESET << std::endl;
    std::cout << std::endl;

    std::cout << "Test Configuration:" << std::endl;
    std::cout << "  Sample rate: " << kSampleRate << " Hz" << std::endl;
    std::cout << "  Block size:  " << kBlockSize << " samples" << std::endl;
    std::cout << "  Delay lines: " << kNumLines << std::endl;
    std::cout << std::endl;

    // Run all tests
    std::vector<TestResult> results;
    results.push_back(testDistanceAttenuation());
    results.push_back(testDopplerShiftCalculation());
    results.push_back(testEnergyInvariance());
    results.push_back(testNumericalStability());
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
        std::cout << COLOR_GREEN << "✓ All spatial DSP tests passed" << COLOR_RESET << std::endl;
        std::cout << std::endl;
        std::cout << "SpatialProcessor verified for correct 3D positioning," << std::endl;
        std::cout << "distance attenuation, Doppler shift, and stability." << std::endl;
        std::cout << std::endl;
        return 0;
    }
    else
    {
        std::cout << COLOR_RED << "✗ Some spatial DSP tests failed" << COLOR_RESET << std::endl;
        std::cout << std::endl;
        std::cout << "Spatial processing issues detected. Review failures above" << std::endl;
        std::cout << "and fix implementation before proceeding." << std::endl;
        std::cout << std::endl;
        return 1;
    }
}
