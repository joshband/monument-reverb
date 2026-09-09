/**
 * Monument Reverb - Parameter Smoothing Test
 *
 * Tests that parameter changes do not produce audible clicks or pops.
 * All parameters must be smoothed to prevent discontinuities in the audio signal.
 *
 * Success Criteria (both must hold for every parameter):
 * - No abrupt jump in short-window high-frequency energy during the sweep
 *   (see calculateMaxTransientJump and kMaxTransientJumpDb)
 * - No sample-to-sample discontinuity exceeding 0.1
 *
 * History: this test previously scored the *absolute* broadband HF RMS of the
 * whole sweep and required it to sit below a fixed level (originally -60 dB,
 * later relaxed to -15 dB because Monument's own dense reverb tail was enough
 * to trip it). That measured steady-state timbre, not transients, so it could
 * not distinguish a click from a stage legitimately generating harmonic
 * content — the `safetyClipDrive` sweep failed under it purely because driving
 * a tanh saturator harder produces more harmonics by design, not because of an
 * unsmoothed parameter. The metric now measures the largest window-to-window
 * *rise* in HF energy instead, which is what a click or an unsmoothed
 * parameter step actually looks like; steady or gradually-rising harmonic
 * content no longer registers.
 *
 * kMaxTransientJumpDb and the analysis window size were calibrated
 * empirically together, not guessed independently — the two interact. A first
 * pass used a 5 ms window (long enough for a stable RMS estimate at a glance)
 * and found a clean population maxing at ~0.7 dB vs. ~2.1 dB for the
 * reintroduced pre-fix bug. But `safetyClip`'s enable crossfade is only 8 ms,
 * and a 5 ms window can't resolve a legitimate, smooth 8 ms ramp from an
 * instant step — most of the ramp's energy rise lands in one window either
 * way, so the *correctly smoothed* signal itself measured 2.1 dB and the test
 * would have failed on the fix, not the bug. Narrowing to a 1 ms window
 * resolved the two cases cleanly: clean population maxes at 1.3 dB, the
 * reintroduced bug measures 2.4 dB. 1.8 dB sits between them with margin on
 * both sides. If this flakes in CI, the noise floor of the clean population
 * (not the regression value) is what should move the threshold — never raise
 * it past the regression value, and if a legitimately-smoothed short crossfade
 * ever needs a shorter ramp than 8 ms, re-run this calibration rather than
 * just widening the threshold.
 */

#include <JuceHeader.h>
#include "plugin/PluginProcessor.h"
#include <cmath>
#include <vector>
#include <iostream>
#include <iomanip>

// ANSI color codes for terminal output
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_BLUE "\033[0;34m"
#define COLOR_RESET "\033[0m"

// See the calibration note above: 1.3 dB (clean) vs 2.4 dB (real regression).
constexpr float kMaxTransientJumpDb = 1.8f;

struct ClickDetectionResult
{
    std::string parameterName;
    bool passed;
    float maxTransientJump; // Largest abrupt window-to-window HF energy rise (dB)
    float maxSampleDiff;    // Maximum sample-to-sample difference
    int clickCount;         // Number of detected clicks
};

/**
 * Detect clicks using sample-to-sample difference threshold
 * A click is defined as a sudden jump in sample value > threshold
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
 * Calculate maximum sample-to-sample difference (indicator of discontinuities)
 */
float calculateMaxSampleDiff(const juce::AudioBuffer<float>& buffer)
{
    float maxDiff = 0.0f;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const float* samples = buffer.getReadPointer(ch);
        for (int i = 1; i < buffer.getNumSamples(); ++i)
        {
            float diff = std::abs(samples[i] - samples[i-1]);
            maxDiff = std::max(maxDiff, diff);
        }
    }

    return maxDiff;
}

