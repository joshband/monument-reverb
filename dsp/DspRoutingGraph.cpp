#include "DspRoutingGraph.h"
#include "dsp/DspModules.h"
#include "dsp/TubeRayTracer.h"
#include "dsp/ElasticHallway.h"
#include "dsp/AlienAmplification.h"
#include <initializer_list>

namespace monument
{
namespace dsp
{

//==============================================================================
// Constructor / Destructor
//==============================================================================

DspRoutingGraph::DspRoutingGraph()
{
    // Initialize all modules
    foundation = std::make_unique<Foundation>();
    pillars = std::make_unique<Pillars>();
    chambers = std::make_unique<Chambers>();
    weathering = std::make_unique<Weathering>();
    tubeRayTracer = std::make_unique<TubeRayTracer>();
    elasticHallway = std::make_unique<ElasticHallway>();
    alienAmplification = std::make_unique<AlienAmplification>();
    buttress = std::make_unique<Buttress>();
    facade = std::make_unique<Facade>();

    routingConnections.reserve(kMaxRoutingConnections);
    buildPresetData();
    loadRoutingPreset(RoutingPresetType::TraditionalCathedral);
}

DspRoutingGraph::~DspRoutingGraph() = default;

//==============================================================================
// Preparation
//==============================================================================

void DspRoutingGraph::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    sampleRateHz = sampleRate;
    maxBlockSizeInternal = maxBlockSize;
    numChannelsInternal = numChannels;

    // Prepare all modules
    foundation->prepare(sampleRate, maxBlockSize, numChannels);
    pillars->prepare(sampleRate, maxBlockSize, numChannels);
    chambers->prepare(sampleRate, maxBlockSize, numChannels);
    weathering->prepare(sampleRate, maxBlockSize, numChannels);
    tubeRayTracer->prepare(sampleRate, maxBlockSize, numChannels);
    elasticHallway->prepare(sampleRate, maxBlockSize, numChannels);
    alienAmplification->prepare(sampleRate, maxBlockSize, numChannels);
    buttress->prepare(sampleRate, maxBlockSize, numChannels);
    facade->prepare(sampleRate, maxBlockSize, numChannels);

    // Allocate temp buffers for parallel processing
    for (auto& buffer : tempBuffers)
    {
        buffer.setSize(numChannels, maxBlockSize);
        buffer.clear();
    }

    for (auto& buffer : moduleOutputBuffers)
    {
        buffer.setSize(numChannels, maxBlockSize);
        buffer.clear();
    }

    feedbackBuffer.setSize(numChannels, maxBlockSize);
    feedbackBuffer.clear();

    dryBuffer.setSize(numChannels, maxBlockSize);
    dryBuffer.clear();

    // Initialize feedback safety components
    feedbackGainSmoothed.reset(sampleRate, 0.05);  // 50ms smoothing to prevent clicks
    feedbackGainSmoothed.setCurrentAndTargetValue(0.0f);

    // Low-pass filter at 8kHz to prevent high-frequency buildup in feedback loops
    auto coefficients = juce::dsp::IIR::Coefficients<float>::makeLowPass(sampleRate, 8000.0);
    feedbackLowpassL.coefficients = coefficients;
    feedbackLowpassR.coefficients = coefficients;
    feedbackLowpassL.reset();
    feedbackLowpassR.reset();

    // Load default routing preset
    loadRoutingPreset(RoutingPresetType::TraditionalCathedral);
    isPrepared = true;
}

void DspRoutingGraph::reset()
{
    foundation->reset();
    pillars->reset();
    chambers->reset();
    weathering->reset();
    tubeRayTracer->reset();
    elasticHallway->reset();
    alienAmplification->reset();
    buttress->reset();
    facade->reset();

    // Reset all buffers
    feedbackBuffer.clear();
    dryBuffer.clear();
    for (auto& buffer : tempBuffers)
        buffer.clear();
    for (auto& buffer : moduleOutputBuffers)
        buffer.clear();

    // Reset feedback safety components
    feedbackGainSmoothed.setCurrentAndTargetValue(0.0f);
    feedbackLowpassL.reset();
    feedbackLowpassR.reset();
}

//==============================================================================
// Processing
//==============================================================================

void DspRoutingGraph::process(juce::AudioBuffer<float>& buffer)
{
    if (!isPrepared)
    {
        jassertfalse;
        buffer.clear();
        return;
    }

    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    // Save dry signal for parallel modes (dedicated buffer for clarity)
    const bool dryReady = dryBuffer.getNumChannels() >= numChannels
        && dryBuffer.getNumSamples() >= numSamples;
    jassert(dryReady);
    if (dryReady)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            dryBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
    }

