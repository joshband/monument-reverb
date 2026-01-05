#pragma once

#include "dsp/DspModule.h"
#include "dsp/ParameterSmoother.h"
#include <JuceHeader.h>
#include <array>

namespace monument
{
namespace dsp
{

/**
 * @brief AlienAmplification creates non-Euclidean acoustic behavior with impossible physics.
 *
 * This module violates normal acoustic rules to create "alien" atmospheres:
 * - Pitch evolution: Frequency content morphs with time (spectral rotation)
 * - Paradox resonance: Specific frequencies amplify instead of decaying
 * - Non-local absorption: Frequency-dependent absorption that changes over time
 * - Energy inversion: Sound can get louder as it ages (carefully controlled)
 *
 * All effects are carefully bounded to maintain stability while creating
 * the sense of "impossible" sound behavior.
 */
class AlienAmplification final : public DSPModule
{
public:
    AlienAmplification();
    ~AlienAmplification() override;

    void prepare(double sampleRate, int blockSize, int numChannels) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer) override;

    /**
     * @brief Set degree of impossibility (0 = normal physics, 1 = alien physics).
     * @param normalized [0, 1]
     */
    void setImpossibilityDegree(float normalized);

    /**
     * @brief Set rate of pitch evolution (0 = static, 1 = rapid spectral morphing).
     * @param normalized [0, 1]
     */
    void setPitchEvolutionRate(float normalized);

    /**
     * @brief Set paradox resonance frequency (50-5000 Hz) - the "impossible" peak.
     * @param normalized [0, 1] → maps to frequency range
     */
    void setParadoxResonanceFreq(float normalized);

    /**
     * @brief Set paradox resonance gain (1.00-1.05) - amplification factor.
     * @param normalized [0, 1] → maps to gain [1.0, 1.05]
     *
     * SAFETY: Clamped to prevent runaway feedback. Gain > 1.0 violates physics
     * but is carefully controlled to remain stable.
     */
    void setParadoxGain(float normalized);

private:
    static constexpr int kNumPitchBands = 8; // Spectral bands for pitch evolution

    double sampleRateHz{48000.0};
    int maxBlockSizeInternal{2048};
    int numChannelsInternal{2};

    // Parameter targets and smoothers
    float impossibilityDegreeTarget{0.3f};     // [0, 1]
    float pitchEvolutionRateTarget{0.3f};      // [0, 1]
    float paradoxResonanceFreqTarget{0.5f};    // [0, 1]
    float paradoxGainTarget{0.3f};             // [0, 1]

    ParameterSmoother impossibilitySmoother;
    ParameterSmoother pitchEvolutionRateSmoother;
    ParameterSmoother paradoxFreqSmoother;
    ParameterSmoother paradoxGainSmoother;

    // Pitch evolution: Allpass cascade for frequency-dependent phase shift
    // This creates spectral "rotation" where frequencies gradually shift
    std::array<juce::dsp::IIR::Filter<float>, kNumPitchBands> pitchEvolutionFilters;
    float pitchEvolutionPhase{0.0f};          // Slow LFO for phase modulation

    // Paradox resonance: Narrow peak that amplifies instead of decays
    juce::dsp::IIR::Filter<float> paradoxResonanceFilter;
    float paradoxFrequencyHz{432.0f};         // Current paradox freq (default: 432 Hz)
    float paradoxGain{1.0f};                  // Current gain (1.0 = unity, >1.0 = amplification)
    float lastCachedParadoxGain{-1.0f};       // Cache for coefficient updates

    // Non-local absorption: Frequency-dependent filter that drifts
    std::array<float, 16> absorptionCurve;    // Per-band absorption profile
    juce::dsp::IIR::Filter<float> absorptionFilter;
    float absorptionDriftPhase{0.0f};
    juce::AudioBuffer<float> wetBuffer;       // Pre-allocated buffer for absorption effect

    // Age tracking (for pitch evolution that responds to signal "age")
    float signalAgeSeconds{0.0f};

    /**
     * @brief Initialize pitch evolution filters (allpass cascade).
     * Each band gets a different center frequency for spectral rotation.
     */
    void initializePitchEvolutionFilters();

    /**
     * @brief Update pitch evolution phase and filter coefficients.
     * Called at block rate to create slow spectral morphing.
     */
    void updatePitchEvolution();

    /**
     * @brief Update paradox resonance filter based on current frequency and gain.
     */
    void updateParadoxResonance();

    /**
     * @brief Apply pitch evolution (spectral rotation) to buffer.
     */
    void applyPitchEvolution(juce::AudioBuffer<float>& buffer);

    /**
     * @brief Apply paradox resonance (impossible amplification) to buffer.
     */
    void applyParadoxResonance(juce::AudioBuffer<float>& buffer);

    /**
     * @brief Apply non-local absorption (drifting frequency-dependent damping).
     */
    void applyNonLocalAbsorption(juce::AudioBuffer<float>& buffer);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AlienAmplification)
};

} // namespace dsp
} // namespace monument
