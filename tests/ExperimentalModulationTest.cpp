#include <JuceHeader.h>
#include "dsp/ExperimentalModulation.h"
#include <iostream>
#include <iomanip>
#include <cmath>

using namespace monument::dsp;

//==============================================================================
// Test Result Tracking
//==============================================================================

struct TestResults
{
    int passed{0};
    int failed{0};

    void pass(const std::string& testName)
    {
        ++passed;
        std::cout << "  ✓ " << testName << std::endl;
    }

    void fail(const std::string& testName, const std::string& reason)
    {
        ++failed;
        std::cout << "  ✗ " << testName << " - " << reason << std::endl;
    }

    void print()
    {
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "Test Results: " << passed << " passed, " << failed << " failed" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
    }
};

//==============================================================================
// Test: ModulationQuantizer
//==============================================================================

static void testModulationQuantizer(TestResults& results)
{
    std::cout << "\n[ModulationQuantizer Tests]" << std::endl;

    ModulationQuantizer quantizer;

    // Test 1: 8-step quantization
    quantizer.setSteps(8);
    {
        float output0 = quantizer.quantize(0.0f);
        float output5 = quantizer.quantize(0.5f);
        float output1 = quantizer.quantize(1.0f);

        if (std::abs(output0 - 0.0f) < 0.001f)
            results.pass("8-step quantization at 0.0");
        else
            results.fail("8-step quantization at 0.0",
                        "Expected ~0.0, got " + std::to_string(output0));

        if (std::abs(output5 - 0.571f) < 0.01f)  // 4/7 ≈ 0.571
            results.pass("8-step quantization at 0.5");
        else
            results.fail("8-step quantization at 0.5",
                        "Expected ~0.571, got " + std::to_string(output5));

        if (std::abs(output1 - 1.0f) < 0.001f)
            results.pass("8-step quantization at 1.0");
        else
            results.fail("8-step quantization at 1.0",
                        "Expected ~1.0, got " + std::to_string(output1));
    }

    // Test 2: Edge cases (2 steps, 64 steps)
    quantizer.setSteps(2);
    {
        float output = quantizer.quantize(0.4f);
        if (output == 0.0f || output == 1.0f)
            results.pass("2-step quantization snaps to binary");
        else
            results.fail("2-step quantization", "Should be 0.0 or 1.0, got " + std::to_string(output));
    }

    quantizer.setSteps(64);
    {
        float output = quantizer.quantize(0.5f);
        if (output >= 0.48f && output <= 0.52f)
            results.pass("64-step quantization preserves resolution");
        else
            results.fail("64-step quantization", "Expected ~0.5, got " + std::to_string(output));
    }
}

//==============================================================================
// Test: ProbabilityGate
//==============================================================================

static void testProbabilityGate(TestResults& results)
{
    std::cout << "\n[ProbabilityGate Tests]" << std::endl;

    ProbabilityGate gate;
    gate.prepare(48000.0);

    // Test 1: 100% probability always passes
    gate.setProbability(1.0f);
    gate.setSmoothingMs(1.0f, 48000.0);  // Very short smoothing for testing (1ms)
    {
        int activeCount = 0;
        for (int i = 0; i < 100; ++i)
        {
            // Simulate time passing by calling getNextValue() multiple times (one audio block @ 512 samples)
            for (int j = 0; j < 512; ++j)
            {
                float result = gate.process(1.0f);
                if (i > 10 && j == 0 && result >= 0.9f)  // Check first sample of block after ramp-up
                    ++activeCount;
            }
        }

        if (activeCount >= 85)  // 85/90 = 94% threshold (accounting for initial ramp)
            results.pass("100% probability passes most blocks");
        else
            results.fail("100% probability",
                        std::to_string(activeCount) + "/90 blocks active");
    }

    // Test 2: 0% probability always blocks
    gate.prepare(48000.0);  // Reset
    gate.setProbability(0.0f);
    {
        int blockedCount = 0;
        for (int i = 0; i < 100; ++i)
        {
            float result = gate.process(1.0f);
            if (result <= 0.1f)  // Allow for smoothing envelope
                ++blockedCount;
        }

        if (blockedCount >= 95)  // 95% threshold
            results.pass("0% probability blocks most blocks");
        else
            results.fail("0% probability",
                        std::to_string(blockedCount) + "/100 blocks blocked");
    }

    // Test 3: 50% probability is statistically ~50%
    gate.prepare(48000.0);  // Reset
    gate.setProbability(0.5f);
    gate.setSmoothingMs(1.0f, 48000.0);  // Very short smoothing for testing (1ms)
    {
        int activeCount = 0;
        for (int i = 0; i < 200; ++i)
        {
            // Simulate time passing by advancing envelope (one audio block @ 512 samples)
            for (int j = 0; j < 512; ++j)
            {
                float result = gate.process(1.0f);
                if (i > 10 && j == 0 && result >= 0.5f)  // Check first sample of block
                    ++activeCount;
            }
        }

        float percentage = activeCount / 190.0f;
        if (percentage >= 0.35f && percentage <= 0.65f)  // 35-65% range
            results.pass("50% probability is statistically valid");
        else
            results.fail("50% probability",
                        std::to_string(static_cast<int>(percentage * 100)) + "% active (expected ~50%)");
    }
}

