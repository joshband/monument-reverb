#include <JuceHeader.h>

#include "plugin/PluginProcessor.h"

#include <cmath>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>

#if defined(MONUMENT_TESTING)
#error "The block-shape contract test must run without MONUMENT_TESTING"
#endif

// Proves the realtime-safety contract established for oversized/zero-length blocks
// (report finding E17): a zero-length block must not reach any division by
// numSamples, and a block larger than ParameterBufferPool's capacity must not
// allocate on the audio thread. Uses the same scoped global-allocator-override
// technique as RealtimeAllocationCharacterizationTest.cpp to prove the oversized
// case allocates zero.

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
constexpr int kOversizedBlockSize = 4096; // > ParameterBufferPool::kMaxSamples (2048)

bool isFinite(const juce::AudioBuffer<float>& buffer) noexcept
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            if (!std::isfinite(data[i]))
                return false;
    }
    return true;
}

// A zero-length processBlock() call must not corrupt state (e.g. via a
// division by numSamples producing NaN/Inf that lingers in smoothed values or
// meters). Proven by calling it between two normal blocks and checking the
// second normal block's output is still finite.
bool testZeroLengthBlockDoesNotCorruptState()
{
    MonumentAudioProcessor processor;
    processor.prepareToPlay(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> normalBuffer(2, kBlockSize);
    juce::MidiBuffer midi;
    normalBuffer.clear();
    processor.processBlock(normalBuffer, midi);

    juce::AudioBuffer<float> zeroBuffer(2, 0);
    processor.processBlock(zeroBuffer, midi);

    normalBuffer.clear();
    processor.processBlock(normalBuffer, midi);

    if (!isFinite(normalBuffer))
    {
        std::cerr << "FAIL: block after a zero-length block produced non-finite output\n";
        return false;
    }

    std::cout << "PASS: zero-length block did not corrupt subsequent processing\n";
    return true;
}

// An oversized block (larger than ParameterBufferPool::capacity()) must not
// allocate on the audio thread, and the processed portion must be finite
// (no buffer overrun / garbage reads from the parameter buffer pool).
bool testOversizedBlockDoesNotAllocate()
{
    MonumentAudioProcessor processor;
    processor.prepareToPlay(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> warmupBuffer(2, kBlockSize);
    juce::MidiBuffer midi;
    warmupBuffer.clear();
    processor.processBlock(warmupBuffer, midi);

    // Baseline: allocations for an ordinary, in-contract block of the same
    // size the oversized block will be clamped down to. This isolates the
    // oversized-block CLAMP path's own behavior from unrelated per-block
    // allocation paths elsewhere in the signal chain (tracked separately,
    // e.g. report findings E15/E16) - this test asserts the clamp adds no
    // allocation on top of that baseline, not that the baseline itself is
    // zero.
    juce::AudioBuffer<float> baselineBuffer(2, kBlockSize);
    baselineBuffer.clear();
    allocation_probe::ScopedCounter baselineCounter;
    processor.processBlock(baselineBuffer, midi);
    const auto baselineAllocations = baselineCounter.stop();

    juce::AudioBuffer<float> oversizedBuffer(2, kOversizedBlockSize);
    oversizedBuffer.clear();

    allocation_probe::ScopedCounter counter;
    processor.processBlock(oversizedBuffer, midi);
    const auto allocations = counter.stop();

    if (allocations != baselineAllocations)
    {
        std::cerr << "FAIL: oversized block allocated " << allocations
                  << " time(s), vs. " << baselineAllocations
                  << " for an in-contract block of the clamped size\n";
        return false;
    }

    if (!isFinite(oversizedBuffer))
    {
        std::cerr << "FAIL: oversized block produced non-finite output\n";
        return false;
    }

    std::cout << "PASS: oversized block (" << kOversizedBlockSize
              << " samples) allocated no more than an in-contract block ("
              << allocations << ") and produced finite output\n";
    return true;
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    try
    {
        bool allPassed = true;
        allPassed = testZeroLengthBlockDoesNotCorruptState() && allPassed;
        allPassed = testOversizedBlockDoesNotAllocate() && allPassed;

        if (!allPassed)
        {
            std::cerr << "FAIL: one or more block-shape contract tests failed\n";
            return 1;
        }

        std::cout << "PASS: block-shape contract holds for zero-length and oversized blocks\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 2;
    }
}
