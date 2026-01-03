#pragma once

#include <JuceHeader.h>

namespace monument
{
namespace dsp
{

/**
 * @brief MacroMapper converts high-level macro controls into coordinated parameter sets.
 *
 * The macro system provides 6 intuitive, musically-meaningful controls that map to
 * multiple underlying reverb parameters in coordinated ways. This creates complex,
 * coherent parameter mutations from simple user input.
 *
 * All macro inputs are normalized [0, 1] following Monument's parameter convention.
 */
class MacroMapper final
{
public:
    /**
     * @brief Output parameter values computed from macro positions.
     * All values are normalized [0, 1] and map to APVTS parameters.
     */
    struct ParameterTargets
    {
        // Primary parameters (7 controls)
        float time{0.55f};          // Tail duration / feedback gain
        float mass{0.5f};           // Weight and darkness / damping
        float density{0.5f};        // Reflection complexity / diffusion
        float bloom{0.5f};          // Late swell / envelope shape
        float air{0.5f};            // Upper-band lift / brightness
        float width{0.5f};          // Stereo spread (wet only)
        float mix{0.5f};            // Wet/dry blend (0-1 for internal use)

        // Advanced parameters (5 controls)
        float warp{0.0f};           // Space topology bend / matrix morph
        float drift{0.0f};          // Micro motion / delay modulation
        float gravity{0.5f};        // Spectral tilt / frequency decay
        float pillarShape{0.5f};    // Early reflection spacing
        // pillarMode is discrete (not continuous), handled separately

        // Future physical modeling parameters (placeholders)
        float tubeCount{0.0f};
        float metallicResonance{0.0f};
        float elasticity{0.0f};
        float impossibilityDegree{0.0f};
    };

    /**
     * @brief Macro control inputs (0-1 normalized).
     */
    struct MacroInputs
    {
        float material{0.5f};        // 0 = soft material → 1 = hard material
        float topology{0.5f};        // 0 = regular room → 1 = non-Euclidean space
        float viscosity{0.5f};       // 0 = airy → 1 = thick medium
        float evolution{0.5f};       // 0 = static → 1 = blooming/changing
        float chaosIntensity{0.0f};  // 0 = stable → 1 = chaotic
        float elasticityDecay{0.0f}; // 0 = instant recovery → 1 = slow deformation
    };

    MacroMapper() = default;
    ~MacroMapper() = default;

    /**
     * @brief Compute parameter targets from current macro positions.
     *
     * This function maps the 6 high-level macro controls to all underlying
     * reverb parameters. The mappings are designed to be musically coherent:
     * - Material affects both mass (damping) and density (diffusion)
     * - Topology drives warp (matrix morphing) and drift (micro-motion)
     * - Viscosity influences time (feedback) and air (brightness)
     * - Evolution controls bloom (envelope) and modulation depth
     * - Chaos adds controlled instability to warp and drift
     * - Elasticity enables future physical modeling features
     *
     * @param macros Current macro control values (all [0, 1])
     * @return ParameterTargets Computed target values for all parameters
     */
    ParameterTargets computeTargets(const MacroInputs& macros) const noexcept;

    /**
     * @brief Compute targets with individual macro arguments (convenience overload).
     */
    ParameterTargets computeTargets(
        float material,
        float topology,
        float viscosity,
        float evolution,
        float chaosIntensity,
        float elasticityDecay) const noexcept;

private:
    // Mapping functions: macro [0, 1] → parameter [0, 1]
    // These define the musical relationships between macros and parameters.

    float mapMaterialToTime(float material) const noexcept;
    float mapMaterialToMass(float material) const noexcept;
    float mapMaterialToDensity(float material) const noexcept;

    float mapTopologyToWarp(float topology) const noexcept;
    float mapTopologyToDrift(float topology) const noexcept;

    float mapViscosityToTime(float viscosity) const noexcept;
    float mapViscosityToAir(float viscosity) const noexcept;
    float mapViscosityToMass(float viscosity) const noexcept;

    float mapEvolutionToBloom(float evolution) const noexcept;
    float mapEvolutionToDrift(float evolution) const noexcept;

    float mapChaosToWarp(float chaos) const noexcept;
    float mapChaosToDrift(float chaos) const noexcept;

    // Utility: combine multiple macro influences on a single parameter
    float combineInfluences(float base, float influence1, float influence2,
                           float weight1 = 0.5f, float weight2 = 0.5f) const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroMapper)
};

} // namespace dsp
} // namespace monument
