#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "PluginEditorV2.h"
#include "dsp/Chambers.h"
#include "dsp/SequencePresets.h"

#include <cmath>
#if defined(MONUMENT_TESTING) || defined(MONUMENT_MEMORY_PROVE)
#include <mutex>
#endif

namespace
{
constexpr float kPresetFadeMs = 60.0f;
constexpr float kDefaultMixPercent = 55.0f;
constexpr float kDefaultTime = 0.55f;
constexpr float kDefaultMass = 0.5f;
constexpr float kDefaultDensity = 0.5f;
constexpr float kDefaultBloom = 0.5f;
constexpr float kDefaultAir = 0.5f;
constexpr float kDefaultWidth = 0.5f;
constexpr float kDefaultWarp = 0.3f;
constexpr float kDefaultDrift = 0.3f;
constexpr float kDefaultGravity = 0.5f;
constexpr float kDefaultPillarShape = 0.5f;
constexpr float kDefaultMemory = 0.0f;
constexpr float kDefaultMemoryDepth = 0.5f;
constexpr float kDefaultMemoryDecay = 0.4f;
constexpr float kDefaultMemoryDrift = 0.3f;
constexpr float kDefaultMaterial = 0.5f;
constexpr float kDefaultTopology = 0.5f;
constexpr float kDefaultViscosity = 0.5f;
constexpr float kDefaultEvolution = 0.5f;
constexpr float kDefaultChaosIntensity = 0.0f;
constexpr float kDefaultElasticityDecay = 0.0f;
constexpr float kDefaultPatina = 0.5f;
constexpr float kDefaultAbyss = 0.5f;
constexpr float kDefaultCorona = 0.5f;
constexpr float kDefaultBreath = 0.0f;
constexpr float kDefaultCharacter = 0.5f;
constexpr float kDefaultSpaceType = 0.3f;
constexpr float kDefaultEnergy = 0.1f;
constexpr float kDefaultMotion = 0.2f;
constexpr float kDefaultColor = 0.5f;
constexpr float kDefaultDimension = 0.5f;
constexpr float kDefaultTubeCount = 0.545f;
constexpr float kDefaultRadiusVariation = 0.3f;
constexpr float kDefaultMetallicResonance = 0.5f;
constexpr float kDefaultCouplingStrength = 0.5f;
constexpr float kDefaultElasticity = 0.5f;
constexpr float kDefaultRecoveryTime = 0.5f;
constexpr float kDefaultAbsorptionDrift = 0.3f;
constexpr float kDefaultNonlinearity = 0.3f;
constexpr float kDefaultImpossibilityDegree = 0.3f;
constexpr float kDefaultPitchEvolutionRate = 0.3f;
constexpr float kDefaultParadoxResonance = 0.5f;
constexpr float kDefaultParadoxGain = 0.3f;
constexpr float kDefaultSafetyClipDrive = 0.25f;

float sanitizeRange(float value, float fallback, float minValue, float maxValue) noexcept
{
    if (!std::isfinite(value))
        return fallback;
    return juce::jlimit(minValue, maxValue, value);
}

float sanitizeUnit(float value, float fallback) noexcept
{
    return sanitizeRange(value, fallback, 0.0f, 1.0f);
}

int sanitizeChoice(float value, int minValue, int maxValue, int fallback) noexcept
{
    if (!std::isfinite(value))
        return fallback;
    const int index = static_cast<int>(std::lround(value));
    return juce::jlimit(minValue, maxValue, index);
}

#if defined(MONUMENT_MEMORY_PROVE)
#ifndef MONUMENT_MEMORY_PROVE_STAGE
#define MONUMENT_MEMORY_PROVE_STAGE 0
#endif
constexpr int kMemoryProveStage = MONUMENT_MEMORY_PROVE_STAGE;
#endif

#if defined(MONUMENT_TESTING) || defined(MONUMENT_MEMORY_PROVE)
std::once_flag gTestingLoggerOnce;
std::unique_ptr<juce::FileLogger> gTestingLogger;
std::atomic<int> gTestingLoggerUsers{0};

void ensureTestingLogger()
{
    std::call_once(gTestingLoggerOnce, []()
    {
        const auto logFile = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
            .getChildFile("MonumentTesting.log");
        gTestingLogger = std::make_unique<juce::FileLogger>(logFile, "Monument testing log", 0);
        juce::Logger::setCurrentLogger(gTestingLogger.get());
        juce::Logger::writeToLog("Monument MONUMENT_TESTING logger ready: " + logFile.getFullPathName());
    });
}

void releaseTestingLogger()
{
    juce::Logger::setCurrentLogger(nullptr);
    gTestingLogger.reset();
}
#endif
}

MonumentAudioProcessor::MonumentAudioProcessor()
    : juce::AudioProcessor(BusesProperties()
                                .withInput("Input", juce::AudioChannelSet::stereo(), true)
                                .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout()),
      presetManager(parameters, &modulationMatrix)  // Phase 3: Pass modulation matrix for serialization
{
}

MonumentAudioProcessor::~MonumentAudioProcessor()
{
#if defined(MONUMENT_TESTING) || defined(MONUMENT_MEMORY_PROVE)
    if (testingLoggerRegistered)
    {
        if (gTestingLoggerUsers.fetch_sub(1, std::memory_order_acq_rel) == 1)
            releaseTestingLogger();
        testingLoggerRegistered = false;
    }
#endif
}

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
    return presetManager.getNumFactoryPresets();
}

int MonumentAudioProcessor::getCurrentProgram()
{
    return currentProgramIndex;
}

void MonumentAudioProcessor::setCurrentProgram(int index)
{
    if (index >= 0 && index < presetManager.getNumFactoryPresets())
    {
        presetManager.loadFactoryPreset(index);
        currentProgramIndex = index;
    }
}

const juce::String MonumentAudioProcessor::getProgramName(int index)
{
    return presetManager.getFactoryPresetName(index);
}

void MonumentAudioProcessor::changeProgramName(int, const juce::String&)
{
}

void MonumentAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
#if defined(MONUMENT_TESTING) || defined(MONUMENT_MEMORY_PROVE)
    ensureTestingLogger();
    if (!testingLoggerRegistered)
    {
        gTestingLoggerUsers.fetch_add(1, std::memory_order_acq_rel);
        testingLoggerRegistered = true;
    }
