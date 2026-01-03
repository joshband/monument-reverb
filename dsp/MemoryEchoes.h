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

    // Renders memory surfacing and optionally injects into the pre-Chambers buffer.
    void process(juce::AudioBuffer<float>& buffer) override;

    // Captures post-Chambers wet output into memory buffers.
    void captureWet(const juce::AudioBuffer<float>& wetBuffer);

    void setMemory(float amount);
    void setDepth(float depth);
    void setDecay(float decay);
    void setDrift(float drift);
    void setChambersInputGain(float inputGain);
    void setFreeze(bool shouldFreeze);
    void setInjectToBuffer(bool shouldInject);
    const juce::AudioBuffer<float>& getRecallBuffer() const;
#if defined(MONUMENT_TESTING)
    void setRandomSeed(int64_t seed);
#endif

private:
    enum class SurfaceState
    {
        Idle,
        FadeIn,
        Hold,
        FadeOut
    };

    void maybeStartSurface(int blockSamples,
        float memoryAmount,
        float depth,
        float decayAmount,
        float driftAmount);
    void startSurface(bool useLong,
        float memoryAmount,
        float decayAmount,
        float driftAmount);
    void advanceSurface();
    void readShortMemory(float readPos, float& outAge, float& outL, float& outR) const;
    float readLongMemory(float readPos, float& outAge) const;

    double sampleRateHz = 44100.0;
    int maxBlockSize = 0;
    int channels = 0;

    juce::AudioBuffer<float> shortBuffer;
    juce::AudioBuffer<float> longBuffer;
    juce::AudioBuffer<float> recallBuffer;
    int shortLengthSamples = 0;
    int longLengthSamples = 0;
    int shortWritePos = 0;
    int longWritePos = 0;
    int shortFilledSamples = 0;
    int longFilledSamples = 0;
    float shortForgetFactor = 1.0f;
    float longForgetFactor = 1.0f;
    float shortCaptureGain = 1.0f;
    float longCaptureGain = 1.0f;
    float captureRmsCoeff = 0.0f;
    float lastCaptureRms = 0.0f;

    ParameterSmoother memorySmoother;
    ParameterSmoother depthSmoother;
    ParameterSmoother decaySmoother;
    ParameterSmoother driftSmoother;
    float memoryTarget = 0.0f;
    float depthTarget = 0.5f;
    float decayTarget = 0.4f;
    float driftTarget = 0.3f;
    float chambersInputGain = 0.25f;
    bool smoothersPrimed = false;
    bool memoryEnabled = false;
    float memoryAmountForCapture = 0.0f;
    bool injectToBuffer = true;
    bool freezeEnabled = false;

    juce::Random random;
    SurfaceState surfaceState = SurfaceState::Idle;
    bool surfaceUsesLong = false;
    float surfaceCenterPos = 0.0f;
    int surfaceWidthSamples = 0;
    int surfaceTotalSamples = 0;
    int surfaceSamplesProcessed = 0;
    float surfaceBaseGain = 0.0f;
    int surfaceFadeInSamples = 0;
    int surfaceHoldSamples = 0;
    int surfaceFadeOutSamples = 0;
    int surfaceSamplesRemaining = 0;
    float surfaceGain = 0.0f;
    float surfaceGainStep = 0.0f;
    int surfaceCooldownSamples = 0;
    float surfacePlaybackPos = 0.0f;
    float surfacePlaybackStep = 0.0f;
    float surfaceLowpassStateL = 0.0f;
    float surfaceLowpassStateR = 0.0f;
    float surfaceDriftCents = 0.0f;
    float surfaceDriftTarget = 0.0f;
    float surfaceDriftCentsMax = 0.0f;
    float driftSlewCoeff = 0.0f;
    int driftUpdateSamples = 0;
    int driftUpdateRemaining = 0;
    float lastMemoryAmount = 0.0f;
    float lastDepthAmount = 0.5f;
    float lastDecayAmount = 0.4f;
    float lastDriftAmount = 0.3f;
};

} // namespace dsp
} // namespace monument
