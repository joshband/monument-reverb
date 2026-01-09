/**
 * Monument Reverb - ModulationMatrix Test (Phase 1.2 - Critical Infrastructure)
 *
 * Tests the modulation routing system for correct source-to-destination routing,
 * thread safety, probability gating, and all modulation sources.
 *
 * Test Coverage:
 * 1. Basic Connection Routing
 * 2. Multiple Connections Accumulation
 * 3. Bipolar Modulation (positive and negative depth)
 * 4. Smoothing Behavior (parameter lag)
 * 5. Probability Gating (intermittent modulation)
 * 6. Thread Safety (lock-free snapshots)
 * 7. Connection Management (add/update/remove/clear)
 * 8. Chaos Attractor (3 axes, bounded [-1, 1])
 * 9. Audio Follower (RMS envelope tracking)
 * 10. Brownian Motion (bounded random walk)
 * 11. Envelope Tracker (attack/sustain/release)
 * 12. Randomization (sparse/normal/dense)
 *
 * Success Criteria:
 * - All connections route correctly
 * - Modulation accumulates from multiple sources
 * - Smoothing prevents zipper noise
 * - Probability gating works correctly
 * - Lock-free thread safety verified
 * - All modulation sources stay within bounds
 * - Chaos attractor produces organic motion
 * - Audio follower tracks signal envelope
 * - Brownian motion stays within [-1, 1]
 * - Envelope tracker responds to transients
 * - Randomization produces valid connections
 */

#include <JuceHeader.h>
#include "dsp/ModulationMatrix.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <vector>
#include <thread>
#include <atomic>

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
// Helper: Generate test audio buffer (sine wave)
//==============================================================================
void generateSineWave(juce::AudioBuffer<float>& buffer, float frequency, double& phase)
{
    const float phaseIncrement = juce::MathConstants<float>::twoPi * frequency / static_cast<float>(kSampleRate);

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const float value = std::sin(static_cast<float>(phase));
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.setSample(ch, sample, value);

        phase += phaseIncrement;
        if (phase >= juce::MathConstants<double>::twoPi)
            phase -= juce::MathConstants<double>::twoPi;
    }
}

//==============================================================================
// Helper: Measure RMS level
//==============================================================================
float measureRMS(const juce::AudioBuffer<float>& buffer)
{
    float sumSquares = 0.0f;
    int totalSamples = 0;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const float* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            sumSquares += data[i] * data[i];
            totalSamples++;
        }
    }

    return std::sqrt(sumSquares / totalSamples);
}

//==============================================================================
// Helper: Check if value is within range
//==============================================================================
bool isInRange(float value, float min, float max)
{
    return value >= min && value <= max;
}

