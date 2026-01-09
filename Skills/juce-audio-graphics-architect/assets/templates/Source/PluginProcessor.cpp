#include "PluginProcessor.h"
#include "PluginEditor.h"

@PLUGIN_NAME@AudioProcessor::@PLUGIN_NAME@AudioProcessor()
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
      ),
      apvts(*this, nullptr, "PARAMS", createParameterLayout())
{
}

@PLUGIN_NAME@AudioProcessor::~@PLUGIN_NAME@AudioProcessor() = default;

const juce::String @PLUGIN_NAME@AudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool @PLUGIN_NAME@AudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool @PLUGIN_NAME@AudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool @PLUGIN_NAME@AudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double @PLUGIN_NAME@AudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int @PLUGIN_NAME@AudioProcessor::getNumPrograms()
{
    return 1;
}

int @PLUGIN_NAME@AudioProcessor::getCurrentProgram()
{
    return 0;
}

void @PLUGIN_NAME@AudioProcessor::setCurrentProgram(int)
{
}

const juce::String @PLUGIN_NAME@AudioProcessor::getProgramName(int)
{
    return {};
}

void @PLUGIN_NAME@AudioProcessor::changeProgramName(int, const juce::String&)
{
}

void @PLUGIN_NAME@AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

    delay.prepare(spec);
    reverb.prepare(spec);
    reverb.reset();

    mixSmoothed.reset(sampleRate, 0.05);
    dryBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock);
}

void @PLUGIN_NAME@AudioProcessor::releaseResources()
{
}

bool @PLUGIN_NAME@AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
}

void @PLUGIN_NAME@AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int totalNumInputChannels = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    fftAnalyzer.pushSamples(buffer);
    dryBuffer.makeCopyOf(buffer, true);

    const float mix = apvts.getRawParameterValue("mix")->load();
    const float delayTimeMs = apvts.getRawParameterValue("delayTimeMs")->load();
    const float delayFeedback = apvts.getRawParameterValue("delayFeedback")->load();
    const float reverbSize = apvts.getRawParameterValue("reverbSize")->load();
    const float reverbDamping = apvts.getRawParameterValue("reverbDamping")->load();
    const float reverbMix = apvts.getRawParameterValue("reverbMix")->load();

    mixSmoothed.setTargetValue(mix);

    delay.setParams(delayTimeMs, delayFeedback, 1.0f);
    delay.process(buffer);

    juce::dsp::Reverb::Parameters params;
    params.roomSize = reverbSize;
    params.damping = reverbDamping;
    params.wetLevel = reverbMix;
    params.dryLevel = 1.0f - reverbMix;
    params.width = 1.0f;
    reverb.setParameters(params);

    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    reverb.process(context);

    const int numChannels = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float mixValue = mixSmoothed.getNextValue();
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float* wet = buffer.getWritePointer(ch);
            const float* dry = dryBuffer.getReadPointer(ch);
            wet[sample] = dry[sample] + (wet[sample] - dry[sample]) * mixValue;
        }
    }
}

bool @PLUGIN_NAME@AudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* @PLUGIN_NAME@AudioProcessor::createEditor()
{
    return new @PLUGIN_NAME@AudioProcessorEditor(*this);
}

void @PLUGIN_NAME@AudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        std::unique_ptr<juce::XmlElement> xml(state.createXml());
        copyXmlToBinary(*xml, destData);
    }
}

void @PLUGIN_NAME@AudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorValueTreeState::ParameterLayout
@PLUGIN_NAME@AudioProcessor::createParameterLayout()
{
    using Param = juce::AudioParameterFloat;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<Param>(
        "mix", "Mix", juce::NormalisableRange<float>(0.0f, 1.0f), 0.5f));
    params.push_back(std::make_unique<Param>(
        "delayTimeMs", "Delay Time", juce::NormalisableRange<float>(1.0f, 2000.0f, 1.0f, 0.5f), 350.0f));
    params.push_back(std::make_unique<Param>(
        "delayFeedback", "Delay Feedback", juce::NormalisableRange<float>(0.0f, 0.95f), 0.35f));
    params.push_back(std::make_unique<Param>(
        "reverbSize", "Reverb Size", juce::NormalisableRange<float>(0.0f, 1.0f), 0.6f));
    params.push_back(std::make_unique<Param>(
        "reverbDamping", "Reverb Damping", juce::NormalisableRange<float>(0.0f, 1.0f), 0.4f));
    params.push_back(std::make_unique<Param>(
        "reverbMix", "Reverb Mix", juce::NormalisableRange<float>(0.0f, 1.0f), 0.35f));

    return { params.begin(), params.end() };
}

void @PLUGIN_NAME@AudioProcessor::FftAnalyzer::pushSamples(const juce::AudioBuffer<float>& buffer)
{
    const float* channel = buffer.getReadPointer(0);
    const int numSamples = buffer.getNumSamples();
    for (int i = 0; i < numSamples; ++i)
        pushNextSample(channel[i]);
}

bool @PLUGIN_NAME@AudioProcessor::FftAnalyzer::popMagnitudes(std::array<float, fftSize>& outMagnitudes)
{
    if (!nextFftReady.exchange(false, std::memory_order_acq_rel))
        return false;

    window.multiplyWithWindowingTable(fftData.data(), fftSize);
    fft.performFrequencyOnlyForwardTransform(fftData.data());

    std::copy_n(fftData.begin(), fftSize, outMagnitudes.begin());
    return true;
}

void @PLUGIN_NAME@AudioProcessor::FftAnalyzer::pushNextSample(float sample)
{
    fifo[fifoIndex++] = sample;

    if (fifoIndex == fftSize)
    {
        std::copy(fifo.begin(), fifo.end(), fftData.begin());
        std::fill(fftData.begin() + fftSize, fftData.end(), 0.0f);
        fifoIndex = 0;
        nextFftReady.store(true, std::memory_order_release);
    }
}
