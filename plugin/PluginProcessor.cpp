#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>

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
    weathering.prepare(sampleRate, samplesPerBlock, numChannels);
    buttress.prepare(sampleRate, samplesPerBlock, numChannels);
    facade.prepare(sampleRate, samplesPerBlock, numChannels);
}

void MonumentAudioProcessor::releaseResources()
{
    foundation.reset();
    pillars.reset();
    chambers.reset();
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

    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    const auto mixPercent = parameters.getRawParameterValue("mix")->load();
    const auto time = parameters.getRawParameterValue("time")->load();
    const auto mass = parameters.getRawParameterValue("mass")->load();
    const auto density = parameters.getRawParameterValue("density")->load();
    const auto bloom = parameters.getRawParameterValue("bloom")->load();
    const auto air = parameters.getRawParameterValue("air")->load();
    const auto width = parameters.getRawParameterValue("width")->load();
    const auto warp = parameters.getRawParameterValue("warp")->load();
    const auto drift = parameters.getRawParameterValue("drift")->load();
    const auto gravity = parameters.getRawParameterValue("gravity")->load();
    const auto freeze = parameters.getRawParameterValue("freeze")->load() > 0.5f;

    pillars.setDensity(density);
    chambers.setTime(time);
    chambers.setMass(mass);
    chambers.setDensity(density);
    chambers.setBloom(bloom);
    chambers.setGravity(gravity);
    chambers.setFreeze(freeze);
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
    chambers.process(buffer);
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

int MonumentAudioProcessor::getNumPresets() const
{
    return presetManager.getNumPresets();
}

std::string MonumentAudioProcessor::getPresetName(int index) const
{
    return presetManager.getPresetName(index);
}

void MonumentAudioProcessor::loadPreset(int index)
{
    presetManager.loadPreset(index);
}

bool MonumentAudioProcessor::loadPresetByName(const std::string& name)
{
    return presetManager.loadPresetByName(name);
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
        "gravity",
        "Gravity",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));

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
