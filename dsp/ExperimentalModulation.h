#pragma once

#include <JuceHeader.h>
#include <random>
#include <vector>
#include <memory>

namespace monument
{
namespace dsp
{

/**
 * @brief Probability gate for intermittent modulation
 *
 * Modulation only applies a certain percentage of the time, creating
 * unpredictable, organic behavior.
 *
 * Example: ChaosAttractor → Warp (Probability: 30%)
 * Result: Space warps intermittently, not continuously
 */
class ProbabilityGate final
{
public:
    ProbabilityGate();

    /**
     * @brief Set probability of modulation passing through (0.0-1.0)
     * 0.0 = never, 1.0 = always, 0.5 = 50% of blocks
     */
    void setProbability(float prob) noexcept;

    /**
     * @brief Set smoothing time for fade in/out (ms)
     */
    void setSmoothingMs(float ms, double sampleRate) noexcept;

    /**
     * @brief Prepare for processing
     */
    void prepare(double sampleRate);

    /**
     * @brief Process modulation value (per-block, not per-sample)
     *
     * @param inputModulation Raw modulation value
     * @return float Gated modulation value (smoothed)
     */
    float process(float inputModulation);

private:
    float probability{1.0f};
    bool currentlyActive{false};
    juce::SmoothedValue<float> gateEnvelope;
    std::mt19937 rng;
    std::uniform_real_distribution<float> dist{0.0f, 1.0f};

    bool shouldBeActive();
};

/**
 * @brief Quantized modulation for stepped, rhythmic effects
 *
 * Instead of smooth modulation, values snap to discrete steps.
 *
 * Example: AudioFollower → Time (Quantized: 8 steps)
 * Result: Time jumps between 8 discrete values (rhythmic gating)
 */
class ModulationQuantizer final
{
public:
    /**
     * @brief Set number of quantization steps (2-64)
     */
    void setSteps(int numSteps) noexcept;

    /**
     * @brief Quantize a modulation value
     *
     * @param smoothValue Input value (typically [0, 1])
     * @return float Quantized value (stepped)
     */
    float quantize(float smoothValue) const noexcept;

private:
    int steps{8};
};

/**
 * @brief Cross-modulation: one source modulates another source's parameters
 *
 * Example: AudioFollower → ChaosAttractor.Rate (Depth: 0.8)
 * Result: Chaos speed increases with input volume (dynamic chaos)
 */
struct CrossModConnection
{
    enum class TargetParameter
    {
        Rate,         // Modulation source update rate (e.g., chaos iteration speed)
        Depth,        // Modulation depth multiplier
        Offset        // DC offset added to source output
    };

    int modulatorSourceIndex{0};  // Which source modulates (e.g., AudioFollower)
    int targetSourceIndex{1};     // Which source is modulated (e.g., ChaosAttractor)
    TargetParameter parameter{TargetParameter::Rate};
    float depth{0.5f};            // Modulation amount
    bool enabled{true};
};

/**
 * @brief Preset morphing: blend between 2-4 presets in a 2D space
 *
 * Allows smooth interpolation between different sonic worlds.
 * Can be modulated by LFOs, chaos, or manual control.
 */
class PresetMorpher final
{
public:
    PresetMorpher();

    /**
     * @brief Set the 4 corner presets for 2D morphing
     *
     * Layout:
     *   [0: Top-Left]    [1: Top-Right]
     *   [2: Bottom-Left] [3: Bottom-Right]
     *
     * @param presetIndices Indices into preset bank (0-127)
     */
    void setCornerPresets(int topLeft, int topRight, int bottomLeft, int bottomRight);

    /**
     * @brief Set morph position in 2D space
     *
     * @param x Horizontal position (0.0 = left, 1.0 = right)
     * @param y Vertical position (0.0 = top, 1.0 = bottom)
     */
    void setMorphPosition(float x, float y) noexcept;

    /**
     * @brief Get current morph position
     */
    juce::Point<float> getMorphPosition() const noexcept { return {morphX, morphY}; }

    /**
     * @brief Compute morphed parameter values (bilinear interpolation)
     *
     * @return float Interpolated value for a specific parameter
     */
    float getMorphedParameter(int parameterIndex) const noexcept;

    /**
     * @brief Load parameter states for all 4 presets
     */
    void loadPresetStates(const std::vector<std::vector<float>>& presetParams);

private:
    std::array<int, 4> cornerPresets{0, 1, 2, 3};
    std::array<std::vector<float>, 4> presetParameters;

    float morphX{0.5f};
    float morphY{0.5f};

