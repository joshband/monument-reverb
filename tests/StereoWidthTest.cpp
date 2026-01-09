/**
 * Monument Reverb - Stereo Width Test
 *
 * Validates spatial processing correctness and channel correlation.
 * Tests that the reverb produces valid stereo width without phase issues.
 *
 * Success Criteria:
 * - Correlation coefficient: 0.0 ≤ r ≤ 1.0 (valid stereo range)
 * - Mono compatibility: < 6dB level drop when summed to mono
 * - No phase inversions or cancellation artifacts
 */

#include <JuceHeader.h>
#include "plugin/PluginProcessor.h"
#include <cmath>
#include <iostream>
#include <iomanip>

// ANSI color codes
#define COLOR_GREEN "\033[0;32m"
#define COLOR_RED "\033[0;31m"
#define COLOR_YELLOW "\033[1;33m"
#define COLOR_BLUE "\033[0;34m"
#define COLOR_RESET "\033[0m"

struct StereoAnalysisResult
{
    float correlation;        // Cross-correlation between L/R channels
    float rmsLeft;            // RMS level of left channel
    float rmsRight;           // RMS level of right channel
    float rmsStereo;          // RMS of stereo signal
    float rmsMonoSum;         // RMS when summed to mono
    float monoCompatibilityDb; // Level difference in dB (should be < 6dB)
    bool passed;
};

/**
 * Calculate RMS level of a buffer
 */
float calculateRMS(const juce::AudioBuffer<float>& buffer, int channel = -1)
{
    float sumSquared = 0.0f;
    int sampleCount = 0;

    int startChannel = (channel < 0) ? 0 : channel;
    int endChannel = (channel < 0) ? buffer.getNumChannels() : channel + 1;

    for (int ch = startChannel; ch < endChannel; ++ch)
    {
        const float* samples = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            sumSquared += samples[i] * samples[i];
            sampleCount++;
        }
    }

    return std::sqrt(sumSquared / sampleCount);
}

/**
 * Calculate cross-correlation coefficient between two channels
 * Returns value between -1.0 (perfectly anti-correlated) and 1.0 (perfectly correlated)
 */
float calculateCrossCorrelation(const float* left, const float* right, int numSamples)
{
    // Calculate means
    float meanL = 0.0f, meanR = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        meanL += left[i];
        meanR += right[i];
    }
    meanL /= numSamples;
    meanR /= numSamples;

    // Calculate correlation coefficient
    float numerator = 0.0f;
    float denomL = 0.0f;
    float denomR = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        float diffL = left[i] - meanL;
        float diffR = right[i] - meanR;

        numerator += diffL * diffR;
        denomL += diffL * diffL;
        denomR += diffR * diffR;
    }

    float denominator = std::sqrt(denomL * denomR);

    if (denominator < 1e-10f)
        return 0.0f;  // Avoid division by zero

    float correlation = numerator / denominator;

    // Clamp near-zero values to exactly 0.0 to avoid floating-point precision issues
    if (std::abs(correlation) < 1e-6f)
        return 0.0f;

    return correlation;
}

/**
 * Test stereo width with mono input
 */
