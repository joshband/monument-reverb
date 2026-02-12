// monument_adapter.h
// Audio DSP QA Harness adapter for Monument Reverb
// Phase 1: Initial adapter with 15 core parameters

#pragma once

#include "core/dsp_under_test.h"
#include "core/effect_capabilities.h"
#include "core/midi_events.h"
#include "plugin/PluginProcessor.h"

#include <memory>
#include <juce_audio_processors/juce_audio_processors.h>

namespace monument {
namespace qa {

/**
 * @brief DspUnderTest adapter for Monument Reverb
 *
 * Wraps MonumentAudioProcessor to provide qa::DspUnderTest interface.
 *
 * Parameter Mapping (Phase 1: 15 core parameters):
 *   0: mix (0-100%)
 *   1: time (0-1)
 *   2: mass (0-1)
 *   3: density (0-1)
 *   4: bloom (0-1)
 *   5: air (0-1)
 *   6: width (0-1)
 *   7: warp (0-1)
 *   8: drift (0-1)
 *   9: memory (0-1)
 *   10: memoryDepth (0-1)
 *   11: memoryDecay (0-1)
 *   12: memoryDrift (0-1)
 *   13: gravity (0-1)
 *   14: freeze (bool, 0=off, 1=on)
 *
 * Future expansion (Phase 2+): 35-40 total parameters
 */
class MonumentAdapter : public ::qa::DspUnderTest
{
public:
    MonumentAdapter();
    ~MonumentAdapter() override = default;

    // Lifecycle
    void prepare(double sampleRate, int maxBlockSize, int numChannels) override;
    void release() override;
    void reset() noexcept override;

    // Processing
    void processBlock(float** channelData, int numChannels, int numSamples) noexcept override;

    // Parameters
    void setParameter(int index, ::qa::NormalizedParam value) noexcept override;
    int getParameterCount() const noexcept override { return kNumParameters; }

    // MIDI (monument accepts MIDI but doesn't require it for reverb operation)
    void processMidiEvents(const ::qa::MidiEvent* events, int numEvents) noexcept override;

    // Capabilities
    bool getCapabilities(::qa::EffectCapabilities& outCapabilities) const override;

    // Optional features
    ::qa::OptionalFeatures getOptionalFeatures() const override;

private:
    static constexpr int kNumParameters = 15;  // Phase 1: core parameters only

    // JUCE plugin processor instance
    std::unique_ptr<MonumentAudioProcessor> processor_;

    // JUCE audio/MIDI buffers
    juce::AudioBuffer<float> audioBuffer_;
    juce::MidiBuffer midiBuffer_;

    // Sample rate cache
    double sampleRate_{48000.0};

    // Re-init flag: when parameters are set after prepare() but before the first
    // processBlock(), re-calls prepareToPlay() so JUCE SmoothedValues start at
    // the correct values. NOT triggered during automation (would crash).
    bool needsReinit_{false};
    bool hasProcessed_{false};

    // Parameter ID mapping
    struct ParameterIds
    {
        static constexpr const char* Mix = "mix";
        static constexpr const char* Time = "time";
        static constexpr const char* Mass = "mass";
        static constexpr const char* Density = "density";
        static constexpr const char* Bloom = "bloom";
        static constexpr const char* Air = "air";
        static constexpr const char* Width = "width";
        static constexpr const char* Warp = "warp";
        static constexpr const char* Drift = "drift";
        static constexpr const char* Memory = "memory";
        static constexpr const char* MemoryDepth = "memoryDepth";
        static constexpr const char* MemoryDecay = "memoryDecay";
        static constexpr const char* MemoryDrift = "memoryDrift";
        static constexpr const char* Gravity = "gravity";
        static constexpr const char* Freeze = "freeze";
    };

    // Helper: Get JUCE parameter by ID
    juce::RangedAudioParameter* getJuceParameter(const char* paramId) const;

    // Helper: Set JUCE parameter value (normalized [0,1] or percentage [0,100])
    void setJuceParameter(const char* paramId, float normalizedValue);
};

} // namespace qa
} // namespace monument
