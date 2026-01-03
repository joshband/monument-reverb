#include "MemoryEchoes.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float kShortMemorySeconds = 24.0f;
constexpr float kLongMemorySeconds = 180.0f;
constexpr float kShortTargetDecayDb = -18.0f;
constexpr float kLongTargetDecayDb = -45.0f;
constexpr float kShortCaptureGain = 0.35f;
constexpr float kLongCaptureGain = 0.002f;
constexpr float kFreezeCaptureScale = 0.1f;
constexpr float kMemoryEpsilon = 1.0e-4f;
constexpr float kCaptureRmsTimeMs = 250.0f;

constexpr float kMemorySmoothingMs = 300.0f;
constexpr float kDepthSmoothingMs = 300.0f;
constexpr float kDecaySmoothingMs = 450.0f;
constexpr float kDriftSmoothingMs = 450.0f;

constexpr float kSurfaceQuietThreshold = 0.03f;
constexpr float kSurfaceIntervalMinSeconds = 6.0f;
constexpr float kSurfaceIntervalMaxSeconds = 18.0f;
constexpr float kSurfaceCooldownMinSeconds = 2.0f;
constexpr float kSurfaceCooldownMaxSeconds = 6.0f;

constexpr float kSurfaceWidthMinMs = 200.0f;
constexpr float kSurfaceWidthMaxMs = 800.0f;
constexpr float kSurfaceWidthLongMinMs = 350.0f;
constexpr float kSurfaceWidthLongMaxMs = 900.0f;

constexpr float kSurfaceFadeMinSeconds = 1.0f;
constexpr float kSurfaceFadeMaxSeconds = 3.0f;
constexpr float kSurfaceHoldMinSeconds = 0.5f;
constexpr float kSurfaceHoldMaxSeconds = 2.0f;

constexpr float kSurfaceTargetPeakShort = 0.012f;
constexpr float kSurfaceTargetPeakLong = 0.008f;
constexpr float kSurfaceProbeMin = 0.0015f;
constexpr float kSurfaceGainMax = 0.25f;

constexpr float kLowpassMaxHz = 12000.0f;
constexpr float kLowpassMinHz = 2500.0f;
constexpr float kSaturationDriveMax = 1.6f;
constexpr float kAgeGainReductionMax = 0.35f;

constexpr float kDriftCentsMax = 15.0f;
constexpr float kDriftUpdateMs = 140.0f;
constexpr float kDriftSlewMs = 200.0f;

#if defined(MONUMENT_TESTING)
constexpr float kTestSurfaceRateScale = 3.0f;
#endif

inline float coeffFromMs(float timeMs, double sampleRate)
{
    const double timeSeconds = static_cast<double>(timeMs) / 1000.0;
    return static_cast<float>(std::exp(-1.0 / (timeSeconds * sampleRate)));
}

inline float coeffFromHz(float cutoffHz, double sampleRate)
{
    const double omega = 2.0 * juce::MathConstants<double>::pi
        * static_cast<double>(cutoffHz) / sampleRate;
    return static_cast<float>(1.0 - std::exp(-omega));
}

inline float forgetFactorFromDb(float targetDb, float durationSeconds, double sampleRate)
{
    const double linear = std::pow(10.0, static_cast<double>(targetDb) / 20.0);
    return static_cast<float>(std::pow(linear, 1.0 / (durationSeconds * sampleRate)));
}

inline float clampFinite(float value)
{
    return std::isfinite(value) ? value : 0.0f;
}

inline float randomRange(juce::Random& random, float minValue, float maxValue)
{
    return minValue + (maxValue - minValue) * random.nextFloat();
}

inline int secondsToSamples(float seconds, double sampleRate)
{
    return juce::jmax(1, static_cast<int>(std::round(seconds * sampleRate)));
}
} // namespace