/**
 * Measure the largest abrupt jump in high-frequency energy across the sweep.
 *
 * This deliberately measures the *change* in short-window HF energy, not the
 * absolute HF level. A click or a parameter step is a sudden discontinuity, so
 * its energy appears in one window and not its neighbour; steady-state
 * harmonic content — a saturation stage being driven harder, or Monument's own
 * dense reverb tail — raises the HF floor smoothly across many windows and is
 * not an artifact. See the file header for why this replaced a broadband
 * absolute-level check.
 *
 * Returns the maximum window-to-window increase in dB (0.0 = perfectly steady).
 */
float calculateMaxTransientJump(const juce::AudioBuffer<float>& buffer, double sampleRate)
{
    // ~1 ms analysis windows. Must be short enough to resolve the shortest
    // legitimate smoothing ramp in the plugin (safetyClipEnableSmoother's 8 ms
    // crossfade) from an instant step — see the calibration note above; a 5 ms
    // window was tried first and was too coarse to tell the two apart.
    const int windowSamples = juce::jmax(32, static_cast<int>(sampleRate * 0.001));
    const int numSamples = buffer.getNumSamples();
    if (numSamples < windowSamples * 2)
        return 0.0f;

    // Same one-pole high-pass as before (~10 kHz) to isolate fast content.
    const float alpha = 0.9f;
    float prevSample = 0.0f;
    float prevFiltered = 0.0f;

    const float* samples = buffer.getReadPointer(0);

    std::vector<float> windowDb;
    windowDb.reserve(static_cast<size_t>(numSamples / windowSamples) + 1);

    float sumSquared = 0.0f;
    int windowCount = 0;

    for (int i = 0; i < numSamples; ++i)
    {
        const float filtered = alpha * (prevFiltered + samples[i] - prevSample);
        prevSample = samples[i];
        prevFiltered = filtered;

        sumSquared += filtered * filtered;
        if (++windowCount == windowSamples)
        {
            const float rms = std::sqrt(sumSquared / static_cast<float>(windowCount));
            windowDb.push_back(20.0f * std::log10(std::max(rms, 1e-10f)));
            sumSquared = 0.0f;
            windowCount = 0;
        }
    }

    // Largest single-window rise. A click shows up as one window jumping well
    // above its predecessor; a gradual distortion or decay ramp does not.
    float maxJumpDb = 0.0f;
    for (size_t i = 1; i < windowDb.size(); ++i)
        maxJumpDb = std::max(maxJumpDb, windowDb[i] - windowDb[i - 1]);

    return maxJumpDb;
}

/**
 * Test a single parameter sweep for clicks/pops
 */
ClickDetectionResult testParameterSweep(
    MonumentAudioProcessor& processor,
    juce::RangedAudioParameter* param,
    double sampleRate,
    int blockSize,
    float durationSeconds = 2.0f)
{
    ClickDetectionResult result;
    result.parameterName = param->getName(32).toStdString();
    result.maxTransientJump = 0.0f;
    result.maxSampleDiff = 0.0f;
    result.clickCount = 0;

    int totalSamples = static_cast<int>(sampleRate * durationSeconds);
    int numBlocks = (totalSamples + blockSize - 1) / blockSize;

    juce::AudioBuffer<float> fullBuffer(2, totalSamples);
    fullBuffer.clear();

    // Generate test tone (1kHz sine wave)
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < totalSamples; ++i)
        {
            float phase = 2.0f * juce::MathConstants<float>::pi * 1000.0f * i / sampleRate;
            fullBuffer.setSample(ch, i, 0.3f * std::sin(phase));
        }
    }

    int samplesProcessed = 0;

    // Process in blocks while sweeping parameter
    for (int block = 0; block < numBlocks; ++block)
    {
        // Calculate parameter value for this block (linear sweep 0.0 → 1.0)
        float normalizedValue = static_cast<float>(block) / numBlocks;
        param->setValueNotifyingHost(normalizedValue);

        int samplesToProcess = std::min(blockSize, totalSamples - samplesProcessed);

        // Create buffer view for this block
        juce::AudioBuffer<float> blockBuffer(2, samplesToProcess);
        for (int ch = 0; ch < 2; ++ch)
        {
            blockBuffer.copyFrom(ch, 0, fullBuffer, ch, samplesProcessed, samplesToProcess);
        }

        // Process through plugin
        juce::MidiBuffer midiBuffer;
        processor.processBlock(blockBuffer, midiBuffer);

        // Copy processed audio back to full buffer
        for (int ch = 0; ch < 2; ++ch)
        {
            fullBuffer.copyFrom(ch, samplesProcessed, blockBuffer, ch, 0, samplesToProcess);
        }

        samplesProcessed += samplesToProcess;
    }

    // Analyze full buffer for clicks
    result.clickCount = detectClicks(fullBuffer, 0.1f);
    result.maxSampleDiff = calculateMaxSampleDiff(fullBuffer);
    result.maxTransientJump = calculateMaxTransientJump(fullBuffer, sampleRate);

    result.passed = (result.maxTransientJump < kMaxTransientJumpDb) && (result.maxSampleDiff < 0.1f);

    return result;
}

