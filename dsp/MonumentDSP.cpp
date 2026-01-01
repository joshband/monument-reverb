#include "MonumentDSP.h"

void MonumentDSP::prepare(const juce::dsp::ProcessSpec& spec)
{
    reverb.reset();
    reverb.prepare(spec);
}

void MonumentDSP::reset()
{
    reverb.reset();
}

void MonumentDSP::setParameters(float time, float mix)
{
    juce::dsp::Reverb::Parameters params;
    params.roomSize = time;
    params.damping = 0.5f;
    params.wetLevel = mix;
    params.dryLevel = 1.0f - mix;
    params.width = 1.0f;
    params.freezeMode = 0.0f;

    reverb.setParameters(params);
}

void MonumentDSP::process(juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    reverb.process(context);
}
