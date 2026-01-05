/**
 * AudioCapture.h
 *
 * Captures audio buffers and exports to WAV files.
 * Supports both streaming (real-time append) and batch (single write) modes.
 */

#pragma once

#include <JuceHeader.h>
#include <memory>

namespace monument {
namespace tools {

class AudioCapture
{
public:
    AudioCapture();
    ~AudioCapture();

    /**
     * Start capturing audio to memory.
     *
     * @param sampleRate Sample rate in Hz
     * @param numChannels Number of audio channels
     * @param expectedLengthSeconds Expected duration (for pre-allocation)
     */
    void startCapture(double sampleRate, int numChannels, double expectedLengthSeconds = 10.0);

    /**
     * Append audio samples to the capture buffer.
     *
     * @param buffer Audio buffer to append (will be copied)
     */
    void appendAudio(const juce::AudioBuffer<float>& buffer);

    /**
     * Stop capturing and finalize the buffer.
     */
    void stopCapture();

    /**
     * Export captured audio to WAV file.
     *
     * @param outputPath Full path to output WAV file
     * @param bitDepth Bit depth (16, 24, or 32)
     * @return true if export succeeded
     */
    bool exportToWav(const juce::String& outputPath, int bitDepth = 24);

    /**
     * Get the captured audio buffer (read-only).
     */
    const juce::AudioBuffer<float>& getCapturedAudio() const { return captureBuffer; }

    /**
     * Clear the capture buffer.
     */
    void clear();

    /**
     * Check if currently capturing.
     */
    bool isCapturing() const { return capturing; }

    /**
     * Get capture statistics.
     */
    double getSampleRate() const { return sampleRate; }
    int getNumChannels() const { return numChannels; }
    int getNumSamples() const { return capturePosition; }
    double getDurationSeconds() const;

private:
    juce::AudioBuffer<float> captureBuffer;
    int capturePosition{0};
    bool capturing{false};

    double sampleRate{44100.0};
    int numChannels{2};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioCapture)
};

} // namespace tools
} // namespace monument