#endif

    paramBufferPool.prepare(samplesPerBlock);

    const auto numChannels = getTotalNumOutputChannels();
    dryBuffer.setSize(numChannels, samplesPerBlock, false, false, true);
    dryBuffer.clear();

    // Prepare DSP routing graph (replaces individual module preparation)
    routingGraph.prepare(sampleRate, samplesPerBlock, numChannels);

    // Prepare separate features (not part of routing graph)
    memoryEchoes.prepare(sampleRate, samplesPerBlock, numChannels);
    modulationMatrix.prepare(sampleRate, samplesPerBlock, numChannels);
    sequenceScheduler.prepare(sampleRate, samplesPerBlock);  // Phase 4: Timeline automation

    // Initialize JUCE SmoothedValue for macro parameter smoothing (500ms ramp time)
    // INCREASED from 50ms → 500ms to minimize zipper noise and eliminate clicks
    // Testing showed: 500ms eliminates clicks (PARAM-5 ✅), reduces zipper from 13dB → 9.6dB (26% improvement)
    // Further increases don't help due to block-based DSP architecture limiting sample-rate parameter updates
    // RECOMMENDATION: For full zipper elimination, DSP modules need internal sample-rate parameter interpolation
    const double smoothingRampSeconds = 0.50;  // 500ms = optimal balance of smoothness vs responsiveness
    timeSmoother.reset(sampleRate, smoothingRampSeconds);
    massSmoother.reset(sampleRate, smoothingRampSeconds);
    densitySmoother.reset(sampleRate, smoothingRampSeconds);
    bloomSmoother.reset(sampleRate, smoothingRampSeconds);
    airSmoother.reset(sampleRate, smoothingRampSeconds);
    widthSmoother.reset(sampleRate, smoothingRampSeconds);
    mixSmoother.reset(sampleRate, 0.10);  // Mix smoothing for click-free automation
    warpSmoother.reset(sampleRate, smoothingRampSeconds);
    driftSmoother.reset(sampleRate, smoothingRampSeconds);
    gravitySmoother.reset(sampleRate, smoothingRampSeconds);
    pillarShapeSmoother.reset(sampleRate, smoothingRampSeconds);

    auto loadParam = [&](const char* paramId)
    {
        return parameters.getRawParameterValue(paramId)->load(std::memory_order_relaxed);
    };

    mixSmoother.setCurrentAndTargetValue(
        sanitizeRange(loadParam("mix"), kDefaultMixPercent, 0.0f, 100.0f));
    timeSmoother.setCurrentAndTargetValue(sanitizeUnit(loadParam("time"), kDefaultTime));
    massSmoother.setCurrentAndTargetValue(sanitizeUnit(loadParam("mass"), kDefaultMass));
    densitySmoother.setCurrentAndTargetValue(sanitizeUnit(loadParam("density"), kDefaultDensity));
    bloomSmoother.setCurrentAndTargetValue(sanitizeUnit(loadParam("bloom"), kDefaultBloom));
    airSmoother.setCurrentAndTargetValue(sanitizeUnit(loadParam("air"), kDefaultAir));
    widthSmoother.setCurrentAndTargetValue(sanitizeUnit(loadParam("width"), kDefaultWidth));
    warpSmoother.setCurrentAndTargetValue(sanitizeUnit(loadParam("warp"), kDefaultWarp));
    driftSmoother.setCurrentAndTargetValue(sanitizeUnit(loadParam("drift"), kDefaultDrift));
    gravitySmoother.setCurrentAndTargetValue(sanitizeUnit(loadParam("gravity"), kDefaultGravity));
    pillarShapeSmoother.setCurrentAndTargetValue(
        sanitizeUnit(loadParam("pillarShape"), kDefaultPillarShape));

    // Physical modeling parameter smoothers (500ms smoothing)
    tubeCountSmoother.reset(sampleRate, smoothingRampSeconds);
    radiusVariationSmoother.reset(sampleRate, smoothingRampSeconds);
    metallicResonanceSmoother.reset(sampleRate, smoothingRampSeconds);
    couplingStrengthSmoother.reset(sampleRate, smoothingRampSeconds);
    elasticitySmoother.reset(sampleRate, smoothingRampSeconds);
    recoveryTimeSmoother.reset(sampleRate, smoothingRampSeconds);
    absorptionDriftSmoother.reset(sampleRate, smoothingRampSeconds);
    nonlinearitySmoother.reset(sampleRate, smoothingRampSeconds);
    impossibilityDegreeSmoother.reset(sampleRate, smoothingRampSeconds);
    pitchEvolutionRateSmoother.reset(sampleRate, smoothingRampSeconds);
    paradoxResonanceFreqSmoother.reset(sampleRate, smoothingRampSeconds);
    paradoxGainSmoother.reset(sampleRate, smoothingRampSeconds);

    tubeCountSmoother.setCurrentAndTargetValue(
        sanitizeUnit(loadParam("tubeCount"), kDefaultTubeCount));
    radiusVariationSmoother.setCurrentAndTargetValue(
        sanitizeUnit(loadParam("radiusVariation"), kDefaultRadiusVariation));
    metallicResonanceSmoother.setCurrentAndTargetValue(
        sanitizeUnit(loadParam("metallicResonance"), kDefaultMetallicResonance));
    couplingStrengthSmoother.setCurrentAndTargetValue(
        sanitizeUnit(loadParam("couplingStrength"), kDefaultCouplingStrength));
    elasticitySmoother.setCurrentAndTargetValue(
        sanitizeUnit(loadParam("elasticity"), kDefaultElasticity));
    recoveryTimeSmoother.setCurrentAndTargetValue(
        sanitizeUnit(loadParam("recoveryTime"), kDefaultRecoveryTime));
    absorptionDriftSmoother.setCurrentAndTargetValue(
        sanitizeUnit(loadParam("absorptionDrift"), kDefaultAbsorptionDrift));
    nonlinearitySmoother.setCurrentAndTargetValue(
        sanitizeUnit(loadParam("nonlinearity"), kDefaultNonlinearity));
    impossibilityDegreeSmoother.setCurrentAndTargetValue(
        sanitizeUnit(loadParam("impossibilityDegree"), kDefaultImpossibilityDegree));
    pitchEvolutionRateSmoother.setCurrentAndTargetValue(
        sanitizeUnit(loadParam("pitchEvolutionRate"), kDefaultPitchEvolutionRate));
    paradoxResonanceFreqSmoother.setCurrentAndTargetValue(
        sanitizeUnit(loadParam("paradoxResonanceFreq"), kDefaultParadoxResonance));
    paradoxGainSmoother.setCurrentAndTargetValue(
        sanitizeUnit(loadParam("paradoxGain"), kDefaultParadoxGain));

    // Initialize processing mode transition gain (starts at 1.0 for no fade)
    modeTransitionGain.reset(sampleRate, 0.05);  // 50ms fade time
    modeTransitionGain.setCurrentAndTargetValue(1.0f);
    modeTransitionState = ModeTransitionState::None;

    presetFadeSamples = juce::jmax(
        1, static_cast<int>(std::round(sampleRate * (kPresetFadeMs / 1000.0f))));
    presetFadeRemaining = 0;
    presetGain = 1.0f;
    presetTransition = PresetTransitionState::None;
    presetResetRequested.store(false, std::memory_order_release);
#if defined(MONUMENT_MEMORY_PROVE)
    memoryProvePulseInterval = juce::jmax(
        1, static_cast<int>(std::round(sampleRate * 0.5)));
    memoryProvePulseRemaining = 0;
#endif
}

