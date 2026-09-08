#include <JuceHeader.h>

#include "plugin/PluginProcessor.h"

#include <array>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>

// Proves report finding E31's remaining half (step 3): the exact compile flags
// monument_qa uses (MONUMENT_TESTING=1, matching CMakeLists.txt's monument_qa
// target) must not allocate per processBlock() call due to debug logging. The
// harness's own profiling (wired in report step 4b) measures allocations under
// precisely this build, so any per-block logging left gated on MONUMENT_TESTING
// alone would silently confound that measurement again.

#if !defined(MONUMENT_TESTING)
#error "This test must build WITH MONUMENT_TESTING=1, matching monument_qa's own compile definitions"
#endif
#if defined(MONUMENT_TESTING_VERBOSE_LOG)
#error "This test must build WITHOUT MONUMENT_TESTING_VERBOSE_LOG - that is the separate, opt-in diagnostic-logging flag this test proves is NOT implied by MONUMENT_TESTING alone"
#endif

namespace allocation_probe
{
thread_local bool isCounting = false;
thread_local std::size_t allocationCount = 0;

static void recordAllocation() noexcept
{
    if (isCounting)
        ++allocationCount;
}

static void* allocate(std::size_t size)
{
    if (void* memory = std::malloc(size == 0 ? 1 : size))
    {
        recordAllocation();
        return memory;
    }

    throw std::bad_alloc{};
}

static void* allocateAligned(std::size_t size, std::size_t alignment)
{
    void* memory = nullptr;
    if (posix_memalign(&memory, alignment, size == 0 ? 1 : size) == 0)
    {
        recordAllocation();
        return memory;
    }

    throw std::bad_alloc{};
}

class ScopedCounter final
{
public:
    ScopedCounter()
    {
        allocationCount = 0;
        isCounting = true;
    }

    ~ScopedCounter()
    {
        isCounting = false;
    }

    std::size_t stop() noexcept
    {
        isCounting = false;
        return allocationCount;
    }
};
} // namespace allocation_probe

void* operator new(std::size_t size)
{
    return allocation_probe::allocate(size);
}

void* operator new[](std::size_t size)
{
    return allocation_probe::allocate(size);
}

void* operator new(std::size_t size, std::align_val_t alignment)
{
    return allocation_probe::allocateAligned(size, static_cast<std::size_t>(alignment));
}

void* operator new[](std::size_t size, std::align_val_t alignment)
{
    return allocation_probe::allocateAligned(size, static_cast<std::size_t>(alignment));
}

void operator delete(void* memory) noexcept
{
    std::free(memory);
}

void operator delete[](void* memory) noexcept
{
    std::free(memory);
}

void operator delete(void* memory, std::size_t) noexcept
{
    std::free(memory);
}

void operator delete[](void* memory, std::size_t) noexcept
{
    std::free(memory);
}

void operator delete(void* memory, std::align_val_t) noexcept
{
    std::free(memory);
}

void operator delete[](void* memory, std::align_val_t) noexcept
{
    std::free(memory);
}

void operator delete(void* memory, std::size_t, std::align_val_t) noexcept
{
    std::free(memory);
}

void operator delete[](void* memory, std::size_t, std::align_val_t) noexcept
{
    std::free(memory);
}

namespace
{
constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 512;
constexpr int kWarmupBlocks = 4;
constexpr int kMeasuredBlocks = 8;

// Mirrors profileDspPerformance()'s own warmup/measure split (see
// external/audio-dsp-qa-harness/runners/performance_profiler.cpp) so this
// test exercises the same steady-state condition the harness measures.
std::array<std::size_t, kMeasuredBlocks> characterizeWarmedBlockAllocations()
{
    MonumentAudioProcessor processor;
    processor.prepareToPlay(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    juce::MidiBuffer midi;

    for (int i = 0; i < kWarmupBlocks; ++i)
    {
        buffer.clear();
        processor.processBlock(buffer, midi);
    }

    std::array<std::size_t, kMeasuredBlocks> counts{};
    for (int i = 0; i < kMeasuredBlocks; ++i)
    {
        buffer.clear();
        allocation_probe::ScopedCounter counter;
        processor.processBlock(buffer, midi);
        counts[static_cast<std::size_t>(i)] = counter.stop();
    }

    return counts;
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    try
    {
        const auto counts = characterizeWarmedBlockAllocations();

        bool allocated = false;
        for (int i = 0; i < kMeasuredBlocks; ++i)
        {
            const auto count = counts[static_cast<std::size_t>(i)];
            allocated = allocated || count != 0;
            std::cout << "MONUMENT_TESTING-build processBlock[" << i << "] allocations: " << count << '\n';
        }

        if (allocated)
        {
            std::cerr << "FAIL: a warmed processBlock() call allocated under the exact flags "
                         "monument_qa uses (MONUMENT_TESTING=1, no verbose log flag); counts above\n";
            return 1;
        }

        std::cout << "PASS: MONUMENT_TESTING build made zero counted allocations per warmed processBlock\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 2;
    }
}
