#include "SequencePresets.h"
#include <cmath>

namespace monument
{
namespace dsp
{

SequenceScheduler::Sequence SequencePresets::createEvolvingCathedral()
{
    using Keyframe = SequenceScheduler::Keyframe;
    using ParamId = SequenceScheduler::ParameterId;
    using Interpolation = SequenceScheduler::InterpolationType;

    SequenceScheduler::Sequence sequence("Evolving Cathedral");
    sequence.timingMode = SequenceScheduler::TimingMode::Beats;
    sequence.playbackMode = SequenceScheduler::PlaybackMode::Loop;
    sequence.durationBeats = 16.0;
    sequence.enabled = false;  // Will be enabled when loaded

    // Keyframe 0 (Beat 0): Small room
    Keyframe kf0(0.0, Interpolation::SCurve);
    kf0.setParameter(ParamId::Time, 0.2f);      // Short decay
    kf0.setParameter(ParamId::Density, 0.3f);   // Sparse reflections
    kf0.setParameter(ParamId::Mass, 0.2f);      // Light, quick response
    kf0.setParameter(ParamId::Bloom, 0.3f);     // Minimal diffusion
    sequence.addKeyframe(kf0);

    // Keyframe 1 (Beat 4): Growing space
    Keyframe kf1(4.0, Interpolation::SCurve);
    kf1.setParameter(ParamId::Time, 0.5f);      // Medium decay
    kf1.setParameter(ParamId::Density, 0.5f);   // More reflections
    kf1.setParameter(ParamId::Mass, 0.4f);      // Gaining weight
    kf1.setParameter(ParamId::Bloom, 0.5f);     // Increasing diffusion
    sequence.addKeyframe(kf1);

    // Keyframe 2 (Beat 8): Large hall
    Keyframe kf2(8.0, Interpolation::SCurve);
    kf2.setParameter(ParamId::Time, 0.75f);     // Long decay
    kf2.setParameter(ParamId::Density, 0.7f);   // Dense reflections
    kf2.setParameter(ParamId::Mass, 0.6f);      // Heavy, slow response
    kf2.setParameter(ParamId::Bloom, 0.7f);     // High diffusion
    sequence.addKeyframe(kf2);

    // Keyframe 3 (Beat 12): Massive cathedral
    Keyframe kf3(12.0, Interpolation::SCurve);
    kf3.setParameter(ParamId::Time, 1.0f);      // Maximum decay
    kf3.setParameter(ParamId::Density, 0.9f);   // Very dense
    kf3.setParameter(ParamId::Mass, 0.8f);      // Maximum mass
    kf3.setParameter(ParamId::Bloom, 0.9f);     // Maximum diffusion
    sequence.addKeyframe(kf3);

    // Keyframe 4 (Beat 16): Hold at massive (loop point)
    Keyframe kf4(16.0, Interpolation::Linear);
    kf4.setParameter(ParamId::Time, 1.0f);
    kf4.setParameter(ParamId::Density, 0.9f);
    kf4.setParameter(ParamId::Mass, 0.8f);
    kf4.setParameter(ParamId::Bloom, 0.9f);
    sequence.addKeyframe(kf4);

    return sequence;
}

SequenceScheduler::Sequence SequencePresets::createSpatialJourney()
{
    using Keyframe = SequenceScheduler::Keyframe;
    using ParamId = SequenceScheduler::ParameterId;
    using Interpolation = SequenceScheduler::InterpolationType;

    SequenceScheduler::Sequence sequence("Spatial Journey");
    sequence.timingMode = SequenceScheduler::TimingMode::Beats;
    sequence.playbackMode = SequenceScheduler::PlaybackMode::Loop;
    sequence.durationBeats = 16.0;
    sequence.enabled = false;

    // Create circular path in X/Y plane (8 keyframes for smooth circle)
    const int numSteps = 8;
    const double beatsPerStep = 16.0 / numSteps;

    for (int i = 0; i <= numSteps; ++i)
    {
        const double beat = i * beatsPerStep;
        const double angle = (i / static_cast<double>(numSteps)) * 2.0 * M_PI;

        Keyframe kf(beat, Interpolation::SCurve);

        // Circular path in X/Y plane
        float x = 0.0f + 0.8f * static_cast<float>(std::cos(angle));  // Circle radius 0.8
        float y = 0.0f + 0.8f * static_cast<float>(std::sin(angle));

        // Z oscillates up and down (figure-8 in 3D)
        float z = 0.5f + 0.3f * static_cast<float>(std::sin(2.0 * angle));

        // Velocity for Doppler shift (tangent to circle)
        float velocityX = -0.3f * static_cast<float>(std::sin(angle));

        // Remap to [0, 1] range (assuming spatial coordinates are [-1, +1])
        float xNorm = (x + 1.0f) * 0.5f;
        float yNorm = (y + 1.0f) * 0.5f;
        float zNorm = juce::jlimit(0.0f, 1.0f, z);
        float velXNorm = (velocityX + 1.0f) * 0.5f;

        kf.setParameter(ParamId::PositionX, xNorm);
        kf.setParameter(ParamId::PositionY, yNorm);
        kf.setParameter(ParamId::PositionZ, zNorm);
        kf.setParameter(ParamId::VelocityX, velXNorm);

        sequence.addKeyframe(kf);
    }

    return sequence;
}

SequenceScheduler::Sequence SequencePresets::createLivingSpace()
{
    using Keyframe = SequenceScheduler::Keyframe;
    using ParamId = SequenceScheduler::ParameterId;
    using Interpolation = SequenceScheduler::InterpolationType;

    SequenceScheduler::Sequence sequence("Living Space");
    sequence.timingMode = SequenceScheduler::TimingMode::Seconds;
    sequence.playbackMode = SequenceScheduler::PlaybackMode::Loop;
    sequence.durationSeconds = 32.0;
    sequence.enabled = false;

    // Keyframe 0 (0s): Neutral starting point
    Keyframe kf0(0.0, Interpolation::SCurve);
    kf0.setParameter(ParamId::Warp, 0.0f);      // No shimmer
    kf0.setParameter(ParamId::Drift, 0.0f);     // No pitch drift
    kf0.setParameter(ParamId::Bloom, 0.4f);     // Moderate density
    kf0.setParameter(ParamId::Gravity, 0.5f);   // Neutral damping
    sequence.addKeyframe(kf0);

    // Keyframe 1 (8s): Shimmer begins, bloom expands
    Keyframe kf1(8.0, Interpolation::SCurve);
    kf1.setParameter(ParamId::Warp, 0.3f);      // Shimmer appears
    kf1.setParameter(ParamId::Drift, 0.1f);     // Subtle pitch drift
    kf1.setParameter(ParamId::Bloom, 0.6f);     // Expanding density
    kf1.setParameter(ParamId::Gravity, 0.6f);   // More damping
    sequence.addKeyframe(kf1);

    // Keyframe 2 (16s): Peak evolution
    Keyframe kf2(16.0, Interpolation::SCurve);
    kf2.setParameter(ParamId::Warp, 0.4f);      // Maximum shimmer
    kf2.setParameter(ParamId::Drift, 0.2f);     // More pitch drift
    kf2.setParameter(ParamId::Bloom, 0.7f);     // Dense, blooming
    kf2.setParameter(ParamId::Gravity, 0.7f);   // High damping
    sequence.addKeyframe(kf2);

    // Keyframe 3 (24s): Returning to calm
    Keyframe kf3(24.0, Interpolation::SCurve);
    kf3.setParameter(ParamId::Warp, 0.2f);      // Shimmer fading
    kf3.setParameter(ParamId::Drift, 0.1f);     // Drift reducing
    kf3.setParameter(ParamId::Bloom, 0.5f);     // Contracting
    kf3.setParameter(ParamId::Gravity, 0.5f);   // Neutral damping
    sequence.addKeyframe(kf3);

    // Keyframe 4 (32s): Back to start (loop point)
    Keyframe kf4(32.0, Interpolation::SCurve);
    kf4.setParameter(ParamId::Warp, 0.0f);
    kf4.setParameter(ParamId::Drift, 0.0f);
    kf4.setParameter(ParamId::Bloom, 0.4f);
    kf4.setParameter(ParamId::Gravity, 0.5f);
    sequence.addKeyframe(kf4);

    return sequence;
}

SequenceScheduler::Sequence SequencePresets::createInfiniteAbyss()
{
    using Keyframe = SequenceScheduler::Keyframe;
    using ParamId = SequenceScheduler::ParameterId;
    using Interpolation = SequenceScheduler::InterpolationType;

    SequenceScheduler::Sequence sequence("Infinite Abyss");
    sequence.timingMode = SequenceScheduler::TimingMode::Beats;
    sequence.playbackMode = SequenceScheduler::PlaybackMode::Loop;
    sequence.durationBeats = 64.0;
    sequence.enabled = false;

    // Keyframe 0 (0 beats): Deep pit begins
    Keyframe kf0(0.0, Interpolation::SCurve);
    kf0.setParameter(ParamId::Time, 1.0f);       // Maximum decay
    kf0.setParameter(ParamId::Mass, 0.9f);       // Ultra-heavy
    kf0.setParameter(ParamId::Density, 0.85f);   // Dense reflections
    kf0.setParameter(ParamId::Bloom, 0.8f);      // High diffusion
    kf0.setParameter(ParamId::Gravity, 0.3f);    // Light damping (eternal tail)
    // Memory system for eternal feedback (fixes documentation mismatch)
    kf0.setParameter(ParamId::Memory, 0.8f);      // High memory amount
    kf0.setParameter(ParamId::MemoryDepth, 0.7f); // Strong feedback injection
    kf0.setParameter(ParamId::MemoryDecay, 0.9f); // Very slow decay (near-infinite)
    kf0.setParameter(ParamId::MemoryDrift, 0.3f); // Moderate drift for organic aging
    sequence.addKeyframe(kf0);

    // Keyframe 1 (16 beats): Gravity destabilizes, memory intensifies
    Keyframe kf1(16.0, Interpolation::SCurve);
    kf1.setParameter(ParamId::Gravity, 0.1f);    // Even lighter (chaos begins)
    kf1.setParameter(ParamId::MemoryDepth, 0.85f); // Peak feedback injection
    sequence.addKeyframe(kf1);

    // Keyframe 2 (32 beats): Gravity oscillates, memory stabilizes
    Keyframe kf2(32.0, Interpolation::SCurve);
    kf2.setParameter(ParamId::Gravity, 0.5f);    // Heavier
    kf2.setParameter(ParamId::MemoryDepth, 0.65f); // Slightly reduced feedback
    sequence.addKeyframe(kf2);

    // Keyframe 3 (48 beats): Return to light, memory drifts
    Keyframe kf3(48.0, Interpolation::SCurve);
    kf3.setParameter(ParamId::Gravity, 0.2f);
    kf3.setParameter(ParamId::MemoryDrift, 0.5f);  // Increased drift for variation
    sequence.addKeyframe(kf3);

    // Keyframe 4 (64 beats): Loop point, return to initial memory state
    Keyframe kf4(64.0, Interpolation::SCurve);
    kf4.setParameter(ParamId::Gravity, 0.3f);
    kf4.setParameter(ParamId::MemoryDepth, 0.7f);  // Back to initial
    kf4.setParameter(ParamId::MemoryDrift, 0.3f);  // Back to initial
    sequence.addKeyframe(kf4);

    return sequence;
}

SequenceScheduler::Sequence SequencePresets::createQuantumTunneling()
{
    using Keyframe = SequenceScheduler::Keyframe;
    using ParamId = SequenceScheduler::ParameterId;
    using Interpolation = SequenceScheduler::InterpolationType;

    SequenceScheduler::Sequence sequence("Quantum Tunneling");
    sequence.timingMode = SequenceScheduler::TimingMode::Beats;
    sequence.playbackMode = SequenceScheduler::PlaybackMode::Loop;
    sequence.durationBeats = 32.0;
    sequence.enabled = false;

    // Base parameters: sparse, warped, drifting
    Keyframe base(0.0, Interpolation::Linear);
    base.setParameter(ParamId::Time, 0.85f);
    base.setParameter(ParamId::Density, 0.15f);   // Ultra-sparse
    base.setParameter(ParamId::Bloom, 0.9f);      // High bloom for artifacts
    base.setParameter(ParamId::Warp, 1.0f);       // Maximum warp
    base.setParameter(ParamId::Drift, 0.8f);      // High drift
    base.setParameter(ParamId::Mass, 0.3f);       // Light
    sequence.addKeyframe(base);

    // Create rapid spatial jumps (8 keyframes over 32 beats = 4 beat intervals)
    const int numJumps = 8;
    const double beatsPerJump = 32.0 / numJumps;

    for (int i = 1; i <= numJumps; ++i)
    {
        const double beat = i * beatsPerJump;
        const double phase = (i / static_cast<double>(numJumps)) * 2.0 * M_PI;

        Keyframe kf(beat, Interpolation::Step);  // Step = instant jump (quantum tunnel)

        // Positions jump discontinuously through space
        float x = 0.5f + 0.4f * static_cast<float>(std::cos(phase * 3.0));
        float y = 0.5f + 0.4f * static_cast<float>(std::sin(phase * 2.0));
        float z = 0.5f + 0.3f * static_cast<float>(std::sin(phase * 5.0));

        // Velocity spikes create Doppler shifts
        float velX = 0.5f + 0.4f * static_cast<float>(std::cos(phase * 7.0));

        kf.setParameter(ParamId::PositionX, juce::jlimit(0.0f, 1.0f, x));
        kf.setParameter(ParamId::PositionY, juce::jlimit(0.0f, 1.0f, y));
        kf.setParameter(ParamId::PositionZ, juce::jlimit(0.0f, 1.0f, z));
        kf.setParameter(ParamId::VelocityX, juce::jlimit(0.0f, 1.0f, velX));

        sequence.addKeyframe(kf);
    }

    return sequence;
}

SequenceScheduler::Sequence SequencePresets::createTimeDissolution()
{
    using Keyframe = SequenceScheduler::Keyframe;
    using ParamId = SequenceScheduler::ParameterId;
    using Interpolation = SequenceScheduler::InterpolationType;

    SequenceScheduler::Sequence sequence("Time Dissolution");
    sequence.timingMode = SequenceScheduler::TimingMode::Seconds;
    sequence.playbackMode = SequenceScheduler::PlaybackMode::Loop;
    sequence.durationSeconds = 120.0;  // 2 minutes of slow evolution
    sequence.enabled = false;

    // Keyframe 0 (0s): Stable starting point
    Keyframe kf0(0.0, Interpolation::SCurve);
    kf0.setParameter(ParamId::Time, 0.9f);       // Long decay
    kf0.setParameter(ParamId::Mass, 0.1f);       // Weightless
    kf0.setParameter(ParamId::Drift, 0.5f);      // Moderate drift
    kf0.setParameter(ParamId::Bloom, 0.6f);
    kf0.setParameter(ParamId::Density, 0.5f);
    sequence.addKeyframe(kf0);

    // Keyframe 1 (30s): Time becomes unstable
    Keyframe kf1(30.0, Interpolation::SCurve);
    kf1.setParameter(ParamId::Drift, 1.0f);      // Maximum drift
    kf1.setParameter(ParamId::Time, 0.6f);       // Time speeds up
    sequence.addKeyframe(kf1);

    // Keyframe 2 (60s): Peak instability
    Keyframe kf2(60.0, Interpolation::SCurve);
    kf2.setParameter(ParamId::Drift, 0.8f);
    kf2.setParameter(ParamId::Time, 1.0f);       // Time slows to maximum
    kf2.setParameter(ParamId::Warp, 0.5f);       // Add shimmer
    sequence.addKeyframe(kf2);

    // Keyframe 3 (90s): Returning
    Keyframe kf3(90.0, Interpolation::SCurve);
    kf3.setParameter(ParamId::Drift, 0.4f);
    kf3.setParameter(ParamId::Time, 0.8f);
    kf3.setParameter(ParamId::Warp, 0.2f);
    sequence.addKeyframe(kf3);

    // Keyframe 4 (120s): Back to start (loop)
    Keyframe kf4(120.0, Interpolation::SCurve);
    kf4.setParameter(ParamId::Drift, 0.5f);
    kf4.setParameter(ParamId::Time, 0.9f);
    kf4.setParameter(ParamId::Warp, 0.0f);
    sequence.addKeyframe(kf4);

    return sequence;
}

SequenceScheduler::Sequence SequencePresets::createCrystallineVoid()
{
    using Keyframe = SequenceScheduler::Keyframe;
    using ParamId = SequenceScheduler::ParameterId;
    using Interpolation = SequenceScheduler::InterpolationType;

    SequenceScheduler::Sequence sequence("Crystalline Void");
    sequence.timingMode = SequenceScheduler::TimingMode::Beats;
    sequence.playbackMode = SequenceScheduler::PlaybackMode::Loop;
    sequence.durationBeats = 48.0;
    sequence.enabled = false;

    // Keyframe 0 (0 beats): Crystalline space
    Keyframe kf0(0.0, Interpolation::Linear);
    kf0.setParameter(ParamId::Time, 0.9f);
    kf0.setParameter(ParamId::Mass, 0.85f);      // Heavy, metallic
    kf0.setParameter(ParamId::Density, 0.05f);   // Ultra-sparse (crystalline)
    kf0.setParameter(ParamId::Bloom, 0.95f);     // Maximum bloom
    kf0.setParameter(ParamId::Gravity, 0.6f);    // Some damping
    sequence.addKeyframe(kf0);

    // Create pillar shape modulation (creates responsive crystals)
    const int numSteps = 12;
    const double beatsPerStep = 48.0 / numSteps;

    for (int i = 1; i <= numSteps; ++i)
    {
        const double beat = i * beatsPerStep;
        const double phase = (i / static_cast<double>(numSteps)) * 2.0 * M_PI;

        Keyframe kf(beat, Interpolation::SCurve);

        // Topology creates different room shapes (crystalline resonances)
        float shape = 0.7f + 0.25f * static_cast<float>(std::sin(phase));
        kf.setParameter(ParamId::Topology, juce::jlimit(0.0f, 1.0f, shape));

        // Subtle density variation
        float dens = 0.05f + 0.03f * static_cast<float>(std::cos(phase * 2.0));
        kf.setParameter(ParamId::Density, juce::jlimit(0.02f, 0.1f, dens));

        sequence.addKeyframe(kf);
    }

    return sequence;
}

SequenceScheduler::Sequence SequencePresets::createHyperdimensionalFold()
{
    using Keyframe = SequenceScheduler::Keyframe;
    using ParamId = SequenceScheduler::ParameterId;
    using Interpolation = SequenceScheduler::InterpolationType;

    SequenceScheduler::Sequence sequence("Hyperdimensional Fold");
    sequence.timingMode = SequenceScheduler::TimingMode::Beats;
    sequence.playbackMode = SequenceScheduler::PlaybackMode::Loop;
    sequence.durationBeats = 64.0;
    sequence.enabled = false;

    // Create complex evolution across all major parameters
    const int numKeyframes = 16;
    const double beatsPerKeyframe = 64.0 / numKeyframes;

    for (int i = 0; i <= numKeyframes; ++i)
    {
        const double beat = i * beatsPerKeyframe;
        const double phase = (i / static_cast<double>(numKeyframes)) * 2.0 * M_PI;

        Keyframe kf(beat, Interpolation::SCurve);

        // All parameters evolve with different periods
        kf.setParameter(ParamId::Time,
            0.5f + 0.4f * static_cast<float>(std::sin(phase * 1.0)));

        kf.setParameter(ParamId::Mass,
            0.5f + 0.3f * static_cast<float>(std::cos(phase * 1.5)));

        kf.setParameter(ParamId::Density,
            0.5f + 0.4f * static_cast<float>(std::sin(phase * 2.0)));

        kf.setParameter(ParamId::Bloom,
            0.5f + 0.4f * static_cast<float>(std::cos(phase * 0.7)));

        kf.setParameter(ParamId::Gravity,
            0.5f + 0.3f * static_cast<float>(std::sin(phase * 1.3)));

        kf.setParameter(ParamId::Warp,
            0.3f + 0.5f * static_cast<float>(std::sin(phase * 3.0)));

        kf.setParameter(ParamId::Drift,
            0.2f + 0.4f * static_cast<float>(std::cos(phase * 2.5)));

        // Clamp all values
        for (auto& [id, val] : kf.parameterValues)
            val = juce::jlimit(0.0f, 1.0f, val);

        sequence.addKeyframe(kf);
    }

    return sequence;
}

std::vector<SequenceScheduler::Sequence> SequencePresets::getAllPresets()
{
    return {
        createEvolvingCathedral(),
        createSpatialJourney(),
        createLivingSpace(),
        createInfiniteAbyss(),
        createQuantumTunneling(),
        createTimeDissolution(),
        createCrystallineVoid(),
        createHyperdimensionalFold()
    };
}

SequenceScheduler::Sequence SequencePresets::getPreset(int index)
{
    switch (index)
    {
        case 0: return createEvolvingCathedral();
        case 1: return createSpatialJourney();
        case 2: return createLivingSpace();
        case 3: return createInfiniteAbyss();
        case 4: return createQuantumTunneling();
        case 5: return createTimeDissolution();
        case 6: return createCrystallineVoid();
        case 7: return createHyperdimensionalFold();
        default: return createEvolvingCathedral();
    }
}

juce::String SequencePresets::getPresetName(int index)
{
    switch (index)
    {
        case 0: return "Evolving Cathedral";
        case 1: return "Spatial Journey";
        case 2: return "Living Space";
        case 3: return "Infinite Abyss";
        case 4: return "Quantum Tunneling";
        case 5: return "Time Dissolution";
        case 6: return "Crystalline Void";
        case 7: return "Hyperdimensional Fold";
        default: return "Unknown";
    }
}

juce::String SequencePresets::getPresetDescription(int index)
{
    switch (index)
    {
        case 0: return "Reverb morphs from small room to massive cathedral over 16 bars";
        case 1: return "Sound source travels through 3D space in tempo-synced circular patterns";
        case 2: return "Subtle organic drift in room characteristics over 32 seconds";
        case 3: return "Bottomless pit with eternal memory feedback, gravity oscillates over 64 beats";
        case 4: return "Sound teleports through impossible geometry with rapid spatial jumps";
        case 5: return "Time becomes unstable, extreme drift creates wildly shifting pitch";
        case 6: return "Ultra-sparse crystalline resonances with dancing pillar positions";
        case 7: return "All dimensions modulate simultaneously, never-repeating impossible space";
        default: return "";
    }
}

} // namespace dsp
} // namespace monument