    // Lock-free preset read: Load current preset index atomically
    const size_t presetIdx = activePresetIndex.load(std::memory_order_acquire);
    const auto& currentPresetData = presetData[presetIdx];
    const uint32_t bypassMaskValue = bypassMask.load(std::memory_order_acquire);

    std::array<bool, static_cast<size_t>(ModuleType::Count)> moduleHasOutput{};

    auto isBypassed = [bypassMaskValue](ModuleType module)
    {
        return (bypassMaskValue & moduleBit(module)) != 0;
    };

    auto copyBuffer = [numChannels, numSamples](juce::AudioBuffer<float>& dest,
                                                const juce::AudioBuffer<float>& src)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            dest.copyFrom(ch, 0, src, ch, 0, numSamples);
    };

    auto ensureModuleOutput = [&](ModuleType module,
                                  const juce::AudioBuffer<float>& input) -> juce::AudioBuffer<float>&
    {
        const auto idx = static_cast<size_t>(module);
        auto& outBuf = moduleOutputBuffers[idx];
        if (!moduleHasOutput[idx])
        {
            copyBuffer(outBuf, input);
            if (!isBypassed(module))
                processModule(module, outBuf, bypassMaskValue);
            moduleHasOutput[idx] = true;
        }
        return outBuf;
    };

    // Process each connection in order (from pre-allocated preset data)
    for (size_t i = 0; i < currentPresetData.connectionCount; ++i)
    {
        const auto& conn = currentPresetData.connections[i];

        // Skip disabled connections
        if (!conn.enabled)
            continue;

        auto& sourceBuf = ensureModuleOutput(conn.source, dryBuffer);

        switch (conn.mode)
        {
            case RoutingMode::Series:
            {
                auto& destBuf = ensureModuleOutput(conn.destination, sourceBuf);
                copyBuffer(buffer, destBuf);
                break;
            }

            case RoutingMode::Parallel:
            {
                // Skip if destination module is bypassed (early exit before buffer ops)
                if (isBypassed(conn.destination))
                {
                    ensureModuleOutput(conn.destination, sourceBuf);
                    break;
                }

                // Parallel: process in temp buffer, then blend with main
                auto& parallelBuf = tempBuffers[static_cast<size_t>(conn.destination)];

                copyBuffer(parallelBuf, sourceBuf);

                processModule(conn.destination, parallelBuf, bypassMaskValue);
                copyBuffer(moduleOutputBuffers[static_cast<size_t>(conn.destination)], parallelBuf);
                moduleHasOutput[static_cast<size_t>(conn.destination)] = true;

                // Use JUCE's optimized addFrom with gain
                for (int ch = 0; ch < numChannels; ++ch)
                    buffer.addFrom(ch, 0, parallelBuf, ch, 0, numSamples, conn.blendAmount);
                break;
            }

            case RoutingMode::ParallelMix:
            {
                // Skip if destination module is bypassed
                if (isBypassed(conn.destination))
                {
                    ensureModuleOutput(conn.destination, sourceBuf);
                    break;
                }

                // Parallel with dry mix: process module, blend with original dry
                auto& parallelBuf = tempBuffers[static_cast<size_t>(conn.destination)];

                copyBuffer(parallelBuf, sourceBuf);

                processModule(conn.destination, parallelBuf, bypassMaskValue);
                copyBuffer(moduleOutputBuffers[static_cast<size_t>(conn.destination)], parallelBuf);
                moduleHasOutput[static_cast<size_t>(conn.destination)] = true;

                // Optimized mix using JUCE buffer operations
                const float dryGain = 1.0f - conn.blendAmount;
                const float wetGain = conn.blendAmount;

                for (int ch = 0; ch < numChannels; ++ch)
                {
                    buffer.copyFrom(ch, 0, dryBuffer, ch, 0, numSamples);
                    buffer.applyGain(ch, 0, numSamples, dryGain);
                    buffer.addFrom(ch, 0, parallelBuf, ch, 0, numSamples, wetGain);
                }
                break;
            }

            case RoutingMode::Feedback:
            {
                if (isBypassed(conn.destination))
                {
                    copyBuffer(feedbackBuffer, sourceBuf);
                    if (numChannels >= 1)
                    {
                        auto* dataL = feedbackBuffer.getWritePointer(0);
                        for (int i = 0; i < numSamples; ++i)
                            dataL[i] = feedbackLowpassL.processSample(dataL[i]);
                    }
                    if (numChannels >= 2)
                    {
                        auto* dataR = feedbackBuffer.getWritePointer(1);
                        for (int i = 0; i < numSamples; ++i)
                            dataR[i] = feedbackLowpassR.processSample(dataR[i]);
                    }
                    break;
                }

                // Clamp feedback gain to safety limit
                const float safeGain = juce::jlimit(0.0f, kMaxFeedbackGain, conn.feedbackGain);
                feedbackGainSmoothed.setTargetValue(safeGain);

                copyBuffer(buffer, sourceBuf);

                // Mix feedback buffer into input with smoothed gain
                for (int sample = 0; sample < numSamples; ++sample)
                {
                    const float smoothedGain = feedbackGainSmoothed.getNextValue();
                    for (int ch = 0; ch < numChannels; ++ch)
                    {
                        float fbSample = feedbackBuffer.getSample(ch, sample);
                        buffer.setSample(ch, sample,
                            buffer.getSample(ch, sample) + fbSample * smoothedGain);
                    }
                }

                // Process module
                processModule(conn.destination, buffer, bypassMaskValue);
                copyBuffer(moduleOutputBuffers[static_cast<size_t>(conn.destination)], buffer);
                moduleHasOutput[static_cast<size_t>(conn.destination)] = true;

                // Update feedback buffer using source output (1-block delay)
                copyBuffer(feedbackBuffer, sourceBuf);

                // Apply low-pass filter to feedback to prevent high-frequency buildup
                if (numChannels >= 1)
                {
                    auto* dataL = feedbackBuffer.getWritePointer(0);
                    for (int i = 0; i < numSamples; ++i)
                        dataL[i] = feedbackLowpassL.processSample(dataL[i]);
                }
                if (numChannels >= 2)
                {
                    auto* dataR = feedbackBuffer.getWritePointer(1);
                    for (int i = 0; i < numSamples; ++i)
                        dataR[i] = feedbackLowpassR.processSample(dataR[i]);
                }
                break;
            }

            case RoutingMode::Crossfeed:
            {
                // L/R channel crossfeed using optimized operations
                if (numChannels >= 2)
                {
                    const float crossfeed = conn.crossfeedAmount;
                    const float dryAmount = 1.0f - crossfeed;

                    // Store original channels in temp buffer
                    auto& tempL = tempBuffers[0];
                    auto& tempR = tempBuffers[1];
                    tempL.copyFrom(0, 0, buffer, 0, 0, numSamples);
                    tempR.copyFrom(0, 0, buffer, 1, 0, numSamples);

                    // L = L * dry + (L+R)/2 * crossfeed
                    // R = R * dry + (L+R)/2 * crossfeed
                    for (int ch = 0; ch < 2; ++ch)
                    {
                        buffer.applyGain(ch, 0, numSamples, dryAmount);
                        buffer.addFrom(ch, 0, tempL, 0, 0, numSamples, crossfeed * 0.5f);
                        buffer.addFrom(ch, 0, tempR, 0, 0, numSamples, crossfeed * 0.5f);
                    }
                }
                break;
            }

            case RoutingMode::Bypass:
                // Skip this module entirely
                break;
        }
    }
}

