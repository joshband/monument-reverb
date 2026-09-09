/**
 * Monument Reverb - Pillars Fractional Delay Zipper Test
 *
 * Tests Phase 5's fractional delay interpolation effectiveness at eliminating
 * zipper noise during Pillars tap position changes.
 *
 * This test specifically targets Pillars parameters (shape, density, warp) and
 * uses quiet input to allow the deferred tap update mechanism to trigger.
 *
 * Success Criteria:
 * - Zipper noise < -40 dB during rapid pillarShape sweeps
 * - Zipper noise < -40 dB during rapid density sweeps
 * - Zipper noise < -40 dB during rapid warp sweeps
 * - Combined parameter sweeps also < -40 dB
 *
 * Background:
 * - Pillars only updates tap layout when input < -60 dB (kTapUpdateThreshold)
 * - Phase 5 added fractional delays with 500ms position smoothing
 * - This test verifies those changes eliminate position-change artifacts
 *
 * Usage:
 *   ./monument_pillars_zipper_test              # Full test suite
 *   ./monument_pillars_zipper_test --quick      # Quick subset (30s)
 */

#include <JuceHeader.h>
#include "plugin/PluginProcessor.h"
#include <iostream>
#include <iomanip>
#include <memory>
#include <cmath>
#include <vector>
#include <algorithm>
#include <random>

// ANSI color codes for terminal output
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_BLUE "\033[0;34m"
#define COLOR_CYAN "\033[0;36m"
#define COLOR_MAGENTA "\033[0;35m"
#define COLOR_RESET "\033[0m"

// Test configuration
constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 512;
constexpr int kNumChannels = 2;
constexpr int kTestDurationSeconds = 10;  // Duration for stress tests
constexpr int kNumBlocks = static_cast<int>((kSampleRate * kTestDurationSeconds) / kBlockSize);

// Signal levels
constexpr float kQuietNoiseDb = -70.0f;  // Below -60dB threshold to allow tap updates
constexpr float kQuietNoiseLinear = 0.000316f;  // 10^(-70/20)

// Zipper noise threshold
constexpr float kZipperThresholdDb = -40.0f;

struct TestResult
{
    std::string testName;
    bool passed;
    std::string message;
    double value{0.0};  // Numeric result (dB)
};

//==============================================================================
// Helper: Calculate Maximum Sample-to-Sample Jump (Zipper Noise)
//==============================================================================
static float calculateMaxJump(const juce::AudioBuffer<float>& buffer)
{
    float maxJump = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const float* channelData = buffer.getReadPointer(ch);
        for (int i = 1; i < buffer.getNumSamples(); ++i)
        {
            float jump = std::abs(channelData[i] - channelData[i - 1]);
            maxJump = std::max(maxJump, jump);
        }
    }
    return maxJump;
}

//==============================================================================
// Helper: Check for Inf/NaN
//==============================================================================
static bool hasInvalidNumbers(const juce::AudioBuffer<float>& buffer)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const float* channelData = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            if (!std::isfinite(channelData[i]))
                return true;
        }
    }
    return false;
}

//==============================================================================
// Helper: Generate Quiet Pink Noise (-70 dB)
//==============================================================================
static void generateQuietNoise(juce::AudioBuffer<float>& buffer, std::mt19937& rng)
{
    std::uniform_real_distribution<float> dist(-kQuietNoiseLinear, kQuietNoiseLinear);
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        float* channelData = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            channelData[i] = dist(rng);
        }
    }
}

