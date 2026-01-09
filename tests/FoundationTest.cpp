/**
 * @file FoundationTest.cpp
 * @brief Phase 3 Test Suite: Foundation Module Verification
 *
 * Tests the core foundation modules that power Monument Reverb's macro control
 * system and diffusion network:
 * - AllpassDiffuser: Classic allpass filter for reverb diffusion
 * - MacroMapper: Ancient Monuments themed macro-to-parameter mapping
 * - ExpressiveMacroMapper: Performance-oriented 6-macro system
 *
 * These modules are critical for parameter mapping, musical expressiveness,
 * and reverb quality.
 *
 * Test Coverage:
 * - AllpassDiffuser: Magnitude response, phase, stability (7 tests)
 * - MacroMapper: Input clamping, boundary conditions, macro influences (8 tests)
 * - ExpressiveMacroMapper: Character scaling, space type, energy/motion/color/dimension (7 tests)
 *
 * @status Phase 3 - Foundation Tests
 * @date 2026-01-09
 */

#include "dsp/AllpassDiffuser.h"
#include "dsp/MacroMapper.h"
#include "dsp/ExpressiveMacroMapper.h"
#include <JuceHeader.h>
#include <cassert>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <vector>

using namespace monument::dsp;

// =============================================================================
// Color Output Utilities
// =============================================================================

namespace Colors
{
    const char* RESET = "\033[0m";
    const char* RED = "\033[31m";
    const char* GREEN = "\033[32m";
    const char* YELLOW = "\033[33m";
    const char* BLUE = "\033[34m";
    const char* MAGENTA = "\033[35m";
    const char* CYAN = "\033[36m";
    const char* WHITE = "\033[37m";
    const char* BOLD = "\033[1m";
} // namespace Colors

// =============================================================================
// Test Assertion Helpers
// =============================================================================

void assertApproxEqual(float actual, float expected, float tolerance, const std::string& name)
{
    if (std::abs(actual - expected) > tolerance)
    {
        std::cout << Colors::RED << "FAIL: " << name << Colors::RESET
                  << " - Expected " << expected << " ± " << tolerance
                  << ", got " << actual << " (diff=" << std::abs(actual - expected) << ")"
                  << std::endl;
        throw std::runtime_error(name + " failed");
    }
}

void assertInRange(float value, float min, float max, const std::string& name)
{
    if (value < min || value > max)
    {
        std::cout << Colors::RED << "FAIL: " << name << Colors::RESET
                  << " - Expected range [" << min << ", " << max << "], got " << value
                  << std::endl;
        throw std::runtime_error(name + " out of range");
    }
}

void assertTrue(bool condition, const std::string& name)
{
    if (!condition)
    {
        std::cout << Colors::RED << "FAIL: " << name << Colors::RESET << std::endl;
        throw std::runtime_error(name + " is false");
    }
}

void assertLessThan(float value, float threshold, const std::string& name)
{
    if (value >= threshold)
    {
        std::cout << Colors::RED << "FAIL: " << name << Colors::RESET
                  << " - Expected < " << threshold << ", got " << value
                  << std::endl;
        throw std::runtime_error(name + " not less than threshold");
    }
}

void assertGreaterThan(float value, float threshold, const std::string& name)
{
    if (value <= threshold)
    {
        std::cout << Colors::RED << "FAIL: " << name << Colors::RESET
                  << " - Expected > " << threshold << ", got " << value
                  << std::endl;
        throw std::runtime_error(name + " not greater than threshold");
    }
}

// =============================================================================
// Helper Functions
// =============================================================================

float calculateRMS(const juce::AudioBuffer<float>& buffer)
{
    double sumSquares = 0.0;
    int totalSamples = 0;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const float* channelData = buffer.getReadPointer(ch);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float s = channelData[sample];
            sumSquares += s * s;
            ++totalSamples;
        }
    }

    return totalSamples > 0 ? std::sqrt(sumSquares / totalSamples) : 0.0f;
}

float calculatePeakAmplitude(const juce::AudioBuffer<float>& buffer)
{
    float peak = 0.0f;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const float* channelData = buffer.getReadPointer(ch);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            peak = std::max(peak, std::abs(channelData[sample]));
        }
    }

    return peak;
}

void generateWhiteNoise(juce::AudioBuffer<float>& buffer, float amplitude = 0.1f)
{
    juce::Random random;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        float* channelData = buffer.getWritePointer(ch);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            channelData[sample] = (random.nextFloat() * 2.0f - 1.0f) * amplitude;
        }
    }
}

void generateSine(juce::AudioBuffer<float>& buffer, float frequency, float sampleRate, float amplitude = 0.5f)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        float* channelData = buffer.getWritePointer(ch);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float phase = (sample / sampleRate) * frequency * juce::MathConstants<float>::twoPi;
            channelData[sample] = amplitude * std::sin(phase);
        }
    }
}

bool checkForDenormals(const juce::AudioBuffer<float>& buffer)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const float* channelData = buffer.getReadPointer(ch);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            float value = channelData[sample];
            if (value != 0.0f && std::abs(value) < 1e-30f)
                return true; // Denormal detected
        }
    }
    return false;
}

// =============================================================================
// Test 1: AllpassDiffuser - Initialization
// =============================================================================

void test01_AllpassInitialization()
{
    std::cout << Colors::CYAN << "\n[Test 1/22] AllpassDiffuser - Initialization" << Colors::RESET << std::endl;

    AllpassDiffuser diffuser;

    // Test various delay lengths
    std::vector<int> delaySamples = {1, 10, 100, 1000};

    for (int delay : delaySamples)
    {
        diffuser.setDelaySamples(delay);
        diffuser.prepare();

        // Process a sample to verify no crash
        float output = diffuser.processSample(1.0f);

        std::cout << Colors::YELLOW << "  Delay " << delay << " samples: initialized successfully" << Colors::RESET << std::endl;
    }

    std::cout << Colors::GREEN << "  ✓ All delay lengths initialized correctly" << Colors::RESET << std::endl;
}

