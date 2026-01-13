#include "ElasticHallway.h"
#include <cmath>

namespace monument
{
namespace dsp
{

ElasticHallway::ElasticHallway() = default;
ElasticHallway::~ElasticHallway() = default;

void ElasticHallway::prepare(double sampleRate, int blockSize, int numChannels)
{
    sampleRateHz = sampleRate;
    maxBlockSizeInternal = blockSize;
    numChannelsInternal = numChannels;

    // Prepare modal filters
    for (auto& mode : roomModes)
    {
        mode.filter.prepare({sampleRate, static_cast<juce::uint32>(blockSize),
                            static_cast<juce::uint32>(numChannels)});
        mode.filter.reset();
    }

    // Pre-allocate modal buffer for applyModalResonances
    modalBuffer.setSize(numChannels, blockSize, false, false, true);

    // Prepare pressure tracking filter (low-pass for RMS smoothing)
    auto pressureCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(
        sampleRate, 2.0, 0.707); // 2 Hz cutoff for slow pressure tracking
    *pressureFilter.state = *pressureCoeffs;
    pressureFilter.prepare({sampleRate, static_cast<juce::uint32>(blockSize),
                           static_cast<juce::uint32>(numChannels)});
    pressureFilter.reset();

    // Initialize parameter smoothers
    elasticitySmoother.prepare(sampleRate);
    elasticitySmoother.setSmoothingTimeMs(100.0f); // 100ms smoothing
    recoveryTimeSmoother.prepare(sampleRate);
    recoveryTimeSmoother.setSmoothingTimeMs(200.0f); // 200ms smoothing
    absorptionDriftSmoother.prepare(sampleRate);
    absorptionDriftSmoother.setSmoothingTimeMs(100.0f);
    nonlinearitySmoother.prepare(sampleRate);
    nonlinearitySmoother.setSmoothingTimeMs(100.0f);

    elasticitySmoother.setTarget(elasticityTarget);
    recoveryTimeSmoother.setTarget(recoveryTimeTarget);
    absorptionDriftSmoother.setTarget(absorptionDriftTarget);
    nonlinearitySmoother.setTarget(nonlinearityTarget);

    // Compute initial room modes
    computeRoomModes();
    updateModalFrequencies();
    updateModalFilters();
}

void ElasticHallway::reset()
{
    elasticDeformation = 0.0f;
    internalPressure = 0.0f;
    absorptionDriftPhase = 0.0f;

    for (auto& mode : roomModes)
    {
        mode.amplitude = 0.0f;
        mode.filter.reset();
    }

    pressureFilter.reset();
}

void ElasticHallway::process(juce::AudioBuffer<float>& buffer)
{
    juce::ScopedNoDenormals noDenormals;

    // Update parameters (smoothed)
    elasticitySmoother.setTarget(elasticityTarget);
    recoveryTimeSmoother.setTarget(recoveryTimeTarget);
    absorptionDriftSmoother.setTarget(absorptionDriftTarget);
    nonlinearitySmoother.setTarget(nonlinearityTarget);

    // Block-rate: Update wall deformation based on input pressure
    updateWallDeformation(buffer);

    // Block-rate: Update modal frequencies based on deformation
    updateModalFrequencies();
    updateModalFilters();

    // Sample-rate: Apply modal resonances
    applyModalResonances(buffer);
}

void ElasticHallway::setElasticity(float normalized)
{
    elasticityTarget = juce::jlimit(0.0f, 1.0f, normalized);
}

void ElasticHallway::setRecoveryTime(float normalized)
{
    recoveryTimeTarget = juce::jlimit(0.0f, 1.0f, normalized);

    // Map [0, 1] → [100ms, 5000ms]
    recoveryTimeSeconds = 0.1f + normalized * 4.9f;
}

void ElasticHallway::setAbsorptionDrift(float normalized)
{
    absorptionDriftTarget = juce::jlimit(0.0f, 1.0f, normalized);

    // Map [0, 1] → [0.01 Hz, 0.2 Hz] (very slow drift)
    absorptionDriftRate = 0.01f + normalized * 0.19f;
}

void ElasticHallway::setNonlinearity(float normalized)
{
    nonlinearityTarget = juce::jlimit(0.0f, 1.0f, normalized);
}

float ElasticHallway::getDelayTimeModulation() const noexcept
{
    // Deformation affects room size, which affects delay times
    // Positive deformation (expansion) → longer delays
    // Negative deformation (compression) → shorter delays
    // Map deformation [-0.2, +0.2] → delay multiplier [0.8, 1.2]
    return 1.0f + elasticDeformation;
}

void ElasticHallway::computeRoomModes()
{
    // Rectangular room mode equation:
    // f_nxnynz = (c/2) * sqrt((nx/Lx)^2 + (ny/Ly)^2 + (nz/Lz)^2)
    // where c = speed of sound (343 m/s)

    const float speedOfSound = 343.0f; // m/s

    // Generate first 8 modes (low-order combinations of nx, ny, nz)
    const std::array<std::array<int, 3>, 8> modeIndices = {{
        {1, 0, 0}, // Axial modes
        {0, 1, 0},
        {0, 0, 1},
        {1, 1, 0}, // Tangential modes
        {1, 0, 1},
        {0, 1, 1},
        {1, 1, 1}, // Oblique mode
        {2, 0, 0}  // Second axial mode
    }};

    for (size_t i = 0; i < kNumModes; ++i)
    {
        int nx = modeIndices[i][0];
        int ny = modeIndices[i][1];
        int nz = modeIndices[i][2];

        float term1 = (nx / roomWidthMeters) * (nx / roomWidthMeters);
        float term2 = (ny / roomHeightMeters) * (ny / roomHeightMeters);
        float term3 = (nz / roomDepthMeters) * (nz / roomDepthMeters);

        float frequency = (speedOfSound / 2.0f) * std::sqrt(term1 + term2 + term3);

        // Clamp to audible range
        frequency = juce::jlimit(20.0f, 20000.0f, frequency);

        roomModes[i].baseFrequency = frequency;
        roomModes[i].currentFrequency = frequency;
        roomModes[i].amplitude = 0.0f;
    }
}

void ElasticHallway::updateModalFrequencies()
{
    // Deformation shifts modal frequencies
    // Expansion (positive deformation) → lower frequencies
    // Compression (negative deformation) → higher frequencies

    const float elasticity = elasticitySmoother.getNextValue();

    for (auto& mode : roomModes)
    {
        // Frequency shift proportional to deformation
        // deformation = 0.1 → 10% expansion → frequencies drop by ~5%
        float frequencyMultiplier = 1.0f - (elasticDeformation * elasticity * 0.5f);
        frequencyMultiplier = juce::jlimit(0.7f, 1.3f, frequencyMultiplier);

        mode.currentFrequency = mode.baseFrequency * frequencyMultiplier;
        mode.currentFrequency = juce::jlimit(20.0f, 20000.0f, mode.currentFrequency);
    }
}

void ElasticHallway::updateModalFilters()
{
    // Q factor depends on absorption drift
    const float absorptionDrift = absorptionDriftSmoother.getNextValue();

    // Absorption drift modulates Q over time (LFO)
    absorptionDriftPhase += absorptionDriftRate *
                            (maxBlockSizeInternal / static_cast<float>(sampleRateHz));
    if (absorptionDriftPhase > juce::MathConstants<float>::twoPi)
        absorptionDriftPhase -= juce::MathConstants<float>::twoPi;

    float driftModulation = std::sin(absorptionDriftPhase) * absorptionDrift;

    for (auto& mode : roomModes)
    {
        // Base Q: 5.0 (moderate resonance)
        // Drift adds ±30% variation
        float baseQ = 5.0f;
        float Q = baseQ * (1.0f + driftModulation * 0.3f);
        Q = juce::jlimit(1.0f, 15.0f, Q);

        // Only update coefficients when frequency changes significantly (avoid unnecessary allocations)
        static constexpr float kFreqUpdateThreshold = 0.5f;  // 0.5 Hz threshold
        if (std::abs(mode.currentFrequency - mode.lastCachedFrequency) > kFreqUpdateThreshold)
        {
            // Create bandpass filter at modal frequency
            auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(
                sampleRateHz, mode.currentFrequency, Q);

            *mode.filter.state = *coeffs;
            mode.lastCachedFrequency = mode.currentFrequency;
        }
    }
}

void ElasticHallway::updateWallDeformation(const juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Measure RMS across all channels (pressure proxy)
    float sumSquares = 0.0f;
    int totalSamples = 0;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* channelData = buffer.getReadPointer(ch);
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float s = channelData[sample];
            sumSquares += s * s;
            ++totalSamples;
        }
    }

