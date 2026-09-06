#include <JuceHeader.h>

#include "dsp/TubeRayTracer.h"
#include "plugin/PluginProcessor.h"

#include <array>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <new>
#include <stdexcept>

#if defined(MONUMENT_TESTING)
#error "The realtime allocation characterization must run without MONUMENT_TESTING"
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
constexpr int kMinTubeCount = 5;
constexpr int kMaxTubeCount = 16;
constexpr int kTubeBoundaryCount = kMaxTubeCount - kMinTubeCount;

std::size_t characterizeTimelinePresetChange()
{
    MonumentAudioProcessor processor;
    processor.prepareToPlay(kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    juce::MidiBuffer midi;
    buffer.clear();
    processor.processBlock(buffer, midi);

    auto* timelinePreset = dynamic_cast<juce::AudioParameterChoice*>(
        processor.getAPVTS().getParameter("timelinePreset"));
    if (timelinePreset == nullptr)
        throw std::runtime_error("timelinePreset parameter is unavailable");

    timelinePreset->setValueNotifyingHost(timelinePreset->convertTo0to1(2.0f));
    buffer.clear();

    allocation_probe::ScopedCounter counter;
    processor.processBlock(buffer, midi);
    return counter.stop();
}

float normalizedTubeCount(int tubeCount) noexcept
{
    if (tubeCount == kMaxTubeCount)
        return 1.0f;

    const auto countOffset = static_cast<float>(tubeCount - kMinTubeCount);
    return (countOffset + 0.5f) / static_cast<float>(kMaxTubeCount - kMinTubeCount);
}

std::array<std::size_t, kTubeBoundaryCount> characterizeTubeCountBoundaries()
{
    monument::dsp::TubeRayTracer tracer;
    tracer.prepare(kSampleRate, kBlockSize, 2);

    juce::AudioBuffer<float> buffer(2, kBlockSize);
    buffer.clear();
    tracer.setTubeCount(0.0f);
    tracer.process(buffer);

    std::array<std::size_t, kTubeBoundaryCount> allocationCounts{};
    for (int tubeCount = kMinTubeCount + 1; tubeCount <= kMaxTubeCount; ++tubeCount)
    {
        tracer.setTubeCount(normalizedTubeCount(tubeCount));
        buffer.clear();

        allocation_probe::ScopedCounter counter;
        tracer.process(buffer);
        allocationCounts[static_cast<std::size_t>(tubeCount - kMinTubeCount - 1)] = counter.stop();
    }

    return allocationCounts;
}
} // namespace

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInitialiser;

    try
    {
        const auto timelineAllocations = characterizeTimelinePresetChange();
        const auto tubeAllocations = characterizeTubeCountBoundaries();

        bool allocated = timelineAllocations != 0;
        std::cout << "timelinePreset post-change processBlock allocations: "
                  << timelineAllocations << '\n';

        for (int targetCount = kMinTubeCount + 1; targetCount <= kMaxTubeCount; ++targetCount)
        {
            const auto count = tubeAllocations[
                static_cast<std::size_t>(targetCount - kMinTubeCount - 1)];
            allocated = allocated || count != 0;
            std::cout << "TubeRayTracer " << (targetCount - 1) << " -> " << targetCount
                      << " process allocations: " << count << '\n';
        }

        if (allocated)
        {
            std::cerr << "FAIL: warmed realtime processing calls allocated; counts are listed above\n";
            return 1;
        }

        std::cout << "PASS: warmed realtime processing calls made zero counted allocations\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 2;
    }
}