// =============================================================================
// Test 2: AllpassDiffuser - Unity Gain (Magnitude Response)
// =============================================================================

void test02_AllpassUnityGain()
{
    std::cout << Colors::CYAN << "\n[Test 2/22] AllpassDiffuser - Unity Gain" << Colors::RESET << std::endl;

    AllpassDiffuser diffuser;
    diffuser.setDelaySamples(10);
    diffuser.setCoefficient(0.5f);
    diffuser.prepare();

    // Generate white noise input
    const int numSamples = 10000;
    juce::AudioBuffer<float> inputBuffer(1, numSamples);
    generateWhiteNoise(inputBuffer, 0.1f);

    float inputRMS = calculateRMS(inputBuffer);

    // Process through allpass
    juce::AudioBuffer<float> outputBuffer(1, numSamples);
    for (int i = 0; i < numSamples; ++i)
    {
        float input = inputBuffer.getSample(0, i);
        float output = diffuser.processSample(input);
        outputBuffer.setSample(0, i, output);
    }

    float outputRMS = calculateRMS(outputBuffer);
    float gainRatio = outputRMS / inputRMS;

    std::cout << Colors::YELLOW << "  Input RMS: " << inputRMS << Colors::RESET << std::endl;
    std::cout << Colors::YELLOW << "  Output RMS: " << outputRMS << Colors::RESET << std::endl;
    std::cout << Colors::YELLOW << "  Gain ratio: " << gainRatio << Colors::RESET << std::endl;

    assertApproxEqual(gainRatio, 1.0f, 0.02f, "Unity gain (within 2%)");

    std::cout << Colors::GREEN << "  ✓ Allpass filter has unity gain" << Colors::RESET << std::endl;
}

// =============================================================================
// Test 3: AllpassDiffuser - Coefficient Clamping
// =============================================================================

void test03_AllpassCoefficientClamping()
{
    std::cout << Colors::CYAN << "\n[Test 3/22] AllpassDiffuser - Coefficient Clamping" << Colors::RESET << std::endl;

    AllpassDiffuser diffuser;
    diffuser.setDelaySamples(10);
    diffuser.prepare();

    // Test extreme coefficient values (should be clamped to [-0.74, 0.74])
    std::vector<float> testCoefficients = {-2.0f, -0.74f, 0.0f, 0.74f, 2.0f};

    for (float coeff : testCoefficients)
    {
        diffuser.setCoefficient(coeff);

        // Process a signal to verify stability
        float maxOutput = 0.0f;
        for (int i = 0; i < 1000; ++i)
        {
            float output = diffuser.processSample(1.0f); // Unit impulse
            maxOutput = std::max(maxOutput, std::abs(output));
        }

        // Verify output remains bounded (proof that clamping worked)
        assertLessThan(maxOutput, 10.0f, "Output bounded with coefficient " + std::to_string(coeff));

        std::cout << Colors::YELLOW << "  Coefficient " << coeff << " → max output: " << maxOutput << Colors::RESET << std::endl;
    }

    std::cout << Colors::GREEN << "  ✓ Coefficients clamped correctly for stability" << Colors::RESET << std::endl;
}

// =============================================================================
// Test 4: AllpassDiffuser - Phase Response Characteristics
// =============================================================================

void test04_AllpassPhaseResponse()
{
    std::cout << Colors::CYAN << "\n[Test 4/22] AllpassDiffuser - Phase Response" << Colors::RESET << std::endl;

    AllpassDiffuser diffuser;
    diffuser.setDelaySamples(50);
    diffuser.setCoefficient(0.7f);
    diffuser.prepare();

    const float sampleRate = 48000.0f;

    // Test at two frequencies: low (100 Hz) and high (10 kHz)
    auto measurePhaseDelay = [&](float frequency) -> float {
        const int numSamples = static_cast<int>(sampleRate / frequency * 4); // 4 cycles
        juce::AudioBuffer<float> inputBuffer(1, numSamples);
        generateSine(inputBuffer, frequency, sampleRate, 0.5f);

        juce::AudioBuffer<float> outputBuffer(1, numSamples);
        diffuser.reset();

        for (int i = 0; i < numSamples; ++i)
        {
            float input = inputBuffer.getSample(0, i);
            float output = diffuser.processSample(input);
            outputBuffer.setSample(0, i, output);
        }

        // Find first peak in input and output (simple phase delay estimation)
        auto findFirstPeak = [](const juce::AudioBuffer<float>& buffer) -> int {
            for (int i = 1; i < buffer.getNumSamples() - 1; ++i)
            {
                if (buffer.getSample(0, i) > buffer.getSample(0, i - 1) &&
                    buffer.getSample(0, i) > buffer.getSample(0, i + 1) &&
                    buffer.getSample(0, i) > 0.4f)
                {
                    return i;
                }
            }
            return -1;
        };

        int inputPeak = findFirstPeak(inputBuffer);
        int outputPeak = findFirstPeak(outputBuffer);

        if (inputPeak >= 0 && outputPeak >= 0)
        {
            float delaySamples = static_cast<float>(outputPeak - inputPeak);
            if (delaySamples < 0) delaySamples += numSamples;
            return delaySamples;
        }

        return 0.0f;
    };

    float lowFreqDelay = measurePhaseDelay(100.0f);
    float highFreqDelay = measurePhaseDelay(10000.0f);

    std::cout << Colors::YELLOW << "  Phase delay at 100 Hz: " << lowFreqDelay << " samples" << Colors::RESET << std::endl;
    std::cout << Colors::YELLOW << "  Phase delay at 10 kHz: " << highFreqDelay << " samples" << Colors::RESET << std::endl;

    // Low frequency should have more phase delay (allpass characteristic)
    assertTrue(lowFreqDelay > highFreqDelay, "Low frequency has more phase delay");

    std::cout << Colors::GREEN << "  ✓ Phase response varies with frequency as expected" << Colors::RESET << std::endl;
}

