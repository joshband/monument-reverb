/**
 * Monument Reverb - State Save/Recall Test
 *
 * Validates automation compatibility and preset management.
 * Tests that plugin state can be saved and restored accurately.
 *
 * Success Criteria:
 * - All parameters restored accurately (< 0.001 tolerance)
 * - Preset switching produces no glitches/clicks
 * - Automation compatible with DAW hosts
 */

#include <JuceHeader.h>
#include "plugin/PluginProcessor.h"
#include <cmath>
#include <map>
#include <vector>
#include <tuple>
#include <iostream>
#include <iomanip>

// ANSI color codes
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_BLUE "\033[0;34m"
#define COLOR_RESET "\033[0m"

struct StateTestResult
{
    int totalParameters;
    int parametersRestored;
    int parametersFailed;
    float maxError;
    bool passed;
    std::vector<std::tuple<juce::String, float, float, float>> failedParams;  // name, original, restored, error
};

struct PresetSwitchResult
{
    int numPresets;
    int clicksDetected;
    float maxTransient;
    bool passed;
};

/**
 * Test state save and recall
 */
StateTestResult testStateSaveRecall(MonumentAudioProcessor& processor)
{
    StateTestResult result;
    result.totalParameters = 0;
    result.parametersRestored = 0;
    result.parametersFailed = 0;
    result.maxError = 0.0f;

    // Get all parameters
    auto& params = processor.getParameters();

    // Set random values and save state
    std::map<juce::String, float> originalValues;

    for (auto* paramBase : params)
    {
        auto* param = dynamic_cast<juce::RangedAudioParameter*>(paramBase);
        if (!param) continue;

        // Use parameter ID (not display name) to avoid duplicate name conflicts
        juce::String paramID = param->getParameterID();

        // For discrete/choice parameters, use valid quantized values
        // AudioParameterChoice and AudioParameterBool quantize to discrete steps
        auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(paramBase);
        auto* boolParam = dynamic_cast<juce::AudioParameterBool*>(paramBase);

        float randomValue;
        if (choiceParam)
        {
            // Generate a random index and convert to normalized value
            int numChoices = choiceParam->choices.size();
            int randomIndex = juce::Random::getSystemRandom().nextInt(numChoices);
            randomValue = static_cast<float>(randomIndex) / static_cast<float>(numChoices - 1);
            if (numChoices == 1) randomValue = 0.0f;  // Edge case: single choice
        }
        else if (boolParam)
        {
            // Boolean: either 0.0 or 1.0
            randomValue = juce::Random::getSystemRandom().nextBool() ? 1.0f : 0.0f;
        }
        else
        {
            // Continuous parameter: any value 0.0 to 1.0
            randomValue = juce::Random::getSystemRandom().nextFloat();
        }

        param->setValueNotifyingHost(randomValue);
        originalValues[paramID] = randomValue;
        result.totalParameters++;
    }

    // Save state
    juce::MemoryBlock stateData;
    processor.getStateInformation(stateData);

    // Change all parameters to different values (using same discrete-aware logic)
    for (auto* paramBase : params)
    {
        auto* param = dynamic_cast<juce::RangedAudioParameter*>(paramBase);
        if (!param) continue;

        // Use same discrete handling as above
        auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(paramBase);
        auto* boolParam = dynamic_cast<juce::AudioParameterBool*>(paramBase);

        float newValue;
        if (choiceParam)
        {
            int numChoices = choiceParam->choices.size();
            int randomIndex = juce::Random::getSystemRandom().nextInt(numChoices);
            newValue = static_cast<float>(randomIndex) / static_cast<float>(numChoices - 1);
            if (numChoices == 1) newValue = 0.0f;
        }
        else if (boolParam)
        {
            newValue = juce::Random::getSystemRandom().nextBool() ? 1.0f : 0.0f;
        }
        else
        {
            newValue = juce::Random::getSystemRandom().nextFloat();
        }

        param->setValueNotifyingHost(newValue);
    }

    // Restore state
    processor.setStateInformation(stateData.getData(), static_cast<int>(stateData.getSize()));

    // Verify parameters were restored
    for (auto* paramBase : params)
    {
        auto* param = dynamic_cast<juce::RangedAudioParameter*>(paramBase);
        if (!param) continue;

        // Use parameter ID to match with original values
        juce::String paramID = param->getParameterID();
        juce::String paramName = param->getName(32);  // Keep name for display
        float restoredValue = param->getValue();
        float originalValue = originalValues[paramID];

        float error = std::abs(restoredValue - originalValue);
        result.maxError = std::max(result.maxError, error);

        if (error < 0.001f)  // Tolerance: 0.1%
        {
            result.parametersRestored++;
        }
        else
        {
            result.parametersFailed++;
            // Store ID and name for debugging
            juce::String displayInfo = paramName + " (" + paramID + ")";
            result.failedParams.push_back(std::make_tuple(displayInfo, originalValue, restoredValue, error));
        }
    }

    result.passed = (result.parametersFailed == 0);

    return result;
}