//==============================================================================
// Routing Configuration
//==============================================================================

void DspRoutingGraph::buildPresetData()
{
    auto fillPreset = [this](
        RoutingPresetType preset,
        std::initializer_list<RoutingConnection> connections,
        std::initializer_list<ModuleType> bypassed = {})
    {
        auto& data = presetData[static_cast<size_t>(preset)];
        data.connectionCount = 0;
        data.bypass.fill(false);

        for (const auto& connection : connections)
        {
            if (data.connectionCount >= kMaxRoutingConnections)
            {
                jassertfalse;
                break;
            }
            data.connections[data.connectionCount++] = connection;
        }

        for (const auto module : bypassed)
            data.bypass[static_cast<size_t>(module)] = true;

        data.bypassMask = computeBypassMask(data.bypass);
    };

    // Foundation → Pillars → Chambers → Weathering → Facade
    fillPreset(RoutingPresetType::TraditionalCathedral, {
        {ModuleType::Foundation, ModuleType::Pillars},
        {ModuleType::Pillars, ModuleType::Chambers},
        {ModuleType::Chambers, ModuleType::Weathering},
        {ModuleType::Weathering, ModuleType::Facade}
    });

    // Foundation → Pillars → TubeRayTracer → Facade (bypass Chambers)
    fillPreset(RoutingPresetType::MetallicGranular, {
        {ModuleType::Foundation, ModuleType::Pillars},
        {ModuleType::Pillars, ModuleType::TubeRayTracer},
        {ModuleType::TubeRayTracer, ModuleType::Facade}
    }, {ModuleType::Chambers});

    // Foundation → Pillars → ElasticHallway → Chambers → AlienAmplification → Facade
    RoutingConnection elasticFeedback{ModuleType::ElasticHallway, ModuleType::Pillars,
                                      RoutingMode::Feedback};
    elasticFeedback.feedbackGain = 0.3f;
    fillPreset(RoutingPresetType::ElasticFeedbackDream, {
        {ModuleType::Foundation, ModuleType::Pillars},
        {ModuleType::Pillars, ModuleType::ElasticHallway},
        {ModuleType::ElasticHallway, ModuleType::Chambers},
        {ModuleType::Chambers, ModuleType::AlienAmplification},
        {ModuleType::AlienAmplification, ModuleType::Facade},
        elasticFeedback
    });

    // Foundation → Pillars → [Chambers + TubeRayTracer + ElasticHallway] parallel → Facade
    RoutingConnection parallelChambers{ModuleType::Pillars, ModuleType::Chambers,
                                       RoutingMode::Parallel};
    parallelChambers.blendAmount = 0.33f;
    RoutingConnection parallelTubes{ModuleType::Pillars, ModuleType::TubeRayTracer,
                                    RoutingMode::Parallel};
    parallelTubes.blendAmount = 0.33f;
    RoutingConnection parallelElastic{ModuleType::Pillars, ModuleType::ElasticHallway,
                                      RoutingMode::Parallel};
    parallelElastic.blendAmount = 0.34f;
    fillPreset(RoutingPresetType::ParallelWorlds, {
        {ModuleType::Foundation, ModuleType::Pillars},
        parallelChambers,
        parallelTubes,
        parallelElastic,
        {ModuleType::Chambers, ModuleType::Facade}
    });

    // Foundation → Pillars → Chambers → AlienAmplification → Facade
    RoutingConnection shimmerFeedback{ModuleType::AlienAmplification, ModuleType::Chambers,
                                      RoutingMode::Feedback};
    shimmerFeedback.feedbackGain = 0.4f;
    fillPreset(RoutingPresetType::ShimmerInfinity, {
        {ModuleType::Foundation, ModuleType::Pillars},
        {ModuleType::Pillars, ModuleType::Chambers},
        {ModuleType::Chambers, ModuleType::AlienAmplification},
        {ModuleType::AlienAmplification, ModuleType::Facade},
        shimmerFeedback
    });

    // Foundation → Pillars → AlienAmplification → TubeRayTracer → Chambers → Facade
    fillPreset(RoutingPresetType::ImpossibleChaos, {
        {ModuleType::Foundation, ModuleType::Pillars},
        {ModuleType::Pillars, ModuleType::AlienAmplification},
        {ModuleType::AlienAmplification, ModuleType::TubeRayTracer},
        {ModuleType::TubeRayTracer, ModuleType::Chambers},
        {ModuleType::Chambers, ModuleType::Facade}
    });

    // Foundation → Pillars → ElasticHallway → Weathering → Chambers → Facade
    fillPreset(RoutingPresetType::OrganicBreathing, {
        {ModuleType::Foundation, ModuleType::Pillars},
        {ModuleType::Pillars, ModuleType::ElasticHallway},
        {ModuleType::ElasticHallway, ModuleType::Weathering},
        {ModuleType::Weathering, ModuleType::Chambers},
        {ModuleType::Chambers, ModuleType::Facade}
    });

    // Foundation → Pillars → Facade (bypass reverb core)
    fillPreset(RoutingPresetType::MinimalSparse, {
        {ModuleType::Foundation, ModuleType::Pillars},
        {ModuleType::Pillars, ModuleType::Facade}
    }, {ModuleType::Chambers, ModuleType::Weathering});

    // Custom routing (empty by default)
    fillPreset(RoutingPresetType::Custom, {});
}

