#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "dsp/Chambers.h"

#include <cmath>
#if defined(MONUMENT_TESTING) || defined(MONUMENT_MEMORY_PROVE)
#include <mutex>
#endif

namespace
{
constexpr float kPresetFadeMs = 60.0f;

#if defined(MONUMENT_MEMORY_PROVE)
#ifndef MONUMENT_MEMORY_PROVE_STAGE
#define MONUMENT_MEMORY_PROVE_STAGE 0
#endif
constexpr int kMemoryProveStage = MONUMENT_MEMORY_PROVE_STAGE;
#endif

#if defined(MONUMENT_TESTING) || defined(MONUMENT_MEMORY_PROVE)
std::once_flag gTestingLoggerOnce;
std::unique_ptr<juce::FileLogger> gTestingLogger;

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
#endif

    const auto numChannels = getTotalNumOutputChannels();
    dryBuffer.setSize(numChannels, samplesPerBlock, false, false, true);
    dryBuffer.clear();

    // Prepare DSP routing graph (replaces individual module preparation)
    routingGraph.prepare(sampleRate, samplesPerBlock, numChannels);

    // Prepare separate features (not part of routing graph)
    memoryEchoes.prepare(sampleRate, samplesPerBlock, numChannels);
    modulationMatrix.prepare(sampleRate, samplesPerBlock, numChannels);
    sequenceScheduler.prepare(sampleRate, samplesPerBlock);  // Phase 4: Timeline automation

    // Initialize JUCE SmoothedValue for macro parameter smoothing (50ms ramp time)
    const double smoothingRampSeconds = 0.05;  // 50ms = smooth but responsive
    timeSmoother.reset(sampleRate, smoothingRampSeconds);
    massSmoother.reset(sampleRate, smoothingRampSeconds);
    densitySmoother.reset(sampleRate, smoothingRampSeconds);
    bloomSmoother.reset(sampleRate, smoothingRampSeconds);
    airSmoother.reset(sampleRate, smoothingRampSeconds);
    widthSmoother.reset(sampleRate, smoothingRampSeconds);
    warpSmoother.reset(sampleRate, smoothingRampSeconds);
    driftSmoother.reset(sampleRate, smoothingRampSeconds);
    gravitySmoother.reset(sampleRate, smoothingRampSeconds);
    pillarShapeSmoother.reset(sampleRate, smoothingRampSeconds);

    // Physical modeling parameter smoothers (50ms smoothing)
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

    // Initialize processing mode transition gain (starts at 1.0 for no fade)
    modeTransitionGain.reset(sampleRate, 0.05);  // 50ms fade time
    modeTransitionGain.setCurrentAndTargetValue(1.0f);

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

