#include <JuceHeader.h>

#include "dsp/ModulationMatrix.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

// Characterizes the concurrency hazard the reset report (finding E24) raised
// against ModulationMatrix::publishConnectionsSnapshot(): its two-slot
// double-buffer has no reader-ownership handshake before a writer reuses a
// slot. If the message thread publishes twice while the audio thread is
// still mid-read of the previously-active slot, the second publish
// overwrites data the audio thread is concurrently reading.
//
// This does not assume ThreadSanitizer is available, but is designed to be
// built with -fsanitize=thread (see CMakeLists.txt) so a genuine data race
// is proven, not just inferred from reading the source. Run under a plain
// build, this is a best-effort stress test; TSan turns it into a verified
// characterization.
//
// One audio-thread stand-in calls process() in a tight loop (the fastest
// realistic reader) while the message thread stand-in mutates connections
// as fast as possible with no synchronization beyond what ModulationMatrix
// itself provides - deliberately worse than realistic UI-driven mutation
// rates, to maximize the chance of hitting the race window in a bounded
// run.

namespace
{
constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 512;
constexpr int kNumChannels = 2;
constexpr int kMutationIterations = 50000;
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    using namespace monument::dsp;

    ModulationMatrix matrix;
    matrix.prepare(kSampleRate, kBlockSize, kNumChannels);

    juce::AudioBuffer<float> buffer(kNumChannels, kBlockSize);
    buffer.clear();

    std::atomic<bool> stopAudioThread{false};
    std::atomic<std::uint64_t> processCallCount{0};

    std::thread audioThreadStandIn([&]
    {
        while (!stopAudioThread.load(std::memory_order_relaxed))
        {
            matrix.process(buffer, buffer.getNumSamples());
            processCallCount.fetch_add(1, std::memory_order_relaxed);
        }
    });

    // Message-thread stand-in: mutate connections as fast as possible, with
    // no throttling, for kMutationIterations iterations.
    for (int i = 0; i < kMutationIterations; ++i)
    {
        matrix.setConnection(ModulationMatrix::SourceType::ChaosAttractor,
                              ModulationMatrix::DestinationType::Time,
                              0, 0.5f, 50.0f);
        matrix.setConnection(ModulationMatrix::SourceType::BrownianMotion,
                              ModulationMatrix::DestinationType::Mass,
                              0, 0.3f, 100.0f);
        matrix.removeConnection(ModulationMatrix::SourceType::ChaosAttractor,
                                 ModulationMatrix::DestinationType::Time, 0);
        matrix.removeConnection(ModulationMatrix::SourceType::BrownianMotion,
                                 ModulationMatrix::DestinationType::Mass, 0);
    }

    stopAudioThread.store(true, std::memory_order_relaxed);
    audioThreadStandIn.join();

    std::cout << "Completed " << kMutationIterations
              << " connection-mutation iterations against "
              << processCallCount.load(std::memory_order_relaxed)
              << " concurrent process() calls.\n";
    std::cout << "PASS: stress loop completed. Under -fsanitize=thread, "
                 "ThreadSanitizer independently reports any detected data "
                 "race to stderr and this run's exit status.\n";
    return 0;
}
