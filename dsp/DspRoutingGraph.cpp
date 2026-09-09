#include "DspRoutingGraph.h"
#include "dsp/DspModules.h"
#include "dsp/TubeRayTracer.h"
#include "dsp/ElasticHallway.h"
#include "dsp/AlienAmplification.h"
#include "dsp/MemoryEchoes.h"
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
}

//==============================================================================
// Routing Configuration
//==============================================================================

void DspRoutingGraph::buildPresetData()
{
    // Each preset's only live effect is which modules it bypasses; the fixed
    // chains (processAncientWay/processResonantHalls/processBreathingStone)
    // are what actually render the signal, identically regardless of preset.
    auto fillPreset = [this](
        RoutingPresetType preset,
        std::initializer_list<ModuleType> bypassed = {})
    {
        auto& data = presetData[static_cast<size_t>(preset)];
        data.bypass.fill(false);

        for (const auto module : bypassed)
            data.bypass[static_cast<size_t>(module)] = true;

        data.bypassMask = computeBypassMask(data.bypass);
    };

    fillPreset(RoutingPresetType::TraditionalCathedral);
    fillPreset(RoutingPresetType::MetallicGranular, {ModuleType::Chambers});
    fillPreset(RoutingPresetType::ElasticFeedbackDream);
    fillPreset(RoutingPresetType::ParallelWorlds);
    fillPreset(RoutingPresetType::ShimmerInfinity);
    fillPreset(RoutingPresetType::ImpossibleChaos);
    fillPreset(RoutingPresetType::OrganicBreathing);
    fillPreset(RoutingPresetType::MinimalSparse, {ModuleType::Chambers, ModuleType::Weathering});
    fillPreset(RoutingPresetType::Custom);
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
    // Store per-sample shape buffer (write-only cache; not currently read back)
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
    // Store per-sample parameter buffers (write-only cache; not currently read back)
    chambersTimeBuffer = time;
    chambersMassBuffer = mass;
    chambersDensityBuffer = density;
    chambersBloomBuffer = bloom;
    chambersGravityBuffer = gravity;

#if defined(MONUMENT_TESTING_VERBOSE_LOG)
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
    // Store per-sample parameter buffers (write-only cache; not currently read back)
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

#if defined(MONUMENT_TESTING_VERBOSE_LOG)
    // DEBUG: Check signal before Chambers
    float preChambersRMS = 0.0f;
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        preChambersRMS = juce::jmax(preChambersRMS, buffer.getRMSLevel(ch, 0, buffer.getNumSamples()));
#endif

    // MemoryEchoes surfaces recalled material into the buffer Chambers is
    // about to reverberate (see setMemoryEchoes()'s docs). Runs unconditionally
    // (independent of Chambers's own bypass bit) so recall stays audible even
    // when a routing preset bypasses Chambers itself.
    if (memoryEchoes != nullptr)
        memoryEchoes->process(buffer);

    processModule(ModuleType::Chambers, buffer, bypassMaskValue);

    // Capture Chambers' wet output as future recall material.
    if (memoryEchoes != nullptr)
        memoryEchoes->captureWet(buffer);

#if defined(MONUMENT_TESTING_VERBOSE_LOG)
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
