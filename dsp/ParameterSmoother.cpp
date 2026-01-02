#include "ParameterSmoother.h"

#include <cmath>

namespace monument
{
namespace dsp
{
void ParameterSmoother::prepare(double sampleRate)
{
    sampleRateHz = sampleRate > 0.0 ? sampleRate : 44100.0;
    updateCoefficient();
}

void ParameterSmoother::setSmoothingTimeMs(float timeMs)
{
    smoothingTimeMs = juce::jmax(0.0f, timeMs);
    updateCoefficient();
}

void ParameterSmoother::setTarget(float value)
{
    target = value;
}

float ParameterSmoother::getNextValue()
{
    if (coefficient <= 0.0f)
    {
        current = target;
        return current;
    }

    current = target + (current - target) * coefficient;

    if (std::abs(current) < 1.0e-12f)
        current = 0.0f;

    return current;
}

void ParameterSmoother::reset(float value)
{
    current = value;
    target = value;
}

void ParameterSmoother::updateCoefficient()
{
    if (smoothingTimeMs <= 0.0f || sampleRateHz <= 0.0)
    {
        coefficient = 0.0f;
        return;
    }

    const double timeSeconds = static_cast<double>(smoothingTimeMs) * 0.001;
    const double alpha = std::exp(-1.0 / (timeSeconds * sampleRateHz));
    coefficient = static_cast<float>(alpha);
}

} // namespace dsp
} // namespace monument
