#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_core/juce_core.h>
#include <array>
#include <vector>
#include <optional>

namespace monument
{
namespace dsp
{

/**
 * @brief SequenceScheduler provides timeline-based parameter automation and preset morphing.
 *
 * This system allows parameters to evolve over time according to a predefined sequence of keyframes.
 * Keyframes can be placed on a timeline (in beats or seconds) and the scheduler smoothly
 * interpolates between them, creating evolving soundscapes and automated preset transitions.
 *
 * Features:
 * - Keyframe-based timeline with arbitrary parameter targets
 * - Tempo-synchronized playback (beats/bars) or free-running (seconds)
 * - Smooth interpolation between keyframes (linear, exponential, S-curve)
 * - Loop modes: one-shot, loop, ping-pong
 * - Real-time safe (pre-allocated storage, no locks in process())
 *
 * Use cases:
 * - "Evolving Cathedral": Reverb morphs from small to massive over 16 bars
 * - "Spatial Journey": Sound source moves through 3D space in sync with tempo
 * - "Living Space": Room characteristics drift subtly over time
 */
class SequenceScheduler final
{
public:
    /**
     * @brief Interpolation curve types for keyframe transitions.
     */
    enum class InterpolationType
    {
        Linear = 0,      // Constant velocity between keyframes
        Exponential,     // Accelerating curve (ease-in)
        SCurve,          // Ease-in-out (smooth start and end)
        Step,            // Instant jump (no interpolation)
        Count
    };

    /**
     * @brief Timeline playback modes.
     */
    enum class PlaybackMode
    {
        OneShot = 0,     // Play once and stop at last keyframe
        Loop,            // Loop from start to end continuously
        PingPong,        // Play forward, then backward, then forward...
        Count
    };

    /**
     * @brief Timing reference for keyframe positions.
     */
    enum class TimingMode
    {
        Beats = 0,       // Keyframes positioned in beats (tempo-synced)
        Seconds,         // Keyframes positioned in seconds (free-running)
        Count
    };

    /**
     * @brief Parameter destinations that can be automated by the timeline.
     *
     * This matches PresetManager::PresetValues fields for easy preset morphing.
     */
    enum class ParameterId
    {
        Time = 0,
        Mass,
        Density,
        Bloom,
        Gravity,
        Warp,
        Drift,
        Memory,
        MemoryDepth,
        MemoryDecay,
        MemoryDrift,
        Mix,
        // Macro parameters
        Material,
        Topology,
        Viscosity,
        Evolution,
        ChaosIntensity,
        ElasticityDecay,
        Patina,
        Abyss,
        Corona,
        Breath,
        // Spatial parameters (Three-System Plan)
        PositionX,
        PositionY,
        PositionZ,
        VelocityX,
        Count
    };

    /**
     * @brief A single keyframe: a timestamp and target parameter values.
     */
    struct Keyframe
    {
        double time{0.0};                                           // Position in beats or seconds
        InterpolationType interpolation{InterpolationType::Linear}; // Curve to next keyframe

        // Sparse parameter storage: only store parameters that should be automated
        std::vector<std::pair<ParameterId, float>> parameterValues;

        Keyframe() = default;
        Keyframe(double t, InterpolationType interp = InterpolationType::Linear)
            : time(t), interpolation(interp) {}

        // Helper: set a parameter value in this keyframe
        void setParameter(ParameterId param, float value)
        {
            // Update existing or add new
            for (auto& [id, val] : parameterValues)
            {
                if (id == param)
                {
                    val = value;
                    return;
                }
            }
            parameterValues.emplace_back(param, value);
        }

        // Helper: get parameter value (returns nullopt if not set)
        std::optional<float> getParameter(ParameterId param) const
        {
            for (const auto& [id, val] : parameterValues)
            {
                if (id == param)
                    return val;
            }
            return std::nullopt;
        }
    };

    /**
     * @brief A complete timeline sequence with keyframes and playback settings.
     */
    struct Sequence
    {
        juce::String name{"Untitled Sequence"};
        std::vector<Keyframe> keyframes;
        TimingMode timingMode{TimingMode::Beats};
        PlaybackMode playbackMode{PlaybackMode::Loop};
        double durationBeats{16.0};   // Total duration in beats (for tempo sync)
        double durationSeconds{8.0};  // Total duration in seconds (for free-running)
        bool enabled{false};           // Sequence active/bypassed