uint32_t DspRoutingGraph::computeBypassMask(
    const std::array<bool, static_cast<size_t>(ModuleType::Count)>& bypass) noexcept
{
    uint32_t mask = 0;
    for (size_t i = 0; i < bypass.size(); ++i)
    {
        if (bypass[i])
            mask |= (1u << static_cast<uint32_t>(i));
    }
    return mask;
}

void DspRoutingGraph::updateRoutingCache(size_t presetIndex) const
{
    if (presetIndex >= presetData.size())
        return;

    const auto& data = presetData[presetIndex];

    // Update vector for backward compatibility (not used in audio thread)
    routingConnections.assign(data.connections.begin(),
                              data.connections.begin() + data.connectionCount);
    routingCachePresetIndex = presetIndex;
}

const std::vector<RoutingConnection>& DspRoutingGraph::getRouting() const noexcept
{
    const size_t presetIndex = activePresetIndex.load(std::memory_order_acquire);
    if (presetIndex != routingCachePresetIndex)
        updateRoutingCache(presetIndex);

    return routingConnections;
}

void DspRoutingGraph::loadRoutingPreset(RoutingPresetType preset)
{
    // Lock-free preset switch: update the atomic index and bypass mask only.
    // The audio thread reads presetData[activePresetIndex] directly.
    const auto presetIndex = static_cast<size_t>(preset);
    if (presetIndex >= presetData.size())
        return;

    activePresetIndex.store(presetIndex, std::memory_order_release);
    bypassMask.store(presetData[presetIndex].bypassMask, std::memory_order_release);
}

