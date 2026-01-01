#include "PluginProcessor.h"
#include "PluginEditor.h"

MonumentAudioProcessor::MonumentAudioProcessor()
    : juce::AudioProcessor(BusesProperties()
                                .withInput("Input", juce::AudioChannelSet::stereo(), true)
                                .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
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
    return 10.0;
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
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());
    dsp.prepare(spec);
}

void MonumentAudioProcessor::releaseResources()
{
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

    const auto time = parameters.getRawParameterValue("time")->load();
    const auto mix = parameters.getRawParameterValue("mix")->load();

    dsp.setParameters(time, mix);
    dsp.process(buffer);
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

MonumentAudioProcessor::APVTS::ParameterLayout MonumentAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "time",
        "Time",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.55f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "mix",
        "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.35f));

    return {params.begin(), params.end()};
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MonumentAudioProcessor();
}