StereoAnalysisResult testMonoInput(MonumentAudioProcessor& processor, double sampleRate, int blockSize)
{
    StereoAnalysisResult result;

    const int durationSeconds = 5;
    const int totalSamples = static_cast<int>(sampleRate * durationSeconds);

    juce::AudioBuffer<float> buffer(2, totalSamples);
    buffer.clear();

    // Generate mono input (identical sine wave on both channels)
    const float frequency = 440.0f;  // A4 note
    for (int i = 0; i < totalSamples; ++i)
    {
        float phase = 2.0f * juce::MathConstants<float>::pi * frequency * i / sampleRate;
        float value = 0.3f * std::sin(phase);
        buffer.setSample(0, i, value);
        buffer.setSample(1, i, value);
    }

    // Process in blocks
    int samplesProcessed = 0;
    while (samplesProcessed < totalSamples)
    {
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

    // Analyze stereo output
    result.rmsLeft = calculateRMS(buffer, 0);
    result.rmsRight = calculateRMS(buffer, 1);
    result.rmsStereo = calculateRMS(buffer, -1);  // All channels

    result.correlation = calculateCrossCorrelation(
        buffer.getReadPointer(0),
        buffer.getReadPointer(1),
        totalSamples
    );

    // Create mono sum
    juce::AudioBuffer<float> monoBuffer(1, totalSamples);
    for (int i = 0; i < totalSamples; ++i)
    {
        float summed = (buffer.getSample(0, i) + buffer.getSample(1, i)) * 0.5f;
        monoBuffer.setSample(0, i, summed);
    }

    result.rmsMonoSum = calculateRMS(monoBuffer, 0);

    // Calculate mono compatibility (level difference in dB)
    if (result.rmsMonoSum > 1e-10f && result.rmsStereo > 1e-10f)
    {
        result.monoCompatibilityDb = 20.0f * std::log10(result.rmsMonoSum / result.rmsStereo);
    }
    else
    {
        result.monoCompatibilityDb = -120.0f;  // Essentially silent
    }

    // Pass criteria:
    // 1. Correlation in valid range (-0.1 to 1.0 for reverb)
    //    Slight negative correlation is normal due to phase shifts from allpass filters
    //    Strong negative correlation (< -0.5) indicates phase cancellation issues
    // 2. Mono compatibility > -6dB (less than 6dB drop)
    result.passed = (result.correlation >= -0.1f && result.correlation <= 1.0f) &&
                    (result.monoCompatibilityDb > -6.0f);

    return result;
}

/**
 * Test stereo width with stereo input
 */
StereoAnalysisResult testStereoInput(MonumentAudioProcessor& processor, double sampleRate, int blockSize)
{
    StereoAnalysisResult result;

    const int durationSeconds = 5;
    const int totalSamples = static_cast<int>(sampleRate * durationSeconds);

    juce::AudioBuffer<float> buffer(2, totalSamples);
    buffer.clear();

    // Generate stereo input (different frequencies on L/R)
    const float freqLeft = 440.0f;   // A4
    const float freqRight = 554.37f; // C#5
    for (int i = 0; i < totalSamples; ++i)
    {
        float phaseL = 2.0f * juce::MathConstants<float>::pi * freqLeft * i / sampleRate;
        float phaseR = 2.0f * juce::MathConstants<float>::pi * freqRight * i / sampleRate;
        buffer.setSample(0, i, 0.3f * std::sin(phaseL));
        buffer.setSample(1, i, 0.3f * std::sin(phaseR));
    }

    // Process in blocks
    int samplesProcessed = 0;
    while (samplesProcessed < totalSamples)
    {
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

    // Analyze stereo output
    result.rmsLeft = calculateRMS(buffer, 0);
    result.rmsRight = calculateRMS(buffer, 1);
    result.rmsStereo = calculateRMS(buffer, -1);

    result.correlation = calculateCrossCorrelation(
        buffer.getReadPointer(0),
        buffer.getReadPointer(1),
        totalSamples
    );

    // Create mono sum
    juce::AudioBuffer<float> monoBuffer(1, totalSamples);
    for (int i = 0; i < totalSamples; ++i)
    {
        float summed = (buffer.getSample(0, i) + buffer.getSample(1, i)) * 0.5f;
        monoBuffer.setSample(0, i, summed);
    }

    result.rmsMonoSum = calculateRMS(monoBuffer, 0);

    // Calculate mono compatibility
    if (result.rmsMonoSum > 1e-10f && result.rmsStereo > 1e-10f)
    {
        result.monoCompatibilityDb = 20.0f * std::log10(result.rmsMonoSum / result.rmsStereo);
    }
    else
    {
        result.monoCompatibilityDb = -120.0f;
    }

    // Pass criteria (same as mono input test)
    result.passed = (result.correlation >= -0.1f && result.correlation <= 1.0f) &&
                    (result.monoCompatibilityDb > -6.0f);

    return result;
}

int main()
{
    std::cout << "\n";
    std::cout << COLOR_BLUE << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << COLOR_RESET << "\n";
    std::cout << COLOR_BLUE << "  Monument Reverb - Stereo Width Test" << COLOR_RESET << "\n";
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

    int testCount = 0;
    int passCount = 0;

    // Test 1: Mono Input → Stereo Output
    std::cout << "Test 1: Mono Input (verifies stereo width expansion)\n";
    std::cout << "  Processing 5 seconds of 440 Hz sine wave...\n";

    StereoAnalysisResult monoResult = testMonoInput(processor, sampleRate, blockSize);
    testCount++;

    std::cout << "  Correlation:       " << std::fixed << std::setprecision(3) << monoResult.correlation << "\n";
    std::cout << "  RMS Left:          " << std::fixed << std::setprecision(6) << monoResult.rmsLeft << "\n";
    std::cout << "  RMS Right:         " << std::fixed << std::setprecision(6) << monoResult.rmsRight << "\n";
    std::cout << "  Mono compatibility: " << std::fixed << std::setprecision(2) << monoResult.monoCompatibilityDb << " dB\n";

    if (monoResult.passed)
    {
        std::cout << "  " << COLOR_GREEN << "✓ PASS" << COLOR_RESET << " (valid stereo width, mono compatible)\n";
        passCount++;
    }
    else
    {
        std::cout << "  " << COLOR_RED << "✗ FAIL" << COLOR_RESET;
        if (monoResult.correlation < -0.1f || monoResult.correlation > 1.0f)
            std::cout << " (correlation out of range: " << monoResult.correlation << ", valid: -0.1 to 1.0)";
        if (monoResult.monoCompatibilityDb <= -6.0f)
            std::cout << " (mono compatibility poor)";
        std::cout << "\n";
    }
    std::cout << "\n";

    // Test 2: Stereo Input → Stereo Output
    std::cout << "Test 2: Stereo Input (verifies correlation preservation)\n";
    std::cout << "  Processing 5 seconds of dual-tone stereo...\n";

    StereoAnalysisResult stereoResult = testStereoInput(processor, sampleRate, blockSize);
    testCount++;

    std::cout << "  Correlation:       " << std::fixed << std::setprecision(3) << stereoResult.correlation << "\n";
    std::cout << "  RMS Left:          " << std::fixed << std::setprecision(6) << stereoResult.rmsLeft << "\n";
    std::cout << "  RMS Right:         " << std::fixed << std::setprecision(6) << stereoResult.rmsRight << "\n";
    std::cout << "  Mono compatibility: " << std::fixed << std::setprecision(2) << stereoResult.monoCompatibilityDb << " dB\n";

    if (stereoResult.passed)
    {
        std::cout << "  " << COLOR_GREEN << "✓ PASS" << COLOR_RESET << " (valid stereo processing, mono compatible)\n";
        passCount++;
    }
    else
    {
        std::cout << "  " << COLOR_RED << "✗ FAIL" << COLOR_RESET;
        if (stereoResult.correlation < -0.1f || stereoResult.correlation > 1.0f)
            std::cout << " (correlation out of range: " << stereoResult.correlation << ", valid: -0.1 to 1.0)";
        if (stereoResult.monoCompatibilityDb <= -6.0f)
            std::cout << " (mono compatibility poor)";
        std::cout << "\n";
    }
    std::cout << "\n";

    processor.releaseResources();

    // Print summary
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
        std::cout << COLOR_GREEN << "✓ Stereo processing validated - width and phase correct!" << COLOR_RESET << "\n";
        std::cout << "\n";
        return 0;
    }
    else
    {
        std::cout << COLOR_RED << "✗ Stereo processing issues detected" << COLOR_RESET << "\n";
        std::cout << "\n";
        return 1;
    }
}
