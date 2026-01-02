#pragma once

#include <JuceHeader.h>

namespace monument
{
namespace dsp
{
class ParameterSmoother
{
public:
    void prepare(double sampleRate);
    void setSmoothingTimeMs(float timeMs);
    void setTarget(float value);
    float getNextValue();
    void reset(float value);

private:
    void updateCoefficient();

    double sampleRateHz = 44100.0;
    float smoothingTimeMs = 0.0f;
    float target = 0.0f;
    float current = 0.0f;
    float coefficient = 0.0f;
};

} // namespace dsp
} // namespace monument