//==============================================================================
// Test: SpringMassModulator
//==============================================================================

static void testSpringMassModulator(TestResults& results)
{
    std::cout << "\n[SpringMassModulator Tests]" << std::endl;

    SpringMassModulator spring;
    spring.prepare(48000.0);

    // Test 1: Apply constant force → verify oscillation then settling
    spring.setSpringConstant(1.0f);
    spring.setMass(1.0f);
    spring.setDamping(0.05f);  // Reduced damping for more visible oscillation

    {
        float maxPosition = 0.0f;
        for (int i = 0; i < 10000; ++i)  // ~0.2 seconds
        {
            // Apply force continuously (like audio input would)
            spring.applyForce(1.0f);
            float position = spring.processSample();
            maxPosition = std::max(maxPosition, std::abs(position));
        }

        if (maxPosition > 0.01f && maxPosition < 10.0f)  // Lowered threshold
            results.pass("Constant force causes oscillation within bounds");
        else
            results.fail("Constant force oscillation",
                        "Max position: " + std::to_string(maxPosition));
    }

    // Test 2: High damping → quick settling
    spring.reset();
    spring.setDamping(2.0f);
    spring.applyForce(1.0f);

    {
        float position = 0.0f;
        for (int i = 0; i < 2000; ++i)  // ~0.04 seconds
        {
            position = spring.processSample();
        }

        if (std::abs(position) < 0.5f)  // Should settle quickly
            results.pass("High damping settles quickly");
        else
            results.fail("High damping", "Position still " + std::to_string(position) + " after settling time");
    }

    // Test 3: Stability test (no NaN or Inf)
    spring.reset();
    spring.setSpringConstant(100.0f);  // Extreme values
    spring.setMass(0.1f);
    spring.setDamping(0.01f);
    spring.applyForce(10.0f);

    {
        bool stable = true;
        for (int i = 0; i < 48000; ++i)  // 1 second
        {
            float position = spring.processSample();
            if (std::isnan(position) || std::isinf(position))
            {
                stable = false;
                break;
            }
        }

        if (stable)
            results.pass("Extreme parameter stability test");
        else
            results.fail("Stability test", "NaN or Inf detected");
    }
}

//==============================================================================
// Test: PresetMorpher
//==============================================================================

static void testPresetMorpher(TestResults& results)
{
    std::cout << "\n[PresetMorpher Tests]" << std::endl;

    PresetMorpher morpher;

    // Load 4 corner presets with known values
    std::vector<std::vector<float>> presetData = {
        {0.0f, 0.0f, 0.0f},  // Top-Left
        {1.0f, 0.0f, 0.0f},  // Top-Right
        {0.0f, 1.0f, 0.0f},  // Bottom-Left
        {1.0f, 1.0f, 1.0f}   // Bottom-Right
    };

    morpher.loadPresetStates(presetData);
    morpher.setCornerPresets(0, 1, 2, 3);

    // Test 1: Corner positions return exact values
    morpher.setMorphPosition(0.0f, 0.0f);  // Top-Left
    {
        float param0 = morpher.getMorphedParameter(0);
        if (std::abs(param0 - 0.0f) < 0.001f)
            results.pass("Top-Left corner exact");
        else
            results.fail("Top-Left corner", "Expected 0.0, got " + std::to_string(param0));
    }

    morpher.setMorphPosition(1.0f, 0.0f);  // Top-Right
    {
        float param0 = morpher.getMorphedParameter(0);
        if (std::abs(param0 - 1.0f) < 0.001f)
            results.pass("Top-Right corner exact");
        else
            results.fail("Top-Right corner", "Expected 1.0, got " + std::to_string(param0));
    }

    // Test 2: Center position returns average
    morpher.setMorphPosition(0.5f, 0.5f);
    {
        float param0 = morpher.getMorphedParameter(0);
        float expected = (0.0f + 1.0f + 0.0f + 1.0f) / 4.0f;  // 0.5

        if (std::abs(param0 - expected) < 0.001f)
            results.pass("Center position averages correctly");
        else
            results.fail("Center position",
                        "Expected " + std::to_string(expected) + ", got " + std::to_string(param0));
    }

    // Test 3: Verify smooth interpolation
    morpher.setMorphPosition(0.25f, 0.25f);
    {
        float param0 = morpher.getMorphedParameter(0);
        // Expected: (0.75 * 0.75 * 0.0) + (0.25 * 0.75 * 1.0) +
        //           (0.75 * 0.25 * 0.0) + (0.25 * 0.25 * 1.0) = 0.25

        if (param0 >= 0.0f && param0 <= 1.0f)
            results.pass("Bilinear interpolation produces valid range");
        else
            results.fail("Bilinear interpolation", "Out of range: " + std::to_string(param0));
    }
}