namespace monument
{
namespace dsp
{
void MemoryEchoes::prepare(double sampleRate, int blockSize, int numChannels)
{
    sampleRateHz = sampleRate;
    maxBlockSize = blockSize;
    channels = numChannels;

    shortLengthSamples = juce::jmax(1, static_cast<int>(std::round(sampleRateHz * kShortMemorySeconds)));
    longLengthSamples = juce::jmax(1, static_cast<int>(std::round(sampleRateHz * kLongMemorySeconds)));

    constexpr int shortChannels = 2;
    constexpr int longChannels = 1;
    shortBuffer.setSize(shortChannels, shortLengthSamples, false, false, true);
    shortBuffer.clear();
    longBuffer.setSize(longChannels, longLengthSamples, false, false, true);
    longBuffer.clear();
    recallBuffer.setSize(2, maxBlockSize, false, false, true);
    recallBuffer.clear();

    shortWritePos = 0;
    longWritePos = 0;
    shortFilledSamples = 0;
    longFilledSamples = 0;

    shortForgetFactor = forgetFactorFromDb(kShortTargetDecayDb, kShortMemorySeconds, sampleRateHz);
    longForgetFactor = forgetFactorFromDb(kLongTargetDecayDb, kLongMemorySeconds, sampleRateHz);
    shortCaptureGain = kShortCaptureGain;
    longCaptureGain = kLongCaptureGain;
    captureRmsCoeff = coeffFromMs(kCaptureRmsTimeMs, sampleRateHz);
    lastCaptureRms = 0.0f;

    memorySmoother.prepare(sampleRateHz);
    memorySmoother.setSmoothingTimeMs(kMemorySmoothingMs);
    memorySmoother.setTarget(memoryTarget);

    depthSmoother.prepare(sampleRateHz);
    depthSmoother.setSmoothingTimeMs(kDepthSmoothingMs);
    depthSmoother.setTarget(depthTarget);

    decaySmoother.prepare(sampleRateHz);
    decaySmoother.setSmoothingTimeMs(kDecaySmoothingMs);
    decaySmoother.setTarget(decayTarget);

    driftSmoother.prepare(sampleRateHz);
    driftSmoother.setSmoothingTimeMs(kDriftSmoothingMs);
    driftSmoother.setTarget(driftTarget);

    driftSlewCoeff = coeffFromMs(kDriftSlewMs, sampleRateHz);
    driftUpdateSamples = juce::jmax(1,
        static_cast<int>(std::round(sampleRateHz * (kDriftUpdateMs / 1000.0f))));
    driftUpdateRemaining = driftUpdateSamples;

    reset();
}

void MemoryEchoes::reset()
{
    shortBuffer.clear();
    longBuffer.clear();
    recallBuffer.clear();

    shortWritePos = 0;
    longWritePos = 0;
    shortFilledSamples = 0;
    longFilledSamples = 0;
    lastCaptureRms = 0.0f;

    memoryEnabled = false;
    memoryAmountForCapture = 0.0f;
    freezeEnabled = false;

    surfaceState = SurfaceState::Idle;
    surfaceUsesLong = false;
    surfaceCenterPos = 0.0f;
    surfaceWidthSamples = 0;
    surfaceTotalSamples = 0;
    surfaceSamplesProcessed = 0;
    surfaceBaseGain = 0.0f;
    surfaceFadeInSamples = 0;
    surfaceHoldSamples = 0;
    surfaceFadeOutSamples = 0;
    surfaceSamplesRemaining = 0;
    surfaceGain = 0.0f;
    surfaceGainStep = 0.0f;
    surfaceCooldownSamples = 0;
    surfacePlaybackPos = 0.0f;
    surfacePlaybackStep = 0.0f;
    surfaceLowpassStateL = 0.0f;
    surfaceLowpassStateR = 0.0f;
    surfaceDriftCents = 0.0f;
    surfaceDriftTarget = 0.0f;
    surfaceDriftCentsMax = 0.0f;
    driftUpdateRemaining = driftUpdateSamples;

    smoothersPrimed = false;
    chambersInputGain = 0.25f;
}

void MemoryEchoes::process(juce::AudioBuffer<float>& buffer)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();
    if (numSamples == 0 || numChannels == 0)
        return;

    if (!smoothersPrimed)
    {
        memorySmoother.reset(memoryTarget);
        depthSmoother.reset(depthTarget);
        decaySmoother.reset(decayTarget);
        driftSmoother.reset(driftTarget);
        smoothersPrimed = true;
    }