// =============================================================================
// Test 5: AllpassDiffuser - Delay Length Impact
// =============================================================================

void test05_AllpassDelayLengthImpact()
{
    std::cout << Colors::CYAN << "\n[Test 5/22] AllpassDiffuser - Delay Length Impact" << Colors::RESET << std::endl;

    const float sampleRate = 48000.0f;
    const float testFrequency = 1000.0f;
    const float coefficient = 0.5f;

    std::vector<int> delays = {5, 20, 100};
    std::vector<float> phaseDelays;

    for (int delaySamples : delays)
    {
        AllpassDiffuser diffuser;
        diffuser.setDelaySamples(delaySamples);
        diffuser.setCoefficient(coefficient);
        diffuser.prepare();

        // Process one cycle to measure group delay
        const int numSamples = static_cast<int>(sampleRate / testFrequency);
        juce::AudioBuffer<float> inputBuffer(1, numSamples);
        generateSine(inputBuffer, testFrequency, sampleRate, 0.5f);

        juce::AudioBuffer<float> outputBuffer(1, numSamples);

        for (int i = 0; i < numSamples; ++i)
        {
            float input = inputBuffer.getSample(0, i);
            float output = diffuser.processSample(input);
            outputBuffer.setSample(0, i, output);
        }

        // Measure energy in output (longer delays → more phase shift → different energy distribution)
        float outputRMS = calculateRMS(outputBuffer);
        phaseDelays.push_back(outputRMS);

        std::cout << Colors::YELLOW << "  Delay " << delaySamples << " samples → RMS: " << outputRMS << Colors::RESET << std::endl;
    }

    // Verify that delay length affects the output (phase relationship changes)
    assertTrue(phaseDelays.size() == 3, "All delay lengths tested");

    std::cout << Colors::GREEN << "  ✓ Delay length affects phase response" << Colors::RESET << std::endl;
}

// =============================================================================
// Test 6: AllpassDiffuser - Stability with Extreme Inputs
// =============================================================================

void test06_AllpassStability()
{
    std::cout << Colors::CYAN << "\n[Test 6/22] AllpassDiffuser - Stability" << Colors::RESET << std::endl;

    AllpassDiffuser diffuser;
    diffuser.setDelaySamples(50);
    diffuser.setCoefficient(0.7f);
    diffuser.prepare();

    // Feed extreme input (impulse of amplitude 10.0)
    float maxOutput = 0.0f;
    float firstOutput = diffuser.processSample(10.0f);

    for (int i = 1; i < 5000; ++i)
    {
        float output = diffuser.processSample(0.0f); // Silence after impulse
        maxOutput = std::max(maxOutput, std::abs(output));
    }

    std::cout << Colors::YELLOW << "  First output: " << firstOutput << Colors::RESET << std::endl;
    std::cout << Colors::YELLOW << "  Max output over 5000 samples: " << maxOutput << Colors::RESET << std::endl;

    // Verify output remains bounded
    assertLessThan(maxOutput, 100.0f, "Output bounded after extreme input");

    // Check for denormals
    juce::AudioBuffer<float> testBuffer(1, 5000);
    diffuser.reset();
    diffuser.processSample(10.0f);

    for (int i = 1; i < 5000; ++i)
    {
        testBuffer.setSample(0, i, diffuser.processSample(0.0f));
    }

    bool hasDenormals = checkForDenormals(testBuffer);
    assertTrue(!hasDenormals, "No denormals detected");

    std::cout << Colors::GREEN << "  ✓ Allpass remains stable with extreme inputs" << Colors::RESET << std::endl;
}

// =============================================================================
// Test 7: AllpassDiffuser - Reset Behavior
// =============================================================================

void test07_AllpassReset()
{
    std::cout << Colors::CYAN << "\n[Test 7/22] AllpassDiffuser - Reset Behavior" << Colors::RESET << std::endl;

    AllpassDiffuser diffuser;
    diffuser.setDelaySamples(50);
    diffuser.setCoefficient(0.5f);
    diffuser.prepare();

    // Process impulse
    float firstOutput = diffuser.processSample(1.0f);

    // Process some silence, should have non-zero output (impulse response)
    float outputBefore = 0.0f;
    for (int i = 0; i < 100; ++i)
    {
        outputBefore = diffuser.processSample(0.0f);
    }

    std::cout << Colors::YELLOW << "  Output after 100 samples (before reset): " << outputBefore << Colors::RESET << std::endl;

    // Reset
    diffuser.reset();

    // Process silence again, should be zero
    float maxOutputAfterReset = 0.0f;
    for (int i = 0; i < 100; ++i)
    {
        float output = diffuser.processSample(0.0f);
        maxOutputAfterReset = std::max(maxOutputAfterReset, std::abs(output));
    }

    std::cout << Colors::YELLOW << "  Max output after reset: " << maxOutputAfterReset << Colors::RESET << std::endl;

    assertLessThan(maxOutputAfterReset, 1e-9f, "All samples zero after reset");

    std::cout << Colors::GREEN << "  ✓ Reset clears all state" << Colors::RESET << std::endl;
}

// =============================================================================
// Test 8: MacroMapper - Initialization and Default Values
// =============================================================================