    // FIXED: Batch parameter atomic loads into cache (improves cache locality, reduces overhead)
    // This reduces 25+ sequential atomic loads with memory fences to a single structure update
    paramCache.mix = parameters.getRawParameterValue("mix")->load();
    paramCache.time = parameters.getRawParameterValue("time")->load();
    paramCache.mass = parameters.getRawParameterValue("mass")->load();
    paramCache.density = parameters.getRawParameterValue("density")->load();
    paramCache.bloom = parameters.getRawParameterValue("bloom")->load();
    paramCache.air = parameters.getRawParameterValue("air")->load();
    paramCache.width = parameters.getRawParameterValue("width")->load();
    paramCache.warp = parameters.getRawParameterValue("warp")->load();
    paramCache.drift = parameters.getRawParameterValue("drift")->load();
    paramCache.gravity = parameters.getRawParameterValue("gravity")->load();
    paramCache.pillarShape = parameters.getRawParameterValue("pillarShape")->load();
    paramCache.pillarMode = parameters.getRawParameterValue("pillarMode")->load();
    paramCache.memory = parameters.getRawParameterValue("memory")->load();
    paramCache.memoryDepth = parameters.getRawParameterValue("memoryDepth")->load();
    paramCache.memoryDecay = parameters.getRawParameterValue("memoryDecay")->load();
    paramCache.memoryDrift = parameters.getRawParameterValue("memoryDrift")->load();
    paramCache.freeze = parameters.getRawParameterValue("freeze")->load() > 0.5f;
    paramCache.material = parameters.getRawParameterValue("material")->load();
    paramCache.topology = parameters.getRawParameterValue("topology")->load();
    paramCache.viscosity = parameters.getRawParameterValue("viscosity")->load();
    paramCache.evolution = parameters.getRawParameterValue("evolution")->load();
    paramCache.chaosIntensity = parameters.getRawParameterValue("chaosIntensity")->load();
    paramCache.elasticityDecay = parameters.getRawParameterValue("elasticityDecay")->load();
    paramCache.patina = parameters.getRawParameterValue("patina")->load();
    paramCache.abyss = parameters.getRawParameterValue("abyss")->load();
    paramCache.corona = parameters.getRawParameterValue("corona")->load();
    paramCache.breath = parameters.getRawParameterValue("breath")->load();
    paramCache.character = parameters.getRawParameterValue("character")->load();
    paramCache.spaceType = parameters.getRawParameterValue("spaceType")->load();
    paramCache.energy = parameters.getRawParameterValue("energy")->load();
    paramCache.motion = parameters.getRawParameterValue("motion")->load();
    paramCache.color = parameters.getRawParameterValue("color")->load();
    paramCache.dimension = parameters.getRawParameterValue("dimension")->load();
    paramCache.tubeCount = parameters.getRawParameterValue("tubeCount")->load();
    paramCache.radiusVariation = parameters.getRawParameterValue("radiusVariation")->load();
    paramCache.metallicResonance = parameters.getRawParameterValue("metallicResonance")->load();
    paramCache.couplingStrength = parameters.getRawParameterValue("couplingStrength")->load();
    paramCache.elasticity = parameters.getRawParameterValue("elasticity")->load();
    paramCache.recoveryTime = parameters.getRawParameterValue("recoveryTime")->load();
    paramCache.absorptionDrift = parameters.getRawParameterValue("absorptionDrift")->load();
    paramCache.nonlinearity = parameters.getRawParameterValue("nonlinearity")->load();
    paramCache.impossibilityDegree = parameters.getRawParameterValue("impossibilityDegree")->load();
    paramCache.pitchEvolutionRate = parameters.getRawParameterValue("pitchEvolutionRate")->load();
    paramCache.paradoxResonanceFreq = parameters.getRawParameterValue("paradoxResonanceFreq")->load();
    paramCache.paradoxGain = parameters.getRawParameterValue("paradoxGain")->load();
    paramCache.routingPreset = parameters.getRawParameterValue("routingPreset")->load();
    paramCache.macroMode = parameters.getRawParameterValue("macroMode")->load();

    // Process sequence scheduler (Phase 4: Timeline automation)
    // This must happen BEFORE using paramCache values, so sequenced values can override them
    sequenceScheduler.process(getPlayHead()->getPosition(), buffer.getNumSamples());

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

    // Handle routing preset changes (Phase 1.5)
    const auto currentRoutingPreset = static_cast<int>(paramCache.routingPreset);
    if (currentRoutingPreset != lastRoutingPreset)
    {
        // Map parameter index to RoutingPresetType enum
        const auto presetType = static_cast<monument::dsp::RoutingPresetType>(currentRoutingPreset);
        routingGraph.loadRoutingPreset(presetType);
        lastRoutingPreset = currentRoutingPreset;
    }

    // Use cached values (no more atomic loads)
    const auto mixPercentRaw = paramCache.mix;
    const auto time = paramCache.time;
    const auto mass = paramCache.mass;
    const auto density = paramCache.density;
    const auto bloom = paramCache.bloom;
    const auto air = paramCache.air;
    const auto width = paramCache.width;
    const auto warp = paramCache.warp;
    const auto drift = paramCache.drift;
    const auto gravity = paramCache.gravity;
    const auto pillarShape = paramCache.pillarShape;
    const auto pillarModeRaw = paramCache.pillarMode;
    const auto memory = paramCache.memory;
    const auto memoryDepth = paramCache.memoryDepth;
    const auto memoryDecay = paramCache.memoryDecay;
    const auto memoryDrift = paramCache.memoryDrift;
    const auto freeze = paramCache.freeze;
    const auto material = paramCache.material;
    const auto topology = paramCache.topology;
    const auto viscosity = paramCache.viscosity;
    const auto evolution = paramCache.evolution;
    const auto chaosIntensity = paramCache.chaosIntensity;
    const auto elasticityDecay = paramCache.elasticityDecay;
    const auto patina = paramCache.patina;
    const auto abyss = paramCache.abyss;
    const auto corona = paramCache.corona;
    const auto breath = paramCache.breath;

    // Physical modeling parameters (Phase 5)
    const auto tubeCount = paramCache.tubeCount;
    const auto radiusVariation = paramCache.radiusVariation;
    const auto metallicResonance = paramCache.metallicResonance;
    const auto couplingStrength = paramCache.couplingStrength;
    const auto elasticity = paramCache.elasticity;
    const auto recoveryTime = paramCache.recoveryTime;
    const auto absorptionDrift = paramCache.absorptionDrift;
    const auto nonlinearity = paramCache.nonlinearity;
    const auto impossibilityDegree = paramCache.impossibilityDegree;
    const auto pitchEvolutionRate = paramCache.pitchEvolutionRate;
    const auto paradoxResonanceFreq = paramCache.paradoxResonanceFreq;
    const auto paradoxGain = paramCache.paradoxGain;

