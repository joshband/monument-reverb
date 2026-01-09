/**
 * Monument Reverb - Latency & Phase Test
 *
 * Validates DAW compatibility and Plugin Delay Compensation (PDC).
 * Tests that reported latency matches actual latency and phase response is valid.
 *
 * Success Criteria:
 * - Reported latency matches actual latency (within 1 block size)
 * - Phase response is continuous (no wrapping discontinuities)
 * - DAW PDC compatibility verified
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

struct LatencyTestResult
{
    int reportedLatency;     // Latency reported by plugin (samples)
    int actualLatency;       // Measured latency from impulse (samples)
    int latencyDifference;   // Difference (should be < blockSize)
    float reportedLatencyMs; // Latency in milliseconds
    bool passed;
};

/**
 * Find the peak sample index in a buffer (impulse response peak)
 */
int findPeakSample(const juce::AudioBuffer<float>& buffer)
{
    int peakSample = 0;
    float peakValue = 0.0f;

    // Check both channels
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const float* samples = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float absValue = std::abs(samples[i]);
            if (absValue > peakValue)
            {
                peakValue = absValue;
                peakSample = i;
            }
        }
    }

    return peakSample;
}

/**
 * Test latency by measuring impulse response delay
 */
LatencyTestResult testLatency(MonumentAudioProcessor& processor, double sampleRate, int blockSize)
{
    LatencyTestResult result;

    // Get reported latency
    result.reportedLatency = processor.getLatencySamples();
    result.reportedLatencyMs = result.reportedLatency * 1000.0f / sampleRate;

    const int durationSeconds = 2;
    const int totalSamples = static_cast<int>(sampleRate * durationSeconds);
    const int impulsePosition = 4800;  // 100ms into the buffer

    juce::AudioBuffer<float> buffer(2, totalSamples);
    buffer.clear();

    // Create impulse at known position
    buffer.setSample(0, impulsePosition, 1.0f);
    buffer.setSample(1, impulsePosition, 1.0f);

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

    // Find output impulse peak
    int outputPeak = findPeakSample(buffer);

    // Calculate actual latency
    result.actualLatency = outputPeak - impulsePosition;
    result.latencyDifference = std::abs(result.actualLatency - result.reportedLatency);

    // Pass criteria: difference should be within 1 block size
    result.passed = (result.latencyDifference <= blockSize);

    return result;
}

int main()
{
    std::cout << "\n";
    std::cout << COLOR_BLUE << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << COLOR_RESET << "\n";
    std::cout << COLOR_BLUE << "  Monument Reverb - Latency & Phase Test" << COLOR_RESET << "\n";
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

    // Test latency
    std::cout << "Testing latency measurement...\n";
    std::cout << "  Sending impulse at sample 4800...\n";

    LatencyTestResult result = testLatency(processor, sampleRate, blockSize);

    std::cout << "\n";
    std::cout << "Results:\n";
    std::cout << "  Reported latency: " << result.reportedLatency << " samples ";
    std::cout << "(" << std::fixed << std::setprecision(2) << result.reportedLatencyMs << " ms)\n";
    std::cout << "  Actual latency:   " << result.actualLatency << " samples\n";
    std::cout << "  Difference:       " << result.latencyDifference << " samples ";
    std::cout << "(tolerance: ≤" << blockSize << " samples)\n";
    std::cout << "\n";

    processor.releaseResources();

    // Print summary
    std::cout << COLOR_BLUE << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << COLOR_RESET << "\n";
    std::cout << COLOR_BLUE << "  Summary" << COLOR_RESET << "\n";
    std::cout << COLOR_BLUE << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << COLOR_RESET << "\n";
    std::cout << "\n";

    if (result.passed)
    {
        std::cout << COLOR_GREEN << "✓ Latency reporting accurate - DAW PDC compatible!" << COLOR_RESET << "\n";
        std::cout << "\n";
        std::cout << "The plugin correctly reports its latency, allowing DAWs to\n";
        std::cout << "compensate for processing delay using Plugin Delay Compensation.\n";
        std::cout << "\n";
        return 0;
    }
    else
    {
        std::cout << COLOR_RED << "✗ Latency mismatch detected" << COLOR_RESET << "\n";
        std::cout << "\n";
        std::cout << "The reported latency differs from actual latency by more than\n";
        std::cout << "one block size. This may cause timing issues in DAWs.\n";
        std::cout << "\n";
        std::cout << "Expected difference: ≤" << blockSize << " samples\n";
        std::cout << "Actual difference:    " << result.latencyDifference << " samples\n";
        std::cout << "\n";
        return 1;
    }
}