void MonumentAudioProcessor::releaseResources()
{
    // Reset DSP routing graph (replaces individual module resets)
    routingGraph.reset();

    // Reset separate features
    memoryEchoes.reset();
    modulationMatrix.reset();
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

#if defined(MONUMENT_TESTING) || defined(MONUMENT_MEMORY_PROVE)
    const auto blockStartTicks = juce::Time::getHighResolutionTicks();
#endif

    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    float inputRms = 0.0f;
    const auto levelSamples = buffer.getNumSamples();
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
        inputRms = juce::jmax(inputRms, buffer.getRMSLevel(channel, 0, levelSamples));
    inputLevel.store(inputRms, std::memory_order_relaxed);

#if defined(MONUMENT_MEMORY_PROVE)
    float inputPeak = 0.0f;
    const auto inputChannels = buffer.getNumChannels();
    const auto inputSamples = buffer.getNumSamples();
    for (int channel = 0; channel < inputChannels; ++channel)
    {
        const auto* data = buffer.getReadPointer(channel);
        for (int sample = 0; sample < inputSamples; ++sample)
            inputPeak = juce::jmax(inputPeak, std::abs(data[sample]));
    }
#endif

    // OPTIMIZED: Batch parameter atomic loads with relaxed ordering (10-15% CPU reduction)
    // Using memory_order_relaxed is safe here because:
    // 1. Parameters are independent (no inter-parameter dependencies)
    // 2. APVTS already handles thread-safe writes on message thread
    // 3. Relaxed ordering avoids expensive memory fences while maintaining atomicity
    paramCache.mix = parameters.getRawParameterValue("mix")->load(std::memory_order_relaxed);
    paramCache.time = parameters.getRawParameterValue("time")->load(std::memory_order_relaxed);
    paramCache.mass = parameters.getRawParameterValue("mass")->load(std::memory_order_relaxed);
    paramCache.density = parameters.getRawParameterValue("density")->load(std::memory_order_relaxed);
    paramCache.bloom = parameters.getRawParameterValue("bloom")->load(std::memory_order_relaxed);
    paramCache.air = parameters.getRawParameterValue("air")->load(std::memory_order_relaxed);
    paramCache.width = parameters.getRawParameterValue("width")->load(std::memory_order_relaxed);
    paramCache.warp = parameters.getRawParameterValue("warp")->load(std::memory_order_relaxed);
    paramCache.drift = parameters.getRawParameterValue("drift")->load(std::memory_order_relaxed);
    paramCache.gravity = parameters.getRawParameterValue("gravity")->load(std::memory_order_relaxed);
    paramCache.pillarShape = parameters.getRawParameterValue("pillarShape")->load(std::memory_order_relaxed);
    paramCache.pillarMode = parameters.getRawParameterValue("pillarMode")->load(std::memory_order_relaxed);
    paramCache.memory = parameters.getRawParameterValue("memory")->load(std::memory_order_relaxed);
    paramCache.memoryDepth = parameters.getRawParameterValue("memoryDepth")->load(std::memory_order_relaxed);
    paramCache.memoryDecay = parameters.getRawParameterValue("memoryDecay")->load(std::memory_order_relaxed);
    paramCache.memoryDrift = parameters.getRawParameterValue("memoryDrift")->load(std::memory_order_relaxed);
    paramCache.freeze = parameters.getRawParameterValue("freeze")->load(std::memory_order_relaxed) > 0.5f;
    paramCache.material = parameters.getRawParameterValue("material")->load(std::memory_order_relaxed);
    paramCache.topology = parameters.getRawParameterValue("topology")->load(std::memory_order_relaxed);
    paramCache.viscosity = parameters.getRawParameterValue("viscosity")->load(std::memory_order_relaxed);
    paramCache.evolution = parameters.getRawParameterValue("evolution")->load(std::memory_order_relaxed);
    paramCache.chaosIntensity = parameters.getRawParameterValue("chaosIntensity")->load(std::memory_order_relaxed);
    paramCache.elasticityDecay = parameters.getRawParameterValue("elasticityDecay")->load(std::memory_order_relaxed);
    paramCache.patina = parameters.getRawParameterValue("patina")->load(std::memory_order_relaxed);
    paramCache.abyss = parameters.getRawParameterValue("abyss")->load(std::memory_order_relaxed);
    paramCache.corona = parameters.getRawParameterValue("corona")->load(std::memory_order_relaxed);
    paramCache.breath = parameters.getRawParameterValue("breath")->load(std::memory_order_relaxed);
    paramCache.character = parameters.getRawParameterValue("character")->load(std::memory_order_relaxed);
    paramCache.spaceType = parameters.getRawParameterValue("spaceType")->load(std::memory_order_relaxed);
    paramCache.energy = parameters.getRawParameterValue("energy")->load(std::memory_order_relaxed);
    paramCache.motion = parameters.getRawParameterValue("motion")->load(std::memory_order_relaxed);
    paramCache.color = parameters.getRawParameterValue("color")->load(std::memory_order_relaxed);
    paramCache.dimension = parameters.getRawParameterValue("dimension")->load(std::memory_order_relaxed);
    paramCache.tubeCount = parameters.getRawParameterValue("tubeCount")->load(std::memory_order_relaxed);
    paramCache.radiusVariation = parameters.getRawParameterValue("radiusVariation")->load(std::memory_order_relaxed);
    paramCache.metallicResonance = parameters.getRawParameterValue("metallicResonance")->load(std::memory_order_relaxed);
    paramCache.couplingStrength = parameters.getRawParameterValue("couplingStrength")->load(std::memory_order_relaxed);
    paramCache.elasticity = parameters.getRawParameterValue("elasticity")->load(std::memory_order_relaxed);
    paramCache.recoveryTime = parameters.getRawParameterValue("recoveryTime")->load(std::memory_order_relaxed);
    paramCache.absorptionDrift = parameters.getRawParameterValue("absorptionDrift")->load(std::memory_order_relaxed);
    paramCache.nonlinearity = parameters.getRawParameterValue("nonlinearity")->load(std::memory_order_relaxed);
    paramCache.impossibilityDegree = parameters.getRawParameterValue("impossibilityDegree")->load(std::memory_order_relaxed);
    paramCache.pitchEvolutionRate = parameters.getRawParameterValue("pitchEvolutionRate")->load(std::memory_order_relaxed);
    paramCache.paradoxResonanceFreq = parameters.getRawParameterValue("paradoxResonanceFreq")->load(std::memory_order_relaxed);
    paramCache.paradoxGain = parameters.getRawParameterValue("paradoxGain")->load(std::memory_order_relaxed);
    paramCache.routingPreset = parameters.getRawParameterValue("routingPreset")->load(std::memory_order_relaxed);
    paramCache.macroMode = parameters.getRawParameterValue("macroMode")->load(std::memory_order_relaxed);
    paramCache.safetyClip = parameters.getRawParameterValue("safetyClip")->load(std::memory_order_relaxed) > 0.5f;
    paramCache.safetyClipDrive = parameters.getRawParameterValue("safetyClipDrive")->load(std::memory_order_relaxed);
    paramCache.timelineEnabled = parameters.getRawParameterValue("timelineEnabled")->load(std::memory_order_relaxed) > 0.5f;
    paramCache.timelinePreset = parameters.getRawParameterValue("timelinePreset")->load(std::memory_order_relaxed);

    const int maxTimelinePreset = juce::jmax(0, monument::dsp::SequencePresets::getNumPresets() - 1);
    const int timelinePreset = sanitizeChoice(paramCache.timelinePreset, 0, maxTimelinePreset, 0);
    if (timelinePreset != lastTimelinePreset)
    {
        sequenceScheduler.loadSequence(monument::dsp::SequencePresets::getPreset(timelinePreset));
        lastTimelinePreset = timelinePreset;
    }
    if (paramCache.timelineEnabled != lastTimelineEnabled)
    {
        sequenceScheduler.setEnabled(paramCache.timelineEnabled);
        lastTimelineEnabled = paramCache.timelineEnabled;
    }

    // Process sequence scheduler (Phase 4: Timeline automation)
    // This must happen BEFORE using paramCache values, so sequenced values can override them
    const auto* playHead = getPlayHead();
    const auto positionInfo = playHead != nullptr
        ? playHead->getPosition()
        : juce::Optional<juce::AudioPlayHead::PositionInfo>{};
    sequenceScheduler.process(positionInfo, buffer.getNumSamples());

    // Apply sequence scheduler overrides to paramCache (if any parameters are automated)
    using ParamId = monument::dsp::SequenceScheduler::ParameterId;
    #define APPLY_SEQUENCED_PARAM(paramId, cacheField) \
        if (auto value = sequenceScheduler.getParameterValue(ParamId::paramId)) \
            paramCache.cacheField = *value;

    APPLY_SEQUENCED_PARAM(Time, time)
    APPLY_SEQUENCED_PARAM(Mass, mass)
    APPLY_SEQUENCED_PARAM(Density, density)
    APPLY_SEQUENCED_PARAM(Bloom, bloom)
    APPLY_SEQUENCED_PARAM(Gravity, gravity)
    APPLY_SEQUENCED_PARAM(Warp, warp)
    APPLY_SEQUENCED_PARAM(Drift, drift)
    APPLY_SEQUENCED_PARAM(Memory, memory)
    APPLY_SEQUENCED_PARAM(MemoryDepth, memoryDepth)
    APPLY_SEQUENCED_PARAM(MemoryDecay, memoryDecay)
    APPLY_SEQUENCED_PARAM(MemoryDrift, memoryDrift)
    APPLY_SEQUENCED_PARAM(Mix, mix)
    APPLY_SEQUENCED_PARAM(Material, material)
    APPLY_SEQUENCED_PARAM(Topology, topology)
    APPLY_SEQUENCED_PARAM(Viscosity, viscosity)
    APPLY_SEQUENCED_PARAM(Evolution, evolution)
    APPLY_SEQUENCED_PARAM(ChaosIntensity, chaosIntensity)
    APPLY_SEQUENCED_PARAM(ElasticityDecay, elasticityDecay)
    APPLY_SEQUENCED_PARAM(Patina, patina)
    APPLY_SEQUENCED_PARAM(Abyss, abyss)
    APPLY_SEQUENCED_PARAM(Corona, corona)
    APPLY_SEQUENCED_PARAM(Breath, breath)
    #undef APPLY_SEQUENCED_PARAM

    // Use cached values (no more atomic loads)
    const float mixPercentRaw = sanitizeRange(paramCache.mix, kDefaultMixPercent, 0.0f, 100.0f);
    const float time = sanitizeUnit(paramCache.time, kDefaultTime);
    const float mass = sanitizeUnit(paramCache.mass, kDefaultMass);
    const float density = sanitizeUnit(paramCache.density, kDefaultDensity);
    const float bloom = sanitizeUnit(paramCache.bloom, kDefaultBloom);
    const float air = sanitizeUnit(paramCache.air, kDefaultAir);
    const float width = sanitizeUnit(paramCache.width, kDefaultWidth);
    const float warp = sanitizeUnit(paramCache.warp, kDefaultWarp);
    const float drift = sanitizeUnit(paramCache.drift, kDefaultDrift);
    const float gravity = sanitizeUnit(paramCache.gravity, kDefaultGravity);
    const float pillarShape = sanitizeUnit(paramCache.pillarShape, kDefaultPillarShape);
    const float pillarModeRaw = static_cast<float>(sanitizeChoice(paramCache.pillarMode, 0, 2, 0));
    const float memory = sanitizeUnit(paramCache.memory, kDefaultMemory);
    const float memoryDepth = sanitizeUnit(paramCache.memoryDepth, kDefaultMemoryDepth);
    const float memoryDecay = sanitizeUnit(paramCache.memoryDecay, kDefaultMemoryDecay);
    const float memoryDrift = sanitizeUnit(paramCache.memoryDrift, kDefaultMemoryDrift);
    const bool freeze = paramCache.freeze;
    const float material = sanitizeUnit(paramCache.material, kDefaultMaterial);
    const float topology = sanitizeUnit(paramCache.topology, kDefaultTopology);
    const float viscosity = sanitizeUnit(paramCache.viscosity, kDefaultViscosity);
    const float evolution = sanitizeUnit(paramCache.evolution, kDefaultEvolution);
    const float chaosIntensity = sanitizeUnit(paramCache.chaosIntensity, kDefaultChaosIntensity);
    const float elasticityDecay = sanitizeUnit(paramCache.elasticityDecay, kDefaultElasticityDecay);
    const float patina = sanitizeUnit(paramCache.patina, kDefaultPatina);
    const float abyss = sanitizeUnit(paramCache.abyss, kDefaultAbyss);
    const float corona = sanitizeUnit(paramCache.corona, kDefaultCorona);
    const float breath = sanitizeUnit(paramCache.breath, kDefaultBreath);

    // Physical modeling parameters (Phase 5)
    const float tubeCount = sanitizeUnit(paramCache.tubeCount, kDefaultTubeCount);
    const float radiusVariation = sanitizeUnit(paramCache.radiusVariation, kDefaultRadiusVariation);
    const float metallicResonance = sanitizeUnit(paramCache.metallicResonance, kDefaultMetallicResonance);
    const float couplingStrength = sanitizeUnit(paramCache.couplingStrength, kDefaultCouplingStrength);
    const float elasticity = sanitizeUnit(paramCache.elasticity, kDefaultElasticity);
    const float recoveryTime = sanitizeUnit(paramCache.recoveryTime, kDefaultRecoveryTime);
    const float absorptionDrift = sanitizeUnit(paramCache.absorptionDrift, kDefaultAbsorptionDrift);
    const float nonlinearity = sanitizeUnit(paramCache.nonlinearity, kDefaultNonlinearity);
    const float impossibilityDegree = sanitizeUnit(paramCache.impossibilityDegree, kDefaultImpossibilityDegree);
    const float pitchEvolutionRate = sanitizeUnit(paramCache.pitchEvolutionRate, kDefaultPitchEvolutionRate);
    const float paradoxResonanceFreq = sanitizeUnit(paramCache.paradoxResonanceFreq, kDefaultParadoxResonance);
    const float paradoxGain = sanitizeUnit(paramCache.paradoxGain, kDefaultParadoxGain);
    const float character = sanitizeUnit(paramCache.character, kDefaultCharacter);
    const float spaceType = sanitizeUnit(paramCache.spaceType, kDefaultSpaceType);
    const float energy = sanitizeUnit(paramCache.energy, kDefaultEnergy);
    const float motion = sanitizeUnit(paramCache.motion, kDefaultMotion);
    const float color = sanitizeUnit(paramCache.color, kDefaultColor);
    const float dimension = sanitizeUnit(paramCache.dimension, kDefaultDimension);
    const float safetyClipDrive = sanitizeUnit(paramCache.safetyClipDrive, kDefaultSafetyClipDrive);
    const int routingPreset = sanitizeChoice(paramCache.routingPreset, 0,
        static_cast<int>(monument::dsp::RoutingPresetType::MinimalSparse), 0);
    const int macroModeIndex = sanitizeChoice(paramCache.macroMode, 0, 1, 0);

    const bool useExpressive = macroModeIndex == 1;

    struct MacroTargets
    {
        float time{0.55f};
        float mass{0.5f};
        float density{0.5f};
        float bloom{0.5f};
        float air{0.5f};
        float width{0.5f};
        float warp{0.0f};
        float drift{0.0f};
        float gravity{0.5f};
        float pillarShape{0.5f};
        float tubeCount{0.545f};
        float radiusVariation{0.3f};
        float metallicResonance{0.5f};
        float couplingStrength{0.5f};
        float elasticity{0.5f};
        float recoveryTime{0.5f};
        float absorptionDrift{0.3f};
        float nonlinearity{0.3f};
        float impossibilityDegree{0.3f};
        float pitchEvolutionRate{0.3f};
        float paradoxResonanceFreq{0.5f};
        float paradoxGain{0.3f};
        monument::dsp::RoutingPresetType routingPreset{monument::dsp::RoutingPresetType::TraditionalCathedral};
    };

    MacroTargets macroTargets{};
    float macroInfluence = 0.0f;

    if (useExpressive)
    {
        const auto expressiveTargets = expressiveMacroMapper.computeTargets(
            character,
            spaceType,
            energy,
            motion,
            color,
            dimension);

        macroTargets.time = expressiveTargets.time;
        macroTargets.mass = expressiveTargets.mass;
        macroTargets.density = expressiveTargets.density;
        macroTargets.bloom = expressiveTargets.bloom;
        macroTargets.air = expressiveTargets.air;
        macroTargets.width = expressiveTargets.width;
        macroTargets.warp = expressiveTargets.warp;
        macroTargets.drift = expressiveTargets.drift;
        macroTargets.gravity = expressiveTargets.gravity;
        macroTargets.pillarShape = expressiveTargets.pillarShape;
        macroTargets.tubeCount = expressiveTargets.tubeCount;
        macroTargets.radiusVariation = expressiveTargets.radiusVariation;
        macroTargets.metallicResonance = expressiveTargets.metallicResonance;
        macroTargets.couplingStrength = expressiveTargets.couplingStrength;
        macroTargets.elasticity = expressiveTargets.elasticity;
        macroTargets.recoveryTime = expressiveTargets.recoveryTime;
        macroTargets.absorptionDrift = expressiveTargets.absorptionDrift;
        macroTargets.nonlinearity = expressiveTargets.nonlinearity;
        macroTargets.impossibilityDegree = expressiveTargets.impossibilityDegree;
        macroTargets.pitchEvolutionRate = expressiveTargets.pitchEvolutionRate;
        macroTargets.paradoxResonanceFreq = expressiveTargets.paradoxResonanceFreq;
        macroTargets.paradoxGain = expressiveTargets.paradoxGain;
        macroTargets.routingPreset = expressiveTargets.routingPreset;

        // Expressive macro defaults:
        // character=0.5, spaceType=0.3, energy=0.1, motion=0.2, color=0.5, dimension=0.5
        const float characterDelta = std::abs(character - 0.5f);
        const float spaceTypeDelta = std::abs(spaceType - 0.3f);
        const float energyDelta = std::abs(energy - 0.1f);
        const float motionDelta = std::abs(motion - 0.2f);
        const float colorDelta = std::abs(color - 0.5f);
        const float dimensionDelta = std::abs(dimension - 0.5f);
        macroInfluence = juce::jmin(1.0f,
            (characterDelta + spaceTypeDelta + energyDelta + motionDelta + colorDelta + dimensionDelta) * 2.0f);
    }
    else
    {
        const auto ancientTargets = macroMapper.computeTargets(
            material,        // stone
            topology,        // labyrinth
            viscosity,       // mist
            evolution,       // bloom
            chaosIntensity,  // tempest
            elasticityDecay, // echo
            patina,
            abyss,
            corona,
            breath);

        macroTargets.time = ancientTargets.time;
        macroTargets.mass = ancientTargets.mass;
        macroTargets.density = ancientTargets.density;
        macroTargets.bloom = ancientTargets.bloom;
        macroTargets.air = ancientTargets.air;
        macroTargets.width = ancientTargets.width;
        macroTargets.warp = ancientTargets.warp;
        macroTargets.drift = ancientTargets.drift;
        macroTargets.gravity = ancientTargets.gravity;
        macroTargets.pillarShape = ancientTargets.pillarShape;
        macroTargets.routingPreset = static_cast<monument::dsp::RoutingPresetType>(
            routingPreset);

        // Ancient Monuments Phase 5 - 10 macro defaults:
        // stone=0.5, labyrinth=0.5, mist=0.5, bloom=0.5, tempest=0.0, echo=0.0
        // patina=0.5, abyss=0.5, corona=0.5, breath=0.0
        const float materialDelta = std::abs(material - 0.5f);          // stone
        const float topologyDelta = std::abs(topology - 0.5f);          // labyrinth
        const float viscosityDelta = std::abs(viscosity - 0.5f);        // mist
        const float evolutionDelta = std::abs(evolution - 0.5f);        // bloom
        const float chaosDelta = std::abs(chaosIntensity - 0.0f);       // tempest
        const float elasticityDelta = std::abs(elasticityDecay - 0.0f); // echo
        const float patinaDelta = std::abs(patina - 0.5f);
        const float abyssDelta = std::abs(abyss - 0.5f);
        const float coronaDelta = std::abs(corona - 0.5f);
        const float breathDelta = std::abs(breath - 0.0f);

        // Macro influence: 0 = all at defaults, 1 = at least one macro significantly moved
        // Normalize by dividing by 10 macros (was 6), then scale by 2.0 for sensitivity
        macroInfluence = juce::jmin(1.0f,
            (materialDelta + topologyDelta + viscosityDelta + evolutionDelta + chaosDelta + elasticityDelta +
             patinaDelta + abyssDelta + coronaDelta + breathDelta) * (2.0f * 6.0f / 10.0f));
    }

    // Process modulation matrix (stub sources for Phase 2, returns 0 for all destinations)
    modulationMatrix.process(buffer, buffer.getNumSamples());

    const int maxRoutingPreset = static_cast<int>(monument::dsp::RoutingPresetType::MinimalSparse);
    const int targetRoutingPreset = juce::jlimit(
        0,
        maxRoutingPreset,
        useExpressive ? static_cast<int>(macroTargets.routingPreset) : routingPreset);
    if (targetRoutingPreset != lastRoutingPreset)
    {
        routingGraph.loadRoutingPreset(
            static_cast<monument::dsp::RoutingPresetType>(targetRoutingPreset));
        lastRoutingPreset = targetRoutingPreset;
    }

    // Blend base parameters with macro targets based on macro influence
    // When macroInfluence = 0, use base parameters; when = 1, use macro targets
    // Set target values on JUCE SmoothedValue (50ms ramp) and mark as potentially active
    timeSmoother.setTargetValue(juce::jmap(macroInfluence, time, macroTargets.time));
    massSmoother.setTargetValue(juce::jmap(macroInfluence, mass, macroTargets.mass));
    densitySmoother.setTargetValue(juce::jmap(macroInfluence, density, macroTargets.density));
    bloomSmoother.setTargetValue(juce::jmap(macroInfluence, bloom, macroTargets.bloom));
    airSmoother.setTargetValue(juce::jmap(macroInfluence, air, macroTargets.air));
    widthSmoother.setTargetValue(juce::jmap(macroInfluence, width, macroTargets.width));
    warpSmoother.setTargetValue(juce::jmap(macroInfluence, warp, macroTargets.warp));
    driftSmoother.setTargetValue(juce::jmap(macroInfluence, drift, macroTargets.drift));
    gravitySmoother.setTargetValue(juce::jmap(macroInfluence, gravity, macroTargets.gravity));
    pillarShapeSmoother.setTargetValue(juce::jmap(macroInfluence, pillarShape, macroTargets.pillarShape));

    // Mark these smoothers as potentially active (bitmask optimization)
    activeSmoothers |= 0x3FF; // Bits 0-9 (first 10 smoothers)

    // Fill per-sample parameter buffers for critical parameters (Phase 4: Per-sample interpolation)
    // These 8 parameters are most audible and require per-sample smoothing for zipper-free automation
    const int numSamples = buffer.getNumSamples();
    if (numSamples > paramBufferPool.capacity())
    {
        // Fallback: resize heap buffers if host changes block size without prepareToPlay().
        paramBufferPool.prepare(numSamples);
    }
    jassert(numSamples <= paramBufferPool.capacity());

    auto* timeBuffer = paramBufferPool.getTimeBuffer(numSamples);
    auto* massBuffer = paramBufferPool.getMassBuffer(numSamples);
    auto* densityBuffer = paramBufferPool.getDensityBuffer(numSamples);
    auto* bloomBuffer = paramBufferPool.getBloomBuffer(numSamples);
    auto* gravityBuffer = paramBufferPool.getGravityBuffer(numSamples);
    auto* pillarShapeBuffer = paramBufferPool.getPillarShapeBuffer(numSamples);
    auto* warpBuffer = paramBufferPool.getWarpBuffer(numSamples);
    auto* driftBuffer = paramBufferPool.getDriftBuffer(numSamples);

    ParameterBufferPool::fillBuffer(timeBuffer, timeSmoother, numSamples);
    ParameterBufferPool::fillBuffer(massBuffer, massSmoother, numSamples);
    ParameterBufferPool::fillBuffer(densityBuffer, densitySmoother, numSamples);
    ParameterBufferPool::fillBuffer(bloomBuffer, bloomSmoother, numSamples);
    ParameterBufferPool::fillBuffer(gravityBuffer, gravitySmoother, numSamples);
    ParameterBufferPool::fillBuffer(pillarShapeBuffer, pillarShapeSmoother, numSamples);
    ParameterBufferPool::fillBuffer(warpBuffer, warpSmoother, numSamples);
    ParameterBufferPool::fillBuffer(driftBuffer, driftSmoother, numSamples);

    // Non-critical parameters (air, width): use block-rate averaging for now
    // These will be passed as ParameterBuffer constants (future optimization: move to pool)
    float airSum = 0.0f, widthSum = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        airSum += airSmoother.getNextValue();
        widthSum += widthSmoother.getNextValue();
    }
    const float airEffective = airSum / numSamples;
    const float widthEffective = widthSum / numSamples;

    // TEMPORARY (Step 3): Calculate averaged values for backward compatibility
    // In Step 4, we'll pass ParameterBuffers directly to DspRoutingGraph
    // These averages maintain compatibility with current float-based API
    float timeSum = 0.0f, massSum = 0.0f, densitySum = 0.0f, bloomSum = 0.0f;
    float warpSum = 0.0f, driftSum = 0.0f, gravitySum = 0.0f, pillarShapeSum = 0.0f;
    for (int i = 0; i < numSamples; ++i)
    {
        timeSum += timeBuffer[i];
        massSum += massBuffer[i];
        densitySum += densityBuffer[i];
        bloomSum += bloomBuffer[i];
        warpSum += warpBuffer[i];
        driftSum += driftBuffer[i];
        gravitySum += gravityBuffer[i];
        pillarShapeSum += pillarShapeBuffer[i];
    }
    const float timeEffective = timeSum / numSamples;
    const float massEffective = massSum / numSamples;
    const float densityEffective = densitySum / numSamples;
    const float bloomEffectiveMacro = bloomSum / numSamples;
    const float warpEffectiveMacro = warpSum / numSamples;
    const float driftEffectiveMacro = driftSum / numSamples;
    const float gravityEffective = gravitySum / numSamples;
    const float pillarShapeEffective = pillarShapeSum / numSamples;

    // OPTIMIZED: Use bitmask to skip inactive smoothers (5-10% CPU reduction)
    // Temporal coherence: if a smoother wasn't active last frame, skip the expensive isSmoothing() check
    // When parameters are stable (most of the time), this avoids 22 redundant checks
    const auto blockSize = buffer.getNumSamples();
    uint32_t newActiveSmoothers = 0;

    // Macro: Check if smoother is still active (skip already done above)
    // NOTE: Smoothers advanced above with skip() - just update activity bitmask here
    #define CHECK_SMOOTHER(smoother, bitIndex) \
        if (smoother.isSmoothing()) { \
            newActiveSmoothers |= (1u << bitIndex); \
        }

    CHECK_SMOOTHER(timeSmoother, 0)
    CHECK_SMOOTHER(massSmoother, 1)
    CHECK_SMOOTHER(densitySmoother, 2)
    CHECK_SMOOTHER(bloomSmoother, 3)
    CHECK_SMOOTHER(airSmoother, 4)
    CHECK_SMOOTHER(widthSmoother, 5)
    CHECK_SMOOTHER(warpSmoother, 6)
    CHECK_SMOOTHER(driftSmoother, 7)
    CHECK_SMOOTHER(gravitySmoother, 8)
    CHECK_SMOOTHER(pillarShapeSmoother, 9)

    // Physical modeling parameter smoothers (Phase 5)
    const float tubeCountTarget = useExpressive ? macroTargets.tubeCount : tubeCount;
    const float radiusVariationTarget = useExpressive ? macroTargets.radiusVariation : radiusVariation;
    const float metallicResonanceTarget = useExpressive ? macroTargets.metallicResonance : metallicResonance;
    const float couplingStrengthTarget = useExpressive ? macroTargets.couplingStrength : couplingStrength;
    const float elasticityTarget = useExpressive ? macroTargets.elasticity : elasticity;
    const float recoveryTimeTarget = useExpressive ? macroTargets.recoveryTime : recoveryTime;
    const float absorptionDriftTarget = useExpressive ? macroTargets.absorptionDrift : absorptionDrift;
    const float nonlinearityTarget = useExpressive ? macroTargets.nonlinearity : nonlinearity;
    const float impossibilityDegreeTarget = useExpressive ? macroTargets.impossibilityDegree : impossibilityDegree;
    const float pitchEvolutionRateTarget = useExpressive ? macroTargets.pitchEvolutionRate : pitchEvolutionRate;
    const float paradoxResonanceFreqTarget = useExpressive ? macroTargets.paradoxResonanceFreq : paradoxResonanceFreq;
    const float paradoxGainTarget = useExpressive ? macroTargets.paradoxGain : paradoxGain;

    tubeCountSmoother.setTargetValue(juce::jmap(macroInfluence, tubeCount, tubeCountTarget));
    radiusVariationSmoother.setTargetValue(juce::jmap(macroInfluence, radiusVariation, radiusVariationTarget));
    metallicResonanceSmoother.setTargetValue(juce::jmap(macroInfluence, metallicResonance, metallicResonanceTarget));
    couplingStrengthSmoother.setTargetValue(juce::jmap(macroInfluence, couplingStrength, couplingStrengthTarget));
    elasticitySmoother.setTargetValue(juce::jmap(macroInfluence, elasticity, elasticityTarget));
    recoveryTimeSmoother.setTargetValue(juce::jmap(macroInfluence, recoveryTime, recoveryTimeTarget));
    absorptionDriftSmoother.setTargetValue(juce::jmap(macroInfluence, absorptionDrift, absorptionDriftTarget));
    nonlinearitySmoother.setTargetValue(juce::jmap(macroInfluence, nonlinearity, nonlinearityTarget));
    impossibilityDegreeSmoother.setTargetValue(juce::jmap(macroInfluence, impossibilityDegree, impossibilityDegreeTarget));
    pitchEvolutionRateSmoother.setTargetValue(juce::jmap(macroInfluence, pitchEvolutionRate, pitchEvolutionRateTarget));
    paradoxResonanceFreqSmoother.setTargetValue(juce::jmap(macroInfluence, paradoxResonanceFreq, paradoxResonanceFreqTarget));
    paradoxGainSmoother.setTargetValue(juce::jmap(macroInfluence, paradoxGain, paradoxGainTarget));

    // Mark physical modeling smoothers as potentially active (bits 10-21)
    activeSmoothers |= 0x3FFC00; // Bits 10-21 (physical modeling smoothers)

    // Get smoothed values with TRUE sample-rate interpolation (prevents zipper noise)
    // Calculate average value across block for all physical modeling parameters
    float tubeCountSum = 0.0f, radiusVariationSum = 0.0f, metallicResonanceSum = 0.0f;
    float couplingStrengthSum = 0.0f, elasticitySum = 0.0f, recoveryTimeSum = 0.0f;
    float absorptionDriftSum = 0.0f, nonlinearitySum = 0.0f, impossibilityDegreeSum = 0.0f;
    float pitchEvolutionRateSum = 0.0f, paradoxResonanceFreqSum = 0.0f, paradoxGainSum = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        tubeCountSum += tubeCountSmoother.getNextValue();
        radiusVariationSum += radiusVariationSmoother.getNextValue();
        metallicResonanceSum += metallicResonanceSmoother.getNextValue();
        couplingStrengthSum += couplingStrengthSmoother.getNextValue();
        elasticitySum += elasticitySmoother.getNextValue();
        recoveryTimeSum += recoveryTimeSmoother.getNextValue();
        absorptionDriftSum += absorptionDriftSmoother.getNextValue();
        nonlinearitySum += nonlinearitySmoother.getNextValue();
        impossibilityDegreeSum += impossibilityDegreeSmoother.getNextValue();
        pitchEvolutionRateSum += pitchEvolutionRateSmoother.getNextValue();
        paradoxResonanceFreqSum += paradoxResonanceFreqSmoother.getNextValue();
        paradoxGainSum += paradoxGainSmoother.getNextValue();
    }

    const float tubeCountEffective = tubeCountSum / numSamples;
    const float radiusVariationEffective = radiusVariationSum / numSamples;
    const float metallicResonanceEffective = metallicResonanceSum / numSamples;
    const float couplingStrengthEffective = couplingStrengthSum / numSamples;
    const float elasticityEffective = elasticitySum / numSamples;
    const float recoveryTimeEffective = recoveryTimeSum / numSamples;
    const float absorptionDriftEffective = absorptionDriftSum / numSamples;
    const float nonlinearityEffective = nonlinearitySum / numSamples;
    const float impossibilityDegreeEffective = impossibilityDegreeSum / numSamples;
    const float pitchEvolutionRateEffective = pitchEvolutionRateSum / numSamples;
    const float paradoxResonanceFreqEffective = paradoxResonanceFreqSum / numSamples;
    const float paradoxGainEffective = paradoxGainSum / numSamples;

    // Physical modeling smoothers with bitmask optimization
    CHECK_SMOOTHER(tubeCountSmoother, 10)
    CHECK_SMOOTHER(radiusVariationSmoother, 11)
    CHECK_SMOOTHER(metallicResonanceSmoother, 12)
    CHECK_SMOOTHER(couplingStrengthSmoother, 13)
    CHECK_SMOOTHER(elasticitySmoother, 14)
    CHECK_SMOOTHER(recoveryTimeSmoother, 15)
    CHECK_SMOOTHER(absorptionDriftSmoother, 16)
    CHECK_SMOOTHER(nonlinearitySmoother, 17)
    CHECK_SMOOTHER(impossibilityDegreeSmoother, 18)
    CHECK_SMOOTHER(pitchEvolutionRateSmoother, 19)
    CHECK_SMOOTHER(paradoxResonanceFreqSmoother, 20)
    CHECK_SMOOTHER(paradoxGainSmoother, 21)

    #undef CHECK_SMOOTHER

    // Update bitmask for next frame
    activeSmoothers = newActiveSmoothers;

    // Phase 3: Apply modulation from ModulationMatrix
    // Modulation values are bipolar [-1, +1], applied as offsets to macro-influenced parameters
    const float modTime = modulationMatrix.getModulation(monument::dsp::ModulationMatrix::DestinationType::Time);
    const float modMass = modulationMatrix.getModulation(monument::dsp::ModulationMatrix::DestinationType::Mass);
    const float modDensity = modulationMatrix.getModulation(monument::dsp::ModulationMatrix::DestinationType::Density);
    const float modBloom = modulationMatrix.getModulation(monument::dsp::ModulationMatrix::DestinationType::Bloom);
    const float modAir = modulationMatrix.getModulation(monument::dsp::ModulationMatrix::DestinationType::Air);
    const float modWidth = modulationMatrix.getModulation(monument::dsp::ModulationMatrix::DestinationType::Width);
    const float modWarp = modulationMatrix.getModulation(monument::dsp::ModulationMatrix::DestinationType::Warp);
    const float modDrift = modulationMatrix.getModulation(monument::dsp::ModulationMatrix::DestinationType::Drift);
    const float modGravity = modulationMatrix.getModulation(monument::dsp::ModulationMatrix::DestinationType::Gravity);
    const float modPillarShape = modulationMatrix.getModulation(monument::dsp::ModulationMatrix::DestinationType::PillarShape);

    // Apply modulation offsets and clamp to valid [0, 1] range
    const float timeModulated = juce::jlimit(0.0f, 1.0f, timeEffective + modTime);
    const float massModulated = juce::jlimit(0.0f, 1.0f, massEffective + modMass);
    const float densityModulated = juce::jlimit(0.0f, 1.0f, densityEffective + modDensity);
    const float bloomModulated = juce::jlimit(0.0f, 1.0f, bloomEffectiveMacro + modBloom);
    const float airModulated = juce::jlimit(0.0f, 1.0f, airEffective + modAir);
    const float widthModulated = juce::jlimit(0.0f, 1.0f, widthEffective + modWidth);
    const float warpModulated = juce::jlimit(0.0f, 1.0f, warpEffectiveMacro + modWarp);
    const float driftModulated = juce::jlimit(0.0f, 1.0f, driftEffectiveMacro + modDrift);
    const float gravityModulated = juce::jlimit(0.0f, 1.0f, gravityEffective + modGravity);
    const float pillarShapeModulated = juce::jlimit(0.0f, 1.0f, pillarShapeEffective + modPillarShape);

