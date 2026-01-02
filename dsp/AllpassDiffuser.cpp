#include "AllpassDiffuser.h"

namespace monument
{
namespace dsp
{
void AllpassDiffuser::setDelaySamples(int samples)
{
    delaySamples = juce::jmax(1, samples);
}

void AllpassDiffuser::setCoefficient(float coefficientIn)
{
    coefficient = juce::jlimit(-0.74f, 0.74f, coefficientIn);
}

void AllpassDiffuser::prepare()
{
    bufferLength = delaySamples + 1;
    buffer.calloc(static_cast<size_t>(bufferLength));
    writePosition = 0;
}

void AllpassDiffuser::reset()
{
    if (bufferLength > 0 && buffer.get() != nullptr)
        juce::zeromem(buffer.get(), sizeof(float) * static_cast<size_t>(bufferLength));
    writePosition = 0;
}

float AllpassDiffuser::processSample(float input)
{
    if (bufferLength == 0 || buffer.get() == nullptr)
        return input;

    int readPosition = writePosition - delaySamples;
    if (readPosition < 0)
        readPosition += bufferLength;

    const float delayed = buffer[readPosition];
    const float output = delayed - coefficient * input;

    buffer[writePosition] = input + coefficient * output;

    ++writePosition;
    if (writePosition >= bufferLength)
        writePosition = 0;

    return output;
}

} // namespace dsp
} // namespace monument
