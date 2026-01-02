#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

namespace
{
constexpr float kPresetFadeMs = 60.0f;
}

MonumentAudioProcessor::MonumentAudioProcessor()
    : juce::AudioProcessor(BusesProperties()
                                .withInput("Input", juce::AudioChannelSet::stereo(), true)
                                .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout()),
      presetManager(parameters)
{
}

MonumentAudioProcessor::~MonumentAudioProcessor() = default;

const juce::String MonumentAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MonumentAudioProcessor::acceptsMidi() const
{
    return false;
}

bool MonumentAudioProcessor::producesMidi() const
{
    return false;
}

bool MonumentAudioProcessor::isMidiEffect() const
{
    return false;
}

double MonumentAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MonumentAudioProcessor::getNumPrograms()
{
    return 1;
}

int MonumentAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MonumentAudioProcessor::setCurrentProgram(int)
{
}

const juce::String MonumentAudioProcessor::getProgramName(int)
{
    return {};
}

void MonumentAudioProcessor::changeProgramName(int, const juce::String&)
{
}

void MonumentAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    const auto numChannels = getTotalNumOutputChannels();
    dryBuffer.setSize(numChannels, samplesPerBlock, false, false, true);
    dryBuffer.clear();

    foundation.prepare(sampleRate, samplesPerBlock, numChannels);
    pillars.prepare(sampleRate, samplesPerBlock, numChannels);
    chambers.prepare(sampleRate, samplesPerBlock, numChannels);
    memoryEchoes.prepare(sampleRate, samplesPerBlock, numChannels);
    weathering.prepare(sampleRate, samplesPerBlock, numChannels);
    buttress.prepare(sampleRate, samplesPerBlock, numChannels);
    facade.prepare(sampleRate, samplesPerBlock, numChannels);

    presetFadeSamples = juce::jmax(
        1, static_cast<int>(std::round(sampleRate * (kPresetFadeMs / 1000.0f))));
    presetFadeRemaining = 0;
    presetGain = 1.0f;
    presetTransition = PresetTransitionState::None;
    presetResetRequested.store(false, std::memory_order_release);
}

void MonumentAudioProcessor::releaseResources()
{
    foundation.reset();
    pillars.reset();
    chambers.reset();
    memoryEchoes.reset();
    weathering.reset();
    buttress.reset();
    facade.reset();
}

