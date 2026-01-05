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

std::vector<SequenceScheduler::Sequence> SequencePresets::getAllPresets()
{
    return {
        createEvolvingCathedral(),
        createSpatialJourney(),
        createLivingSpace()
    };
}

SequenceScheduler::Sequence SequencePresets::getPreset(int index)
{
    switch (index)
    {
        case 0: return createEvolvingCathedral();
        case 1: return createSpatialJourney();
        case 2: return createLivingSpace();
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
        default: return "";
    }
}

} // namespace dsp
} // namespace monument
