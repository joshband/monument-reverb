#pragma once

#include "SequenceScheduler.h"
#include <vector>

namespace monument
{
namespace dsp
{

/**
 * @brief Factory presets for the SequenceScheduler.
 *
 * These presets demonstrate evolving soundscapes using timeline-based automation:
 * - Evolving Cathedral: Small room gradually expands to massive cathedral over 16 bars
 * - Spatial Journey: Sound source travels through 3D space in tempo-synced patterns
 * - Living Space: Subtle, organic drift in room characteristics over time
 */
class SequencePresets
{
public:
    /**
     * @brief Preset 1: Evolving Cathedral
     *
     * A reverb that morphs from a small, intimate space into a massive cathedral over 16 bars.
     * Perfect for building tension and creating epic crescendos.
     *
     * Timeline (16 beats, tempo-synced):
     * - Beat 0-4: Small room (Time=0.2, Density=0.3, Mass=0.2)
     * - Beat 4-8: Growing space (Time=0.5, Density=0.5, Mass=0.4)
     * - Beat 8-12: Large hall (Time=0.75, Density=0.7, Mass=0.6)
     * - Beat 12-16: Massive cathedral (Time=1.0, Density=0.9, Mass=0.8)
     */
    static SequenceScheduler::Sequence createEvolvingCathedral();

    /**
     * @brief Preset 2: Spatial Journey
     *
     * Sound source moves through 3D space in tempo-synced circular patterns.
     * Creates a sense of motion and spatial dimension synchronized to the beat.
     *
     * Timeline (16 beats, tempo-synced):
     * - Circular path: PositionX/Y trace a circle, PositionZ oscillates
     * - VelocityX creates subtle Doppler shifts as the source moves
     * - S-curve interpolation for smooth, organic motion
     */
    static SequenceScheduler::Sequence createSpatialJourney();

    /**
     * @brief Preset 3: Living Space
     *
     * Subtle, organic drift in room characteristics over 32 seconds.
     * Parameters evolve slowly and continuously, creating a "breathing" reverb.
     *
     * Timeline (32 seconds, free-running):
     * - Warp: 0.0 → 0.3 → 0.0 (shimmer comes and goes)
     * - Drift: 0.0 → 0.2 → 0.0 (subtle pitch modulation)
     * - Bloom: 0.4 → 0.7 → 0.4 (density breathing)
     * - Loop mode: Creates endless evolution
     */
    static SequenceScheduler::Sequence createLivingSpace();

    /**
     * @brief Get all factory presets.
     */
    static std::vector<SequenceScheduler::Sequence> getAllPresets();

    /**
     * @brief Get preset by index.
     */
    static SequenceScheduler::Sequence getPreset(int index);

    /**
     * @brief Get number of factory presets.
     */
    static constexpr int getNumPresets() { return 3; }

    /**
     * @brief Get preset name by index.
     */
    static juce::String getPresetName(int index);

    /**
     * @brief Get preset description by index.
     */
    static juce::String getPresetDescription(int index);
};

} // namespace dsp
} // namespace monument
