#include "MemoryEchoes.h"

#include <cmath>

namespace
{
constexpr float kRecentMemorySeconds = 3.0f;
constexpr float kDistantMemorySeconds = 20.0f;
constexpr float kCaptureGain = 0.25f;
constexpr float kCaptureClamp = 0.5f;
constexpr float kCaptureThreshold = 2.0e-4f;
constexpr int kCaptureStrideSamples = 4;
constexpr float kEnvelopeAttackMs = 5.0f;
constexpr float kEnvelopeReleaseMs = 180.0f;

constexpr float kRecallMinSeconds = 1.0f;
constexpr float kRecallMaxSeconds = 4.0f;
constexpr float kFragmentMinMs = 150.0f;
constexpr float kFragmentMaxMs = 400.0f;
constexpr float kFragmentFadeMs = 20.0f;
constexpr float kInjectionGainMax = 0.08f;
constexpr float kInjectionClamp = 0.15f;
constexpr float kMemoryEpsilon = 1.0e-4f;

constexpr float kMemorySmoothingMs = 250.0f;
constexpr float kDepthSmoothingMs = 250.0f;
constexpr float kDecaySmoothingMs = 350.0f;
constexpr float kDriftSmoothingMs = 350.0f;
constexpr float kLowpassMaxHz = 12000.0f;
constexpr float kLowpassMinHz = 2500.0f;
constexpr float kSaturationDriveMax = 1.4f;
constexpr float kDriftCentsMax = 15.0f;
constexpr float kDriftUpdateMs = 120.0f;
constexpr float kDriftSlewMs = 160.0f;
constexpr float kRecallAgeScaleMax = 1.7f;
constexpr float kRecallMemoryScaleMax = 2.4f;

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

    recentLengthSamples = juce::jmax(1, static_cast<int>(std::round(sampleRateHz * kRecentMemorySeconds)));
    distantLengthSamples = juce::jmax(1, static_cast<int>(std::round(sampleRateHz * kDistantMemorySeconds)));

    const int bufferChannels = 2;
    recentBuffer.setSize(bufferChannels, recentLengthSamples, false, false, true);
    recentBuffer.clear();
    distantBuffer.setSize(bufferChannels, distantLengthSamples, false, false, true);
    distantBuffer.clear();

    recentWritePos = 0;
    distantWritePos = 0;
    captureStride = kCaptureStrideSamples;
    captureCounter = 0;
    captureEnvelope = 0.0f;
    captureThreshold = kCaptureThreshold;
    captureAttackCoeff = coeffFromMs(kEnvelopeAttackMs, sampleRateHz);
    captureReleaseCoeff = coeffFromMs(kEnvelopeReleaseMs, sampleRateHz);

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

    smoothersPrimed = false;
    memoryEnabled = false;
    memoryAmountForCapture = 0.0f;
    freezeEnabled = false;

    samplesUntilRecall = 0;
    fragmentActive = false;
    blockHadFragment = false;
    fragmentSamplesRemaining = 0;
    fragmentLengthSamples = 0;
    fragmentFadeSamples = 0;
    fragmentGain = 0.0f;
    fragmentReadPos = 0.0f;
    fragmentAge = 0.0f;
    fragmentLowpassCoeff = 0.0f;
    fragmentLowpassStateL = 0.0f;
    fragmentLowpassStateR = 0.0f;
    fragmentSaturationDrive = 1.0f;
    fragmentSaturationNorm = 1.0f;
    fragmentDriftCents = 0.0f;
    fragmentDriftTarget = 0.0f;
    fragmentDriftCentsMax = 0.0f;
    lastRecallAge = 0.0f;
}

void MemoryEchoes::reset()
{
    recentBuffer.clear();
    distantBuffer.clear();
    recentWritePos = 0;
    distantWritePos = 0;
    captureCounter = 0;
    captureEnvelope = 0.0f;
    memoryEnabled = false;
    memoryAmountForCapture = 0.0f;
    samplesUntilRecall = 0;
    fragmentActive = false;
    blockHadFragment = false;
    fragmentSamplesRemaining = 0;
    fragmentLengthSamples = 0;
    fragmentFadeSamples = 0;
    fragmentGain = 0.0f;
    fragmentReadPos = 0.0f;
    fragmentAge = 0.0f;
    fragmentLowpassCoeff = 0.0f;
    fragmentLowpassStateL = 0.0f;
    fragmentLowpassStateR = 0.0f;
    fragmentSaturationDrive = 1.0f;
    fragmentSaturationNorm = 1.0f;
    fragmentDriftCents = 0.0f;
    fragmentDriftTarget = 0.0f;
    fragmentDriftCentsMax = 0.0f;
    driftUpdateRemaining = driftUpdateSamples;
    lastRecallAge = 0.0f;
    smoothersPrimed = false;
}

