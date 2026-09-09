/**
 * Monument Reverb - MemoryEchoes / DspRoutingGraph Integration Test
 *
 * Proves that MemoryEchoes is actually reached from the live signal path
 * (DspRoutingGraph::processAncientWay()), not just prepared/reset alongside
 * it. Wires a MemoryEchoes instance in via setMemoryEchoes(), bypasses every
 * DSP module so the routing graph becomes a pass-through except for the
 * memory hook, primes the recall buffers with real signal fed through the
 * graph itself, then verifies:
 *
 * 1. With memory > 0, once primed, feeding silence through the graph
 *    eventually produces non-zero output purely from recalled material.
 * 2. With memory == 0, the same procedure never produces non-zero output.
 */

#include <JuceHeader.h>
#include "dsp/DspRoutingGraph.h"
#include "dsp/MemoryEchoes.h"

#include <cmath>
#include <iostream>

using monument::dsp::DspRoutingGraph;
using monument::dsp::MemoryEchoes;
using monument::dsp::ModuleType;
using monument::dsp::RoutingPresetType;

namespace
{
constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 256;
constexpr int kNumChannels = 2;

void bypassEveryModule(DspRoutingGraph& graph)
{
    for (int m = 0; m < static_cast<int>(ModuleType::Count); ++m)
        graph.setModuleBypass(static_cast<ModuleType>(m), true);
}

bool bufferHasSignal(const juce::AudioBuffer<float>& buffer, float threshold)
{
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        const auto* data = buffer.getReadPointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
            if (std::abs(data[sample]) > threshold)
                return true;
    }
    return false;
}

// Runs priming (real signal through the graph, so MemoryEchoes::captureWet
// gets fed) followed by a silence-only scan window, looking for recalled
// energy appearing in the graph's output with no live input at all.
bool primeThenScanForRecall(DspRoutingGraph& graph, juce::AudioBuffer<float>& buffer,
                             double primeSeconds, double scanSeconds)
{
    const int primeBlocks = static_cast<int>(std::ceil(primeSeconds * kSampleRate / kBlockSize));
    for (int block = 0; block < primeBlocks; ++block)
    {
        buffer.clear();
        for (int channel = 0; channel < kNumChannels; ++channel)
        {
            auto* data = buffer.getWritePointer(channel);
            for (int sample = 0; sample < kBlockSize; ++sample)
                data[sample] = 0.35f;
        }
        graph.processAncientWay(buffer);
    }

    const int scanBlocks = static_cast<int>(std::ceil(scanSeconds * kSampleRate / kBlockSize));
    for (int block = 0; block < scanBlocks; ++block)
    {
        buffer.clear();
        graph.processAncientWay(buffer);
        if (bufferHasSignal(buffer, 1.0e-5f))
            return true;
    }
    return false;
}
} // namespace

int main()
{
    // --- Case 1: memory > 0 must eventually produce recalled output ---
    {
        DspRoutingGraph graph;
        graph.prepare(kSampleRate, kBlockSize, kNumChannels);
        graph.loadRoutingPreset(RoutingPresetType::TraditionalCathedral);
        bypassEveryModule(graph);

        MemoryEchoes memory;
        memory.prepare(kSampleRate, kBlockSize, kNumChannels);
        memory.reset();
#if defined(MONUMENT_TESTING)
        memory.setRandomSeed(0x12345678);
#endif
        memory.setMemory(1.0f);
        memory.setDepth(0.6f);
        memory.setDecay(0.5f);
        memory.setDrift(0.3f);
        memory.setFreeze(false);
        memory.setInjectToBuffer(true);
        memory.setChambersInputGain(1.0f);

        graph.setMemoryEchoes(&memory);

        // Priming/scan durations and seed match the proven-reliable recipe
        // already used by MemoryEchoesTest.cpp for this same stochastic
        // surfacing mechanic.
        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        const bool recalled = primeThenScanForRecall(graph, buffer, 2.0, 20.0);

        if (!recalled)
        {
            std::cerr << "FAIL: memory=1.0 wired via setMemoryEchoes() never produced "
                         "recalled output through processAncientWay() within the scan window.\n";
            return 1;
        }
    }

    // --- Case 2: memory == 0 must never produce recalled output ---
    {
        DspRoutingGraph graph;
        graph.prepare(kSampleRate, kBlockSize, kNumChannels);
        graph.loadRoutingPreset(RoutingPresetType::TraditionalCathedral);
        bypassEveryModule(graph);

        MemoryEchoes memory;
        memory.prepare(kSampleRate, kBlockSize, kNumChannels);
        memory.reset();
#if defined(MONUMENT_TESTING)
        memory.setRandomSeed(0x12345678);
#endif
        memory.setMemory(0.0f);
        memory.setDepth(0.6f);
        memory.setDecay(0.5f);
        memory.setDrift(0.3f);
        memory.setFreeze(false);
        memory.setInjectToBuffer(true);
        memory.setChambersInputGain(1.0f);

        graph.setMemoryEchoes(&memory);

        juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
        const bool recalled = primeThenScanForRecall(graph, buffer, 2.0, 20.0);

        if (recalled)
        {
            std::cerr << "FAIL: memory=0.0 produced recalled output through "
                         "processAncientWay() — memory amount is not gating injection.\n";
            return 1;
        }
    }

    std::cout << "PASS: MemoryEchoes recall reaches DspRoutingGraph::processAncientWay() "
                 "output when memory>0 and stays silent when memory==0.\n";
    return 0;
}