int main()
{
    std::cout << "\n";
    std::cout << COLOR_BLUE << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << COLOR_RESET << "\n";
    std::cout << COLOR_BLUE << "  Monument Reverb - Parameter Smoothing Test" << COLOR_RESET << "\n";
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

    // Get all parameters
    auto& params = processor.getParameters();

    // Test each parameter
    std::cout << "Testing parameter sweeps (0.0 → 1.0 over 2 seconds)...\n";
    std::cout << "\n";

    std::vector<ClickDetectionResult> results;
    int testCount = 0;
    int passCount = 0;

    for (auto* paramBase : params)
    {
        auto* param = dynamic_cast<juce::RangedAudioParameter*>(paramBase);
        if (!param) continue;

        // Skip non-macro parameters (e.g., mix)
        juce::String paramName = param->getName(32);
        if (paramName.toLowerCase().contains("mix"))
            continue;

        testCount++;
        std::cout << "  Testing: " << std::setw(20) << std::left << paramName.toStdString() << " ";
        std::cout.flush();

        ClickDetectionResult result = testParameterSweep(processor, param, sampleRate, blockSize);
        results.push_back(result);

        if (result.passed)
        {
            std::cout << COLOR_GREEN << "✓ PASS" << COLOR_RESET;
            std::cout << " (jump: " << std::fixed << std::setprecision(1) << result.maxTransientJump << " dB)";
            passCount++;
        }
        else
        {
            std::cout << COLOR_RED << "✗ FAIL" << COLOR_RESET;
            std::cout << " (jump: " << std::fixed << std::setprecision(1) << result.maxTransientJump << " dB, ";
            std::cout << "clicks: " << result.clickCount << ")";
        }
        std::cout << "\n";

        // Reset parameter to default
        param->setValueNotifyingHost(param->getDefaultValue());
    }

    processor.releaseResources();

    // Print summary
    std::cout << "\n";
    std::cout << COLOR_BLUE << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << COLOR_RESET << "\n";
    std::cout << COLOR_BLUE << "  Summary" << COLOR_RESET << "\n";
    std::cout << COLOR_BLUE << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << COLOR_RESET << "\n";
    std::cout << "\n";
    std::cout << "  Total tests:  " << testCount << "\n";
    std::cout << "  Passed:       " << COLOR_GREEN << passCount << COLOR_RESET << "\n";
    std::cout << "  Failed:       " << COLOR_RED << (testCount - passCount) << COLOR_RESET << "\n";
    std::cout << "\n";

    if (passCount == testCount)
    {
        std::cout << COLOR_GREEN << "✓ All parameters smooth - no clicks detected!" << COLOR_RESET << "\n";
        std::cout << "\n";
        return 0;
    }
    else
    {
        std::cout << COLOR_RED << "✗ Some parameters produced clicks" << COLOR_RESET << "\n";
        std::cout << "\n";
        std::cout << "Failed parameters:\n";
        for (const auto& result : results)
        {
            if (!result.passed)
            {
                std::cout << "  • " << result.parameterName << ": ";
                std::cout << result.maxTransientJump << " dB jump, ";
                std::cout << result.clickCount << " clicks\n";
            }
        }
        std::cout << "\n";
        return 1;
    }
}
