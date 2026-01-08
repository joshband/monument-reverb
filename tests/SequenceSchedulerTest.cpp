/**
 * SequenceSchedulerTest.cpp
 *
 * Unit test for SequenceScheduler timeline automation system.
 * Verifies keyframe interpolation, tempo sync, playback modes, and parameter automation.
 *
 * Part of Three-System Plan Phase 4.
 */

#include "../dsp/SequenceScheduler.h"
#include "../dsp/SequencePresets.h"
#include <iostream>
#include <cmath>
#include <cassert>

using namespace monument::dsp;

namespace
{
    constexpr double kSampleRate = 48000.0;
    constexpr int kBlockSize = 512;
    constexpr float kEpsilon = 0.0001f;
}

static void testBasicKeyframeInterpolation()
{
    std::cout << "Testing basic keyframe interpolation...\n";

    SequenceScheduler scheduler;
    scheduler.prepare(kSampleRate, kBlockSize);

    // Create simple 2-keyframe sequence
    SequenceScheduler::Sequence sequence("Test");
    sequence.timingMode = SequenceScheduler::TimingMode::Seconds;
    sequence.playbackMode = SequenceScheduler::PlaybackMode::OneShot;
    sequence.durationSeconds = 4.0;
    sequence.enabled = true;

    // Keyframe 0: Time = 0.0
    SequenceScheduler::Keyframe kf0(0.0, SequenceScheduler::InterpolationType::Linear);
    kf0.setParameter(SequenceScheduler::ParameterId::Time, 0.0f);
    sequence.addKeyframe(kf0);

    // Keyframe 1: Time = 1.0
    SequenceScheduler::Keyframe kf1(4.0, SequenceScheduler::InterpolationType::Linear);
    kf1.setParameter(SequenceScheduler::ParameterId::Time, 1.0f);
    sequence.addKeyframe(kf1);

    scheduler.loadSequence(sequence);

    // Process for 2 seconds (should be at 50% = 0.5)
    const int numSamples = static_cast<int>(2.0 * kSampleRate);
    const int numBlocks = numSamples / kBlockSize;

    for (int i = 0; i < numBlocks; ++i)
    {
        scheduler.process(juce::nullopt, kBlockSize);
    }

    auto timeValue = scheduler.getParameterValue(SequenceScheduler::ParameterId::Time);
    assert(timeValue.has_value());
    assert(std::abs(*timeValue - 0.5f) < 0.01f);  // Allow 1% tolerance

    std::cout << "  ✓ Linear interpolation: Time = " << *timeValue << " (expected 0.5)\n";
    std::cout << "  ✓ Position: " << scheduler.getCurrentPosition() << " seconds\n";
}

static void testMultipleParameters()
{
    std::cout << "\nTesting multiple parameter automation...\n";

    SequenceScheduler scheduler;
    scheduler.prepare(kSampleRate, kBlockSize);

    SequenceScheduler::Sequence sequence("Multi-param");
    sequence.timingMode = SequenceScheduler::TimingMode::Seconds;
    sequence.playbackMode = SequenceScheduler::PlaybackMode::OneShot;
    sequence.durationSeconds = 1.0;
    sequence.enabled = true;

    // Start keyframe
    SequenceScheduler::Keyframe kf0(0.0, SequenceScheduler::InterpolationType::Linear);
    kf0.setParameter(SequenceScheduler::ParameterId::Time, 0.0f);
    kf0.setParameter(SequenceScheduler::ParameterId::Mass, 0.2f);
    kf0.setParameter(SequenceScheduler::ParameterId::Density, 0.3f);
    sequence.addKeyframe(kf0);

    // End keyframe
    SequenceScheduler::Keyframe kf1(1.0, SequenceScheduler::InterpolationType::Linear);
    kf1.setParameter(SequenceScheduler::ParameterId::Time, 1.0f);
    kf1.setParameter(SequenceScheduler::ParameterId::Mass, 0.8f);
    kf1.setParameter(SequenceScheduler::ParameterId::Density, 0.9f);
    sequence.addKeyframe(kf1);

    scheduler.loadSequence(sequence);

    // Process to midpoint
    const int numSamples = static_cast<int>(0.5 * kSampleRate);
    const int numBlocks = numSamples / kBlockSize;

    for (int i = 0; i < numBlocks; ++i)
    {
        scheduler.process(juce::nullopt, kBlockSize);
    }

    auto time = scheduler.getParameterValue(SequenceScheduler::ParameterId::Time);
    auto mass = scheduler.getParameterValue(SequenceScheduler::ParameterId::Mass);
    auto density = scheduler.getParameterValue(SequenceScheduler::ParameterId::Density);

    assert(time.has_value() && std::abs(*time - 0.5f) < 0.01f);
    assert(mass.has_value() && std::abs(*mass - 0.5f) < 0.01f);
    assert(density.has_value() && std::abs(*density - 0.6f) < 0.01f);

    std::cout << "  ✓ Time: " << *time << " (expected 0.5)\n";
    std::cout << "  ✓ Mass: " << *mass << " (expected 0.5)\n";
    std::cout << "  ✓ Density: " << *density << " (expected 0.6)\n";
}

