#pragma once

#include "dsp/AllpassDiffuser.h"
#include "dsp/DspModule.h"
#include "dsp/ParameterSmoother.h"

#include <array>

namespace monument
{
namespace dsp
{
class Chambers final : public DSPModule
{
public:
    void prepare(double sampleRate, int blockSize, int numChannels) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void setTime(float time);
    void setMass(float mass);
    void setDensity(float density);
    void setBloom(float bloom);
    void setGravity(float gravity);
    void setWarp(float warp);
    void setFreeze(bool shouldFreeze);

private:
    static constexpr int kNumLines = 8;
    double sampleRateHz = 44100.0;
    int maxBlockSize = 0;
    int channels = 0;
    juce::AudioBuffer<float> delayLines;
    std::array<float, kNumLines> delaySamples{};
    std::array<int, kNumLines> writePositions{};
    std::array<int, kNumLines> freezeWritePositions{};
    std::array<float, kNumLines> lowpassState{};
    std::array<float, kNumLines> gravityLowpassState{};
    std::array<float, kNumLines> dampingCoefficients{};
    int delayBufferLength = 0;

    ParameterSmoother timeSmoother;
    ParameterSmoother massSmoother;
    ParameterSmoother densitySmoother;
    ParameterSmoother gravitySmoother;
    ParameterSmoother bloomSmoother;
    ParameterSmoother warpSmoother;
    float timeTarget = 0.55f;
    float massTarget = 0.5f;
    float densityTarget = 0.5f;
    float gravityTarget = 0.5f;
    float bloomTarget = 0.5f;
    float warpTarget = 0.0f;
    float warpSmoothed = 0.0f;
    float lastMatrixBlend = 1.0f;
    float gravityCoeffMin = 1.0f;
    float gravityCoeffMax = 1.0f;
    bool smoothersPrimed = false;
    bool isFrozen = false;
    bool wasFrozen = false;
    int freezeRampSamples = 0;
    int freezeRampRemaining = 0;
    float freezeRampStep = 1.0f;
    float freezeBlend = 1.0f;

    float envelopeTimeSeconds = 0.0f;
    float envelopeValue = 1.0f;
    float envelopeResetThreshold = 1.0e-4f;
    bool envelopeTriggerArmed = true;

    std::array<AllpassDiffuser, 2> inputDiffusers;
    std::array<AllpassDiffuser, kNumLines> lateDiffusers;
    std::array<std::array<float, kNumLines>, kNumLines> warpMatrix{};
    std::array<std::array<float, kNumLines>, kNumLines> warpMatrixFrozen{};
    std::array<std::array<float, kNumLines>, kNumLines> feedbackMatrix{};
};

} // namespace dsp
} // namespace monument
