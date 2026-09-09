#pragma once

#include <JuceHeader.h>
#include <array>
#include <cstdint>
#include <vector>
#include <memory>
#include <unordered_map>
#include "dsp/ParameterBuffers.h"

namespace monument
{
namespace dsp
{

// Forward declarations of all DSP modules
class Foundation;
class Pillars;
class Chambers;
class Weathering;
class TubeRayTracer;
class ElasticHallway;
class AlienAmplification;
class Buttress;
class Facade;
class MemoryEchoes;

/**
 * @brief DSP module types available for routing
 */
enum class ModuleType
{
    Foundation = 0,
    Pillars,
    Chambers,
    Weathering,
    TubeRayTracer,
    ElasticHallway,
    AlienAmplification,
    Buttress,
    Facade,
    Count
};

/**
 * @brief Named routing-preset identities and their per-module bypass mask.
 *
 * These no longer select a distinct signal topology (the generic
 * series/parallel/feedback/crossfeed graph executor that once interpreted
 * them was dead code — nothing in the plugin ever called it — and has been
 * removed). Only the bypass mask each preset carries is live: it's what the
 * fixed chains (processAncientWay/processResonantHalls/processBreathingStone)
 * honor via processModule(). Preset names describe historical intent, not
 * rendered topology.
 */
enum class RoutingPresetType
{
    TraditionalCathedral,      // Foundation → Pillars → Chambers → Weathering → Facade
    MetallicGranular,          // Foundation → Pillars → TubeRayTracer → Granular → Facade (bypass Chambers)
    ElasticFeedbackDream,      // Foundation → ElasticHallway ⟲ (Feedback) → Chambers → Alien → Facade
    ParallelWorlds,            // Foundation → [Chambers + Tubes + Elastic] parallel → Facade
    ShimmerInfinity,           // Foundation → Chambers → PitchShift ⟲ Feedback → Facade
    ImpossibleChaos,           // Foundation → Alien → Tubes → Chambers → Facade
    OrganicBreathing,          // Foundation → Elastic → Weathering → Chambers → Facade
    MinimalSparse,             // Foundation → Pillars → Facade (bypass reverb core)
    Custom                     // User-defined routing
};

/**
 * @brief Dispatches audio through Monument's fixed DSP signal chains.
 *
 * Three fixed-chain implementations exist (processAncientWay,
 * processResonantHalls, processBreathingStone); PluginProcessor selects
 * one via ProcessingMode. Each preset in RoutingPresetType carries a
 * per-module bypass mask that all three chains honor identically via
 * setModuleBypass()/isModuleBypassed().
 */
class DspRoutingGraph final
{
public:
    DspRoutingGraph();
    ~DspRoutingGraph();

    /**
     * @brief Prepare all modules and routing buffers
     */
    void prepare(double sampleRate, int maxBlockSize, int numChannels);

    /**
     * @brief Reset all module states
     */
    void reset();

    /**
     * @brief Process buffer using Ancient Way routing (Traditional)
     *
     * Signal flow: Foundation → Pillars → Chambers → Weathering →
     *              TubeRayTracer → ElasticHallway → AlienAmplification →
     *              Buttress → Facade
     */
    void processAncientWay(juce::AudioBuffer<float>& buffer);

    /**
     * @brief Process buffer using Resonant Halls routing (Metallic First)
     *
     * Signal flow: Foundation → Pillars → TubeRayTracer → Chambers →
     *              Weathering → ElasticHallway → AlienAmplification →
     *              Buttress → Facade
     *
     * Bright metallic resonances before reverb diffusion.
     */
    void processResonantHalls(juce::AudioBuffer<float>& buffer);

    /**
     * @brief Process buffer using Breathing Stone routing (Elastic Core)
     *
     * Signal flow: Foundation → Pillars → ElasticHallway → Chambers →
     *              ElasticHallway → Weathering → TubeRayTracer →
     *              AlienAmplification → Buttress → Facade
     *
     * Organic breathing walls embrace the reverb core.
     * Includes safety clipping between stages to prevent feedback runaway.
     */
    void processBreathingStone(juce::AudioBuffer<float>& buffer);

    /**
     * @brief Load a preset routing template
     *
     * Instantly reconfigures the graph for dramatic sonic diversity.
     *
     * @param preset Routing preset type
     */
    void loadRoutingPreset(RoutingPresetType preset);

    /**
     * @brief Get active preset index (lock-free, audio-thread safe)
     */
    size_t getActivePresetIndex() const noexcept { return activePresetIndex.load(std::memory_order_acquire); }

    /**
     * @brief Set individual module bypass state
     */
    void setModuleBypass(ModuleType module, bool bypass);

    /**
     * @brief Check if a module is currently bypassed
     */
    bool isModuleBypassed(ModuleType module) const noexcept;

    /**
     * @brief Get current routing preset type
     */
    RoutingPresetType getCurrentPreset() const noexcept
    {
        return static_cast<RoutingPresetType>(getActivePresetIndex());
    }

