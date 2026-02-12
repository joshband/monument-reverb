// monument_adapter.cpp
// Audio DSP QA Harness adapter for Monument Reverb

#include "monument_adapter.h"
#include <cmath>

namespace monument {
namespace qa {

MonumentAdapter::MonumentAdapter()
    : processor_(std::make_unique<MonumentAudioProcessor>())
{
}

void MonumentAdapter::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    sampleRate_ = sampleRate;
    needsReinit_ = false;
    hasProcessed_ = false;

    // Prepare JUCE plugin processor
    processor_->setRateAndBufferSizeDetails(sampleRate, maxBlockSize);
    processor_->prepareToPlay(sampleRate, maxBlockSize);

    // Allocate JUCE buffers
    audioBuffer_.setSize(numChannels, maxBlockSize, false, true, true);
    audioBuffer_.clear();
    midiBuffer_.clear();
}

void MonumentAdapter::release()
{
    processor_->releaseResources();
    audioBuffer_.setSize(0, 0);
}

void MonumentAdapter::reset() noexcept
{
    processor_->reset();
    midiBuffer_.clear();
}

void MonumentAdapter::processBlock(float** channelData, int numChannels, int numSamples) noexcept
{
    // Re-initialize processor on first call so SmoothedValues pick up
    // parameter values set after the initial prepareToPlay().
    if (needsReinit_)
    {
        needsReinit_ = false;
        processor_->prepareToPlay(sampleRate_, numSamples);
    }
    hasProcessed_ = true;

    // Copy input to JUCE buffer
    for (int ch = 0; ch < numChannels; ++ch)
    {
        if (channelData[ch] != nullptr)
        {
            std::memcpy(audioBuffer_.getWritePointer(ch), channelData[ch],
                       static_cast<size_t>(numSamples) * sizeof(float));
        }
    }

    // Process through JUCE plugin
    processor_->processBlock(audioBuffer_, midiBuffer_);

    // Copy output back
    for (int ch = 0; ch < numChannels; ++ch)
    {
        if (channelData[ch] != nullptr)
        {
            std::memcpy(channelData[ch], audioBuffer_.getReadPointer(ch),
                       static_cast<size_t>(numSamples) * sizeof(float));
        }
    }

    // Clear MIDI buffer for next block
    midiBuffer_.clear();
}

void MonumentAdapter::setParameter(int index, ::qa::NormalizedParam value) noexcept
{
    const float normalizedValue = value;  // Already normalized [0,1]
    if (!hasProcessed_)
        needsReinit_ = true;  // Only schedule re-init before first processBlock

    switch (index)
    {
        case 0:  setJuceParameter(ParameterIds::Mix, normalizedValue * 100.0f); break;  // Mix: [0,100]%
        case 1:  setJuceParameter(ParameterIds::Time, normalizedValue); break;
        case 2:  setJuceParameter(ParameterIds::Mass, normalizedValue); break;
        case 3:  setJuceParameter(ParameterIds::Density, normalizedValue); break;
        case 4:  setJuceParameter(ParameterIds::Bloom, normalizedValue); break;
        case 5:  setJuceParameter(ParameterIds::Air, normalizedValue); break;
        case 6:  setJuceParameter(ParameterIds::Width, normalizedValue); break;
        case 7:  setJuceParameter(ParameterIds::Warp, normalizedValue); break;
        case 8:  setJuceParameter(ParameterIds::Drift, normalizedValue); break;
        case 9:  setJuceParameter(ParameterIds::Memory, normalizedValue); break;
        case 10: setJuceParameter(ParameterIds::MemoryDepth, normalizedValue); break;
        case 11: setJuceParameter(ParameterIds::MemoryDecay, normalizedValue); break;
        case 12: setJuceParameter(ParameterIds::MemoryDrift, normalizedValue); break;
        case 13: setJuceParameter(ParameterIds::Gravity, normalizedValue); break;
        case 14: setJuceParameter(ParameterIds::Freeze, normalizedValue > 0.5f ? 1.0f : 0.0f); break;
        default: break;  // Ignore invalid indices
    }
}

void MonumentAdapter::processMidiEvents(const ::qa::MidiEvent* events, int numEvents) noexcept
{
    // Convert qa::MidiEvent to JUCE MidiMessage
    for (int i = 0; i < numEvents; ++i)
    {
        const auto& event = events[i];
        juce::MidiMessage msg;

        if (event.type == ::qa::MidiEventType::NOTE_ON)
        {
            msg = juce::MidiMessage::noteOn(event.channel, event.data1, event.data2 / 127.0f);
        }
        else if (event.type == ::qa::MidiEventType::NOTE_OFF)
        {
            msg = juce::MidiMessage::noteOff(event.channel, event.data1, event.data2 / 127.0f);
        }
        else if (event.type == ::qa::MidiEventType::CONTROL_CHANGE)
        {
            msg = juce::MidiMessage::controllerEvent(event.channel, event.data1, event.data2);
        }
        else if (event.type == ::qa::MidiEventType::PITCH_BEND)
        {
            msg = juce::MidiMessage::pitchWheel(event.channel, event.data16);
        }

        // Add to JUCE MIDI buffer at sample position
        midiBuffer_.addEvent(msg, static_cast<int>(event.sampleOffset));
    }
}

bool MonumentAdapter::getCapabilities(::qa::EffectCapabilities& outCapabilities) const
{
    // Monument is a reverb effect
    outCapabilities.effectTypes = ::qa::EffectType::REVERB;

    // Monument has stateful behavior (reverb tail, memory system)
    outCapabilities.behaviors = ::qa::BehaviorFlag::STATEFUL;

    outCapabilities.description = "Monument: Architectural reverb with physical modeling";

    return true;
}

::qa::OptionalFeatures MonumentAdapter::getOptionalFeatures() const
{
    ::qa::OptionalFeatures features;
    features.supportsReset = true;
    features.supportsMidiInput = true;   // Monument accepts MIDI for modulation
    features.supportsMidiOutput = false;  // Monument doesn't produce MIDI
    features.supportsTransport = false;   // Monument doesn't use transport state
    features.supportsCapabilities = true;
    features.supportsRoutingIntrospection = false;
    return features;
}

// Private helpers

juce::RangedAudioParameter* MonumentAdapter::getJuceParameter(const char* paramId) const
{
    auto& apvts = processor_->getAPVTS();
    return apvts.getParameter(paramId);
}

void MonumentAdapter::setJuceParameter(const char* paramId, float value)
{
    if (auto* param = getJuceParameter(paramId))
    {
        // JUCE parameters expect denormalized values in their native range
        // For most Monument params, this is [0,1], but Mix is [0,100]
        param->setValueNotifyingHost(param->convertTo0to1(value));
    }
}

} // namespace qa
} // namespace monument
