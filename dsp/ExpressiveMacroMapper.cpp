#include "ExpressiveMacroMapper.h"
#include <cmath>

namespace monument
{
namespace dsp
{

//==============================================================================
// Main Compute Function
//==============================================================================

ExpressiveMacroMapper::ParameterTargets ExpressiveMacroMapper::computeTargets(
    const MacroInputs& macros) const noexcept
{
    return computeTargets(
        macros.character,
        macros.spaceType,
        macros.energy,
        macros.motion,
        macros.color,
        macros.dimension
    );
}

ExpressiveMacroMapper::ParameterTargets ExpressiveMacroMapper::computeTargets(
    float character,
    float spaceType,
    float energy,
    float motion,
    float color,
    float dimension) const noexcept
{
    // Sanitize inputs
    character = juce::jlimit(0.0f, 1.0f, character);
    spaceType = juce::jlimit(0.0f, 1.0f, spaceType);
    energy = juce::jlimit(0.0f, 1.0f, energy);
    motion = juce::jlimit(0.0f, 1.0f, motion);
    color = juce::jlimit(0.0f, 1.0f, color);
    dimension = juce::jlimit(0.0f, 1.0f, dimension);

    ParameterTargets targets;

    // 1. SELECT ROUTING PRESET BASED ON SPACE TYPE
    targets.routingPreset = mapSpaceTypeToRouting(spaceType);

    // 2. DIMENSION: Space size (controls time, density, width, impossibility)
    targets.time = mapDimensionToTime(dimension);
    targets.density = mapDimensionToDensity(dimension);
    targets.width = mapDimensionToWidth(dimension);
    targets.impossibilityDegree = mapDimensionToImpossibility(dimension);

    // 3. COLOR: Spectral character (controls mass, air, gravity, metallic resonance)
    targets.mass = mapColorToMass(color);
    targets.air = mapColorToAir(color);
    targets.gravity = mapColorToGravity(color);
    targets.metallicResonance = mapColorToMetallicResonance(color);

    // 4. ENERGY: Decay behavior (controls bloom, feedback via freeze behavior)
    targets.bloom = mapEnergyToBloom(energy);
    targets.paradoxGain = mapEnergyToParadoxGain(energy);
    // Note: Freeze is boolean, handled separately in processor

    // 5. MOTION: Temporal evolution (controls drift, warp, modulation depth)
    targets.drift = mapMotionToDrift(motion);
    targets.warp = mapMotionToWarp(motion);

    // 6. SPACE TYPE MODIFIERS: Fine-tune parameters per routing type
    applySpaceTypeModifiers(targets, spaceType);

    // 7. CHARACTER: Global intensity scaling (applied LAST to scale everything)
    const float intensityScale = mapCharacterToIntensity(character);
    targets.warp = applyCharacterScaling(targets.warp, intensityScale);
    targets.drift = applyCharacterScaling(targets.drift, intensityScale);
    targets.bloom = applyCharacterScaling(targets.bloom, intensityScale);
    targets.paradoxGain = applyCharacterScaling(targets.paradoxGain, intensityScale);

    // Set remaining parameters with safe defaults
    targets.mix = 1.0f;  // Always 100% wet in internal routing
    targets.pillarShape = 0.5f;  // Neutral early reflection spacing

    // Physical modeling defaults (can be modulated by Space Type)
    targets.tubeCount = 0.545f;
    targets.radiusVariation = 0.3f;
    targets.couplingStrength = 0.5f;
    targets.elasticity = 0.5f;
    targets.recoveryTime = 0.5f;
    targets.absorptionDrift = 0.3f;
    targets.nonlinearity = 0.3f;
    targets.pitchEvolutionRate = 0.3f;
    targets.paradoxResonanceFreq = 0.5f;

    return targets;
}

//==============================================================================
// Character Mappings (Global Intensity Scaling)
//==============================================================================

float ExpressiveMacroMapper::mapCharacterToIntensity(float character) const noexcept
{
    // 0.0 = subtle (0.5x intensity), 0.5 = neutral (1.0x), 1.0 = extreme (2.0x)
    return 0.5f + character * 1.5f;
}

float ExpressiveMacroMapper::applyCharacterScaling(float baseValue, float character) const noexcept
{
    // Scale effect intensity while keeping parameters in valid range
    const float centered = (baseValue - 0.5f) * 2.0f;  // Map to [-1, 1]
    const float scaled = centered * character;          // Scale by character
    return juce::jlimit(0.0f, 1.0f, 0.5f + scaled * 0.5f);  // Map back to [0, 1]
}

//==============================================================================
// Space Type Mappings (Discrete Modes + Routing Selection)
//==============================================================================

RoutingPresetType ExpressiveMacroMapper::mapSpaceTypeToRouting(float spaceType) const noexcept
{
    // Discrete routing selection with soft transitions
    // 0.0-0.2: Chamber → TraditionalCathedral
    // 0.2-0.4: Hall → TraditionalCathedral (larger)
    // 0.4-0.6: Shimmer → ShimmerInfinity
    // 0.6-0.8: Granular → ParallelWorlds
    // 0.8-1.0: Metallic → MetallicGranular

    if (spaceType < 0.2f)
        return RoutingPresetType::TraditionalCathedral;
    else if (spaceType < 0.4f)
        return RoutingPresetType::TraditionalCathedral;
    else if (spaceType < 0.6f)
        return RoutingPresetType::ShimmerInfinity;
    else if (spaceType < 0.8f)
        return RoutingPresetType::ParallelWorlds;
    else
        return RoutingPresetType::MetallicGranular;
}

void ExpressiveMacroMapper::applySpaceTypeModifiers(
    ParameterTargets& targets,
    float spaceType) const noexcept
{
    // Fine-tune parameters based on space type character
    // Each mode has subtle adjustments to emphasize its sonic character

    if (spaceType < 0.2f)  // Chamber
    {
        targets.density *= 0.7f;  // Less diffuse
        targets.time *= 0.6f;     // Shorter decay
    }
    else if (spaceType < 0.4f)  // Hall
    {
        targets.density *= 1.1f;  // More diffuse
        targets.bloom *= 1.2f;    // Enhanced swell
    }
    else if (spaceType < 0.6f)  // Shimmer
    {
        targets.air *= 1.3f;      // Brighter
        targets.pitchEvolutionRate = 0.6f;  // Enable pitch shifting
        targets.paradoxGain *= 1.4f;  // Enhance shimmer
    }
    else if (spaceType < 0.8f)  // Granular
    {
        targets.warp *= 1.4f;     // More spatial warping
        targets.nonlinearity = 0.6f;  // Textured processing
    }
    else  // Metallic (0.8-1.0)
    {
        targets.metallicResonance *= 1.5f;  // Enhance tube resonances
        targets.couplingStrength = 0.7f;    // Stronger coupling
        targets.tubeCount = 0.8f;           // More tubes
    }
}

//==============================================================================
// Energy Mappings (Decay Behavior)
//==============================================================================

float ExpressiveMacroMapper::mapEnergyToFeedback(float energy) const noexcept
{
    // 0.0-0.2: Decay (low feedback)
    // 0.3-0.5: Sustain (medium feedback)
    // 0.6-0.8: Grow (high feedback + bloom)
    // 0.9-1.0: Chaos (very high feedback + paradox)

    std::vector<float> breakpoints{0.0f, 0.2f, 0.5f, 0.8f, 1.0f};
    std::vector<float> values{0.3f, 0.5f, 0.7f, 0.85f, 0.95f};
    return piecewiseLinear(energy, breakpoints, values);
}

float ExpressiveMacroMapper::mapEnergyToBloom(float energy) const noexcept
{
    // Bloom increases dramatically in "Grow" and "Chaos" modes
    if (energy < 0.5f)
        return juce::jmap(energy, 0.0f, 0.5f, 0.2f, 0.4f);  // Low bloom
    else if (energy < 0.8f)
        return juce::jmap(energy, 0.5f, 0.8f, 0.6f, 0.9f);  // Growing bloom
    else
        return juce::jmap(energy, 0.8f, 1.0f, 0.9f, 1.0f);  // Maximum bloom
}

bool ExpressiveMacroMapper::mapEnergyToFreeze(float energy) const noexcept
{
    // Freeze enabled in "Sustain" mode (0.3-0.5)
    return energy >= 0.3f && energy <= 0.5f;
}

float ExpressiveMacroMapper::mapEnergyToParadoxGain(float energy) const noexcept
{
    // Paradox gain increases in "Chaos" mode
    if (energy < 0.8f)
        return juce::jmap(energy, 0.0f, 0.8f, 0.1f, 0.3f);
    else
        return juce::jmap(energy, 0.8f, 1.0f, 0.5f, 0.9f);  // High chaos
}

//==============================================================================
// Motion Mappings (Temporal Evolution)
//==============================================================================

float ExpressiveMacroMapper::mapMotionToDrift(float motion) const noexcept
{
    // 0.0-0.2: Still (no drift)
    // 0.3-0.5: Drift (slow Brownian)
    // 0.6-0.8: Pulse (rhythmic LFO)
    // 0.9-1.0: Random (chaos attractor)

    std::vector<float> breakpoints{0.0f, 0.2f, 0.5f, 0.8f, 1.0f};
    std::vector<float> values{0.0f, 0.2f, 0.5f, 0.7f, 0.9f};
    return piecewiseLinear(motion, breakpoints, values);
}

float ExpressiveMacroMapper::mapMotionToWarp(float motion) const noexcept
{
    // Warp increases with motion intensity
    if (motion < 0.2f)
        return 0.0f;  // Still = no warp
    else if (motion < 0.6f)
        return juce::jmap(motion, 0.2f, 0.6f, 0.1f, 0.4f);  // Gentle
    else
        return juce::jmap(motion, 0.6f, 1.0f, 0.5f, 0.9f);  // Dramatic
}

float ExpressiveMacroMapper::mapMotionToModulationDepth(float motion) const noexcept
{
    // Global modulation depth multiplier
    return motion;  // Direct linear mapping
}

//==============================================================================
// Color Mappings (Spectral Character)
//==============================================================================

float ExpressiveMacroMapper::mapColorToMass(float color) const noexcept
{
    // 0.0-0.2: Dark (high mass = damping)
    // 0.3-0.6: Balanced (neutral mass)
    // 0.7-0.8: Bright (low mass = ringing)
    // 0.9-1.0: Spectral (very low mass)

    std::vector<float> breakpoints{0.0f, 0.2f, 0.6f, 0.8f, 1.0f};
    std::vector<float> values{0.8f, 0.6f, 0.5f, 0.3f, 0.1f};  // Inverted: dark = high
    return piecewiseLinear(color, breakpoints, values);
}

float ExpressiveMacroMapper::mapColorToAir(float color) const noexcept
{
    // Air (high-frequency lift) increases with brightness
    if (color < 0.3f)
        return juce::jmap(color, 0.0f, 0.3f, 0.2f, 0.4f);  // Dark
    else if (color < 0.7f)
        return juce::jmap(color, 0.3f, 0.7f, 0.5f, 0.7f);  // Balanced
    else
        return juce::jmap(color, 0.7f, 1.0f, 0.8f, 1.0f);  // Bright/Spectral
}

float ExpressiveMacroMapper::mapColorToGravity(float color) const noexcept
{
    // Gravity (spectral tilt) - higher = darker
    return 1.0f - color;  // Invert: dark = high gravity
}

float ExpressiveMacroMapper::mapColorToMetallicResonance(float color) const noexcept
{
    // Metallic resonance only relevant in spectral range
    if (color < 0.8f)
        return juce::jmap(color, 0.0f, 0.8f, 0.2f, 0.5f);
    else
        return juce::jmap(color, 0.8f, 1.0f, 0.6f, 1.0f);  // Maximum at spectral
}

//==============================================================================
// Dimension Mappings (Perceived Space Size)
//==============================================================================

float ExpressiveMacroMapper::mapDimensionToTime(float dimension) const noexcept
{
    // 0.0-0.2: Intimate (short decay)
    // 0.3-0.5: Room (medium decay)
    // 0.6-0.8: Cathedral (long decay)
    // 0.9-1.0: Infinite (very long decay)

    std::vector<float> breakpoints{0.0f, 0.2f, 0.5f, 0.8f, 1.0f};
    std::vector<float> values{0.2f, 0.4f, 0.6f, 0.8f, 1.0f};
    return piecewiseLinear(dimension, breakpoints, values);
}

float ExpressiveMacroMapper::mapDimensionToDensity(float dimension) const noexcept
{
    // Density increases with space size (more reflections in larger spaces)
    return juce::jmap(dimension, 0.0f, 1.0f, 0.3f, 0.9f);
}

float ExpressiveMacroMapper::mapDimensionToWidth(float dimension) const noexcept
{
    // Width increases with space size
    return juce::jmap(dimension, 0.0f, 1.0f, 0.4f, 1.0f);
}

float ExpressiveMacroMapper::mapDimensionToImpossibility(float dimension) const noexcept
{
    // Impossibility (alien physics) only kicks in at "Infinite" range
    if (dimension < 0.8f)
        return juce::jmap(dimension, 0.0f, 0.8f, 0.0f, 0.2f);
    else
        return juce::jmap(dimension, 0.8f, 1.0f, 0.3f, 0.9f);  // Dramatic jump
}

//==============================================================================
// Utility Functions
//==============================================================================

float ExpressiveMacroMapper::piecewiseLinear(
    float input,
    const std::vector<float>& breakpoints,
    const std::vector<float>& values) const noexcept
{
    jassert(breakpoints.size() == values.size());
    jassert(breakpoints.size() >= 2);

    // Clamp input
    if (input <= breakpoints.front())
        return values.front();
    if (input >= breakpoints.back())
        return values.back();

    // Find segment containing input
    for (size_t i = 0; i < breakpoints.size() - 1; ++i)
    {
        if (input >= breakpoints[i] && input <= breakpoints[i + 1])
        {
            // Linear interpolation within segment
            const float t = (input - breakpoints[i]) / (breakpoints[i + 1] - breakpoints[i]);
            return values[i] + t * (values[i + 1] - values[i]);
        }
    }

    return values.back();  // Fallback
}

} // namespace dsp
} // namespace monument