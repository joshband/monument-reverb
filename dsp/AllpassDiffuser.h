#pragma once

#include <JuceHeader.h>

namespace monument
{
namespace dsp
{
class AllpassDiffuser
{
public:
    void setDelaySamples(int samples);
    void setCoefficient(float coefficientIn);

    void prepare();
    void reset();

    float processSample(float input);

private:
    int delaySamples = 1;
    int bufferLength = 0;
    int writePosition = 0;
    float coefficient = 0.5f;
    juce::HeapBlock<float> buffer;
};

} // namespace dsp
} // namespace monument
