#pragma once

#include <JuceHeader.h>
#include <array>
#include <vector>
#include <memory>

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
        // Future physical modeling parameters
        TubeCount,
        MetallicResonance,
        Elasticity,
        ImpossibilityDegree,
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
     */
    void setConnection(SourceType source, DestinationType destination,
                      int sourceAxis, float depth, float smoothingMs);

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
     */
    const std::vector<Connection>& getConnections() const noexcept { return connections; }

    /**
     * @brief Set all connections at once (for preset loading).
     */
    void setConnections(const std::vector<Connection>& newConnections);

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

    // Active modulation connections
    std::vector<Connection> connections;
    mutable juce::SpinLock connectionsLock;  // Thread-safe access to connections vector

    // Per-destination modulation accumulators (smoothed output values)
    std::array<float, static_cast<size_t>(DestinationType::Count)> modulationValues{};

    // Smoothing filters (one per destination)
    std::array<juce::SmoothedValue<float>, static_cast<size_t>(DestinationType::Count)> smoothers;

    // Helper: find existing connection index, or -1 if not found
    int findConnectionIndex(SourceType source, DestinationType destination, int axis) const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationMatrix)
};

} // namespace dsp
} // namespace monument
