/**
 * AudioCapture.cpp
 */

#include "AudioCapture.h"

namespace monument {
namespace tools {

AudioCapture::AudioCapture()
{
}

AudioCapture::~AudioCapture()
{
}

void AudioCapture::startCapture(double sampleRate, int numChannels, double expectedLengthSeconds)
{
    this->sampleRate = sampleRate;
    this->numChannels = numChannels;

    // Pre-allocate buffer
    int expectedSamples = static_cast<int>(expectedLengthSeconds * sampleRate);
    captureBuffer.setSize(numChannels, expectedSamples, false, true, false);
    captureBuffer.clear();

    capturePosition = 0;
    capturing = true;

    DBG("Started audio capture: " << numChannels << " channels @ " << sampleRate << " Hz");
}

void AudioCapture::appendAudio(const juce::AudioBuffer<float>& buffer)
{
    if (!capturing)
    {
        jassertfalse; // Must call startCapture first
        return;
    }

    int numSamplesToAdd = buffer.getNumSamples();
    int newPosition = capturePosition + numSamplesToAdd;

    // Resize buffer if needed (with headroom)
    if (newPosition > captureBuffer.getNumSamples())
    {
        int newSize = juce::jmax(newPosition, captureBuffer.getNumSamples() * 2);
        captureBuffer.setSize(numChannels, newSize, true, true, false);
        DBG("Resized capture buffer to " << newSize << " samples");
    }

    // Copy audio data
    for (int ch = 0; ch < juce::jmin(numChannels, buffer.getNumChannels()); ++ch)
    {
        captureBuffer.copyFrom(ch, capturePosition, buffer, ch, 0, numSamplesToAdd);
    }

    capturePosition = newPosition;
}

void AudioCapture::stopCapture()
{
    if (!capturing)
        return;

    // Trim buffer to actual captured length
    if (capturePosition < captureBuffer.getNumSamples())
    {
        captureBuffer.setSize(numChannels, capturePosition, true, true, false);
    }

    capturing = false;

    DBG("Stopped audio capture: " << capturePosition << " samples ("
        << getDurationSeconds() << " seconds)");
}

bool AudioCapture::exportToWav(const juce::String& outputPath, int bitDepth)
{
    juce::File outputFile(outputPath);

    // Create parent directory if needed
    outputFile.getParentDirectory().createDirectory();

    // Delete existing file
    if (outputFile.exists())
        outputFile.deleteFile();

    // Create output stream
    std::unique_ptr<juce::FileOutputStream> outputStream(outputFile.createOutputStream());

    if (!outputStream || outputStream->failedToOpen())
    {
        DBG("Failed to create output file: " << outputPath);
        return false;
    }

    // Create WAV writer
    juce::WavAudioFormat wavFormat;

    std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(
        outputStream.get(),
        sampleRate,
        static_cast<unsigned int>(numChannels),
        bitDepth,
        {},
        0));

    if (!writer)
    {
        DBG("Failed to create WAV writer");
        return false;
    }

    // Release ownership of the stream to the writer
    outputStream.release();

    // Write audio data
    const float* const* audioData = captureBuffer.getArrayOfReadPointers();
    bool success = writer->writeFromAudioSampleBuffer(captureBuffer, 0, captureBuffer.getNumSamples());

    writer.reset(); // Close file

    if (success)
    {
        DBG("Exported audio to: " << outputPath);
        DBG("  Format: " << numChannels << " ch, " << sampleRate << " Hz, " << bitDepth << "-bit");
        DBG("  Duration: " << getDurationSeconds() << " seconds");
        DBG("  File size: " << outputFile.getSize() << " bytes");
    }
    else
    {
        DBG("Failed to write audio data");
    }

    return success;
}

void AudioCapture::clear()
{
    captureBuffer.clear();
    capturePosition = 0;
    capturing = false;
}

double AudioCapture::getDurationSeconds() const
{
    if (sampleRate <= 0.0)
        return 0.0;

    return capturePosition / sampleRate;
}

} // namespace tools
} // namespace monument