bool MonumentAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto mainOutput = layouts.getMainOutputChannelSet();
    if (mainOutput != juce::AudioChannelSet::mono() && mainOutput != juce::AudioChannelSet::stereo())
        return false;

    if (mainOutput != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void MonumentAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

#if defined(MONUMENT_TESTING)
    const auto blockStartTicks = juce::Time::getHighResolutionTicks();
#endif

    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    const auto mixPercentRaw = parameters.getRawParameterValue("mix")->load();
    const auto time = parameters.getRawParameterValue("time")->load();
    const auto mass = parameters.getRawParameterValue("mass")->load();
    const auto density = parameters.getRawParameterValue("density")->load();
    const auto bloom = parameters.getRawParameterValue("bloom")->load();
    const auto air = parameters.getRawParameterValue("air")->load();
    const auto width = parameters.getRawParameterValue("width")->load();
    const auto warp = parameters.getRawParameterValue("warp")->load();
    const auto drift = parameters.getRawParameterValue("drift")->load();
    const auto gravity = parameters.getRawParameterValue("gravity")->load();
    const auto pillarShape = parameters.getRawParameterValue("pillarShape")->load();
    const auto pillarModeRaw = parameters.getRawParameterValue("pillarMode")->load();
    const auto memory = parameters.getRawParameterValue("memory")->load();
    const auto memoryDepth = parameters.getRawParameterValue("memoryDepth")->load();
    const auto memoryDecay = parameters.getRawParameterValue("memoryDecay")->load();
    const auto memoryDrift = parameters.getRawParameterValue("memoryDrift")->load();
    const auto freeze = parameters.getRawParameterValue("freeze")->load() > 0.5f;

    const float mixPercent = std::isfinite(mixPercentRaw) ? mixPercentRaw : 0.0f;
    float pillarModeSafe = std::isfinite(pillarModeRaw) ? pillarModeRaw : 0.0f;
    pillarModeSafe = juce::jlimit(0.0f, 2.0f, pillarModeSafe);

    if (presetResetRequested.exchange(false, std::memory_order_acq_rel))
    {
        presetTransition = PresetTransitionState::FadingOut;
        presetFadeRemaining = presetFadeSamples;
    }

    pillars.setDensity(density);
    pillars.setWarp(warp);
    pillars.setShape(pillarShape);
    pillars.setMode(static_cast<int>(std::round(pillarModeSafe)));
    chambers.setTime(time);
    chambers.setMass(mass);
    chambers.setDensity(density);
    chambers.setBloom(bloom);
    chambers.setGravity(gravity);
    chambers.setFreeze(freeze);
    memoryEchoes.setMemory(memory);
    memoryEchoes.setDepth(memoryDepth);
    memoryEchoes.setDecay(memoryDecay);
    memoryEchoes.setDrift(memoryDrift);
    memoryEchoes.setFreeze(freeze);
    weathering.setWarp(warp);
    weathering.setDrift(drift);
    buttress.setDrive(juce::jmap(mass, 0.9f, 1.6f));
    buttress.setFreeze(freeze);
    facade.setAir(air);
    facade.setWidth(juce::jmap(width, 0.0f, 2.0f));

    const auto mix = juce::jlimit(0.0f, 100.0f, mixPercent) / 100.0f;
    const auto dryGain = std::cos(mix * juce::MathConstants<float>::halfPi);
    const auto wetGain = std::sin(mix * juce::MathConstants<float>::halfPi);

    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();
    const bool dryReady = dryBuffer.getNumChannels() >= numChannels
        && dryBuffer.getNumSamples() >= numSamples;

    jassert(dryReady);
    if (dryReady)
    {
        for (int channel = 0; channel < numChannels; ++channel)
            dryBuffer.copyFrom(channel, 0, buffer, channel, 0, numSamples);
    }

    foundation.process(buffer);
    pillars.process(buffer);
    // Memory Echoes injects faint recall fragments before Chambers and captures after.
    memoryEchoes.process(buffer);
    chambers.process(buffer);
    memoryEchoes.captureWet(buffer);
    weathering.process(buffer);
    buttress.process(buffer);
    facade.process(buffer);

    if (!dryReady)
    {
        buffer.applyGain(wetGain);
        return;
    }

    for (int channel = 0; channel < numChannels; ++channel)
    {
        const auto* dry = dryBuffer.getReadPointer(channel);
        auto* wet = buffer.getWritePointer(channel);

        for (int sample = 0; sample < numSamples; ++sample)
            wet[sample] = dry[sample] * dryGain + wet[sample] * wetGain;
    }

    if (presetTransition != PresetTransitionState::None)
    {
        const float step = (presetTransition == PresetTransitionState::FadingOut ? -1.0f : 1.0f)
            / static_cast<float>(juce::jmax(1, presetFadeSamples));
        auto* left = buffer.getWritePointer(0);
        auto* right = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;
        int remaining = presetFadeRemaining;
        float gain = presetGain;

        // Apply a short gain ramp around preset changes to avoid clicks when clearing DSP state.
        for (int sample = 0; sample < numSamples; ++sample)
        {
            left[sample] *= gain;
            if (right != nullptr)
                right[sample] *= gain;

            if (remaining > 0)
            {
                gain = juce::jlimit(0.0f, 1.0f, gain + step);
                --remaining;
            }
        }

        presetGain = gain;
        presetFadeRemaining = remaining;

        if (presetTransition == PresetTransitionState::FadingOut && presetFadeRemaining == 0)
        {
            foundation.reset();
            pillars.reset();
            chambers.reset();
            memoryEchoes.reset();
            weathering.reset();
            buttress.reset();
            facade.reset();

            presetTransition = PresetTransitionState::FadingIn;
            presetFadeRemaining = presetFadeSamples;
            presetGain = 0.0f;
        }
        else if (presetTransition == PresetTransitionState::FadingIn && presetFadeRemaining == 0)
        {
            presetTransition = PresetTransitionState::None;
            presetGain = 1.0f;
        }
    }

#if defined(MONUMENT_TESTING)
    float peak = 0.0f;
    for (int channel = 0; channel < numChannels; ++channel)
    {
        const auto* data = buffer.getReadPointer(channel);
        for (int sample = 0; sample < numSamples; ++sample)
            peak = juce::jmax(peak, std::abs(data[sample]));
    }

    const auto blockEndTicks = juce::Time::getHighResolutionTicks();
    const double elapsedMs = 1000.0
        * static_cast<double>(blockEndTicks - blockStartTicks)
        / juce::Time::getHighResolutionTicksPerSecond();
    juce::Logger::writeToLog("Monument MONUMENT_TESTING peak=" + juce::String(peak, 6)
        + " blockMs=" + juce::String(elapsedMs, 3));
#endif
}

