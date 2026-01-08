/**
 * Unit test for Facade 3D panning constant power law verification
 *
 * Phase 2: Three-System Plan - Constant Power Panning Test
 *
 * This test verifies that the Facade 3D panning implementation maintains
 * constant perceived loudness using the constant power panning law:
 * L² + R² = 1.0 (constant total energy)
 */

#include <JuceHeader.h>
#include "../dsp/DspModules.h"
#include <iostream>
#include <cmath>

using namespace monument::dsp;

// Test tolerance for floating point comparison
constexpr float kTestTolerance = 0.001f;

static bool testConstantPowerLaw(float azimuthDegrees, float elevationDegrees = 0.0f)
{
    // Create and prepare Facade module
    Facade facade;
    facade.prepare(48000.0, 512, 2);
    facade.setOutputGain(1.0f);  // Ensure output gain is 1.0
    facade.setAir(0.0f);  // Disable air filter for cleaner measurement

    // Enable 3D panning mode
    facade.set3DPanning(true);

    // Set spatial position
    facade.setSpatialPositions(azimuthDegrees, elevationDegrees);

    // Create test buffer filled with unit signal (1.0 in both channels)
    juce::AudioBuffer<float> buffer(2, 512);

    // Process multiple blocks to ensure smoothers have settled
    for (int i = 0; i < 5; ++i)
    {
        for (int sample = 0; sample < 512; ++sample)
        {
            buffer.setSample(0, sample, 1.0f);
            buffer.setSample(1, sample, 1.0f);
        }
        facade.process(buffer);
    }

    // Extract gains from output (mono input 1.0 → panned output)
    // Read from the last sample which should be fully settled
    const float leftGain = buffer.getSample(0, 511);
    const float rightGain = buffer.getSample(1, 511);

    // Verify constant power law with elevation scaling applied
    const float totalPower = leftGain * leftGain + rightGain * rightGain;
    const float elevationRadians = elevationDegrees * juce::MathConstants<float>::pi / 180.0f;
    const float elevationScale = std::max(0.0f, std::cos(elevationRadians));
    const float expectedPower = elevationScale * elevationScale;
    const bool powerLawValid = std::abs(totalPower - expectedPower) < kTestTolerance;

    std::cout << "Azimuth: " << azimuthDegrees << "°, Elevation: " << elevationDegrees << "°\n";
    std::cout << "  Left gain:  " << leftGain << "\n";
    std::cout << "  Right gain: " << rightGain << "\n";
    std::cout << "  Total power (L² + R²): " << totalPower << "\n";
    std::cout << "  Expected power: " << expectedPower << "\n";
    std::cout << "  Constant power law: " << (powerLawValid ? "PASS" : "FAIL") << "\n\n";

    return powerLawValid;
}

