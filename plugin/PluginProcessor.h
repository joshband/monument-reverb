#pragma once

#include <JuceHeader.h>

#include <atomic>

#include "dsp/DspModules.h"
#include "dsp/MemoryEchoes.h"
#include "dsp/MacroMapper.h"
#include "dsp/ModulationMatrix.h"
#include "PresetManager.h"

class MonumentAudioProcessor : public juce::AudioProcessor
{
public:
    MonumentAudioProcessor();
    ~MonumentAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    using APVTS = juce::AudioProcessorValueTreeState;

    APVTS& getAPVTS();
    static APVTS::ParameterLayout createParameterLayout();

    int getNumFactoryPresets() const;
    juce::String getFactoryPresetName(int index) const;
    juce::String getFactoryPresetDescription(int index) const;
    void loadFactoryPreset(int index);
    void saveUserPreset(const juce::String& name, const juce::String& description);
    void saveUserPreset(const juce::File& targetFile,
                        const juce::String& name,
                        const juce::String& description);
    void loadUserPreset(const juce::File& sourceFile);

private:
    enum class PresetTransitionState
    {
        None,
        FadingOut,
        FadingIn
    };

    APVTS parameters;
    PresetManager presetManager;
    juce::AudioBuffer<float> dryBuffer;
    monument::dsp::Foundation foundation;
    monument::dsp::Pillars pillars;
    monument::dsp::Chambers chambers;
    monument::dsp::MemoryEchoes memoryEchoes;
    monument::dsp::Weathering weathering;
    monument::dsp::Buttress buttress;
    monument::dsp::Facade facade;
    monument::dsp::MacroMapper macroMapper;
    monument::dsp::ModulationMatrix modulationMatrix;

    // JUCE SmoothedValue for block-rate parameter smoothing (prevents zipper noise)
    juce::SmoothedValue<float> timeSmoother;
    juce::SmoothedValue<float> massSmoother;
    juce::SmoothedValue<float> densitySmoother;
    juce::SmoothedValue<float> bloomSmoother;
    juce::SmoothedValue<float> airSmoother;
    juce::SmoothedValue<float> widthSmoother;
    juce::SmoothedValue<float> warpSmoother;
    juce::SmoothedValue<float> driftSmoother;
    juce::SmoothedValue<float> gravitySmoother;
    juce::SmoothedValue<float> pillarShapeSmoother;

    std::atomic<bool> presetResetRequested{false};
    PresetTransitionState presetTransition = PresetTransitionState::None;
    int presetFadeSamples = 0;
    int presetFadeRemaining = 0;
    float presetGain = 1.0f;
#if defined(MONUMENT_MEMORY_PROVE)
    int memoryProvePulseInterval = 0;
    int memoryProvePulseRemaining = 0;
#endif

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MonumentAudioProcessor)
};
