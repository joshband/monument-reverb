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
    void prepare(double sampleRate, int blockSize, int numChannels) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void setDensity(float density);

private:
    static constexpr int kNumTaps = 6;
    double sampleRateHz = 44100.0;
    int maxBlockSize = 0;
    int channels = 0;
    std::array<int, kNumTaps> tapSamples{};
    std::array<float, kNumTaps> tapGains{};
    juce::AudioBuffer<float> delayBuffer;
    int delayBufferLength = 0;
    int writePosition = 0;
    float densityAmount = 0.5f;
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
    float airGain = 0.0f;
};

} // namespace dsp
} // namespace monument