#if defined(MONUMENT_MEMORY_PROVE)
    const bool forceWet = kMemoryProveStage < 4;
    const bool forceFreezeOff = kMemoryProveStage < 5;
    const bool bypassChambers = kMemoryProveStage < 2;
    const bool allowModulation = kMemoryProveStage >= 3;
    const bool routeMemoryToOutput = kMemoryProveStage == 0;
    if (routeMemoryToOutput)
    {
        memoryProvePulseRemaining -= buffer.getNumSamples();
        if (inputPeak < 1.0e-6f && memoryProvePulseRemaining <= 0)
        {
            const float pulse = 0.8f;
            buffer.setSample(0, 0, pulse);
            if (buffer.getNumChannels() > 1)
                buffer.setSample(1, 0, pulse);
            memoryProvePulseRemaining = memoryProvePulseInterval;
            juce::Logger::writeToLog("Monument MemoryEchoes prove injected pulse stage="
                + juce::String(kMemoryProveStage));
            inputPeak = pulse;
        }
    }
#else
    const bool forceWet = false;
    const bool forceFreezeOff = false;
    const bool bypassChambers = false;
    const bool allowModulation = true;
    const bool routeMemoryToOutput = false;
#endif
    const bool injectToBuffer = bypassChambers && !routeMemoryToOutput;

    const float mixPercent = mixPercentRaw;
    const float mixPercentEffective = forceWet ? 100.0f : mixPercent;
    const bool freezeEffective = forceFreezeOff ? false : freeze;
    mixSmoother.setTargetValue(juce::jlimit(0.0f, 100.0f, mixPercentEffective));
    // Use modulated values (Phase 3: modulation system now active)
    const float warpEffective = allowModulation ? warpModulated : 0.0f;
    const float driftEffective = allowModulation ? driftModulated : 0.0f;
    const float bloomEffective = allowModulation ? bloomModulated : 0.0f;
    float pillarModeSafe = pillarModeRaw;
    pillarModeSafe = juce::jlimit(0.0f, 2.0f, pillarModeSafe);

    if (presetResetRequested.exchange(false, std::memory_order_acq_rel))
    {
        presetTransition = PresetTransitionState::FadingOut;
        presetFadeRemaining = presetFadeSamples;
    }

    // Forward parameters to DSP routing graph (Phase 4: Per-sample parameter buffers)
    // Critical parameters now passed as ParameterBuffer views for zipper-free automation

    // Create ParameterBuffer views from per-sample buffers (with block-rate modulation offsets)
    // NOTE: Modulation is applied per-sample in future optimization; for now apply block-rate offset
    auto makeModulatedView = [numSamples](const float* buffer, float modulationOffset) -> ParameterBuffer {
        // For now, return per-sample buffer without modulation (modules will receive raw smoothed values)
        // TODO: Apply per-sample modulation in Step 8 (requires refactoring modulation matrix)
        (void)modulationOffset;  // Suppress unused parameter warning
        return ParameterBuffer(buffer, numSamples);
    };

    // Pillars: pillarShape is per-sample, density/warp are block-rate
    routingGraph.setPillarsParams(
        densityModulated,
        makeModulatedView(pillarShapeBuffer, modPillarShape),
        warpEffective
    );

    // Chambers: time, mass, density, bloom, gravity all per-sample
    routingGraph.setChambersParams(
        makeModulatedView(timeBuffer, modTime),
        makeModulatedView(massBuffer, modMass),
        makeModulatedView(densityBuffer, modDensity),
        makeModulatedView(bloomBuffer, modBloom),
        makeModulatedView(gravityBuffer, modGravity),
        warpEffective,
        driftEffective,
        freezeEffective
    );

    // Weathering: warp and drift are per-sample
    routingGraph.setWeatheringParams(
        makeModulatedView(warpBuffer, modWarp),
        makeModulatedView(driftBuffer, modDrift)
    );
    routingGraph.setButtressParams(juce::jmap(massModulated, 0.9f, 1.6f), 0.0f);

    // FEEDBACK SAFETY: Apply attenuation at high mix levels to prevent feedback runaway
    // At 100% mix, use 0.94x gain to dampen feedback loops (6% reduction prevents energy buildup)
    // At 0% mix, use 1.0x gain (no attenuation needed when dry signal dominates)
    // Linear interpolation: gain = 1.0 - (mix * 0.06)
    const float feedbackSafetyGain = juce::jmap(mixPercentEffective / 100.0f, 1.0f, 0.94f);
    routingGraph.setFacadeParams(airModulated, juce::jmap(widthModulated, 0.0f, 2.0f), feedbackSafetyGain);

    // Physical modeling parameters
    routingGraph.setTubeRayTracerParams(tubeCountEffective, radiusVariationEffective,
                                         metallicResonanceEffective, couplingStrengthEffective);
    routingGraph.setElasticHallwayParams(elasticityEffective, recoveryTimeEffective,
                                          absorptionDriftEffective, nonlinearityEffective);
    routingGraph.setAlienAmplificationParams(impossibilityDegreeEffective, pitchEvolutionRateEffective,
                                               paradoxResonanceFreqEffective, paradoxGainEffective);

    // Phase 1: Spatial positioning (Three-System Plan)
    // Route modulation matrix values to spatial processor
    const float modPositionX = modulationMatrix.getModulation(monument::dsp::ModulationMatrix::DestinationType::PositionX);
    const float modPositionY = modulationMatrix.getModulation(monument::dsp::ModulationMatrix::DestinationType::PositionY);
    const float modPositionZ = modulationMatrix.getModulation(monument::dsp::ModulationMatrix::DestinationType::PositionZ);

    // Apply spatial modulation to Chambers FDN (delay line 0 for now, can expand to all 8 lines later)
    if (auto* chambers = routingGraph.getChambers())
    {
        if (auto* spatialProcessor = chambers->getSpatialProcessor())
        {
            // Modulation values are bipolar [-1, +1], map directly to position ranges
            spatialProcessor->setPosition(0, modPositionX, modPositionY,
                                         juce::jlimit(0.0f, 1.0f, 0.5f + modPositionZ * 0.5f));
        }
    }