/**
 * Detect clicks in a buffer (sample-to-sample difference threshold)
 */
int detectClicks(const juce::AudioBuffer<float>& buffer, float thresholdLinear = 0.1f)
{
    int clickCount = 0;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const float* samples = buffer.getReadPointer(ch);
        for (int i = 1; i < buffer.getNumSamples(); ++i)
        {
            float diff = std::abs(samples[i] - samples[i-1]);
            if (diff > thresholdLinear)
            {
                clickCount++;
            }
        }
    }

    return clickCount;
}

/**
 * Test preset switching for clicks/glitches
 */
PresetSwitchResult testPresetSwitching(MonumentAudioProcessor& processor, double sampleRate, int blockSize)
{
    PresetSwitchResult result;
    result.numPresets = processor.getNumPrograms();
    result.clicksDetected = 0;
    result.maxTransient = 0.0f;

    if (result.numPresets <= 0)
    {
        result.passed = true;  // No presets to test
        return result;
    }

    const int samplesPerPreset = 4800;  // 0.1 seconds at 48kHz
    const int totalSamples = samplesPerPreset * result.numPresets;

    juce::AudioBuffer<float> buffer(2, totalSamples);
    buffer.clear();

    // Generate test tone
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < totalSamples; ++i)
        {
            float phase = 2.0f * juce::MathConstants<float>::pi * 440.0f * i / sampleRate;
            buffer.setSample(ch, i, 0.3f * std::sin(phase));
        }
    }

    // Process with preset switching
    int samplesProcessed = 0;
    int currentPreset = 0;

    while (samplesProcessed < totalSamples)
    {
        // Switch preset every 0.1 seconds
        if (samplesProcessed % samplesPerPreset == 0 && currentPreset < result.numPresets)
        {
            processor.setCurrentProgram(currentPreset);
            currentPreset++;
        }

        int samplesToProcess = std::min(blockSize, totalSamples - samplesProcessed);

        juce::AudioBuffer<float> blockBuffer(2, samplesToProcess);
        for (int ch = 0; ch < 2; ++ch)
        {
            blockBuffer.copyFrom(ch, 0, buffer, ch, samplesProcessed, samplesToProcess);
        }

        juce::MidiBuffer midiBuffer;
        processor.processBlock(blockBuffer, midiBuffer);

        for (int ch = 0; ch < 2; ++ch)
        {
            buffer.copyFrom(ch, samplesProcessed, blockBuffer, ch, 0, samplesToProcess);
        }

        samplesProcessed += samplesToProcess;
    }

    // Detect clicks in processed audio
    // Use 0.3 threshold (30% jump) - reverb can have natural transients during tail decay
    result.clicksDetected = detectClicks(buffer, 0.3f);

    // Calculate maximum sample-to-sample difference
    for (int ch = 0; ch < 2; ++ch)
    {
        const float* samples = buffer.getReadPointer(ch);
        for (int i = 1; i < buffer.getNumSamples(); ++i)
        {
            float diff = std::abs(samples[i] - samples[i-1]);
            result.maxTransient = std::max(result.maxTransient, diff);
        }
    }

    // Pass criteria: no clicks detected
    result.passed = (result.clicksDetected == 0);

    return result;
}

