#pragma once

#include <JuceHeader.h>
#include "DspRoutingGraph.h"

namespace monument
{
namespace dsp
{

/**
 * @brief Expressive macro controls for immediate musical results
 *
 * This system replaces the conceptual macros (Material, Topology, Viscosity)
 * with immediately musical, performance-oriented controls that create dramatic
 * sonic diversity without parameter conflicts.
 *
 * Key Design Principles:
 * 1. **Minimal Overlap**: Each macro controls orthogonal aspects
 * 2. **Musical Meaning**: Names map directly to sonic results
 * 3. **Dramatic Range**: Extreme values create fundamentally different sounds
 * 4. **Performance-Ready**: Ideal for live tweaking and automation
 *
 * Macro Descriptions:
 *
 * **Character** (0 = Subtle → 1 = Extreme)
 * - Controls overall intensity: drive, saturation, density, feedback
 * - Low: Transparent, mixing-friendly
 * - High: Dramatic, effect-centric, sound design
 *
 * **Space Type** (Discrete Modes with Morphing)
 * - 0.0-0.2: Chamber (small, resonant, focused)
 * - 0.2-0.4: Hall (large, smooth, musical)
 * - 0.4-0.6: Shimmer (pitched, bright, ethereal)
 * - 0.6-0.8: Granular (textured, diffuse, cloud)
 * - 0.8-1.0: Metallic (tube resonances, ringing)
 * - Selects routing preset + module enables
 *
 * **Energy** (Decay Behavior)
 * - 0.0-0.2: Decay (traditional fade-out)
 * - 0.3-0.5: Sustain (stable hold, freeze-like)
 * - 0.6-0.8: Grow (bloom, building swell)
 * - 0.9-1.0: Chaos (unpredictable, oscillating)
 * - Controls feedback, bloom, freeze, paradox gain
 *
 * **Motion** (Temporal Evolution)
 * - 0.0-0.2: Still (static, frozen, architectural)
 * - 0.3-0.5: Drift (slow Brownian wander)
 * - 0.6-0.8: Pulse (rhythmic LFO modulation)
 * - 0.9-1.0: Random (chaotic attractor jumps)
 * - Controls drift, warp, modulation depth, LFO rate
 *
 * **Color** (Spectral Character)
 * - 0.0-0.2: Dark (lo-fi, vintage, muffled)
 * - 0.3-0.6: Balanced (neutral, transparent)
 * - 0.7-0.8: Bright (air, shimmer, clarity)
 * - 0.9-1.0: Spectral (harmonic distortion, ringing)
 * - Controls mass, air, gravity, metallic resonance
 *
 * **Dimension** (Perceived Space Size)
 * - 0.0-0.2: Intimate (close, personal, booth)
 * - 0.3-0.5: Room (standard studio space)
 * - 0.6-0.8: Cathedral (large, vast, deep)
 * - 0.9-1.0: Infinite (impossible, endless, alien)
 * - Controls time, density, width, impossibility degree
 */
class ExpressiveMacroMapper final
{
public:
    /**
     * @brief Expressive macro inputs (all [0, 1] normalized)
     */
    struct MacroInputs
    {
        float character{0.5f};      // Subtle (0) → Extreme (1)
        float spaceType{0.2f};      // Chamber → Hall → Shimmer → Granular → Metallic
        float energy{0.1f};         // Decay → Sustain → Grow → Chaos
        float motion{0.2f};         // Still → Drift → Pulse → Random
        float color{0.5f};          // Dark → Balanced → Bright → Spectral
        float dimension{0.5f};      // Intimate → Room → Cathedral → Infinite
    };

    /**
     * @brief Output parameter targets computed from macros
     */
    struct ParameterTargets
    {
        // Primary parameters
        float time{0.55f};
        float mass{0.5f};
        float density{0.5f};
        float bloom{0.5f};
        float air{0.5f};
        float width{0.5f};
        float mix{0.5f};

        // Advanced parameters
        float warp{0.0f};
        float drift{0.0f};
        float gravity{0.5f};
        float pillarShape{0.5f};

        // Physical modeling parameters
        float tubeCount{0.545f};
        float radiusVariation{0.3f};
        float metallicResonance{0.5f};
        float couplingStrength{0.5f};
        float elasticity{0.5f};
        float recoveryTime{0.5f};
        float absorptionDrift{0.3f};
        float nonlinearity{0.3f};
        float impossibilityDegree{0.3f};
        float pitchEvolutionRate{0.3f};
        float paradoxResonanceFreq{0.5f};
        float paradoxGain{0.3f};

        // DSP routing control
        RoutingPresetType routingPreset{RoutingPresetType::TraditionalCathedral};
    };

    ExpressiveMacroMapper() = default;
    ~ExpressiveMacroMapper() = default;

    /**
     * @brief Compute parameter targets from expressive macros
     *
     * This is the core mapping function that translates high-level musical
     * intent into coordinated parameter sets.
     *
     * Key Mapping Rules:
     * - **Character** scales intensity of all effects (global multiplier)
     * - **Space Type** selects routing preset + module enables (discrete)
     * - **Energy** controls decay behavior exclusively (no conflicts)
     * - **Motion** controls modulation exclusively (no conflicts)
     * - **Color** controls spectral balance exclusively (no conflicts)
     * - **Dimension** controls size/time exclusively (no conflicts)
     *
     * @param macros Current macro positions
     * @return ParameterTargets Computed targets for all DSP parameters
     */
    ParameterTargets computeTargets(const MacroInputs& macros) const noexcept;

    /**
     * @brief Convenience overload with individual arguments
     */
    ParameterTargets computeTargets(
        float character,
        float spaceType,
        float energy,
        float motion,
        float color,
        float dimension) const noexcept;

private:
    // Character: Scales intensity of all effects (0=subtle, 1=extreme)
    float mapCharacterToIntensity(float character) const noexcept;
    float applyCharacterScaling(float baseValue, float character) const noexcept;

    // Space Type: Discrete mode selection + routing
    RoutingPresetType mapSpaceTypeToRouting(float spaceType) const noexcept;
    void applySpaceTypeModifiers(ParameterTargets& targets, float spaceType) const noexcept;

    // Energy: Decay behavior (decay/sustain/grow/chaos)
    float mapEnergyToFeedback(float energy) const noexcept;
    float mapEnergyToBloom(float energy) const noexcept;
    bool mapEnergyToFreeze(float energy) const noexcept;
    float mapEnergyToParadoxGain(float energy) const noexcept;

    // Motion: Temporal evolution (still/drift/pulse/random)
    float mapMotionToDrift(float motion) const noexcept;
    float mapMotionToWarp(float motion) const noexcept;
    float mapMotionToModulationDepth(float motion) const noexcept;

    // Color: Spectral character (dark/balanced/bright/spectral)
    float mapColorToMass(float color) const noexcept;
    float mapColorToAir(float color) const noexcept;
    float mapColorToGravity(float color) const noexcept;
    float mapColorToMetallicResonance(float color) const noexcept;

    // Dimension: Space size (intimate/room/cathedral/infinite)
    float mapDimensionToTime(float dimension) const noexcept;
    float mapDimensionToDensity(float dimension) const noexcept;
    float mapDimensionToWidth(float dimension) const noexcept;
    float mapDimensionToImpossibility(float dimension) const noexcept;

    // Utility: Piecewise linear interpolation for discrete mode transitions
    float piecewiseLinear(float input,
                          const std::vector<float>& breakpoints,
                          const std::vector<float>& values) const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ExpressiveMacroMapper)
};

} // namespace dsp
} // namespace monument
