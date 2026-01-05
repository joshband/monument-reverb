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
 * @brief TubeRayTracer simulates sound bouncing through a series of metal tubes.
 *
 * This module creates metallic coloration through:
 * - Distance-based frequency loss (high-frequency rolloff)
 * - Modal resonances at tube-specific frequencies (Helmholtz resonances)
 * - Ray-traced energy propagation through tube network
 * - Coupling between adjacent tubes
 *
 * Processing is block-rate for efficiency (ray tracing once per buffer).
 * Resonant filtering is sample-rate for audio quality.
 */
class TubeRayTracer final : public DSPModule
{
public:
    TubeRayTracer();
    ~TubeRayTracer() override;

    void prepare(double sampleRate, int blockSize, int numChannels) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer) override;

    /**
     * @brief Set number of virtual tubes (5-16).
     * @param normalized [0, 1] → 5-16 tubes
     */
    void setTubeCount(float normalized);

    /**
     * @brief Set variation in tube diameters (0 = uniform, 1 = highly varied).
     * @param normalized [0, 1]
     */
    void setRadiusVariation(float normalized);

    /**
     * @brief Set emphasis of metallic resonance peaks (0 = natural, 1 = pronounced).
     * @param normalized [0, 1] → resonance Q factor
     */
    void setMetallicResonance(float normalized);

    /**
     * @brief Set energy transfer between adjacent tubes (0 = isolated, 1 = strong coupling).
     * @param normalized [0, 1]
     */
    void setCouplingStrength(float normalized);

private:
    /**
     * @brief Represents a single tube in the network.
     */
    struct Tube
    {
        float lengthMeters{0.5f};           // Physical length: 0.5m - 10m
        float diameterMM{25.0f};            // Diameter: 5mm - 50mm
        float absorptionPerMeter{0.1f};     // High-frequency absorption coefficient
        std::vector<float> modalFrequencies; // Helmholtz resonance frequencies
        juce::dsp::IIR::Filter<float> resonanceFilter; // Modal resonance emphasis
        float currentEnergy{0.0f};          // Ray energy in this tube (block-rate)
        float lastCachedFundamentalFreq{-1.0f}; // Cache for coefficient updates
    };

    static constexpr int kMinTubes = 5;
    static constexpr int kMaxTubes = 16;
    static constexpr int kRayCount = 64; // Rays traced per block

    double sampleRateHz{48000.0};
    int maxBlockSizeInternal{2048};
    int numChannelsInternal{2};

    // Parameter targets and smoothers
    float tubeCountTarget{11.0f};         // [5, 16]
    float radiusVariationTarget{0.3f};    // [0, 1]
    float metallicResonanceTarget{0.5f};  // [0, 1]
    float couplingStrengthTarget{0.5f};   // [0, 1]

    ParameterSmoother radiusVariationSmoother;
    ParameterSmoother metallicResonanceSmoother;
    ParameterSmoother couplingSmoother;

    // Tube network
    std::vector<Tube> tubes;
    int activeTubeCount{11};
    bool tubesNeedReconfiguration{true};
    bool raysInitialized{false};

    // Ray tracing state (updated at block rate)
    std::vector<float> rayEnergies;      // Per-ray energy levels
    std::vector<int> rayTubeIndices;     // Which tube each ray is currently in

    // Audio processing buffers
    juce::AudioBuffer<float> tubeOutputBuffer; // Per-tube audio accumulation
    juce::AudioBuffer<float> colorationBuffer; // Pre-allocated temporary buffer for tube filtering

    /**
     * @brief Reconfigure tube network when tube count changes.
     * Pre-computes tube lengths, diameters, modal frequencies.
     */
    void reconfigureTubes();

    /**
     * @brief Compute Helmholtz resonance frequencies for a tube.
     * @param lengthMeters Tube length
     * @param diameterMM Tube diameter
     * @return Vector of modal frequencies (typically 3-5 modes)
     */
    std::vector<float> computeModalFrequencies(float lengthMeters, float diameterMM) const;

    /**
     * @brief Update resonance filter for a tube based on current metallicResonance.
     * @param tube Tube to update
     */
    void updateTubeResonanceFilter(Tube& tube);

    /**
     * @brief Trace rays through tube network (called once per block).
     * Updates per-tube energy levels based on ray propagation.
     */
    void traceRays();

    /**
     * @brief Apply tube resonances and absorption to audio buffer.
     * Uses per-tube energy to scale resonant filtering.
     */
    void applyTubeColoration(juce::AudioBuffer<float>& buffer);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TubeRayTracer)
};

} // namespace dsp
} // namespace monument