    // Bilinear interpolation helper
    float bilinearInterpolate(float topLeft, float topRight,
                               float bottomLeft, float bottomRight,
                               float x, float y) const noexcept;
};

/**
 * @brief Gesture recorder: record parameter movements as custom modulation sources
 *
 * Captures manual knob movements and plays them back as LFOs.
 */
class GestureRecorder final
{
public:
    GestureRecorder();

    /**
     * @brief Start recording parameter changes
     */
    void startRecording();

    /**
     * @brief Stop recording and prepare for playback
     */
    void stopRecording();

    /**
     * @brief Record a parameter value (called per block during recording)
     */
    void recordValue(float value);

    /**
     * @brief Start playback of recorded gesture
     *
     * @param speed Playback speed multiplier (1.0 = original speed)
     * @param loop Whether to loop the gesture
     */
    void startPlayback(float speed = 1.0f, bool loop = true);

    /**
     * @brief Stop playback
     */
    void stopPlayback();

    /**
     * @brief Get current playback sample (per-block)
     */
    float getSample();

    /**
     * @brief Check if currently recording
     */
    bool isRecording() const noexcept { return recording; }

    /**
     * @brief Check if currently playing back
     */
    bool isPlaying() const noexcept { return playing; }

    /**
     * @brief Get recorded gesture length in samples
     */
    int getLength() const noexcept { return static_cast<int>(recordedValues.size()); }

private:
    std::vector<float> recordedValues;
    int playbackIndex{0};
    float playbackSpeed{1.0f};
    bool recording{false};
    bool playing{false};
    bool looping{true};
};

/**
 * @brief Physics-based modulator: spring-mass-damper system
 *
 * Creates organic, physical response to input dynamics.
 * Can be driven by audio follower or other sources.
 */
class SpringMassModulator final
{
public:
    SpringMassModulator();

    /**
     * @brief Set spring constant (stiffness)
     * Higher = faster oscillation, tighter response
     */
    void setSpringConstant(float k) noexcept;

    /**
     * @brief Set mass (inertia)
     * Higher = slower response, more momentum
     */
    void setMass(float m) noexcept;

    /**
     * @brief Set damping coefficient
     * Higher = faster settling, less oscillation
     */
    void setDamping(float c) noexcept;

    /**
     * @brief Apply external force (driven by audio or other modulation)
     */
    void applyForce(float force) noexcept;

    /**
     * @brief Prepare for processing at given sample rate
     */
    void prepare(double sampleRate);

    /**
     * @brief Process one sample and return spring position
     *
     * Position is the modulation output value.
     */
    float processSample();

    /**
     * @brief Reset spring to rest position
     */
    void reset() noexcept;

private:
    float position{0.0f};
    float velocity{0.0f};
    float externalForce{0.0f};

    float springConstant{1.0f};
    float mass{1.0f};
    float damping{0.1f};

    double dt{1.0 / 48000.0};  // Time step (1 / sample rate)

    // Differential equation solver (semi-implicit Euler)
    void updatePhysics();
};

/**
 * @brief Chaos seed generator: one-click randomization of all modulation
 *
 * Creates instant random modulation routings for exploration.
 */
class ChaosSeeder final
{
public:
    /**
     * @brief Generate random modulation connections
     *
     * @param numConnections Number of random connections to create (4-12)
     * @param numSources Total number of modulation sources available
     * @param numDestinations Total number of parameter destinations
     * @return std::vector<std::tuple<int, int, float>> Source, Dest, Depth tuples
     */
    static std::vector<std::tuple<int, int, float>> generateRandomConnections(
        int numConnections,
        int numSources,
        int numDestinations);

    /**
     * @brief Generate random probability values for each connection
     */
    static std::vector<float> generateRandomProbabilities(int numConnections);

    /**
     * @brief Generate random quantization step counts
     */
    static std::vector<int> generateRandomQuantization(int numConnections);

private:
    static std::mt19937& getRng();
};

/**
 * @brief Enhanced ModulationMatrix connection with experimental features
 *
 * Extends basic Source→Destination routing with:
 * - Probability gates (intermittent modulation)
 * - Quantization (stepped values)
 * - Cross-modulation support
 */
struct ExperimentalModConnection
{
    int sourceIndex{0};
    int destinationIndex{0};
    int sourceAxis{0};
    float depth{0.5f};
    float smoothingMs{200.0f};

    // Experimental features
    bool probabilityEnabled{false};
    float probability{1.0f};

    bool quantizationEnabled{false};
    int quantizationSteps{8};

    bool enabled{true};

    // Runtime objects (allocated on heap to avoid copying issues)
    std::unique_ptr<ProbabilityGate> probabilityGate;
    std::unique_ptr<ModulationQuantizer> quantizer;
};

} // namespace dsp
} // namespace monument