bool DspRoutingGraph::setRouting(const std::vector<RoutingConnection>& connections)
{
    // Validate routing (check for cycles except intentional feedback)
    if (!isRoutingValid(connections))
        return false;

    routingConnections = connections;
    routingCachePresetIndex = static_cast<size_t>(RoutingPresetType::Custom);
    auto& data = presetData[static_cast<size_t>(RoutingPresetType::Custom)];
    data.connectionCount = 0;
    const uint32_t currentBypassMask = bypassMask.load(std::memory_order_acquire);
    for (size_t i = 0; i < data.bypass.size(); ++i)
        data.bypass[i] = (currentBypassMask & moduleBit(static_cast<ModuleType>(i))) != 0;
    data.bypassMask = currentBypassMask;

    for (const auto& connection : connections)
    {
        if (data.connectionCount >= kMaxRoutingConnections)
        {
            jassertfalse;
            break;
        }
        data.connections[data.connectionCount++] = connection;
    }

    activePresetIndex.store(static_cast<size_t>(RoutingPresetType::Custom),
                            std::memory_order_release);
    return true;
}

//==============================================================================
// Module Control
//==============================================================================

void DspRoutingGraph::setModuleBypass(ModuleType module, bool bypass)
{
    const uint32_t bit = moduleBit(module);
    if (bypass)
        bypassMask.fetch_or(bit, std::memory_order_release);
    else
        bypassMask.fetch_and(~bit, std::memory_order_release);
}

bool DspRoutingGraph::isModuleBypassed(ModuleType module) const noexcept
{
    return (bypassMask.load(std::memory_order_acquire) & moduleBit(module)) != 0;
}

//==============================================================================
// Module Parameter Forwarding
//==============================================================================

void DspRoutingGraph::setFoundationParams(float drive, [[maybe_unused]] float tilt)
{
    if (foundation)
        foundation->setInputGainDb(drive);
    // Note: 'tilt' parameter reserved for future tilt EQ implementation
}

void DspRoutingGraph::setPillarsParams(float density, const ParameterBuffer& shape, float warp)
{
    // Store per-sample shape buffer for use in process()
    pillarsShapeBuffer = shape;

    if (pillars)
    {
        pillars->setDensity(density);
        pillars->setShape(shape);  // Phase 4 Step 6: Pass ParameterBuffer directly
        pillars->setWarp(warp);
    }
}

