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

        // Physical modeling parameters (Phase 5)
        float tubeCount{0.545f};              // Tube count (5-16 tubes)
        float radiusVariation{0.3f};          // Tube diameter variation
        float metallicResonance{0.5f};        // Metallic resonance emphasis
        float couplingStrength{0.5f};         // Tube coupling strength
        float elasticity{0.5f};               // Wall elasticity
        float recoveryTime{0.5f};             // Wall recovery time
        float absorptionDrift{0.3f};          // Absorption drift amount
        float nonlinearity{0.3f};             // Wall nonlinearity
        float impossibilityDegree{0.3f};      // Alien physics intensity
        float pitchEvolutionRate{0.3f};       // Pitch morphing speed
        float paradoxResonanceFreq{0.5f};     // Paradox resonance frequency
        float paradoxGain{0.3f};              // Paradox amplification gain
    };

    /**
     * @brief Macro control inputs (0-1 normalized).
     * Ancient Monuments theme: Poetic architectural and temporal aesthetic 🗿
     */
    struct MacroInputs
    {
        // Ancient Monuments - Core 6 macros (Phase 1-5)
        float stone{0.5f};           // STONE: 0 = soft limestone → 1 = hard granite
        float labyrinth{0.5f};       // LABYRINTH: 0 = simple hall → 1 = twisted maze
        float mist{0.5f};            // MIST: 0 = clear air → 1 = dense fog
        float bloom{0.5f};           // BLOOM: 0 = barren → 1 = overgrown
        float tempest{0.0f};         // TEMPEST: 0 = calm → 1 = storm
        float echo{0.0f};            // ECHO: 0 = instant → 1 = resonating memory

        // Ancient Monuments - Expanded 4 macros (Phase 5+)
        float patina{0.5f};          // PATINA: 0 = pristine → 1 = weathered
        float abyss{0.5f};           // ABYSS: 0 = shallow → 1 = infinite void
        float corona{0.5f};          // CORONA: 0 = shadow → 1 = sacred halo
        float breath{0.0f};          // BREATH: 0 = dormant → 1 = living pulse

        // Backward compatibility aliases (deprecated, use new names)
        [[deprecated("Use 'stone' instead")]] float& material = stone;
        [[deprecated("Use 'labyrinth' instead")]] float& topology = labyrinth;
        [[deprecated("Use 'mist' instead")]] float& viscosity = mist;
        [[deprecated("Use 'bloom' instead (no change)")]] float& evolution = bloom;
        [[deprecated("Use 'tempest' instead")]] float& chaosIntensity = tempest;
        [[deprecated("Use 'echo' instead")]] float& elasticityDecay = echo;
    };

    MacroMapper() = default;
    ~MacroMapper() = default;

    /**
     * @brief Compute parameter targets from current macro positions.
     *
     * This function maps the 10 Ancient Monuments macro controls to all underlying
     * reverb parameters. The mappings are designed to be musically coherent and
     * evoke the weathering of ancient architectural structures over time.
     *
     * Ancient Monuments - Core 6 macros:
     * - STONE: Affects mass (damping) and density (diffusion) - material hardness
     * - LABYRINTH: Drives warp (matrix morphing) and drift (spatial complexity)
     * - MIST: Influences time (feedback) and air (atmospheric density)
     * - BLOOM: Controls bloom (envelope) and growth over time
     * - TEMPEST: Adds controlled chaos to warp and drift (storm intensity)
     * - ECHO: Enables resonating memory (elastic temporal response)
     *
     * Ancient Monuments - Expanded 4 macros:
     * - PATINA: Controls reflection texture weathering (density, air, bloom)
     * - ABYSS: Drives infinite spatial depth (size, time, width)
     * - CORONA: Sacred radiant shimmer (bloom, air, warp)
     * - BREATH: Living rhythmic pulse (evolution, drift, gravity)
     *
     * @param macros Current macro control values (all [0, 1])
     * @return ParameterTargets Computed target values for all parameters
     */
    ParameterTargets computeTargets(const MacroInputs& macros) const noexcept;

    /**
     * @brief Compute targets with individual Ancient Monuments macro arguments.
     * @param stone Foundation material (soft limestone → hard granite)
     * @param labyrinth Spatial complexity (simple hall → twisted maze)
     * @param mist Atmospheric density (clear air → dense fog)
     * @param bloom Organic growth (barren → overgrown)
     * @param tempest Storm intensity (calm → raging)
     * @param echo Resonating memory (instant → lingering)
     * @param patina Surface weathering (pristine → aged)
     * @param abyss Spatial depth (shallow → infinite void)
     * @param corona Radiant halo (shadow → sacred light)
     * @param breath Living pulse (dormant → breathing)
     */
    ParameterTargets computeTargets(
        float stone,
        float labyrinth,
        float mist,
        float bloom,
        float tempest,
        float echo,
        float patina,
        float abyss,
        float corona,
        float breath) const noexcept;

private:
    // Ancient Monuments macro mapping functions: macro [0, 1] → parameter [0, 1]
    // These define the musical relationships between macros and parameters.

    // STONE mappings (foundation material)
    float mapStoneToTime(float stone) const noexcept;
    float mapStoneToMass(float stone) const noexcept;
    float mapStoneToDensity(float stone) const noexcept;

    // LABYRINTH mappings (spatial complexity)
    float mapLabyrinthToWarp(float labyrinth) const noexcept;
    float mapLabyrinthToDrift(float labyrinth) const noexcept;

    // MIST mappings (atmospheric density)
    float mapMistToTime(float mist) const noexcept;
    float mapMistToAir(float mist) const noexcept;
    float mapMistToMass(float mist) const noexcept;

    // BLOOM mappings (organic growth)
    float mapBloomToBloom(float bloom) const noexcept;
    float mapBloomToDrift(float bloom) const noexcept;

    // TEMPEST mappings (storm chaos)
    float mapTempestToWarp(float tempest) const noexcept;
    float mapTempestToDrift(float tempest) const noexcept;

    // PATINA mappings (surface weathering)
    float mapPatinaToDensity(float patina) const noexcept;
    float mapPatinaToAir(float patina) const noexcept;
    float mapPatinaToBloom(float patina) const noexcept;

    // ABYSS mappings (infinite depth)
    float mapAbyssToSize(float abyss) const noexcept;
    float mapAbyssToTime(float abyss) const noexcept;
    float mapAbyssToWidth(float abyss) const noexcept;

    // CORONA mappings (sacred radiance)
    float mapCoronaToBloom(float corona) const noexcept;
    float mapCoronaToAir(float corona) const noexcept;
    float mapCoronaToWarp(float corona) const noexcept;

    // BREATH mappings (living pulse)
    float mapBreathToBloom(float breath) const noexcept;
    float mapBreathToDrift(float breath) const noexcept;
    float mapBreathToGravity(float breath) const noexcept;

    // Utility: combine multiple macro influences on a single parameter
    float combineInfluences(float base, float influence1, float influence2,
                           float weight1 = 0.5f, float weight2 = 0.5f) const noexcept;
    float combineInfluences(float base, float inf1, float inf2, float inf3,
                           float w1, float w2, float w3) const noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MacroMapper)
};

} // namespace dsp
} // namespace monument