void MemoryEchoes::process(juce::AudioBuffer<float>& buffer)
{
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

    auto* left = buffer.getWritePointer(0);
    auto* right = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    const auto* recentL = recentBuffer.getReadPointer(0);
    const auto* recentR = recentBuffer.getReadPointer(1);
    const auto* distantL = distantBuffer.getReadPointer(0);
    const auto* distantR = distantBuffer.getReadPointer(1);

    blockHadFragment = false;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float memoryAmount = juce::jlimit(0.0f, 1.0f, memorySmoother.getNextValue());
        const float depth = juce::jlimit(0.0f, 1.0f, depthSmoother.getNextValue());
        const float decayAmount = juce::jlimit(0.0f, 1.0f, decaySmoother.getNextValue());
        const float driftAmount = juce::jlimit(0.0f, 1.0f, driftSmoother.getNextValue());
        memoryAmountForCapture = memoryAmount;

        const bool enabledNow = memoryAmount > kMemoryEpsilon;
        if (enabledNow != memoryEnabled)
        {
            memoryEnabled = enabledNow;
            fragmentActive = memoryEnabled ? fragmentActive : false;
            samplesUntilRecall = memoryEnabled ? samplesUntilRecall : 0;
            if (memoryEnabled)
                scheduleNextRecall(memoryAmount);
            else
                lastRecallAge = 0.0f;
        }

        if (memoryEnabled && !fragmentActive)
        {
            if (samplesUntilRecall <= 0)
                scheduleNextRecall(memoryAmount);
            --samplesUntilRecall;
            if (samplesUntilRecall <= 0)
                startFragment(depth, memoryAmount, decayAmount, driftAmount);
        }

        float injectL = 0.0f;
        float injectR = 0.0f;

        if (fragmentActive)
        {
            blockHadFragment = true;
            // Single fragment at a time: subtle recall, no overlap or granular spray.
            const bool useDistant = activeBuffer == BufferChoice::Distant;
            const int bufferLength = useDistant ? distantLengthSamples : recentLengthSamples;
            if (bufferLength > 0)
            {
                int index0 = static_cast<int>(fragmentReadPos);
                if (index0 >= bufferLength)
                    index0 = 0;
                int index1 = index0 + 1;
                if (index1 >= bufferLength)
                    index1 = 0;
                const float frac = fragmentReadPos - static_cast<float>(index0);

                float sourceL = useDistant ? distantL[index0] : recentL[index0];
                float sourceR = useDistant ? distantR[index0] : recentR[index0];

                if (frac > 0.0f && bufferLength > 1)
                {
                    const float nextL = useDistant ? distantL[index1] : recentL[index1];
                    const float nextR = useDistant ? distantR[index1] : recentR[index1];
                    sourceL += (nextL - sourceL) * frac;
                    sourceR += (nextR - sourceR) * frac;
                }

                // Age shaping: older memories are darker, slightly saturated, and quieter.
                fragmentLowpassStateL += fragmentLowpassCoeff * (sourceL - fragmentLowpassStateL);
                fragmentLowpassStateR += fragmentLowpassCoeff * (sourceR - fragmentLowpassStateR);
                sourceL = fragmentLowpassStateL;
                sourceR = fragmentLowpassStateR;

                if (fragmentSaturationDrive > 1.0f)
                {
                    sourceL = std::tanh(fragmentSaturationDrive * sourceL) * fragmentSaturationNorm;
                    sourceR = std::tanh(fragmentSaturationDrive * sourceR) * fragmentSaturationNorm;
                }

                const int elapsed = fragmentLengthSamples - fragmentSamplesRemaining;
                float fadeGain = 1.0f;
                if (fragmentFadeSamples > 0)
                {
                    if (elapsed < fragmentFadeSamples)
                        fadeGain = static_cast<float>(elapsed) / static_cast<float>(fragmentFadeSamples);
                    else if (fragmentSamplesRemaining < fragmentFadeSamples)
                        fadeGain = static_cast<float>(fragmentSamplesRemaining) / static_cast<float>(fragmentFadeSamples);
                }

                const float gain = fragmentGain * fadeGain;
                injectL = juce::jlimit(-kInjectionClamp, kInjectionClamp, sourceL * gain);
                injectR = juce::jlimit(-kInjectionClamp, kInjectionClamp, sourceR * gain);

                // Drift is a slow random walk in cents to avoid audible modulation.
                if (fragmentDriftCentsMax > 0.0f && !freezeEnabled)
                {
                    if (--driftUpdateRemaining <= 0)
                    {
                        driftUpdateRemaining = driftUpdateSamples;
                        fragmentDriftTarget = (random.nextFloat() * 2.0f - 1.0f) * fragmentDriftCentsMax;
                    }
                    fragmentDriftCents = fragmentDriftTarget
                        + driftSlewCoeff * (fragmentDriftCents - fragmentDriftTarget);
                }
                else if (fragmentDriftCentsMax <= 0.0f)
                {
                    fragmentDriftCents = 0.0f;
                }

                const float driftRatio = std::pow(2.0f, fragmentDriftCents / 1200.0f);
                fragmentReadPos += driftRatio;
                if (fragmentReadPos >= bufferLength)
                    fragmentReadPos -= bufferLength;

                --fragmentSamplesRemaining;
                if (fragmentSamplesRemaining <= 0)
                {
                    fragmentActive = false;
                    samplesUntilRecall = 0;
                }
            }
            else
            {
                fragmentActive = false;
            }
        }

        if (injectL != 0.0f || injectR != 0.0f)
        {
            left[sample] += injectL;
            if (right != nullptr)
                right[sample] += injectR;
        }
    }
}