void DspRoutingGraph::setChambersParams(const ParameterBuffer& time,
                                         const ParameterBuffer& mass,
                                         const ParameterBuffer& density,
                                         const ParameterBuffer& bloom,
                                         const ParameterBuffer& gravity,
                                         float warp,
                                         float drift,
                                         bool freeze,
                                         float adaptiveMatrixAmount,
                                         float feedbackSaturationAmount,
                                         float delayJitterAmount)
{
    // Store per-sample parameter buffers for use in process()
    chambersTimeBuffer = time;
    chambersMassBuffer = mass;
    chambersDensityBuffer = density;
    chambersBloomBuffer = bloom;
    chambersGravityBuffer = gravity;

#if defined(MONUMENT_TESTING) || defined(MONUMENT_MEMORY_PROVE)
    // DEBUG: Log first parameter value from each buffer (sample 0)
    static int logCounter = 0;
    if (++logCounter % 100 == 0)  // Log every 100th call to avoid flooding
    {
        const float timeVal = time.numSamples > 0 ? time[0] : 0.0f;
        const float massVal = mass.numSamples > 0 ? mass[0] : 0.0f;
        const float densityVal = density.numSamples > 0 ? density[0] : 0.0f;
        juce::Logger::writeToLog("Monument DEBUG: Chambers params time=" + juce::String(timeVal, 3) +
                                 " mass=" + juce::String(massVal, 3) +
                                 " density=" + juce::String(densityVal, 3) +
                                 " warp=" + juce::String(warp, 3) +
                                 " drift=" + juce::String(drift, 3) +
                                 " freeze=" + juce::String(freeze ? 1 : 0));
    }
#endif

    if (chambers)
    {
        // Phase 4 Step 5: Pass ParameterBuffer references directly (no averaging needed)
        // Chambers now consumes per-sample buffers directly for zipper-free automation
        chambers->setTime(time);
        chambers->setMass(mass);
        chambers->setDensity(density);
        chambers->setBloom(bloom);
        chambers->setGravity(gravity);

        // Block-rate parameters for Chambers reverb characteristics
        chambers->setWarp(warp);
        chambers->setDrift(drift);
        chambers->setFreeze(freeze);
        chambers->setAdaptiveMatrixAmount(adaptiveMatrixAmount);
        chambers->setFeedbackSaturation(feedbackSaturationAmount);
        chambers->setDelayJitter(delayJitterAmount);
    }
}

void DspRoutingGraph::setWeatheringParams(const ParameterBuffer& warp, const ParameterBuffer& drift)
{
    // Store per-sample parameter buffers for use in process()
    weatheringWarpBuffer = warp;
    weatheringDriftBuffer = drift;

    if (weathering)
    {
        // TEMPORARY: Average buffers for backward compatibility until Step 7 refactor
        auto averageBuffer = [](const ParameterBuffer& buf) -> float {
            float sum = 0.0f;
            for (int i = 0; i < buf.numSamples; ++i)
                sum += buf[i];
            return sum / static_cast<float>(buf.numSamples);
        };

        weathering->setWarp(averageBuffer(warp));
        weathering->setDrift(averageBuffer(drift));
    }
}

void DspRoutingGraph::setTubeRayTracerParams(float tubeCount, float radiusVariation,
                                               float metallicResonance, float couplingStrength)
{
    if (tubeRayTracer)
    {
        tubeRayTracer->setTubeCount(tubeCount);
        tubeRayTracer->setRadiusVariation(radiusVariation);
        tubeRayTracer->setMetallicResonance(metallicResonance);
        tubeRayTracer->setCouplingStrength(couplingStrength);
    }
}

void DspRoutingGraph::setElasticHallwayParams(float elasticity, float recoveryTime,
                                                 float absorptionDrift, float nonlinearity)
{
    if (elasticHallway)
    {
        elasticHallway->setElasticity(elasticity);
        elasticHallway->setRecoveryTime(recoveryTime);
        elasticHallway->setAbsorptionDrift(absorptionDrift);
        elasticHallway->setNonlinearity(nonlinearity);
    }
}