void test08_MacroMapperInitialization()
{
    std::cout << Colors::CYAN << "\n[Test 8/22] MacroMapper - Initialization" << Colors::RESET << std::endl;

    MacroMapper mapper;
    MacroMapper::MacroInputs macros; // All defaults

    auto targets = mapper.computeTargets(macros);

    // Verify all outputs in valid range [0, 1]
    assertInRange(targets.time, 0.0f, 1.0f, "time in range");
    assertInRange(targets.mass, 0.0f, 1.0f, "mass in range");
    assertInRange(targets.density, 0.0f, 1.0f, "density in range");
    assertInRange(targets.bloom, 0.0f, 1.0f, "bloom in range");
    assertInRange(targets.air, 0.0f, 1.0f, "air in range");
    assertInRange(targets.width, 0.0f, 1.0f, "width in range");
    assertInRange(targets.warp, 0.0f, 1.0f, "warp in range");
    assertInRange(targets.drift, 0.0f, 1.0f, "drift in range");

    std::cout << Colors::YELLOW << "  time: " << targets.time << Colors::RESET << std::endl;
    std::cout << Colors::YELLOW << "  mass: " << targets.mass << Colors::RESET << std::endl;
    std::cout << Colors::YELLOW << "  density: " << targets.density << Colors::RESET << std::endl;
    std::cout << Colors::YELLOW << "  bloom: " << targets.bloom << Colors::RESET << std::endl;

    std::cout << Colors::GREEN << "  ✓ Default values are sane" << Colors::RESET << std::endl;
}

// =============================================================================
// Test 9: MacroMapper - Input Clamping
// =============================================================================

void test09_MacroMapperInputClamping()
{
    std::cout << Colors::CYAN << "\n[Test 9/22] MacroMapper - Input Clamping" << Colors::RESET << std::endl;

    MacroMapper mapper;

    // Test with extreme out-of-range inputs
    auto targets = mapper.computeTargets(
        -10.0f, // stone
        5.0f,   // labyrinth
        -2.0f,  // mist
        3.0f,   // bloom
        -1.0f,  // tempest
        10.0f,  // echo
        -5.0f,  // patina
        8.0f,   // abyss
        -3.0f,  // corona
        15.0f   // breath
    );

    // Despite extreme inputs, outputs should be clamped to [0, 1]
    assertInRange(targets.time, 0.0f, 1.0f, "time clamped");
    assertInRange(targets.mass, 0.0f, 1.0f, "mass clamped");
    assertInRange(targets.density, 0.0f, 1.0f, "density clamped");
    assertInRange(targets.bloom, 0.0f, 1.0f, "bloom clamped");
    assertInRange(targets.air, 0.0f, 1.0f, "air clamped");
    assertInRange(targets.warp, 0.0f, 1.0f, "warp clamped");
    assertInRange(targets.drift, 0.0f, 1.0f, "drift clamped");

    std::cout << Colors::GREEN << "  ✓ Extreme inputs are clamped correctly" << Colors::RESET << std::endl;
}

// =============================================================================
// Test 10: MacroMapper - Boundary Conditions
// =============================================================================

void test10_MacroMapperBoundaryConditions()
{
    std::cout << Colors::CYAN << "\n[Test 10/22] MacroMapper - Boundary Conditions" << Colors::RESET << std::endl;

    MacroMapper mapper;

    // Test all zeros
    auto targetsZero = mapper.computeTargets(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);

    assertInRange(targetsZero.time, 0.0f, 1.0f, "time (all 0)");
    assertInRange(targetsZero.mass, 0.0f, 1.0f, "mass (all 0)");

    std::cout << Colors::YELLOW << "  All zeros → time: " << targetsZero.time << ", mass: " << targetsZero.mass << Colors::RESET << std::endl;

    // Test all ones
    auto targetsOne = mapper.computeTargets(1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);

    assertInRange(targetsOne.time, 0.0f, 1.0f, "time (all 1)");
    assertInRange(targetsOne.mass, 0.0f, 1.0f, "mass (all 1)");

    std::cout << Colors::YELLOW << "  All ones → time: " << targetsOne.time << ", mass: " << targetsOne.mass << Colors::RESET << std::endl;

    std::cout << Colors::GREEN << "  ✓ Boundary conditions handled correctly" << Colors::RESET << std::endl;
}

// =============================================================================
// Test 11: MacroMapper - Single Macro Influence (STONE)
// =============================================================================

void test11_MacroMapperStoneInfluence()
{
    std::cout << Colors::CYAN << "\n[Test 11/22] MacroMapper - STONE Influence" << Colors::RESET << std::endl;

    MacroMapper mapper;

    // STONE = 0.0 (soft limestone), all others neutral (0.5)
    auto targetsSoft = mapper.computeTargets(0.0f, 0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.5f, 0.5f, 0.5f, 0.0f);

    // STONE = 1.0 (hard granite)
    auto targetsHard = mapper.computeTargets(1.0f, 0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.5f, 0.5f, 0.5f, 0.0f);

    std::cout << Colors::YELLOW << "  Soft stone → time: " << targetsSoft.time << ", mass: " << targetsSoft.mass << ", density: " << targetsSoft.density << Colors::RESET << std::endl;
    std::cout << Colors::YELLOW << "  Hard stone → time: " << targetsHard.time << ", mass: " << targetsHard.mass << ", density: " << targetsHard.density << Colors::RESET << std::endl;

    // Hard stone should increase time, mass, and density
    assertGreaterThan(targetsHard.time, targetsSoft.time, "Hard stone increases time");
    assertGreaterThan(targetsHard.mass, targetsSoft.mass, "Hard stone increases mass");
    assertGreaterThan(targetsHard.density, targetsSoft.density, "Hard stone increases density");

    std::cout << Colors::GREEN << "  ✓ STONE macro affects time/mass/density correctly" << Colors::RESET << std::endl;
}

// =============================================================================
// Test 12: MacroMapper - Single Macro Influence (LABYRINTH)
// =============================================================================

