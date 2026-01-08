#include "SequenceScheduler.h"
#include <cmath>
#include <algorithm>

namespace monument
{
namespace dsp
{

SequenceScheduler::SequenceScheduler()
{
    // Initialize with empty sequence
    currentSequence.name = "Empty Sequence";
    currentSequence.enabled = false;
}

SequenceScheduler::~SequenceScheduler()
{
}

void SequenceScheduler::prepare(double sampleRate, int maxBlockSize)
{
    sampleRateHz = sampleRate;
    maxBlockSizeInternal = maxBlockSize;

    // Initialize all parameter values to nullopt (not automated)
    for (auto& val : currentValues)
        val = std::nullopt;
}

void SequenceScheduler::reset()
{
    currentPosition = 0.0;
    playingForward = true;

    // Clear all current values
    for (auto& val : currentValues)
        val = std::nullopt;
}

void SequenceScheduler::process(const juce::Optional<juce::AudioPlayHead::PositionInfo>& positionInfo, int numSamples)
{
    // Skip processing if sequence is disabled or empty
    if (!currentSequence.enabled || currentSequence.keyframes.empty())
    {
        return;
    }

    // Get tempo info for beat-based timing
    double tempoBeatsPerMinute = 120.0;  // Default tempo
    if (positionInfo.hasValue() && positionInfo->getBpm().hasValue())
    {
        tempoBeatsPerMinute = *positionInfo->getBpm();
    }
    lastTempoBeatsPerMinute = tempoBeatsPerMinute;

    // Calculate time delta for this block
    double deltaSeconds = static_cast<double>(numSamples) / sampleRateHz;

    // Advance playback position
    advancePosition(deltaSeconds, tempoBeatsPerMinute);

    // Update all interpolated parameter values
    updateCurrentValues();
}

std::optional<float> SequenceScheduler::getParameterValue(ParameterId param) const noexcept
{
    if (static_cast<size_t>(param) >= currentValues.size())
        return std::nullopt;

    return currentValues[static_cast<size_t>(param)];
}

void SequenceScheduler::loadSequence(const Sequence& sequence)
{
    currentSequence = sequence;
    reset();
}

void SequenceScheduler::setEnabled(bool shouldBeEnabled)
{
    currentSequence.enabled = shouldBeEnabled;

    if (!shouldBeEnabled)
    {
        // Clear all current values when disabled
        for (auto& val : currentValues)
            val = std::nullopt;
    }
}

void SequenceScheduler::setCurrentPosition(double newPosition)
{
    currentPosition = newPosition;

    // Clamp to valid range
    if (currentSequence.timingMode == TimingMode::Beats)
    {
        currentPosition = juce::jlimit(0.0, currentSequence.durationBeats, currentPosition);
    }
    else
    {
        currentPosition = juce::jlimit(0.0, currentSequence.durationSeconds, currentPosition);
    }

    updateCurrentValues();
}

juce::String SequenceScheduler::parameterIdToString(ParameterId param)
{
    switch (param)
    {
        case ParameterId::Time: return "Time";
        case ParameterId::Mass: return "Mass";
        case ParameterId::Density: return "Density";
        case ParameterId::Bloom: return "Bloom";
        case ParameterId::Gravity: return "Gravity";
        case ParameterId::Warp: return "Warp";
        case ParameterId::Drift: return "Drift";
        case ParameterId::Memory: return "Memory";
        case ParameterId::MemoryDepth: return "MemoryDepth";
        case ParameterId::MemoryDecay: return "MemoryDecay";
        case ParameterId::MemoryDrift: return "MemoryDrift";
        case ParameterId::Mix: return "Mix";
        case ParameterId::Material: return "Material";
        case ParameterId::Topology: return "Topology";
        case ParameterId::Viscosity: return "Viscosity";
        case ParameterId::Evolution: return "Evolution";
        case ParameterId::ChaosIntensity: return "ChaosIntensity";
        case ParameterId::ElasticityDecay: return "ElasticityDecay";
        case ParameterId::Patina: return "Patina";
        case ParameterId::Abyss: return "Abyss";
        case ParameterId::Corona: return "Corona";
        case ParameterId::Breath: return "Breath";
        case ParameterId::PositionX: return "PositionX";
        case ParameterId::PositionY: return "PositionY";
        case ParameterId::PositionZ: return "PositionZ";
        case ParameterId::VelocityX: return "VelocityX";
        case ParameterId::Count: return "Count";
        default: return "Unknown";
    }
}

SequenceScheduler::ParameterId SequenceScheduler::stringToParameterId(const juce::String& str)
{
    if (str == "Time") return ParameterId::Time;
    if (str == "Mass") return ParameterId::Mass;
    if (str == "Density") return ParameterId::Density;
    if (str == "Bloom") return ParameterId::Bloom;
    if (str == "Gravity") return ParameterId::Gravity;
    if (str == "Warp") return ParameterId::Warp;
    if (str == "Drift") return ParameterId::Drift;
    if (str == "Memory") return ParameterId::Memory;
    if (str == "MemoryDepth") return ParameterId::MemoryDepth;
    if (str == "MemoryDecay") return ParameterId::MemoryDecay;
    if (str == "MemoryDrift") return ParameterId::MemoryDrift;
    if (str == "Mix") return ParameterId::Mix;
    if (str == "Material") return ParameterId::Material;
    if (str == "Topology") return ParameterId::Topology;
    if (str == "Viscosity") return ParameterId::Viscosity;
    if (str == "Evolution") return ParameterId::Evolution;
    if (str == "ChaosIntensity") return ParameterId::ChaosIntensity;
    if (str == "ElasticityDecay") return ParameterId::ElasticityDecay;
    if (str == "Patina") return ParameterId::Patina;
    if (str == "Abyss") return ParameterId::Abyss;
    if (str == "Corona") return ParameterId::Corona;
    if (str == "Breath") return ParameterId::Breath;
    if (str == "PositionX") return ParameterId::PositionX;
    if (str == "PositionY") return ParameterId::PositionY;
    if (str == "PositionZ") return ParameterId::PositionZ;
    if (str == "VelocityX") return ParameterId::VelocityX;

    return ParameterId::Time;  // Default fallback
}

// Private helper methods

SequenceScheduler::KeyframePair SequenceScheduler::findSurroundingKeyframes() const noexcept
{
    KeyframePair result;

    if (currentSequence.keyframes.empty())
    {
        result.valid = false;
        return result;
    }

    // Single keyframe: hold its values
    if (currentSequence.keyframes.size() == 1)
    {
        result.beforeIndex = 0;
        result.afterIndex = 0;
        result.valid = true;
        return result;
    }

    // Find keyframes surrounding current position
    for (size_t i = 0; i < currentSequence.keyframes.size() - 1; ++i)
    {
        if (currentPosition >= currentSequence.keyframes[i].time &&
            currentPosition <= currentSequence.keyframes[i + 1].time)
        {
            result.beforeIndex = i;
            result.afterIndex = i + 1;
            result.valid = true;
            return result;
        }
    }

    // Position is before first keyframe
    if (currentPosition < currentSequence.keyframes.front().time)
    {
        result.beforeIndex = 0;
        result.afterIndex = 0;
        result.valid = true;
        return result;
    }

    // Position is after last keyframe
    if (currentPosition >= currentSequence.keyframes.back().time)
    {
        size_t lastIndex = currentSequence.keyframes.size() - 1;
        result.beforeIndex = lastIndex;
        result.afterIndex = lastIndex;
        result.valid = true;
        return result;
    }

    result.valid = false;
    return result;
}

float SequenceScheduler::interpolateParameter(const Keyframe& before, const Keyframe& after,
                                              double fraction, ParameterId param) const noexcept
{
    // Get parameter values from both keyframes
    auto beforeValue = before.getParameter(param);
    auto afterValue = after.getParameter(param);

    // If parameter not set in either keyframe, return 0.5 (neutral)
    if (!beforeValue.has_value() && !afterValue.has_value())
        return 0.5f;

    // If only one keyframe has the parameter, use that value
    if (!beforeValue.has_value())
        return *afterValue;
    if (!afterValue.has_value())
        return *beforeValue;

    // Both keyframes have the parameter: interpolate
    float t = applyCurve(static_cast<float>(fraction), before.interpolation);
    return *beforeValue + t * (*afterValue - *beforeValue);
}

float SequenceScheduler::applyCurve(float t, InterpolationType type) const noexcept
{
    // Clamp input to [0, 1]
    t = juce::jlimit(0.0f, 1.0f, t);

    switch (type)
    {
        case InterpolationType::Linear:
            return t;

        case InterpolationType::Exponential:
            // Ease-in (accelerating)
            return t * t;

        case InterpolationType::SCurve:
            // Smooth ease-in-out (S-curve)
            return t * t * (3.0f - 2.0f * t);

        case InterpolationType::Step:
            // Instant jump at midpoint
            return t < 0.5f ? 0.0f : 1.0f;

        case InterpolationType::Count:
            return t;

        default:
            return t;
    }
}

void SequenceScheduler::updateCurrentValues()
{
    // Find surrounding keyframes
    auto pair = findSurroundingKeyframes();

    if (!pair.valid || currentSequence.keyframes.empty())
    {
        // No valid keyframes: clear all values
        for (auto& val : currentValues)
            val = std::nullopt;
        return;
    }

    const auto& beforeKeyframe = currentSequence.keyframes[pair.beforeIndex];
    const auto& afterKeyframe = currentSequence.keyframes[pair.afterIndex];

    // Calculate interpolation fraction
    double fraction = 0.0;
    if (pair.beforeIndex != pair.afterIndex)
    {
        double timeDelta = afterKeyframe.time - beforeKeyframe.time;
        if (timeDelta > 0.0)
        {
            fraction = (currentPosition - beforeKeyframe.time) / timeDelta;
            fraction = juce::jlimit(0.0, 1.0, fraction);
        }
    }

    // Interpolate all parameters
    for (size_t i = 0; i < static_cast<size_t>(ParameterId::Count); ++i)
    {
        auto param = static_cast<ParameterId>(i);

        // Check if this parameter is automated by either keyframe
        bool isAutomated = beforeKeyframe.getParameter(param).has_value() ||
                          afterKeyframe.getParameter(param).has_value();

        if (isAutomated)
        {
            currentValues[i] = interpolateParameter(beforeKeyframe, afterKeyframe, fraction, param);
        }
        else
        {
            currentValues[i] = std::nullopt;
        }
    }
}

void SequenceScheduler::advancePosition(double deltaSeconds, double tempoBeatsPerMinute)
{
    // Calculate position increment
    double increment = 0.0;

    if (currentSequence.timingMode == TimingMode::Beats)
    {
        // Convert seconds to beats using tempo
        double beatsPerSecond = tempoBeatsPerMinute / 60.0;
        increment = deltaSeconds * beatsPerSecond;
    }
    else
    {
        // Free-running time
        increment = deltaSeconds;
    }

    // Apply playback direction
    if (!playingForward)
        increment = -increment;

    // Advance position
    currentPosition += increment;

    // Handle playback mode boundaries
    double duration = (currentSequence.timingMode == TimingMode::Beats)
        ? currentSequence.durationBeats
        : currentSequence.durationSeconds;

    switch (currentSequence.playbackMode)
    {
        case PlaybackMode::OneShot:
            // Clamp to [0, duration]
            currentPosition = juce::jlimit(0.0, duration, currentPosition);

            // Stop at end
            if (currentPosition >= duration)
            {
                currentPosition = duration;
                // Could add a "finished" callback here
            }
            break;

        case PlaybackMode::Loop:
            // Wrap around
            while (currentPosition >= duration)
                currentPosition -= duration;
            while (currentPosition < 0.0)
                currentPosition += duration;
            break;

        case PlaybackMode::PingPong:
            // Bounce at boundaries
            if (currentPosition >= duration)
            {
                currentPosition = duration - (currentPosition - duration);
                playingForward = false;
            }
            else if (currentPosition < 0.0)
            {
                currentPosition = -currentPosition;
                playingForward = true;
            }
            break;

        case PlaybackMode::Count:
            currentPosition = juce::jlimit(0.0, duration, currentPosition);
            break;

        default:
            break;
    }
}

} // namespace dsp
} // namespace monument
