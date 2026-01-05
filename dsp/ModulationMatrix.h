#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>
#include <memory>
#include <random>

namespace monument
{
namespace dsp
{

/**
 * @brief ModulationMatrix routes modulation sources to parameter destinations.
 *
 * The modulation system allows multiple sources (chaos, audio follower, Brownian motion, etc.)
 * to modulate any parameter with configurable depth and smoothing. This creates "alive" parameter
 * evolution that responds to input dynamics and exhibits controlled unpredictability.
 *
 * All processing happens at block-rate (not sample-rate) for efficiency, as modulation sources
 * are inherently slow/smooth and don't need per-sample updates.
 */
class ModulationMatrix final
{
public:
    /**
     * @brief Modulation source types.
     */
    enum class SourceType
    {
        ChaosAttractor = 0,  // Deterministic chaos (Lorenz/Rössler attractors)
        AudioFollower,       // Input signal envelope tracking
        BrownianMotion,      // Smooth random walk (1/f noise)
        EnvelopeTracker,     // Multi-stage envelope detection
        Count                // Total number of sources
    };

    /**
     * @brief Parameter destinations (maps to APVTS parameters).
     */
    enum class DestinationType
    {
        Time = 0,
        Mass,
        Density,
        Bloom,
        Air,
        Width,
        Mix,
        Warp,
        Drift,
        Gravity,
        PillarShape,
        // Physical modeling parameters (Phase 5)
        TubeCount,
        RadiusVariation,
        MetallicResonance,
        CouplingStrength,
        Elasticity,
        RecoveryTime,
        AbsorptionDrift,
        Nonlinearity,
        ImpossibilityDegree,
        PitchEvolutionRate,
        ParadoxResonanceFreq,
        ParadoxGain,
        // Spatial positioning (Phase 1: Three-System Plan)
        PositionX,           // Spatial X position for delay line 0 [-1, +1]
        PositionY,           // Spatial Y position for delay line 0 [-1, +1]
        PositionZ,           // Spatial Z position for delay line 0 [0, +1]
        Count  // Total number of destinations
    };

    /**
     * @brief A single modulation connection.
     */
    struct Connection
    {
        SourceType source{SourceType::ChaosAttractor};
        DestinationType destination{DestinationType::Warp};
        int sourceAxis{0};              // Some sources have multiple outputs (e.g., chaos X/Y/Z)
        float depth{0.0f};             // Modulation amount: -1 to +1 (bipolar)
        float smoothingMs{200.0f};     // Lag filter time constant (20-1000ms)
        float probability{1.0f};       // Probability gate: 0.0 = never, 1.0 = always (intermittent modulation)
        bool enabled{false};           // Connection active/inactive

        Connection() = default;
        Connection(SourceType src, DestinationType dst, float d)
            : source(src), destination(dst), depth(d), enabled(true) {}
    };

    ModulationMatrix();
    ~ModulationMatrix();

    /**
     * @brief Prepare for processing at given sample rate and block size.
     *
     * This allocates all internal buffers and initializes modulation sources.
     * Must be called before first process() call and whenever sample rate changes.
     */
    void prepare(double sampleRate, int maxBlockSize, int numChannels);

    /**
     * @brief Reset all modulation state (clear chaos, envelope, Brownian history).
     */
    void reset();

    /**
     * @brief Process one audio block, updating all modulation sources.
     *
     * This function:
     * 1. Updates all modulation sources (chaos iteration, envelope tracking, etc.)
     * 2. Computes modulation values for each destination
     * 3. Applies smoothing to prevent abrupt parameter jumps
     *
     * @param audioBuffer Input audio (for AudioFollower/EnvelopeTracker sources)
     * @param numSamples Number of samples in this block
     */
    void process(const juce::AudioBuffer<float>& audioBuffer, int numSamples);

