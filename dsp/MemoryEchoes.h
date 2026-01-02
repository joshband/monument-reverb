#pragma once

#include "dsp/DspModule.h"
#include "dsp/ParameterSmoother.h"

#include <JuceHeader.h>

namespace monument
{
namespace dsp
{
class MemoryEchoes final : public DSPModule
{
public:
    void prepare(double sampleRate, int blockSize, int numChannels) override;
    void reset() override;

    // Applies memory recall injection before Chambers.
    void process(juce::AudioBuffer<float>& buffer) override;

    // Captures post-Chambers wet output into memory buffers.
    void captureWet(const juce::AudioBuffer<float>& wetBuffer);

    void setMemory(float amount);
    void setDepth(float depth);
    void setFreeze(bool shouldFreeze);

private:
    enum class BufferChoice
    {
        Recent,
        Distant
    };

    void scheduleNextRecall();
    void startFragment(float depth, float memoryAmount);

    double sampleRateHz = 44100.0;
    int maxBlockSize = 0;
    int channels = 0;

    juce::AudioBuffer<float> recentBuffer;
    juce::AudioBuffer<float> distantBuffer;
    int recentLengthSamples = 0;
    int distantLengthSamples = 0;
    int recentWritePos = 0;
    int distantWritePos = 0;

    int captureStride = 4;
    int captureCounter = 0;
    float captureEnvelope = 0.0f;
    float captureThreshold = 1.0e-4f;
    float captureAttackCoeff = 0.0f;
    float captureReleaseCoeff = 0.0f;

    ParameterSmoother memorySmoother;
    ParameterSmoother depthSmoother;
    float memoryTarget = 0.0f;
    float depthTarget = 0.5f;
    bool smoothersPrimed = false;
    bool memoryEnabled = false;
    float memoryAmountForCapture = 0.0f;

    bool freezeEnabled = false;

    juce::Random random;
    int samplesUntilRecall = 0;
    bool fragmentActive = false;
    BufferChoice activeBuffer = BufferChoice::Recent;
    int fragmentReadPos = 0;
    int fragmentLengthSamples = 0;
    int fragmentSamplesRemaining = 0;
    int fragmentFadeSamples = 0;
    float fragmentGain = 0.0f;
};

} // namespace dsp
} // namespace monument