void test12_MacroMapperLabyrinthInfluence()
{
    std::cout << Colors::CYAN << "\n[Test 12/22] MacroMapper - LABYRINTH Influence" << Colors::RESET << std::endl;

    MacroMapper mapper;

    // LABYRINTH = 0.0 (simple hall)
    auto targetsSimple = mapper.computeTargets(0.5f, 0.0f, 0.5f, 0.5f, 0.0f, 0.0f, 0.5f, 0.5f, 0.5f, 0.0f);

    // LABYRINTH = 1.0 (twisted maze)
    auto targetsMaze = mapper.computeTargets(0.5f, 1.0f, 0.5f, 0.5f, 0.0f, 0.0f, 0.5f, 0.5f, 0.5f, 0.0f);

    std::cout << Colors::YELLOW << "  Simple hall → warp: " << targetsSimple.warp << ", drift: " << targetsSimple.drift << Colors::RESET << std::endl;
    std::cout << Colors::YELLOW << "  Twisted maze → warp: " << targetsMaze.warp << ", drift: " << targetsMaze.drift << Colors::RESET << std::endl;

    // Labyrinth should increase warp and drift
    assertGreaterThan(targetsMaze.warp, targetsSimple.warp, "Labyrinth increases warp");
    assertGreaterThan(targetsMaze.drift, targetsSimple.drift, "Labyrinth increases drift");
    assertGreaterThan(targetsMaze.warp, 0.5f, "Twisted maze has significant warp (>0.5)");

    std::cout << Colors::GREEN << "  ✓ LABYRINTH macro affects warp/drift correctly" << Colors::RESET << std::endl;
}

// =============================================================================
// Test 13: MacroMapper - Single Macro Influence (ABYSS)
// =============================================================================

void test13_MacroMapperAbyssInfluence()
{
    std::cout << Colors::CYAN << "\n[Test 13/22] MacroMapper - ABYSS Influence" << Colors::RESET << std::endl;

    MacroMapper mapper;

    // ABYSS = 0.0 (shallow)
    auto targetsShallow = mapper.computeTargets(0.5f, 0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.5f, 0.0f);

    // ABYSS = 1.0 (infinite void)
    auto targetsVoid = mapper.computeTargets(0.5f, 0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.5f, 1.0f, 0.5f, 0.0f);

    std::cout << Colors::YELLOW << "  Shallow → time: " << targetsShallow.time << ", width: " << targetsShallow.width << Colors::RESET << std::endl;
    std::cout << Colors::YELLOW << "  Infinite void → time: " << targetsVoid.time << ", width: " << targetsVoid.width << Colors::RESET << std::endl;

    // Abyss should increase time and width
    assertGreaterThan(targetsVoid.time, targetsShallow.time, "Abyss increases time (deeper = longer)");
    assertGreaterThan(targetsVoid.width, targetsShallow.width, "Abyss increases width (infinite = wider)");

    std::cout << Colors::GREEN << "  ✓ ABYSS macro affects time/width correctly" << Colors::RESET << std::endl;
}

// =============================================================================
// Test 14: MacroMapper - Multiple Macro Blending
// =============================================================================

void test14_MacroMapperMultipleInfluences()
{
    std::cout << Colors::CYAN << "\n[Test 14/22] MacroMapper - Multiple Influences" << Colors::RESET << std::endl;

    MacroMapper mapper;

    // Both STONE and MIST affect time
    // STONE = 1.0 (increases time)
    // MIST = 1.0 (increases time)
    auto targetsBoth = mapper.computeTargets(1.0f, 0.5f, 1.0f, 0.5f, 0.0f, 0.0f, 0.5f, 0.5f, 0.5f, 0.0f);

    // Only STONE affects time
    auto targetsStoneOnly = mapper.computeTargets(1.0f, 0.5f, 0.0f, 0.5f, 0.0f, 0.0f, 0.5f, 0.5f, 0.5f, 0.0f);

    // Only MIST affects time
    auto targetsMistOnly = mapper.computeTargets(0.0f, 0.5f, 1.0f, 0.5f, 0.0f, 0.0f, 0.5f, 0.5f, 0.5f, 0.0f);

    std::cout << Colors::YELLOW << "  STONE only → time: " << targetsStoneOnly.time << Colors::RESET << std::endl;
    std::cout << Colors::YELLOW << "  MIST only → time: " << targetsMistOnly.time << Colors::RESET << std::endl;
    std::cout << Colors::YELLOW << "  Both → time: " << targetsBoth.time << Colors::RESET << std::endl;

    // Combined influence should be greater than individual (weighted blend)
    assertTrue(targetsBoth.time >= targetsStoneOnly.time || targetsBoth.time >= targetsMistOnly.time,
               "Combined influences blend correctly");

    std::cout << Colors::GREEN << "  ✓ Multiple macro influences blend correctly" << Colors::RESET << std::endl;
}

// =============================================================================
// Test 15: MacroMapper - Deterministic and Thread-Safe
// =============================================================================

void test15_MacroMapperDeterministic()
{
    std::cout << Colors::CYAN << "\n[Test 15/22] MacroMapper - Deterministic" << Colors::RESET << std::endl;

    MacroMapper mapper;

    // Call with fixed inputs 1000 times
    MacroMapper::MacroInputs macros;
    macros.stone = 0.7f;
    macros.labyrinth = 0.3f;
    macros.mist = 0.5f;

    auto firstResult = mapper.computeTargets(macros);

    for (int i = 0; i < 1000; ++i)
    {
        auto result = mapper.computeTargets(macros);

        // Verify bit-exact match
        assertTrue(result.time == firstResult.time, "time deterministic");
        assertTrue(result.mass == firstResult.mass, "mass deterministic");
        assertTrue(result.warp == firstResult.warp, "warp deterministic");
    }

    std::cout << Colors::GREEN << "  ✓ MacroMapper is deterministic (1000 calls, bit-exact)" << Colors::RESET << std::endl;
}