static void testLoopMode()
{
    std::cout << "\nTesting loop playback mode...\n";

    SequenceScheduler scheduler;
    scheduler.prepare(kSampleRate, kBlockSize);

    SequenceScheduler::Sequence sequence("Loop");
    sequence.timingMode = SequenceScheduler::TimingMode::Seconds;
    sequence.playbackMode = SequenceScheduler::PlaybackMode::Loop;
    sequence.durationSeconds = 1.0;
    sequence.enabled = true;

    SequenceScheduler::Keyframe kf0(0.0, SequenceScheduler::InterpolationType::Linear);
    kf0.setParameter(SequenceScheduler::ParameterId::Warp, 0.0f);
    sequence.addKeyframe(kf0);

    SequenceScheduler::Keyframe kf1(1.0, SequenceScheduler::InterpolationType::Linear);
    kf1.setParameter(SequenceScheduler::ParameterId::Warp, 1.0f);
    sequence.addKeyframe(kf1);

    scheduler.loadSequence(sequence);

    // Process for 2.5 seconds (should loop and be at 0.5 in second cycle)
    const int numSamples = static_cast<int>(2.5 * kSampleRate);
    const int numBlocks = numSamples / kBlockSize;

    for (int i = 0; i < numBlocks; ++i)
    {
        scheduler.process(juce::nullopt, kBlockSize);
    }

    auto warp = scheduler.getParameterValue(SequenceScheduler::ParameterId::Warp);
    double position = scheduler.getCurrentPosition();

    // Position should wrap to 0.5 (2.5 - 2.0 = 0.5)
    assert(std::abs(position - 0.5) < 0.01);
    assert(warp.has_value() && std::abs(*warp - 0.5f) < 0.01f);

    std::cout << "  ✓ Looped position: " << position << " (expected 0.5)\n";
    std::cout << "  ✓ Warp value: " << *warp << " (expected 0.5)\n";
}

static void testInterpolationCurves()
{
    std::cout << "\nTesting interpolation curve types...\n";

    SequenceScheduler scheduler;
    scheduler.prepare(kSampleRate, kBlockSize);

    // Test S-curve interpolation
    SequenceScheduler::Sequence sequence("S-Curve");
    sequence.timingMode = SequenceScheduler::TimingMode::Seconds;
    sequence.playbackMode = SequenceScheduler::PlaybackMode::OneShot;
    sequence.durationSeconds = 1.0;
    sequence.enabled = true;

    SequenceScheduler::Keyframe kf0(0.0, SequenceScheduler::InterpolationType::SCurve);
    kf0.setParameter(SequenceScheduler::ParameterId::Drift, 0.0f);
    sequence.addKeyframe(kf0);

    SequenceScheduler::Keyframe kf1(1.0, SequenceScheduler::InterpolationType::SCurve);
    kf1.setParameter(SequenceScheduler::ParameterId::Drift, 1.0f);
    sequence.addKeyframe(kf1);

    scheduler.loadSequence(sequence);

    // Sample at 0.25 (should be less than 0.25 due to ease-in)
    scheduler.setCurrentPosition(0.25);
    auto val25 = scheduler.getParameterValue(SequenceScheduler::ParameterId::Drift);

    // Sample at 0.5 (should be exactly 0.5)
    scheduler.setCurrentPosition(0.5);
    auto val50 = scheduler.getParameterValue(SequenceScheduler::ParameterId::Drift);

    // Sample at 0.75 (should be greater than 0.75 due to ease-out)
    scheduler.setCurrentPosition(0.75);
    auto val75 = scheduler.getParameterValue(SequenceScheduler::ParameterId::Drift);

    assert(val25.has_value() && *val25 < 0.25f);
    assert(val50.has_value() && std::abs(*val50 - 0.5f) < kEpsilon);
    assert(val75.has_value() && *val75 > 0.75f);

    std::cout << "  ✓ S-curve at 0.25: " << *val25 << " (< 0.25, ease-in)\n";
    std::cout << "  ✓ S-curve at 0.50: " << *val50 << " (= 0.50, midpoint)\n";
    std::cout << "  ✓ S-curve at 0.75: " << *val75 << " (> 0.75, ease-out)\n";
}

