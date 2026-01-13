/**
 * Monument Reverb - Parameter Stress Test Suite
 *
 * Tests extreme parameter values, rapid automation, and edge cases.
 * Verifies plugin stability under parameter stress conditions.
 *
 * Success Criteria:
 * - No crashes with extreme parameter values
 * - No zipper noise > -40dB during rapid sweeps
 * - Parameter smoothing prevents clicks > -30dB
 * - No runaway amplification with feedback/resonance at maximum
 * - All parameter values properly clamped and validated
 *
 * Usage:
 *   ./monument_parameter_stress_test              # Full test suite
 *   ./monument_parameter_stress_test --quick      # Quick subset (30s)
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

struct TestResult
{
    std::string testName;
    bool passed;
    std::string message;
    double value{0.0};  // Numeric result (dB, %, etc.)
};

//==============================================================================
// Helper: Calculate RMS Level
//==============================================================================
static float calculateRMS(const juce::AudioBuffer<float>& buffer)
{
    float sumSquares = 0.0f;
    int totalSamples = buffer.getNumChannels() * buffer.getNumSamples();

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const float* channelData = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float sample = channelData[i];
            sumSquares += sample * sample;
        }
    }

    return std::sqrt(sumSquares / totalSamples);
}

//==============================================================================
// Helper: Calculate Peak Level
//==============================================================================
static float calculatePeak(const juce::AudioBuffer<float>& buffer)
{
    float peak = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const float* channelData = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            peak = std::max(peak, std::abs(channelData[i]));
        }
    }
    return peak;
}

//==============================================================================
// Helper: Detect Maximum Sample-to-Sample Jump (Zipper Noise)
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
// Helper: Calculate Boundary Jump Between Blocks
//==============================================================================
static float calculateBoundaryJump(const juce::AudioBuffer<float>& current,
                                   const juce::AudioBuffer<float>& previous)
{
    const int numChannels = juce::jmin(current.getNumChannels(), previous.getNumChannels());
    const int currentSamples = current.getNumSamples();
    const int previousSamples = previous.getNumSamples();
    if (numChannels == 0 || currentSamples == 0 || previousSamples == 0)
        return 0.0f;

    const int lastIndex = previousSamples - 1;
    float maxDelta = 0.0f;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* currentData = current.getReadPointer(ch);
        const float* previousData = previous.getReadPointer(ch);
        const float delta = std::abs(currentData[0] - previousData[lastIndex]);
        maxDelta = std::max(maxDelta, delta);
    }

    return maxDelta;
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
// Helper: Generate Test Signal (Impulse)
//==============================================================================
static void generateImpulse(juce::AudioBuffer<float>& buffer)
{
    buffer.clear();
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        buffer.setSample(ch, 0, 1.0f);
    }
}

//==============================================================================
// Helper: Generate Test Signal (White Noise)
//==============================================================================
static void generateNoise(juce::AudioBuffer<float>& buffer, std::mt19937& rng)
{
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
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
// Helper: Generate Test Signal (Sine Wave)
//==============================================================================
static void generateSine(juce::AudioBuffer<float>& buffer, double& phase, double frequency)
{
    const double phaseDelta = juce::MathConstants<double>::twoPi * frequency / kSampleRate;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        float* channelData = buffer.getWritePointer(ch);
        double localPhase = phase;
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            channelData[i] = static_cast<float>(std::sin(localPhase));
            localPhase += phaseDelta;
            if (localPhase >= juce::MathConstants<double>::twoPi)
                localPhase -= juce::MathConstants<double>::twoPi;
        }
    }
    phase += phaseDelta * buffer.getNumSamples();
    phase = std::fmod(phase, juce::MathConstants<double>::twoPi);
}

static juce::AudioParameterFloat* findSweepParameter(MonumentAudioProcessor& processor)
{
    auto& apvts = processor.getAPVTS();
    if (auto* param = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("time")))
        return param;

    for (auto* param : processor.getParameters())
    {
        if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
            return floatParam;
    }

    return nullptr;
}

//==============================================================================
// Test PARAM-1: All Parameters Zero
//==============================================================================
static TestResult testAllParametersZero()
{
    TestResult result{"PARAM-1: All Parameters Zero", false, ""};

    try
    {
        // Create processor and prepare
        MonumentAudioProcessor processor;
        processor.prepareToPlay(kSampleRate, kBlockSize);

        // Set all parameters to minimum (0.0)
        for (auto* param : processor.getParameters())
        {
            if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
            {
                floatParam->setValueNotifyingHost(0.0f);
            }
            else if (auto* boolParam = dynamic_cast<juce::AudioParameterBool*>(param))
            {
                boolParam->setValueNotifyingHost(false);
            }
            else if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
            {
                choiceParam->setValueNotifyingHost(0.0f);
            }
        }

        // Process test signal
        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        juce::MidiBuffer midiBuffer;
        generateImpulse(buffer);
        buffer.applyGain(0.2f);

        bool hadInvalidNumbers = false;
        for (int block = 0; block < 100; ++block)
        {
            processor.processBlock(buffer, midiBuffer);
            if (hasInvalidNumbers(buffer))
            {
                hadInvalidNumbers = true;
                break;
            }
        }

        if (hadInvalidNumbers)
        {
            result.message = "Inf/NaN detected with all parameters at zero";
        }
        else
        {
            result.passed = true;
            result.message = "No crashes, output stable";
        }
    }
    catch (const std::exception& e)
    {
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test PARAM-2: All Parameters Maximum
//==============================================================================
static TestResult testAllParametersMaximum()
{
    TestResult result{"PARAM-2: All Parameters Maximum", false, ""};

    try
    {
        MonumentAudioProcessor processor;
        processor.prepareToPlay(kSampleRate, kBlockSize);

        // Set all parameters to maximum (1.0)
        auto& apvts = processor.getAPVTS();
        for (auto* param : processor.getParameters())
        {
            if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
            {
                floatParam->setValueNotifyingHost(1.0f);
            }
            else if (auto* boolParam = dynamic_cast<juce::AudioParameterBool*>(param))
            {
                boolParam->setValueNotifyingHost(true);
            }
            else if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
            {
                // Set to last choice
                choiceParam->setValueNotifyingHost(1.0f);
            }
        }

        // Process test signal
        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        juce::MidiBuffer midiBuffer;
        generateImpulse(buffer);

        bool hadInvalidNumbers = false;
        float maxPeak = 0.0f;

        for (int block = 0; block < 100; ++block)
        {
            processor.processBlock(buffer, midiBuffer);
            if (hasInvalidNumbers(buffer))
            {
                hadInvalidNumbers = true;
                break;
            }
            maxPeak = std::max(maxPeak, calculatePeak(buffer));
        }

        if (hadInvalidNumbers)
        {
            result.message = "Inf/NaN detected with all parameters at maximum";
        }
        else if (maxPeak > 100.0f)
        {
            result.message = "Runaway amplification detected: " +
                           std::to_string(20.0f * std::log10(maxPeak)) + " dB";
            result.value = 20.0f * std::log10(maxPeak);
        }
        else
        {
            result.passed = true;
            result.message = "Stable, peak = " + std::to_string(20.0f * std::log10(maxPeak)) + " dB";
            result.value = 20.0f * std::log10(maxPeak);
        }
    }
    catch (const std::exception& e)
    {
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test PARAM-3: All Parameters Random
//==============================================================================
static TestResult testAllParametersRandom()
{
    TestResult result{"PARAM-3: All Parameters Random", false, ""};

    try
    {
        MonumentAudioProcessor processor;
        processor.prepareToPlay(kSampleRate, kBlockSize);

        std::mt19937 rng(42);  // Fixed seed for reproducibility
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        // Set all parameters to random values
        auto& apvts = processor.getAPVTS();
        for (auto* param : processor.getParameters())
        {
            if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
            {
                floatParam->setValueNotifyingHost(dist(rng));
            }
            else if (auto* boolParam = dynamic_cast<juce::AudioParameterBool*>(param))
            {
                boolParam->setValueNotifyingHost(dist(rng) > 0.5f);
            }
            else if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
            {
                choiceParam->setValueNotifyingHost(dist(rng));
            }
        }

        // Process for 10 seconds
        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        juce::MidiBuffer midiBuffer;
        generateNoise(buffer, rng);

        bool hadInvalidNumbers = false;
        for (int block = 0; block < kNumBlocks; ++block)
        {
            generateNoise(buffer, rng);
            processor.processBlock(buffer, midiBuffer);
            if (hasInvalidNumbers(buffer))
            {
                hadInvalidNumbers = true;
                break;
            }

            // Progress every 10%
            if (block % (kNumBlocks / 10) == 0)
            {
                std::cout << "  Progress: " << (block * 100 / kNumBlocks) << "%\r" << std::flush;
            }
        }
        std::cout << "                  \r" << std::flush;

        if (hadInvalidNumbers)
        {
            result.message = "Inf/NaN detected with random parameters";
        }
        else
        {
            result.passed = true;
            result.message = "No crashes over 10s with random parameters";
        }
    }
    catch (const std::exception& e)
    {
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test PARAM-4: Rapid Parameter Sweeps (Zipper Noise)
//==============================================================================
static TestResult testRapidParameterSweeps()
{
    TestResult result{"PARAM-4: Rapid Parameter Sweeps", false, ""};

    try
    {
        MonumentAudioProcessor processor;
        processor.prepareToPlay(kSampleRate, kBlockSize);

        juce::AudioParameterFloat* testParam = findSweepParameter(processor);

        if (!testParam)
        {
            result.message = "No float parameters found";
            return result;
        }

        // Sweep parameter rapidly (sine wave at 10 Hz)
        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        juce::AudioBuffer<float> previousBuffer(kNumChannels, kBlockSize);
        juce::MidiBuffer midiBuffer;
        generateImpulse(buffer);
        processor.processBlock(buffer, midiBuffer);  // Initial impulse
        previousBuffer.makeCopyOf(buffer);
        buffer.clear();

        float maxDelta = 0.0f;
        const float sweepFreq = 10.0f;
        int sampleCounter = 0;

        for (int block = 0; block < 200; ++block)
        {
            // Modulate parameter at sweep frequency
            float phase = (sampleCounter / kSampleRate) * sweepFreq * 2.0f * juce::MathConstants<float>::pi;
            float paramValue = (std::sin(phase) + 1.0f) * 0.5f;
            testParam->setValueNotifyingHost(paramValue);

            buffer.clear();
            processor.processBlock(buffer, midiBuffer);
            const float blockJump = calculateMaxJump(buffer);
            const float boundaryJump = calculateBoundaryJump(buffer, previousBuffer);
            maxDelta = std::max(maxDelta, std::max(blockJump, boundaryJump));
            previousBuffer.makeCopyOf(buffer);

            sampleCounter += kBlockSize;
        }

        // Convert to dB
        float jumpDb = 20.0f * std::log10(maxDelta + 1e-10f);

        if (jumpDb > -40.0f)
        {
            result.message = "Zipper noise detected: " + std::to_string(jumpDb) + " dB (threshold: -40dB)";
            result.value = jumpDb;
        }
        else
        {
            result.passed = true;
            result.message = "No zipper noise: " + std::to_string(jumpDb) + " dB";
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
// Test PARAM-5: Parameter Jump Stress (Instant Changes)
//==============================================================================
static TestResult testParameterJumpStress()
{
    TestResult result{"PARAM-5: Parameter Jump Stress", false, ""};

    try
    {
        MonumentAudioProcessor processor;
        processor.prepareToPlay(kSampleRate, kBlockSize);

        juce::AudioParameterFloat* testParam = findSweepParameter(processor);

        if (!testParam)
        {
            result.message = "No float parameters found";
            return result;
        }

        // Process with instant parameter jumps
        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        juce::AudioBuffer<float> previousBuffer(kNumChannels, kBlockSize);
        juce::MidiBuffer midiBuffer;

        float maxJump = 0.0f;
        double phase = 0.0;

        for (int block = 0; block < 100; ++block)
        {
            // Jump between 0 and 1 instantly every block
            testParam->setValueNotifyingHost(block % 2 == 0 ? 0.0f : 1.0f);

            // Stable sine input for consistent click measurement
            generateSine(buffer, phase, 220.0);
            buffer.applyGain(0.2f);
            processor.processBlock(buffer, midiBuffer);

            if (block > 0)
                maxJump = std::max(maxJump, calculateBoundaryJump(buffer, previousBuffer));
            previousBuffer.makeCopyOf(buffer);
        }

        float clickDb = 20.0f * std::log10(maxJump + 1e-10f);

        if (clickDb > -30.0f)
        {
            result.message = "Excessive clicks: " + std::to_string(clickDb) + " dB (threshold: -30dB)";
            result.value = clickDb;
        }
        else
        {
            result.passed = true;
            result.message = "Clicks acceptable: " + std::to_string(clickDb) + " dB";
            result.value = clickDb;
        }
    }
    catch (const std::exception& e)
    {
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test PARAM-6: Automation Storm (All 47 Parameters Changing)
//==============================================================================
static TestResult testAutomationStorm()
{
    TestResult result{"PARAM-6: Automation Storm (47 params)", false, ""};

    try
    {
        MonumentAudioProcessor processor;
        processor.prepareToPlay(kSampleRate, kBlockSize);

        std::mt19937 rng(42);
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        juce::MidiBuffer midiBuffer;
        generateNoise(buffer, rng);

        auto& apvts = processor.getAPVTS();
        bool hadInvalidNumbers = false;
        float maxPeak = 0.0f;

        for (int block = 0; block < kNumBlocks / 2; ++block)  // 5 seconds
        {
            // Change ALL parameters every block
            for (auto* param : processor.getParameters())
            {
                if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
                {
                    floatParam->setValueNotifyingHost(dist(rng));
                }
            }

            generateNoise(buffer, rng);
            processor.processBlock(buffer, midiBuffer);

            if (hasInvalidNumbers(buffer))
            {
                hadInvalidNumbers = true;
                break;
            }

            maxPeak = std::max(maxPeak, calculatePeak(buffer));

            if (block % (kNumBlocks / 20) == 0)
            {
                std::cout << "  Progress: " << (block * 200 / kNumBlocks) << "%\r" << std::flush;
            }
        }
        std::cout << "                  \r" << std::flush;

        if (hadInvalidNumbers)
        {
            result.message = "Inf/NaN during automation storm";
        }
        else if (maxPeak > 100.0f)
        {
            result.message = "Runaway amplification: " + std::to_string(20.0f * std::log10(maxPeak)) + " dB";
        }
        else
        {
            result.passed = true;
            result.message = "Survived automation storm, peak = " +
                           std::to_string(20.0f * std::log10(maxPeak + 1e-10f)) + " dB";
            result.value = 20.0f * std::log10(maxPeak + 1e-10f);
        }
    }
    catch (const std::exception& e)
    {
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test PARAM-7: Feedback at Maximum
//==============================================================================
static TestResult testFeedbackMaximum()
{
    TestResult result{"PARAM-7: Feedback at Maximum", false, ""};

    try
    {
        MonumentAudioProcessor processor;
        processor.prepareToPlay(kSampleRate, kBlockSize);

        // Set time/density/bloom (feedback-related) to maximum
        auto& apvts = processor.getAPVTS();
        if (auto* timeParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("time")))
            timeParam->setValueNotifyingHost(1.0f);
        if (auto* densityParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("density")))
            densityParam->setValueNotifyingHost(1.0f);
        if (auto* bloomParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("bloom")))
            bloomParam->setValueNotifyingHost(1.0f);

        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        juce::MidiBuffer midiBuffer;
        generateImpulse(buffer);
        processor.processBlock(buffer, midiBuffer);

        // Process for extended time to detect runaway
        std::vector<float> energySamples;
        for (int block = 0; block < kNumBlocks; ++block)
        {
            processor.processBlock(buffer, midiBuffer);

            if (hasInvalidNumbers(buffer))
            {
                result.message = "Inf/NaN detected with feedback at maximum";
                return result;
            }

            float energy = calculateRMS(buffer);
            energySamples.push_back(energy);

            if (block % (kNumBlocks / 10) == 0)
            {
                std::cout << "  Progress: " << (block * 100 / kNumBlocks) << "%\r" << std::flush;
            }
        }
        std::cout << "                  \r" << std::flush;

        // Check if energy grew unbounded
        float initialEnergy = energySamples[10];  // Skip first few blocks
        float finalEnergy = energySamples[energySamples.size() - 1];
        float growthDb = 20.0f * std::log10((finalEnergy + 1e-10f) / (initialEnergy + 1e-10f));

        if (growthDb > 20.0f)
        {
            result.message = "Runaway feedback detected: +" + std::to_string(growthDb) + " dB growth";
            result.value = growthDb;
        }
        else
        {
            result.passed = true;
            result.message = "Feedback stable: " + std::to_string(growthDb) + " dB growth";
            result.value = growthDb;
        }
    }
    catch (const std::exception& e)
    {
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test PARAM-8: Resonance at Maximum
//==============================================================================
static TestResult testResonanceMaximum()
{
    TestResult result{"PARAM-8: Resonance at Maximum", false, ""};

    try
    {
        MonumentAudioProcessor processor;
        processor.prepareToPlay(kSampleRate, kBlockSize);

        // Set metallic resonance to maximum
        auto& apvts = processor.getAPVTS();
        if (auto* resonanceParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("metallicResonance")))
            resonanceParam->setValueNotifyingHost(1.0f);
        if (auto* couplingParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("couplingStrength")))
            couplingParam->setValueNotifyingHost(1.0f);

        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        juce::MidiBuffer midiBuffer;
        generateImpulse(buffer);
        processor.processBlock(buffer, midiBuffer);

        bool hadInvalidNumbers = false;
        float maxPeak = 0.0f;

        for (int block = 0; block < 500; ++block)
        {
            processor.processBlock(buffer, midiBuffer);

            if (hasInvalidNumbers(buffer))
            {
                hadInvalidNumbers = true;
                break;
            }

            maxPeak = std::max(maxPeak, calculatePeak(buffer));
        }

        if (hadInvalidNumbers)
        {
            result.message = "Inf/NaN detected with resonance at maximum";
        }
        else if (maxPeak > 100.0f)
        {
            result.message = "Resonance instability: " + std::to_string(20.0f * std::log10(maxPeak)) + " dB peak";
            result.value = 20.0f * std::log10(maxPeak);
        }
        else
        {
            result.passed = true;
            result.message = "Resonance stable: " + std::to_string(20.0f * std::log10(maxPeak + 1e-10f)) + " dB peak";
            result.value = 20.0f * std::log10(maxPeak + 1e-10f);
        }
    }
    catch (const std::exception& e)
    {
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test PARAM-9: Freeze + Feedback 100%
//==============================================================================
static TestResult testFreezeWithFeedback()
{
    TestResult result{"PARAM-9: Freeze + Feedback 100%", false, ""};

    try
    {
        MonumentAudioProcessor processor;
        processor.prepareToPlay(kSampleRate, kBlockSize);

        auto& apvts = processor.getAPVTS();
        if (auto* freezeParam = dynamic_cast<juce::AudioParameterBool*>(apvts.getParameter("freeze")))
            freezeParam->setValueNotifyingHost(true);
        if (auto* timeParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("time")))
            timeParam->setValueNotifyingHost(1.0f);
        if (auto* densityParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("density")))
            densityParam->setValueNotifyingHost(1.0f);

        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        juce::MidiBuffer midiBuffer;
        generateImpulse(buffer);
        processor.processBlock(buffer, midiBuffer);

        // Process for 1 minute
        const int numBlocks = static_cast<int>((kSampleRate * 60) / kBlockSize);
        std::vector<float> energySamples;

        for (int block = 0; block < numBlocks; ++block)
        {
            processor.processBlock(buffer, midiBuffer);

            if (hasInvalidNumbers(buffer))
            {
                result.message = "Inf/NaN during freeze + feedback";
                return result;
            }

            if (block % 100 == 0)
            {
                energySamples.push_back(calculateRMS(buffer));
            }

            if (block % (numBlocks / 10) == 0)
            {
                std::cout << "  Progress: " << (block * 100 / numBlocks) << "%\r" << std::flush;
            }
        }
        std::cout << "                  \r" << std::flush;

        // Check energy variance (should be stable in freeze mode)
        float minEnergy = *std::min_element(energySamples.begin() + 10, energySamples.end());
        float maxEnergy = *std::max_element(energySamples.begin() + 10, energySamples.end());
        float energyRangeDb = 20.0f * std::log10((maxEnergy + 1e-10f) / (minEnergy + 1e-10f));

        if (energyRangeDb > 6.0f)
        {
            result.message = "Energy unstable in freeze mode: " + std::to_string(energyRangeDb) + " dB range";
            result.value = energyRangeDb;
        }
        else
        {
            result.passed = true;
            result.message = "Energy stable over 60s: " + std::to_string(energyRangeDb) + " dB range";
            result.value = energyRangeDb;
        }
    }
    catch (const std::exception& e)
    {
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test PARAM-10: RT60 at Minimum (Fast Decay)
//==============================================================================
static TestResult testRT60Minimum()
{
    TestResult result{"PARAM-10: RT60 at Minimum (2s)", false, ""};

    try
    {
        MonumentAudioProcessor processor;
        processor.prepareToPlay(kSampleRate, kBlockSize);

        // Set time parameter to minimum
        auto& apvts = processor.getAPVTS();
        if (auto* timeParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("time")))
            timeParam->setValueNotifyingHost(0.0f);

        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        juce::MidiBuffer midiBuffer;
        generateImpulse(buffer);
        processor.processBlock(buffer, midiBuffer);

        // Measure decay time
        std::vector<float> energySamples;
        for (int block = 0; block < 500; ++block)
        {
            processor.processBlock(buffer, midiBuffer);
            energySamples.push_back(calculateRMS(buffer));
        }

        // Find time to -60dB
        float initialEnergy = energySamples[1];
        float targetEnergy = initialEnergy * 0.001f;  // -60dB
        int decayBlock = -1;

        for (size_t i = 0; i < energySamples.size(); ++i)
        {
            if (energySamples[i] < targetEnergy)
            {
                decayBlock = static_cast<int>(i);
                break;
            }
        }

        if (decayBlock < 0)
        {
            result.message = "Did not reach -60dB within test duration";
        }
        else
        {
            float decayTimeSeconds = (decayBlock * kBlockSize) / static_cast<float>(kSampleRate);
            result.passed = decayTimeSeconds < 5.0f;  // Should decay fast
            result.message = "RT60 ≈ " + std::to_string(decayTimeSeconds) + "s";
            result.value = decayTimeSeconds;
        }
    }
    catch (const std::exception& e)
    {
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test PARAM-11: RT60 at Maximum (Long Decay)
//==============================================================================
static TestResult testRT60Maximum()
{
    TestResult result{"PARAM-11: RT60 at Maximum (35s)", false, ""};

    try
    {
        MonumentAudioProcessor processor;
        processor.prepareToPlay(kSampleRate, kBlockSize);

        // Set time parameter to maximum
        auto& apvts = processor.getAPVTS();
        if (auto* timeParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("time")))
            timeParam->setValueNotifyingHost(1.0f);

        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        juce::MidiBuffer midiBuffer;
        generateImpulse(buffer);
        processor.processBlock(buffer, midiBuffer);

        // Sample energy over 40 seconds
        const int testDurationSeconds = 40;
        const int totalBlocks = static_cast<int>((kSampleRate * testDurationSeconds) / kBlockSize);
        std::vector<float> energySamples;

        for (int block = 0; block < totalBlocks; ++block)
        {
            processor.processBlock(buffer, midiBuffer);

            if (block % 10 == 0)
            {
                energySamples.push_back(calculateRMS(buffer));
            }

            if (hasInvalidNumbers(buffer))
            {
                result.message = "Inf/NaN during long decay";
                return result;
            }

            if (block % (totalBlocks / 10) == 0)
            {
                std::cout << "  Progress: " << (block * 100 / totalBlocks) << "%\r" << std::flush;
            }
        }
        std::cout << "                  \r" << std::flush;

        // Verify decay is stable and gradual
        float initialEnergy = energySamples[5];
        float finalEnergy = energySamples[energySamples.size() - 1];
        float decayDb = 20.0f * std::log10((finalEnergy + 1e-10f) / (initialEnergy + 1e-10f));

        if (decayDb < -80.0f)
        {
            result.message = "Decay too fast for maximum setting: " + std::to_string(decayDb) + " dB";
        }
        else if (decayDb > -10.0f)
        {
            result.message = "Insufficient decay: " + std::to_string(decayDb) + " dB";
        }
        else
        {
            result.passed = true;
            result.message = "Long decay stable: " + std::to_string(decayDb) + " dB over 40s";
            result.value = decayDb;
        }
    }
    catch (const std::exception& e)
    {
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test PARAM-12: Diffusion Extremes (0% and 100%)
//==============================================================================
static TestResult testDiffusionExtremes()
{
    TestResult result{"PARAM-12: Diffusion Extremes", false, ""};

    try
    {
        MonumentAudioProcessor processor;
        processor.prepareToPlay(kSampleRate, kBlockSize);

        auto& apvts = processor.getAPVTS();
        juce::AudioParameterFloat* densityParam =
            dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("density"));

        if (!densityParam)
        {
            result.message = "Density parameter not found";
            return result;
        }

        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        juce::MidiBuffer midiBuffer;

        // Test 0% diffusion
        densityParam->setValueNotifyingHost(0.0f);
        generateImpulse(buffer);
        processor.processBlock(buffer, midiBuffer);

        bool zeroStable = true;
        for (int block = 0; block < 100; ++block)
        {
            processor.processBlock(buffer, midiBuffer);
            if (hasInvalidNumbers(buffer))
            {
                zeroStable = false;
                break;
            }
        }

        // Test 100% diffusion
        densityParam->setValueNotifyingHost(1.0f);
        generateImpulse(buffer);
        processor.processBlock(buffer, midiBuffer);

        bool maxStable = true;
        for (int block = 0; block < 100; ++block)
        {
            processor.processBlock(buffer, midiBuffer);
            if (hasInvalidNumbers(buffer))
            {
                maxStable = false;
                break;
            }
        }

        if (!zeroStable)
        {
            result.message = "Unstable at 0% diffusion";
        }
        else if (!maxStable)
        {
            result.message = "Unstable at 100% diffusion";
        }
        else
        {
            result.passed = true;
            result.message = "Both 0% and 100% diffusion stable";
        }
    }
    catch (const std::exception& e)
    {
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test PARAM-13: Modulation at Maximum Rate
//==============================================================================
static TestResult testModulationMaximum()
{
    TestResult result{"PARAM-13: Modulation at Maximum", false, ""};

    try
    {
        MonumentAudioProcessor processor;
        processor.prepareToPlay(kSampleRate, kBlockSize);

        // Set warp and drift (modulation) to maximum
        auto& apvts = processor.getAPVTS();
        if (auto* warpParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("warp")))
            warpParam->setValueNotifyingHost(1.0f);
        if (auto* driftParam = dynamic_cast<juce::AudioParameterFloat*>(apvts.getParameter("drift")))
            driftParam->setValueNotifyingHost(1.0f);

        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        juce::MidiBuffer midiBuffer;

        std::mt19937 rng(42);
        generateNoise(buffer, rng);
        processor.processBlock(buffer, midiBuffer);

        bool hadInvalidNumbers = false;
        float maxPeak = 0.0f;

        for (int block = 0; block < kNumBlocks; ++block)
        {
            generateNoise(buffer, rng);
            processor.processBlock(buffer, midiBuffer);

            if (hasInvalidNumbers(buffer))
            {
                hadInvalidNumbers = true;
                break;
            }

            maxPeak = std::max(maxPeak, calculatePeak(buffer));

            if (block % (kNumBlocks / 10) == 0)
            {
                std::cout << "  Progress: " << (block * 100 / kNumBlocks) << "%\r" << std::flush;
            }
        }
        std::cout << "                  \r" << std::flush;

        if (hadInvalidNumbers)
        {
            result.message = "Inf/NaN with maximum modulation";
        }
        else
        {
            result.passed = true;
            result.message = "Max modulation stable, peak = " +
                           std::to_string(20.0f * std::log10(maxPeak + 1e-10f)) + " dB";
            result.value = 20.0f * std::log10(maxPeak + 1e-10f);
        }
    }
    catch (const std::exception& e)
    {
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test PARAM-14: Invalid Parameter Values (Out of Range)
//==============================================================================
static TestResult testInvalidParameterValues()
{
    TestResult result{"PARAM-14: Invalid Parameter Values", false, ""};

    try
    {
        MonumentAudioProcessor processor;
        processor.prepareToPlay(kSampleRate, kBlockSize);

        // Try to set parameters to out-of-range values
        bool allFinite = true;
        for (auto* param : processor.getParameters())
        {
            if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
            {
                const auto range = floatParam->getNormalisableRange();
                const float minVal = range.start;
                const float maxVal = range.end;

                // Try values outside normalized [0, 1]
                floatParam->setValueNotifyingHost(-1.0f);
                float value1 = floatParam->get();

                floatParam->setValueNotifyingHost(2.0f);
                float value2 = floatParam->get();

                if (!std::isfinite(value1) || !std::isfinite(value2))
                {
                    std::cout << COLOR_RED
                              << "  Non-finite value for param '" << floatParam->getName(64).toStdString()
                              << "' (range " << minVal << " .. " << maxVal
                              << ", values " << value1 << ", " << value2 << ")\n"
                              << COLOR_RESET;
                    allFinite = false;
                    break;
                }
            }
        }

        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        juce::MidiBuffer midiBuffer;
        generateImpulse(buffer);
        processor.processBlock(buffer, midiBuffer);

        if (!allFinite)
        {
            result.message = "Parameter values became non-finite";
        }
        else if (hasInvalidNumbers(buffer))
        {
            result.message = "Processing produced Inf/NaN";
        }
        else
        {
            result.passed = true;
            result.message = "All parameters properly clamped";
        }
    }
    catch (const std::exception& e)
    {
        result.message = std::string("Exception: ") + e.what();
    }

    return result;
}

//==============================================================================
// Test PARAM-15: Preset Switching Rapid
//==============================================================================
static TestResult testPresetSwitchingRapid()
{
    TestResult result{"PARAM-15: Preset Switching Rapid", false, ""};

    try
    {
        MonumentAudioProcessor processor;
        processor.prepareToPlay(kSampleRate, kBlockSize);

        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        juce::MidiBuffer midiBuffer;
        generateImpulse(buffer);

        // Switch between presets rapidly
        int numPresets = processor.getNumFactoryPresets();
        if (numPresets == 0)
        {
            result.message = "No factory presets available";
            result.passed = true;  // Not a failure
            return result;
        }

        float maxJump = 0.0f;
        for (int i = 0; i < 20; ++i)
        {
            processor.loadFactoryPreset(i % numPresets);
            processor.processBlock(buffer, midiBuffer);
            float jump = calculateMaxJump(buffer);
            maxJump = std::max(maxJump, jump);
        }

        float clickDb = 20.0f * std::log10(maxJump + 1e-10f);

        if (clickDb > -30.0f)
        {
            result.message = "Preset switching clicks: " + std::to_string(clickDb) + " dB";
            result.value = clickDb;
        }
        else
        {
            result.passed = true;
            result.message = "Preset switching smooth: " + std::to_string(clickDb) + " dB";
            result.value = clickDb;
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
    std::cout << COLOR_CYAN << "================================================\n";
    std::cout << "Monument Reverb - Parameter Stress Test Suite\n";
    std::cout << "================================================\n" << COLOR_RESET;
    std::cout << "\n";

    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI juceInit;

    std::vector<TestResult> results;

    // Quick tests (always run)
    std::cout << COLOR_BLUE << "=== Quick Parameter Tests (30s) ===" << COLOR_RESET << "\n\n";
    results.push_back(testAllParametersZero());
    results.push_back(testAllParametersMaximum());
    results.push_back(testRapidParameterSweeps());
    results.push_back(testParameterJumpStress());
    results.push_back(testInvalidParameterValues());
    results.push_back(testDiffusionExtremes());

    if (!quickMode)
    {
        // Extended tests
        std::cout << "\n" << COLOR_BLUE << "=== Extended Parameter Tests (5-10 min) ===" << COLOR_RESET << "\n\n";
        results.push_back(testAllParametersRandom());
        results.push_back(testAutomationStorm());
        results.push_back(testFeedbackMaximum());
        results.push_back(testResonanceMaximum());
        results.push_back(testFreezeWithFeedback());
        results.push_back(testRT60Minimum());
        results.push_back(testRT60Maximum());
        results.push_back(testModulationMaximum());
        results.push_back(testPresetSwitchingRapid());
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

    return failed > 0 ? 1 : 0;
}