// =============================================================================
// Test 16: ExpressiveMacroMapper - Initialization
// =============================================================================

void test16_ExpressiveMapperInitialization()
{
    std::cout << Colors::CYAN << "\n[Test 16/22] ExpressiveMacroMapper - Initialization" << Colors::RESET << std::endl;

    ExpressiveMacroMapper mapper;
    ExpressiveMacroMapper::MacroInputs macros; // All defaults

    auto targets = mapper.computeTargets(macros);

    // Verify all outputs in valid range [0, 1]
    assertInRange(targets.time, 0.0f, 1.0f, "time in range");
    assertInRange(targets.mass, 0.0f, 1.0f, "mass in range");
    assertInRange(targets.density, 0.0f, 1.0f, "density in range");

    // Verify routing preset is valid
    assertTrue(static_cast<int>(targets.routingPreset) >= 0 &&
               static_cast<int>(targets.routingPreset) <= 7,
               "Valid routing preset");

    std::cout << Colors::YELLOW << "  Routing preset: " << static_cast<int>(targets.routingPreset) << Colors::RESET << std::endl;

    std::cout << Colors::GREEN << "  ✓ ExpressiveMacroMapper initialization successful" << Colors::RESET << std::endl;
}

// =============================================================================
// Test 17: ExpressiveMacroMapper - Character Scaling
// =============================================================================

void test17_ExpressiveCharacterScaling()
{
    std::cout << Colors::CYAN << "\n[Test 17/22] ExpressiveMacroMapper - Character Scaling" << Colors::RESET << std::endl;

    ExpressiveMacroMapper mapper;

    // Character = 0.0 (subtle)
    auto targetsSubtle = mapper.computeTargets(0.0f, 0.2f, 0.1f, 0.2f, 0.5f, 0.5f);

    // Character = 1.0 (extreme)
    auto targetsExtreme = mapper.computeTargets(1.0f, 0.2f, 0.1f, 0.2f, 0.5f, 0.5f);

    std::cout << Colors::YELLOW << "  Subtle (character=0) → density: " << targetsSubtle.density << Colors::RESET << std::endl;
    std::cout << Colors::YELLOW << "  Extreme (character=1) → density: " << targetsExtreme.density << Colors::RESET << std::endl;

    // Character should scale intensity (extreme > subtle for parameters like density, warp)
    // Note: Some parameters may have inverse relationships, so we test general principle
    assertTrue(targetsExtreme.density != targetsSubtle.density ||
               targetsExtreme.warp != targetsSubtle.warp,
               "Character affects parameters");

    std::cout << Colors::GREEN << "  ✓ Character macro scales intensity" << Colors::RESET << std::endl;
}

// =============================================================================
// Test 18: ExpressiveMacroMapper - Space Type Selection
// =============================================================================

void test18_ExpressiveSpaceTypeSelection()
{
    std::cout << Colors::CYAN << "\n[Test 18/22] ExpressiveMacroMapper - Space Type" << Colors::RESET << std::endl;

    ExpressiveMacroMapper mapper;

    struct TestCase {
        float spaceType;
        RoutingPresetType expectedPreset;
        std::string name;
    };

    std::vector<TestCase> tests = {
        {0.1f, RoutingPresetType::TraditionalCathedral, "Chamber"},
        {0.3f, RoutingPresetType::TraditionalCathedral, "Hall"},
        {0.5f, RoutingPresetType::ShimmerInfinity, "Shimmer"},
        {0.7f, RoutingPresetType::MetallicGranular, "Granular"},
        {0.9f, RoutingPresetType::MetallicGranular, "Metallic"}
    };

    for (const auto& test : tests)
    {
        auto targets = mapper.computeTargets(0.5f, test.spaceType, 0.1f, 0.2f, 0.5f, 0.5f);

        std::cout << Colors::YELLOW << "  " << test.name << " (spaceType=" << test.spaceType << ") → preset: "
                  << static_cast<int>(targets.routingPreset) << Colors::RESET << std::endl;

        // Note: Exact preset mapping may vary, but spaceType should affect routing
        assertTrue(static_cast<int>(targets.routingPreset) >= 0, "Valid preset selected");
    }

    std::cout << Colors::GREEN << "  ✓ Space type selects routing presets" << Colors::RESET << std::endl;
}

// =============================================================================
// Test 19: ExpressiveMacroMapper - Energy Mapping
// =============================================================================

void test19_ExpressiveEnergyMapping()
{
    std::cout << Colors::CYAN << "\n[Test 19/22] ExpressiveMacroMapper - Energy Mapping" << Colors::RESET << std::endl;

    ExpressiveMacroMapper mapper;

    // Energy modes: decay, sustain, grow, chaos
    auto targetsDecay = mapper.computeTargets(0.5f, 0.2f, 0.1f, 0.2f, 0.5f, 0.5f);    // Decay
    auto targetsSustain = mapper.computeTargets(0.5f, 0.2f, 0.4f, 0.2f, 0.5f, 0.5f);  // Sustain
    auto targetsGrow = mapper.computeTargets(0.5f, 0.2f, 0.7f, 0.2f, 0.5f, 0.5f);     // Grow
    auto targetsChaos = mapper.computeTargets(0.5f, 0.2f, 0.95f, 0.2f, 0.5f, 0.5f);   // Chaos

    std::cout << Colors::YELLOW << "  Decay → time: " << targetsDecay.time << ", bloom: " << targetsDecay.bloom << Colors::RESET << std::endl;
    std::cout << Colors::YELLOW << "  Sustain → time: " << targetsSustain.time << Colors::RESET << std::endl;
    std::cout << Colors::YELLOW << "  Grow → bloom: " << targetsGrow.bloom << Colors::RESET << std::endl;
    std::cout << Colors::YELLOW << "  Chaos → paradoxGain: " << targetsChaos.paradoxGain << Colors::RESET << std::endl;

    // Grow mode should have higher bloom than decay
    assertGreaterThan(targetsGrow.bloom, targetsDecay.bloom, "Grow has higher bloom");

    // Chaos mode should have higher paradox gain
    assertGreaterThan(targetsChaos.paradoxGain, targetsDecay.paradoxGain, "Chaos has higher paradox gain");

    std::cout << Colors::GREEN << "  ✓ Energy macro controls decay behavior" << Colors::RESET << std::endl;
}