static void testTempoSync()
{
    std::cout << "\nTesting tempo-synced beat timing...\n";

    SequenceScheduler scheduler;
    scheduler.prepare(kSampleRate, kBlockSize);

    SequenceScheduler::Sequence sequence("Tempo");
    sequence.timingMode = SequenceScheduler::TimingMode::Beats;
    sequence.playbackMode = SequenceScheduler::PlaybackMode::OneShot;
    sequence.durationBeats = 4.0;
    sequence.enabled = true;

    SequenceScheduler::Keyframe kf0(0.0, SequenceScheduler::InterpolationType::Linear);
    kf0.setParameter(SequenceScheduler::ParameterId::Bloom, 0.0f);
    sequence.addKeyframe(kf0);

    SequenceScheduler::Keyframe kf1(4.0, SequenceScheduler::InterpolationType::Linear);
    kf1.setParameter(SequenceScheduler::ParameterId::Bloom, 1.0f);
    sequence.addKeyframe(kf1);

    scheduler.loadSequence(sequence);

    // Simulate 120 BPM (2 beats per second)
    // After 1 second, should be at beat 2 (50% through 4-beat sequence)
    juce::AudioPlayHead::PositionInfo posInfo;
    posInfo.setBpm(120.0);

    const int numSamples = static_cast<int>(1.0 * kSampleRate);
    const int numBlocks = numSamples / kBlockSize;

    for (int i = 0; i < numBlocks; ++i)
    {
        scheduler.process(posInfo, kBlockSize);
    }

    auto bloom = scheduler.getParameterValue(SequenceScheduler::ParameterId::Bloom);
    double position = scheduler.getCurrentPosition();

    assert(std::abs(position - 2.0) < 0.1);  // Should be around beat 2
    assert(bloom.has_value() && std::abs(*bloom - 0.5f) < 0.1f);

    std::cout << "  ✓ Position after 1s @ 120 BPM: " << position << " beats (expected ~2)\n";
    std::cout << "  ✓ Bloom value: " << *bloom << " (expected ~0.5)\n";
}

static void testFactoryPresets()
{
    std::cout << "\nTesting factory presets...\n";

    // Test that all presets can be created without errors
    auto preset0 = SequencePresets::getPreset(0);
    auto preset1 = SequencePresets::getPreset(1);
    auto preset2 = SequencePresets::getPreset(2);

    assert(preset0.name == "Evolving Cathedral");
    assert(preset1.name == "Spatial Journey");
    assert(preset2.name == "Living Space");

    assert(!preset0.keyframes.empty());
    assert(!preset1.keyframes.empty());
    assert(!preset2.keyframes.empty());

    std::cout << "  ✓ Evolving Cathedral: " << preset0.keyframes.size() << " keyframes\n";
    std::cout << "  ✓ Spatial Journey: " << preset1.keyframes.size() << " keyframes\n";
    std::cout << "  ✓ Living Space: " << preset2.keyframes.size() << " keyframes\n";

    // Load and verify Evolving Cathedral
    SequenceScheduler scheduler;
    scheduler.prepare(kSampleRate, kBlockSize);
    scheduler.loadSequence(preset0);
    scheduler.setEnabled(true);

    // Process once to compute initial values
    juce::AudioPlayHead::PositionInfo posInfo;
    posInfo.setBpm(120.0);
    scheduler.process(posInfo, kBlockSize);

    // Should start at small room values
    auto timeStart = scheduler.getParameterValue(SequenceScheduler::ParameterId::Time);
    assert(timeStart.has_value() && *timeStart < 0.3f);

    std::cout << "  ✓ Evolving Cathedral initial Time: " << *timeStart << "\n";
}

static void testDisabledSequence()
{
    std::cout << "\nTesting disabled sequence (bypass)...\n";

    SequenceScheduler scheduler;
    scheduler.prepare(kSampleRate, kBlockSize);

    SequenceScheduler::Sequence sequence("Disabled");
    sequence.timingMode = SequenceScheduler::TimingMode::Seconds;
    sequence.durationSeconds = 1.0;
    sequence.enabled = false;  // Disabled

    SequenceScheduler::Keyframe kf(0.0);
    kf.setParameter(SequenceScheduler::ParameterId::Mix, 0.5f);
    sequence.addKeyframe(kf);

    scheduler.loadSequence(sequence);

    // Process some audio
    scheduler.process(juce::nullopt, kBlockSize);

    // Should return nullopt because sequence is disabled
    auto mix = scheduler.getParameterValue(SequenceScheduler::ParameterId::Mix);
    assert(!mix.has_value());

    std::cout << "  ✓ Disabled sequence returns no values\n";

    // Enable and verify it works
    scheduler.setEnabled(true);
    scheduler.process(juce::nullopt, kBlockSize);
    mix = scheduler.getParameterValue(SequenceScheduler::ParameterId::Mix);
    assert(mix.has_value() && std::abs(*mix - 0.5f) < kEpsilon);

    std::cout << "  ✓ Enabled sequence returns values: Mix = " << *mix << "\n";
}

int main()
{
    std::cout << "===== SequenceScheduler Unit Tests =====\n\n";

    try
    {
        testBasicKeyframeInterpolation();
        testMultipleParameters();
        testLoopMode();
        testInterpolationCurves();
        testTempoSync();
        testFactoryPresets();
        testDisabledSequence();

        std::cout << "\n✅ All tests passed!\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "\n❌ Test failed with exception: " << e.what() << "\n";
        return 1;
    }
    catch (...)
    {
        std::cerr << "\n❌ Test failed with unknown exception\n";
        return 1;
    }
}