    /**
     * @brief Set module parameters (forwarded to individual modules)
     *
     * DESIGN NOTE: Critical parameters now accept ParameterBuffer references for
     * per-sample interpolation, eliminating zipper noise and parameter smoothing artifacts.
     *
     * Per-sample parameters (ParameterBuffer):
     * - time, mass, density, bloom, gravity (Chambers FDN - most audible)
     * - pillarShape (Pillars tap layout - modulates early reflections)
     * - warp, drift (Weathering modulation depth)
     *
     * Block-rate parameters (float):
     * - air, width, mix (Facade - less critical, lower CPU overhead)
     * - drive, tilt (Foundation/Buttress - input/output stages)
     */
    void setFoundationParams(float drive, float tilt);
    void setPillarsParams(float density, const ParameterBuffer& shape, float warp);
    void setChambersParams(const ParameterBuffer& time,
                           const ParameterBuffer& mass,
                           const ParameterBuffer& density,
                           const ParameterBuffer& bloom,
                           const ParameterBuffer& gravity,
                           float warp,
                           float drift,
                           bool freeze,
                           float adaptiveMatrixAmount,
                           float feedbackSaturationAmount,
                           float delayJitterAmount);
    void setWeatheringParams(const ParameterBuffer& warp, const ParameterBuffer& drift);
    void setTubeRayTracerParams(float tubeCount, float radiusVariation,
                                 float metallicResonance, float couplingStrength);
    void setElasticHallwayParams(float elasticity, float recoveryTime,
                                   float absorptionDrift, float nonlinearity);
    void setAlienAmplificationParams(float impossibilityDegree, float pitchEvolutionRate,
                                      float paradoxFrequency, float paradoxGain);
    void setButtressParams(float drive, float feedbackLimit);
    void setFacadeParams(float air, float width, float mix);

    /**
     * @brief Get direct access to Chambers module for spatial processor control
     */
    Chambers* getChambers() noexcept { return chambers.get(); }

    /**
     * @brief Wire in the MemoryEchoes instance owned by PluginProcessor.
     *
     * processAncientWay() calls memoryEchoes->process() immediately before
     * Chambers (injecting recalled material into the buffer Chambers will
     * reverberate) and memoryEchoes->captureWet() immediately after
     * (capturing Chambers' wet output for future recall). Non-owning:
     * DspRoutingGraph does not prepare/reset/own this pointer's lifetime.
     */
    void setMemoryEchoes(MemoryEchoes* memory) noexcept { memoryEchoes = memory; }

private:
    // Module instances (allocated once in constructor)
    std::unique_ptr<Foundation> foundation;
    std::unique_ptr<Pillars> pillars;
    std::unique_ptr<Chambers> chambers;
    std::unique_ptr<Weathering> weathering;
    std::unique_ptr<TubeRayTracer> tubeRayTracer;
    std::unique_ptr<ElasticHallway> elasticHallway;
    std::unique_ptr<AlienAmplification> alienAmplification;
    std::unique_ptr<Buttress> buttress;
    std::unique_ptr<Facade> facade;

    // Non-owning: set via setMemoryEchoes(), owned/prepared/reset by PluginProcessor.
    MemoryEchoes* memoryEchoes = nullptr;

    // Module bypass states (lock-free)
    std::atomic<uint32_t> bypassMask{0};

    static constexpr size_t kRoutingPresetCount =
        static_cast<size_t>(RoutingPresetType::Custom) + 1;

    struct PresetRoutingData
    {
        std::array<bool, static_cast<size_t>(ModuleType::Count)> bypass{};
        uint32_t bypassMask{0};
    };

    std::array<PresetRoutingData, kRoutingPresetCount> presetData{};
    std::atomic<size_t> activePresetIndex{0};  // Lock-free preset switching

    double sampleRateHz{48000.0};
    int maxBlockSizeInternal{2048};
    int numChannelsInternal{2};
    bool isPrepared{false};

    // Parameter buffer storage (references to PluginProcessor's parameter pools)
    // These are set via setXXXParams()
    // Chambers critical parameters (per-sample)
    ParameterBuffer chambersTimeBuffer;
    ParameterBuffer chambersMassBuffer;
    ParameterBuffer chambersDensityBuffer;
    ParameterBuffer chambersBloomBuffer;
    ParameterBuffer chambersGravityBuffer;

    // Pillars critical parameters (per-sample)
    ParameterBuffer pillarsShapeBuffer;

    // Weathering critical parameters (per-sample)
    ParameterBuffer weatheringWarpBuffer;
    ParameterBuffer weatheringDriftBuffer;

    // Helper: Get module processor by type
    void processModule(ModuleType module, juce::AudioBuffer<float>& buffer, uint32_t bypassMask);

    // Helper: Build each preset's bypass mask
    void buildPresetData();

    static constexpr uint32_t moduleBit(ModuleType module) noexcept
    {
        return 1u << static_cast<uint32_t>(module);
    }

    static uint32_t computeBypassMask(
        const std::array<bool, static_cast<size_t>(ModuleType::Count)>& bypass) noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DspRoutingGraph)
};

} // namespace dsp
} // namespace monument