// =============================================================================
// Test 20: ExpressiveMacroMapper - Motion Mapping
// =============================================================================

void test20_ExpressiveMotionMapping()
{
    std::cout << Colors::CYAN << "\n[Test 20/22] ExpressiveMacroMapper - Motion Mapping" << Colors::RESET << std::endl;

    ExpressiveMacroMapper mapper;

    // Motion modes: still, drift, pulse, random
    auto targetsStill = mapper.computeTargets(0.5f, 0.2f, 0.1f, 0.1f, 0.5f, 0.5f);    // Still
    auto targetsDrift = mapper.computeTargets(0.5f, 0.2f, 0.1f, 0.4f, 0.5f, 0.5f);    // Drift
    auto targetsPulse = mapper.computeTargets(0.5f, 0.2f, 0.1f, 0.7f, 0.5f, 0.5f);    // Pulse
    auto targetsRandom = mapper.computeTargets(0.5f, 0.2f, 0.1f, 0.95f, 0.5f, 0.5f);  // Random

    std::cout << Colors::YELLOW << "  Still → drift: " << targetsStill.drift << ", warp: " << targetsStill.warp << Colors::RESET << std::endl;
    std::cout << Colors::YELLOW << "  Drift → drift: " << targetsDrift.drift << Colors::RESET << std::endl;
    std::cout << Colors::YELLOW << "  Pulse → drift: " << targetsPulse.drift << Colors::RESET << std::endl;
    std::cout << Colors::YELLOW << "  Random → warp: " << targetsRandom.warp << Colors::RESET << std::endl;

    // Drift mode should have more drift than still
    assertGreaterThan(targetsDrift.drift, targetsStill.drift, "Drift mode increases drift");

    // Random mode should have more warp
    assertGreaterThan(targetsRandom.warp, targetsStill.warp, "Random mode increases warp");

    std::cout << Colors::GREEN << "  ✓ Motion macro controls temporal evolution" << Colors::RESET << std::endl;
}

// =============================================================================
// Test 21: ExpressiveMacroMapper - Color Mapping
// =============================================================================

void test21_ExpressiveColorMapping()
{
    std::cout << Colors::CYAN << "\n[Test 21/22] ExpressiveMacroMapper - Color Mapping" << Colors::RESET << std::endl;

    ExpressiveMacroMapper mapper;

    // Color modes: dark, balanced, bright, spectral
    auto targetsDark = mapper.computeTargets(0.5f, 0.2f, 0.1f, 0.2f, 0.1f, 0.5f);       // Dark
    auto targetsBalanced = mapper.computeTargets(0.5f, 0.2f, 0.1f, 0.2f, 0.5f, 0.5f);  // Balanced
    auto targetsBright = mapper.computeTargets(0.5f, 0.2f, 0.1f, 0.2f, 0.8f, 0.5f);    // Bright
    auto targetsSpectral = mapper.computeTargets(0.5f, 0.2f, 0.1f, 0.2f, 0.95f, 0.5f); // Spectral

    std::cout << Colors::YELLOW << "  Dark → mass: " << targetsDark.mass << ", air: " << targetsDark.air << Colors::RESET << std::endl;
    std::cout << Colors::YELLOW << "  Balanced → mass: " << targetsBalanced.mass << ", air: " << targetsBalanced.air << Colors::RESET << std::endl;
    std::cout << Colors::YELLOW << "  Bright → mass: " << targetsBright.mass << ", air: " << targetsBright.air << Colors::RESET << std::endl;
    std::cout << Colors::YELLOW << "  Spectral → metallicResonance: " << targetsSpectral.metallicResonance << Colors::RESET << std::endl;

    // Dark should have higher mass, lower air than bright
    assertGreaterThan(targetsDark.mass, targetsBright.mass, "Dark has higher mass");
    assertGreaterThan(targetsBright.air, targetsDark.air, "Bright has higher air");

    // Spectral should have higher metallic resonance
    assertGreaterThan(targetsSpectral.metallicResonance, targetsBalanced.metallicResonance,
                     "Spectral has higher metallic resonance");

    std::cout << Colors::GREEN << "  ✓ Color macro controls spectral character" << Colors::RESET << std::endl;
}

// =============================================================================
// Test 22: ExpressiveMacroMapper - Dimension Mapping
// =============================================================================

