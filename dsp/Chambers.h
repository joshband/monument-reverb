#pragma once

#include "dsp/AllpassDiffuser.h"
#include "dsp/DspModule.h"

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
    void setFreeze(bool shouldFreeze);

private:
    static constexpr int kNumLines = 8;
    double sampleRateHz = 44100.0;
    int maxBlockSize = 0;
    int channels = 0;
    juce::AudioBuffer<float> delayLines;
    std::array<float, kNumLines> delaySamples{};
    std::array<int, kNumLines> writePositions{};
    std::array<float, kNumLines> lowpassState{};
    std::array<float, kNumLines> dampingCoefficients{};
    int delayBufferLength = 0;
    float feedbackBase = 0.75f;
    float dampingBase = 0.55f;
    float densityInputGain = 0.25f;
    float densityEarlyMix = 0.35f;
    float densityAmount = 0.5f;
    float bloomAmount = 0.0f;
    float gravityAmount = 0.0f;
    bool freezeEnabled = false;

    std::array<AllpassDiffuser, 2> inputDiffusers;
    std::array<AllpassDiffuser, kNumLines> lateDiffusers;
};

} // namespace dsp
} // namespace monument