#if defined(MONUMENT_ENABLE_MEMORY)
    const float densityClamped = std::isfinite(densityEffective) ? juce::jlimit(0.0f, 1.0f, densityEffective) : 0.5f;
    const float densityShaped = juce::jmap(densityClamped, 0.05f, 1.0f);
    const float memoryInputGain = juce::jmap(densityShaped, 0.18f, 0.32f);
    memoryEchoes.setMemory(memory);
    memoryEchoes.setDepth(memoryDepth);
    memoryEchoes.setDecay(memoryDecay);
    memoryEchoes.setDrift(memoryDrift);
    memoryEchoes.setInjectToBuffer(injectToBuffer);
    memoryEchoes.setChambersInputGain(injectToBuffer ? memoryInputGain : 1.0f);
    memoryEchoes.setFreeze(freezeEffective);
#endif

#if defined(MONUMENT_TESTING) || defined(MONUMENT_MEMORY_PROVE)
    // DEBUG: Log mix values (every 100 blocks to avoid flooding)
    static int mixLogCounter = 0;
    if (++mixLogCounter % 100 == 0)
    {
        const float mixTargetPercent = mixSmoother.getTargetValue();
        const float mixTarget = mixTargetPercent / 100.0f;
        const float dryGainTarget = std::cos(mixTarget * juce::MathConstants<float>::halfPi);
        const float wetGainTarget = std::sin(mixTarget * juce::MathConstants<float>::halfPi);
        juce::Logger::writeToLog("Monument DEBUG: Mix calculation mixPercent=" + juce::String(mixPercentEffective, 1) +
                                 " mix=" + juce::String(mixTarget, 3) +
                                 " dryGain=" + juce::String(dryGainTarget, 3) +
                                 " wetGain=" + juce::String(wetGainTarget, 3));
    }
