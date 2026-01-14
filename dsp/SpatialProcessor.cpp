#include "SpatialProcessor.h"
#include <algorithm>
#include <cmath>

namespace
{
constexpr std::array<float, 8> kDefaultPositionsX{
    -0.9f, 0.9f, -0.7f, 0.7f, -0.5f, 0.5f, -0.3f, 0.3f
};
}

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
        positionsX_[i] = i < kDefaultPositionsX.size() ? kDefaultPositionsX[i] : 0.0f;
        positionsY_[i] = 0.0f;
        positionsZ_[i] = 0.5f; // Middle elevation
        velocitiesX_[i] = 0.0f;
        distances_[i] = kReferenceDistance;
        attenuationGains_[i] = 1.0f; // No attenuation at reference distance
        panLeft_[i] = 0.7071f;
        panRight_[i] = 0.7071f;
        airAbsorptionGains_[i] = 1.0f;
        ambisonicW_[i] = kFOAW;
        ambisonicX_[i] = 0.0f;
        ambisonicY_[i] = 0.0f;
        ambisonicZ_[i] = 0.0f;
        motionPhase_[i] = 0.0f;
        motionOffsetX_[i] = 0.0f;
        motionOffsetY_[i] = 0.0f;
        motionOffsetZ_[i] = 0.0f;
    }

    distanceScale_ = 1.0f;
    dopplerScale_ = 0.5f;
    mode_ = Mode::StereoApprox;
    motionPath_ = MotionPath::Static;
    crossfeedAmount_ = 0.0f;
    airAbsorption_ = 0.0f;
    motionRateHz_ = 0.0f;
    motionRadius_ = 0.0f;
    motionDepth_ = 0.0f;
}

