#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cmath>

namespace monument
{
namespace dsp
{

/**
 * SpatialProcessor provides 3D spatial positioning for delay lines in the reverb network.
 *
 * Features:
 * - Distance-based attenuation (1/r² inverse square law)
 * - 3D position tracking (X, Y, Z) per delay line
 * - Velocity-based Doppler shift calculation
 * - Real-time safe: no allocations, no locks, block-rate updates
 *
 * Integration with Chambers FDN:
 * - Each of 8 delay lines can have independent spatial position
 * - Attenuation coefficients applied per-sample in FDN loop
 * - Doppler shifts applied to fractional delay time
 *
 * Physical Model:
 * - Distance attenuation: gain = referenceDistance² / (distance² + epsilon)
 * - Doppler shift: delaySamples += velocity * dopplerScale
 * - Positions normalized: X/Y ∈ [-1, +1], Z ∈ [0, +1] (Z=0 is listener plane)
 */
class SpatialProcessor final
{
public:
    SpatialProcessor() = default;
    ~SpatialProcessor() = default;

    // Non-copyable (contains state arrays)
    SpatialProcessor(const SpatialProcessor&) = delete;
    SpatialProcessor& operator=(const SpatialProcessor&) = delete;

    /**
     * Prepares the spatial processor for audio processing.
     * Must be called before process() or any audio-rate methods.
     *
     * @param sampleRate Sample rate in Hz
     * @param blockSize Maximum expected block size
     * @param numLines Number of delay lines (typically 8 for Chambers FDN)
     */
    void prepare(double sampleRate, int blockSize, int numLines) noexcept;

    /**
     * Resets all spatial state to defaults (centered positions, zero velocity).
     */
    void reset() noexcept;

    /**
     * Updates spatial calculations for the current block.
     * Call once per block before getting per-line attenuation/Doppler values.
     *
     * Thread safety: Must be called from audio thread only.
     */
    void process() noexcept;

    /**
     * Gets the distance attenuation gain for a specific delay line.
     * Uses inverse square law: gain = referenceDistance² / (distance² + epsilon)
     *
     * @param lineIndex Index of the delay line [0, numLines-1]
     * @return Attenuation gain [0, 1], where 0 = infinite distance, 1 = reference distance
     */
    float getAttenuationGain(int lineIndex) const noexcept;

    /**
     * Gets the Doppler shift in samples for a specific delay line.
     * Positive shift = delay increases (source moving away, pitch down)
     * Negative shift = delay decreases (source moving closer, pitch up)
     *
     * @param lineIndex Index of the delay line [0, numLines-1]
     * @return Doppler shift in samples, range depends on dopplerScale
     */
    float getDopplerShift(int lineIndex) const noexcept;

    //==========================================================================
    // Parameter setters (thread-safe, use atomic writes if needed in future)
    //==========================================================================

    /**
     * Sets the 3D position for a specific delay line.
     * Positions are normalized coordinates relative to listener at origin.
     *
     * @param lineIndex Index of the delay line [0, numLines-1]
     * @param x Horizontal position [-1, +1], left to right
     * @param y Depth position [-1, +1], front to back
     * @param z Vertical position [0, +1], listener plane to above
     */
    void setPosition(int lineIndex, float x, float y, float z) noexcept;

    /**
     * Sets the velocity for a specific delay line (for Doppler effect).
     * Velocity is normalized: +1.0 = maximum speed away, -1.0 = maximum speed toward.
     *
     * @param lineIndex Index of the delay line [0, numLines-1]
     * @param velocityX Horizontal velocity component [-1, +1]
     */
    void setVelocity(int lineIndex, float velocityX) noexcept;

    /**
     * Sets the global distance scale factor.
     * Multiplies all distance calculations, effectively zooming the spatial field.
     *
     * @param scale Distance scale [0, 1], where 1.0 = normal, 0.0 = all sources at origin
     */
    void setDistanceScale(float scale) noexcept;

    /**
     * Enables or disables spatial processing entirely.
     * When disabled, all attenuation gains return 1.0 (no effect).
     *
     * @param shouldEnable true to enable spatial processing, false to bypass
     */
    void setEnabled(bool shouldEnable) noexcept;

    /**
     * Sets the Doppler effect intensity.
     *
     * @param scale Doppler scale [0, 1], where 0 = no Doppler, 1 = full effect
     */
    void setDopplerScale(float scale) noexcept;

private:
    static constexpr int kMaxLines = 8;              // Maximum delay lines (Chambers FDN)
    static constexpr float kReferenceDistance = 1.0f; // Reference distance for 0dB attenuation
    static constexpr float kEpsilon = 0.01f;          // Prevents division by zero at origin
    static constexpr float kMaxDopplerShiftSamples = 2400.0f; // ±50ms @ 48kHz

    double sampleRate_ = 48000.0;
    int numLines_ = 8;
    bool enabled_ = true;

    // Spatial state per delay line
    std::array<float, kMaxLines> positionsX_{};   // [-1, +1]
    std::array<float, kMaxLines> positionsY_{};   // [-1, +1]
    std::array<float, kMaxLines> positionsZ_{};   // [0, +1]
    std::array<float, kMaxLines> velocitiesX_{};  // [-1, +1]
    std::array<float, kMaxLines> distances_{};    // Computed distance from listener
    std::array<float, kMaxLines> attenuationGains_{}; // Cached attenuation coefficients

    float distanceScale_ = 1.0f;
    float dopplerScale_ = 0.5f; // Default: moderate Doppler effect

    // Computes Euclidean distance from listener (origin) to 3D position
    float computeDistance(float x, float y, float z) const noexcept;

    // Computes inverse square law attenuation from distance
    float computeAttenuation(float distance) const noexcept;
};

} // namespace dsp
} // namespace monument
