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
     * @brief Preset 4: Infinite Abyss
     *
     * Bottomless pit with eternal memory feedback. The reverb never truly ends,
     * with the memory system creating cascading recursive echoes.
     *
     * Timeline (64 beats, tempo-synced):
     * - Chaos Attractor modulates Gravity (creates unstable floor sensation)
     * - Ultra-long decay with maximum memory depth and feedback
     * - Dense, massive space that feels like falling forever
     */
    static SequenceScheduler::Sequence createInfiniteAbyss();

    /**
     * @brief Preset 5: Quantum Tunneling
     *
     * Sound teleports through impossible geometry using extreme spatial warp.
     * Creates Doppler-shifted echoes as the sound source jumps through space.
     *
     * Timeline (32 beats, tempo-synced):
     * - Rapid 3D spiral path (PositionX/Y/Z)
     * - Maximum warp + drift for spatial distortion
     * - Sparse density with high bloom creates metallic artifacts
     */
    static SequenceScheduler::Sequence createQuantumTunneling();

    /**
     * @brief Preset 6: Time Dissolution
     *
     * Time itself becomes unstable. Extreme drift with brownian motion
     * causes the decay rate to organically wander, creating unpredictable evolution.
     *
     * Timeline (free-running, 120 seconds):
     * - Brownian motion modulates both Drift and Time
     * - Weightless (low mass) for ethereal, unstable quality
     * - Creates wildly shifting pitch and temporal artifacts
     */
    static SequenceScheduler::Sequence createTimeDissolution();

    /**
     * @brief Preset 7: Crystalline Void
     *
     * Ultra-sparse delay taps create metallic, glass-like resonances in vast space.
     * Pillar positions dance with input signal, creating responsive crystalline artifacts.
     *
     * Timeline (48 beats, tempo-synced):
     * - Ultra-sparse density (0.05) with maximum bloom
     * - High mass + extreme pillar shape for metallic quality
     * - Audio follower modulates tap positions for reactive crystals
     */
    static SequenceScheduler::Sequence createCrystallineVoid();

    /**
     * @brief Preset 8: Hyperdimensional Fold
     *
     * Every dimension modulates simultaneously. All 4 Ancient Monument macros
     * (Material, Topology, Viscosity, Evolution) morph continuously over 64 beats.
     * Multiple modulation sources create never-repeating impossible space.
     *
     * Timeline (64 beats, tempo-synced):
     * - Material: 0.0 → 1.0 → 0.0 (wood → metal → glass)
     * - Topology: complex wave pattern (room shape constantly shifts)
     * - Viscosity: 0.2 → 0.9 (air density changes)
     * - Evolution: slow drift (entire character evolves)
     * - Chaos + Brownian modulators create organic chaos
     */
    static SequenceScheduler::Sequence createHyperdimensionalFold();

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
    static constexpr int getNumPresets() { return 8; }

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
