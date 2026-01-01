#pragma once

#include <JuceHeader.h>

class MonumentDSP
{
public:
    void prepare(const juce::dsp::ProcessSpec& spec);
    void reset();

    void setParameters(float time, float mix);
    void process(juce::AudioBuffer<float>& buffer);

private:
    juce::dsp::Reverb reverb;
};
