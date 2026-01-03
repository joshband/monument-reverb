#include "Chambers.h"

#include <cmath>

namespace
{
constexpr float kInvSqrt8 = 0.3535533905932738f;
using Matrix8 = std::array<std::array<float, 8>, 8>;

constexpr float kHouseholderDiag = 0.75f;
constexpr float kHouseholderOff = -0.25f;

constexpr Matrix8 kMatrixHadamard{{
    {{ kInvSqrt8,  kInvSqrt8,  kInvSqrt8,  kInvSqrt8,  kInvSqrt8,  kInvSqrt8,  kInvSqrt8,  kInvSqrt8 }},
    {{ kInvSqrt8, -kInvSqrt8,  kInvSqrt8, -kInvSqrt8,  kInvSqrt8, -kInvSqrt8,  kInvSqrt8, -kInvSqrt8 }},
    {{ kInvSqrt8,  kInvSqrt8, -kInvSqrt8, -kInvSqrt8,  kInvSqrt8,  kInvSqrt8, -kInvSqrt8, -kInvSqrt8 }},
    {{ kInvSqrt8, -kInvSqrt8, -kInvSqrt8,  kInvSqrt8,  kInvSqrt8, -kInvSqrt8, -kInvSqrt8,  kInvSqrt8 }},
    {{ kInvSqrt8,  kInvSqrt8,  kInvSqrt8,  kInvSqrt8, -kInvSqrt8, -kInvSqrt8, -kInvSqrt8, -kInvSqrt8 }},
    {{ kInvSqrt8, -kInvSqrt8,  kInvSqrt8, -kInvSqrt8, -kInvSqrt8,  kInvSqrt8, -kInvSqrt8,  kInvSqrt8 }},
    {{ kInvSqrt8,  kInvSqrt8, -kInvSqrt8, -kInvSqrt8, -kInvSqrt8, -kInvSqrt8,  kInvSqrt8,  kInvSqrt8 }},
    {{ kInvSqrt8, -kInvSqrt8, -kInvSqrt8,  kInvSqrt8, -kInvSqrt8,  kInvSqrt8,  kInvSqrt8, -kInvSqrt8 }}
}};

constexpr Matrix8 kMatrixHouseholder{{
    {{ kHouseholderDiag, kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff }},
    {{ kHouseholderOff,  kHouseholderDiag, kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff }},
    {{ kHouseholderOff,  kHouseholderOff,  kHouseholderDiag, kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff }},
    {{ kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderDiag, kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff }},
    {{ kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderDiag, kHouseholderOff,  kHouseholderOff,  kHouseholderOff }},
    {{ kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderDiag, kHouseholderOff,  kHouseholderOff }},
    {{ kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderDiag, kHouseholderOff }},
    {{ kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderDiag }}
}};

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

// Freeze crossfades are kept long enough to avoid clicks and level jumps.
constexpr float kFreezeReleaseMs = 100.0f;
constexpr float kFreezeOutputFadeMs = 100.0f;
constexpr float kFreezeLimiterCeiling = 0.9f;
constexpr float kWetLimiterCeiling = 0.95f;
// Feedback coefficient is clamped below unity for stability (long tails without runaway).
constexpr float kMaxFeedback = 0.995f;
constexpr float kMinFeedback = 0.35f;
constexpr float kBloomSmoothingMs = 40.0f;
constexpr float kWarpSmoothingMs = 1200.0f;
constexpr float kDriftSmoothingMs = 1500.0f;
constexpr float kDriftRateMinHz = 0.05f;
constexpr float kDriftRateMaxHz = 0.2f;
// Drift depth stays within +/-1.0 sample to avoid audible pitch wobble.
constexpr float kDriftDepthMaxSamples = 1.0f;
constexpr float kEnvelopeMinTimeSeconds = 1.0f;
constexpr float kEnvelopeMaxTimeSeconds = 12.0f;
constexpr float kBloomPeakGain = 0.5f; // Up to 1.5x at Bloom=1.
constexpr float kWarpMatrixEpsilon = 1.0e-4f;
constexpr float kMatrixNormEpsilon = 1.0e-6f;
constexpr float kMemoryInjectionGain = 1.8f;
constexpr float kMemoryEnvelopeTriggerScale = 1.5f;

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

inline float sanitizeNormalizedParameter(float value, float fallback, const char* label, bool& warned)
{
    juce::ignoreUnused(label, warned);
    if (!std::isfinite(value))
    {
#if JUCE_DEBUG
        if (!warned)
        {
            DBG("Chambers: non-finite " + juce::String(label) + " parameter ignored.");
            warned = true;
        }
#endif
        return fallback;
    }
    if (value < 0.0f || value > 1.0f)
    {
#if JUCE_DEBUG
        if (!warned)
        {
            DBG("Chambers: " + juce::String(label) + " parameter clamped.");
            warned = true;
        }
#endif
        return juce::jlimit(0.0f, 1.0f, value);
    }
    return value;
}

inline void blendMatrices(const Matrix8& a, const Matrix8& b, float blend, Matrix8& dest)
{
    const float invBlend = 1.0f - blend;
    for (size_t row = 0; row < 8; ++row)
        for (size_t col = 0; col < 8; ++col)
            dest[row][col] = invBlend * a[row][col] + blend * b[row][col];
}

inline void normalizeColumns(Matrix8& matrix)
{
    for (size_t col = 0; col < 8; ++col)
    {
        float norm = 0.0f;
        for (size_t row = 0; row < 8; ++row)
            norm += matrix[row][col] * matrix[row][col];
        if (norm > kMatrixNormEpsilon)
        {
            const float invNorm = 1.0f / std::sqrt(norm);
            for (size_t row = 0; row < 8; ++row)
                matrix[row][col] *= invNorm;
        }
    }
}

inline void computeWarpMatrix(float warp, Matrix8& dest)
{
    // Warp morphs between orthogonal feedback topologies while keeping column energy stable.
    blendMatrices(kMatrixHadamard, kMatrixHouseholder, warp, dest);
    normalizeColumns(dest);
}

inline void applyMatrix(const Matrix8& matrix, const float* input, float* output)
{
    for (size_t row = 0; row < 8; ++row)
    {
        float sum = 0.0f;
        for (size_t col = 0; col < 8; ++col)
            sum += matrix[row][col] * input[col];
        output[row] = sum;
    }
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
    float delaySum = 0.0f;

    for (size_t i = 0; i < kNumLines; ++i)
    {
        delaySamples[i] = juce::jmax(1.0f, static_cast<float>(kDelaySamples48k[i]) * scale);
        maxDelay = juce::jmax(maxDelay, delaySamples[i]);
        delaySum += delaySamples[i];
    }

    delayBufferLength = juce::jmax(1, static_cast<int>(std::ceil(maxDelay)) + 2);
    meanDelaySeconds = (delaySum / static_cast<float>(kNumLines))
        / static_cast<float>(sampleRateHz);
    delayLines.setSize(kNumLines, delayBufferLength);
    delayLines.clear();
    writePositions.fill(0);
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

    driftDepthMaxSamples = kDriftDepthMaxSamples;
    {
        juce::Random random;
        for (size_t i = 0; i < kNumLines; ++i)
        {
            driftRateHz[i] = juce::jmap(random.nextFloat(), kDriftRateMinHz, kDriftRateMaxHz);
            driftPhase[i] = random.nextFloat() * juce::MathConstants<float>::twoPi;
        }
    }

    gravityCoeffMin = onePoleCoeffFromHz(kGravityCutoffMinHz, sampleRateHz);
    gravityCoeffMax = onePoleCoeffFromHz(kGravityCutoffMaxHz, sampleRateHz);
    freezeRampSamples = juce::jmax(
        1, static_cast<int>(std::round(sampleRateHz * (kFreezeReleaseMs / 1000.0f))));
    freezeOutputFadeSamples = juce::jmax(
        1, static_cast<int>(std::round(sampleRateHz * (kFreezeOutputFadeMs / 1000.0f))));
    freezeRampStep = 1.0f / static_cast<float>(freezeRampSamples);
    freezeRampRemaining = 0;
    freezeBlend = 1.0f;
    freezeRampingDown = false;
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

    warpSmoother.prepare(sampleRateHz);
    warpSmoother.setSmoothingTimeMs(kWarpSmoothingMs); // Warp is intentionally slow to avoid motion artifacts.
    warpSmoother.setTarget(warpTarget);

    driftSmoother.prepare(sampleRateHz);
    driftSmoother.setSmoothingTimeMs(kDriftSmoothingMs); // Drift stays gentle and motion-safe.
    driftSmoother.setTarget(driftTarget);

    warpSmoothed = warpTarget;
    computeWarpMatrix(warpSmoothed, warpMatrix);
    warpMatrixFrozen = warpMatrix;
    feedbackMatrix = warpMatrix;
    lastMatrixBlend = 1.0f;

    smoothersPrimed = false;
    envelopeTimeSeconds = 0.0f;
    envelopeValue = 1.0f;
    envelopeTriggerArmed = true;
}

void Chambers::reset()
{
    delayLines.clear();
    writePositions.fill(0);
    lowpassState.fill(0.0f);
    gravityLowpassState.fill(0.0f);
    for (auto& diffuser : inputDiffusers)
        diffuser.reset();
    for (auto& diffuser : lateDiffusers)
        diffuser.reset();
    smoothersPrimed = false;
    freezeRampRemaining = 0;
    freezeBlend = 1.0f;
    freezeRampingDown = false;
    isFrozen = false;
    wasFrozen = false;
    externalInjection = nullptr;
    envelopeTimeSeconds = 0.0f;
    envelopeValue = 1.0f;
    envelopeTriggerArmed = true;
    warpSmoothed = warpTarget;
    computeWarpMatrix(warpSmoothed, warpMatrix);
    warpMatrixFrozen = warpMatrix;
    feedbackMatrix = warpMatrix;
    lastMatrixBlend = 1.0f;
}

void Chambers::process(juce::AudioBuffer<float>& buffer)
{
    juce::ScopedNoDenormals noDenormals;
    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();

    const float outputScale = kOutputGain; // Normalizes constant-power output mix to unity.

    auto* left = buffer.getWritePointer(0);
    auto* right = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;
    const auto* injectionBuffer = externalInjection;
    const bool hasExternalInjection = injectionBuffer != nullptr
        && injectionBuffer->getNumChannels() >= 2
        && injectionBuffer->getNumSamples() >= numSamples;
    const auto* injectionL = hasExternalInjection ? injectionBuffer->getReadPointer(0) : nullptr;
    const auto* injectionR = hasExternalInjection ? injectionBuffer->getReadPointer(1) : nullptr;

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
        warpSmoother.reset(warpTarget);
        driftSmoother.reset(driftTarget);
        warpSmoothed = warpTarget;
        computeWarpMatrix(warpSmoothed, warpMatrix);
        warpMatrixFrozen = warpMatrix;
        feedbackMatrix = warpMatrix;
        lastMatrixBlend = 1.0f;
        smoothersPrimed = true;
    }