    const bool recallReady = recallBuffer.getNumSamples() >= numSamples
        && recallBuffer.getNumChannels() >= 2;
    float* recallL = recallReady ? recallBuffer.getWritePointer(0) : nullptr;
    float* recallR = recallReady ? recallBuffer.getWritePointer(1) : nullptr;
    if (recallReady)
    {
        std::fill_n(recallL, numSamples, 0.0f);
        std::fill_n(recallR, numSamples, 0.0f);
    }

    auto* left = buffer.getWritePointer(0);
    auto* right = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    if (surfaceCooldownSamples > 0)
        surfaceCooldownSamples = juce::jmax(0, surfaceCooldownSamples - numSamples);

    float memoryAmount = juce::jlimit(0.0f, 1.0f, memorySmoother.getNextValue());
    float depth = juce::jlimit(0.0f, 1.0f, depthSmoother.getNextValue());
    float decayAmount = juce::jlimit(0.0f, 1.0f, decaySmoother.getNextValue());
    float driftAmount = juce::jlimit(0.0f, 1.0f, driftSmoother.getNextValue());

    if (!freezeEnabled)
        maybeStartSurface(numSamples, memoryAmount, depth, decayAmount, driftAmount);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        if (sample > 0)
        {
            memoryAmount = juce::jlimit(0.0f, 1.0f, memorySmoother.getNextValue());
            depth = juce::jlimit(0.0f, 1.0f, depthSmoother.getNextValue());
            decayAmount = juce::jlimit(0.0f, 1.0f, decaySmoother.getNextValue());
            driftAmount = juce::jlimit(0.0f, 1.0f, driftSmoother.getNextValue());
        }

        lastMemoryAmount = memoryAmount;
        lastDepthAmount = depth;
        lastDecayAmount = decayAmount;
        lastDriftAmount = driftAmount;
        memoryAmountForCapture = memoryAmount;
        memoryEnabled = memoryAmount > kMemoryEpsilon;

        float outL = 0.0f;
        float outR = 0.0f;

        if (memoryEnabled && surfaceState != SurfaceState::Idle && !freezeEnabled)
        {
            float age = 0.0f;
            float sampleL = 0.0f;
            float sampleR = 0.0f;
            const float readPos = surfaceCenterPos + surfacePlaybackPos;

            if (surfaceUsesLong)
            {
                const float mono = readLongMemory(readPos, age);
                sampleL = mono;
                sampleR = mono;
            }
            else
            {
                readShortMemory(readPos, age, sampleL, sampleR);
            }

            const float ageWeight = juce::jlimit(0.0f, 1.0f, age * (0.35f + 0.65f * decayAmount));
            const float cutoff = juce::jmap(ageWeight, kLowpassMaxHz, kLowpassMinHz);
            const float lowpassCoeff = coeffFromHz(cutoff, sampleRateHz);

            surfaceLowpassStateL += lowpassCoeff * (sampleL - surfaceLowpassStateL);
            surfaceLowpassStateR += lowpassCoeff * (sampleR - surfaceLowpassStateR);
            sampleL = surfaceLowpassStateL;
            sampleR = surfaceLowpassStateR;

            const float drive = juce::jmap(ageWeight, 1.0f, kSaturationDriveMax);
            if (drive > 1.001f)
            {
                const float norm = 1.0f / std::tanh(drive);
                sampleL = std::tanh(drive * sampleL) * norm;
                sampleR = std::tanh(drive * sampleR) * norm;
            }

            float gainErosion = 1.0f - kAgeGainReductionMax * ageWeight;
            if (surfaceUsesLong)
                gainErosion *= 0.9f;

            const float gain = surfaceBaseGain * surfaceGain * gainErosion;
            outL = juce::jlimit(-1.0f, 1.0f, sampleL * gain);
            outR = juce::jlimit(-1.0f, 1.0f, sampleR * gain);

            if (surfaceDriftCentsMax > 0.0f)
            {
                if (--driftUpdateRemaining <= 0)
                {
                    driftUpdateRemaining = driftUpdateSamples;
                    surfaceDriftTarget = (random.nextFloat() * 2.0f - 1.0f) * surfaceDriftCentsMax;
                }
                surfaceDriftCents = surfaceDriftTarget
                    + driftSlewCoeff * (surfaceDriftCents - surfaceDriftTarget);
            }
            else
            {
                surfaceDriftCents = 0.0f;
            }

            const float driftRatio = std::pow(2.0f, surfaceDriftCents / 1200.0f);
            surfacePlaybackPos += surfacePlaybackStep * driftRatio;
            const float halfWidth = 0.5f * static_cast<float>(surfaceWidthSamples);
            if (surfacePlaybackPos > halfWidth)
                surfacePlaybackPos = halfWidth;
            else if (surfacePlaybackPos < -halfWidth)
                surfacePlaybackPos = -halfWidth;

            if (surfaceState == SurfaceState::FadeIn || surfaceState == SurfaceState::FadeOut)
                surfaceGain = juce::jlimit(0.0f, 1.0f, surfaceGain + surfaceGainStep);

            ++surfaceSamplesProcessed;
            if (--surfaceSamplesRemaining <= 0)
                advanceSurface();
        }
        else if (!memoryEnabled)
        {
            surfaceState = SurfaceState::Idle;
            surfaceSamplesRemaining = 0;
            surfaceGain = 0.0f;
        }

