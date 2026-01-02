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

// Constant-power pan weights per line (no sign flips) so mono sum keeps all taps.
// Pan positions: {-0.9, 0.9, -0.7, 0.7, -0.5, 0.5, -0.3, 0.3}.
constexpr std::array<float, 8> kOutputLeft{
    0.9969173f, 0.0784591f, 0.9723699f, 0.2334454f,
    0.9238795f, 0.3826834f, 0.8526402f, 0.5224986f
};

constexpr std::array<float, 8> kOutputRight{
    0.0784591f, 0.9969173f, 0.2334454f, 0.9723699f,
    0.3826834f, 0.9238795f, 0.5224986f, 0.8526402f
};

constexpr float kOutputGain = 0.5f; // sum(L^2) == sum(R^2) == 4.0 -> normalize to unity.

constexpr float kGravityCutoffMinHz = 20.0f;
constexpr float kGravityCutoffMaxHz = 200.0f;

constexpr float kFreezeReleaseMs = 40.0f;
constexpr float kFreezeLimiterCeiling = 0.9f;
constexpr float kWetLimiterCeiling = 0.95f;
constexpr float kMaxFeedback = 0.98f;
constexpr float kBloomSmoothingMs = 40.0f;
constexpr float kEnvelopeMinTimeSeconds = 1.0f;
constexpr float kEnvelopeMaxTimeSeconds = 12.0f;
constexpr float kBloomPeakGain = 0.5f; // Up to 1.5x at Bloom=1.

inline float onePoleCoeffFromHz(float cutoffHz, double sampleRate)
{
    const double omega = 2.0 * juce::MathConstants<double>::pi
        * static_cast<double>(cutoffHz) / sampleRate;
    return static_cast<float>(std::exp(-omega));
}

inline float freezeHardLimit(float value)
{
    return juce::jlimit(-kFreezeLimiterCeiling, kFreezeLimiterCeiling, value);
}

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
    freezeWritePositions = writePositions;
    lowpassState.fill(0.0f);
    gravityLowpassState.fill(0.0f);
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

    gravityCoeffMin = onePoleCoeffFromHz(kGravityCutoffMinHz, sampleRateHz);
    gravityCoeffMax = onePoleCoeffFromHz(kGravityCutoffMaxHz, sampleRateHz);
    freezeRampSamples = juce::jmax(
        1, static_cast<int>(std::round(sampleRateHz * (kFreezeReleaseMs / 1000.0f))));
    freezeRampStep = 1.0f / static_cast<float>(freezeRampSamples);
    freezeRampRemaining = 0;
    freezeBlend = 1.0f;
    wasFrozen = isFrozen;

    // Per-parameter smoothing times are tuned to feel responsive while preventing zipper noise.
    timeSmoother.prepare(sampleRateHz);
    timeSmoother.setSmoothingTimeMs(40.0f); // Time (feedback) needs smooth tail-safe motion.
    timeSmoother.setTarget(timeTarget);

    massSmoother.prepare(sampleRateHz);
    massSmoother.setSmoothingTimeMs(60.0f); // Mass (damping) is slower to avoid HF flutter.
    massSmoother.setTarget(massTarget);

    densitySmoother.prepare(sampleRateHz);
    densitySmoother.setSmoothingTimeMs(30.0f); // Density can move faster without clicks.
    densitySmoother.setTarget(densityTarget);

    gravitySmoother.prepare(sampleRateHz);
    gravitySmoother.setSmoothingTimeMs(80.0f); // Gravity is slow to avoid LF pumping.
    gravitySmoother.setTarget(gravityTarget);

    bloomSmoother.prepare(sampleRateHz);
    bloomSmoother.setSmoothingTimeMs(kBloomSmoothingMs); // Bloom envelope changes should be smooth.
    bloomSmoother.setTarget(bloomTarget);

    smoothersPrimed = false;
    envelopeTimeSeconds = 0.0f;
    envelopeValue = 1.0f;
    envelopeTriggerArmed = true;
}

