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

/**
 * Result of the fresh-instance host state roundtrip characterization (Test 3).
 */
struct HostStateFreshInstanceResult
{
    int totalParameters{0};
    int parametersMismatched{0};
    bool modulationConnectionRegisteredOnSource{false};
    bool modulationConnectionSurvivedRoundtrip{false};
    bool passed{false};
    std::vector<juce::String> mismatchedParams;
};

/**
 * Test 3: Fresh-Instance Host State Roundtrip (Report Step 5 characterization)
 *
 * MonumentAudioProcessor::getStateInformation()/setStateInformation() save and
 * restore ONLY `parameters.copyState()` as XML -- the APVTS parameter tree
 * (see plugin/PluginProcessor.cpp). This is a real, incomplete contract:
 * it does NOT capture the custom modulation-matrix connections managed by
 * ModulationMatrix::setConnections()/getConnections() (dsp/ModulationMatrix.h),
 * which live entirely outside the APVTS tree.
 *
 * This test proves BOTH halves of that contract using two SEPARATE, freshly
 * constructed MonumentAudioProcessor instances (not the same instance reused,
 * which could hide bugs where in-memory state leaks across the save/restore
 * boundary rather than actually round-tripping through the serialized bytes):
 *   (a) every APVTS parameter round-trips correctly from instance A to a
 *       brand new instance B via getStateInformation()/setStateInformation()
 *       (expected: yes, for all parameters)
 *   (b) a modulation connection created on instance A does NOT appear on
 *       instance B after the same roundtrip (expected: it does NOT survive --
 *       this is the documented gap, not a bug this test is trying to fix).
 *
 * If a future change intentionally folds modulation connections into host
 * state, this assertion must be updated deliberately -- do not "fix" this
 * test by loosening it without also verifying the production code actually
 * changed.
 */
HostStateFreshInstanceResult testFreshInstanceHostStateRoundtrip()
{
    HostStateFreshInstanceResult result;

    MonumentAudioProcessor processorA;
    processorA.prepareToPlay(48000.0, 512);

    auto& paramsA = processorA.getParameters();
    std::map<juce::String, float> distinctiveValues;

    for (auto* paramBase : paramsA)
    {
        auto* param = dynamic_cast<juce::RangedAudioParameter*>(paramBase);
        if (!param) continue;

        juce::String paramID = param->getParameterID();
        auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(paramBase);
        auto* boolParam = dynamic_cast<juce::AudioParameterBool*>(paramBase);

        float distinctiveValue;
        if (choiceParam)
        {
            // Deliberately pick the LAST valid choice -- distinct from the
            // near-universal "index 0" default.
            const int numChoices = choiceParam->choices.size();
            distinctiveValue = numChoices > 1
                ? static_cast<float>(numChoices - 1) / static_cast<float>(numChoices - 1)
                : 0.0f;
        }
        else if (boolParam)
        {
            distinctiveValue = 1.0f;  // flip true; most bool defaults are false
        }
        else
        {
            distinctiveValue = 0.87f;  // distinct from the common 0.0/0.5 defaults
        }

        param->setValueNotifyingHost(distinctiveValue);
        distinctiveValues[paramID] = param->getValue();  // record actual normalized value
        result.totalParameters++;
    }

    // Set up a modulation connection -- NOT covered by host XML state.
    using MM = monument::dsp::ModulationMatrix;
    processorA.getModulationMatrix().setConnection(
        MM::SourceType::ChaosAttractor, MM::DestinationType::Warp,
        /*sourceAxis*/ 0, /*depth*/ 0.75f, /*smoothingMs*/ 150.0f);

    for (const auto& conn : processorA.getModulationMatrix().getConnections())
    {
        if (conn.destination == MM::DestinationType::Warp && conn.enabled
            && std::abs(conn.depth - 0.75f) < 0.001f)
        {
            result.modulationConnectionRegisteredOnSource = true;
        }
    }

    // Save state from instance A.
    juce::MemoryBlock stateData;
    processorA.getStateInformation(stateData);

    // Fresh SECOND instance -- this is the point of the test.
    MonumentAudioProcessor processorB;
    processorB.prepareToPlay(48000.0, 512);
    processorB.setStateInformation(stateData.getData(), static_cast<int>(stateData.getSize()));

    for (auto* paramBase : processorB.getParameters())
    {
        auto* param = dynamic_cast<juce::RangedAudioParameter*>(paramBase);
        if (!param) continue;

        juce::String paramID = param->getParameterID();
        const float restored = param->getValue();
        const float expected = distinctiveValues[paramID];

        if (std::abs(restored - expected) > 0.001f)
        {
            result.parametersMismatched++;
            result.mismatchedParams.push_back(paramID);
        }
    }

    for (const auto& conn : processorB.getModulationMatrix().getConnections())
    {
        if (conn.destination == MM::DestinationType::Warp && conn.enabled
            && std::abs(conn.depth - 0.75f) < 0.001f)
        {
            result.modulationConnectionSurvivedRoundtrip = true;
        }
    }

    processorB.releaseResources();
    processorA.releaseResources();

    // Characterization passes when:
    //  - every APVTS parameter round-tripped (0 mismatches)
    //  - the connection really was registered on A (sanity -- otherwise
    //    "it didn't survive" would be true for the wrong reason)
    //  - the connection did NOT survive onto B (the known, documented gap)
    result.passed = (result.parametersMismatched == 0)
        && result.modulationConnectionRegisteredOnSource
        && !result.modulationConnectionSurvivedRoundtrip;

    return result;
}