        if (recallReady)
        {
            recallL[sample] = outL;
            recallR[sample] = outR;
        }

        if (injectToBuffer && (outL != 0.0f || outR != 0.0f))
        {
            left[sample] += outL * chambersInputGain;
            if (right != nullptr)
                right[sample] += outR * chambersInputGain;
        }
    }

#if defined(MONUMENT_TESTING)
    if (memoryEnabled && surfaceState != SurfaceState::Idle && recallReady)
    {
        float peak = 0.0f;
        double sumSq = 0.0;
        int count = 0;
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float l = recallL[sample];
            const float r = recallR[sample];
            peak = juce::jmax(peak, juce::jmax(std::abs(l), std::abs(r)));
            sumSq += static_cast<double>(l * l + r * r);
            count += 2;
        }
        const float rms = count > 0 ? static_cast<float>(std::sqrt(sumSq / count)) : 0.0f;
        juce::Logger::writeToLog("Monument MemoryEchoes surface out peak="
            + juce::String(peak, 6)
            + " rms=" + juce::String(rms, 6)
            + " gain=" + juce::String(surfaceBaseGain, 5)
            + " fade=" + juce::String(surfaceGain, 3)
            + " uses=" + juce::String(surfaceUsesLong ? "long" : "short")
            + " rmsIn=" + juce::String(lastCaptureRms, 4));
    }
#endif
}

void MemoryEchoes::captureWet(const juce::AudioBuffer<float>& wetBuffer)
{
    juce::ScopedNoDenormals noDenormals;

    if (memoryAmountForCapture <= kMemoryEpsilon)
        return;

    const int numSamples = wetBuffer.getNumSamples();
    const int numChannels = wetBuffer.getNumChannels();
    if (numSamples == 0 || numChannels == 0)
        return;

    const auto* left = wetBuffer.getReadPointer(0);
    const auto* right = numChannels > 1 ? wetBuffer.getReadPointer(1) : left;

    auto* shortL = shortBuffer.getWritePointer(0);
    auto* shortR = shortBuffer.getWritePointer(1);
    auto* longData = longBuffer.getWritePointer(0);

    float captureScale = juce::jlimit(0.0f, 1.0f, memoryAmountForCapture);
    if (freezeEnabled)
        captureScale *= kFreezeCaptureScale;

    const float shortGain = shortCaptureGain * captureScale;
    const float longGain = longCaptureGain * captureScale;

    double sumSquares = 0.0;
    int count = 0;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float inL = clampFinite(left[sample]);
        float inR = clampFinite(right[sample]);
        const float mono = 0.5f * (inL + inR);

        shortL[shortWritePos] = shortL[shortWritePos] * shortForgetFactor + inL * shortGain;
        shortR[shortWritePos] = shortR[shortWritePos] * shortForgetFactor + inR * shortGain;
        longData[longWritePos] = longData[longWritePos] * longForgetFactor + mono * longGain;

        sumSquares += static_cast<double>(mono * mono);
        ++count;

        ++shortWritePos;
        if (shortWritePos >= shortLengthSamples)
            shortWritePos = 0;
        ++longWritePos;
        if (longWritePos >= longLengthSamples)
            longWritePos = 0;

        if (shortFilledSamples < shortLengthSamples)
            ++shortFilledSamples;
        if (longFilledSamples < longLengthSamples)
            ++longFilledSamples;
    }

    if (count > 0)
    {
        const float rms = static_cast<float>(std::sqrt(sumSquares / static_cast<double>(count)));
        lastCaptureRms = captureRmsCoeff * lastCaptureRms + (1.0f - captureRmsCoeff) * rms;
    }
}