    /**
     * @brief Get current modulation value for a specific destination.
     *
     * Returns the smoothed, accumulated modulation from all active connections
     * targeting this destination. Values are bipolar: [-1, +1].
     *
     * @param destination Target parameter
     * @return float Modulation amount (bipolar, smoothed)
     */
    float getModulation(DestinationType destination) const noexcept;

    /**
     * @brief Add or update a modulation connection.
     *
     * If a connection from this source to this destination already exists, it's updated.
     * Otherwise, a new connection is created.
     *
     * @param probability Probability gate (0.0-1.0). 1.0 = always active, 0.5 = active 50% of time
     */
    void setConnection(SourceType source, DestinationType destination,
                      int sourceAxis, float depth, float smoothingMs, float probability = 1.0f);

    /**
     * @brief Remove a modulation connection.
     */
    void removeConnection(SourceType source, DestinationType destination, int sourceAxis);

    /**
     * @brief Remove all connections.
     */
    void clearConnections();

    /**
     * @brief Get all active connections (for preset save/load and UI display).
     *
     * Returns a copy of the connections vector for preset serialization and UI.
     * This is not called during real-time audio processing.
     */
    std::vector<Connection> getConnections() const noexcept;

    /**
     * @brief Set all connections at once (for preset loading).
     */
    void setConnections(const std::vector<Connection>& newConnections);

    /**
     * @brief Randomize all modulation connections for instant sound design exploration.
     *
     * Creates 4-8 random connections with musical constraints:
     * - Depth limited to ±60% (not ±100%) for safety
     * - Smoothing always ≥100ms to prevent zipper noise
     * - Skips duplicate source/destination pairs
     *
     * This provides "happy accidents" and instant sonic exploration without overwhelming
     * the user or creating unstable/extreme parameter values.
     */
    void randomizeAll();

    /**
     * @brief Randomize with sparse connections (subtle modulation).
     *
     * Creates 2-3 random connections with conservative depth (±20-40%).
     * Ideal for subtle, organic parameter evolution.
     */
    void randomizeSparse();

    /**
     * @brief Randomize with dense connections (extreme modulation).
     *
     * Creates 8-12 random connections with higher depth (±40-80%).
     * Ideal for chaotic, evolving soundscapes.
     */
    void randomizeDense();

    /**
     * @brief Get raw output from a specific modulation source (for UI visualization).
     */
    float getSourceValue(SourceType source, int axis = 0) const noexcept;

private:
    // Forward declarations for modulation sources (implemented in Phase 2)
    class ChaosAttractor;
    class AudioFollower;
    class BrownianMotion;
    class EnvelopeTracker;

    double sampleRateHz{48000.0};
    int maxBlockSizeInternal{2048};
    int numChannelsInternal{2};

    // Modulation sources (allocated in prepare(), processed in process())
    std::unique_ptr<ChaosAttractor> chaosGen;
    std::unique_ptr<AudioFollower> audioFollower;
    std::unique_ptr<BrownianMotion> brownianGen;
    std::unique_ptr<EnvelopeTracker> envTracker;

    // Active modulation connections (fixed-size array to prevent real-time allocations)
    static constexpr int kMaxConnections = 256;
    std::array<Connection, kMaxConnections> connections{};
    int connectionCount = 0;
    mutable juce::SpinLock connectionsLock;  // Thread-safe access to connections array

    // Per-destination modulation accumulators (smoothed output values)
    std::array<float, static_cast<size_t>(DestinationType::Count)> modulationValues{};

    // Smoothing filters (one per destination)
    std::array<juce::SmoothedValue<float>, static_cast<size_t>(DestinationType::Count)> smoothers;

    // Random number generator for probability gating (mutable for const process())
    mutable std::mt19937 probabilityRng;
    mutable std::uniform_real_distribution<float> probabilityDist{0.0f, 1.0f};

    // Helper: find existing connection index, or -1 if not found
    int findConnectionIndex(SourceType source, DestinationType destination, int axis) const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationMatrix)
};

} // namespace dsp
} // namespace monument