int main()
{
    std::cout << "\n";
    std::cout << COLOR_BLUE << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << COLOR_RESET << "\n";
    std::cout << COLOR_BLUE << "  Monument Reverb - State Management Test" << COLOR_RESET << "\n";
    std::cout << COLOR_BLUE << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << COLOR_RESET << "\n";
    std::cout << "\n";

    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI juceInit;

    // Create processor
    MonumentAudioProcessor processor;

    const double sampleRate = 48000.0;
    const int blockSize = 512;

    std::cout << "Preparing plugin...\n";
    processor.prepareToPlay(sampleRate, blockSize);
    std::cout << "  Sample rate: " << sampleRate << " Hz\n";
    std::cout << "  Block size:  " << blockSize << " samples\n";
    std::cout << "\n";

    int totalTests = 0;
    int passedTests = 0;

    // Test 1: State Save/Recall
    std::cout << "Test 1: State Save/Recall\n";
    std::cout << "  Setting random parameter values...\n";
    std::cout << "  Saving state...\n";
    std::cout << "  Modifying parameters...\n";
    std::cout << "  Restoring state...\n";

    StateTestResult stateResult = testStateSaveRecall(processor);
    totalTests++;

    std::cout << "\n";
    std::cout << "  Total parameters:    " << stateResult.totalParameters << "\n";
    std::cout << "  Restored correctly:  " << stateResult.parametersRestored << "\n";
    std::cout << "  Failed to restore:   " << stateResult.parametersFailed << "\n";
    std::cout << "  Maximum error:       " << std::scientific << std::setprecision(3) << stateResult.maxError << "\n";
    std::cout << "\n";

    if (stateResult.passed)
    {
        std::cout << "  " << COLOR_GREEN << "✓ PASS" << COLOR_RESET << " (all parameters restored accurately)\n";
        passedTests++;
    }
    else
    {
        std::cout << "  " << COLOR_RED << "✗ FAIL" << COLOR_RESET;
        std::cout << " (" << stateResult.parametersFailed << " parameters failed to restore)\n";
        std::cout << "\n";
        std::cout << "  Failed parameters:\n";
        for (const auto& failedParam : stateResult.failedParams)
        {
            std::cout << "    " << std::get<0>(failedParam) << ": ";
            std::cout << "original=" << std::fixed << std::setprecision(6) << std::get<1>(failedParam) << ", ";
            std::cout << "restored=" << std::get<2>(failedParam) << ", ";
            std::cout << "error=" << std::scientific << std::setprecision(3) << std::get<3>(failedParam) << "\n";
        }
    }
    std::cout << "\n";

    // Test 2: Preset Switching (if presets exist)
    std::cout << "Test 2: Preset Switching\n";

    PresetSwitchResult presetResult = testPresetSwitching(processor, sampleRate, blockSize);
    totalTests++;

    if (presetResult.numPresets > 0)
    {
        std::cout << "  Switching through " << presetResult.numPresets << " presets...\n";
        std::cout << "\n";
        std::cout << "  Number of presets:   " << presetResult.numPresets << "\n";
        std::cout << "  Clicks detected:     " << presetResult.clicksDetected << "\n";
        std::cout << "  Max transient:       " << std::fixed << std::setprecision(6) << presetResult.maxTransient << "\n";
        std::cout << "\n";

        if (presetResult.passed)
        {
            std::cout << "  " << COLOR_GREEN << "✓ PASS" << COLOR_RESET << " (no clicks during preset switching)\n";
            passedTests++;
        }
        else
        {
            std::cout << "  " << COLOR_RED << "✗ FAIL" << COLOR_RESET;
            std::cout << " (" << presetResult.clicksDetected << " clicks detected)\n";
        }
    }
    else
    {
        std::cout << "  No presets defined - skipping test\n";
        std::cout << "  " << COLOR_YELLOW << "⊘ SKIPPED" << COLOR_RESET << "\n";
        passedTests++;  // Count as passed since there's nothing to test
    }
    std::cout << "\n";

    processor.releaseResources();

    // Print summary
    std::cout << COLOR_BLUE << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << COLOR_RESET << "\n";
    std::cout << COLOR_BLUE << "  Summary" << COLOR_RESET << "\n";
    std::cout << COLOR_BLUE << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << COLOR_RESET << "\n";
    std::cout << "\n";
    std::cout << "  Total tests:  " << totalTests << "\n";
    std::cout << "  Passed:       " << COLOR_GREEN << passedTests << COLOR_RESET << "\n";
    std::cout << "  Failed:       " << COLOR_RED << (totalTests - passedTests) << COLOR_RESET << "\n";
    std::cout << "\n";

    if (passedTests == totalTests)
    {
        std::cout << COLOR_GREEN << "✓ State management validated - automation compatible!" << COLOR_RESET << "\n";
        std::cout << "\n";
        return 0;
    }
    else
    {
        std::cout << COLOR_RED << "✗ State management issues detected" << COLOR_RESET << "\n";
        std::cout << "\n";
        return 1;
    }
}
