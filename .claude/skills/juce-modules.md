# JUCE Modules Reference

Quick reference for essential JUCE audio APIs.

## juce::dsp Module

### ProcessSpec
```cpp
juce::dsp::ProcessSpec spec;
spec.sampleRate = sampleRate;
spec.maximumBlockSize = (juce::uint32)blockSize;
spec.numChannels = (juce::uint32)numChannels;

filter.prepare(spec);
```

### Built-in Processors
```cpp
// IIR Filter
juce::dsp::IIR::Filter<float> filter;
filter.coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sr, freq, Q);

// State Variable (TPT)
juce::dsp::StateVariableTPTFilter<float> svf;
svf.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
svf.setCutoffFrequency(1000.0f);

// Gain
juce::dsp::Gain<float> gain;
gain.setGainDecibels(-6.0f);

// Compressor
juce::dsp::Compressor<float> comp;
comp.setThreshold(-20.0f);
comp.setRatio(4.0f);
comp.setAttack(10.0f);
comp.setRelease(100.0f);

// Delay
juce::dsp::DelayLine<float> delay{44100};
delay.setDelay(22050.0f);

// Waveshaper
juce::dsp::WaveShaper<float> shaper;
shaper.functionToUse = [](float x) { return std::tanh(x); };

// Convolution
juce::dsp::Convolution conv;
conv.loadImpulseResponse(file, Stereo::yes, Trim::yes, size);
```

### Processing Contexts
```cpp
// Replacing (in-place)
juce::dsp::AudioBlock<float> block(buffer);
juce::dsp::ProcessContextReplacing<float> ctx(block);
filter.process(ctx);

// Non-replacing
juce::dsp::ProcessContextNonReplacing<float> ctx(inBlock, outBlock);
```

### Processor Chain
```cpp
using Chain = juce::dsp::ProcessorChain<
    juce::dsp::IIR::Filter<float>,
    juce::dsp::Gain<float>,
    juce::dsp::Compressor<float>
>;

Chain chain;
auto& filter = chain.get<0>();
auto& gain = chain.get<1>();

chain.prepare(spec);
chain.process(ctx);
```

### Oversampling
```cpp
juce::dsp::Oversampling<float> os{2, 2,  // channels, factor (2^2=4x)
    juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR};

void prepare(double sr, int bs) {
    os.initProcessing(bs);
}

void process(juce::AudioBuffer<float>& buf) {
    juce::dsp::AudioBlock<float> block(buf);
    auto upsampled = os.processSamplesUp(block);
    // Process at higher rate
    os.processSamplesDown(block);
}
```

## AudioProcessorValueTreeState

### Parameter Layout
```cpp
juce::AudioProcessorValueTreeState::ParameterLayout createParams() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
    
    p.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"gain", 1}, "Gain",
        juce::NormalisableRange<float>(-60.f, 12.f, 0.1f), 0.f));
    
    p.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"type", 1}, "Type",
        juce::StringArray{"A", "B", "C"}, 0));
    
    p.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"bypass", 1}, "Bypass", false));
    
    return {p.begin(), p.end()};
}
```

### Access in processBlock
```cpp
float gain = apvts.getRawParameterValue("gain")->load();
```

### UI Attachments
```cpp
juce::AudioProcessorValueTreeState::SliderAttachment gainAtt{apvts, "gain", slider};
juce::AudioProcessorValueTreeState::ComboBoxAttachment typeAtt{apvts, "type", combo};
juce::AudioProcessorValueTreeState::ButtonAttachment bypAtt{apvts, "bypass", button};
```

## SmoothedValue

```cpp
juce::SmoothedValue<float> gain;

void prepare(double sr) {
    gain.reset(sr, 0.02);  // 20ms smoothing
}

void process() {
    gain.setTargetValue(targetGain);
    
    for (int i = 0; i < numSamples; ++i) {
        float g = gain.getNextValue();
        // Apply g
    }
}
```

## MIDI Processing

```cpp
void processBlock(juce::AudioBuffer<float>& buf, juce::MidiBuffer& midi) {
    for (const auto meta : midi) {
        auto msg = meta.getMessage();
        int sample = meta.samplePosition;
        
        if (msg.isNoteOn()) {
            int note = msg.getNoteNumber();
            float vel = msg.getFloatVelocity();
        }
        else if (msg.isNoteOff()) {
            int note = msg.getNoteNumber();
        }
        else if (msg.isController()) {
            int cc = msg.getControllerNumber();
            int val = msg.getControllerValue();
        }
        else if (msg.isPitchWheel()) {
            int pw = msg.getPitchWheelValue();  // 0-16383, center=8192
        }
    }
}
```

## Utility Functions

```cpp
// Decibels
float db = juce::Decibels::gainToDecibels(linear);
float lin = juce::Decibels::decibelsToGain(db);

// MIDI to frequency
float freq = 440.0f * std::pow(2.0f, (midiNote - 69) / 12.0f);

// Time to samples
int samples = (int)(timeMs * 0.001 * sampleRate);

// Denormal protection
juce::ScopedNoDenormals noDenormals;
```

## State Save/Restore

```cpp
void getStateInformation(juce::MemoryBlock& dest) override {
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, dest);
}

void setStateInformation(const void* data, int size) override {
    auto xml = getXmlFromBinary(data, size);
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}
```
