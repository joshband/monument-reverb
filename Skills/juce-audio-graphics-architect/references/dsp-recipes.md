# DSP Recipes for JUCE Plugins

## Table of Contents
1. Parameter layout and attachments
2. DSP chain pattern (reverb + delay + physical modeling)
3. Parameter smoothing
4. FFT capture pipeline
5. Audio-to-UI ring buffer
6. Metering helpers

## 1. Parameter layout and attachments
Define parameters once and bind them in the editor.

```cpp
static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using Param = juce::AudioParameterFloat;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<Param>(
        "mix",
        "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));

    params.push_back(std::make_unique<Param>(
        "delayTimeMs",
        "Delay Time",
        juce::NormalisableRange<float>(1.0f, 2000.0f, 1.0f, 0.5f),
        350.0f));

    params.push_back(std::make_unique<Param>(
        "reverbSize",
        "Reverb Size",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.6f));

    return { params.begin(), params.end() };
}
```

## 2. DSP chain pattern (reverb + delay + physical modeling)
Use a `ProcessorChain` for clear ordering. For physical modeling, start with a Karplus-Strong style delay and damping filter.

```cpp
using Delay = juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>;
using Reverb = juce::dsp::Reverb;

struct PhysicalModel
{
    void prepare(const juce::dsp::ProcessSpec& spec)
    {
        delay = Delay(static_cast<int>(spec.sampleRate));
        damping.reset();
        feedback = 0.98f;
    }

    void setFrequency(float frequency, double sampleRate)
    {
        const auto delaySamples = static_cast<float>(sampleRate / frequency);
        delay.setDelay(delaySamples);
    }

    float processSample(float input)
    {
        const float delayed = delay.popSample(0);
        const float next = (input + delayed) * 0.5f;
        const float damped = damping.processSample(next);
        delay.pushSample(0, damped * feedback);
        return delayed;
    }

    Delay delay { 48000 };
    juce::dsp::IIR::Filter<float> damping;
    float feedback = 0.98f;
};

using Chain = juce::dsp::ProcessorChain<PhysicalModel, Delay, Reverb>;
```

## 3. Parameter smoothing
Smooth parameters on the audio thread to avoid zipper noise.

```cpp
juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed;

void prepareToPlay(double sampleRate, int samplesPerBlock)
{
    mixSmoothed.reset(sampleRate, 0.05); // 50ms smoothing
}

void updateParams(float mix)
{
    mixSmoothed.setTargetValue(mix);
}

float nextMix = mixSmoothed.getNextValue();
```

## 4. FFT capture pipeline
Push samples into a FIFO, then run FFT on the UI or a background thread.

```cpp
constexpr int fftOrder = 11;
constexpr int fftSize = 1 << fftOrder;

juce::dsp::FFT fft { fftOrder };
juce::dsp::WindowingFunction<float> window { fftSize, juce::dsp::WindowingFunction<float>::hann };

std::array<float, fftSize> fifo {};
std::array<float, fftSize * 2> fftData {};
int fifoIndex = 0;
std::atomic<bool> nextFftReady { false };

void pushNextSample(float sample)
{
    fifo[fifoIndex++] = sample;

    if (fifoIndex == fftSize)
    {
        std::copy(fifo.begin(), fifo.end(), fftData.begin());
        fifoIndex = 0;
        nextFftReady.store(true, std::memory_order_release);
    }
}

void performFft()
{
    if (!nextFftReady.exchange(false, std::memory_order_acq_rel))
        return;

    window.multiplyWithWindowingTable(fftData.data(), fftSize);
    fft.performFrequencyOnlyForwardTransform(fftData.data());
}
```

## 5. Audio-to-UI ring buffer
Use `juce::AbstractFifo` for lock-free transfer to the UI thread.

```cpp
class AudioFifo
{
public:
    explicit AudioFifo(int capacity)
        : fifo(capacity), buffer(1, capacity) {}

    void push(const float* data, int numSamples)
    {
        int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
        fifo.prepareToWrite(numSamples, start1, size1, start2, size2);
        if (size1 > 0) buffer.copyFrom(0, start1, data, size1);
        if (size2 > 0) buffer.copyFrom(0, start2, data + size1, size2);
        fifo.finishedWrite(size1 + size2);
    }

    int pull(float* dest, int numSamples)
    {
        int start1 = 0, size1 = 0, start2 = 0, size2 = 0;
        fifo.prepareToRead(numSamples, start1, size1, start2, size2);
        if (size1 > 0) juce::FloatVectorOperations::copy(dest, buffer.getReadPointer(0, start1), size1);
        if (size2 > 0) juce::FloatVectorOperations::copy(dest + size1, buffer.getReadPointer(0, start2), size2);
        fifo.finishedRead(size1 + size2);
        return size1 + size2;
    }

private:
    juce::AbstractFifo fifo;
    juce::AudioBuffer<float> buffer;
};
```

## 6. Metering helpers
Compute RMS/peak for audio-reactive visuals.

```cpp
float computeRms(const juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const float* channel = buffer.getReadPointer(0);
    float sum = 0.0f;
    for (int i = 0; i < numSamples; ++i)
        sum += channel[i] * channel[i];
    return std::sqrt(sum / static_cast<float>(numSamples));
}
```

## Notes
- Keep all heap allocations out of `processBlock`.
- If `DelayLine::popSample` or `pushSample` signatures differ in your JUCE version, adjust accordingly.

## Related references
- `references/skill_package/algorithms/` for FDN, delay line, and reverb/echo notes.
- `references/skill_package/devices/` for modern delay/reverb device inspiration.