void MemoryEchoes::captureWet(const juce::AudioBuffer<float>& wetBuffer)
{
    // Freeze pauses capture to keep the frozen tail stable; recall may continue in the pre-Chambers path.
    // Avoid Memory -> Memory feedback by pausing capture for blocks that contained recall.
    if (freezeEnabled || blockHadFragment || memoryAmountForCapture <= kMemoryEpsilon)
        return;

    const int numSamples = wetBuffer.getNumSamples();
    const int numChannels = wetBuffer.getNumChannels();
    if (numSamples == 0 || numChannels == 0)
        return;

    const auto* left = wetBuffer.getReadPointer(0);
    const auto* right = numChannels > 1 ? wetBuffer.getReadPointer(1) : nullptr;

    auto* recentL = recentBuffer.getWritePointer(0);
    auto* recentR = recentBuffer.getWritePointer(1);
    auto* distantL = distantBuffer.getWritePointer(0);
    auto* distantR = distantBuffer.getWritePointer(1);

    const float captureGain = kCaptureGain * memoryAmountForCapture;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float wetL = left[sample];
        const float wetR = right != nullptr ? right[sample] : wetL;
        if (!std::isfinite(wetL) || !std::isfinite(wetR))
        {
            ++recentWritePos;
            if (recentWritePos >= recentLengthSamples)
                recentWritePos = 0;
            ++distantWritePos;
            if (distantWritePos >= distantLengthSamples)
                distantWritePos = 0;
            ++captureCounter;
            if (captureCounter >= captureStride)
                captureCounter = 0;
            continue;
        }
        const float inputMag = 0.5f * (std::abs(wetL) + std::abs(wetR));
        const float coeff = inputMag > captureEnvelope ? captureAttackCoeff : captureReleaseCoeff;
        captureEnvelope = inputMag + coeff * (captureEnvelope - inputMag);

        if (captureCounter == 0 && captureEnvelope > captureThreshold)
        {
            // Capture is gated and throttled to avoid recording silence or noise floor.
            const float writeL = juce::jlimit(-kCaptureClamp, kCaptureClamp, wetL * captureGain);
            const float writeR = juce::jlimit(-kCaptureClamp, kCaptureClamp, wetR * captureGain);
            recentL[recentWritePos] = writeL;
            recentR[recentWritePos] = writeR;
            distantL[distantWritePos] = writeL;
            distantR[distantWritePos] = writeR;
        }

        ++recentWritePos;
        if (recentWritePos >= recentLengthSamples)
            recentWritePos = 0;
        ++distantWritePos;
        if (distantWritePos >= distantLengthSamples)
            distantWritePos = 0;

        ++captureCounter;
        if (captureCounter >= captureStride)
            captureCounter = 0;
    }
}

void MemoryEchoes::setMemory(float amount)
{
    if (!std::isfinite(amount))
        return;
    memoryTarget = juce::jlimit(0.0f, 1.0f, amount);
    memorySmoother.setTarget(memoryTarget);
}

void MemoryEchoes::setDepth(float depth)
{
    if (!std::isfinite(depth))
        return;
    depthTarget = juce::jlimit(0.0f, 1.0f, depth);
    depthSmoother.setTarget(depthTarget);
}

void MemoryEchoes::setDecay(float decay)
{
    if (!std::isfinite(decay))
        return;
    decayTarget = juce::jlimit(0.0f, 1.0f, decay);
    decaySmoother.setTarget(decayTarget);
}

void MemoryEchoes::setDrift(float drift)
{
    if (!std::isfinite(drift))
        return;
    driftTarget = juce::jlimit(0.0f, 1.0f, drift);
    driftSmoother.setTarget(driftTarget);
}