juce::AudioProcessorEditor* MonumentAudioProcessor::createEditor()
{
    return new MonumentAudioProcessorEditor(*this);
}

bool MonumentAudioProcessor::hasEditor() const
{
    return true;
}

void MonumentAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void MonumentAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

MonumentAudioProcessor::APVTS& MonumentAudioProcessor::getAPVTS()
{
    return parameters;
}

int MonumentAudioProcessor::getNumFactoryPresets() const
{
    return presetManager.getNumFactoryPresets();
}

juce::String MonumentAudioProcessor::getFactoryPresetName(int index) const
{
    return presetManager.getFactoryPresetName(index);
}

juce::String MonumentAudioProcessor::getFactoryPresetDescription(int index) const
{
    return presetManager.getFactoryPresetDescription(index);
}

void MonumentAudioProcessor::loadFactoryPreset(int index)
{
    if (!presetManager.loadFactoryPreset(index))
        return;
    if (auto* param = parameters.getParameter("freeze"))
        param->setValueNotifyingHost(0.0f);
    presetResetRequested.store(true, std::memory_order_release);
}

void MonumentAudioProcessor::saveUserPreset(const juce::String& name, const juce::String& description)
{
    presetManager.saveUserPreset(name, description);
}

void MonumentAudioProcessor::saveUserPreset(const juce::File& targetFile,
    const juce::String& name,
    const juce::String& description)
{
    presetManager.saveUserPreset(targetFile, name, description);
}

void MonumentAudioProcessor::loadUserPreset(const juce::File& sourceFile)
{
    if (!presetManager.loadUserPreset(sourceFile))
        return;
    if (auto* param = parameters.getParameter("freeze"))
        param->setValueNotifyingHost(0.0f);
    presetResetRequested.store(true, std::memory_order_release);
}

MonumentAudioProcessor::APVTS::ParameterLayout MonumentAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "mix",
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f),
        0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "time",
        "Time",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.55f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "mass",
        "Mass",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "density",
        "Density",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "bloom",
        "Bloom",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "air",
        "Air",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "width",
        "Width",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "warp",
        "Warp",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.3f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "drift",
        "Drift",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.3f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "memory",
        "Memory",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "memoryDepth",
        "Memory Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "memoryDecay",
        "Memory Decay",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.4f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "memoryDrift",
        "Memory Drift",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.3f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "gravity",
        "Gravity",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "pillarShape",
        "Pillar Shape",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "pillarMode",
        "Pillar Mode",
        juce::StringArray{"Glass", "Stone", "Fog"},
        0));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "freeze",
        "Freeze",
        false));

    return {params.begin(), params.end()};
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MonumentAudioProcessor();
}