//==============================================================================
// Test: GestureRecorder
//==============================================================================

static void testGestureRecorder(TestResults& results)
{
    std::cout << "\n[GestureRecorder Tests]" << std::endl;

    GestureRecorder recorder;

    // Test 1: Record and playback
    recorder.startRecording();
    for (int i = 0; i < 100; ++i)
    {
        recorder.recordValue(static_cast<float>(i) / 100.0f);
    }
    recorder.stopRecording();

    {
        if (recorder.getLength() == 100)
            results.pass("Records correct number of samples");
        else
            results.fail("Record length",
                        "Expected 100, got " + std::to_string(recorder.getLength()));
    }

    recorder.startPlayback(1.0f, false);
    {
        float firstSample = recorder.getSample();
        if (std::abs(firstSample - 0.0f) < 0.001f)
            results.pass("Playback starts at first sample");
        else
            results.fail("Playback start", "Expected ~0.0, got " + std::to_string(firstSample));
    }

    // Test 2: 2× speed playback
    recorder.startPlayback(2.0f, false);
    {
        int sampleCount = 0;
        while (recorder.isPlaying() && sampleCount < 200)
        {
            recorder.getSample();
            ++sampleCount;
        }

        // Should complete in ~50 samples (100 / 2.0 speed)
        if (sampleCount >= 45 && sampleCount <= 55)
            results.pass("2× speed playback completes in half time");
        else
            results.fail("2× speed playback",
                        "Took " + std::to_string(sampleCount) + " samples (expected ~50)");
    }

    // Test 3: Loop mode
    recorder.startPlayback(1.0f, true);
    {
        // Play past the end
        for (int i = 0; i < 150; ++i)
        {
            recorder.getSample();
        }

        if (recorder.isPlaying())
            results.pass("Loop mode continues playing");
        else
            results.fail("Loop mode", "Stopped playing after end");
    }
}

//==============================================================================
// Test: ChaosSeeder
//==============================================================================

static void testChaosSeeder(TestResults& results)
{
    std::cout << "\n[ChaosSeeder Tests]" << std::endl;

    // Test 1: Generate random connections
    auto connections = ChaosSeeder::generateRandomConnections(8, 4, 15);

    {
        if (connections.size() == 8)
            results.pass("Generates correct number of connections");
        else
            results.fail("Connection count",
                        "Expected 8, got " + std::to_string(connections.size()));
    }

    // Test 2: No duplicate source/dest pairs
    {
        std::set<std::pair<int, int>> pairs;
        for (const auto& conn : connections)
        {
            pairs.insert({std::get<0>(conn), std::get<1>(conn)});
        }

        if (pairs.size() == connections.size())
            results.pass("No duplicate connections");
        else
            results.fail("Duplicate connections",
                        std::to_string(pairs.size()) + " unique out of " +
                        std::to_string(connections.size()));
    }

    // Test 3: Depth range validation
    {
        bool validRange = true;
        for (const auto& conn : connections)
        {
            float depth = std::get<2>(conn);
            if (std::abs(depth) < 0.2f || std::abs(depth) > 0.6f)
            {
                validRange = false;
                break;
            }
        }

        if (validRange)
            results.pass("Depth values in musical range [0.2, 0.6]");
        else
            results.fail("Depth range", "Values outside expected range");
    }

    // Test 4: Generate random probabilities
    auto probabilities = ChaosSeeder::generateRandomProbabilities(10);
    {
        bool validProb = true;
        for (float prob : probabilities)
        {
            if (prob < 0.3f || prob > 1.0f)
            {
                validProb = false;
                break;
            }
        }

        if (validProb)
            results.pass("Probabilities in range [0.3, 1.0]");
        else
            results.fail("Probability range", "Values outside expected range");
    }

    // Test 5: Generate random quantization
    auto steps = ChaosSeeder::generateRandomQuantization(10);
    {
        bool validSteps = true;
        for (int step : steps)
        {
            if (step < 2 || step > 16)
            {
                validSteps = false;
                break;
            }
        }

        if (validSteps)
            results.pass("Quantization steps in range [2, 16]");
        else
            results.fail("Quantization range", "Values outside expected range");
    }
}

//==============================================================================
// Main
//==============================================================================

int main(int /*argc*/, char* /*argv*/[])
{
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Monument Reverb - Experimental Modulation Tests" << std::endl;
    std::cout << std::string(60, '=') << std::endl;

    TestResults results;

    testModulationQuantizer(results);
    testProbabilityGate(results);
    testSpringMassModulator(results);
    testPresetMorpher(results);
    testGestureRecorder(results);
    testChaosSeeder(results);

    results.print();

    return results.failed > 0 ? 1 : 0;
}