void MemoryEchoes::setMemory(float amount)
{
    memoryTarget = amount;
    memorySmoother.setTarget(amount);
}

void MemoryEchoes::setDepth(float depth)
{
    depthTarget = depth;
    depthSmoother.setTarget(depth);
}

void MemoryEchoes::setDecay(float decay)
{
    decayTarget = decay;
    decaySmoother.setTarget(decay);
}

void MemoryEchoes::setDrift(float drift)
{
    driftTarget = drift;
    driftSmoother.setTarget(drift);
}

void MemoryEchoes::setChambersInputGain(float inputGain)
{
    chambersInputGain = inputGain;
}

void MemoryEchoes::setFreeze(bool shouldFreeze)
{
    freezeEnabled = shouldFreeze;
}

void MemoryEchoes::setInjectToBuffer(bool shouldInject)
{
    injectToBuffer = shouldInject;
}

const juce::AudioBuffer<float>& MemoryEchoes::getRecallBuffer() const
{
    return recallBuffer;
}

#if defined(MONUMENT_TESTING)
void MemoryEchoes::setRandomSeed(int64_t seed)
{
    random.setSeed(static_cast<int64_t>(seed));
}
#endif

void MemoryEchoes::maybeStartSurface(int blockSamples,
    float memoryAmount,
    float depth,
    float decayAmount,
    float driftAmount)
{
    if (surfaceState != SurfaceState::Idle || surfaceCooldownSamples > 0)
        return;

    if (memoryAmount <= kMemoryEpsilon)
        return;

    const float quietFactor = juce::jlimit(0.0f, 1.0f,
        (kSurfaceQuietThreshold - lastCaptureRms) / kSurfaceQuietThreshold);
    const float quietWeight = quietFactor * quietFactor;
    if (quietWeight <= 0.0f)
        return;

    const float intervalSeconds = juce::jmap(memoryAmount,
        kSurfaceIntervalMaxSeconds, kSurfaceIntervalMinSeconds);
    const float blockSeconds = static_cast<float>(blockSamples) / static_cast<float>(sampleRateHz);
    float probability = blockSeconds / intervalSeconds;
    probability *= quietWeight;

#if defined(MONUMENT_TESTING)
    probability *= kTestSurfaceRateScale;
#endif

    if (random.nextFloat() < probability)
    {
        const float longBias = depth * depth;
        bool useLong = random.nextFloat() < longBias;
        const bool longReady = longFilledSamples >= longLengthSamples / 4;
        const bool shortReady = shortFilledSamples >= shortLengthSamples / 4;
        if (useLong && !longReady)
            useLong = shortReady;
        if (!useLong && !shortReady)
            useLong = longReady;
        if (!longReady && !shortReady)
            return;

        startSurface(useLong, memoryAmount, decayAmount, driftAmount);
    }
}