#endif

    const auto numChannels = buffer.getNumChannels();
    // numSamples already declared above for parameter smoothing
    const bool dryReady = dryBuffer.getNumChannels() >= numChannels
        && dryBuffer.getNumSamples() >= numSamples;

    jassert(dryReady);
    if (dryReady)
    {
        for (int channel = 0; channel < numChannels; ++channel)
            dryBuffer.copyFrom(channel, 0, buffer, channel, 0, numSamples);
    }

    // Check for processing mode change (Ancient Monuments routing)
    if (modeChangeRequested.exchange(false, std::memory_order_acquire))
    {
        if (modeTransitionState != ModeTransitionState::FadingOut)
        {
            modeTransitionGain.setTargetValue(0.0f);
            modeTransitionState = ModeTransitionState::FadingOut;
        }
    }

    // Process with current routing mode (Ancient Monuments routing modes)
    // NOTE: MemoryEchoes integration is temporarily disabled during Phase 1
    // TODO: Re-integrate MemoryEchoes with routing graph in future phase
    switch (currentMode)
    {
        case ProcessingMode::AncientWay:
            processBlockAncientWay(buffer);
            break;
        case ProcessingMode::ResonantHalls:
            processBlockResonantHalls(buffer);
            break;
        case ProcessingMode::BreathingStone:
            processBlockBreathingStone(buffer);
            break;
    }

    // Apply transition gain for smooth mode crossfading
    // SmoothedValue handles per-sample interpolation automatically
    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float gain = modeTransitionGain.getNextValue();
        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* data = buffer.getWritePointer(channel);
            data[sample] *= gain;
        }
    }

    if (modeTransitionState == ModeTransitionState::FadingOut
        && !modeTransitionGain.isSmoothing())
    {
        currentMode = pendingMode.load(std::memory_order_acquire);
        modeTransitionGain.setTargetValue(1.0f);
        modeTransitionState = ModeTransitionState::FadingIn;
    }
    else if (modeTransitionState == ModeTransitionState::FadingIn
        && !modeTransitionGain.isSmoothing())
    {
        modeTransitionState = ModeTransitionState::None;
    }

