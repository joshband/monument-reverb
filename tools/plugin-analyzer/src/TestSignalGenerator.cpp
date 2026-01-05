/**
 * TestSignalGenerator.cpp
 */

#include "TestSignalGenerator.h"
#include <cmath>

namespace monument {
namespace tools {

juce::AudioBuffer<float> TestSignalGenerator::generate(
    SignalType type,
    double durationSeconds,
    double sampleRate,
    int numChannels,
    float amplitude)
{
    switch (type)
    {
        case SignalType::Impulse:
            return generateImpulse(durationSeconds, sampleRate, numChannels, amplitude);
        case SignalType::SineSweep:
            return generateSineSweep(durationSeconds, sampleRate, 20.0f, 20000.0f, numChannels, amplitude);
        case SignalType::WhiteNoise:
            return generateWhiteNoise(durationSeconds, sampleRate, numChannels, amplitude);
        case SignalType::PinkNoise:
            return generatePinkNoise(durationSeconds, sampleRate, numChannels, amplitude);
        default:
            return generateImpulse(durationSeconds, sampleRate, numChannels, amplitude);
    }
}

juce::AudioBuffer<float> TestSignalGenerator::generateImpulse(
    double durationSeconds,
    double sampleRate,
    int numChannels,
    float amplitude)
{
    int numSamples = static_cast<int>(durationSeconds * sampleRate);
    juce::AudioBuffer<float> buffer(numChannels, numSamples);
    buffer.clear();

    // Set first sample to amplitude
    for (int ch = 0; ch < numChannels; ++ch)
        buffer.setSample(ch, 0, amplitude);

    return buffer;
}

juce::AudioBuffer<float> TestSignalGenerator::generateSineSweep(
    double durationSeconds,
    double sampleRate,
    float startFreq,
    float endFreq,
    int numChannels,
    float amplitude)
{
    int numSamples = static_cast<int>(durationSeconds * sampleRate);
    juce::AudioBuffer<float> buffer(numChannels, numSamples);

    // Logarithmic sweep (exponential frequency change)
    float startLogFreq = std::log2(startFreq);
    float endLogFreq = std::log2(endFreq);
    float logFreqRange = endLogFreq - startLogFreq;

    double phase = 0.0;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float t = static_cast<float>(sample) / numSamples;
        float logFreq = startLogFreq + t * logFreqRange;
        float frequency = std::pow(2.0f, logFreq);

        float value = amplitude * std::sin(phase);

        for (int ch = 0; ch < numChannels; ++ch)
            buffer.setSample(ch, sample, value);

        phase += juce::MathConstants<double>::twoPi * frequency / sampleRate;

        // Wrap phase to avoid precision loss
        if (phase > juce::MathConstants<double>::twoPi)
            phase -= juce::MathConstants<double>::twoPi;
    }

    return buffer;
}

juce::AudioBuffer<float> TestSignalGenerator::generateWhiteNoise(
    double durationSeconds,
    double sampleRate,
    int numChannels,
    float amplitude)
{
    int numSamples = static_cast<int>(durationSeconds * sampleRate);
    juce::AudioBuffer<float> buffer(numChannels, numSamples);

    juce::Random random;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float value = (random.nextFloat() * 2.0f - 1.0f) * amplitude;
            buffer.setSample(ch, sample, value);
        }
    }

    return buffer;
}

juce::AudioBuffer<float> TestSignalGenerator::generatePinkNoise(
    double durationSeconds,
    double sampleRate,
    int numChannels,
    float amplitude)
{
    int numSamples = static_cast<int>(durationSeconds * sampleRate);
    juce::AudioBuffer<float> buffer(numChannels, numSamples);

    // Voss-McCartney algorithm for pink noise
    for (int ch = 0; ch < numChannels; ++ch)
    {
        PinkNoiseState state;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Find rightmost zero bit
            int diff = state.runningSum ^ (state.runningSum + 1);
            state.runningSum++;

            // Update corresponding generator
            for (int i = 0; i < 16; ++i)
            {
                if (diff & (1 << i))
                {
                    state.rows[i] = state.random.nextFloat() * 2.0f - 1.0f;
                }
            }

            // Sum all generators
            float sum = 0.0f;
            for (int i = 0; i < 16; ++i)
                sum += state.rows[i];

            // Normalize and apply amplitude
            float value = (sum / 8.0f) * amplitude;
            buffer.setSample(ch, sample, juce::jlimit(-1.0f, 1.0f, value));
        }
    }

    return buffer;
}

} // namespace tools
} // namespace monument
