#include <JuceHeader.h>

#include "qa/monument_adapter.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

// Proves the QA-adapter lifecycle/buffer contract established for report
// finding E18: a partial block must not be silently widened to the
// preallocated buffer's full capacity; the needsReinit_ warm-up must size
// the wrapped processor for the declared maximum block size, not whatever
// numSamples the first processBlock() call happens to carry; and reset()
// must actually clear the wrapped processor's DSP state instead of relying
// on juce::AudioProcessor's no-op default.

namespace
{
constexpr double kSampleRate = 48000.0;
constexpr int kMaxBlockSize = 512;
constexpr int kPartialBlockSize = 64;
constexpr int kNumChannels = 2;

// Index of the Mix parameter in MonumentAdapter's parameter map (see
// monument_adapter.cpp: case 0 -> ParameterIds::Mix, scaled to [0,100]%).
constexpr int kMixParameterIndex = 0;

std::vector<std::vector<float>> makeChannelBuffers(int numChannels, int numSamples, float fillValue)
{
    std::vector<std::vector<float>> buffers(static_cast<size_t>(numChannels),
                                             std::vector<float>(static_cast<size_t>(numSamples), fillValue));
    return buffers;
}

std::vector<float*> channelPointers(std::vector<std::vector<float>>& buffers)
{
    std::vector<float*> pointers;
    pointers.reserve(buffers.size());
    for (auto& channel : buffers)
        pointers.push_back(channel.data());
    return pointers;
}

float peakAbs(const std::vector<std::vector<float>>& buffers, int startSample = 0)
{
    float peak = 0.0f;
    for (const auto& channel : buffers)
        for (size_t i = static_cast<size_t>(startSample); i < channel.size(); ++i)
            peak = std::max(peak, std::abs(channel[i]));
    return peak;
}

// A partial block (numSamples < maxBlockSize from prepare()) must forward
// exactly numSamples to the wrapped processor - not the preallocated
// buffer's full maxBlockSize capacity, which would feed the processor stale
// samples left over from a previous, larger call.
bool testPartialBlockIsNotWidenedToFullBufferCapacity()
{
    monument::qa::MonumentAdapter adapter;
    adapter.prepare(kSampleRate, kMaxBlockSize, kNumChannels);

    // Warm-up call at the declared maximum, so this test isolates the
    // partial-block buffer-view behavior from the needsReinit_ warm-up path
    // (covered separately below).
    auto warmup = makeChannelBuffers(kNumChannels, kMaxBlockSize, 0.0f);
    auto warmupPointers = channelPointers(warmup);
    adapter.processBlock(warmupPointers.data(), kNumChannels, kMaxBlockSize);

    auto partial = makeChannelBuffers(kNumChannels, kPartialBlockSize, 0.0f);
    auto partialPointers = channelPointers(partial);
    adapter.processBlock(partialPointers.data(), kNumChannels, kPartialBlockSize);

    const int lastProcessed = adapter.getLastProcessedBlockSamplesForTesting();
    if (lastProcessed != kPartialBlockSize)
    {
        std::cerr << "FAIL: partial block of " << kPartialBlockSize
                  << " samples was forwarded to the processor as " << lastProcessed
                  << " samples (expected exactly " << kPartialBlockSize << ")\n";
        return false;
    }

    std::cout << "PASS: partial block forwarded to the processor at its declared size, not the buffer's capacity\n";
    return true;
}

// The needsReinit_ warm-up (triggered by setParameter() before the first
// processBlock()) must prepareToPlay() the wrapped processor for the
// declared maxBlockSize, not for the first call's numSamples. Otherwise a
// small first (in-contract) partial block undersizes the processor's
// internal buffers, and a later, larger, still in-contract block gets
// silently clamped/passed-through instead of fully processed.
bool testReinitWarmupUsesDeclaredMaxBlockSize()
{
    monument::qa::MonumentAdapter adapter;
    adapter.prepare(kSampleRate, kMaxBlockSize, kNumChannels);

    // Set a parameter before the first processBlock() to arm needsReinit_,
    // and drive the reverb fully wet so a loud impulse produces output that
    // clearly differs from dry passthrough wherever the processor actually
    // ran.
    adapter.setParameter(kMixParameterIndex, 1.0f);

    // First call: a small, in-contract partial block. Under the bug, this
    // value (not kMaxBlockSize) gets baked into the processor's internal
    // buffer sizing.
    auto firstCall = makeChannelBuffers(kNumChannels, kPartialBlockSize, 0.0f);
    auto firstPointers = channelPointers(firstCall);
    adapter.processBlock(firstPointers.data(), kNumChannels, kPartialBlockSize);

    // Second call: a full-size, still in-contract block (numSamples ==
    // maxBlockSize from prepare()). Feed a loud, sustained signal so a wet
    // reverb definitely alters it wherever the processor actually runs.
    auto secondCall = makeChannelBuffers(kNumChannels, kMaxBlockSize, 1.0f);
    auto secondPointers = channelPointers(secondCall);
    adapter.processBlock(secondPointers.data(), kNumChannels, kMaxBlockSize);

    // If the processor's internal buffers were undersized to
    // kPartialBlockSize by the bug, samples beyond that offset are never
    // touched by processing and come back as raw dry passthrough (== 1.0f,
    // the input fill value). A correctly-sized warm-up processes the full
    // block, so a wet reverb driven this hard measurably alters those
    // samples away from the raw input value.
    float maxDeviationFromInput = 0.0f;
    for (const auto& channel : secondCall)
        for (size_t i = static_cast<size_t>(kPartialBlockSize); i < channel.size(); ++i)
            maxDeviationFromInput = std::max(maxDeviationFromInput, std::abs(channel[i] - 1.0f));

    constexpr float kMinExpectedDeviation = 1.0e-3f;
    if (maxDeviationFromInput < kMinExpectedDeviation)
    {
        std::cerr << "FAIL: samples beyond the first call's partial-block size were not "
                     "processed (max deviation from raw input: "
                  << maxDeviationFromInput << "); the processor's buffers were likely "
                     "undersized by an incorrectly-sized needsReinit_ warm-up\n";
        return false;
    }

    std::cout << "PASS: a later full-size block was fully processed after an earlier "
                 "partial-block warm-up (max deviation from raw input: "
              << maxDeviationFromInput << ")\n";
    return true;
}

// reset() must actually clear the wrapped processor's reverb tail, not rely
// on juce::AudioProcessor::reset()'s no-op default.
bool testResetClearsReverbTail()
{
    monument::qa::MonumentAdapter adapter;
    adapter.prepare(kSampleRate, kMaxBlockSize, kNumChannels);
    adapter.setParameter(kMixParameterIndex, 1.0f); // fully wet

    // Loud impulse to seed a long, clearly audible reverb tail.
    auto impulse = makeChannelBuffers(kNumChannels, kMaxBlockSize, 1.0f);
    auto impulsePointers = channelPointers(impulse);
    adapter.processBlock(impulsePointers.data(), kNumChannels, kMaxBlockSize);

    // A few silent blocks to confirm the tail genuinely persists without a
    // reset (establishes this isn't already silent for an unrelated reason).
    float peakBeforeReset = 0.0f;
    for (int block = 0; block < 3; ++block)
    {
        auto silence = makeChannelBuffers(kNumChannels, kMaxBlockSize, 0.0f);
        auto silencePointers = channelPointers(silence);
        adapter.processBlock(silencePointers.data(), kNumChannels, kMaxBlockSize);
        peakBeforeReset = std::max(peakBeforeReset, peakAbs(silence));
    }

    constexpr float kAudibleTailThreshold = 1.0e-3f;
    if (peakBeforeReset < kAudibleTailThreshold)
    {
        std::cerr << "FAIL: reverb tail was already near-silent before reset() (peak "
                  << peakBeforeReset << "); test setup does not exercise a real tail\n";
        return false;
    }

    adapter.reset();

    auto afterReset = makeChannelBuffers(kNumChannels, kMaxBlockSize, 0.0f);
    auto afterResetPointers = channelPointers(afterReset);
    adapter.processBlock(afterResetPointers.data(), kNumChannels, kMaxBlockSize);
    const float peakAfterReset = peakAbs(afterReset);

    constexpr float kSilenceThreshold = 1.0e-4f;
    if (peakAfterReset >= kSilenceThreshold)
    {
        std::cerr << "FAIL: reset() did not clear the reverb tail (peak before: "
                  << peakBeforeReset << ", peak immediately after reset(): " << peakAfterReset
                  << ", expected < " << kSilenceThreshold << ")\n";
        return false;
    }

    std::cout << "PASS: reset() cleared the reverb tail (peak before: " << peakBeforeReset
              << ", peak after: " << peakAfterReset << ")\n";
    return true;
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    try
    {
        bool allPassed = true;
        allPassed = testPartialBlockIsNotWidenedToFullBufferCapacity() && allPassed;
        allPassed = testReinitWarmupUsesDeclaredMaxBlockSize() && allPassed;
        allPassed = testResetClearsReverbTail() && allPassed;

        if (!allPassed)
        {
            std::cerr << "FAIL: one or more QA adapter contract tests failed\n";
            return 1;
        }

        std::cout << "PASS: QA adapter lifecycle/buffer contract holds\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 2;
    }
}