void MemoryEchoes::startSurface(bool useLong,
    float memoryAmount,
    float decayAmount,
    float driftAmount)
{
    surfaceUsesLong = useLong;
    surfaceState = SurfaceState::FadeIn;
    surfaceSamplesProcessed = 0;

    const int bufferLength = useLong ? longLengthSamples : shortLengthSamples;
    const int writePos = useLong ? longWritePos : shortWritePos;
    const int filledSamples = useLong ? longFilledSamples : shortFilledSamples;

    const float widthMs = useLong
        ? randomRange(random, kSurfaceWidthLongMinMs, kSurfaceWidthLongMaxMs)
        : randomRange(random, kSurfaceWidthMinMs, kSurfaceWidthMaxMs);
    surfaceWidthSamples = juce::jmax(1,
        static_cast<int>(std::round(widthMs * 0.001f * sampleRateHz)));

    const float filledNorm = bufferLength > 0
        ? static_cast<float>(filledSamples) / static_cast<float>(bufferLength)
        : 0.0f;
    const float maxDistance = juce::jlimit(0.2f, 0.95f, filledNorm);
    const float minDistance = juce::jmin(0.1f, maxDistance * 0.6f);

    constexpr int kSurfaceCandidates = 6;
    constexpr int kProbeCount = 12;
    float bestPeak = 0.0f;
    float bestCenter = 0.0f;

    for (int candidate = 0; candidate < kSurfaceCandidates; ++candidate)
    {
        float rand = random.nextFloat();
        if (useLong)
            rand = 1.0f - (1.0f - rand) * (1.0f - rand);
        const float distanceNorm = minDistance + (maxDistance - minDistance) * rand;
        const float distanceSamples = distanceNorm * static_cast<float>(bufferLength - 1);
        float center = static_cast<float>(writePos) - distanceSamples;
        if (center < 0.0f)
            center += static_cast<float>(bufferLength);

        float probePeak = 0.0f;
        for (int i = 0; i < kProbeCount; ++i)
        {
            const float t = kProbeCount > 1
                ? static_cast<float>(i) / static_cast<float>(kProbeCount - 1)
                : 0.5f;
            const float localOffset = (t - 0.5f) * static_cast<float>(surfaceWidthSamples);
            const float readPos = center + localOffset;
            float age = 0.0f;
            if (useLong)
            {
                const float mono = readLongMemory(readPos, age);
                probePeak = juce::jmax(probePeak, std::abs(mono));
            }
            else
            {
                float l = 0.0f;
                float r = 0.0f;
                readShortMemory(readPos, age, l, r);
                probePeak = juce::jmax(probePeak, juce::jmax(std::abs(l), std::abs(r)));
            }
        }

        if (probePeak > bestPeak)
        {
            bestPeak = probePeak;
            bestCenter = center;
        }
    }

    if (bestPeak < kSurfaceProbeMin)
    {
        surfaceState = SurfaceState::Idle;
        const float cooldownSeconds = randomRange(
            random, kSurfaceCooldownMinSeconds, kSurfaceCooldownMaxSeconds);
        surfaceCooldownSamples = secondsToSamples(cooldownSeconds, sampleRateHz);
        return;
    }

    surfaceCenterPos = bestCenter;

    const float fadeInSeconds = randomRange(random, kSurfaceFadeMinSeconds, kSurfaceFadeMaxSeconds);
    const float holdSeconds = randomRange(random, kSurfaceHoldMinSeconds, kSurfaceHoldMaxSeconds);
    const float fadeOutSeconds = randomRange(random, kSurfaceFadeMinSeconds, kSurfaceFadeMaxSeconds);

    surfaceFadeInSamples = secondsToSamples(fadeInSeconds, sampleRateHz);
    surfaceHoldSamples = secondsToSamples(holdSeconds, sampleRateHz);
    surfaceFadeOutSamples = secondsToSamples(fadeOutSeconds, sampleRateHz);
    surfaceSamplesRemaining = surfaceFadeInSamples;
    surfaceTotalSamples = surfaceFadeInSamples + surfaceHoldSamples + surfaceFadeOutSamples;

    surfaceGain = 0.0f;
    surfaceGainStep = surfaceFadeInSamples > 0 ? 1.0f / static_cast<float>(surfaceFadeInSamples) : 1.0f;

    const float targetPeak = useLong ? kSurfaceTargetPeakLong : kSurfaceTargetPeakShort;
    const float normalization = targetPeak / juce::jmax(bestPeak, kSurfaceProbeMin);
    surfaceBaseGain = juce::jlimit(0.0f, kSurfaceGainMax, normalization * memoryAmount);
    surfaceBaseGain *= juce::jmap(decayAmount, 1.0f, 0.85f);
    if (useLong)
        surfaceBaseGain *= 0.9f;

    surfacePlaybackPos = -0.5f * static_cast<float>(surfaceWidthSamples);
    surfacePlaybackStep = surfaceTotalSamples > 0
        ? static_cast<float>(surfaceWidthSamples) / static_cast<float>(surfaceTotalSamples)
        : 0.0f;

    surfaceLowpassStateL = 0.0f;
    surfaceLowpassStateR = 0.0f;

    surfaceDriftCents = 0.0f;
    surfaceDriftTarget = 0.0f;
    surfaceDriftCentsMax = kDriftCentsMax * driftAmount * (useLong ? 1.1f : 1.0f);
    driftUpdateRemaining = driftUpdateSamples;

#if defined(MONUMENT_TESTING)
    juce::Logger::writeToLog("Monument MemoryEchoes surface start buffer="
        + juce::String(useLong ? "long" : "short")
        + " widthMs=" + juce::String(widthMs, 1)
        + " gain=" + juce::String(surfaceBaseGain, 4)
        + " probePeak=" + juce::String(bestPeak, 7));
#endif
}