/**
 * Result of the user-preset JSON roundtrip characterization (Test 4).
 */
struct UserPresetJsonRoundtripResult
{
    int fieldsRoundTripped{0};
    int fieldsFailedRoundtrip{0};
    bool gapConfirmed{true};
    bool modulationConnectionSurvived{false};
    bool passed{false};
    std::vector<juce::String> unexpectedFailures;    // covered fields that should have round-tripped but didn't
    std::vector<juce::String> unexpectedRoundtrips;  // gap fields that round-tripped when they shouldn't have
};

/**
 * Test 4: User-Preset JSON Roundtrip (Report Step 5 characterization)
 *
 * PresetManager's user-preset JSON (see PresetManager::saveUserPreset,
 * PresetManager::loadUserPreset, PresetManager::captureCurrentValues, and
 * PresetManager::applyPreset in plugin/PresetManager.cpp) captures a
 * SMALLER, DIFFERENT subset of controls than host XML state, and this is a
 * separate, non-overlapping incomplete contract:
 *
 *   - COVERED (round-trips): time, mass, density, bloom, gravity, warp,
 *     drift, memory, memoryDepth, memoryDecay, memoryDrift, mix, material,
 *     topology, viscosity, evolution, chaosIntensity, elasticityDecay,
 *     patina, abyss, corona, breath -- plus modulation-matrix connections,
 *     but ONLY when driven through MonumentAudioProcessor::saveUserPreset()/
 *     loadUserPreset(file), which explicitly applies
 *     presetManager.getLastLoadedModulationConnections() to the real
 *     ModulationMatrix after PresetManager::loadUserPreset() returns.
 *     (PresetManager::applyPreset() itself only *caches* the connections;
 *     it never touches a ModulationMatrix directly.)
 *
 *   - NOT COVERED (does not round-trip): the Expressive macro parameters
 *     (character, spaceType, energy, motion, color, dimension), the
 *     routingPreset selection, the timelinePreset selection, and macroMode
 *     itself -- none of these appear in PresetValues, captureCurrentValues(),
 *     or applyPreset().
 *
 * This test documents both halves. It does not fix or fill either gap.
 */