void DspRoutingGraph::setAlienAmplificationParams(float impossibilityDegree, float pitchEvolutionRate,
                                                    float paradoxFrequency, float paradoxGain)
{
    if (alienAmplification)
    {
        alienAmplification->setImpossibilityDegree(impossibilityDegree);
        alienAmplification->setPitchEvolutionRate(pitchEvolutionRate);
        alienAmplification->setParadoxResonanceFreq(paradoxFrequency);
        alienAmplification->setParadoxGain(paradoxGain);
    }
}

void DspRoutingGraph::setButtressParams(float drive, [[maybe_unused]] float feedbackLimit)
{
    if (buttress)
        buttress->setDrive(drive);
    // Note: 'feedbackLimit' parameter reserved for future limiter implementation
}

void DspRoutingGraph::setFacadeParams(float air, float width, float mix)
{
    if (facade)
    {
        facade->setAir(air);
        facade->setWidth(width);
        facade->setOutputGain(mix);
    }
}

//==============================================================================
// Helper Methods
//==============================================================================

void DspRoutingGraph::processModule(ModuleType module, juce::AudioBuffer<float>& buffer,
                                    uint32_t bypassMaskValue)
{
    // Skip bypassed modules
    if ((bypassMaskValue & moduleBit(module)) != 0)
        return;

    switch (module)
    {
        case ModuleType::Foundation:
            if (foundation) foundation->process(buffer);
            break;
        case ModuleType::Pillars:
            if (pillars) pillars->process(buffer);
            break;
        case ModuleType::Chambers:
            if (chambers) chambers->process(buffer);
            break;
        case ModuleType::Weathering:
            if (weathering) weathering->process(buffer);
            break;
        case ModuleType::TubeRayTracer:
            if (tubeRayTracer) tubeRayTracer->process(buffer);
            break;
        case ModuleType::ElasticHallway:
            if (elasticHallway) elasticHallway->process(buffer);
            break;
        case ModuleType::AlienAmplification:
            if (alienAmplification) alienAmplification->process(buffer);
            break;
        case ModuleType::Buttress:
            if (buttress) buttress->process(buffer);
            break;
        case ModuleType::Facade:
            if (facade) facade->process(buffer);
            break;
        case ModuleType::Count:
            break;  // Invalid
    }
}

void DspRoutingGraph::blendBuffers(juce::AudioBuffer<float>& destination,
                                     const juce::AudioBuffer<float>& source,
                                     float blendAmount)
{
    jassert(destination.getNumChannels() == source.getNumChannels());
    jassert(destination.getNumSamples() == source.getNumSamples());

    for (int ch = 0; ch < destination.getNumChannels(); ++ch)
    {
        destination.addFrom(ch, 0, source, ch, 0, destination.getNumSamples(), blendAmount);
    }
}

bool DspRoutingGraph::isRoutingValid([[maybe_unused]] const std::vector<RoutingConnection>& connections) const
{
    // TODO: Implement cycle detection (for now, trust the preset definitions)
    // Allow feedback modes as intentional cycles
    // Note: 'connections' parameter will be used when cycle detection is implemented
    return true;
}

std::vector<ModuleType> DspRoutingGraph::computeProcessingOrder() const
{
    // TODO: Implement topological sort for complex routing graphs
    // For now, routing connections are processed in the order they're defined
    std::vector<ModuleType> order;
    return order;
}

// ============================================================================
// Ancient Monuments Processing Modes
// ============================================================================

