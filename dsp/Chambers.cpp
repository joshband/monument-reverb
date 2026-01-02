#include "Chambers.h"

#include <cmath>

namespace
{
constexpr float kInvSqrt8 = 0.3535533905932738f;

// Delay lengths in samples at 48 kHz.
// Chosen as primes (>5) to avoid common factors with 48 kHz (2^7 * 3 * 5^3),
// and spread from ~50 ms to ~1.23 s for a large, non-repeating late field.
constexpr std::array<int, 8> kDelaySamples48k{
    2411, 4201, 7001, 11003, 17011, 26003, 39019, 59009
};

// Input diffusion delays (48 kHz), 1–5 ms range and incommensurate.
constexpr std::array<int, 2> kInputDiffuserSamples48k{149, 223};

// Late diffusion delays (48 kHz), sub-10 ms, incommensurate across lines.
constexpr std::array<int, 8> kLateDiffuserSamples48k{
    157, 173, 197, 223, 251, 281, 313, 347
};

constexpr std::array<float, 8> kDampingOffsets{
    -0.035f, -0.025f, -0.015f, -0.005f, 0.005f, 0.015f, 0.025f, 0.035f
};

constexpr std::array<float, 8> kLateDiffuserCoeffOffsets{
    -0.06f, -0.045f, -0.03f, -0.015f, 0.015f, 0.03f, 0.045f, 0.06f
};

constexpr std::array<float, 8> kInputMid{
    1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f
};

constexpr std::array<float, 8> kInputSide{
    1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f
};

constexpr std::array<float, 8> kOutputLeft{
    1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f
};

constexpr std::array<float, 8> kOutputRight{
    1.0f, -1.0f, 1.0f, -1.0f, -1.0f, 1.0f, -1.0f, 1.0f
};

inline void hadamard8(float* v)
{
    const float a0 = v[0] + v[1];
    const float a1 = v[0] - v[1];
    const float a2 = v[2] + v[3];
    const float a3 = v[2] - v[3];
    const float a4 = v[4] + v[5];
    const float a5 = v[4] - v[5];
    const float a6 = v[6] + v[7];
    const float a7 = v[6] - v[7];

    const float b0 = a0 + a2;
    const float b1 = a1 + a3;
    const float b2 = a0 - a2;
    const float b3 = a1 - a3;
    const float b4 = a4 + a6;
    const float b5 = a5 + a7;
    const float b6 = a4 - a6;
    const float b7 = a5 - a7;

    v[0] = (b0 + b4) * kInvSqrt8;
    v[1] = (b1 + b5) * kInvSqrt8;
    v[2] = (b2 + b6) * kInvSqrt8;
    v[3] = (b3 + b7) * kInvSqrt8;
    v[4] = (b0 - b4) * kInvSqrt8;
    v[5] = (b1 - b5) * kInvSqrt8;
    v[6] = (b2 - b6) * kInvSqrt8;
    v[7] = (b3 - b7) * kInvSqrt8;
}

inline float readFractionalDelay(const float* line, int length, int writePos, float delaySamples)
{
    const int delayInt = static_cast<int>(delaySamples);
    const float frac = delaySamples - static_cast<float>(delayInt);

    int readPosA = writePos - delayInt;
    if (readPosA < 0)
        readPosA += length;

    int readPosB = readPosA - 1;
    if (readPosB < 0)
        readPosB += length;

    const float a = line[readPosA];
    const float b = line[readPosB];
    return a * (1.0f - frac) + b * frac;
}
} // namespace