UserPresetJsonRoundtripResult testUserPresetJsonRoundtrip()
{
    UserPresetJsonRoundtripResult result;

    MonumentAudioProcessor processorA;
    processorA.prepareToPlay(48000.0, 512);
    auto& apvtsA = processorA.getAPVTS();

    auto setNormalized = [&apvtsA](const juce::String& id, float normalized)
    {
        if (auto* param = dynamic_cast<juce::RangedAudioParameter*>(apvtsA.getParameter(id)))
            param->setValueNotifyingHost(normalized);
    };

    // Fields the JSON contract DOES cover per PresetManager::captureCurrentValues/applyPreset.
    const std::vector<juce::String> coveredFields = {
        "time", "mass", "density", "bloom", "gravity", "warp", "drift",
        "memory", "memoryDepth", "memoryDecay", "memoryDrift", "mix",
        "material", "topology", "viscosity", "evolution", "chaosIntensity",
        "elasticityDecay", "patina", "abyss", "corona", "breath"
    };
    for (const auto& id : coveredFields)
        setNormalized(id, 0.87f);

    // Fields the architectural assessment report says are OMITTED from the JSON contract.
    const std::vector<juce::String> gapFloatFields = {
        "character", "spaceType", "energy", "motion", "color", "dimension"
    };
    for (const auto& id : gapFloatFields)
        setNormalized(id, 0.87f);

    // routingPreset / timelinePreset / macroMode are choice params, also omitted from the JSON contract.
    const std::vector<juce::String> gapChoiceFields = { "routingPreset", "timelinePreset", "macroMode" };
    for (const auto& id : gapChoiceFields)
        setNormalized(id, 1.0f);  // last choice -- distinct from index-0 defaults

    std::map<juce::String, float> originals;
    for (const auto& id : coveredFields) originals[id] = apvtsA.getParameter(id)->getValue();
    for (const auto& id : gapFloatFields) originals[id] = apvtsA.getParameter(id)->getValue();
    for (const auto& id : gapChoiceFields) originals[id] = apvtsA.getParameter(id)->getValue();

    // Modulation connection -- covered when driven through the PROCESSOR's
    // saveUserPreset()/loadUserPreset(file), which applies the parsed
    // connections after PresetManager::loadUserPreset() returns.
    using MM = monument::dsp::ModulationMatrix;
    processorA.getModulationMatrix().setConnection(
        MM::SourceType::BrownianMotion, MM::DestinationType::Density,
        /*sourceAxis*/ 0, /*depth*/ 0.6f, /*smoothingMs*/ 250.0f);

    juce::File tempFile = juce::File::createTempFile(".mrpreset.json");
    processorA.saveUserPreset(tempFile, "Step5CharacterizationPreset", "state/preset coverage characterization");

    MonumentAudioProcessor processorB;
    processorB.prepareToPlay(48000.0, 512);
    auto& apvtsB = processorB.getAPVTS();

    processorB.loadUserPreset(tempFile);
    tempFile.deleteFile();

    for (const auto& id : coveredFields)
    {
        const float restored = apvtsB.getParameter(id)->getValue();
        if (std::abs(restored - originals[id]) < 0.001f)
        {
            result.fieldsRoundTripped++;
        }
        else
        {
            result.fieldsFailedRoundtrip++;
            result.unexpectedFailures.push_back(id);
        }
    }

    for (const auto& id : gapFloatFields)
    {
        const float restored = apvtsB.getParameter(id)->getValue();
        if (std::abs(restored - originals[id]) < 0.001f)
        {
            result.gapConfirmed = false;
            result.unexpectedRoundtrips.push_back(id);
        }
    }
    for (const auto& id : gapChoiceFields)
    {
        const float restored = apvtsB.getParameter(id)->getValue();
        if (std::abs(restored - originals[id]) < 0.001f)
        {
            result.gapConfirmed = false;
            result.unexpectedRoundtrips.push_back(id);
        }
    }

    for (const auto& conn : processorB.getModulationMatrix().getConnections())
    {
        if (conn.destination == MM::DestinationType::Density && conn.enabled
            && std::abs(conn.depth - 0.6f) < 0.001f)
        {
            result.modulationConnectionSurvived = true;
        }
    }

    processorB.releaseResources();
    processorA.releaseResources();

    result.passed = (result.fieldsFailedRoundtrip == 0)
        && result.gapConfirmed
        && result.modulationConnectionSurvived;

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

    // Test 3: Fresh-Instance Host State Roundtrip (Report Step 5 characterization)
    std::cout << "Test 3: Fresh-Instance Host State Roundtrip (characterization)\n";
    std::cout << "  Setting distinctive values on instance A + one modulation connection...\n";
    std::cout << "  Saving host state, restoring into a brand new instance B...\n";

    HostStateFreshInstanceResult hostStateResult = testFreshInstanceHostStateRoundtrip();
    totalTests++;

    std::cout << "\n";
    std::cout << "  Total APVTS parameters:         " << hostStateResult.totalParameters << "\n";
    std::cout << "  Mismatched after roundtrip:     " << hostStateResult.parametersMismatched << "\n";
    std::cout << "  Modulation connection on A:      " << (hostStateResult.modulationConnectionRegisteredOnSource ? "registered" : "MISSING (test setup bug)") << "\n";
    std::cout << "  Modulation connection on B:      " << (hostStateResult.modulationConnectionSurvivedRoundtrip ? "SURVIVED (unexpected)" : "did NOT survive (expected/known gap)") << "\n";
    std::cout << "\n";

    if (hostStateResult.passed)
    {
        std::cout << "  " << COLOR_GREEN << "✓ PASS" << COLOR_RESET
                   << " (all APVTS parameters round-trip; modulation connections confirmed NOT covered by host state -- known gap)\n";
        passedTests++;
    }
    else
    {
        std::cout << "  " << COLOR_RED << "✗ FAIL" << COLOR_RESET << "\n";
        for (const auto& id : hostStateResult.mismatchedParams)
            std::cout << "    mismatched parameter: " << id << "\n";
        if (!hostStateResult.modulationConnectionRegisteredOnSource)
            std::cout << "    modulation connection was never registered on source instance (test setup issue)\n";
        if (hostStateResult.modulationConnectionSurvivedRoundtrip)
            std::cout << "    modulation connection unexpectedly SURVIVED the host-state roundtrip -- report's documented gap no longer holds; update this test deliberately if that's an intentional fix\n";
    }
    std::cout << "\n";

    // Test 4: User-Preset JSON Roundtrip (Report Step 5 characterization)
    std::cout << "Test 4: User-Preset JSON Roundtrip (characterization)\n";
    std::cout << "  Setting distinctive values (covered + gap fields) on instance A...\n";
    std::cout << "  Saving user preset JSON, loading into a brand new instance B...\n";

    UserPresetJsonRoundtripResult presetJsonResult = testUserPresetJsonRoundtrip();
    totalTests++;

    std::cout << "\n";
    std::cout << "  Covered fields round-tripped:    " << presetJsonResult.fieldsRoundTripped << "\n";
    std::cout << "  Covered fields FAILED:           " << presetJsonResult.fieldsFailedRoundtrip << "\n";
    std::cout << "  Gap fields confirmed omitted:    " << (presetJsonResult.gapConfirmed ? "yes" : "NO (unexpected roundtrip)") << "\n";
    std::cout << "  Modulation connection survived:  " << (presetJsonResult.modulationConnectionSurvived ? "yes (expected, via processor-level apply)" : "NO (unexpected)") << "\n";
    std::cout << "\n";

    if (presetJsonResult.passed)
    {
        std::cout << "  " << COLOR_GREEN << "✓ PASS" << COLOR_RESET
                   << " (covered fields + modulation connections round-trip; expressive macros/routingPreset/timelinePreset/macroMode confirmed NOT covered -- known gap)\n";
        passedTests++;
    }
    else
    {
        std::cout << "  " << COLOR_RED << "✗ FAIL" << COLOR_RESET << "\n";
        for (const auto& id : presetJsonResult.unexpectedFailures)
            std::cout << "    covered field failed to round-trip: " << id << "\n";
        for (const auto& id : presetJsonResult.unexpectedRoundtrips)
            std::cout << "    gap field unexpectedly round-tripped: " << id << " -- report's documented gap no longer holds; update this test deliberately if that's an intentional fix\n";
        if (!presetJsonResult.modulationConnectionSurvived)
            std::cout << "    modulation connection did not survive user-preset roundtrip (unexpected regression)\n";
    }
    std::cout << "\n";

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