    const bool freezeActive = isFrozen;
    const float driftPhaseStep = juce::MathConstants<float>::twoPi
        / static_cast<float>(sampleRateHz);
    if (freezeActive && !wasFrozen)
    {
        // Capture the active topology so Freeze holds the current spatial mapping.
        warpMatrixFrozen = feedbackMatrix;
        lastMatrixBlend = 0.0f;
    }

    for (int sample = 0; sample < numSamples; ++sample)
    {
        if (freezeRampRemaining > 0)
        {
            if (freezeRampingDown)
                freezeBlend = juce::jmax(0.0f, freezeBlend - freezeRampStep);
            else
                freezeBlend = juce::jmin(1.0f, freezeBlend + freezeRampStep);
            --freezeRampRemaining;
        }
        else
        {
            freezeBlend = freezeActive ? 0.0f : 1.0f;
        }
        bool warpMatrixDirty = false;
        if (!freezeActive)
        {
            const float warpNext = juce::jlimit(0.0f, 1.0f, warpSmoother.getNextValue());
            if (std::abs(warpNext - warpSmoothed) > kWarpMatrixEpsilon)
            {
                warpSmoothed = warpNext;
                computeWarpMatrix(warpSmoothed, warpMatrix);
                warpMatrixDirty = true;
            }
        }

        const float matrixBlend = freezeActive ? 0.0f : freezeBlend;
        if (freezeActive)
        {
            if (lastMatrixBlend != 0.0f)
            {
                feedbackMatrix = warpMatrixFrozen;
                lastMatrixBlend = 0.0f;
            }
        }
        else if (warpMatrixDirty || std::abs(matrixBlend - lastMatrixBlend) > kWarpMatrixEpsilon)
        {
            if (matrixBlend < 1.0f - kWarpMatrixEpsilon)
            {
                // Crossfade topologies during freeze release to avoid spatial jumps.
                blendMatrices(warpMatrixFrozen, warpMatrix, matrixBlend, feedbackMatrix);
                normalizeColumns(feedbackMatrix);
            }
            else
            {
                feedbackMatrix = warpMatrix;
            }
            lastMatrixBlend = matrixBlend;
        }

        // Per-sample smoothing avoids block-stepped automation artifacts and tail glitches.
        const float timeNorm = juce::jlimit(0.0f, 1.0f, timeSmoother.getNextValue());
        const float massNorm = juce::jlimit(0.0f, 1.0f, massSmoother.getNextValue());
        const float densityNorm = juce::jlimit(0.0f, 1.0f, densitySmoother.getNextValue());
        const float gravityNorm = juce::jlimit(0.0f, 1.0f, gravitySmoother.getNextValue());
        const float bloomNorm = juce::jlimit(0.0f, 1.0f, bloomSmoother.getNextValue());
        // Drift subtly modulates delay lengths; depth ramps with freezeBlend and phases pause on freeze/ramp.
        const float driftNorm = juce::jlimit(0.0f, 1.0f, driftSmoother.getNextValue());
        const float driftDepthBase = driftNorm * driftDepthMaxSamples;
        const float driftDepth = freezeActive ? 0.0f : (driftDepthBase * freezeBlend);
        const bool advanceDrift = !freezeActive && (freezeRampRemaining == 0);

        // Time maps directly to feedback coefficient for long-tail control.
        float feedbackBaseLocal = juce::jmap(timeNorm, kMinFeedback, kMaxFeedback);
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
        // Mass darkens the tail by increasing HF damping up to 0.95.
        const float dampingBaseLocal = juce::jmap(massNorm, 0.1f, 0.95f);

        for (size_t i = 0; i < kNumLines; ++i)
        {
            const float damping = juce::jlimit(0.0f, 0.98f, dampingBaseLocal + kDampingOffsets[i]);
            const float targetCoeff = 1.0f - damping;
            dampingCoefficients[i] = 1.0f + freezeBlend * (targetCoeff - 1.0f);
        }

        // Density extends down to 0.05 for grainier, sparser ambience.
        const float densityShaped = juce::jmap(densityNorm, 0.05f, 1.0f);
        const float densityInputGain = juce::jmap(densityShaped, 0.18f, 0.32f);
        const float densityEarlyMix = juce::jmap(densityShaped, 0.45f, 0.25f);

        // Density drives diffusion strength; coefficients stay below 0.75 for stability.
        const float inputCoeff = juce::jmap(densityShaped, 0.12f, 0.6f);
        const float lateCoeffBase = juce::jmap(densityShaped, 0.18f, 0.7f);
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
        const float earlyMixLocal = juce::jlimit(0.0f, 0.7f, densityEarlyMix * freezeBlend);

        const float inputScale = inputGainLocal * kInvSqrt8;
        const float gravityCoeff = juce::jlimit(
            0.0f, 1.0f, juce::jmap(gravityNorm, gravityCoeffMin, gravityCoeffMax));

        const float inL = left[sample];
        const float inR = right != nullptr ? right[sample] : inL;
        const float inputMagnitude = juce::jmax(std::abs(inL), std::abs(inR));
        const float memoryMagnitude = hasExternalInjection
            ? juce::jmax(std::abs(injectionL[sample]), std::abs(injectionR[sample]))
            : 0.0f;
        const float envelopeInputMagnitude = juce::jmax(
            inputMagnitude, memoryMagnitude * kMemoryEnvelopeTriggerScale);

        if (!freezeActive)
        {
            if (envelopeInputMagnitude > envelopeResetThreshold && envelopeTriggerArmed)
            {
                envelopeTimeSeconds = 0.0f;
                envelopeValue = 1.0f;
                envelopeTriggerArmed = false;
            }
            else if (envelopeInputMagnitude <= envelopeResetThreshold)
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
        float memoryMid = 0.0f;
        float memorySide = 0.0f;
        if (hasExternalInjection)
        {
            float memL = injectionL[sample];
            float memR = injectionR[sample];
            if (!std::isfinite(memL) || !std::isfinite(memR))
            {
                memL = 0.0f;
                memR = 0.0f;
            }
            memoryMid = 0.5f * (memL + memR);
            memorySide = 0.5f * (memL - memR);
        }

        const float outputBlend = freezeBlend;
        float outLive[kNumLines];
        float outFrozen[kNumLines];
        for (size_t i = 0; i < kNumLines; ++i)
        {
            const int readPos = writePositions[i];
            if (advanceDrift)
            {
                driftPhase[i] += driftRateHz[i] * driftPhaseStep;
                if (driftPhase[i] >= juce::MathConstants<float>::twoPi)
                    driftPhase[i] -= juce::MathConstants<float>::twoPi;
            }
            const float modOffset = driftDepth != 0.0f
                ? std::sin(driftPhase[i]) * driftDepth
                : 0.0f;
            const float driftedDelay = juce::jmax(1.0f, delaySamples[i] + modOffset);
            outLive[i] = readFractionalDelay(lineData[i], delayBufferLength, readPos, driftedDelay);
            if (driftDepth == 0.0f)
            {
                outFrozen[i] = outLive[i];
            }
            else
            {
                outFrozen[i] = readFractionalDelay(lineData[i], delayBufferLength, readPos, delaySamples[i]);
            }
        }

        float feedback[kNumLines];
        applyMatrix(feedbackMatrix, outLive, feedback);

        float lateOutLive[kNumLines];
        for (size_t i = 0; i < kNumLines; ++i)
        {
            // Late diffusion is post-read and pre-output mix to increase density
            // without placing allpass recursion inside the feedback loop.
            const float processed = lateDiffusers[i].processSample(outLive[i]);
            lateOutLive[i] = outLive[i] + freezeBlend * (processed - outLive[i]);
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

        float wetLiveL = 0.0f;
        float wetLiveR = 0.0f;
        float wetFrozenL = 0.0f;
        float wetFrozenR = 0.0f;
        for (size_t i = 0; i < kNumLines; ++i)
        {
            wetLiveL += lateOutLive[i] * kOutputLeft[i];
            wetLiveR += lateOutLive[i] * kOutputRight[i];
            wetFrozenL += outFrozen[i] * kOutputLeft[i];
            wetFrozenR += outFrozen[i] * kOutputRight[i];
        }
        wetLiveL *= outputScale * envelopeValue;
        wetLiveR *= outputScale * envelopeValue;
        // Preserve the captured Bloom envelope during freeze crossfades.
        wetFrozenL *= outputScale * envelopeValue;
        wetFrozenR *= outputScale * envelopeValue;

        float wetL = outputBlend * wetLiveL + (1.0f - outputBlend) * wetFrozenL;
        float wetR = outputBlend * wetLiveR + (1.0f - outputBlend) * wetFrozenR;
        wetL = juce::jlimit(-kWetLimiterCeiling, kWetLimiterCeiling, wetL);
        wetR = juce::jlimit(-kWetLimiterCeiling, kWetLimiterCeiling, wetR);

        for (size_t i = 0; i < kNumLines; ++i)
        {
            const float injection = (mid * kInputMid[i] + side * kInputSide[i])
                * inputScale * freezeBlend;
            const float memoryInjection = hasExternalInjection
                ? (memoryMid * kInputMid[i] + memorySide * kInputSide[i])
                    * kInvSqrt8 * kMemoryInjectionGain * freezeBlend
                : 0.0f;
            const float writeValue = injection + memoryInjection + feedback[i] * feedbackLocal;
            const int writePos = writePositions[i];
            const float damped = lowpassState[i] + dampingCoefficients[i] * (writeValue - lowpassState[i]);
            lowpassState[i] = damped;
            // Gravity is a low-end containment high-pass inside the loop, after HF damping.
            const float gravityLow = gravityLowpassState[i]
                + (1.0f - gravityCoeff) * (damped - gravityLowpassState[i]);
            gravityLowpassState[i] = gravityLow;
            const float gravityOut = damped - gravityLow;
            const float writeSample = damped + freezeBlend * (gravityOut - damped);
            lineData[i][writePos] = freezeActive ? freezeHardLimit(writeSample) : writeSample;

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

    externalInjection = nullptr;
    wasFrozen = freezeActive;
}

void Chambers::setExternalInjection(const juce::AudioBuffer<float>* injectionBuffer)
{
    externalInjection = injectionBuffer;
}

void Chambers::setTime(float time)
{
    static bool warned = false;
    timeTarget = sanitizeNormalizedParameter(time, timeTarget, "time", warned);
    timeSmoother.setTarget(timeTarget);
}

void Chambers::setMass(float mass)
{
    static bool warned = false;
    massTarget = sanitizeNormalizedParameter(mass, massTarget, "mass", warned);
    massSmoother.setTarget(massTarget);
}

void Chambers::setDensity(float density)
{
    static bool warned = false;
    densityTarget = sanitizeNormalizedParameter(density, densityTarget, "density", warned);
    densitySmoother.setTarget(densityTarget);
}

void Chambers::setBloom(float bloom)
{
    static bool warned = false;
    bloomTarget = sanitizeNormalizedParameter(bloom, bloomTarget, "bloom", warned);
    bloomSmoother.setTarget(bloomTarget);
}

void Chambers::setGravity(float gravity)
{
    static bool warned = false;
    gravityTarget = sanitizeNormalizedParameter(gravity, gravityTarget, "gravity", warned);
    gravitySmoother.setTarget(gravityTarget);
}

void Chambers::setWarp(float warp)
{
    // Warp is clamped to [0, 1] and the blended matrix is re-normalized per column for stability.
    static bool warned = false;
    warpTarget = sanitizeNormalizedParameter(warp, warpTarget, "warp", warned);
    warpSmoother.setTarget(warpTarget);
}

void Chambers::setDrift(float drift)
{
    static bool warned = false;
    driftTarget = sanitizeNormalizedParameter(drift, driftTarget, "drift", warned);
    driftSmoother.setTarget(driftTarget);
}

void Chambers::setFreeze(bool shouldFreeze)
{
    if (shouldFreeze && !isFrozen)
    {
        isFrozen = true;
        freezeRampingDown = true;
        freezeRampRemaining = juce::jmax(1, freezeOutputFadeSamples);
        freezeRampStep = 1.0f / static_cast<float>(freezeRampRemaining);
    }
    else if (!shouldFreeze && isFrozen)
    {
        isFrozen = false;
        freezeRampingDown = false;
        freezeRampRemaining = juce::jmax(1, freezeRampSamples);
        freezeRampStep = 1.0f / static_cast<float>(freezeRampRemaining);
    }

}

} // namespace dsp
} // namespace monument
