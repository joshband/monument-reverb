#include "AudioEngine.h"

namespace monument::playground
{
AudioEngine::AudioEngine()
{
    fifo.fill(0.0f);
    fftData.fill(0.0f);
}

void AudioEngine::setEnabled(bool shouldPlay)
{
    enabled.store(shouldPlay);
}

void AudioEngine::setGain(float newGain)
{
    gain.store(juce::jlimit(0.0f, 1.0f, newGain));
}

void AudioEngine::setFrequency(float newFrequency)
{
    frequency.store(juce::jlimit(40.0f, 2000.0f, newFrequency));
}

void AudioEngine::setNoiseAmount(float newAmount)
{
    noiseAmount.store(juce::jlimit(0.0f, 1.0f, newAmount));
}

AudioEngine::Metrics AudioEngine::getMetrics() const
{
    return {rmsValue.load(), peakValue.load(), centroidValue.load()};
}

void AudioEngine::audioDeviceAboutToStart(juce::AudioIODevice* device)
{
    sampleRate = device != nullptr ? device->getCurrentSampleRate() : 44100.0;
    phase = 0.0;
    fifoIndex = 0;
    rmsValue.store(0.0f);
    peakValue.store(0.0f);
    centroidValue.store(0.0f);
}

void AudioEngine::audioDeviceIOCallbackWithContext(const float* const* /*inputChannelData*/,
                                                   int /*numInputChannels*/,
                                                   float* const* outputChannelData,
                                                   int numOutputChannels,
                                                   int numSamples,
                                                   const juce::AudioIODeviceCallbackContext& /*context*/)
{
    if (outputChannelData == nullptr)
        return;

    const bool isEnabled = enabled.load();
    const float currentGain = gain.load();
    const float currentFrequency = frequency.load();
    const float currentNoise = noiseAmount.load();

    float peak = 0.0f;
    double phaseDelta = (currentFrequency / sampleRate) * juce::MathConstants<double>::twoPi;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float value = 0.0f;
        if (isEnabled)
        {
            value = std::sin(phase) * currentGain;
            value += (random.nextFloat() * 2.0f - 1.0f) * currentNoise * currentGain;
            phase += phaseDelta;
            if (phase > juce::MathConstants<double>::twoPi)
                phase -= juce::MathConstants<double>::twoPi;
        }

        pushSample(value);
        peak = std::max(peak, std::abs(value));

        for (int channel = 0; channel < numOutputChannels; ++channel)
        {
            if (auto* out = outputChannelData[channel])
                out[sample] = value;
        }
    }

    peakValue.store(peak);
}

void AudioEngine::audioDeviceStopped()
{
    rmsValue.store(0.0f);
    peakValue.store(0.0f);
    centroidValue.store(0.0f);
}

void AudioEngine::pushSample(float sample)
{
    fifo[fifoIndex++] = sample;

    if (fifoIndex >= fftSize)
    {
        processFft();
        fifoIndex = 0;
    }
}

void AudioEngine::processFft()
{
    float sumSquares = 0.0f;

    for (auto sample : fifo)
        sumSquares += sample * sample;

    rmsValue.store(std::sqrt(sumSquares / static_cast<float>(fftSize)));

    std::copy(fifo.begin(), fifo.end(), fftData.begin());
    std::fill(fftData.begin() + fftSize, fftData.end(), 0.0f);

    window.multiplyWithWindowingTable(fftData.data(), fftSize);
    fft.performFrequencyOnlyForwardTransform(fftData.data());

    float weightedSum = 0.0f;
    float magnitudeSum = 0.0f;

    const int binCount = fftSize / 2;
    for (int i = 0; i < binCount; ++i)
    {
        const float magnitude = fftData[i];
        const float frequencyBin = (static_cast<float>(i) * static_cast<float>(sampleRate))
                                   / static_cast<float>(fftSize);
        weightedSum += frequencyBin * magnitude;
        magnitudeSum += magnitude;
    }

    const float centroid = magnitudeSum > 0.0f ? weightedSum / magnitudeSum : 0.0f;
    const float normalizedCentroid = juce::jlimit(0.0f, 1.0f, centroid / 4000.0f);
    centroidValue.store(normalizedCentroid);
}
}  // namespace monument::playground
