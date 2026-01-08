#include "dsp/ExpressiveMacroMapper.h"
#include <iostream>
#include <iomanip>

using namespace monument::dsp;

static void printTargets(const char* presetName, const ExpressiveMacroMapper::ParameterTargets& targets)
{
    std::cout << "\n========================================\n";
    std::cout << "Preset: " << presetName << "\n";
    std::cout << "========================================\n";

    // Primary parameters
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Time:    " << targets.time << " (decay length)\n";
    std::cout << "Mass:    " << targets.mass << " (damping)\n";
    std::cout << "Density: " << targets.density << " (diffusion)\n";
    std::cout << "Bloom:   " << targets.bloom << " (late swell)\n";
    std::cout << "Air:     " << targets.air << " (brightness)\n";
    std::cout << "Width:   " << targets.width << " (stereo)\n";

    // Advanced parameters
    std::cout << "\nAdvanced:\n";
    std::cout << "Warp:    " << targets.warp << " (matrix morph)\n";
    std::cout << "Drift:   " << targets.drift << " (modulation)\n";
    std::cout << "Gravity: " << targets.gravity << " (spectral tilt)\n";

    // Routing
    const char* routingNames[] = {
        "TraditionalCathedral",
        "MetallicGranular",
        "ElasticFeedbackDream",
        "ParallelWorlds",
        "ShimmerInfinity",
        "ImpossibleChaos",
        "OrganicBreathing",
        "MinimalSparse"
    };
    std::cout << "\nRouting: " << routingNames[static_cast<int>(targets.routingPreset)] << "\n";
}

int main()
{
    ExpressiveMacroMapper mapper;

    std::cout << "=================================================\n";
    std::cout << "ExpressiveMacroMapper Demonstration\n";
    std::cout << "=================================================\n";
    std::cout << "\nShowing how 6 musical macros control all parameters...\n";

    // Example 1: Bright, Infinite, Chaotic Shimmer
    {
        ExpressiveMacroMapper::MacroInputs inputs;
        inputs.character = 0.8f;   // Extreme effect
        inputs.spaceType = 0.5f;   // Shimmer mode
        inputs.energy = 0.95f;     // Chaos (unstable, growing)
        inputs.motion = 0.9f;      // Random evolution
        inputs.color = 0.85f;      // Bright/spectral
        inputs.dimension = 0.95f;  // Infinite space

        auto targets = mapper.computeTargets(inputs);
        printTargets("Bright Infinite Chaos", targets);
    }

    // Example 2: Dark, Intimate, Still Chamber
    {
        ExpressiveMacroMapper::MacroInputs inputs;
        inputs.character = 0.4f;   // Subtle effect
        inputs.spaceType = 0.1f;   // Chamber mode
        inputs.energy = 0.1f;      // Decay (traditional fade)
        inputs.motion = 0.1f;      // Still (no evolution)
        inputs.color = 0.1f;       // Dark
        inputs.dimension = 0.1f;   // Intimate (small)

        auto targets = mapper.computeTargets(inputs);
        printTargets("Dark Intimate Chamber", targets);
    }

    // Example 3: Metallic Granular (Space Type demonstration)
    {
        ExpressiveMacroMapper::MacroInputs inputs;
        inputs.character = 0.7f;   // Dramatic
        inputs.spaceType = 0.9f;   // Metallic mode (0.8-1.0)
        inputs.energy = 0.6f;      // Grow
        inputs.motion = 0.7f;      // Pulse
        inputs.color = 0.9f;       // Spectral (metallic)
        inputs.dimension = 0.7f;   // Cathedral size

        auto targets = mapper.computeTargets(inputs);
        printTargets("Metallic Cathedral", targets);
    }

    // Example 4: Traditional Hall (compare to Ancient Monuments "Cathedral")
    {
        ExpressiveMacroMapper::MacroInputs inputs;
        inputs.character = 0.5f;   // Balanced
        inputs.spaceType = 0.3f;   // Hall mode
        inputs.energy = 0.1f;      // Decay
        inputs.motion = 0.2f;      // Still
        inputs.color = 0.5f;       // Balanced
        inputs.dimension = 0.7f;   // Cathedral size

        auto targets = mapper.computeTargets(inputs);
        printTargets("Traditional Cathedral", targets);
    }

    std::cout << "\n=================================================\n";
    std::cout << "Key Observations:\n";
    std::cout << "=================================================\n";
    std::cout << "1. Space Type selects routing presets automatically\n";
    std::cout << "2. Each macro controls orthogonal parameters (no conflicts)\n";
    std::cout << "3. Character scales intensity globally\n";
    std::cout << "4. 6 controls create diverse, expressive sounds\n";
    std::cout << "\nCompare to Ancient Monuments (10 macros) in MacroMapper!\n";
    std::cout << "=================================================\n\n";

    return 0;
}