//==============================================================================
// Test PILLARS-1: Rapid Shape Parameter Sweep
//==============================================================================
static TestResult testShapeParameterSweep()
{
    TestResult result{"PILLARS-1: Shape Parameter Sweep", false, ""};

    try
    {
        MonumentAudioProcessor processor;
        processor.prepareToPlay(kSampleRate, kBlockSize);

        // Get pillarShape parameter
        auto& apvts = processor.getAPVTS();
        auto* shapeParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("pillarShape"));

        if (!shapeParam)
        {
            result.message = "pillarShape parameter not found";
            return result;
        }

        // Set moderate values for other parameters
        if (auto* densityParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("density")))
            densityParam->setValueNotifyingHost(0.5f);
        if (auto* warpParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("warp")))
            warpParam->setValueNotifyingHost(0.0f);  // Disable warp mutations

        // Prepare quiet noise generator
        std::mt19937 rng(42);
        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        juce::MidiBuffer midiBuffer;

        // Process initial blocks to let reverb settle
        for (int block = 0; block < 50; ++block)
        {
            generateQuietNoise(buffer, rng);
            processor.processBlock(buffer, midiBuffer);
        }

        // Sweep shape parameter rapidly (5 Hz sine wave) and measure zipper noise
        float maxJump = 0.0f;
        const float sweepFreq = 5.0f;
        int sampleCounter = 0;

        std::cout << "  Sweeping pillarShape at " << sweepFreq << " Hz...\n";

        for (int block = 0; block < kNumBlocks / 2; ++block)  // 5 seconds
        {
            // Modulate shape parameter at sweep frequency
            float phase = (sampleCounter / kSampleRate) * sweepFreq * 2.0f * juce::MathConstants<float>::pi;
            float paramValue = (std::sin(phase) + 1.0f) * 0.5f;
            shapeParam->setValueNotifyingHost(paramValue);

            generateQuietNoise(buffer, rng);
            processor.processBlock(buffer, midiBuffer);

            if (hasInvalidNumbers(buffer))
            {
                result.message = "Inf/NaN detected during shape sweep";
                return result;
            }

            float jump = calculateMaxJump(buffer);
            maxJump = std::max(maxJump, jump);

            sampleCounter += kBlockSize;

            // Progress indicator
            if (block % (kNumBlocks / 20) == 0)
            {
                std::cout << "    Progress: " << (block * 200 / kNumBlocks) << "%\r" << std::flush;
            }
        }
        std::cout << "                  \r" << std::flush;

        // Convert to dB
        float jumpDb = 20.0f * std::log10(maxJump + 1e-10f);

        if (jumpDb > kZipperThresholdDb)
        {
            result.message = "Zipper noise detected: " + std::to_string(jumpDb) +
                           " dB (threshold: " + std::to_string(kZipperThresholdDb) + " dB)";
            result.value = jumpDb;
        }
        else
        {
            result.passed = true;
            result.message = "Fractional delays effective: " + std::to_string(jumpDb) + " dB";
            result.value = jumpDb;
        }
    }
    catch (const std::exception& e)
    {
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test PILLARS-2: Rapid Density Parameter Sweep
//==============================================================================
static TestResult testDensityParameterSweep()
{
    TestResult result{"PILLARS-2: Density Parameter Sweep", false, ""};

    try
    {
        MonumentAudioProcessor processor;
        processor.prepareToPlay(kSampleRate, kBlockSize);

        auto& apvts = processor.getAPVTS();
        auto* densityParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("density"));

        if (!densityParam)
        {
            result.message = "density parameter not found";
            return result;
        }

        // Set moderate values for other parameters
        if (auto* shapeParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("pillarShape")))
            shapeParam->setValueNotifyingHost(0.5f);
        if (auto* warpParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("warp")))
            warpParam->setValueNotifyingHost(0.0f);

        std::mt19937 rng(43);
        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        juce::MidiBuffer midiBuffer;

        // Settle period
        for (int block = 0; block < 50; ++block)
        {
            generateQuietNoise(buffer, rng);
            processor.processBlock(buffer, midiBuffer);
        }

        // Sweep density
        float maxJump = 0.0f;
        const float sweepFreq = 5.0f;
        int sampleCounter = 0;

        std::cout << "  Sweeping density at " << sweepFreq << " Hz...\n";

        for (int block = 0; block < kNumBlocks / 2; ++block)
        {
            float phase = (sampleCounter / kSampleRate) * sweepFreq * 2.0f * juce::MathConstants<float>::pi;
            float paramValue = (std::sin(phase) + 1.0f) * 0.5f;
            densityParam->setValueNotifyingHost(paramValue);

            generateQuietNoise(buffer, rng);
            processor.processBlock(buffer, midiBuffer);

            if (hasInvalidNumbers(buffer))
            {
                result.message = "Inf/NaN detected during density sweep";
                return result;
            }

            float jump = calculateMaxJump(buffer);
            maxJump = std::max(maxJump, jump);

            sampleCounter += kBlockSize;

            if (block % (kNumBlocks / 20) == 0)
            {
                std::cout << "    Progress: " << (block * 200 / kNumBlocks) << "%\r" << std::flush;
            }
        }
        std::cout << "                  \r" << std::flush;

        float jumpDb = 20.0f * std::log10(maxJump + 1e-10f);

        if (jumpDb > kZipperThresholdDb)
        {
            result.message = "Zipper noise detected: " + std::to_string(jumpDb) + " dB";
            result.value = jumpDb;
        }
        else
        {
            result.passed = true;
            result.message = "Fractional delays effective: " + std::to_string(jumpDb) + " dB";
            result.value = jumpDb;
        }
    }
    catch (const std::exception& e)
    {
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test PILLARS-3: Rapid Warp Parameter Sweep
//==============================================================================
static TestResult testWarpParameterSweep()
{
    TestResult result{"PILLARS-3: Warp Parameter Sweep", false, ""};

    try
    {
        MonumentAudioProcessor processor;
        processor.prepareToPlay(kSampleRate, kBlockSize);

        auto& apvts = processor.getAPVTS();
        auto* warpParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("warp"));

        if (!warpParam)
        {
            result.message = "warp parameter not found";
            return result;
        }

        // Set moderate values for other parameters
        if (auto* shapeParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("pillarShape")))
            shapeParam->setValueNotifyingHost(0.5f);
        if (auto* densityParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("density")))
            densityParam->setValueNotifyingHost(0.5f);

        std::mt19937 rng(44);
        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        juce::MidiBuffer midiBuffer;

        // Settle period
        for (int block = 0; block < 50; ++block)
        {
            generateQuietNoise(buffer, rng);
            processor.processBlock(buffer, midiBuffer);
        }

        // Sweep warp (triggers periodic tap mutations)
        float maxJump = 0.0f;
        const float sweepFreq = 5.0f;
        int sampleCounter = 0;

        std::cout << "  Sweeping warp at " << sweepFreq << " Hz...\n";

        for (int block = 0; block < kNumBlocks / 2; ++block)
        {
            float phase = (sampleCounter / kSampleRate) * sweepFreq * 2.0f * juce::MathConstants<float>::pi;
            float paramValue = (std::sin(phase) + 1.0f) * 0.5f;
            warpParam->setValueNotifyingHost(paramValue);

            generateQuietNoise(buffer, rng);
            processor.processBlock(buffer, midiBuffer);

            if (hasInvalidNumbers(buffer))
            {
                result.message = "Inf/NaN detected during warp sweep";
                return result;
            }

            float jump = calculateMaxJump(buffer);
            maxJump = std::max(maxJump, jump);

            sampleCounter += kBlockSize;

            if (block % (kNumBlocks / 20) == 0)
            {
                std::cout << "    Progress: " << (block * 200 / kNumBlocks) << "%\r" << std::flush;
            }
        }
        std::cout << "                  \r" << std::flush;

        float jumpDb = 20.0f * std::log10(maxJump + 1e-10f);

        if (jumpDb > kZipperThresholdDb)
        {
            result.message = "Zipper noise detected: " + std::to_string(jumpDb) + " dB";
            result.value = jumpDb;
        }
        else
        {
            result.passed = true;
            result.message = "Fractional delays effective: " + std::to_string(jumpDb) + " dB";
            result.value = jumpDb;
        }
    }
    catch (const std::exception& e)
    {
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test PILLARS-4: Combined Multi-Parameter Sweep (Worst Case)
//==============================================================================
static TestResult testCombinedParameterSweep()
{
    TestResult result{"PILLARS-4: Combined Multi-Parameter Sweep", false, ""};

    try
    {
        MonumentAudioProcessor processor;
        processor.prepareToPlay(kSampleRate, kBlockSize);

        auto& apvts = processor.getAPVTS();
        auto* shapeParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("pillarShape"));
        auto* densityParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("density"));
        auto* warpParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("warp"));

        if (!shapeParam || !densityParam || !warpParam)
        {
            result.message = "Required parameters not found";
            return result;
        }

        std::mt19937 rng(45);
        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        juce::MidiBuffer midiBuffer;

        // Settle period
        for (int block = 0; block < 50; ++block)
        {
            generateQuietNoise(buffer, rng);
            processor.processBlock(buffer, midiBuffer);
        }

        // Sweep all three parameters simultaneously at different frequencies
        float maxJump = 0.0f;
        int sampleCounter = 0;

        std::cout << "  Sweeping shape (5Hz), density (7Hz), warp (3Hz) simultaneously...\n";

        for (int block = 0; block < kNumBlocks / 2; ++block)
        {
            float time = sampleCounter / kSampleRate;

            // Different frequencies to create complex modulation
            float shapePhase = time * 5.0f * 2.0f * juce::MathConstants<float>::pi;
            float densityPhase = time * 7.0f * 2.0f * juce::MathConstants<float>::pi;
            float warpPhase = time * 3.0f * 2.0f * juce::MathConstants<float>::pi;

            shapeParam->setValueNotifyingHost((std::sin(shapePhase) + 1.0f) * 0.5f);
            densityParam->setValueNotifyingHost((std::sin(densityPhase) + 1.0f) * 0.5f);
            warpParam->setValueNotifyingHost((std::sin(warpPhase) + 1.0f) * 0.5f);

            generateQuietNoise(buffer, rng);
            processor.processBlock(buffer, midiBuffer);

            if (hasInvalidNumbers(buffer))
            {
                result.message = "Inf/NaN detected during combined sweep";
                return result;
            }

            float jump = calculateMaxJump(buffer);
            maxJump = std::max(maxJump, jump);

            sampleCounter += kBlockSize;

            if (block % (kNumBlocks / 20) == 0)
            {
                std::cout << "    Progress: " << (block * 200 / kNumBlocks) << "%\r" << std::flush;
            }
        }
        std::cout << "                  \r" << std::flush;

        float jumpDb = 20.0f * std::log10(maxJump + 1e-10f);

        if (jumpDb > kZipperThresholdDb)
        {
            result.message = "Zipper noise detected: " + std::to_string(jumpDb) + " dB";
            result.value = jumpDb;
        }
        else
        {
            result.passed = true;
            result.message = "Fractional delays effective under worst-case: " + std::to_string(jumpDb) + " dB";
            result.value = jumpDb;
        }
    }
    catch (const std::exception& e)
    {
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test PILLARS-5: pillarMode Parameter Sweep (Click Safety)
//==============================================================================
static TestResult testModeParameterSweep()
{
    TestResult result{"PILLARS-5: Mode Parameter Sweep", false, ""};

    try
    {
        MonumentAudioProcessor processor;
        processor.prepareToPlay(kSampleRate, kBlockSize);

        auto& apvts = processor.getAPVTS();
        auto* modeParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("pillarMode"));

        if (!modeParam)
        {
            result.message = "pillarMode parameter not found";
            return result;
        }

        std::mt19937 rng(46);
        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        juce::MidiBuffer midiBuffer;

        // Settle period
        for (int block = 0; block < 50; ++block)
        {
            generateQuietNoise(buffer, rng);
            processor.processBlock(buffer, midiBuffer);
        }

        // Cycle Glass -> Stone -> Fog -> Glass rapidly (every 10 blocks) while
        // measuring worst-case sample-to-sample jump, same as the other
        // Pillars parameter sweeps in this file.
        float maxJump = 0.0f;
        int blockCounter = 0;

        std::cout << "  Cycling pillarMode (Glass/Stone/Fog) every 10 blocks...\n";

        for (int block = 0; block < kNumBlocks / 2; ++block)
        {
            if (blockCounter % 10 == 0)
                *modeParam = (blockCounter / 10) % 3;

            generateQuietNoise(buffer, rng);
            processor.processBlock(buffer, midiBuffer);

            if (hasInvalidNumbers(buffer))
            {
                result.message = "Inf/NaN detected during mode sweep";
                return result;
            }

            float jump = calculateMaxJump(buffer);
            maxJump = std::max(maxJump, jump);

            ++blockCounter;

            if (block % (kNumBlocks / 20) == 0)
            {
                std::cout << "    Progress: " << (block * 200 / kNumBlocks) << "%\r" << std::flush;
            }
        }
        std::cout << "                  \r" << std::flush;

        float jumpDb = 20.0f * std::log10(maxJump + 1e-10f);

        if (jumpDb > kZipperThresholdDb)
        {
            result.message = "Zipper noise detected: " + std::to_string(jumpDb) +
                           " dB (threshold: " + std::to_string(kZipperThresholdDb) + " dB)";
            result.value = jumpDb;
        }
        else
        {
            result.passed = true;
            result.message = "Mode switches click-free: " + std::to_string(jumpDb) + " dB";
            result.value = jumpDb;
        }
    }
    catch (const std::exception& e)
    {
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test PILLARS-6: pillarMode Actually Affects Output (Wiring Regression)
//==============================================================================
static TestResult testModeAffectsOutput()
{
    TestResult result{"PILLARS-6: Mode Affects Output", false, ""};

    try
    {
        MonumentAudioProcessor processorGlass;
        MonumentAudioProcessor processorFog;
        processorGlass.prepareToPlay(kSampleRate, kBlockSize);
        processorFog.prepareToPlay(kSampleRate, kBlockSize);

        auto* fogModeParam = dynamic_cast<juce::AudioParameterChoice*>(
            processorFog.getAPVTS().getParameter("pillarMode"));

        if (!fogModeParam)
        {
            result.message = "pillarMode parameter not found";
            return result;
        }

        *fogModeParam = 2; // Fog (Glass is the default on processorGlass)

        std::mt19937 rngGlass(47);
        juce::AudioBuffer<float> bufferGlass(kNumChannels, kBlockSize);
        juce::AudioBuffer<float> bufferFog(kNumChannels, kBlockSize);
        juce::MidiBuffer midiBuffer;

        float maxDifference = 0.0f;

        // pillarMode's lowpass/highpass coloration is applied unconditionally
        // every sample (not deferred like tap layout), so identical input
        // should diverge from the very first block if the mode is wired in.
        for (int block = 0; block < 10; ++block)
        {
            generateQuietNoise(bufferGlass, rngGlass);
            bufferFog.makeCopyOf(bufferGlass);

            processorGlass.processBlock(bufferGlass, midiBuffer);
            processorFog.processBlock(bufferFog, midiBuffer);

            if (hasInvalidNumbers(bufferGlass) || hasInvalidNumbers(bufferFog))
            {
                result.message = "Inf/NaN detected while comparing pillarMode output";
                return result;
            }

            for (int ch = 0; ch < kNumChannels; ++ch)
            {
                const float* glassData = bufferGlass.getReadPointer(ch);
                const float* fogData = bufferFog.getReadPointer(ch);
                for (int i = 0; i < kBlockSize; ++i)
                    maxDifference = std::max(maxDifference, std::abs(glassData[i] - fogData[i]));
            }
        }

        constexpr float kMinExpectedDifference = 1.0e-6f;
        if (maxDifference < kMinExpectedDifference)
        {
            result.message = "pillarMode has no audible effect (max diff " +
                           std::to_string(maxDifference) + " — Glass and Fog outputs are identical)";
            result.value = maxDifference;
        }
        else
        {
            result.passed = true;
            result.message = "pillarMode is wired: max diff " + std::to_string(maxDifference);
            result.value = maxDifference;
        }
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
int main(int argc, char* argv[])
{
    bool quickMode = false;
    if (argc > 1 && std::string(argv[1]) == "--quick")
    {
        quickMode = true;
        std::cout << COLOR_CYAN << "Running in QUICK mode (subset of tests)\n" << COLOR_RESET;
    }

    std::cout << "\n";
    std::cout << COLOR_CYAN << "=======================================================\n";
    std::cout << "Monument Reverb - Pillars Fractional Delay Zipper Test\n";
    std::cout << "=======================================================\n" << COLOR_RESET;
    std::cout << "\n";
    std::cout << COLOR_YELLOW << "Purpose: " << COLOR_RESET
              << "Verify Phase 5's fractional delay interpolation\n";
    std::cout << COLOR_YELLOW << "         " << COLOR_RESET
              << "eliminates zipper noise during tap position changes\n";
    std::cout << "\n";
    std::cout << COLOR_YELLOW << "Method:  " << COLOR_RESET
              << "Quiet input (-70dB) allows deferred tap updates\n";
    std::cout << COLOR_YELLOW << "         " << COLOR_RESET
              << "Rapid parameter sweeps (5-7 Hz) trigger recalculations\n";
    std::cout << "\n";

    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI juceInit;

    std::vector<TestResult> results;

    // Run tests
    std::cout << COLOR_BLUE << "=== Pillars Zipper Noise Tests ===" << COLOR_RESET << "\n\n";

    results.push_back(testShapeParameterSweep());
    results.push_back(testDensityParameterSweep());
    results.push_back(testModeAffectsOutput());

    if (!quickMode)
    {
        results.push_back(testWarpParameterSweep());
        results.push_back(testCombinedParameterSweep());
        results.push_back(testModeParameterSweep());
    }

    // Print results
    std::cout << "\n";
    std::cout << COLOR_CYAN << "===============================================\n";
    std::cout << "Test Results\n";
    std::cout << "===============================================\n" << COLOR_RESET;

    int passed = 0;
    int failed = 0;

    for (const auto& result : results)
    {
        std::string status = result.passed ? COLOR_GREEN "✓ PASS" : COLOR_RED "✗ FAIL";
        std::cout << status << COLOR_RESET << " | " << result.testName << "\n";
        std::cout << "      " << result.message << "\n";

        if (result.passed)
            ++passed;
        else
            ++failed;
    }

    std::cout << "\n";
    std::cout << COLOR_CYAN << "===============================================\n";
    std::cout << "Summary: " << passed << "/" << results.size() << " tests passed";
    if (failed > 0)
        std::cout << " (" << failed << " " << COLOR_RED << "FAILED" << COLOR_CYAN << ")";
    std::cout << "\n===============================================\n" << COLOR_RESET;

    if (passed == results.size())
    {
        std::cout << "\n" << COLOR_GREEN;
        std::cout << "✅ Phase 5 fractional delays successfully eliminate Pillars zipper noise!\n";
        std::cout << COLOR_RESET;
    }
    else
    {
        std::cout << "\n" << COLOR_RED;
        std::cout << "⚠️  Fractional delays did not fully eliminate zipper noise.\n";
        std::cout << "   Consider: longer smoothing time, higher-order interpolation, or cross-fading.\n";
        std::cout << COLOR_RESET;
    }

    return failed > 0 ? 1 : 0;
}