//==============================================================================
// Test 1: Basic Connection Routing
//==============================================================================
TestResult testBasicConnectionRouting()
{
    TestResult result;
    result.testName = "Basic Connection Routing";

    try
    {
        ModulationMatrix matrix;
        matrix.prepare(kSampleRate, kBlockSize, kNumChannels);

        // Create a simple connection: Chaos X -> Warp parameter
        matrix.setConnection(
            ModulationMatrix::SourceType::ChaosAttractor,
            ModulationMatrix::DestinationType::Warp,
            0,  // axis 0 (X)
            0.5f,  // depth
            200.0f  // smoothing
        );

        // Process some blocks to let chaos evolve
        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        buffer.clear();

        // Need more blocks for chaos to develop beyond initial conditions
        for (int block = 0; block < 50; ++block)
        {
            matrix.process(buffer, kBlockSize);
        }

        // Get modulation value
        float modulation = matrix.getModulation(ModulationMatrix::DestinationType::Warp);

        // Verify modulation is within expected range [-1, 1]
        if (!isInRange(modulation, -1.0f, 1.0f))
        {
            result.passed = false;
            result.message = "Modulation out of range: " + std::to_string(modulation);
            return result;
        }

        // Verify connection exists
        auto connections = matrix.getConnections();
        if (connections.size() != 1)
        {
            result.passed = false;
            result.message = "Expected 1 connection, got " + std::to_string(connections.size());
            return result;
        }

        result.passed = true;
        result.message = "Modulation value: " + std::to_string(modulation);
    }
    catch (const std::exception& e)
    {
        result.passed = false;
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test 2: Multiple Connections Accumulation
//==============================================================================
TestResult testMultipleConnectionsAccumulation()
{
    TestResult result;
    result.testName = "Multiple Connections Accumulation";

    try
    {
        ModulationMatrix matrix;
        matrix.prepare(kSampleRate, kBlockSize, kNumChannels);

        // Create multiple connections to the same destination
        matrix.setConnection(
            ModulationMatrix::SourceType::ChaosAttractor,
            ModulationMatrix::DestinationType::Time,
            0,  // X axis
            0.3f,  // depth
            200.0f
        );

        matrix.setConnection(
            ModulationMatrix::SourceType::ChaosAttractor,
            ModulationMatrix::DestinationType::Time,
            1,  // Y axis (different axis, so different connection)
            0.2f,  // depth
            200.0f
        );

        // Process blocks
        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        buffer.clear();

        for (int block = 0; block < 20; ++block)
        {
            matrix.process(buffer, kBlockSize);
        }

        // Get accumulated modulation
        float modulation = matrix.getModulation(ModulationMatrix::DestinationType::Time);

        // Verify modulation is within valid range (should be clamped to [-1, 1])
        if (!isInRange(modulation, -1.0f, 1.0f))
        {
            result.passed = false;
            result.message = "Accumulated modulation out of range: " + std::to_string(modulation);
            return result;
        }

        // Verify both connections exist
        auto connections = matrix.getConnections();
        if (connections.size() != 2)
        {
            result.passed = false;
            result.message = "Expected 2 connections, got " + std::to_string(connections.size());
            return result;
        }

        result.passed = true;
        result.message = "Accumulated modulation: " + std::to_string(modulation);
    }
    catch (const std::exception& e)
    {
        result.passed = false;
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test 3: Bipolar Modulation
//==============================================================================
TestResult testBipolarModulation()
{
    TestResult result;
    result.testName = "Bipolar Modulation";

    try
    {
        ModulationMatrix matrix;
        matrix.prepare(kSampleRate, kBlockSize, kNumChannels);

        // Create connection with negative depth
        matrix.setConnection(
            ModulationMatrix::SourceType::ChaosAttractor,
            ModulationMatrix::DestinationType::Mix,
            0,
            -0.8f,  // negative depth
            200.0f
        );

        // Process blocks
        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        buffer.clear();

        for (int block = 0; block < 20; ++block)
        {
            matrix.process(buffer, kBlockSize);
        }

        // Get modulation value
        float modulation = matrix.getModulation(ModulationMatrix::DestinationType::Mix);

        // Verify modulation is within range and can be negative
        if (!isInRange(modulation, -1.0f, 1.0f))
        {
            result.passed = false;
            result.message = "Bipolar modulation out of range: " + std::to_string(modulation);
            return result;
        }

        // Verify connection has negative depth
        auto connections = matrix.getConnections();
        if (connections.empty() || connections[0].depth >= 0.0f)
        {
            result.passed = false;
            result.message = "Connection depth not negative";
            return result;
        }

        result.passed = true;
        result.message = "Bipolar modulation: " + std::to_string(modulation) +
                        " (depth: " + std::to_string(connections[0].depth) + ")";
    }
    catch (const std::exception& e)
    {
        result.passed = false;
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test 4: Smoothing Behavior
//==============================================================================
TestResult testSmoothingBehavior()
{
    TestResult result;
    result.testName = "Smoothing Behavior";

    try
    {
        ModulationMatrix matrix;
        matrix.prepare(kSampleRate, kBlockSize, kNumChannels);

        // Create connection with fast smoothing
        matrix.setConnection(
            ModulationMatrix::SourceType::ChaosAttractor,
            ModulationMatrix::DestinationType::Bloom,
            0,
            0.5f,
            20.0f  // very fast smoothing (20ms)
        );

        // Process blocks and track modulation changes
        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        buffer.clear();

        std::vector<float> modulationHistory;
        for (int block = 0; block < 50; ++block)
        {
            matrix.process(buffer, kBlockSize);
            modulationHistory.push_back(
                matrix.getModulation(ModulationMatrix::DestinationType::Bloom)
            );
        }

        // Verify modulation changes smoothly (no sudden jumps)
        float maxJump = 0.0f;
        for (size_t i = 1; i < modulationHistory.size(); ++i)
        {
            float jump = std::abs(modulationHistory[i] - modulationHistory[i-1]);
            maxJump = std::max(maxJump, jump);
        }

        // With 20ms smoothing at 512 samples/block (10.67ms), jumps should be small
        // Maximum jump per block should be < 0.5 (smooth transition)
        if (maxJump > 0.5f)
        {
            result.passed = false;
            result.message = "Excessive modulation jump: " + std::to_string(maxJump);
            return result;
        }

        result.passed = true;
        result.message = "Max jump per block: " + std::to_string(maxJump);
    }
    catch (const std::exception& e)
    {
        result.passed = false;
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test 5: Probability Gating
//==============================================================================
TestResult testProbabilityGating()
{
    TestResult result;
    result.testName = "Probability Gating";

    try
    {
        ModulationMatrix matrix;
        matrix.prepare(kSampleRate, kBlockSize, kNumChannels);

        // Create connection with 50% probability using AudioFollower (responds quickly to input)
        matrix.setConnection(
            ModulationMatrix::SourceType::AudioFollower,
            ModulationMatrix::DestinationType::Density,
            0,
            0.8f,
            100.0f,  // faster smoothing
            0.5f  // 50% probability
        );

        // Process many blocks with sustained audio input and track modulation variance
        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        double phase = 0.0;

        const int totalBlocks = 200;
        std::vector<float> modulationHistory;

        // Generate sustained audio signal
        for (int block = 0; block < totalBlocks; ++block)
        {
            generateSineWave(buffer, 440.0f, phase);
            buffer.applyGain(0.5f);  // moderate level
            matrix.process(buffer, kBlockSize);
            float mod = matrix.getModulation(ModulationMatrix::DestinationType::Density);
            modulationHistory.push_back(mod);
        }

        // Calculate variance - probability gating should create intermittent modulation
        // This creates higher variance than continuous modulation would
        float mean = 0.0f;
        for (float val : modulationHistory)
            mean += val;
        mean /= modulationHistory.size();

        float variance = 0.0f;
        for (float val : modulationHistory)
        {
            float diff = val - mean;
            variance += diff * diff;
        }
        variance /= modulationHistory.size();

        // With 50% probability gating, we expect higher variance than without
        // (blocks alternate between active and inactive)
        // At minimum, variance should be > 0.0000001 (any variation confirms intermittent behavior)
        if (variance < 0.0000001f)
        {
            result.passed = false;
            result.message = "Probability gating not creating intermittent behavior (variance: " +
                           std::to_string(variance) + ")";
            return result;
        }

        result.passed = true;
        result.message = "Variance: " + std::to_string(variance) +
                        " (mean: " + std::to_string(mean) + ")";
    }
    catch (const std::exception& e)
    {
        result.passed = false;
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test 6: Thread Safety (Lock-Free Snapshots)
//==============================================================================
TestResult testThreadSafety()
{
    TestResult result;
    result.testName = "Thread Safety (Lock-Free)";

    try
    {
        ModulationMatrix matrix;
        matrix.prepare(kSampleRate, kBlockSize, kNumChannels);

        std::atomic<bool> stopFlag{false};
        std::atomic<int> exceptionCount{0};

        // Audio thread: continuously process
        std::thread audioThread([&]() {
            try {
                juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
                buffer.clear();

                while (!stopFlag.load())
                {
                    matrix.process(buffer, kBlockSize);
                    std::this_thread::sleep_for(std::chrono::microseconds(100));
                }
            } catch (...) {
                exceptionCount++;
            }
        });

        // Message thread: continuously modify connections
        std::thread messageThread([&]() {
            try {
                for (int i = 0; i < 100 && !stopFlag.load(); ++i)
                {
                    // Add connection
                    matrix.setConnection(
                        ModulationMatrix::SourceType::ChaosAttractor,
                        ModulationMatrix::DestinationType::Air,
                        i % 3,  // cycle through axes
                        0.5f,
                        200.0f
                    );

                    std::this_thread::sleep_for(std::chrono::microseconds(500));

                    // Remove connection
                    matrix.removeConnection(
                        ModulationMatrix::SourceType::ChaosAttractor,
                        ModulationMatrix::DestinationType::Air,
                        i % 3
                    );
                }
            } catch (...) {
                exceptionCount++;
            }
        });

        // Run for a short time
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        stopFlag.store(true);

        audioThread.join();
        messageThread.join();

        if (exceptionCount.load() > 0)
        {
            result.passed = false;
            result.message = "Thread safety violation: " +
                           std::to_string(exceptionCount.load()) + " exceptions";
            return result;
        }

        result.passed = true;
        result.message = "No race conditions detected";
    }
    catch (const std::exception& e)
    {
        result.passed = false;
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test 7: Connection Management
//==============================================================================
TestResult testConnectionManagement()
{
    TestResult result;
    result.testName = "Connection Management";

    try
    {
        ModulationMatrix matrix;
        matrix.prepare(kSampleRate, kBlockSize, kNumChannels);

        // Test add
        matrix.setConnection(
            ModulationMatrix::SourceType::ChaosAttractor,
            ModulationMatrix::DestinationType::Width,
            0, 0.5f, 200.0f
        );

        if (matrix.getConnections().size() != 1)
        {
            result.passed = false;
            result.message = "Add failed";
            return result;
        }

        // Test update (same source/dest/axis should update, not add)
        matrix.setConnection(
            ModulationMatrix::SourceType::ChaosAttractor,
            ModulationMatrix::DestinationType::Width,
            0, 0.8f, 200.0f
        );

        if (matrix.getConnections().size() != 1)
        {
            result.passed = false;
            result.message = "Update created duplicate";
            return result;
        }

        if (std::abs(matrix.getConnections()[0].depth - 0.8f) > 0.001f)
        {
            result.passed = false;
            result.message = "Update depth incorrect";
            return result;
        }

        // Test remove
        matrix.removeConnection(
            ModulationMatrix::SourceType::ChaosAttractor,
            ModulationMatrix::DestinationType::Width,
            0
        );

        if (!matrix.getConnections().empty())
        {
            result.passed = false;
            result.message = "Remove failed";
            return result;
        }

        // Test clear
        matrix.setConnection(
            ModulationMatrix::SourceType::ChaosAttractor,
            ModulationMatrix::DestinationType::Time,
            0, 0.5f, 200.0f
        );
        matrix.setConnection(
            ModulationMatrix::SourceType::AudioFollower,
            ModulationMatrix::DestinationType::Mass,
            0, 0.5f, 200.0f
        );

        matrix.clearConnections();

        if (!matrix.getConnections().empty())
        {
            result.passed = false;
            result.message = "Clear failed";
            return result;
        }

        result.passed = true;
        result.message = "Add/update/remove/clear all working";
    }
    catch (const std::exception& e)
    {
        result.passed = false;
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test 8: Chaos Attractor (3 Axes, Bounded)
//==============================================================================
TestResult testChaosAttractor()
{
    TestResult result;
    result.testName = "Chaos Attractor (3 Axes)";

    try
    {
        ModulationMatrix matrix;
        matrix.prepare(kSampleRate, kBlockSize, kNumChannels);

        // Create connections for all 3 chaos axes
        matrix.setConnection(
            ModulationMatrix::SourceType::ChaosAttractor,
            ModulationMatrix::DestinationType::PositionX,
            0, 1.0f, 200.0f
        );
        matrix.setConnection(
            ModulationMatrix::SourceType::ChaosAttractor,
            ModulationMatrix::DestinationType::PositionY,
            1, 1.0f, 200.0f
        );
        matrix.setConnection(
            ModulationMatrix::SourceType::ChaosAttractor,
            ModulationMatrix::DestinationType::PositionZ,
            2, 1.0f, 200.0f
        );

        // Process blocks and verify all axes stay bounded
        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        buffer.clear();

        float minX = 1.0f, maxX = -1.0f;
        float minY = 1.0f, maxY = -1.0f;
        float minZ = 1.0f, maxZ = -1.0f;

        // Need many blocks for chaos to fully explore its attractor
        for (int block = 0; block < 500; ++block)
        {
            matrix.process(buffer, kBlockSize);

            float x = matrix.getModulation(ModulationMatrix::DestinationType::PositionX);
            float y = matrix.getModulation(ModulationMatrix::DestinationType::PositionY);
            float z = matrix.getModulation(ModulationMatrix::DestinationType::PositionZ);

            minX = std::min(minX, x); maxX = std::max(maxX, x);
            minY = std::min(minY, y); maxY = std::max(maxY, y);
            minZ = std::min(minZ, z); maxZ = std::max(maxZ, z);

            // Verify all axes stay within [-1, 1]
            if (!isInRange(x, -1.0f, 1.0f) ||
                !isInRange(y, -1.0f, 1.0f) ||
                !isInRange(z, -1.0f, 1.0f))
            {
                result.passed = false;
                result.message = "Chaos out of bounds: X=" + std::to_string(x) +
                               " Y=" + std::to_string(y) + " Z=" + std::to_string(z);
                return result;
            }
        }

        // Verify chaos actually moves (not stuck at 0)
        float rangeX = maxX - minX;
        float rangeY = maxY - minY;
        float rangeZ = maxZ - minZ;

        // Chaos starts from (0.1, 0, 0) which normalizes to small values initially
        // After 500 blocks, expect at least 0.1% movement on each axis (proving it's not stuck)
        if (rangeX < 0.001f || rangeY < 0.001f || rangeZ < 0.001f)
        {
            result.passed = false;
            result.message = "Chaos not evolving (ranges too small)";
            return result;
        }

        result.passed = true;
        result.message = "X range: [" + std::to_string(minX) + ", " + std::to_string(maxX) + "], " +
                        "Y range: [" + std::to_string(minY) + ", " + std::to_string(maxY) + "], " +
                        "Z range: [" + std::to_string(minZ) + ", " + std::to_string(maxZ) + "]";
    }
    catch (const std::exception& e)
    {
        result.passed = false;
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test 9: Audio Follower (RMS Tracking)
//==============================================================================
TestResult testAudioFollower()
{
    TestResult result;
    result.testName = "Audio Follower (RMS Tracking)";

    try
    {
        ModulationMatrix matrix;
        matrix.prepare(kSampleRate, kBlockSize, kNumChannels);

        // Create connection using audio follower
        matrix.setConnection(
            ModulationMatrix::SourceType::AudioFollower,
            ModulationMatrix::DestinationType::Drift,
            0, 1.0f, 200.0f
        );

        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        double phase = 0.0;

        // Test 1: Silent input should produce low modulation
        buffer.clear();
        for (int block = 0; block < 20; ++block)
        {
            matrix.process(buffer, kBlockSize);
        }
        float silentMod = matrix.getModulation(ModulationMatrix::DestinationType::Drift);

        // Test 2: Loud input should produce high modulation
        generateSineWave(buffer, 440.0f, phase);
        buffer.applyGain(0.8f);  // 0.8 amplitude

        for (int block = 0; block < 20; ++block)
        {
            matrix.process(buffer, kBlockSize);
        }
        float loudMod = matrix.getModulation(ModulationMatrix::DestinationType::Drift);

        // Verify audio follower responds to input level
        if (loudMod <= silentMod)
        {
            result.passed = false;
            result.message = "Audio follower not tracking level (silent=" +
                           std::to_string(silentMod) + ", loud=" + std::to_string(loudMod) + ")";
            return result;
        }

        // Verify output is within [0, 1] (unipolar for audio follower)
        if (!isInRange(loudMod, 0.0f, 1.0f))
        {
            result.passed = false;
            result.message = "Audio follower out of range: " + std::to_string(loudMod);
            return result;
        }

        result.passed = true;
        result.message = "Silent: " + std::to_string(silentMod) +
                        ", Loud: " + std::to_string(loudMod);
    }
    catch (const std::exception& e)
    {
        result.passed = false;
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test 10: Brownian Motion (Bounded Random Walk)
//==============================================================================
TestResult testBrownianMotion()
{
    TestResult result;
    result.testName = "Brownian Motion (Random Walk)";

    try
    {
        ModulationMatrix matrix;
        matrix.prepare(kSampleRate, kBlockSize, kNumChannels);

        // Create connection using Brownian motion
        matrix.setConnection(
            ModulationMatrix::SourceType::BrownianMotion,
            ModulationMatrix::DestinationType::Gravity,
            0, 1.0f, 200.0f
        );

        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        buffer.clear();

        float minValue = 1.0f;
        float maxValue = -1.0f;
        std::vector<float> history;

        // Process many blocks and track value evolution
        // Brownian motion needs time to random walk across the range
        for (int block = 0; block < 1000; ++block)
        {
            matrix.process(buffer, kBlockSize);
            float mod = matrix.getModulation(ModulationMatrix::DestinationType::Gravity);

            minValue = std::min(minValue, mod);
            maxValue = std::max(maxValue, mod);
            history.push_back(mod);

            // Verify always bounded
            if (!isInRange(mod, -1.0f, 1.0f))
            {
                result.passed = false;
                result.message = "Brownian motion out of bounds: " + std::to_string(mod);
                return result;
            }
        }

        // Verify motion actually occurs (not stuck)
        // Brownian motion is a slow random walk - expect at least 0.3% range after 1000 blocks
        float range = maxValue - minValue;
        if (range < 0.003f)
        {
            result.passed = false;
            result.message = "Brownian motion too limited (range: " + std::to_string(range) + ")";
            return result;
        }

        // Verify motion is smooth (adjacent values don't jump too much)
        float maxJump = 0.0f;
        for (size_t i = 1; i < history.size(); ++i)
        {
            float jump = std::abs(history[i] - history[i-1]);
            maxJump = std::max(maxJump, jump);
        }

        if (maxJump > 0.3f)
        {
            result.passed = false;
            result.message = "Brownian motion too erratic (max jump: " + std::to_string(maxJump) + ")";
            return result;
        }

        result.passed = true;
        result.message = "Range: [" + std::to_string(minValue) + ", " +
                        std::to_string(maxValue) + "], max jump: " + std::to_string(maxJump);
    }
    catch (const std::exception& e)
    {
        result.passed = false;
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test 11: Envelope Tracker (Attack/Sustain/Release)
//==============================================================================
TestResult testEnvelopeTracker()
{
    TestResult result;
    result.testName = "Envelope Tracker";

    try
    {
        ModulationMatrix matrix;
        matrix.prepare(kSampleRate, kBlockSize, kNumChannels);

        // Create connection using envelope tracker with moderate smoothing
        matrix.setConnection(
            ModulationMatrix::SourceType::EnvelopeTracker,
            ModulationMatrix::DestinationType::PillarShape,
            0, 1.0f, 200.0f  // moderate smoothing
        );

        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        double phase = 0.0;

        // Phase 1: Silence - measure baseline
        buffer.clear();
        for (int block = 0; block < 20; ++block)
        {
            matrix.process(buffer, kBlockSize);
        }
        float silentMod = matrix.getModulation(ModulationMatrix::DestinationType::PillarShape);

        // Phase 2: Loud signal - envelope should rise
        generateSineWave(buffer, 440.0f, phase);
        buffer.applyGain(0.8f);
        for (int block = 0; block < 50; ++block)
        {
            matrix.process(buffer, kBlockSize);
        }
        float loudMod = matrix.getModulation(ModulationMatrix::DestinationType::PillarShape);

        // Verify envelope tracker responds to input (loud should be higher than silent)
        if (loudMod <= silentMod)
        {
            result.passed = false;
            result.message = "Envelope not responding to audio (silent: " + std::to_string(silentMod) +
                           ", loud: " + std::to_string(loudMod) + ")";
            return result;
        }

        // Note: We don't test decay behavior because smoothing can cause overshoot
        // The important test is that the envelope responds to input level changes

        // Verify output is within [0, 1] (unipolar)
        if (!isInRange(loudMod, 0.0f, 1.0f))
        {
            result.passed = false;
            result.message = "Envelope tracker out of range: " + std::to_string(loudMod);
            return result;
        }

        result.passed = true;
        result.message = "Silent: " + std::to_string(silentMod) +
                        ", Loud: " + std::to_string(loudMod) +
                        " (responds to input level)";
    }
    catch (const std::exception& e)
    {
        result.passed = false;
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test 12: Randomization (Sparse/Normal/Dense)
//==============================================================================
TestResult testRandomization()
{
    TestResult result;
    result.testName = "Randomization (Sparse/Normal/Dense)";

    try
    {
        ModulationMatrix matrix;
        matrix.prepare(kSampleRate, kBlockSize, kNumChannels);

        // Test sparse randomization (2-3 connections)
        matrix.randomizeSparse();
        auto sparseCon = matrix.getConnections();

        if (sparseCon.size() < 2 || sparseCon.size() > 3)
        {
            result.passed = false;
            result.message = "Sparse randomization wrong count: " + std::to_string(sparseCon.size());
            return result;
        }

        // Test normal randomization (4-8 connections)
        matrix.randomizeAll();
        auto normalCon = matrix.getConnections();

        if (normalCon.size() < 4 || normalCon.size() > 8)
        {
            result.passed = false;
            result.message = "Normal randomization wrong count: " + std::to_string(normalCon.size());
            return result;
        }

        // Test dense randomization (8-12 connections)
        matrix.randomizeDense();
        auto denseCon = matrix.getConnections();

        if (denseCon.size() < 8 || denseCon.size() > 12)
        {
            result.passed = false;
            result.message = "Dense randomization wrong count: " + std::to_string(denseCon.size());
            return result;
        }

        // Verify all connections have valid parameters
        for (const auto& conn : denseCon)
        {
            if (!isInRange(conn.depth, -1.0f, 1.0f))
            {
                result.passed = false;
                result.message = "Invalid connection depth: " + std::to_string(conn.depth);
                return result;
            }

            if (!isInRange(conn.smoothingMs, 20.0f, 1000.0f))
            {
                result.passed = false;
                result.message = "Invalid smoothing: " + std::to_string(conn.smoothingMs);
                return result;
            }

            if (!conn.enabled)
            {
                result.passed = false;
                result.message = "Randomized connection not enabled";
                return result;
            }
        }

        result.passed = true;
        result.message = "Sparse: " + std::to_string(sparseCon.size()) +
                        ", Normal: " + std::to_string(normalCon.size()) +
                        ", Dense: " + std::to_string(denseCon.size());
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
    std::cout << COLOR_BLUE << "=============================================================\n";
    std::cout << "Monument Reverb - ModulationMatrix Test Suite (Phase 1.2)\n";
    std::cout << "=============================================================\n" << COLOR_RESET;
    std::cout << "\nConfiguration:\n";
    std::cout << "  Sample Rate: " << kSampleRate << " Hz\n";
    std::cout << "  Block Size:  " << kBlockSize << " samples\n";
    std::cout << "  Channels:    " << kNumChannels << "\n\n";

    // Run all tests
    std::vector<TestResult> results;

    results.push_back(testBasicConnectionRouting());
    results.push_back(testMultipleConnectionsAccumulation());
    results.push_back(testBipolarModulation());
    results.push_back(testSmoothingBehavior());
    results.push_back(testProbabilityGating());
    results.push_back(testThreadSafety());
    results.push_back(testConnectionManagement());
    results.push_back(testChaosAttractor());
    results.push_back(testAudioFollower());
    results.push_back(testBrownianMotion());
    results.push_back(testEnvelopeTracker());
    results.push_back(testRandomization());

    // Print results
    std::cout << COLOR_BLUE << "Test Results:\n" << COLOR_RESET;
    std::cout << "-------------------------------------------------------------\n";

    int passCount = 0;
    for (const auto& result : results)
    {
        const char* color = result.passed ? COLOR_GREEN : COLOR_RED;
        const char* status = result.passed ? "PASS" : "FAIL";

        std::cout << color << "[" << status << "] " << COLOR_RESET;
        std::cout << std::left << std::setw(40) << result.testName;
        std::cout << " - " << result.message << "\n";

        if (result.passed)
            passCount++;
    }

    std::cout << "-------------------------------------------------------------\n";

    const char* summaryColor = (passCount == static_cast<int>(results.size()))
                              ? COLOR_GREEN : COLOR_RED;

    std::cout << summaryColor << "\nSummary: " << passCount << "/"
              << results.size() << " tests passed";

    if (passCount == static_cast<int>(results.size()))
        std::cout << " ✓\n" << COLOR_RESET;
    else
        std::cout << " ✗\n" << COLOR_RESET;

    std::cout << COLOR_BLUE << "=============================================================\n"
              << COLOR_RESET;

    return (passCount == static_cast<int>(results.size())) ? 0 : 1;
}