namespace monument
{
namespace dsp
{
void Chambers::prepare(double sampleRate, int blockSize, int numChannels)
{
    sampleRateHz = sampleRate;
    maxBlockSize = blockSize;
    channels = numChannels;

    const float scale = static_cast<float>(sampleRateHz / 48000.0);
    float maxDelay = 0.0f;

    for (size_t i = 0; i < kNumLines; ++i)
    {
        delaySamples[i] = juce::jmax(1.0f, static_cast<float>(kDelaySamples48k[i]) * scale);
        maxDelay = juce::jmax(maxDelay, delaySamples[i]);
    }

    delayBufferLength = juce::jmax(1, static_cast<int>(std::ceil(maxDelay)) + 2);
    delayLines.setSize(kNumLines, delayBufferLength);
    delayLines.clear();
    writePositions.fill(0);
    lowpassState.fill(0.0f);
    dampingCoefficients.fill(1.0f);

    // Input diffusion: short, per-channel allpass before injection to build density
    // without touching the feedback loop (keeps FDN topology unchanged).
    for (size_t i = 0; i < inputDiffusers.size(); ++i)
    {
        const int diffuserDelaySamples = juce::jmax(
            1, static_cast<int>(std::round(kInputDiffuserSamples48k[i] * scale)));
        inputDiffusers[i].setDelaySamples(diffuserDelaySamples);
        inputDiffusers[i].prepare();
    }

    // Late-field diffusion: per-line allpass after delay read, before output mix.
    // This increases echo density and breaks periodicity without affecting feedback stability.
    for (size_t i = 0; i < kNumLines; ++i)
    {
        const int diffuserDelaySamples = juce::jmax(
            1, static_cast<int>(std::round(kLateDiffuserSamples48k[i] * scale)));
        lateDiffusers[i].setDelaySamples(diffuserDelaySamples);
        lateDiffusers[i].prepare();
    }

    setDensity(densityAmount);
}

void Chambers::reset()
{
    delayLines.clear();
    writePositions.fill(0);
    lowpassState.fill(0.0f);
    for (auto& diffuser : inputDiffusers)
        diffuser.reset();
    for (auto& diffuser : lateDiffusers)
        diffuser.reset();
}

void Chambers::process(juce::AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();

    const float dampingBaseLocal = juce::jlimit(0.0f, 0.98f, dampingBase + gravityAmount * 0.2f);
    for (size_t i = 0; i < kNumLines; ++i)
    {
        const float damping = juce::jlimit(0.0f, 0.98f, dampingBaseLocal + kDampingOffsets[i]);
        dampingCoefficients[i] = 1.0f - damping;
    }

    const float feedbackLocal = freezeEnabled ? 0.99f : feedbackBase;
    const float bloomScale = 1.0f + bloomAmount * 0.6f;
    const float inputGainLocal = densityInputGain * bloomScale;
    float earlyMixLocal = densityEarlyMix * (1.0f - bloomAmount * 0.5f);
    if (freezeEnabled)
        earlyMixLocal = 0.0f;
    earlyMixLocal = juce::jlimit(0.0f, 0.7f, earlyMixLocal);

    const float inputScale = inputGainLocal * kInvSqrt8;
    const float outputScale = kInvSqrt8;

    auto* left = buffer.getWritePointer(0);
    auto* right = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    std::array<float*, kNumLines> lineData{};
    for (size_t i = 0; i < kNumLines; ++i)
        lineData[i] = delayLines.getWritePointer(static_cast<int>(i));

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float inL = left[sample];
        const float inR = right != nullptr ? right[sample] : inL;

        // Input diffusion is pre-FDN to build density without altering the feedback topology.
        const float diffL = inputDiffusers[0].processSample(inL);
        const float diffR = inputDiffusers[1].processSample(inR);
        const float mid = 0.5f * (diffL + diffR);
        const float side = 0.5f * (diffL - diffR);

        float out[kNumLines];
        for (size_t i = 0; i < kNumLines; ++i)
        {
            out[i] = readFractionalDelay(lineData[i], delayBufferLength, writePositions[i], delaySamples[i]);
        }

        float feedback[kNumLines];
        for (size_t i = 0; i < kNumLines; ++i)
            feedback[i] = out[i];
        hadamard8(feedback);

        float lateOut[kNumLines];
        for (size_t i = 0; i < kNumLines; ++i)
        {
            // Late diffusion is post-read and pre-output mix to increase density
            // without placing allpass recursion inside the feedback loop.
            lateOut[i] = lateDiffusers[i].processSample(out[i]);
        }

        float wetL = 0.0f;
        float wetR = 0.0f;
        for (size_t i = 0; i < kNumLines; ++i)
        {
            wetL += lateOut[i] * kOutputLeft[i];
            wetR += lateOut[i] * kOutputRight[i];
        }
        wetL *= outputScale;
        wetR *= outputScale;

        for (size_t i = 0; i < kNumLines; ++i)
        {
            const float injection = freezeEnabled
                ? 0.0f
                : (mid * kInputMid[i] + side * kInputSide[i]) * inputScale;
            const float writeValue = injection + feedback[i] * feedbackLocal;
            const float damped = lowpassState[i] + dampingCoefficients[i] * (writeValue - lowpassState[i]);
            lowpassState[i] = damped;
            lineData[i][writePositions[i]] = damped;

            ++writePositions[i];
            if (writePositions[i] >= delayBufferLength)
                writePositions[i] = 0;
        }

        const float wetBlend = 1.0f - earlyMixLocal;
        if (right != nullptr)
        {
            left[sample] = inL * earlyMixLocal + wetL * wetBlend;
            right[sample] = inR * earlyMixLocal + wetR * wetBlend;
        }
        else
        {
            left[sample] = mid * earlyMixLocal + (wetL + wetR) * 0.5f * wetBlend;
        }
    }
}

void Chambers::setTime(float time)
{
    const auto t = juce::jlimit(0.0f, 1.0f, time);
    feedbackBase = juce::jmap(t, 0.35f, 0.92f);
}

void Chambers::setMass(float mass)
{
    const auto m = juce::jlimit(0.0f, 1.0f, mass);
    dampingBase = juce::jmap(m, 0.1f, 0.85f);
}

void Chambers::setDensity(float density)
{
    densityAmount = juce::jlimit(0.0f, 1.0f, density);
    densityInputGain = juce::jmap(densityAmount, 0.18f, 0.32f);
    densityEarlyMix = juce::jmap(densityAmount, 0.45f, 0.25f);

    // Density maps to diffusion strength: minimal but non-zero at 0,
    // strong at 1 while keeping coefficients safely < 0.75.
    const float inputCoeff = juce::jmap(densityAmount, 0.12f, 0.6f);
    const float lateCoeffBase = juce::jmap(densityAmount, 0.18f, 0.7f);

    for (auto& diffuser : inputDiffusers)
        diffuser.setCoefficient(inputCoeff);

    for (size_t i = 0; i < kNumLines; ++i)
    {
        const float coeff = lateCoeffBase * (1.0f + kLateDiffuserCoeffOffsets[i]);
        lateDiffusers[i].setCoefficient(juce::jlimit(0.05f, 0.74f, coeff));
    }
}

void Chambers::setBloom(float bloom)
{
    bloomAmount = juce::jlimit(0.0f, 1.0f, bloom);
}

void Chambers::setGravity(float gravity)
{
    gravityAmount = juce::jlimit(0.0f, 1.0f, gravity);
}

void Chambers::setFreeze(bool shouldFreeze)
{
    freezeEnabled = shouldFreeze;
}

} // namespace dsp
} // namespace monument