static bool testExtremePositions()
{
    std::cout << "Testing extreme positions:\n\n";

    bool allPassed = true;

    // Test full left (-90°)
    Facade facadeLeft;
    facadeLeft.prepare(48000.0, 512, 2);
    facadeLeft.setOutputGain(1.0f);
    facadeLeft.setAir(0.0f);
    facadeLeft.set3DPanning(true);
    facadeLeft.setSpatialPositions(-90.0f, 0.0f);

    juce::AudioBuffer<float> bufferLeft(2, 512);
    for (int i = 0; i < 5; ++i)
    {
        for (int sample = 0; sample < 512; ++sample)
        {
            bufferLeft.setSample(0, sample, 1.0f);
            bufferLeft.setSample(1, sample, 1.0f);
        }
        facadeLeft.process(bufferLeft);
    }

    const float leftGainAtLeft = bufferLeft.getSample(0, 511);
    const float rightGainAtLeft = bufferLeft.getSample(1, 511);

    const bool leftExtremeValid = (leftGainAtLeft > 0.95f) && (rightGainAtLeft < 0.1f);
    std::cout << "Full left (-90°): L=" << leftGainAtLeft << ", R=" << rightGainAtLeft
              << " [" << (leftExtremeValid ? "PASS" : "FAIL") << "]\n";
    allPassed &= leftExtremeValid;

    // Test full right (+90°)
    Facade facadeRight;
    facadeRight.prepare(48000.0, 512, 2);
    facadeRight.setOutputGain(1.0f);
    facadeRight.setAir(0.0f);
    facadeRight.set3DPanning(true);
    facadeRight.setSpatialPositions(90.0f, 0.0f);

    juce::AudioBuffer<float> bufferRight(2, 512);
    for (int i = 0; i < 5; ++i)
    {
        for (int sample = 0; sample < 512; ++sample)
        {
            bufferRight.setSample(0, sample, 1.0f);
            bufferRight.setSample(1, sample, 1.0f);
        }
        facadeRight.process(bufferRight);
    }

    const float leftGainAtRight = bufferRight.getSample(0, 511);
    const float rightGainAtRight = bufferRight.getSample(1, 511);

    const bool rightExtremeValid = (leftGainAtRight < 0.1f) && (rightGainAtRight > 0.95f);
    std::cout << "Full right (+90°): L=" << leftGainAtRight << ", R=" << rightGainAtRight
              << " [" << (rightExtremeValid ? "PASS" : "FAIL") << "]\n";
    allPassed &= rightExtremeValid;

    // Test center (0°)
    Facade facadeCenter;
    facadeCenter.prepare(48000.0, 512, 2);
    facadeCenter.setOutputGain(1.0f);
    facadeCenter.setAir(0.0f);
    facadeCenter.set3DPanning(true);
    facadeCenter.setSpatialPositions(0.0f, 0.0f);

    juce::AudioBuffer<float> bufferCenter(2, 512);
    for (int i = 0; i < 5; ++i)
    {
        for (int sample = 0; sample < 512; ++sample)
        {
            bufferCenter.setSample(0, sample, 1.0f);
            bufferCenter.setSample(1, sample, 1.0f);
        }
        facadeCenter.process(bufferCenter);
    }

    const float leftGainAtCenter = bufferCenter.getSample(0, 511);
    const float rightGainAtCenter = bufferCenter.getSample(1, 511);

    const bool centerValid = std::abs(leftGainAtCenter - 0.707f) < kTestTolerance &&
                             std::abs(rightGainAtCenter - 0.707f) < kTestTolerance;
    std::cout << "Center (0°): L=" << leftGainAtCenter << ", R=" << rightGainAtCenter
              << " [" << (centerValid ? "PASS" : "FAIL") << "]\n\n";
    allPassed &= centerValid;

    return allPassed;
}

int main()
{
    std::cout << "========================================\n";
    std::cout << "Constant Power Panning Test (Phase 2)\n";
    std::cout << "========================================\n\n";

    bool allTestsPassed = true;

    // Test various azimuth positions at elevation 0°
    std::cout << "Testing constant power law at various azimuth angles:\n\n";
    allTestsPassed &= testConstantPowerLaw(-90.0f, 0.0f);  // Full left
    allTestsPassed &= testConstantPowerLaw(-45.0f, 0.0f);  // Mid-left
    allTestsPassed &= testConstantPowerLaw(0.0f, 0.0f);    // Center
    allTestsPassed &= testConstantPowerLaw(45.0f, 0.0f);   // Mid-right
    allTestsPassed &= testConstantPowerLaw(90.0f, 0.0f);   // Full right

    // Test elevation scaling (should reduce overall level but maintain ratio)
    std::cout << "Testing elevation scaling:\n\n";
    allTestsPassed &= testConstantPowerLaw(0.0f, 30.0f);   // Center, elevated 30°
    allTestsPassed &= testConstantPowerLaw(0.0f, 60.0f);   // Center, elevated 60°
    allTestsPassed &= testConstantPowerLaw(0.0f, 90.0f);   // Center, directly above

    // Test extreme positions
    allTestsPassed &= testExtremePositions();

    std::cout << "========================================\n";
    if (allTestsPassed)
    {
        std::cout << "✅ All constant power panning tests PASSED\n";
        return 0;
    }
    else
    {
        std::cout << "❌ Some tests FAILED\n";
        return 1;
    }
}
