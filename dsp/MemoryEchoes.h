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
    void setDecay(float decay);
    void setDrift(float drift);
    void setFreeze(bool shouldFreeze);

private:
    enum class BufferChoice
    {
        Recent,
        Distant
    };

    void scheduleNextRecall(float memoryAmount);
    void startFragment(float depth, float memoryAmount, float decayAmount, float driftAmount);

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
    ParameterSmoother decaySmoother;
    ParameterSmoother driftSmoother;
    float memoryTarget = 0.0f;
    float depthTarget = 0.5f;
    float decayTarget = 0.4f;
    float driftTarget = 0.3f;
    bool smoothersPrimed = false;
    bool memoryEnabled = false;
    float memoryAmountForCapture = 0.0f;

    bool freezeEnabled = false;

    juce::Random random;
    int samplesUntilRecall = 0;
    bool fragmentActive = false;
    bool blockHadFragment = false;
    BufferChoice activeBuffer = BufferChoice::Recent;
    float fragmentReadPos = 0.0f;
    int fragmentLengthSamples = 0;
    int fragmentSamplesRemaining = 0;
    int fragmentFadeSamples = 0;
    float fragmentGain = 0.0f;
    float fragmentAge = 0.0f;
    float fragmentLowpassCoeff = 0.0f;
    float fragmentLowpassStateL = 0.0f;
    float fragmentLowpassStateR = 0.0f;
    float fragmentSaturationDrive = 1.0f;
    float fragmentSaturationNorm = 1.0f;
    float fragmentDriftCents = 0.0f;
    float fragmentDriftTarget = 0.0f;
    float fragmentDriftCentsMax = 0.0f;
    float driftSlewCoeff = 0.0f;
    int driftUpdateSamples = 0;
    int driftUpdateRemaining = 0;
    float lastRecallAge = 0.0f;
};

} // namespace dsp
} // namespace monument
