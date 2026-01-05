/**
 * TestSignalGenerator.h
 *
 * Generates standard test signals for audio plugin analysis:
 * - Impulse (single sample = 1.0)
 * - Sine sweep (logarithmic frequency sweep)
 * - White noise
 * - Pink noise
 */

#pragma once

#include <JuceHeader.h>

namespace monument {
namespace tools {

enum class SignalType
{
    Impulse,
    SineSweep,
    WhiteNoise,
    PinkNoise
};

class TestSignalGenerator
{
public:
    /**
     * Generate a test signal.
     *
     * @param type Type of signal to generate
     * @param durationSeconds Length of signal in seconds
     * @param sampleRate Sample rate in Hz
     * @param numChannels Number of channels (1=mono, 2=stereo)
     * @param amplitude Peak amplitude (0.0 to 1.0)
     * @return Audio buffer containing the generated signal
     */
    static juce::AudioBuffer<float> generate(
        SignalType type,
        double durationSeconds,
        double sampleRate,
        int numChannels = 2,
        float amplitude = 1.0f);

    /**
     * Generate an impulse signal (single sample = 1.0).
     * Useful for impulse response capture.
     */
    static juce::AudioBuffer<float> generateImpulse(
        double durationSeconds,
        double sampleRate,
        int numChannels = 2,
        float amplitude = 1.0f);

    /**
     * Generate a logarithmic sine sweep.
     * Useful for frequency response analysis.
     *
     * @param startFreq Starting frequency in Hz
     * @param endFreq Ending frequency in Hz
     */
    static juce::AudioBuffer<float> generateSineSweep(
        double durationSeconds,
        double sampleRate,
        float startFreq = 20.0f,
        float endFreq = 20000.0f,
        int numChannels = 2,
        float amplitude = 1.0f);

    /**
     * Generate white noise (uniform spectrum).
     */
    static juce::AudioBuffer<float> generateWhiteNoise(
        double durationSeconds,
        double sampleRate,
        int numChannels = 2,
        float amplitude = 1.0f);

    /**
     * Generate pink noise (1/f spectrum, -3dB/octave).
     */
    static juce::AudioBuffer<float> generatePinkNoise(
        double durationSeconds,
        double sampleRate,
        int numChannels = 2,
        float amplitude = 1.0f);

private:
    // Pink noise filter state (for Voss-McCartney algorithm)
    struct PinkNoiseState
    {
        std::array<float, 16> rows{};
        int runningSum = 0;
        juce::Random random;
    };
};

} // namespace tools
} // namespace monument