    float rms = totalSamples > 0 ? std::sqrt(sumSquares / totalSamples) : 0.0f;

    // Apply non-linearity: energy-dependent response
    const float nonlinearity = nonlinearitySmoother.getNextValue();
    if (nonlinearity > 0.01f)
    {
        // Compress loud signals (walls resist high pressure)
        float compressed = rms / (1.0f + nonlinearity * rms);
        rms = rms * (1.0f - nonlinearity) + compressed * nonlinearity;
    }

    // Update internal pressure with exponential smoothing (slow tracking)
    const float pressureAlpha = 0.1f; // Slow pressure buildup
    internalPressure = internalPressure * (1.0f - pressureAlpha) + rms * pressureAlpha;

    // Calculate target deformation based on pressure
    const float elasticity = elasticitySmoother.getNextValue();
    float targetDeformation = internalPressure * elasticity * 2.0f; // Scale factor

    // Clamp deformation to ±20%
    targetDeformation = juce::jlimit(-0.2f, 0.2f, targetDeformation);

    // Apply elastic recovery (walls return to nominal shape)
    const float recoveryRate = 1.0f / (recoveryTimeSeconds * sampleRateHz / numSamples);
    elasticDeformation += (targetDeformation - elasticDeformation) * recoveryRate;

    // Safety clamp
    elasticDeformation = juce::jlimit(-0.2f, 0.2f, elasticDeformation);
}

void ElasticHallway::applyModalResonances(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Reuse pre-allocated modalBuffer (no allocation)

    // Apply each modal filter and accumulate
    for (auto& mode : roomModes)
    {
        // Copy input to modal buffer
        for (int ch = 0; ch < numChannels; ++ch)
            modalBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

        // Apply modal filter
        juce::dsp::AudioBlock<float> block(modalBuffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        mode.filter.process(context);

        // Mix back into main buffer with mode-dependent gain
        // Lower frequency modes have more energy in typical rooms
        float modeGain = 0.15f / (1.0f + mode.baseFrequency / 500.0f);

        for (int ch = 0; ch < numChannels; ++ch)
            buffer.addFrom(ch, 0, modalBuffer, ch, 0, numSamples, modeGain);
    }

    // Apply wet/dry mix
    // 30% wet (modal resonances), 70% dry (pass-through)
    const float wetGain = 0.3f;
    buffer.applyGain(0.7f + wetGain);
}

} // namespace dsp
} // namespace monument