void MemoryEchoes::setFreeze(bool shouldFreeze)
{
    freezeEnabled = shouldFreeze;
}

void MemoryEchoes::scheduleNextRecall(float memoryAmount)
{
    const float baseSeconds = juce::jmap(random.nextFloat(), kRecallMinSeconds, kRecallMaxSeconds);
    const float ageScale = 1.0f + juce::jlimit(0.0f, 1.0f, lastRecallAge) * (kRecallAgeScaleMax - 1.0f);
    const float memoryClamped = juce::jlimit(0.0f, 1.0f, memoryAmount);
    const float memoryScale = 1.0f + (1.0f - memoryClamped) * (kRecallMemoryScaleMax - 1.0f);
    const float seconds = baseSeconds * ageScale * memoryScale;
    samplesUntilRecall = juce::jmax(1, static_cast<int>(std::round(seconds * sampleRateHz)));
}

void MemoryEchoes::startFragment(float depth, float memoryAmount, float decayAmount, float driftAmount)
{
    const float depthClamped = juce::jlimit(0.0f, 1.0f, depth);
    const float memoryClamped = juce::jlimit(0.0f, 1.0f, memoryAmount);
    if (memoryClamped <= kMemoryEpsilon)
        return;

    activeBuffer = (random.nextFloat() < depthClamped) ? BufferChoice::Distant : BufferChoice::Recent;
    const int bufferLength = activeBuffer == BufferChoice::Distant ? distantLengthSamples : recentLengthSamples;
    if (bufferLength <= 0)
        return;

    const float fragmentMs = juce::jmap(random.nextFloat(), kFragmentMinMs, kFragmentMaxMs);
    fragmentLengthSamples = juce::jlimit(1, bufferLength,
        static_cast<int>(std::round(sampleRateHz * (fragmentMs / 1000.0f))));
    fragmentSamplesRemaining = fragmentLengthSamples;

    fragmentFadeSamples = juce::jmin(fragmentLengthSamples / 4,
        juce::jmax(1, static_cast<int>(std::round(sampleRateHz * (kFragmentFadeMs / 1000.0f)))));

    const int offset = random.nextInt(bufferLength);
    const int writePos = activeBuffer == BufferChoice::Distant ? distantWritePos : recentWritePos;
    fragmentReadPos = static_cast<float>(writePos - offset);
    if (fragmentReadPos < 0.0f)
        fragmentReadPos += static_cast<float>(bufferLength);

    const float offsetNorm = bufferLength > 1
        ? static_cast<float>(offset) / static_cast<float>(bufferLength - 1)
        : 0.0f;
    const float bufferBias = activeBuffer == BufferChoice::Distant ? 0.5f : 0.0f;
    fragmentAge = juce::jlimit(0.0f, 1.0f, bufferBias + 0.5f * offsetNorm);
    lastRecallAge = fragmentAge;

    const float decayScale = juce::jlimit(0.0f, 1.0f, decayAmount * fragmentAge);
    const float cutoffHz = juce::jmap(decayScale, kLowpassMaxHz, kLowpassMinHz);
    fragmentLowpassCoeff = coeffFromHz(cutoffHz, sampleRateHz);
    fragmentLowpassStateL = 0.0f;
    fragmentLowpassStateR = 0.0f;

    if (decayScale > 0.001f)
    {
        fragmentSaturationDrive = 1.0f + decayScale * (kSaturationDriveMax - 1.0f);
        const float norm = std::tanh(fragmentSaturationDrive);
        fragmentSaturationNorm = norm > 0.0f ? 1.0f / norm : 1.0f;
    }
    else
    {
        fragmentSaturationDrive = 1.0f;
        fragmentSaturationNorm = 1.0f;
    }

    const float driftScale = juce::jlimit(0.0f, 1.0f, driftAmount * fragmentAge);
    fragmentDriftCentsMax = kDriftCentsMax * driftScale;
    if (fragmentDriftCentsMax > 0.0f)
    {
        fragmentDriftTarget = (random.nextFloat() * 2.0f - 1.0f) * fragmentDriftCentsMax;
        fragmentDriftCents = fragmentDriftTarget;
        driftUpdateRemaining = driftUpdateSamples;
    }
    else
    {
        fragmentDriftTarget = 0.0f;
        fragmentDriftCents = 0.0f;
        driftUpdateRemaining = driftUpdateSamples;
    }

    const float gainScale = juce::jmax(0.0f, 1.0f - decayScale * 0.35f);
    fragmentGain = juce::jmin(kInjectionGainMax, memoryClamped * kInjectionGainMax) * gainScale;
    fragmentActive = fragmentGain > 0.0f;
}

} // namespace dsp
} // namespace monument