    // Compute macro-driven parameter targets
    // Phase 2: Choose between Ancient Monuments (10 macros) or Expressive (6 macros)
    monument::dsp::MacroMapper::ParameterTargets macroTargets;

    if (paramCache.macroMode < 0.5f)  // 0 = Ancient Monuments
    {
        // Ancient Monuments Phase 5 - 10 macros
        macroTargets = macroMapper.computeTargets(
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
    }
    else  // 1 = Expressive Macros
    {
        // Expressive Macros Phase 2 - 6 macros
        auto expressiveTargets = expressiveMacroMapper.computeTargets(
            paramCache.character,
            paramCache.spaceType,
            paramCache.energy,
            paramCache.motion,
            paramCache.color,
            paramCache.dimension);

        // Convert ExpressiveMacroMapper::ParameterTargets to MacroMapper::ParameterTargets
        // (both have same structure, just copy values)
        macroTargets.time = expressiveTargets.time;
        macroTargets.mass = expressiveTargets.mass;
        macroTargets.density = expressiveTargets.density;
        macroTargets.bloom = expressiveTargets.bloom;
        macroTargets.air = expressiveTargets.air;
        macroTargets.width = expressiveTargets.width;
        macroTargets.mix = expressiveTargets.mix;
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

        // Expressive Macros also selects routing preset via Space Type!
        const auto expressiveRouting = static_cast<int>(expressiveTargets.routingPreset);
        if (expressiveRouting != lastRoutingPreset)
        {
            routingGraph.loadRoutingPreset(expressiveTargets.routingPreset);
            lastRoutingPreset = expressiveRouting;
        }
    }

    // Process modulation matrix (stub sources for Phase 2, returns 0 for all destinations)
    modulationMatrix.process(buffer, buffer.getNumSamples());

    // Calculate macro influence: how far are macros from their defaults?
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
    const float macroInfluence = juce::jmin(1.0f,
        (materialDelta + topologyDelta + viscosityDelta + evolutionDelta + chaosDelta + elasticityDelta +
         patinaDelta + abyssDelta + coronaDelta + breathDelta) * (2.0f * 6.0f / 10.0f));

    // Blend base parameters with macro targets based on macro influence
    // When macroInfluence = 0, use base parameters; when = 1, use macro targets
    // Set target values on JUCE SmoothedValue (50ms ramp)
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

    // Get current smoothed values (block-rate processing)
    const float timeEffective = timeSmoother.getCurrentValue();
    const float massEffective = massSmoother.getCurrentValue();
    const float densityEffective = densitySmoother.getCurrentValue();
    const float bloomEffectiveMacro = bloomSmoother.getCurrentValue();
    const float airEffective = airSmoother.getCurrentValue();
    const float widthEffective = widthSmoother.getCurrentValue();
    const float warpEffectiveMacro = warpSmoother.getCurrentValue();
    const float driftEffectiveMacro = driftSmoother.getCurrentValue();
    const float gravityEffective = gravitySmoother.getCurrentValue();
    const float pillarShapeEffective = pillarShapeSmoother.getCurrentValue();

    // FIXED: Only advance smoothers that are actively ramping (skip the rest for performance)
    // This avoids unnecessary calculations when parameters are stable
    const auto blockSize = buffer.getNumSamples();
    if (timeSmoother.isSmoothing()) timeSmoother.skip(blockSize);
    if (massSmoother.isSmoothing()) massSmoother.skip(blockSize);
    if (densitySmoother.isSmoothing()) densitySmoother.skip(blockSize);
    if (bloomSmoother.isSmoothing()) bloomSmoother.skip(blockSize);
    if (airSmoother.isSmoothing()) airSmoother.skip(blockSize);
    if (widthSmoother.isSmoothing()) widthSmoother.skip(blockSize);
    if (warpSmoother.isSmoothing()) warpSmoother.skip(blockSize);
    if (driftSmoother.isSmoothing()) driftSmoother.skip(blockSize);
    if (gravitySmoother.isSmoothing()) gravitySmoother.skip(blockSize);
    if (pillarShapeSmoother.isSmoothing()) pillarShapeSmoother.skip(blockSize);

    // Physical modeling parameter smoothers (Phase 5)
    tubeCountSmoother.setTargetValue(tubeCount);
    radiusVariationSmoother.setTargetValue(radiusVariation);
    metallicResonanceSmoother.setTargetValue(metallicResonance);
    couplingStrengthSmoother.setTargetValue(couplingStrength);
    elasticitySmoother.setTargetValue(elasticity);
    recoveryTimeSmoother.setTargetValue(recoveryTime);
    absorptionDriftSmoother.setTargetValue(absorptionDrift);
    nonlinearitySmoother.setTargetValue(nonlinearity);
    impossibilityDegreeSmoother.setTargetValue(impossibilityDegree);
    pitchEvolutionRateSmoother.setTargetValue(pitchEvolutionRate);
    paradoxResonanceFreqSmoother.setTargetValue(paradoxResonanceFreq);
    paradoxGainSmoother.setTargetValue(paradoxGain);

    // Get current smoothed values (block-rate)
    const float tubeCountEffective = tubeCountSmoother.getCurrentValue();
    const float radiusVariationEffective = radiusVariationSmoother.getCurrentValue();
    const float metallicResonanceEffective = metallicResonanceSmoother.getCurrentValue();
    const float couplingStrengthEffective = couplingStrengthSmoother.getCurrentValue();
    const float elasticityEffective = elasticitySmoother.getCurrentValue();
    const float recoveryTimeEffective = recoveryTimeSmoother.getCurrentValue();
    const float absorptionDriftEffective = absorptionDriftSmoother.getCurrentValue();
    const float nonlinearityEffective = nonlinearitySmoother.getCurrentValue();
    const float impossibilityDegreeEffective = impossibilityDegreeSmoother.getCurrentValue();
    const float pitchEvolutionRateEffective = pitchEvolutionRateSmoother.getCurrentValue();
    const float paradoxResonanceFreqEffective = paradoxResonanceFreqSmoother.getCurrentValue();
    const float paradoxGainEffective = paradoxGainSmoother.getCurrentValue();

    // Skip physical modeling smoothers if actively ramping
    if (tubeCountSmoother.isSmoothing()) tubeCountSmoother.skip(blockSize);
    if (radiusVariationSmoother.isSmoothing()) radiusVariationSmoother.skip(blockSize);
    if (metallicResonanceSmoother.isSmoothing()) metallicResonanceSmoother.skip(blockSize);
    if (couplingStrengthSmoother.isSmoothing()) couplingStrengthSmoother.skip(blockSize);
    if (elasticitySmoother.isSmoothing()) elasticitySmoother.skip(blockSize);
    if (recoveryTimeSmoother.isSmoothing()) recoveryTimeSmoother.skip(blockSize);
    if (absorptionDriftSmoother.isSmoothing()) absorptionDriftSmoother.skip(blockSize);
    if (nonlinearitySmoother.isSmoothing()) nonlinearitySmoother.skip(blockSize);
    if (impossibilityDegreeSmoother.isSmoothing()) impossibilityDegreeSmoother.skip(blockSize);
    if (pitchEvolutionRateSmoother.isSmoothing()) pitchEvolutionRateSmoother.skip(blockSize);
    if (paradoxResonanceFreqSmoother.isSmoothing()) paradoxResonanceFreqSmoother.skip(blockSize);
    if (paradoxGainSmoother.isSmoothing()) paradoxGainSmoother.skip(blockSize);

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

    const float mixPercent = std::isfinite(mixPercentRaw) ? mixPercentRaw : 0.0f;
    const float mixPercentEffective = forceWet ? 100.0f : mixPercent;
    const bool freezeEffective = forceFreezeOff ? false : freeze;
    // Use modulated values (Phase 3: modulation system now active)
    const float warpEffective = allowModulation ? warpModulated : 0.0f;
    const float driftEffective = allowModulation ? driftModulated : 0.0f;
    const float bloomEffective = allowModulation ? bloomModulated : 0.0f;
    float pillarModeSafe = std::isfinite(pillarModeRaw) ? pillarModeRaw : 0.0f;
    pillarModeSafe = juce::jlimit(0.0f, 2.0f, pillarModeSafe);

    if (presetResetRequested.exchange(false, std::memory_order_acq_rel))
    {
        presetTransition = PresetTransitionState::FadingOut;
        presetFadeRemaining = presetFadeSamples;
    }

    // Forward parameters to DSP routing graph (replaces individual module parameter setting)
    routingGraph.setPillarsParams(densityModulated, pillarShapeModulated, warpEffective);
    routingGraph.setChambersParams(timeModulated, massModulated, densityModulated,
                                     bloomEffective, gravityModulated);
    routingGraph.setWeatheringParams(warpEffective, driftEffective);
    routingGraph.setButtressParams(juce::jmap(massModulated, 0.9f, 1.6f), 0.0f);
    routingGraph.setFacadeParams(airModulated, juce::jmap(widthModulated, 0.0f, 2.0f), 1.0f);

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

    const auto mix = juce::jlimit(0.0f, 100.0f, mixPercentEffective) / 100.0f;
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

    // Check for processing mode change (Ancient Monuments routing)
    if (modeChangeRequested.exchange(false, std::memory_order_acquire))
    {
        // Fade out current mode (50ms)
        modeTransitionGain.setTargetValue(0.0f);

        // Wait for fade to complete (handled smoothly by SmoothedValue)
        // After processing this block at reduced gain, switch mode

        // Switch to pending mode
        currentMode = pendingMode.load(std::memory_order_acquire);

        // Fade in new mode (50ms)
        modeTransitionGain.setTargetValue(1.0f);
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
        "memory",
        "Memory",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "memoryDepth",
        "Memory Depth",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "memoryDecay",
        "Memory Decay",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.4f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "memoryDrift",
        "Memory Drift",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.3f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "gravity",
        "Gravity",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
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

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "material",
        "Material",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // 0 = soft, 1 = hard

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "topology",
        "Topology",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // 0 = regular, 1 = non-Euclidean

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "viscosity",
        "Viscosity",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // 0 = airy, 1 = thick

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "evolution",
        "Evolution",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // 0 = static, 1 = evolving

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "chaosIntensity",
        "Chaos",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.0f));  // 0 = stable, 1 = chaotic

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "elasticityDecay",
        "Elasticity",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.0f));  // 0 = instant recovery, 1 = slow deformation

    // ========================================================================
    // ANCIENT MONUMENTS - EXPANDED MACROS (Phase 5 - Monument v0.6.0)
    // 4 new poetic macro controls: Patina, Abyss, Corona, Breath
    // ========================================================================

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "patina",
        "Patina",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // 0 = pristine, 1 = weathered

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "abyss",
        "Abyss",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // 0 = shallow, 1 = infinite void

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "corona",
        "Corona",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // 0 = shadow, 1 = sacred halo

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "breath",
        "Breath",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.0f));  // 0 = dormant, 1 = living pulse

    // ========================================================================
    // EXPRESSIVE MACRO CONTROLS (Phase 2 - Monument v0.7.0)
    // 6 musically-intuitive, performance-oriented controls
    // ========================================================================

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "character",
        "Character",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // 0 = subtle, 1 = extreme

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "spaceType",
        "Space Type",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.3f));  // 0=Chamber, 0.3=Hall, 0.5=Shimmer, 0.7=Granular, 0.9=Metallic

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "energy",
        "Energy",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.1f));  // 0=Decay, 0.3=Sustain, 0.6=Grow, 0.9=Chaos

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "motion",
        "Motion",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.2f));  // 0=Still, 0.3=Drift, 0.6=Pulse, 0.9=Random

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "color",
        "Color",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // 0=Dark, 0.5=Balanced, 0.8=Bright, 1.0=Spectral

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "dimension",
        "Dimension",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // 0=Intimate, 0.5=Room, 0.7=Cathedral, 1.0=Infinite

    // ========================================================================
    // PHYSICAL MODELING CONTROLS (Phase 5 - Monument v0.4.0)
    // Advanced physical simulation: tube ray tracing, elastic hallway, alien amplification
    // ========================================================================

    // TubeRayTracer parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "tubeCount",
        "Tube Count",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.545f));  // Default: ~11 tubes (mid-range)

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "radiusVariation",
        "Radius Variation",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.3f));  // Default: moderate variation

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "metallicResonance",
        "Metallic Resonance",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // Default: moderate resonance

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "couplingStrength",
        "Coupling Strength",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // Default: moderate coupling

    // ElasticHallway parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "elasticity",
        "Wall Elasticity",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // Default: moderate elasticity

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "recoveryTime",
        "Recovery Time",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // Default: moderate recovery (~1s)

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "absorptionDrift",
        "Absorption Drift",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.3f));  // Default: subtle drift

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "nonlinearity",
        "Wall Nonlinearity",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.3f));  // Default: subtle nonlinearity

    // AlienAmplification parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "impossibilityDegree",
        "Impossibility",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.3f));  // Default: subtle impossibility

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "pitchEvolutionRate",
        "Pitch Evolution",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.3f));  // Default: subtle pitch morphing

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "paradoxResonanceFreq",
        "Paradox Frequency",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));  // Default: 432 Hz (mid-range)

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "paradoxGain",
        "Paradox Gain",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.3f));  // Default: subtle amplification

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
