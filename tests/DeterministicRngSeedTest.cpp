/**
 * Monument Reverb - Deterministic RNG Seed Seam Test
 *
 * Report finding E25: Chambers seeds its late-diffusion drift RNG from an
 * unseeded juce::Random, and ModulationMatrix's BrownianMotion source seeds
 * from a high-resolution-clock timestamp. Both are appropriate for
 * production (each plugin instance gets genuinely different organic motion),
 * but make QA-harness scenario captures unreproducible run to run.
 *
 * This test proves a test-only deterministic seed seam exists for both,
 * that it produces bit-identical output across instances when used, and
 * that production (unseeded) behavior is unchanged - two unseeded instances
 * still diverge, exactly as before this seam was added.
 */

#include <JuceHeader.h>
#include "dsp/Chambers.h"
#include "dsp/ModulationMatrix.h"

#include <cstring>
#include <iostream>

using namespace monument::dsp;

namespace
{
constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 512;
constexpr int kNumChannels = 2;
constexpr int kNumBlocksToProcess = 100; // ~1 second at 48kHz/512

bool buffersEqual(const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
{
    if (a.getNumChannels() != b.getNumChannels() || a.getNumSamples() != b.getNumSamples())
        return false;

    for (int ch = 0; ch < a.getNumChannels(); ++ch)
    {
        if (std::memcmp(a.getReadPointer(ch), b.getReadPointer(ch),
                         static_cast<size_t>(a.getNumSamples()) * sizeof(float)) != 0)
            return false;
    }
    return true;
}

void primeWithImpulse(juce::AudioBuffer<float>& buffer)
{
    buffer.clear();
    buffer.setSample(0, 0, 1.0f);
    buffer.setSample(1, 0, 1.0f);
}

bool testChambersSameSeedProducesIdenticalOutput()
{
    Chambers a, b;
    a.setDeterministicDriftSeedForTesting(12345);
    b.setDeterministicDriftSeedForTesting(12345);
    a.prepare(kSampleRate, kBlockSize, kNumChannels);
    b.prepare(kSampleRate, kBlockSize, kNumChannels);
    a.setDrift(1.0f);
    b.setDrift(1.0f);

    juce::AudioBuffer<float> bufA(kNumChannels, kBlockSize);
    juce::AudioBuffer<float> bufB(kNumChannels, kBlockSize);
    primeWithImpulse(bufA);
    primeWithImpulse(bufB);

    for (int block = 0; block < kNumBlocksToProcess; ++block)
    {
        a.process(bufA);
        b.process(bufB);

        if (!buffersEqual(bufA, bufB))
        {
            std::cerr << "FAIL: Chambers seeded identically diverged at block " << block << '\n';
            return false;
        }

        bufA.clear();
        bufB.clear();
    }

    std::cout << "PASS: Chambers with the same deterministic seed produced identical output over "
              << kNumBlocksToProcess << " blocks\n";
    return true;
}

bool testChambersUnseededDiffersAcrossInstances()
{
    Chambers a, b;
    // No seed set: default production (time/entropy-based) behavior.
    a.prepare(kSampleRate, kBlockSize, kNumChannels);
    b.prepare(kSampleRate, kBlockSize, kNumChannels);
    a.setDrift(1.0f);
    b.setDrift(1.0f);

    juce::AudioBuffer<float> bufA(kNumChannels, kBlockSize);
    juce::AudioBuffer<float> bufB(kNumChannels, kBlockSize);
    primeWithImpulse(bufA);
    primeWithImpulse(bufB);

    bool everDiffered = false;
    for (int block = 0; block < kNumBlocksToProcess; ++block)
    {
        a.process(bufA);
        b.process(bufB);

        if (!buffersEqual(bufA, bufB))
        {
            everDiffered = true;
            break;
        }

        bufA.clear();
        bufB.clear();
    }

    if (!everDiffered)
    {
        std::cerr << "FAIL: two unseeded Chambers instances produced identical output - "
                     "default (production) behavior must remain non-deterministic\n";
        return false;
    }

    std::cout << "PASS: unseeded Chambers instances (default production behavior) diverge, as before\n";
    return true;
}

bool testModulationMatrixSameSeedProducesIdenticalOutput()
{
    ModulationMatrix a, b;
    a.setDeterministicRngSeedForTesting(54321);
    b.setDeterministicRngSeedForTesting(54321);
    a.prepare(kSampleRate, kBlockSize, kNumChannels);
    b.prepare(kSampleRate, kBlockSize, kNumChannels);

    a.setConnection(ModulationMatrix::SourceType::BrownianMotion,
                    ModulationMatrix::DestinationType::Time, 0, 1.0f, 0.0f);
    b.setConnection(ModulationMatrix::SourceType::BrownianMotion,
                    ModulationMatrix::DestinationType::Time, 0, 1.0f, 0.0f);

    juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
    buffer.clear();

    for (int block = 0; block < kNumBlocksToProcess; ++block)
    {
        a.process(buffer, buffer.getNumSamples());
        b.process(buffer, buffer.getNumSamples());

        const float valueA = a.getModulation(ModulationMatrix::DestinationType::Time);
        const float valueB = b.getModulation(ModulationMatrix::DestinationType::Time);

        if (valueA != valueB)
        {
            std::cerr << "FAIL: ModulationMatrix seeded identically diverged at block " << block
                      << " (" << valueA << " vs " << valueB << ")\n";
            return false;
        }
    }

    std::cout << "PASS: ModulationMatrix with the same deterministic seed produced identical output over "
              << kNumBlocksToProcess << " blocks\n";
    return true;
}

bool testModulationMatrixUnseededDiffersAcrossInstances()
{
    ModulationMatrix a, b;
    // No seed set: default production (time-based) behavior.
    a.prepare(kSampleRate, kBlockSize, kNumChannels);
    b.prepare(kSampleRate, kBlockSize, kNumChannels);

    a.setConnection(ModulationMatrix::SourceType::BrownianMotion,
                    ModulationMatrix::DestinationType::Time, 0, 1.0f, 0.0f);
    b.setConnection(ModulationMatrix::SourceType::BrownianMotion,
                    ModulationMatrix::DestinationType::Time, 0, 1.0f, 0.0f);

    juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
    buffer.clear();

    bool everDiffered = false;
    for (int block = 0; block < kNumBlocksToProcess; ++block)
    {
        a.process(buffer, buffer.getNumSamples());
        b.process(buffer, buffer.getNumSamples());

        const float valueA = a.getModulation(ModulationMatrix::DestinationType::Time);
        const float valueB = b.getModulation(ModulationMatrix::DestinationType::Time);

        if (valueA != valueB)
        {
            everDiffered = true;
            break;
        }
    }

    if (!everDiffered)
    {
        std::cerr << "FAIL: two unseeded ModulationMatrix instances produced identical output - "
                     "default (production) behavior must remain non-deterministic\n";
        return false;
    }

    std::cout << "PASS: unseeded ModulationMatrix instances (default production behavior) diverge, as before\n";
    return true;
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    bool allPassed = true;
    allPassed = testChambersSameSeedProducesIdenticalOutput() && allPassed;
    allPassed = testChambersUnseededDiffersAcrossInstances() && allPassed;
    allPassed = testModulationMatrixSameSeedProducesIdenticalOutput() && allPassed;
    allPassed = testModulationMatrixUnseededDiffersAcrossInstances() && allPassed;

    if (!allPassed)
    {
        std::cerr << "FAIL: one or more deterministic RNG seed tests failed\n";
        return 1;
    }

    std::cout << "PASS: deterministic RNG seed seam verified for Chambers and ModulationMatrix\n";
    return 0;
}
