#pragma once

#include <JuceHeader.h>

#include <array>
#include <atomic>

namespace monument::playground
{
class AudioEngine : public juce::AudioIODeviceCallback
{
public:
    struct Metrics
    {
        float rms{0.0f};
        float peak{0.0f};
        float centroid{0.0f};
    };

    AudioEngine();

    void setEnabled(bool shouldPlay);
    void setGain(float newGain);
    void setFrequency(float newFrequency);
    void setNoiseAmount(float newAmount);

    Metrics getMetrics() const;

    // juce::AudioIODeviceCallback overrides
    void audioDeviceAboutToStart(juce::AudioIODevice* device) override;
    void audioDeviceIOCallbackWithContext(const float* const* inputChannelData,
                                          int numInputChannels,
                                          float* const* outputChannelData,
                                          int numOutputChannels,
                                          int numSamples,
                                          const juce::AudioIODeviceCallbackContext& context) override;
    void audioDeviceStopped() override;

private:
    void pushSample(float sample);
    void processFft();

    std::atomic<bool> enabled{false};
    std::atomic<float> gain{0.2f};
    std::atomic<float> frequency{220.0f};
    std::atomic<float> noiseAmount{0.05f};

    double sampleRate{44100.0};
    double phase{0.0};

    juce::Random random;

    static constexpr int fftOrder = 9;
    static constexpr int fftSize = 1 << fftOrder;

    juce::dsp::FFT fft{fftOrder};
    juce::dsp::WindowingFunction<float> window{fftSize, juce::dsp::WindowingFunction<float>::hann};

    std::array<float, fftSize> fifo{};
    std::array<float, fftSize * 2> fftData{};
    int fifoIndex{0};

    std::atomic<float> rmsValue{0.0f};
    std::atomic<float> peakValue{0.0f};
    std::atomic<float> centroidValue{0.0f};
};
}  // namespace monument::playground