void Chambers::reset()
{
    delayLines.clear();
    writePositions.fill(0);
    freezeWritePositions = writePositions;
    lowpassState.fill(0.0f);
    gravityLowpassState.fill(0.0f);
    for (auto& diffuser : inputDiffusers)
        diffuser.reset();
    for (auto& diffuser : lateDiffusers)
        diffuser.reset();
    smoothersPrimed = false;
    freezeRampRemaining = 0;
    freezeBlend = 1.0f;
    wasFrozen = isFrozen;
    envelopeTimeSeconds = 0.0f;
    envelopeValue = 1.0f;
    envelopeTriggerArmed = true;
}

void Chambers::process(juce::AudioBuffer<float>& buffer)
{
    juce::ScopedNoDenormals noDenormals;
    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();

    const float outputScale = kOutputGain; // Normalizes constant-power output mix to unity.

    auto* left = buffer.getWritePointer(0);
    auto* right = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    std::array<float*, kNumLines> lineData{};
    for (size_t i = 0; i < kNumLines; ++i)
        lineData[i] = delayLines.getWritePointer(static_cast<int>(i));

    if (!smoothersPrimed)
    {
        timeSmoother.reset(timeTarget);
        massSmoother.reset(massTarget);
        densitySmoother.reset(densityTarget);
        gravitySmoother.reset(gravityTarget);
        bloomSmoother.reset(bloomTarget);
        smoothersPrimed = true;
    }

    const bool freezeActive = isFrozen;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        if (!freezeActive)
        {
            // Ramp normal processing back in after release to avoid clicks.
            if (freezeRampRemaining > 0)
            {
                freezeBlend = juce::jmin(1.0f, freezeBlend + freezeRampStep);
                --freezeRampRemaining;
            }
            else
            {
                freezeBlend = 1.0f;
            }
        }
        else
        {
            freezeBlend = 0.0f;
        }
        const bool holdState = freezeActive || (freezeRampRemaining > 0);

        // Per-sample smoothing avoids block-stepped automation artifacts and tail glitches.
        const float timeNorm = juce::jlimit(0.0f, 1.0f, timeSmoother.getNextValue());
        const float massNorm = juce::jlimit(0.0f, 1.0f, massSmoother.getNextValue());
        const float densityNorm = juce::jlimit(0.0f, 1.0f, densitySmoother.getNextValue());
        const float gravityNorm = juce::jlimit(0.0f, 1.0f, gravitySmoother.getNextValue());
        const float bloomNorm = juce::jlimit(0.0f, 1.0f, bloomSmoother.getNextValue());

        float feedbackBaseLocal = juce::jmap(timeNorm, 0.35f, 0.92f);
        if (feedbackBaseLocal > kMaxFeedback)
        {
#if JUCE_DEBUG
            static bool warned = false;
            if (!warned)
            {
                DBG("Chambers: feedback clamped for safety.");
                warned = true;
            }
#endif
            feedbackBaseLocal = kMaxFeedback;
        }
        const float dampingBaseLocal = juce::jmap(massNorm, 0.1f, 0.85f);

        for (size_t i = 0; i < kNumLines; ++i)
        {
            const float damping = juce::jlimit(0.0f, 0.98f, dampingBaseLocal + kDampingOffsets[i]);
            const float targetCoeff = 1.0f - damping;
            dampingCoefficients[i] = 1.0f + freezeBlend * (targetCoeff - 1.0f);
        }

        const float densityInputGain = juce::jmap(densityNorm, 0.18f, 0.32f);
        const float densityEarlyMix = juce::jmap(densityNorm, 0.45f, 0.25f);

        // Density drives diffusion strength; coefficients stay below 0.75 for stability.
        const float inputCoeff = juce::jmap(densityNorm, 0.12f, 0.6f);
        const float lateCoeffBase = juce::jmap(densityNorm, 0.18f, 0.7f);
        inputDiffusers[0].setCoefficient(inputCoeff);
        inputDiffusers[1].setCoefficient(inputCoeff);
        for (size_t i = 0; i < kNumLines; ++i)
        {
            const float coeff = lateCoeffBase * (1.0f + kLateDiffuserCoeffOffsets[i]);
            lateDiffusers[i].setCoefficient(juce::jlimit(0.05f, 0.74f, coeff));
        }

        const float feedbackLocal = freezeActive
            ? 1.0f
            : 1.0f + freezeBlend * (feedbackBaseLocal - 1.0f);
        const float inputGainLocal = densityInputGain;
        float earlyMixLocal = densityEarlyMix;
        if (freezeActive)
            earlyMixLocal = 0.0f;
        earlyMixLocal = juce::jlimit(0.0f, 0.7f, earlyMixLocal);

        const float inputScale = inputGainLocal * kInvSqrt8;
        const float gravityCoeff = juce::jlimit(
            0.0f, 1.0f, juce::jmap(gravityNorm, gravityCoeffMin, gravityCoeffMax));

        const float inL = left[sample];
        const float inR = right != nullptr ? right[sample] : inL;
        const float inputMagnitude = juce::jmax(std::abs(inL), std::abs(inR));

        if (!freezeActive)
        {
            if (inputMagnitude > envelopeResetThreshold && envelopeTriggerArmed)
            {
                envelopeTimeSeconds = 0.0f;
                envelopeValue = 1.0f;
                envelopeTriggerArmed = false;
            }
            else if (inputMagnitude <= envelopeResetThreshold)
            {
                envelopeTriggerArmed = true;
            }

            envelopeTimeSeconds += static_cast<float>(1.0 / sampleRateHz);
        }

        // Input diffusion is pre-FDN to build density without altering the feedback topology.
        float diffL = inL;
        float diffR = inR;
        if (!freezeActive)
        {
            const float processedL = inputDiffusers[0].processSample(inL);
            const float processedR = inputDiffusers[1].processSample(inR);
            diffL = inL + freezeBlend * (processedL - inL);
            diffR = inR + freezeBlend * (processedR - inR);
        }
        const float mid = 0.5f * (diffL + diffR);
        const float side = 0.5f * (diffL - diffR);

        float out[kNumLines];
        for (size_t i = 0; i < kNumLines; ++i)
        {
            const int readPos = holdState ? freezeWritePositions[i] : writePositions[i];
            out[i] = readFractionalDelay(lineData[i], delayBufferLength, readPos, delaySamples[i]);
        }

        float feedback[kNumLines];
        for (size_t i = 0; i < kNumLines; ++i)
            feedback[i] = out[i];
        if (!freezeActive)
            hadamard8(feedback);

        float lateOut[kNumLines];
        for (size_t i = 0; i < kNumLines; ++i)
        {
            // Late diffusion is post-read and pre-output mix to increase density
            // without placing allpass recursion inside the feedback loop.
            if (freezeActive)
            {
                lateOut[i] = out[i];
            }
            else
            {
                const float processed = lateDiffusers[i].processSample(out[i]);
                lateOut[i] = out[i] + freezeBlend * (processed - out[i]);
            }
        }

        if (!freezeActive)
        {
            // Bloom shapes the late-field envelope by blending exponential decay with a plateau.
            const float envelopeTime = envelopeTimeSeconds;
            const float decayTimeSeconds = juce::jmap(
                timeNorm, kEnvelopeMinTimeSeconds, kEnvelopeMaxTimeSeconds);
            const float expEnv = std::exp(-envelopeTime / decayTimeSeconds);
            const float plateauFraction = 0.25f + 0.35f * bloomNorm;
            const float plateauTime = decayTimeSeconds * plateauFraction;
            const float plateauEnv = envelopeTime < plateauTime
                ? 1.0f
                : std::exp(-(envelopeTime - plateauTime) / decayTimeSeconds);
            const float bloomGain = 1.0f + kBloomPeakGain * (bloomNorm * bloomNorm);
            const float targetEnvelope = expEnv + bloomNorm * ((plateauEnv * bloomGain) - expEnv);
            envelopeValue = juce::jlimit(0.0f, 1.5f, targetEnvelope);
        }
        else
        {
            envelopeValue = 1.0f;
        }

        const float envelopeApplied = freezeActive ? 1.0f : (1.0f + freezeBlend * (envelopeValue - 1.0f));

        float wetL = 0.0f;
        float wetR = 0.0f;
        for (size_t i = 0; i < kNumLines; ++i)
        {
            wetL += lateOut[i] * kOutputLeft[i];
            wetR += lateOut[i] * kOutputRight[i];
        }
        wetL *= outputScale * envelopeApplied;
        wetR *= outputScale * envelopeApplied;
        wetL = juce::jlimit(-kWetLimiterCeiling, kWetLimiterCeiling, wetL);
        wetR = juce::jlimit(-kWetLimiterCeiling, kWetLimiterCeiling, wetR);

        for (size_t i = 0; i < kNumLines; ++i)
        {
            const float injection = holdState
                ? 0.0f
                : (mid * kInputMid[i] + side * kInputSide[i]) * inputScale;
            const float writeValue = injection + feedback[i] * feedbackLocal;
            const int writePos = holdState ? freezeWritePositions[i] : writePositions[i];
            if (holdState)
            {
                // Freeze/ramp hold: keep stored state fixed and clamp for safety.
                lineData[i][writePos] = freezeHardLimit(lineData[i][writePos]);
            }
            else
            {
                const float damped = lowpassState[i] + dampingCoefficients[i] * (writeValue - lowpassState[i]);
                lowpassState[i] = damped;
                // Gravity is a low-end containment high-pass inside the loop, after HF damping.
                const float gravityLow = gravityLowpassState[i]
                    + (1.0f - gravityCoeff) * (damped - gravityLowpassState[i]);
                gravityLowpassState[i] = gravityLow;
                const float gravityOut = damped - gravityLow;
                const float writeSample = damped + freezeBlend * (gravityOut - damped);
                lineData[i][writePos] = writeSample;
            }

            if (!holdState)
            {
                ++writePositions[i];
                if (writePositions[i] >= delayBufferLength)
                    writePositions[i] = 0;
            }
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
    timeTarget = juce::jlimit(0.0f, 1.0f, time);
    timeSmoother.setTarget(timeTarget);
}

void Chambers::setMass(float mass)
{
    massTarget = juce::jlimit(0.0f, 1.0f, mass);
    massSmoother.setTarget(massTarget);
}

void Chambers::setDensity(float density)
{
    densityTarget = juce::jlimit(0.0f, 1.0f, density);
    densitySmoother.setTarget(densityTarget);
}

void Chambers::setBloom(float bloom)
{
    bloomTarget = juce::jlimit(0.0f, 1.0f, bloom);
    bloomSmoother.setTarget(bloomTarget);
}

void Chambers::setGravity(float gravity)
{
    gravityTarget = juce::jlimit(0.0f, 1.0f, gravity);
    gravitySmoother.setTarget(gravityTarget);
}

void Chambers::setFreeze(bool shouldFreeze)
{
    if (shouldFreeze && !isFrozen)
    {
        freezeWritePositions = writePositions;
        isFrozen = true;
        freezeBlend = 0.0f;
        freezeRampRemaining = 0;
    }
    else if (!shouldFreeze && isFrozen)
    {
        isFrozen = false;
        freezeBlend = 0.0f;
        freezeRampRemaining = freezeRampSamples;
        envelopeTimeSeconds = 0.0f;
        envelopeValue = 1.0f;
        envelopeTriggerArmed = true;
    }

    wasFrozen = isFrozen;
}

} // namespace dsp
} // namespace monument
