#include "MacroMapper.h"
#include <cmath>

namespace monument
{
namespace dsp
{

MacroMapper::ParameterTargets MacroMapper::computeTargets(const MacroInputs& macros) const noexcept
{
    return computeTargets(
        macros.stone,
        macros.labyrinth,
        macros.mist,
        macros.bloom,
        macros.tempest,
        macros.echo,
        macros.patina,
        macros.abyss,
        macros.corona,
        macros.breath
    );
}

MacroMapper::ParameterTargets MacroMapper::computeTargets(
    float stone,
    float labyrinth,
    float mist,
    float bloom,
    float tempest,
    float echo,
    float patina,
    float abyss,
    float corona,
    float breath) const noexcept
{
    // Sanitize inputs (defensive: host automation can send out-of-range values)
    stone = juce::jlimit(0.0f, 1.0f, stone);
    labyrinth = juce::jlimit(0.0f, 1.0f, labyrinth);
    mist = juce::jlimit(0.0f, 1.0f, mist);
    bloom = juce::jlimit(0.0f, 1.0f, bloom);
    tempest = juce::jlimit(0.0f, 1.0f, tempest);
    echo = juce::jlimit(0.0f, 1.0f, echo);
    patina = juce::jlimit(0.0f, 1.0f, patina);
    abyss = juce::jlimit(0.0f, 1.0f, abyss);
    corona = juce::jlimit(0.0f, 1.0f, corona);
    breath = juce::jlimit(0.0f, 1.0f, breath);

    ParameterTargets targets;

    // TIME: Influenced by stone (hard = longer tails), mist (thick = longer sustain), and abyss (depth)
    const float timeFromStone = mapStoneToTime(stone);
    const float timeFromMist = mapMistToTime(mist);
    const float timeFromAbyss = mapAbyssToTime(abyss);
    targets.time = combineInfluences(0.55f, timeFromStone, timeFromMist, timeFromAbyss, 0.5f, 0.3f, 0.2f);

    // MASS: Primary mapping from stone (hard = more damping = darker)
    // Secondary influence from mist (thick = more absorption)
    const float massFromStone = mapStoneToMass(stone);
    const float massFromMist = mapMistToMass(mist);
    targets.mass = combineInfluences(0.5f, massFromStone, massFromMist, 0.7f, 0.3f);

    // DENSITY: Influenced by stone (hard = more reflections) and patina (weathering)
    const float densityFromStone = mapStoneToDensity(stone);
    const float densityFromPatina = mapPatinaToDensity(patina);
    targets.density = combineInfluences(0.5f, densityFromStone, densityFromPatina, 0.6f, 0.4f);

    // BLOOM: Influenced by bloom macro, patina, corona, and breath
    const float bloomFromBloom = mapBloomToBloom(bloom);
    const float bloomFromPatina = mapPatinaToBloom(patina);
    const float bloomFromCorona = mapCoronaToBloom(corona);
    const float bloomFromBreath = mapBreathToBloom(breath);
    targets.bloom = combineInfluences(0.5f, bloomFromBloom, bloomFromPatina, bloomFromCorona, 0.4f, 0.2f, 0.25f);
    targets.bloom = juce::jlimit(0.0f, 1.0f, targets.bloom + (bloomFromBreath - 0.5f) * 0.15f);

    // AIR: Inversely related to mist (airy = bright, thick = muffled), plus patina and corona
    const float airFromMist = mapMistToAir(mist);
    const float airFromPatina = mapPatinaToAir(patina);
    const float airFromCorona = mapCoronaToAir(corona);
    targets.air = combineInfluences(0.5f, airFromMist, airFromPatina, airFromCorona, 0.6f, 0.2f, 0.2f);

    // WIDTH: Influenced by abyss (infinite depth = wider space)
    const float widthFromAbyss = mapAbyssToWidth(abyss);
    targets.width = widthFromAbyss;

    // MIX: Not macro-controlled (always user-adjustable)
    targets.mix = 0.5f;  // Neutral default

    // WARP: Labyrinth is primary driver, tempest adds instability, corona adds shimmer
    const float warpFromLabyrinth = mapLabyrinthToWarp(labyrinth);
    const float warpFromTempest = mapTempestToWarp(tempest);
    const float warpFromCorona = mapCoronaToWarp(corona);
    targets.warp = combineInfluences(0.0f, warpFromLabyrinth, warpFromTempest, warpFromCorona, 0.65f, 0.25f, 0.1f);

    // DRIFT: Influenced by labyrinth, bloom, tempest, and breath
    const float driftFromLabyrinth = mapLabyrinthToDrift(labyrinth);
    const float driftFromBloom = mapBloomToDrift(bloom);
    const float driftFromTempest = mapTempestToDrift(tempest);
    const float driftFromBreath = mapBreathToDrift(breath);
    // Combine four influences: labyrinth is base, others add motion
    float driftBase = driftFromLabyrinth * 0.4f + driftFromBloom * 0.2f + driftFromTempest * 0.2f + driftFromBreath * 0.2f;
    targets.drift = juce::jlimit(0.0f, 1.0f, driftBase);

    // GRAVITY: Influenced by breath (living pulse)
    const float gravityFromBreath = mapBreathToGravity(breath);
    targets.gravity = gravityFromBreath;

    // PILLAR SHAPE: Not macro-controlled in Phase 1 (advanced user control)
    targets.pillarShape = 0.5f;  // Neutral default

    // ========================================================================
    // PHYSICAL MODELING PARAMETERS (Phase 5)
    // Macro-driven parameter mappings for physical simulation modules
    // ========================================================================

    // TubeRayTracer parameters
    // Labyrinth drives tube network complexity (twisted maze = more complex network)
    targets.tubeCount = juce::jmap(labyrinth, 0.3f, 0.8f);  // 0.3 = ~7 tubes, 0.8 = ~14 tubes

    // Stone drives tube uniformity (soft = varied, hard = uniform)
    targets.radiusVariation = juce::jmap(stone, 0.5f, 0.1f);  // Inverted: soft = more variation

    // Stone drives metallic resonance (hard = more metallic)
    targets.metallicResonance = juce::jmap(stone, 0.3f, 0.8f);

    // Labyrinth drives tube coupling (twisted maze = stronger coupling)
    targets.couplingStrength = juce::jmap(labyrinth, 0.3f, 0.7f);

    // ElasticHallway parameters
    // Echo macro directly controls wall elasticity
    targets.elasticity = echo;

    // Mist drives recovery time (thick = slower recovery)
    targets.recoveryTime = juce::jmap(mist, 0.3f, 0.8f);

    // Bloom drives absorption drift (overgrown = more drift)
    targets.absorptionDrift = juce::jmap(bloom, 0.1f, 0.6f);

    // Tempest drives wall nonlinearity (storm = more nonlinear response)
    targets.nonlinearity = juce::jmap(tempest, 0.1f, 0.6f);

    // AlienAmplification parameters
    // Tempest drives impossibility physics (storm = more alien behavior)
    targets.impossibilityDegree = juce::jmap(tempest, 0.1f, 0.7f);

    // Bloom drives pitch evolution (overgrown = more spectral morphing)
    targets.pitchEvolutionRate = juce::jmap(bloom, 0.1f, 0.6f);

    // Paradox resonance frequency: default to 432 Hz (0.5), subtle labyrinth influence
    targets.paradoxResonanceFreq = juce::jmap(labyrinth, 0.4f, 0.6f);  // Subtle range

    // Paradox gain: tempest drives amplification (storm = more gain)
    targets.paradoxGain = juce::jmap(tempest, 0.0f, 0.5f);

    return targets;
}

// ============================================================================
// STONE MAPPINGS (Foundation material)
// ============================================================================

float MacroMapper::mapStoneToTime(float stone) const noexcept
{
    // Soft limestone (0.0) absorbs energy → shorter tails
    // Hard granite (1.0) reflects energy → longer tails
    // Mapping: [0, 1] → [0.3, 0.8] (spans most of Time's useful range)
    return juce::jmap(stone, 0.3f, 0.8f);
}

float MacroMapper::mapStoneToMass(float stone) const noexcept
{
    // Soft limestone (0.0) = less damping = brighter
    // Hard granite (1.0) = more damping = darker
    // This creates the "hard surfaces sound darker" reverb convention
    // Mapping: [0, 1] → [0.2, 0.9]
    return juce::jmap(stone, 0.2f, 0.9f);
}

float MacroMapper::mapStoneToDensity(float stone) const noexcept
{
    // Hard stone = complex reflection patterns = higher density
    // Soft stone = absorption = lower density
    // Mapping: [0, 1] → [0.25, 0.95]
    return juce::jmap(stone, 0.25f, 0.95f);
}

// ============================================================================
// LABYRINTH MAPPINGS (Spatial complexity)
// ============================================================================

float MacroMapper::mapLabyrinthToWarp(float labyrinth) const noexcept
{
    // Simple hall (0.0) = Hadamard matrix (orthogonal, predictable)
    // Twisted maze (1.0) = Householder matrix (dense, complex)
    // Warp morphs between these spatial topologies
    // Mapping: direct (labyrinth complexity maps to warp)
    return labyrinth;
}

float MacroMapper::mapLabyrinthToDrift(float labyrinth) const noexcept
{
    // Twisted mazes have subtle geometry shifts
    // Simple halls are stable
    // Mapping: [0, 1] → [0.0, 0.4] (drift is subtle even in complex mazes)
    return juce::jmap(labyrinth, 0.0f, 0.4f);
}

// ============================================================================
// MIST MAPPINGS (Atmospheric density)
// ============================================================================

float MacroMapper::mapMistToTime(float mist) const noexcept
{
    // Clear air (0.0) = sound travels freely → can sustain longer
    // Dense fog (1.0) = medium resists → shorter effective tail
    // Mapping: [0, 1] → [0.6, 0.4] (inverse relationship)
    return juce::jmap(mist, 0.6f, 0.4f);
}

float MacroMapper::mapMistToAir(float mist) const noexcept
{
    // Clear air (0.0) = bright, open high frequencies
    // Dense fog (1.0) = muffled, rolled-off highs
    // Mapping: [0, 1] → [0.8, 0.2] (strong inverse relationship)
    return juce::jmap(mist, 0.8f, 0.2f);
}

float MacroMapper::mapMistToMass(float mist) const noexcept
{
    // Dense fog absorbs more energy = more damping
    // Mapping: [0, 1] → [0.0, 0.3] (secondary influence on mass)
    return juce::jmap(mist, 0.0f, 0.3f);
}

// ============================================================================
// BLOOM MAPPINGS (Organic growth)
// ============================================================================

float MacroMapper::mapBloomToBloom(float bloom) const noexcept
{
    // Barren (0.0) = no envelope shaping
    // Overgrown (1.0) = strong bloom (late-field swell)
    // Mapping: direct (bloom macro maps 1:1 to bloom parameter)
    return bloom;
}

float MacroMapper::mapBloomToDrift(float bloom) const noexcept
{
    // Overgrown spaces have subtle organic motion
    // Barren spaces are static
    // Mapping: [0, 1] → [0.0, 0.35] (subtle even at full bloom)
    return juce::jmap(bloom, 0.0f, 0.35f);
}

// ============================================================================
// TEMPEST MAPPINGS (Storm chaos)
// ============================================================================

float MacroMapper::mapTempestToWarp(float tempest) const noexcept
{
    // Storm adds unpredictable topology shifts
    // Mapping: [0, 1] → [0.0, 0.3] (tempest is additive, not dominant)
    return juce::jmap(tempest, 0.0f, 0.3f);
}

float MacroMapper::mapTempestToDrift(float tempest) const noexcept
{
    // Storm creates erratic micro-motion
    // Mapping: [0, 1] → [0.0, 0.5] (stronger influence than warp)
    return juce::jmap(tempest, 0.0f, 0.5f);
}

// ============================================================================
// PATINA MAPPINGS (Surface weathering)
// ============================================================================

float MacroMapper::mapPatinaToDensity(float patina) const noexcept
{
    // Pristine (0.0) = smooth reflections = moderate density
    // Weathered (1.0) = rough texture = higher density (scattered reflections)
    // Mapping: [0, 1] → [0.4, 0.85] (weathering increases reflection complexity)
    return juce::jmap(patina, 0.4f, 0.85f);
}

float MacroMapper::mapPatinaToAir(float patina) const noexcept
{
    // Pristine (0.0) = bright, clear highs
    // Weathered (1.0) = duller highs (surface absorption)
    // Mapping: [0, 1] → [0.7, 0.3] (inverse - weathering reduces high frequencies)
    return juce::jmap(patina, 0.7f, 0.3f);
}

float MacroMapper::mapPatinaToBloom(float patina) const noexcept
{
    // Pristine (0.0) = clean envelope
    // Weathered (1.0) = subtle bloom from texture irregularities
    // Mapping: [0, 1] → [0.0, 0.3] (weathering adds subtle bloom)
    return juce::jmap(patina, 0.0f, 0.3f);
}

// ============================================================================
// ABYSS MAPPINGS (Infinite depth)
// ============================================================================

float MacroMapper::mapAbyssToSize(float abyss) const noexcept
{
    // Shallow (0.0) = small space
    // Infinite void (1.0) = vast space
    // Note: Size is not in ParameterTargets yet, using width as proxy
    // Mapping: [0, 1] → [0.3, 1.0] (abyss dramatically increases perceived size)
    return juce::jmap(abyss, 0.3f, 1.0f);
}

float MacroMapper::mapAbyssToTime(float abyss) const noexcept
{
    // Shallow (0.0) = shorter tail
    // Infinite void (1.0) = endless tail
    // Mapping: [0, 1] → [0.4, 0.9] (abyss extends decay time)
    return juce::jmap(abyss, 0.4f, 0.9f);
}

float MacroMapper::mapAbyssToWidth(float abyss) const noexcept
{
    // Shallow (0.0) = narrow stereo
    // Infinite void (1.0) = maximum width
    // Mapping: [0, 1] → [0.3, 0.95] (depth creates spatial width)
    return juce::jmap(abyss, 0.3f, 0.95f);
}

// ============================================================================
// CORONA MAPPINGS (Sacred radiance)
// ============================================================================

float MacroMapper::mapCoronaToBloom(float corona) const noexcept
{
    // Shadow (0.0) = no bloom
    // Sacred halo (1.0) = strong radiant bloom
    // Mapping: [0, 1] → [0.0, 0.8] (corona strongly affects bloom)
    return juce::jmap(corona, 0.0f, 0.8f);
}

float MacroMapper::mapCoronaToAir(float corona) const noexcept
{
    // Shadow (0.0) = neutral air
    // Sacred halo (1.0) = bright, shimmering highs
    // Mapping: [0, 1] → [0.3, 0.85] (corona adds brilliance)
    return juce::jmap(corona, 0.3f, 0.85f);
}

float MacroMapper::mapCoronaToWarp(float corona) const noexcept
{
    // Shadow (0.0) = stable space
    // Sacred halo (1.0) = subtle shimmer (light bending)
    // Mapping: [0, 1] → [0.0, 0.25] (corona adds subtle warp)
    return juce::jmap(corona, 0.0f, 0.25f);
}

// ============================================================================
// BREATH MAPPINGS (Living pulse)
// ============================================================================

float MacroMapper::mapBreathToBloom(float breath) const noexcept
{
    // Dormant (0.0) = static
    // Living pulse (1.0) = rhythmic bloom
    // Mapping: [0, 1] → [0.0, 0.5] (breath adds organic bloom)
    return juce::jmap(breath, 0.0f, 0.5f);
}

float MacroMapper::mapBreathToDrift(float breath) const noexcept
{
    // Dormant (0.0) = stable
    // Living pulse (1.0) = rhythmic motion
    // Mapping: [0, 1] → [0.0, 0.6] (breath creates pulsing drift)
    return juce::jmap(breath, 0.0f, 0.6f);
}

float MacroMapper::mapBreathToGravity(float breath) const noexcept
{
    // Dormant (0.0) = neutral gravity
    // Living pulse (1.0) = rhythmic gravity shifts
    // Mapping: [0, 1] → [0.3, 0.7] (breath modulates spectral balance)
    return juce::jmap(breath, 0.3f, 0.7f);
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

float MacroMapper::combineInfluences(
    float base,
    float influence1,
    float influence2,
    float influence3,
    float weight1,
    float weight2,
    float weight3) const noexcept
{
    // Weighted sum of three influences, starting from base value
    // Weights should sum to 1.0 for proper blending
    const float totalWeight = weight1 + weight2 + weight3;
    const float normalizedWeight1 = weight1 / totalWeight;
    const float normalizedWeight2 = weight2 / totalWeight;
    const float normalizedWeight3 = weight3 / totalWeight;

    const float combined = base + (influence1 - base) * normalizedWeight1
                                + (influence2 - base) * normalizedWeight2
                                + (influence3 - base) * normalizedWeight3;

    return juce::jlimit(0.0f, 1.0f, combined);
}

} // namespace dsp
} // namespace monument
