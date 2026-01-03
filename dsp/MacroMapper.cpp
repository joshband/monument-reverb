#include "MacroMapper.h"
#include <cmath>

namespace monument
{
namespace dsp
{

MacroMapper::ParameterTargets MacroMapper::computeTargets(const MacroInputs& macros) const noexcept
{
    return computeTargets(
        macros.material,
        macros.topology,
        macros.viscosity,
        macros.evolution,
        macros.chaosIntensity,
        macros.elasticityDecay
    );
}

MacroMapper::ParameterTargets MacroMapper::computeTargets(
    float material,
    float topology,
    float viscosity,
    float evolution,
    float chaosIntensity,
    float elasticityDecay) const noexcept
{
    // Sanitize inputs (defensive: host automation can send out-of-range values)
    material = juce::jlimit(0.0f, 1.0f, material);
    topology = juce::jlimit(0.0f, 1.0f, topology);
    viscosity = juce::jlimit(0.0f, 1.0f, viscosity);
    evolution = juce::jlimit(0.0f, 1.0f, evolution);
    chaosIntensity = juce::jlimit(0.0f, 1.0f, chaosIntensity);
    elasticityDecay = juce::jlimit(0.0f, 1.0f, elasticityDecay);

    ParameterTargets targets;

    // TIME: Influenced by material (hard = longer tails) and viscosity (thick = longer sustain)
    const float timeFromMaterial = mapMaterialToTime(material);
    const float timeFromViscosity = mapViscosityToTime(viscosity);
    targets.time = combineInfluences(0.55f, timeFromMaterial, timeFromViscosity, 0.6f, 0.4f);

    // MASS: Primary mapping from material (hard = more damping = darker)
    // Secondary influence from viscosity (thick = more absorption)
    const float massFromMaterial = mapMaterialToMass(material);
    const float massFromViscosity = mapViscosityToMass(viscosity);
    targets.mass = combineInfluences(0.5f, massFromMaterial, massFromViscosity, 0.7f, 0.3f);

    // DENSITY: Primarily material-driven (hard surfaces = more reflections)
    targets.density = mapMaterialToDensity(material);

    // BLOOM: Entirely evolution-driven (static = no bloom, evolving = strong bloom)
    targets.bloom = mapEvolutionToBloom(evolution);

    // AIR: Inversely related to viscosity (airy = bright, thick = muffled)
    targets.air = mapViscosityToAir(viscosity);

    // WIDTH: Not macro-controlled in Phase 1 (remains user-adjustable)
    // Could be influenced by topology in future (non-Euclidean = wider?)
    targets.width = 0.5f;  // Neutral default

    // MIX: Not macro-controlled (always user-adjustable)
    targets.mix = 0.5f;  // Neutral default

    // WARP: Topology is primary driver, chaos adds instability
    const float warpFromTopology = mapTopologyToWarp(topology);
    const float warpFromChaos = mapChaosToWarp(chaosIntensity);
    targets.warp = combineInfluences(0.0f, warpFromTopology, warpFromChaos, 0.75f, 0.25f);

    // DRIFT: Influenced by topology, evolution, and chaos
    const float driftFromTopology = mapTopologyToDrift(topology);
    const float driftFromEvolution = mapEvolutionToDrift(evolution);
    const float driftFromChaos = mapChaosToDrift(chaosIntensity);
    // Combine three influences: topology is base, evolution and chaos add motion
    float driftBase = driftFromTopology * 0.5f + driftFromEvolution * 0.3f + driftFromChaos * 0.2f;
    targets.drift = juce::jlimit(0.0f, 1.0f, driftBase);

    // GRAVITY: Not macro-controlled in Phase 1 (advanced user control)
    targets.gravity = 0.5f;  // Neutral default

    // PILLAR SHAPE: Not macro-controlled in Phase 1 (advanced user control)
    targets.pillarShape = 0.5f;  // Neutral default

    // PHYSICAL MODELING: Placeholder mappings for Phase 3
    // ElasticityDecay macro will drive these once physical modules are implemented
    targets.elasticity = elasticityDecay;
    targets.tubeCount = 0.0f;  // Future: could map to topology
    targets.metallicResonance = 0.0f;  // Future: could map to material
    targets.impossibilityDegree = 0.0f;  // Future: could map to chaos

    return targets;
}

// ============================================================================
// MATERIAL MAPPINGS
// ============================================================================

float MacroMapper::mapMaterialToTime(float material) const noexcept
{
    // Soft materials (0.0) absorb energy → shorter tails
    // Hard materials (1.0) reflect energy → longer tails
    // Mapping: [0, 1] → [0.3, 0.8] (spans most of Time's useful range)
    return juce::jmap(material, 0.3f, 0.8f);
}

float MacroMapper::mapMaterialToMass(float material) const noexcept
{
    // Soft materials (0.0) = less damping = brighter
    // Hard materials (1.0) = more damping = darker
    // This creates the "hard surfaces sound darker" reverb convention
    // Mapping: [0, 1] → [0.2, 0.9]
    return juce::jmap(material, 0.2f, 0.9f);
}

float MacroMapper::mapMaterialToDensity(float material) const noexcept
{
    // Hard materials = complex reflection patterns = higher density
    // Soft materials = absorption = lower density
    // Mapping: [0, 1] → [0.25, 0.95]
    return juce::jmap(material, 0.25f, 0.95f);
}

// ============================================================================
// TOPOLOGY MAPPINGS
// ============================================================================

float MacroMapper::mapTopologyToWarp(float topology) const noexcept
{
    // Regular room (0.0) = Hadamard matrix (orthogonal, predictable)
    // Non-Euclidean (1.0) = Householder matrix (dense, complex)
    // Warp morphs between these topologies
    // Mapping: direct (topology is conceptually equivalent to warp)
    return topology;
}

float MacroMapper::mapTopologyToDrift(float topology) const noexcept
{
    // Non-Euclidean spaces have subtle geometry shifts
    // Regular rooms are stable
    // Mapping: [0, 1] → [0.0, 0.4] (drift is subtle even at max topology)
    return juce::jmap(topology, 0.0f, 0.4f);
}

// ============================================================================
// VISCOSITY MAPPINGS
// ============================================================================

float MacroMapper::mapViscosityToTime(float viscosity) const noexcept
{
    // Airy (0.0) = sound travels freely → can sustain longer
    // Thick (1.0) = medium resists → shorter effective tail
    // Mapping: [0, 1] → [0.6, 0.4] (inverse relationship)
    return juce::jmap(viscosity, 0.6f, 0.4f);
}

float MacroMapper::mapViscosityToAir(float viscosity) const noexcept
{
    // Airy (0.0) = bright, open high frequencies
    // Thick (1.0) = muffled, rolled-off highs
    // Mapping: [0, 1] → [0.8, 0.2] (strong inverse relationship)
    return juce::jmap(viscosity, 0.8f, 0.2f);
}

float MacroMapper::mapViscosityToMass(float viscosity) const noexcept
{
    // Thick mediums absorb more energy = more damping
    // Mapping: [0, 1] → [0.0, 0.3] (secondary influence on mass)
    return juce::jmap(viscosity, 0.0f, 0.3f);
}

// ============================================================================
// EVOLUTION MAPPINGS
// ============================================================================

float MacroMapper::mapEvolutionToBloom(float evolution) const noexcept
{
    // Static (0.0) = no envelope shaping
    // Evolving (1.0) = strong bloom (late-field swell)
    // Mapping: direct (evolution concept maps 1:1 to bloom)
    return evolution;
}

float MacroMapper::mapEvolutionToDrift(float evolution) const noexcept
{
    // Evolving spaces have subtle micro-motion
    // Static spaces are frozen in time
    // Mapping: [0, 1] → [0.0, 0.35] (subtle even at max evolution)
    return juce::jmap(evolution, 0.0f, 0.35f);
}

// ============================================================================
// CHAOS MAPPINGS
// ============================================================================

float MacroMapper::mapChaosToWarp(float chaos) const noexcept
{
    // Chaos adds unpredictable topology shifts
    // Mapping: [0, 1] → [0.0, 0.3] (chaos is additive, not dominant)
    return juce::jmap(chaos, 0.0f, 0.3f);
}

float MacroMapper::mapChaosToDrift(float chaos) const noexcept
{
    // Chaos creates erratic micro-motion
    // Mapping: [0, 1] → [0.0, 0.5] (stronger influence than warp)
    return juce::jmap(chaos, 0.0f, 0.5f);
}

// ============================================================================
// UTILITY
// ============================================================================

float MacroMapper::combineInfluences(
    float base,
    float influence1,
    float influence2,
    float weight1,
    float weight2) const noexcept
{
    // Weighted sum of influences, starting from base value
    // Weights should sum to 1.0 for proper blending
    const float totalWeight = weight1 + weight2;
    const float normalizedWeight1 = weight1 / totalWeight;
    const float normalizedWeight2 = weight2 / totalWeight;

    const float combined = base + (influence1 - base) * normalizedWeight1
                                + (influence2 - base) * normalizedWeight2;

    return juce::jlimit(0.0f, 1.0f, combined);
}

} // namespace dsp
} // namespace monument