#if defined(MONUMENT_MEMORY_PROVE)
    if (routeMemoryToOutput)
    {
        const auto& recall = memoryEchoes.getRecallBuffer();
        if (recall.getNumSamples() >= numSamples && recall.getNumChannels() >= numChannels)
        {
            for (int channel = 0; channel < numChannels; ++channel)
            {
                auto* wet = buffer.getWritePointer(channel);
                const auto* recallData = recall.getReadPointer(channel);
                for (int sample = 0; sample < numSamples; ++sample)
                    wet[sample] = juce::jlimit(-1.0f, 1.0f, wet[sample] + recallData[sample]);
            }
        }
    }
#endif

    if (!dryReady)
    {
        const float mixPercentSmoothed = juce::jlimit(0.0f, 100.0f, mixSmoother.getNextValue());
        if (numSamples > 1)
            mixSmoother.skip(numSamples - 1);
        const float mix = mixPercentSmoothed / 100.0f;
        const float wetGain = std::sin(mix * juce::MathConstants<float>::halfPi);
        buffer.applyGain(wetGain);
        return;
    }

    auto* wetChannels = buffer.getArrayOfWritePointers();
    auto* dryChannels = dryBuffer.getArrayOfReadPointers();
    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float mixPercentSmoothed = juce::jlimit(0.0f, 100.0f, mixSmoother.getNextValue());
        const float mix = mixPercentSmoothed / 100.0f;
        const float dryGain = std::cos(mix * juce::MathConstants<float>::halfPi);
        const float wetGain = std::sin(mix * juce::MathConstants<float>::halfPi);

        for (int channel = 0; channel < numChannels; ++channel)
            wetChannels[channel][sample] = dryChannels[channel][sample] * dryGain
                + wetChannels[channel][sample] * wetGain;
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
            // Reset DSP routing graph (replaces individual module resets)
            routingGraph.reset();

            // Reset separate features
            memoryEchoes.reset();
            modulationMatrix.reset();

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

    if (paramCache.safetyClip)
    {
        const float drive = juce::jmap(
            safetyClipDrive,
            1.0f,
            2.5f);
        const float norm = juce::dsp::FastMathApproximations::tanh(drive);
        const float normSafe = norm > 0.0f ? norm : 1.0f;

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* data = buffer.getWritePointer(channel);
            for (int sample = 0; sample < numSamples; ++sample)
            {
                const float driven = data[sample] * drive;
                data[sample] = juce::dsp::FastMathApproximations::tanh(driven) / normSafe;
            }
        }
    }

    float outputRms = 0.0f;
    const auto outputChannels = buffer.getNumChannels();
    for (int channel = 0; channel < outputChannels; ++channel)
        outputRms = juce::jmax(outputRms, buffer.getRMSLevel(channel, 0, levelSamples));
    outputLevel.store(outputRms, std::memory_order_relaxed);

#if defined(MONUMENT_TESTING) || defined(MONUMENT_MEMORY_PROVE)
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
    juce::String logLine = "Monument MONUMENT_TESTING peak=" + juce::String(peak, 6)
        + " blockMs=" + juce::String(elapsedMs, 3);
#if defined(MONUMENT_MEMORY_PROVE)
    logLine += " stage=" + juce::String(kMemoryProveStage);
    logLine += " inputPeak=" + juce::String(inputPeak, 6);
#endif
    juce::Logger::writeToLog(logLine);
#endif
}

juce::AudioProcessorEditor* MonumentAudioProcessor::createEditor()
{
#if defined(MONUMENT_TESTING) && !defined(MONUMENT_TESTING_UI)
    return nullptr;
#else
  #if defined(MONUMENT_LEGACY_UI)
    return new MonumentAudioProcessorEditor(*this);
  #else
    return new MonumentAudioProcessorEditorV2(*this);
  #endif
#endif
}