void MemoryEchoes::advanceSurface()
{
    switch (surfaceState)
    {
        case SurfaceState::FadeIn:
            surfaceState = SurfaceState::Hold;
            surfaceSamplesRemaining = surfaceHoldSamples;
            surfaceGain = 1.0f;
            surfaceGainStep = 0.0f;
            break;
        case SurfaceState::Hold:
            surfaceState = SurfaceState::FadeOut;
            surfaceSamplesRemaining = surfaceFadeOutSamples;
            surfaceGainStep = surfaceFadeOutSamples > 0
                ? -1.0f / static_cast<float>(surfaceFadeOutSamples)
                : -1.0f;
            break;
        case SurfaceState::FadeOut:
        case SurfaceState::Idle:
        default:
        {
            surfaceState = SurfaceState::Idle;
            surfaceSamplesRemaining = 0;
            surfaceGain = 0.0f;
            surfaceGainStep = 0.0f;
            surfaceSamplesProcessed = 0;

            const float cooldownSeconds = randomRange(
                random, kSurfaceCooldownMinSeconds, kSurfaceCooldownMaxSeconds);
            surfaceCooldownSamples = secondsToSamples(cooldownSeconds, sampleRateHz);
            break;
        }
    }
}

void MemoryEchoes::readShortMemory(float readPos,
    float& outAge,
    float& outL,
    float& outR) const
{
    if (shortLengthSamples <= 0)
    {
        outAge = 0.0f;
        outL = 0.0f;
        outR = 0.0f;
        return;
    }

    float pos = readPos;
    const float length = static_cast<float>(shortLengthSamples);
    if (pos < 0.0f)
        pos += length;
    else if (pos >= length)
        pos -= length;

    const int index0 = static_cast<int>(pos);
    const int index1 = index0 + 1 < shortLengthSamples ? index0 + 1 : 0;
    const float frac = pos - static_cast<float>(index0);

    const auto* left = shortBuffer.getReadPointer(0);
    const auto* right = shortBuffer.getReadPointer(1);

    const float l0 = left[index0];
    const float r0 = right[index0];
    const float l1 = left[index1];
    const float r1 = right[index1];

    outL = l0 + (l1 - l0) * frac;
    outR = r0 + (r1 - r0) * frac;

    float distance = static_cast<float>(shortWritePos) - pos;
    if (distance < 0.0f)
        distance += length;
    outAge = juce::jlimit(0.0f, 1.0f, distance / length);
}

float MemoryEchoes::readLongMemory(float readPos, float& outAge) const
{
    if (longLengthSamples <= 0)
    {
        outAge = 0.0f;
        return 0.0f;
    }

    float pos = readPos;
    const float length = static_cast<float>(longLengthSamples);
    if (pos < 0.0f)
        pos += length;
    else if (pos >= length)
        pos -= length;

    const int index0 = static_cast<int>(pos);
    const int index1 = index0 + 1 < longLengthSamples ? index0 + 1 : 0;
    const float frac = pos - static_cast<float>(index0);

    const auto* data = longBuffer.getReadPointer(0);
    const float s0 = data[index0];
    const float s1 = data[index1];
    const float sample = s0 + (s1 - s0) * frac;

    float distance = static_cast<float>(longWritePos) - pos;
    if (distance < 0.0f)
        distance += length;
    outAge = juce::jlimit(0.0f, 1.0f, distance / length);

    return sample;
}

} // namespace dsp
} // namespace monument
