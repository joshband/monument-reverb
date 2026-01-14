#pragma once

#include <JuceHeader.h>
#include <array>
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
 * @brief Routing modes for connecting modules
 */
enum class RoutingMode
{
    Series,           // A → B (signal flows through B after A)
    Parallel,         // A + B (both process dry signal, outputs blended)
    ParallelMix,      // Dry + (A + B) (both process, blend with dry)
    Feedback,         // B output → A input (with gain control)
    Crossfeed,        // L→R, R→L channel swap
    Bypass            // Skip this module entirely
};

/**
 * @brief Routing preset templates for instant sonic diversity
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
 * @brief A single connection between two modules
 */
struct RoutingConnection
{
    ModuleType source{ModuleType::Foundation};
    ModuleType destination{ModuleType::Facade};
    RoutingMode mode{RoutingMode::Series};

    float blendAmount{0.5f};        // For parallel modes (0.0-1.0)
    float feedbackGain{0.3f};       // For feedback mode (0.0-0.95)
    float crossfeedAmount{0.5f};    // For crossfeed mode (0.0-1.0)

    bool enabled{true};

    RoutingConnection() = default;
    RoutingConnection(ModuleType src, ModuleType dst, RoutingMode m = RoutingMode::Series)
        : source(src), destination(dst), mode(m) {}
};

/**
 * @brief Flexible DSP routing graph for dramatic sonic diversity
 *
 * This class replaces the fixed serial chain with a flexible routing system.
 * Modules can be connected in series, parallel, feedback loops, or bypassed entirely.
 *
 * Key Features:
 * - Series: Traditional signal flow (A → B)
 * - Parallel: Multiple paths blended (A + B)
 * - Feedback: Module output routed back to input (B → A)
 * - Crossfeed: L/R channel swapping
 * - Bypass: Skip modules for CPU savings
 *
 * Routing Presets:
 * - "Traditional Cathedral": Foundation → Pillars → Chambers → Weathering → Facade
 * - "Metallic Granular": Bypass Chambers, use TubeRayTracer + heavy diffusion
 * - "Elastic Feedback Dream": ElasticHallway with feedback to Pillars
 * - "Parallel Worlds": Chambers + Tubes + Elastic all in parallel
 *
 * Real-Time Safety:
 * - No allocations in process()
 * - Pre-allocated temp buffers for parallel processing
 * - Feedback loops use delay to prevent instant recursion
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
     * @brief Process audio block through the routing graph
     *
     * This function executes the current routing topology:
     * 1. Evaluate routing graph (topological sort)
     * 2. Process modules in dependency order
     * 3. Handle parallel blending and feedback
     *
     * @param buffer Audio buffer (modified in-place)
     */
    void process(juce::AudioBuffer<float>& buffer);

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
     * @brief Set custom routing connections
     *
     * Replaces current routing with provided connections.
     * Automatically validates for cycles (except intentional feedback).
     *
     * @param connections Vector of routing connections
     * @return bool True if routing is valid, false if cycles detected
     */
    bool setRouting(const std::vector<RoutingConnection>& connections);

    /**
     * @brief Load a preset routing template
     *
     * Instantly reconfigures the graph for dramatic sonic diversity.
     *
     * @param preset Routing preset type
     */
    void loadRoutingPreset(RoutingPresetType preset);

    /**
     * @brief Get current routing connections (for save/load)
     */
    const std::vector<RoutingConnection>& getRouting() const noexcept { return routingConnections; }

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
    RoutingPresetType getCurrentPreset() const noexcept { return currentPreset; }

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

    // Module bypass states
    std::array<bool, static_cast<size_t>(ModuleType::Count)> moduleBypassed{};

    // Routing preset cache (avoid allocations on preset swaps)
    static constexpr size_t kMaxRoutingConnections = 16;
    static constexpr size_t kRoutingPresetCount =
        static_cast<size_t>(RoutingPresetType::Custom) + 1;

    struct PresetRoutingData
    {
        std::array<RoutingConnection, kMaxRoutingConnections> connections{};
        size_t connectionCount{0};
        std::array<bool, static_cast<size_t>(ModuleType::Count)> bypass{};
    };

    std::array<PresetRoutingData, kRoutingPresetCount> presetData{};
    std::atomic<size_t> activePresetIndex{0};  // Lock-free preset switching

    // Current routing (kept for backward compatibility with setRouting/getRouting)
    std::vector<RoutingConnection> routingConnections;
    RoutingPresetType currentPreset{RoutingPresetType::TraditionalCathedral};

    // Temp buffers for parallel processing (pre-allocated in prepare())
    std::array<juce::AudioBuffer<float>, static_cast<size_t>(ModuleType::Count)> tempBuffers;
    std::array<juce::AudioBuffer<float>, static_cast<size_t>(ModuleType::Count)> moduleOutputBuffers;
    juce::AudioBuffer<float> feedbackBuffer;  // For feedback loops (1 block delay)
    juce::AudioBuffer<float> dryBuffer;       // Dry signal storage for parallel modes

    // Feedback safety: smoothed gains and low-pass filtering
    juce::SmoothedValue<float> feedbackGainSmoothed;
    juce::dsp::IIR::Filter<float> feedbackLowpassL, feedbackLowpassR;
    static constexpr float kMaxFeedbackGain = 0.95f;  // Safety limit

    double sampleRateHz{48000.0};
    int maxBlockSizeInternal{2048};
    int numChannelsInternal{2};
    bool isPrepared{false};

    // Parameter buffer storage (references to PluginProcessor's parameter pools)
    // These are set via setXXXParams() and used during process()
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
    void processModule(ModuleType module, juce::AudioBuffer<float>& buffer);

    // Helper: Blend two buffers for parallel modes
    void blendBuffers(juce::AudioBuffer<float>& destination,
                      const juce::AudioBuffer<float>& source,
                      float blendAmount);

    // Helper: Validate routing (detect cycles)
    bool isRoutingValid(const std::vector<RoutingConnection>& connections) const;

    // Helper: Topological sort for processing order
    std::vector<ModuleType> computeProcessingOrder() const;

    // Helper: Build/apply preset routing data
    void buildPresetData();
    void applyPresetData(RoutingPresetType preset);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DspRoutingGraph)
};

} // namespace dsp
} // namespace monument
