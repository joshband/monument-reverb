#pragma once

#include "dsp/Chambers.h"
#include "dsp/DspModule.h"

#include <array>

namespace monument
{
namespace dsp
{
class Foundation final : public DSPModule
{
public:
    void prepare(double sampleRate, int blockSize, int numChannels) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void setInputGainDb(float gainDb);

private:
    double sampleRateHz = 44100.0;
    int maxBlockSize = 0;
    int channels = 0;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                   juce::dsp::IIR::Coefficients<float>>
        dcBlocker;
    juce::dsp::Gain<float> inputGain;
};

class Pillars final : public DSPModule
{
public:
    enum class Mode
    {
        Glass = 0,
        Stone = 1,
        Fog = 2
    };

    void prepare(double sampleRate, int blockSize, int numChannels) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void setDensity(float density);
    void setShape(float shape);
    void setMode(int modeIndex);
    void setWarp(float warp);

    // Loads a short impulse response (<= 50ms) for pseudo-convolution of early taps.
    // This should be called off the audio thread.
    bool loadImpulseResponse(const juce::File& file);
    void clearImpulseResponse();

private:
    static constexpr int kMaxTaps = 32;
    static constexpr int kMinTaps = 16;
    static constexpr float kMaxIrSeconds = 0.05f;

    void updateTapLayout();
    void updateModeTuning();
    float shapePosition(float position01) const;
    float applyAllpass(float input, float coeff, float& state) const;

    double sampleRateHz = 44100.0;
    int maxBlockSize = 0;
    int channels = 0;

    int tapCount = kMinTaps;
    std::array<int, kMaxTaps> tapSamples{};
    std::array<float, kMaxTaps> tapGains{};
    std::array<float, kMaxTaps> tapAllpassCoeff{};
    juce::AudioBuffer<float> tapAllpassState;

    // Per-sample smoothing for tap coefficients/gains to prevent clicks
    std::array<juce::SmoothedValue<float>, kMaxTaps> tapGainSmoothers;
    std::array<juce::SmoothedValue<float>, kMaxTaps> tapCoeffSmoothers;

    juce::AudioBuffer<float> delayBuffer;
    int delayBufferLength = 0;
    int writePosition = 0;

    float densityAmount = 0.5f;
    float warpAmount = 0.0f;
    float pillarShape = 0.5f;
    Mode pillarMode = Mode::Glass;
    bool tapsDirty = true;
    int mutationSamplesRemaining = 0;
    int mutationIntervalSamples = 0;
    int mutationSeed = 0;

    float modeLowpassCoeff = 0.0f;
    float modeHighpassCoeff = 0.0f;
    float modeDiffusion = 0.18f;
    float modeTapGain = 1.0f;
    juce::AudioBuffer<float> modeLowState;
    juce::AudioBuffer<float> modeHighState;

    juce::AudioBuffer<float> irBuffer;
    int irLengthSamples = 0;
    bool irLoaded = false;

    // Signal threshold for deferred tap updates (prevents clicks during active audio)
    float inputPeakMagnitude = 0.0f;
    static constexpr float kTapUpdateThreshold = 0.001f;  // ~-60dB
};

class Weathering final : public DSPModule
{
public:
    void prepare(double sampleRate, int blockSize, int numChannels) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void setWarp(float warp);
    void setDrift(float drift);

private:
    double sampleRateHz = 44100.0;
    int maxBlockSize = 0;
    int channels = 0;
    juce::AudioBuffer<float> delayBuffer;
    int delayBufferLength = 0;
    int writePosition = 0;
    float baseDelaySamples = 0.0f;
    float depthBaseSamples = 0.0f;
    float depthSamples = 0.0f;
    float mix = 0.25f;
    juce::dsp::Oscillator<float> lfo;
    float lfoRateHz = 0.08f;
    float warpAmount = 0.3f;
    float driftAmount = 0.3f;
};

class Buttress final : public DSPModule
{
public:
    void prepare(double sampleRate, int blockSize, int numChannels) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void setDrive(float driveAmount);
    void setFreeze(bool shouldFreeze);

private:
    double sampleRateHz = 44100.0;
    int maxBlockSize = 0;
    int channels = 0;
    float drive = 1.15f;
    bool freezeEnabled = false;
};

class Facade final : public DSPModule
{
public:
    void prepare(double sampleRate, int blockSize, int numChannels) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void setWidth(float widthAmount);
    void setAir(float airAmount);
    void setOutputGain(float gainLinear);

private:
    double sampleRateHz = 44100.0;
    int maxBlockSize = 0;
    int channels = 0;
    float width = 1.1f;
    float air = 0.5f;
    float outputGain = 1.0f;
    juce::AudioBuffer<float> airState;
    float airCoefficient = 0.0f;
    juce::SmoothedValue<float> airGainSmoother;  // Per-sample smoothing for airGain
};

} // namespace dsp
} // namespace monument