void DspRoutingGraph::processAncientWay(juce::AudioBuffer<float>& buffer)
{
    if (!isPrepared)
    {
        jassertfalse;
        buffer.clear();
        return;
    }

    const uint32_t bypassMaskValue = bypassMask.load(std::memory_order_acquire);

    // Traditional routing: Foundation → Pillars → Chambers → Weathering →
    //                      TubeRayTracer → ElasticHallway → AlienAmplification →
    //                      Buttress → Facade
    processModule(ModuleType::Foundation, buffer, bypassMaskValue);
    processModule(ModuleType::Pillars, buffer, bypassMaskValue);

#if defined(MONUMENT_TESTING) || defined(MONUMENT_MEMORY_PROVE)
    // DEBUG: Check signal before Chambers
    float preChambersRMS = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        preChambersRMS = juce::jmax(preChambersRMS, buffer.getRMSLevel(ch, 0, buffer.getNumSamples()));
#endif

    processModule(ModuleType::Chambers, buffer, bypassMaskValue);

#if defined(MONUMENT_TESTING) || defined(MONUMENT_MEMORY_PROVE)
    // DEBUG: Check signal after Chambers
    float postChambersRMS = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        postChambersRMS = juce::jmax(postChambersRMS, buffer.getRMSLevel(ch, 0, buffer.getNumSamples()));

    const bool chambersBypassed =
        (bypassMaskValue & moduleBit(ModuleType::Chambers)) != 0;
    juce::Logger::writeToLog("Monument DEBUG: Chambers bypassed=" + juce::String(chambersBypassed ? "YES" : "NO") +
                             " preRMS=" + juce::String(preChambersRMS, 6) +
                             " postRMS=" + juce::String(postChambersRMS, 6));
#endif

    processModule(ModuleType::Weathering, buffer, bypassMaskValue);
    processModule(ModuleType::TubeRayTracer, buffer, bypassMaskValue);
    processModule(ModuleType::ElasticHallway, buffer, bypassMaskValue);
    processModule(ModuleType::AlienAmplification, buffer, bypassMaskValue);
    processModule(ModuleType::Buttress, buffer, bypassMaskValue);
    processModule(ModuleType::Facade, buffer, bypassMaskValue);
}

void DspRoutingGraph::processResonantHalls(juce::AudioBuffer<float>& buffer)
{
    if (!isPrepared)
    {
        jassertfalse;
        buffer.clear();
        return;
    }

    const uint32_t bypassMaskValue = bypassMask.load(std::memory_order_acquire);

    // Metallic First routing: Foundation → Pillars → TubeRayTracer → Chambers →
    //                         Weathering → ElasticHallway → AlienAmplification →
    //                         Buttress → Facade
    //
    // Bright metallic tube resonances BEFORE reverb diffusion for focused character
    processModule(ModuleType::Foundation, buffer, bypassMaskValue);
    processModule(ModuleType::Pillars, buffer, bypassMaskValue);
    processModule(ModuleType::TubeRayTracer, buffer, bypassMaskValue);
    processModule(ModuleType::Chambers, buffer, bypassMaskValue);
    processModule(ModuleType::Weathering, buffer, bypassMaskValue);
    processModule(ModuleType::ElasticHallway, buffer, bypassMaskValue);
    processModule(ModuleType::AlienAmplification, buffer, bypassMaskValue);
    processModule(ModuleType::Buttress, buffer, bypassMaskValue);
    processModule(ModuleType::Facade, buffer, bypassMaskValue);
}

void DspRoutingGraph::processBreathingStone(juce::AudioBuffer<float>& buffer)
{
    if (!isPrepared)
    {
        jassertfalse;
        buffer.clear();
        return;
    }

    const uint32_t bypassMaskValue = bypassMask.load(std::memory_order_acquire);

    // Elastic Core routing: Foundation → Pillars → ElasticHallway → Chambers →
    //                       ElasticHallway → Weathering → TubeRayTracer →
    //                       AlienAmplification → Buttress → Facade
    //
    // Chambers sandwiched between elastic walls for organic breathing reverb
    processModule(ModuleType::Foundation, buffer, bypassMaskValue);
    processModule(ModuleType::Pillars, buffer, bypassMaskValue);

    // First elastic pass
    processModule(ModuleType::ElasticHallway, buffer, bypassMaskValue);

    // CRITICAL: Soft clip before Chambers to prevent energy accumulation
    // This prevents feedback runaway when ElasticHallway wraps around Chambers
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            data[i] = std::tanh(data[i] * 0.7f);  // Gentle saturation
    }

    processModule(ModuleType::Chambers, buffer, bypassMaskValue);

    // Second elastic pass (creates the "breathing" effect)
    processModule(ModuleType::ElasticHallway, buffer, bypassMaskValue);

    // CRITICAL: Hard limit before continuing (safety net)
    buffer.applyGain(0.95f);  // Headroom reduction

    processModule(ModuleType::Weathering, buffer, bypassMaskValue);
    processModule(ModuleType::TubeRayTracer, buffer, bypassMaskValue);
    processModule(ModuleType::AlienAmplification, buffer, bypassMaskValue);
    processModule(ModuleType::Buttress, buffer, bypassMaskValue);  // Final safety limiting
    processModule(ModuleType::Facade, buffer, bypassMaskValue);
}

} // namespace dsp
} // namespace monument
