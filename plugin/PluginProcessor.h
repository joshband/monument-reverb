#pragma once

#include <JuceHeader.h>

#include <atomic>

#include "dsp/DspRoutingGraph.h"
#include "dsp/MemoryEchoes.h"
#include "dsp/MacroMapper.h"
#include "dsp/ExpressiveMacroMapper.h"
#include "dsp/ModulationMatrix.h"
#include "PresetManager.h"

/**
 * @brief Processing modes for alternate signal routing
 *
 * Ancient Monuments themed routing modes that reorder physical modeling
 * modules in different positions for dramatic sonic diversity.
 */
enum class ProcessingMode
{
    AncientWay,      // Traditional: Foundation → Pillars → Chambers → Weathering → Physical → Buttress → Facade
    ResonantHalls,   // Metallic: TubeRayTracer BEFORE Chambers (bright metallic resonances)
    BreathingStone   // Elastic: ElasticHallway SURROUNDS Chambers (organic breathing reverb)
};

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

    monument::dsp::ModulationMatrix& getModulationMatrix() { return modulationMatrix; }

    // Processing mode management (Ancient Monuments routing)
    void setProcessingMode(ProcessingMode mode);
    ProcessingMode getProcessingMode() const noexcept;

private:
    enum class PresetTransitionState
    {
        None,
        FadingOut,
        FadingIn
    };

    APVTS parameters;
    PresetManager presetManager;
    int currentProgramIndex{0};  // Track current preset for JUCE program interface
    juce::AudioBuffer<float> dryBuffer;

    // DSP Routing Graph (replaces individual module instances)
    monument::dsp::DspRoutingGraph routingGraph;

    // Separate features (not part of routing graph)
    monument::dsp::MemoryEchoes memoryEchoes;
    monument::dsp::MacroMapper macroMapper;
    monument::dsp::ExpressiveMacroMapper expressiveMacroMapper;
    monument::dsp::ModulationMatrix modulationMatrix;

    // FIXED: Parameter cache for batched atomic loads (reduces overhead from 25+ sequential atomics)
    struct ParameterCache
    {
        float mix, time, mass, density, bloom, air, width;
        float warp, drift, gravity, pillarShape, pillarMode;
        float memory, memoryDepth, memoryDecay, memoryDrift;
        float material, topology, viscosity, evolution;
        float chaosIntensity, elasticityDecay;
        float patina, abyss, corona, breath;  // Ancient Monuments Phase 5 expanded macros
        float character, spaceType, energy, motion, color, dimension;  // Expressive Macros Phase 2
        float tubeCount, radiusVariation, metallicResonance, couplingStrength;
        float elasticity, recoveryTime, absorptionDrift, nonlinearity;
        float impossibilityDegree, pitchEvolutionRate, paradoxResonanceFreq, paradoxGain;
        float routingPreset;  // DSP routing architecture (0-7)
        float macroMode;  // 0 = Ancient Monuments, 1 = Expressive Macros
        bool freeze;
    } paramCache{};

    // Track last routing preset to detect changes
    int lastRoutingPreset{0};

    // Processing mode state (Ancient Monuments routing modes)
    ProcessingMode currentMode{ProcessingMode::AncientWay};
    std::atomic<ProcessingMode> pendingMode{ProcessingMode::AncientWay};
    std::atomic<bool> modeChangeRequested{false};

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

    // Physical modeling parameter smoothers
    juce::SmoothedValue<float> tubeCountSmoother;
    juce::SmoothedValue<float> radiusVariationSmoother;
    juce::SmoothedValue<float> metallicResonanceSmoother;
    juce::SmoothedValue<float> couplingStrengthSmoother;
    juce::SmoothedValue<float> elasticitySmoother;
    juce::SmoothedValue<float> recoveryTimeSmoother;
    juce::SmoothedValue<float> absorptionDriftSmoother;
    juce::SmoothedValue<float> nonlinearitySmoother;
    juce::SmoothedValue<float> impossibilityDegreeSmoother;
    juce::SmoothedValue<float> pitchEvolutionRateSmoother;
    juce::SmoothedValue<float> paradoxResonanceFreqSmoother;
    juce::SmoothedValue<float> paradoxGainSmoother;

    // Processing mode transition gain (prevents clicks on mode change)
    juce::SmoothedValue<float> modeTransitionGain;

    std::atomic<bool> presetResetRequested{false};
    PresetTransitionState presetTransition = PresetTransitionState::None;
    int presetFadeSamples = 0;
    int presetFadeRemaining = 0;
    float presetGain = 1.0f;
#if defined(MONUMENT_MEMORY_PROVE)
    int memoryProvePulseInterval = 0;
    int memoryProvePulseRemaining = 0;
#endif

    // Processing mode implementations (Ancient Monuments routing)
    void processBlockAncientWay(juce::AudioBuffer<float>& buffer);
    void processBlockResonantHalls(juce::AudioBuffer<float>& buffer);
    void processBlockBreathingStone(juce::AudioBuffer<float>& buffer);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MonumentAudioProcessor)
};
