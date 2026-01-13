#pragma once

#include "dsp/DspModule.h"
#include "dsp/ParameterSmoother.h"
#include <JuceHeader.h>
#include <array>
#include <vector>

namespace monument
{
namespace dsp
{

/**
 * @brief ElasticHallway models a room with deformable walls that respond to acoustic pressure.
 *
 * This module creates evolving geometry through:
 * - Energy-responsive wall deformation (louder input → more deformation)
 * - Slow elastic recovery (walls return to nominal shape over time)
 * - Modal resonances that shift with deformation
 * - Absorption drift (wall properties change over time)
 *
 * Unlike physical room modeling, this creates impossible physics where walls
 * push back against sound and geometry morphs smoothly without pitch shifts.
 *
 * NOTE: This module is designed to couple with Chambers (FDN reverb) to modulate
 * its delay times and feedback matrix, but can also work standalone as a modal filter.
 */
class ElasticHallway final : public DSPModule
{
public:
    ElasticHallway();
    ~ElasticHallway() override;

    void prepare(double sampleRate, int blockSize, int numChannels) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer) override;

    /**
     * @brief Set how much walls deform under pressure (0 = rigid, 1 = highly elastic).
     * @param normalized [0, 1]
     */
    void setElasticity(float normalized);

    /**
     * @brief Set wall recovery time constant (100ms - 5000ms).
     * @param normalized [0, 1] → maps to recovery time
     */
    void setRecoveryTime(float normalized);

    /**
     * @brief Set how much wall absorption changes over time (0 = fixed, 1 = drifting).
     * @param normalized [0, 1]
     */
    void setAbsorptionDrift(float normalized);

    /**
     * @brief Set degree of non-linear response (0 = linear, 1 = energy-dependent).
     * @param normalized [0, 1]
     */
    void setNonlinearity(float normalized);

    /**
     * @brief Get current wall deformation amount (for visualization or coupling).
     * @return float [-1, 1] where 0 = no deformation
     */
    float getCurrentDeformation() const noexcept { return elasticDeformation; }

    /**
     * @brief Get deformation-modified delay time multiplier.
     * Can be used to modulate external FDN delay times.
     * @return float [0.8, 1.2] - multiply base delay by this value
     */
    float getDelayTimeModulation() const noexcept;

private:
    /**
     * @brief Room mode (resonance) with frequency and amplitude tracking.
     */
    struct RoomMode
    {
        float baseFrequency{100.0f};       // Nominal modal frequency (Hz)
        float currentFrequency{100.0f};    // Deformation-modified frequency
        float lastCachedFrequency{-1.0f};  // Cache for coefficient updates
        float amplitude{0.0f};             // Modal amplitude (energy)
        juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                       juce::dsp::IIR::Coefficients<float>>
            filter; // Resonant filter for this mode
    };

    static constexpr int kNumModes = 8; // Number of room modes to simulate

    double sampleRateHz{48000.0};
    int maxBlockSizeInternal{2048};
    int numChannelsInternal{2};

    // Parameter targets and smoothers
    float elasticityTarget{0.5f};        // [0, 1]
    float recoveryTimeTarget{0.5f};      // [0, 1]
    float absorptionDriftTarget{0.3f};   // [0, 1]
    float nonlinearityTarget{0.3f};      // [0, 1]

    ParameterSmoother elasticitySmoother;
    ParameterSmoother recoveryTimeSmoother;
    ParameterSmoother absorptionDriftSmoother;
    ParameterSmoother nonlinearitySmoother;

    // Room geometry state
    float roomWidthMeters{10.0f};
    float roomHeightMeters{5.0f};
    float roomDepthMeters{15.0f};

    // Wall deformation state (block-rate updates)
    float elasticDeformation{0.0f};      // Current deformation: [-0.2, +0.2] (±20%)
    float internalPressure{0.0f};        // Accumulated RMS pressure
    float recoveryTimeSeconds{1.0f};     // How fast walls recover

    // Modal resonances
    std::array<RoomMode, kNumModes> roomModes;

    // Pre-allocated buffer for modal processing
    juce::AudioBuffer<float> modalBuffer;

    // Pressure tracking filter (exponential averaging)
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                   juce::dsp::IIR::Coefficients<float>>
        pressureFilter;

    // Absorption drift LFO state
    float absorptionDriftPhase{0.0f};
    float absorptionDriftRate{0.05f}; // Hz

    /**
     * @brief Compute room modal frequencies based on dimensions.
     * Uses rectangular room mode equation: f = (c/2) * sqrt((nx/Lx)^2 + (ny/Ly)^2 + (nz/Lz)^2)
     */
    void computeRoomModes();

    /**
     * @brief Update modal frequencies based on current wall deformation.
     * Deformation shifts frequencies (expanding room → lower frequencies).
     */
    void updateModalFrequencies();

    /**
     * @brief Update resonant filters for all room modes.
     */
    void updateModalFilters();

    /**
     * @brief Measure input signal pressure and update wall deformation.
     * Called once per block (block-rate processing).
     */
    void updateWallDeformation(const juce::AudioBuffer<float>& buffer);

    /**
     * @brief Apply modal resonances to audio buffer.
     * Each mode is a bandpass filter at the modal frequency.
     */
    void applyModalResonances(juce::AudioBuffer<float>& buffer);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ElasticHallway)
};

} // namespace dsp
} // namespace monument