void SpatialProcessor::process(int numSamples) noexcept
{
    const auto lineCount = static_cast<size_t>(numLines_);
    if (!enabled_)
    {
        // When disabled, set all gains to 1.0 (no attenuation)
        for (size_t i = 0; i < lineCount; ++i)
        {
            attenuationGains_[i] = 1.0f;
            panLeft_[i] = 0.7071f;
            panRight_[i] = 0.7071f;
            airAbsorptionGains_[i] = 1.0f;
            ambisonicW_[i] = kFOAW;
            ambisonicX_[i] = 0.0f;
            ambisonicY_[i] = 0.0f;
            ambisonicZ_[i] = 0.0f;
        }
        return;
    }

    const float blockDuration = (numSamples > 0)
        ? static_cast<float>(numSamples / sampleRate_)
        : 0.0f;
    const float phaseStep = motionRateHz_ > 0.0f ? motionRateHz_ * blockDuration : 0.0f;

    // Update distance and attenuation for each line (block-rate calculation)
    for (size_t i = 0; i < lineCount; ++i)
    {
        float motionOffsetX = motionOffsetX_[i];
        float motionOffsetY = motionOffsetY_[i];
        float motionOffsetZ = motionOffsetZ_[i];

        if (motionPath_ != MotionPath::Static && (motionRadius_ > 0.0f || motionDepth_ > 0.0f))
        {
            motionPhase_[i] += phaseStep;
            if (motionPhase_[i] >= 1.0f)
                motionPhase_[i] -= std::floor(motionPhase_[i]);

            const float phase = motionPhase_[i] * juce::MathConstants<float>::twoPi;
            switch (motionPath_)
            {
                case MotionPath::Circle:
                    motionOffsetX = std::cos(phase) * motionRadius_;
                    motionOffsetY = std::sin(phase) * motionRadius_;
                    motionOffsetZ = std::sin(phase * 0.5f) * motionDepth_;
                    velocitiesX_[i] = (motionRateHz_ > 0.0f && motionRadius_ > 0.0f)
                        ? (-std::sin(phase) * juce::MathConstants<float>::twoPi * motionRateHz_ * motionRadius_)
                            / (juce::MathConstants<float>::twoPi * motionRateHz_ * motionRadius_)
                        : 0.0f;
                    break;
                case MotionPath::Figure8:
                    motionOffsetX = std::sin(phase) * motionRadius_;
                    motionOffsetY = std::sin(phase * 2.0f) * (motionRadius_ * 0.6f);
                    motionOffsetZ = std::cos(phase) * motionDepth_;
                    velocitiesX_[i] = (motionRateHz_ > 0.0f && motionRadius_ > 0.0f)
                        ? (std::cos(phase) * juce::MathConstants<float>::twoPi * motionRateHz_ * motionRadius_)
                            / (juce::MathConstants<float>::twoPi * motionRateHz_ * motionRadius_)
                        : 0.0f;
                    break;
                case MotionPath::RandomWalk:
                {
                    const float step = motionRadius_ * 0.05f;
                    const float prevOffsetX = motionOffsetX;
                    motionOffsetX = juce::jlimit(-motionRadius_, motionRadius_,
                        motionOffsetX + (motionRng_.nextFloat() * 2.0f - 1.0f) * step);
                    motionOffsetY = juce::jlimit(-motionRadius_, motionRadius_,
                        motionOffsetY + (motionRng_.nextFloat() * 2.0f - 1.0f) * step);
                    motionOffsetZ = juce::jlimit(-motionDepth_, motionDepth_,
                        motionOffsetZ + (motionRng_.nextFloat() * 2.0f - 1.0f) * step);
                    const float velocityNorm = (motionRadius_ > 0.0f)
                        ? (motionOffsetX - prevOffsetX) / motionRadius_
                        : 0.0f;
                    velocitiesX_[i] = juce::jlimit(-1.0f, 1.0f, velocityNorm);
                    break;
                }
                case MotionPath::Static:
                default:
                    break;
            }

            motionOffsetX_[i] = motionOffsetX;
            motionOffsetY_[i] = motionOffsetY;
            motionOffsetZ_[i] = motionOffsetZ;
        }
        else
        {
            motionOffsetX = 0.0f;
            motionOffsetY = 0.0f;
            motionOffsetZ = 0.0f;
        }

        // Compute distance from listener (origin) to source position
        const float finalX = (positionsX_[i] + motionOffsetX) * distanceScale_;
        const float finalY = (positionsY_[i] + motionOffsetY) * distanceScale_;
        const float finalZ = (positionsZ_[i] + motionOffsetZ) * distanceScale_;
        distances_[i] = computeDistance(finalX, finalY, finalZ);

        // Compute attenuation gain using inverse square law
        attenuationGains_[i] = computeAttenuation(distances_[i]);

        // Stereo pan from X position (constant-power)
        const float pan = juce::jlimit(-1.0f, 1.0f, finalX);
        const float panAngle = (pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
        float left = std::cos(panAngle);
        float right = std::sin(panAngle);

        if (mode_ == Mode::HrtfConvolution)
        {
            // Simple HRTF-like shadowing: reduce far ear slightly based on azimuth.
            const float azimuth = std::atan2(finalX, juce::jmax(0.001f, finalY));
            const float shadow = 0.7f + 0.3f * std::cos(azimuth);
            if (azimuth >= 0.0f)
                left *= shadow;
            else
                right *= shadow;
        }

        // Optional crossfeed
        const float cross = crossfeedAmount_;
        if (cross > 0.0f)
        {
            const float leftBase = left;
            const float rightBase = right;
            left = leftBase * (1.0f - cross) + rightBase * cross;
            right = rightBase * (1.0f - cross) + leftBase * cross;
            const float norm = 1.0f / juce::jmax(1.0f, 1.0f + cross);
            left *= norm;
            right *= norm;
        }

        panLeft_[i] = left;
        panRight_[i] = right;

        // Air absorption gain scales with distance (subtle attenuation)
        const float distanceFactor = juce::jlimit(0.0f, 1.0f,
            (distances_[i] - kReferenceDistance) / (kReferenceDistance * 3.0f));
        airAbsorptionGains_[i] = std::exp(-airAbsorption_ * distanceFactor * 2.0f);

        // FOA coefficients (ACN/SN3D)
        const float azimuth = std::atan2(finalX, finalY);
        const float elevation = std::atan2(finalZ, std::sqrt(finalX * finalX + finalY * finalY));
        ambisonicW_[i] = kFOAW;
        ambisonicX_[i] = std::cos(azimuth) * std::cos(elevation);
        ambisonicY_[i] = std::sin(azimuth) * std::cos(elevation);
        ambisonicZ_[i] = std::sin(elevation);
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

void SpatialProcessor::getStereoGains(int lineIndex, float& left, float& right) const noexcept
{
    if (lineIndex < 0 || lineIndex >= numLines_)
    {
        left = 0.7071f;
        right = 0.7071f;
        return;
    }

    const auto index = static_cast<size_t>(lineIndex);
    left = panLeft_[index];
    right = panRight_[index];
}

void SpatialProcessor::getAmbisonicCoeffs(int lineIndex, float& w, float& x, float& y, float& z) const noexcept
{
    if (lineIndex < 0 || lineIndex >= numLines_)
    {
        w = kFOAW;
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
        return;
    }

    const auto index = static_cast<size_t>(lineIndex);
    w = ambisonicW_[index];
    x = ambisonicX_[index];
    y = ambisonicY_[index];
    z = ambisonicZ_[index];
}

float SpatialProcessor::getAirAbsorptionGain(int lineIndex) const noexcept
{
    if (lineIndex < 0 || lineIndex >= numLines_)
        return 1.0f;

    return airAbsorptionGains_[static_cast<size_t>(lineIndex)];
}

void SpatialProcessor::setPosition(int lineIndex, float x, float y, float z) noexcept
{
    if (lineIndex < 0 || lineIndex >= numLines_)
        return;

    const auto index = static_cast<size_t>(lineIndex);
    // Clamp positions to reasonable ranges (wider than [-1,1] to allow distance testing)
    // For UI parameters, caller should pre-normalize; for testing, allow wider range
    positionsX_[index] = juce::jlimit(-10.0f, 10.0f, x);
    positionsY_[index] = juce::jlimit(-10.0f, 10.0f, y);
    positionsZ_[index] = juce::jlimit(0.0f, 10.0f, z);
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
