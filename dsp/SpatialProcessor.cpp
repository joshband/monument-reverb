#include "SpatialProcessor.h"
#include <algorithm>

namespace monument
{
namespace dsp
{

void SpatialProcessor::prepare(double sampleRate, int blockSize, int numLines) noexcept
{
    (void)blockSize;
    sampleRate_ = sampleRate;
    numLines_ = juce::jlimit(0, kMaxLines, numLines);

    // Initialize all positions to centered (0, 0, 0.5)
    reset();
}

void SpatialProcessor::reset() noexcept
{
    // Reset all spatial positions to centered
    for (size_t i = 0; i < positionsX_.size(); ++i)
    {
        positionsX_[i] = 0.0f;
        positionsY_[i] = 0.0f;
        positionsZ_[i] = 0.5f; // Middle elevation
        velocitiesX_[i] = 0.0f;
        distances_[i] = kReferenceDistance;
        attenuationGains_[i] = 1.0f; // No attenuation at reference distance
    }

    distanceScale_ = 1.0f;
    dopplerScale_ = 0.5f;
}

void SpatialProcessor::process() noexcept
{
    const auto lineCount = static_cast<size_t>(numLines_);
    if (!enabled_)
    {
        // When disabled, set all gains to 1.0 (no attenuation)
        for (size_t i = 0; i < lineCount; ++i)
            attenuationGains_[i] = 1.0f;
        return;
    }

    // Update distance and attenuation for each line (block-rate calculation)
    for (size_t i = 0; i < lineCount; ++i)
    {
        // Compute distance from listener (origin) to source position
        distances_[i] = computeDistance(
            positionsX_[i] * distanceScale_,
            positionsY_[i] * distanceScale_,
            positionsZ_[i] * distanceScale_
        );

        // Compute attenuation gain using inverse square law
        attenuationGains_[i] = computeAttenuation(distances_[i]);
    }
}

float SpatialProcessor::getAttenuationGain(int lineIndex) const noexcept
{
    if (lineIndex < 0 || lineIndex >= numLines_)
        return 1.0f;

    return attenuationGains_[static_cast<size_t>(lineIndex)];
}

float SpatialProcessor::getDopplerShift(int lineIndex) const noexcept
{
    if (!enabled_ || lineIndex < 0 || lineIndex >= numLines_)
        return 0.0f;

    // Doppler shift based on velocity
    // Positive velocity = moving away = delay increases = pitch down
    // Negative velocity = moving closer = delay decreases = pitch up
    float shift = velocitiesX_[static_cast<size_t>(lineIndex)] * kMaxDopplerShiftSamples * dopplerScale_;

    // Clamp to prevent excessive pitch shifting
    return juce::jlimit(-kMaxDopplerShiftSamples, kMaxDopplerShiftSamples, shift);
}

void SpatialProcessor::setPosition(int lineIndex, float x, float y, float z) noexcept
{
    if (lineIndex < 0 || lineIndex >= numLines_)
        return;

    const auto index = static_cast<size_t>(lineIndex);
    // Clamp positions to valid ranges
    positionsX_[index] = juce::jlimit(-1.0f, 1.0f, x);
    positionsY_[index] = juce::jlimit(-1.0f, 1.0f, y);
    positionsZ_[index] = juce::jlimit(0.0f, 1.0f, z);
}

void SpatialProcessor::setVelocity(int lineIndex, float velocityX) noexcept
{
    if (lineIndex < 0 || lineIndex >= numLines_)
        return;

    velocitiesX_[static_cast<size_t>(lineIndex)] = juce::jlimit(-1.0f, 1.0f, velocityX);
}

void SpatialProcessor::setDistanceScale(float scale) noexcept
{
    distanceScale_ = juce::jlimit(0.0f, 1.0f, scale);
}

void SpatialProcessor::setEnabled(bool shouldEnable) noexcept
{
    enabled_ = shouldEnable;
}

void SpatialProcessor::setDopplerScale(float scale) noexcept
{
    dopplerScale_ = juce::jlimit(0.0f, 1.0f, scale);
}

//==============================================================================
// Private helper methods
//==============================================================================

float SpatialProcessor::computeDistance(float x, float y, float z) const noexcept
{
    // Euclidean distance from listener at origin (0, 0, 0) to source at (x, y, z)
    float distanceSq = x * x + y * y + z * z;
    return std::sqrt(distanceSq + kEpsilon); // Add epsilon to prevent zero distance
}

float SpatialProcessor::computeAttenuation(float distance) const noexcept
{
    // Inverse square law: I ∝ 1/r²
    // Gain = (referenceDistance / distance)²
    // Equivalent to: referenceDistance² / distance²

    float referenceSq = kReferenceDistance * kReferenceDistance;
    float distanceSq = distance * distance;

    // Compute attenuation with safeguard against extreme values
    float gain = referenceSq / (distanceSq + kEpsilon);

    // Clamp to [0, 1] range (no amplification, only attenuation)
    return juce::jlimit(0.0f, 1.0f, gain);
}

} // namespace dsp
} // namespace monument
