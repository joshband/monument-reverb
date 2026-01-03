#include "PluginProcessor.h"
#include "PluginEditor.h"

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
      presetManager(parameters)
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
#if defined(MONUMENT_TESTING) || defined(MONUMENT_MEMORY_PROVE)
    ensureTestingLogger();
#endif

    const auto numChannels = getTotalNumOutputChannels();
    dryBuffer.setSize(numChannels, samplesPerBlock, false, false, true);
    dryBuffer.clear();

    foundation.prepare(sampleRate, samplesPerBlock, numChannels);
    pillars.prepare(sampleRate, samplesPerBlock, numChannels);
    chambers.prepare(sampleRate, samplesPerBlock, numChannels);
    memoryEchoes.prepare(sampleRate, samplesPerBlock, numChannels);
    weathering.prepare(sampleRate, samplesPerBlock, numChannels);
    buttress.prepare(sampleRate, samplesPerBlock, numChannels);
    facade.prepare(sampleRate, samplesPerBlock, numChannels);
    modulationMatrix.prepare(sampleRate, samplesPerBlock, numChannels);

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
    foundation.reset();
    pillars.reset();
    chambers.reset();
    memoryEchoes.reset();
    weathering.reset();
    buttress.reset();
    facade.reset();
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

    const auto mixPercentRaw = parameters.getRawParameterValue("mix")->load();
    const auto time = parameters.getRawParameterValue("time")->load();
    const auto mass = parameters.getRawParameterValue("mass")->load();
    const auto density = parameters.getRawParameterValue("density")->load();
    const auto bloom = parameters.getRawParameterValue("bloom")->load();
    const auto air = parameters.getRawParameterValue("air")->load();
    const auto width = parameters.getRawParameterValue("width")->load();
    const auto warp = parameters.getRawParameterValue("warp")->load();
    const auto drift = parameters.getRawParameterValue("drift")->load();
    const auto gravity = parameters.getRawParameterValue("gravity")->load();
    const auto pillarShape = parameters.getRawParameterValue("pillarShape")->load();
    const auto pillarModeRaw = parameters.getRawParameterValue("pillarMode")->load();
    const auto memory = parameters.getRawParameterValue("memory")->load();
    const auto memoryDepth = parameters.getRawParameterValue("memoryDepth")->load();
    const auto memoryDecay = parameters.getRawParameterValue("memoryDecay")->load();
    const auto memoryDrift = parameters.getRawParameterValue("memoryDrift")->load();
    const auto freeze = parameters.getRawParameterValue("freeze")->load() > 0.5f;

    // Phase 2: Poll macro parameters
    const auto material = parameters.getRawParameterValue("material")->load();
    const auto topology = parameters.getRawParameterValue("topology")->load();
    const auto viscosity = parameters.getRawParameterValue("viscosity")->load();
    const auto evolution = parameters.getRawParameterValue("evolution")->load();
    const auto chaosIntensity = parameters.getRawParameterValue("chaosIntensity")->load();
    const auto elasticityDecay = parameters.getRawParameterValue("elasticityDecay")->load();

    // Compute macro-driven parameter targets
    const auto macroTargets = macroMapper.computeTargets(
        material, topology, viscosity, evolution, chaosIntensity, elasticityDecay);

    // Process modulation matrix (stub sources for Phase 2, returns 0 for all destinations)
    modulationMatrix.process(buffer, buffer.getNumSamples());

    // Calculate macro influence: how far are macros from their defaults?
    // Defaults: material=0.5, topology=0.5, viscosity=0.5, evolution=0.5, chaos=0.0, elasticity=0.0
    const float materialDelta = std::abs(material - 0.5f);
    const float topologyDelta = std::abs(topology - 0.5f);
    const float viscosityDelta = std::abs(viscosity - 0.5f);
    const float evolutionDelta = std::abs(evolution - 0.5f);
    const float chaosDelta = std::abs(chaosIntensity - 0.0f);
    const float elasticityDelta = std::abs(elasticityDecay - 0.0f);

    // Macro influence: 0 = all at defaults, 1 = at least one macro significantly moved
    const float macroInfluence = juce::jmin(1.0f,
        (materialDelta + topologyDelta + viscosityDelta + evolutionDelta + chaosDelta + elasticityDelta) * 2.0f);

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

    // Advance smoothers for the block (ramp continues across samples)
    timeSmoother.skip(buffer.getNumSamples());
    massSmoother.skip(buffer.getNumSamples());
    densitySmoother.skip(buffer.getNumSamples());
    bloomSmoother.skip(buffer.getNumSamples());
    airSmoother.skip(buffer.getNumSamples());
    widthSmoother.skip(buffer.getNumSamples());
    warpSmoother.skip(buffer.getNumSamples());
    driftSmoother.skip(buffer.getNumSamples());
    gravitySmoother.skip(buffer.getNumSamples());
    pillarShapeSmoother.skip(buffer.getNumSamples());

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

    // Apply modulated parameters to DSP modules (Phase 3: modulation system active)
    pillars.setDensity(densityModulated);
    pillars.setWarp(warpEffective);
    pillars.setShape(pillarShapeModulated);
    pillars.setMode(static_cast<int>(std::round(pillarModeSafe)));
    chambers.setTime(timeModulated);
    chambers.setMass(massModulated);
    chambers.setDensity(densityModulated);
    chambers.setBloom(bloomEffective);
    chambers.setGravity(gravityModulated);
    chambers.setFreeze(freezeEffective);
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
    weathering.setWarp(warpEffective);
    weathering.setDrift(driftEffective);
    buttress.setDrive(juce::jmap(massModulated, 0.9f, 1.6f));
    buttress.setFreeze(freezeEffective);
    facade.setAir(airModulated);
    facade.setWidth(juce::jmap(widthModulated, 0.0f, 2.0f));

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

    foundation.process(buffer);
    pillars.process(buffer);
#if defined(MONUMENT_ENABLE_MEMORY)
    // Memory Echoes renders recall fragments; Chambers injects them into the FDN input.
    memoryEchoes.process(buffer);
    if (!bypassChambers)
    {
        chambers.setExternalInjection(&memoryEchoes.getRecallBuffer());
        chambers.process(buffer);
    }
    memoryEchoes.captureWet(buffer);
#else
    if (!bypassChambers)
        chambers.process(buffer);
#endif
    weathering.process(buffer);
    buttress.process(buffer);
    facade.process(buffer);

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
            foundation.reset();
            pillars.reset();
            chambers.reset();
            memoryEchoes.reset();
            weathering.reset();
            buttress.reset();
            facade.reset();
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

    return {params.begin(), params.end()};
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MonumentAudioProcessor();
}