        Sequence() = default;
        explicit Sequence(const juce::String& n) : name(n) {}

        // Helper: add a keyframe (keeps keyframes sorted by time)
        void addKeyframe(const Keyframe& keyframe)
        {
            keyframes.push_back(keyframe);
            std::sort(keyframes.begin(), keyframes.end(),
                [](const Keyframe& a, const Keyframe& b) { return a.time < b.time; });
        }

        // Helper: remove keyframe at index
        void removeKeyframe(size_t index)
        {
            if (index < keyframes.size())
                keyframes.erase(keyframes.begin() + static_cast<long>(index));
        }

        // Helper: clear all keyframes
        void clearKeyframes()
        {
            keyframes.clear();
        }
    };

    SequenceScheduler();
    ~SequenceScheduler();

    /**
     * @brief Prepare for processing at given sample rate.
     *
     * Must be called before first process() call and whenever sample rate changes.
     */
    void prepare(double sampleRate, int maxBlockSize);

    /**
     * @brief Reset playback position to start.
     */
    void reset();

    /**
     * @brief Process one audio block, advancing the timeline.
     *
     * This updates the internal playback position based on tempo info (if tempo-synced)
     * or elapsed time (if free-running), then interpolates parameter values between
     * keyframes.
     *
     * @param positionInfo DAW playback position (for tempo sync)
     * @param numSamples Number of samples in this block
     */
    void process(const juce::Optional<juce::AudioPlayHead::PositionInfo>& positionInfo, int numSamples);

    /**
     * @brief Get current interpolated value for a parameter.
     *
     * Returns the value computed from the current timeline position, or nullopt
     * if this parameter is not automated by the active sequence.
     *
     * @param param Parameter to query
     * @return std::optional<float> Current value, or nullopt if not automated
     */
    std::optional<float> getParameterValue(ParameterId param) const noexcept;

    /**
     * @brief Load a sequence and make it active.
     *
     * This replaces the current sequence and resets playback to the start.
     */
    void loadSequence(const Sequence& sequence);

    /**
     * @brief Get the currently loaded sequence.
     */
    const Sequence& getSequence() const noexcept { return currentSequence; }

    /**
     * @brief Set playback enabled/disabled.
     */
    void setEnabled(bool shouldBeEnabled);

    /**
     * @brief Check if playback is enabled.
     */
    bool isEnabled() const noexcept { return currentSequence.enabled; }

    /**
     * @brief Get current playback position (in beats or seconds, depending on timing mode).
     */
    double getCurrentPosition() const noexcept { return currentPosition; }

    /**
     * @brief Set playback position manually (for UI scrubbing).
     */
    void setCurrentPosition(double newPosition);

    /**
     * @brief Get current playback direction (for ping-pong mode).
     */
    bool isPlayingForward() const noexcept { return playingForward; }

    /**
     * @brief Convert ParameterId to parameter name string (for debugging/UI).
     */
    static juce::String parameterIdToString(ParameterId param);

    /**
     * @brief Convert parameter name string to ParameterId.
     */
    static ParameterId stringToParameterId(const juce::String& str);

private:
    double sampleRateHz{48000.0};
    int maxBlockSizeInternal{2048};

    Sequence currentSequence;           // Active timeline sequence
    double currentPosition{0.0};        // Current playback position (beats or seconds)
    bool playingForward{true};          // Direction for ping-pong mode
    double lastTempoBeatsPerMinute{120.0};  // Cached tempo from last process() call

    // Current interpolated parameter values (cache for getParameterValue())
    std::array<std::optional<float>, static_cast<size_t>(ParameterId::Count)> currentValues{};

    // Helper: find keyframe indices surrounding current position
    struct KeyframePair
    {
        size_t beforeIndex{0};
        size_t afterIndex{0};
        bool valid{false};
    };
    KeyframePair findSurroundingKeyframes() const noexcept;

    // Helper: interpolate between two keyframes for a specific parameter
    float interpolateParameter(const Keyframe& before, const Keyframe& after,
                               double fraction, ParameterId param) const noexcept;

    // Helper: apply interpolation curve
    float applyCurve(float t, InterpolationType type) const noexcept;

    // Helper: update all current parameter values based on position
    void updateCurrentValues();

    // Helper: advance playback position by time delta
    void advancePosition(double deltaSeconds, double tempoBeatsPerMinute);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SequenceScheduler)
};

} // namespace dsp
} // namespace monument
