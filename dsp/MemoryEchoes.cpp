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

inline float coeffFromMs(float timeMs, double sampleRate)
{
    const double timeSeconds = static_cast<double>(timeMs) / 1000.0;
    return static_cast<float>(std::exp(-1.0 / (timeSeconds * sampleRate)));
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

    smoothersPrimed = false;
    memoryEnabled = false;
    memoryAmountForCapture = 0.0f;
    freezeEnabled = false;

    samplesUntilRecall = 0;
    fragmentActive = false;
    fragmentSamplesRemaining = 0;
    fragmentLengthSamples = 0;
    fragmentFadeSamples = 0;
    fragmentGain = 0.0f;
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
    fragmentSamplesRemaining = 0;
    fragmentLengthSamples = 0;
    fragmentFadeSamples = 0;
    fragmentGain = 0.0f;
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
        smoothersPrimed = true;
    }

    auto* left = buffer.getWritePointer(0);
    auto* right = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;

    const auto* recentL = recentBuffer.getReadPointer(0);
    const auto* recentR = recentBuffer.getReadPointer(1);
    const auto* distantL = distantBuffer.getReadPointer(0);
    const auto* distantR = distantBuffer.getReadPointer(1);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float memoryAmount = juce::jlimit(0.0f, 1.0f, memorySmoother.getNextValue());
        const float depth = juce::jlimit(0.0f, 1.0f, depthSmoother.getNextValue());
        memoryAmountForCapture = memoryAmount;

        const bool enabledNow = memoryAmount > kMemoryEpsilon;
        if (enabledNow != memoryEnabled)
        {
            memoryEnabled = enabledNow;
            fragmentActive = memoryEnabled ? fragmentActive : false;
            samplesUntilRecall = memoryEnabled ? samplesUntilRecall : 0;
            if (memoryEnabled)
                scheduleNextRecall();
        }

        if (memoryEnabled && !fragmentActive)
        {
            if (samplesUntilRecall <= 0)
                scheduleNextRecall();
            --samplesUntilRecall;
            if (samplesUntilRecall <= 0)
                startFragment(depth, memoryAmount);
        }

        float injectL = 0.0f;
        float injectR = 0.0f;

        if (fragmentActive)
        {
            // Single fragment at a time: subtle recall, no overlap or granular spray.
            const bool useDistant = activeBuffer == BufferChoice::Distant;
            const int bufferLength = useDistant ? distantLengthSamples : recentLengthSamples;
            if (bufferLength > 0)
            {
                const float sourceL = useDistant ? distantL[fragmentReadPos] : recentL[fragmentReadPos];
                const float sourceR = useDistant ? distantR[fragmentReadPos] : recentR[fragmentReadPos];

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

                ++fragmentReadPos;
                if (fragmentReadPos >= bufferLength)
                    fragmentReadPos = 0;

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
    if (freezeEnabled || memoryAmountForCapture <= kMemoryEpsilon)
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

void MemoryEchoes::setFreeze(bool shouldFreeze)
{
    freezeEnabled = shouldFreeze;
}

void MemoryEchoes::scheduleNextRecall()
{
    const float seconds = juce::jmap(random.nextFloat(), kRecallMinSeconds, kRecallMaxSeconds);
    samplesUntilRecall = juce::jmax(1, static_cast<int>(std::round(seconds * sampleRateHz)));
}

void MemoryEchoes::startFragment(float depth, float memoryAmount)
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
    fragmentReadPos = writePos - offset;
    if (fragmentReadPos < 0)
        fragmentReadPos += bufferLength;

    fragmentGain = juce::jmin(kInjectionGainMax, memoryClamped * kInjectionGainMax);
    fragmentActive = fragmentGain > 0.0f;
}

} // namespace dsp
} // namespace monument