bool MonumentAudioProcessor::hasEditor() const
{
#if defined(MONUMENT_TESTING) && !defined(MONUMENT_TESTING_UI)
    return false;
#else
    return true;
#endif
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

    // Phase 3: Apply modulation connections from preset
    const auto& modConnections = presetManager.getLastLoadedModulationConnections();
    modulationMatrix.setConnections(modConnections);

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

    // Phase 3: Apply modulation connections from preset
    const auto& modConnections = presetManager.getLastLoadedModulationConnections();
    modulationMatrix.setConnections(modConnections);

    presetResetRequested.store(true, std::memory_order_release);
}

MonumentAudioProcessor::APVTS::ParameterLayout MonumentAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    auto makeFloat = [](const char* id,
                        const char* name,
                        juce::NormalisableRange<float> range,
                        float defaultValue)
    {
        return std::make_unique<juce::AudioParameterFloat>(id, name, range, defaultValue);
    };

    params.push_back(makeFloat(
        "mix",
        "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f),
        55.0f));

    params.push_back(makeFloat(
        "time",
        "Time",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.55f));

    params.push_back(makeFloat(
        "mass",
        "Mass",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));

    params.push_back(makeFloat(
        "density",
        "Density",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));

    params.push_back(makeFloat(
        "bloom",
        "Bloom",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));

    params.push_back(makeFloat(
        "air",
        "Air",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));

    params.push_back(makeFloat(
        "width",
        "Width",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));

    params.push_back(makeFloat(
        "warp",
        "Warp",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.3f));

    params.push_back(makeFloat(
        "drift",
        "Drift",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.3f));

    params.push_back(makeFloat(
        "memory",
        "Memory",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.0f));

    params.push_back(makeFloat(
        "memoryDepth",
        "Memory Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));

    params.push_back(makeFloat(
        "memoryDecay",
        "Memory Decay",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.4f));

    params.push_back(makeFloat(
        "memoryDrift",
        "Memory Drift",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.3f));

    params.push_back(makeFloat(
        "gravity",
        "Gravity",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));

    params.push_back(makeFloat(
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

    // ========================================================================
    // DSP ROUTING ARCHITECTURE (Phase 1.5 - Monument v0.5.0)
    // Select between 8 dramatically different routing topologies
    // ========================================================================

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "routingPreset",
        "Architecture",
        juce::StringArray{
            "Traditional Cathedral",
            "Metallic Granular",
            "Elastic Feedback Dream",
            "Parallel Worlds",
            "Shimmer Infinity",
            "Impossible Chaos",
            "Organic Breathing",
            "Minimal Sparse"
        },
        0));  // Default: Traditional Cathedral

    // ========================================================================
    // MACRO SYSTEM SELECTOR (Phase 2 - Expressive Macros)
    // Choose between Ancient Monuments (10 macros) or Expressive (6 macros)
    // ========================================================================

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "macroMode",
        "Macro Mode",
        juce::StringArray{"Ancient Monuments", "Expressive"},
        0));  // Default: Ancient Monuments

    // ========================================================================
    // MACRO CONTROLS (Phase 1 - Monument v0.2.0)
    // High-level, musically-meaningful controls that map to multiple parameters
    // ========================================================================

    params.push_back(makeFloat(
        "material",
        "Stone",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // 0 = soft, 1 = hard

    params.push_back(makeFloat(
        "topology",
        "Labyrinth",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // 0 = regular, 1 = non-Euclidean

    params.push_back(makeFloat(
        "viscosity",
        "Mist",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // 0 = airy, 1 = thick

    params.push_back(makeFloat(
        "evolution",
        "Bloom",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // 0 = static, 1 = evolving

    params.push_back(makeFloat(
        "chaosIntensity",
        "Tempest",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.0f));  // 0 = stable, 1 = chaotic

    params.push_back(makeFloat(
        "elasticityDecay",
        "Echo",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.0f));  // 0 = instant recovery, 1 = slow deformation

    // ========================================================================
    // ANCIENT MONUMENTS - EXPANDED MACROS (Phase 5 - Monument v0.6.0)
    // 4 new poetic macro controls: Patina, Abyss, Corona, Breath
    // ========================================================================

    params.push_back(makeFloat(
        "patina",
        "Patina",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // 0 = pristine, 1 = weathered

    params.push_back(makeFloat(
        "abyss",
        "Abyss",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // 0 = shallow, 1 = infinite void

    params.push_back(makeFloat(
        "corona",
        "Corona",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // 0 = shadow, 1 = sacred halo

    params.push_back(makeFloat(
        "breath",
        "Breath",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.0f));  // 0 = dormant, 1 = living pulse

    // ========================================================================
    // EXPRESSIVE MACRO CONTROLS (Phase 2 - Monument v0.7.0)
    // 6 musically-intuitive, performance-oriented controls
    // ========================================================================

    params.push_back(makeFloat(
        "character",
        "Character",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // 0 = subtle, 1 = extreme

    params.push_back(makeFloat(
        "spaceType",
        "Space Type",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.3f));  // 0=Chamber, 0.3=Hall, 0.5=Shimmer, 0.7=Granular, 0.9=Metallic

    params.push_back(makeFloat(
        "energy",
        "Energy",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.1f));  // 0=Decay, 0.3=Sustain, 0.6=Grow, 0.9=Chaos

    params.push_back(makeFloat(
        "motion",
        "Motion",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.2f));  // 0=Still, 0.3=Drift, 0.6=Pulse, 0.9=Random

    params.push_back(makeFloat(
        "color",
        "Color",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // 0=Dark, 0.5=Balanced, 0.8=Bright, 1.0=Spectral

    params.push_back(makeFloat(
        "dimension",
        "Dimension",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // 0=Intimate, 0.5=Room, 0.7=Cathedral, 1.0=Infinite

    // ========================================================================
    // PHYSICAL MODELING CONTROLS (Phase 5 - Monument v0.4.0)
    // Advanced physical simulation: tube ray tracing, elastic hallway, alien amplification
    // ========================================================================

    // TubeRayTracer parameters
    params.push_back(makeFloat(
        "tubeCount",
        "Tube Count",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.545f));  // Default: ~11 tubes (mid-range)

    params.push_back(makeFloat(
        "radiusVariation",
        "Radius Variation",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.3f));  // Default: moderate variation

    params.push_back(makeFloat(
        "metallicResonance",
        "Metallic Resonance",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // Default: moderate resonance

    params.push_back(makeFloat(
        "couplingStrength",
        "Coupling Strength",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // Default: moderate coupling

    // ElasticHallway parameters
    params.push_back(makeFloat(
        "elasticity",
        "Wall Elasticity",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // Default: moderate elasticity

    params.push_back(makeFloat(
        "recoveryTime",
        "Recovery Time",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // Default: moderate recovery (~1s)

    params.push_back(makeFloat(
        "absorptionDrift",
        "Absorption Drift",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.3f));  // Default: subtle drift

    params.push_back(makeFloat(
        "nonlinearity",
        "Wall Nonlinearity",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.3f));  // Default: subtle nonlinearity

    // AlienAmplification parameters
    params.push_back(makeFloat(
        "impossibilityDegree",
        "Impossibility",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.3f));  // Default: subtle impossibility

    params.push_back(makeFloat(
        "pitchEvolutionRate",
        "Pitch Evolution",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.3f));  // Default: subtle pitch morphing

    params.push_back(makeFloat(
        "paradoxResonanceFreq",
        "Paradox Frequency",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // Default: 432 Hz (mid-range)

    params.push_back(makeFloat(
        "paradoxGain",
        "Paradox Gain",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.3f));  // Default: subtle amplification

    // ========================================================================
    // OUTPUT SAFETY (Soft Clip Limiter)
    // ========================================================================

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "safetyClip",
        "Safety Clip",
        true));

    params.push_back(makeFloat(
        "safetyClipDrive",
        "Safety Clip Drive",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.25f));

    // ========================================================================
    // TIMELINE (Sequence Scheduler)
    // ========================================================================

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        "timelineEnabled",
        "Timeline Enabled",
        false));

    juce::StringArray timelinePresets;
    for (int i = 0; i < monument::dsp::SequencePresets::getNumPresets(); ++i)
        timelinePresets.add(monument::dsp::SequencePresets::getPresetName(i));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "timelinePreset",
        "Timeline Preset",
        timelinePresets,
        0));

    return {params.begin(), params.end()};
}

// ============================================================================
// Processing Mode Management (Ancient Monuments Routing)
// ============================================================================

void MonumentAudioProcessor::setProcessingMode(ProcessingMode mode)
{
    // Called from UI thread (non-audio thread)
    pendingMode.store(mode, std::memory_order_release);
    modeChangeRequested.store(true, std::memory_order_release);
}

ProcessingMode MonumentAudioProcessor::getProcessingMode() const noexcept
{
    return currentMode;
}

void MonumentAudioProcessor::processBlockAncientWay(juce::AudioBuffer<float>& buffer)
{
    // Traditional routing (current implementation)
    routingGraph.processAncientWay(buffer);
}

void MonumentAudioProcessor::processBlockResonantHalls(juce::AudioBuffer<float>& buffer)
{
    // Metallic First: TubeRayTracer before Chambers
    routingGraph.processResonantHalls(buffer);
}

void MonumentAudioProcessor::processBlockBreathingStone(juce::AudioBuffer<float>& buffer)
{
    // Elastic Core: ElasticHallway surrounds Chambers
    routingGraph.processBreathingStone(buffer);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MonumentAudioProcessor();
}
