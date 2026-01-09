#pragma once

#include <JuceHeader.h>

class @PLUGIN_NAME@AudioProcessor : public juce::AudioProcessor
{
public:
    @PLUGIN_NAME@AudioProcessor();
    ~@PLUGIN_NAME@AudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    double getTailLengthSeconds() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    struct FftAnalyzer
    {
        static constexpr int fftOrder = 11;
        static constexpr int fftSize = 1 << fftOrder;

        void pushSamples(const juce::AudioBuffer<float>& buffer);
        bool popMagnitudes(std::array<float, fftSize>& outMagnitudes);

    private:
        void pushNextSample(float sample);

        juce::dsp::FFT fft { fftOrder };
        juce::dsp::WindowingFunction<float> window {
            fftSize, juce::dsp::WindowingFunction<float>::hann
        };

        std::array<float, fftSize> fifo {};
        std::array<float, fftSize * 2> fftData {};
        int fifoIndex = 0;
        std::atomic<bool> nextFftReady { false };
    };

    FftAnalyzer& getFftAnalyzer() { return fftAnalyzer; }

private:
    using Delay = juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>;
    using Reverb = juce::dsp::Reverb;

    struct DelayWithFeedback
    {
        void prepare(const juce::dsp::ProcessSpec& spec)
        {
            sampleRate = spec.sampleRate;
            delay.reset();
        }

        void setParams(float delayMs, float feedbackAmount, float mix)
        {
            const auto delaySamples = delayMs * 0.001f * static_cast<float>(sampleRate);
            delay.setDelay(delaySamples);
            feedback = juce::jlimit(0.0f, 0.98f, feedbackAmount);
            wetMix = juce::jlimit(0.0f, 1.0f, mix);
        }

        void process(juce::AudioBuffer<float>& buffer)
        {
            const int channels = buffer.getNumChannels();
            const int samples = buffer.getNumSamples();
            for (int ch = 0; ch < channels; ++ch)
            {
                auto* data = buffer.getWritePointer(ch);
                for (int i = 0; i < samples; ++i)
                {
                    const float input = data[i];
                    const float delayed = delay.popSample(ch);
                    const float next = input + delayed * feedback;
                    delay.pushSample(ch, next);
                    data[i] = input * (1.0f - wetMix) + delayed * wetMix;
                }
            }
        }

        Delay delay { 192000 };
        double sampleRate = 44100.0;
        float feedback = 0.35f;
        float wetMix = 0.5f;
    };

    DelayWithFeedback delay;
    Reverb reverb;
    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> mixSmoothed;
    juce::AudioBuffer<float> dryBuffer;

    FftAnalyzer fftAnalyzer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(@PLUGIN_NAME@AudioProcessor)
};
