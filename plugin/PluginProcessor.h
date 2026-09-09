#pragma once

#include <JuceHeader.h>

#include <atomic>

#include "dsp/DspRoutingGraph.h"
#include "dsp/MemoryEchoes.h"
#include "dsp/MacroMapper.h"
#include "dsp/ExpressiveMacroMapper.h"
#include "dsp/ModulationMatrix.h"
#include "dsp/SequenceScheduler.h"
#include "dsp/ParameterBuffers.h"
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
    void reset() override;

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
    monument::dsp::SequenceScheduler& getSequenceScheduler() { return sequenceScheduler; }  // Phase 4

    // Processing mode management (Ancient Monuments routing)
    void setProcessingMode(ProcessingMode mode);
    ProcessingMode getProcessingMode() const noexcept;

    float getInputLevel() const noexcept { return inputLevel.load(std::memory_order_relaxed); }
    float getOutputLevel() const noexcept { return outputLevel.load(std::memory_order_relaxed); }

#if defined(MONUMENT_TESTING)
    int getLastRoutingPresetForTesting() const noexcept { return lastRoutingPreset; }
#endif

private:
    enum class PresetTransitionState
    {
        None,
        FadingOut,
        FadingIn
    };

    enum class ModeTransitionState
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
    monument::dsp::SequenceScheduler sequenceScheduler;  // Phase 4: Timeline automation
    // Factory timeline presets, populated once in prepareToPlay() (off the audio thread).
    // processBlock() aliases into this via SequenceScheduler::loadSequenceRef() to avoid
    // constructing/copying a Sequence on every timelinePreset change.
    std::vector<monument::dsp::SequenceScheduler::Sequence> cachedTimelineSequences;

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
        float warpClustering;  // Chambers FDN delay-clustering mode (0-3, see Chambers::WarpClusteringMode)
        float densityEvolution;  // Chambers echo density drift over the decay (-1..1)
        float attackTime;  // Chambers slow-attack swell time (0..1)
        float macroMode;  // 0 = Ancient Monuments, 1 = Expressive Macros
        float safetyClipDrive;  // Output soft clip intensity (0..1)
        float timelinePreset;  // Timeline preset index
        bool safetyClip;
        bool timelineEnabled;
        bool freeze;
    } paramCache{};

    // Cached raw-parameter atomic pointers, resolved once (constructor) instead
    // of via ~54 string-keyed APVTS::getRawParameterValue() lookups every block.
    // Each pointer aliases the same std::atomic<float> the string lookup would
    // return; JUCE keeps these valid for the processor's lifetime, so caching
    // them is safe and the audio thread only ever performs the atomic load.
    struct ParameterPointerCache
    {
        std::atomic<float>* mix{};
        std::atomic<float>* time{};
        std::atomic<float>* mass{};
        std::atomic<float>* density{};
        std::atomic<float>* bloom{};
        std::atomic<float>* air{};
        std::atomic<float>* width{};
        std::atomic<float>* warp{};
        std::atomic<float>* drift{};
        std::atomic<float>* gravity{};
        std::atomic<float>* pillarShape{};
        std::atomic<float>* pillarMode{};
        std::atomic<float>* memory{};
        std::atomic<float>* memoryDepth{};
        std::atomic<float>* memoryDecay{};
        std::atomic<float>* memoryDrift{};
        std::atomic<float>* material{};
        std::atomic<float>* topology{};
        std::atomic<float>* viscosity{};
        std::atomic<float>* evolution{};
        std::atomic<float>* chaosIntensity{};
        std::atomic<float>* elasticityDecay{};
        std::atomic<float>* patina{};
        std::atomic<float>* abyss{};
        std::atomic<float>* corona{};
        std::atomic<float>* breath{};
        std::atomic<float>* character{};
        std::atomic<float>* spaceType{};
        std::atomic<float>* energy{};
        std::atomic<float>* motion{};
        std::atomic<float>* color{};
        std::atomic<float>* dimension{};
        std::atomic<float>* tubeCount{};
        std::atomic<float>* radiusVariation{};
        std::atomic<float>* metallicResonance{};
        std::atomic<float>* couplingStrength{};
        std::atomic<float>* elasticity{};
        std::atomic<float>* recoveryTime{};
        std::atomic<float>* absorptionDrift{};
        std::atomic<float>* nonlinearity{};
        std::atomic<float>* impossibilityDegree{};
        std::atomic<float>* pitchEvolutionRate{};
        std::atomic<float>* paradoxResonanceFreq{};
        std::atomic<float>* paradoxGain{};
        std::atomic<float>* routingPreset{};
        std::atomic<float>* warpClustering{};
        std::atomic<float>* densityEvolution{};
        std::atomic<float>* attackTime{};
        std::atomic<float>* macroMode{};
        std::atomic<float>* safetyClip{};
        std::atomic<float>* safetyClipDrive{};
        std::atomic<float>* timelineEnabled{};
        std::atomic<float>* timelinePreset{};
        std::atomic<float>* freeze{};
    } paramPointers{};

    // Populates paramPointers from `parameters`. Called once from the
    // constructor (after APVTS is fully constructed); never called from
    // processBlockCore/prepareToPlay's hot path.
    void cacheParameterPointers();

    // Track last routing preset to detect changes
    int lastRoutingPreset{0};
    int lastTimelinePreset{-1};
    bool lastTimelineEnabled{false};

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
    juce::SmoothedValue<float> mixSmoother;
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

    // Output safety-clip stage smoothers: `safetyClipDrive` and the
    // `safetyClip` on/off switch were previously applied straight from the
    // raw atomic parameter value with no interpolation, so a per-block
    // change (drive) or an on/off toggle (enable) produced a step
    // discontinuity in the output transfer function. safetyClipDriveSmoother
    // interpolates the drive amount; safetyClipEnableSmoother crossfades
    // between the unclipped and clipped signal instead of switching instantly.
    juce::SmoothedValue<float> safetyClipDriveSmoother;
    juce::SmoothedValue<float> safetyClipEnableSmoother;

    // Smoother activity tracking bitmask (optimization: skip inactive smoothers)
    // Each bit represents one smoother (0-21); bit=1 means smoother was active last frame
    uint32_t activeSmoothers{0};

    // Per-sample parameter buffers (Phase 4: Per-sample interpolation)
    // Stack-allocated pool (64KB, cache-aligned) for critical parameters
    ParameterBufferPool paramBufferPool;

    // Block size declared via the most recent prepareToPlay() call. Every
    // internal buffer (dryBuffer, routingGraph's internal buffers, DSP module
    // buffers) is sized to exactly this value, so it - not
    // paramBufferPool.capacity(), which can be larger - is the true safety
    // ceiling processBlock() must clamp to for an out-of-contract oversized
    // block. See processBlock()'s zero/oversized-block contract.
    int preparedBlockSize{0};

    // Processing mode transition gain (prevents clicks on mode change)
    juce::SmoothedValue<float> modeTransitionGain;
    ModeTransitionState modeTransitionState{ModeTransitionState::None};

    std::atomic<bool> presetResetRequested{false};
    PresetTransitionState presetTransition = PresetTransitionState::None;
    int presetFadeSamples = 0;
    int presetFadeRemaining = 0;
    float presetGain = 1.0f;
#if defined(MONUMENT_TESTING_VERBOSE_LOG)
    bool testingLoggerRegistered{false};
#endif

    // Processing mode implementations (Ancient Monuments routing)
    void processBlockAncientWay(juce::AudioBuffer<float>& buffer);
    void processBlockResonantHalls(juce::AudioBuffer<float>& buffer);
    void processBlockBreathingStone(juce::AudioBuffer<float>& buffer);

    // The actual processing body (unchanged), operating on a buffer already
    // known to be non-empty and within preparedBlockSize. See processBlock()
    // for the realtime-safe zero/oversized-block contract.
    void processBlockCore(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages);

    std::atomic<float> inputLevel{0.0f};
    std::atomic<float> outputLevel{0.0f};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MonumentAudioProcessor)
};