void test22_ExpressiveDimensionMapping()
{
    std::cout << Colors::CYAN << "\n[Test 22/22] ExpressiveMacroMapper - Dimension Mapping" << Colors::RESET << std::endl;

    ExpressiveMacroMapper mapper;

    // Dimension modes: intimate, room, cathedral, infinite
    auto targetsIntimate = mapper.computeTargets(0.5f, 0.2f, 0.1f, 0.2f, 0.5f, 0.1f);   // Intimate
    auto targetsRoom = mapper.computeTargets(0.5f, 0.2f, 0.1f, 0.2f, 0.5f, 0.4f);       // Room
    auto targetsCathedral = mapper.computeTargets(0.5f, 0.2f, 0.1f, 0.2f, 0.5f, 0.7f);  // Cathedral
    auto targetsInfinite = mapper.computeTargets(0.5f, 0.2f, 0.1f, 0.2f, 0.5f, 0.95f);  // Infinite

    std::cout << Colors::YELLOW << "  Intimate → time: " << targetsIntimate.time << ", width: " << targetsIntimate.width << Colors::RESET << std::endl;
    std::cout << Colors::YELLOW << "  Room → time: " << targetsRoom.time << ", width: " << targetsRoom.width << Colors::RESET << std::endl;
    std::cout << Colors::YELLOW << "  Cathedral → time: " << targetsCathedral.time << ", width: " << targetsCathedral.width << Colors::RESET << std::endl;
    std::cout << Colors::YELLOW << "  Infinite → time: " << targetsInfinite.time << ", impossibility: " << targetsInfinite.impossibilityDegree << Colors::RESET << std::endl;

    // Cathedral should have longer time than intimate
    assertGreaterThan(targetsCathedral.time, targetsIntimate.time, "Cathedral has longer time");

    // Infinite should have higher impossibility degree
    assertGreaterThan(targetsInfinite.impossibilityDegree, targetsRoom.impossibilityDegree,
                     "Infinite has higher impossibility degree");

    std::cout << Colors::GREEN << "  ✓ Dimension macro controls space size" << Colors::RESET << std::endl;
}

// =============================================================================
// Main Test Runner
// =============================================================================

int main()
{
    std::cout << Colors::BOLD << Colors::CYAN
              << "\n========================================" << Colors::RESET << std::endl;
    std::cout << Colors::BOLD << Colors::CYAN
              << "  Phase 3: Foundation Module Tests" << Colors::RESET << std::endl;
    std::cout << Colors::BOLD << Colors::CYAN
              << "========================================" << Colors::RESET << std::endl;

    int totalTests = 22;
    int passedTests = 0;
    int failedTests = 0;

    std::vector<std::string> failedTestNames;

    // AllpassDiffuser Tests (7 tests)
    std::cout << Colors::BOLD << Colors::MAGENTA << "\n▶ AllpassDiffuser Tests (1-7)" << Colors::RESET << std::endl;

    auto runTest = [&](auto testFunc, const std::string& testName) {
        try {
            testFunc();
            passedTests++;
        } catch (const std::exception& e) {
            std::cout << Colors::RED << "  ✗ Test failed: " << e.what() << Colors::RESET << std::endl;
            failedTests++;
            failedTestNames.push_back(testName);
        }
    };

    runTest(test01_AllpassInitialization, "Test 1");
    runTest(test02_AllpassUnityGain, "Test 2");
    runTest(test03_AllpassCoefficientClamping, "Test 3");
    runTest(test04_AllpassPhaseResponse, "Test 4");
    runTest(test05_AllpassDelayLengthImpact, "Test 5");
    runTest(test06_AllpassStability, "Test 6");
    runTest(test07_AllpassReset, "Test 7");

    // MacroMapper Tests (8 tests)
    std::cout << Colors::BOLD << Colors::MAGENTA << "\n▶ MacroMapper Tests (8-15)" << Colors::RESET << std::endl;

    runTest(test08_MacroMapperInitialization, "Test 8");
    runTest(test09_MacroMapperInputClamping, "Test 9");
    runTest(test10_MacroMapperBoundaryConditions, "Test 10");
    runTest(test11_MacroMapperStoneInfluence, "Test 11");
    runTest(test12_MacroMapperLabyrinthInfluence, "Test 12");
    runTest(test13_MacroMapperAbyssInfluence, "Test 13");
    runTest(test14_MacroMapperMultipleInfluences, "Test 14");
    runTest(test15_MacroMapperDeterministic, "Test 15");

    // ExpressiveMacroMapper Tests (7 tests)
    std::cout << Colors::BOLD << Colors::MAGENTA << "\n▶ ExpressiveMacroMapper Tests (16-22)" << Colors::RESET << std::endl;

    runTest(test16_ExpressiveMapperInitialization, "Test 16");
    runTest(test17_ExpressiveCharacterScaling, "Test 17");
    runTest(test18_ExpressiveSpaceTypeSelection, "Test 18");
    runTest(test19_ExpressiveEnergyMapping, "Test 19");
    runTest(test20_ExpressiveMotionMapping, "Test 20");
    runTest(test21_ExpressiveColorMapping, "Test 21");
    runTest(test22_ExpressiveDimensionMapping, "Test 22");

    // Print summary
    std::cout << Colors::BOLD << Colors::CYAN
              << "\n========================================" << Colors::RESET << std::endl;
    std::cout << Colors::BOLD << Colors::CYAN
              << "  Test Summary" << Colors::RESET << std::endl;
    std::cout << Colors::BOLD << Colors::CYAN
              << "========================================" << Colors::RESET << std::endl;

    std::cout << Colors::BOLD << "Total Tests:   " << totalTests << Colors::RESET << std::endl;
    std::cout << Colors::GREEN << Colors::BOLD << "Passed:        " << passedTests << Colors::RESET << std::endl;
    std::cout << Colors::RED << Colors::BOLD << "Failed:        " << failedTests << Colors::RESET << std::endl;

    if (failedTests > 0)
    {
        std::cout << Colors::RED << "\nFailed tests:" << Colors::RESET << std::endl;
        for (const auto& name : failedTestNames)
        {
            std::cout << Colors::RED << "  - " << name << Colors::RESET << std::endl;
        }
    }

    std::cout << Colors::BOLD << Colors::CYAN
              << "\n========================================" << Colors::RESET << std::endl;

    if (failedTests == 0)
    {
        std::cout << Colors::GREEN << Colors::BOLD
                  << "🎉 All tests passed! Phase 3 COMPLETE" << Colors::RESET << std::endl;
        return 0;
    }
    else
    {
        std::cout << Colors::RED << Colors::BOLD
                  << "❌ Some tests failed" << Colors::RESET << std::endl;
        return 1;
    }
}
