#pragma once

#include <JuceHeader.h>

namespace monument
{
namespace dsp
{
class DSPModule
{
public:
    DSPModule() = default;
    virtual ~DSPModule() = default;

    virtual void prepare(double sampleRate, int blockSize, int numChannels) = 0;
    virtual void reset() = 0;
    // Must be real-time safe: no allocations, locks, or logging.
    virtual void process(juce::AudioBuffer<float>& buffer) = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DSPModule)
};

} // namespace dsp
} // namespace monument
