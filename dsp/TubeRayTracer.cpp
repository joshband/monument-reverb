#include "TubeRayTracer.h"
#include <cmath>
#include <random>

namespace monument
{
namespace dsp
{

TubeRayTracer::TubeRayTracer() = default;
TubeRayTracer::~TubeRayTracer() = default;

void TubeRayTracer::prepare(double sampleRate, int blockSize, int numChannels)
{
    sampleRateHz = sampleRate;
    maxBlockSizeInternal = blockSize;
    numChannelsInternal = numChannels;

    // Pre-allocate tube network (max capacity)
    tubes.resize(kMaxTubes);
    for (auto& tube : tubes)
    {
        tube.modalFrequencies.reserve(5);
        tube.resonanceFilter.prepare({sampleRate, static_cast<juce::uint32>(blockSize),
                                      static_cast<juce::uint32>(numChannels)});
        tube.resonanceFilter.reset();
    }

    // Pre-allocate ray tracing buffers
    rayEnergies.resize(kRayCount, 0.0f);
    rayTubeIndices.resize(kRayCount, 0);

    // Pre-allocate audio buffer for tube mixing
    tubeOutputBuffer.setSize(numChannels, blockSize);

    // Pre-allocate coloration buffer (used in applyTubeColoration)
    colorationBuffer.setSize(numChannels, blockSize, false, false, true);

    // Initialize parameter smoothers
    radiusVariationSmoother.prepare(sampleRate);
    radiusVariationSmoother.setSmoothingTimeMs(50.0f); // 50ms smoothing
    metallicResonanceSmoother.prepare(sampleRate);
    metallicResonanceSmoother.setSmoothingTimeMs(100.0f); // 100ms smoothing
    couplingSmoother.prepare(sampleRate);
    couplingSmoother.setSmoothingTimeMs(50.0f);

    radiusVariationSmoother.setTarget(radiusVariationTarget);
    metallicResonanceSmoother.setTarget(metallicResonanceTarget);
    couplingSmoother.setTarget(couplingStrengthTarget);

    tubesNeedReconfiguration = true;
}

void TubeRayTracer::reset()
{
    for (auto& tube : tubes)
    {
        tube.currentEnergy = 0.0f;
        tube.resonanceFilter.reset();
    }

    std::fill(rayEnergies.begin(), rayEnergies.end(), 0.0f);
    std::fill(rayTubeIndices.begin(), rayTubeIndices.end(), 0);

    tubeOutputBuffer.clear();
}

void TubeRayTracer::process(juce::AudioBuffer<float>& buffer)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Reconfigure tubes if count changed
    if (tubesNeedReconfiguration)
    {
        reconfigureTubes();
        tubesNeedReconfiguration = false;
    }

    // Update parameters (smoothed)
    radiusVariationSmoother.setTarget(radiusVariationTarget);
    metallicResonanceSmoother.setTarget(metallicResonanceTarget);
    couplingSmoother.setTarget(couplingStrengthTarget);

    // Block-rate: Trace rays through tube network
    traceRays();

    // Sample-rate: Apply tube coloration with resonant filtering
    applyTubeColoration(buffer);
}

void TubeRayTracer::setTubeCount(float normalized)
{
    if (!std::isfinite(normalized))
        return;
    // Map [0, 1] → [5, 16] tubes
    int newCount = kMinTubes + static_cast<int>(normalized * (kMaxTubes - kMinTubes));
    newCount = juce::jlimit(kMinTubes, kMaxTubes, newCount);

    if (newCount != activeTubeCount)
    {
        tubeCountTarget = static_cast<float>(newCount);
        activeTubeCount = newCount;
        tubesNeedReconfiguration = true;
    }
}

void TubeRayTracer::setRadiusVariation(float normalized)
{
    if (!std::isfinite(normalized))
        return;
    radiusVariationTarget = juce::jlimit(0.0f, 1.0f, normalized);
}

void TubeRayTracer::setMetallicResonance(float normalized)
{
    if (!std::isfinite(normalized))
        return;
    metallicResonanceTarget = juce::jlimit(0.0f, 1.0f, normalized);
}

void TubeRayTracer::setCouplingStrength(float normalized)
{
    if (!std::isfinite(normalized))
        return;
    couplingStrengthTarget = juce::jlimit(0.0f, 1.0f, normalized);
}

void TubeRayTracer::reconfigureTubes()
{
    // Use deterministic random number generator (seeded in prepare, not here for RT safety)
    // For now, use a simple deterministic pattern based on tube index

    const float baseLength = 2.0f; // 2 meters base length
    const float baseDiameter = 25.0f; // 25mm base diameter

    for (int i = 0; i < activeTubeCount; ++i)
    {
        auto& tube = tubes[i];

        // Vary length using sine pattern for determinism
        float lengthVariation = 1.0f + radiusVariationTarget *
                                std::sin(i * juce::MathConstants<float>::pi / activeTubeCount);
        tube.lengthMeters = baseLength * lengthVariation;
        tube.lengthMeters = juce::jlimit(0.5f, 10.0f, tube.lengthMeters);

        // Vary diameter
        float diameterVariation = 1.0f + radiusVariationTarget *
                                  std::cos(i * juce::MathConstants<float>::pi / activeTubeCount);
        tube.diameterMM = baseDiameter * diameterVariation;
        tube.diameterMM = juce::jlimit(5.0f, 50.0f, tube.diameterMM);

        // Absorption increases with smaller diameter (narrow tubes lose more HF)
        tube.absorptionPerMeter = 0.05f + (50.0f - tube.diameterMM) / 50.0f * 0.15f;

        // Compute modal frequencies
        tube.modalFrequencies = computeModalFrequencies(tube.lengthMeters, tube.diameterMM);

        // Update resonance filter
        updateTubeResonanceFilter(tube);

        // Initialize energy
        tube.currentEnergy = 0.0f;
    }
}

std::vector<float> TubeRayTracer::computeModalFrequencies(float lengthMeters, float diameterMM) const
{
    // Helmholtz resonator: f = (c / 2π) * sqrt(A / (V * L))
    // For cylindrical tube: fundamental frequency and harmonics

    const float speedOfSound = 343.0f; // m/s at 20°C
    const float fundamentalFreq = speedOfSound / (2.0f * lengthMeters);

    std::vector<float> modes;
    modes.reserve(5);

    // Generate first 5 harmonics
    for (int harmonic = 1; harmonic <= 5; ++harmonic)
    {
        float freq = fundamentalFreq * harmonic;

        // Clamp to audible range
        if (freq >= 20.0f && freq <= 20000.0f)
            modes.push_back(freq);
    }

    // Add diameter-dependent resonance (cross-sectional mode)
    float diameterMeters = diameterMM / 1000.0f;
    float crossSectionalMode = speedOfSound / (juce::MathConstants<float>::pi * diameterMeters);
    if (crossSectionalMode >= 20.0f && crossSectionalMode <= 20000.0f)
        modes.push_back(crossSectionalMode);

    return modes;
}

void TubeRayTracer::updateTubeResonanceFilter(Tube& tube)
{
    // Create resonant peak at fundamental frequency
    if (tube.modalFrequencies.empty())
        return;

    const float fundamentalFreq = tube.modalFrequencies[0];

    // Map metallicResonance [0, 1] → Q [1.0, 10.0]
    const float resonanceQ = 1.0f + metallicResonanceTarget * 9.0f;

    // Only update coefficients when frequency changes significantly (avoid unnecessary allocations)
    static constexpr float kFreqUpdateThreshold = 1.0f;  // 1 Hz threshold
    if (std::abs(fundamentalFreq - tube.lastCachedFundamentalFreq) > kFreqUpdateThreshold)
    {
        // Create bandpass filter at fundamental frequency
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(
            sampleRateHz, fundamentalFreq, resonanceQ);

        *tube.resonanceFilter.state = *coeffs;
        tube.lastCachedFundamentalFreq = fundamentalFreq;
    }
}

void TubeRayTracer::traceRays()
{
    // Block-rate ray tracing: distribute energy across tubes

    // Initialize ray positions if needed
    if (!raysInitialized)
    {
        for (int ray = 0; ray < kRayCount; ++ray)
        {
            rayTubeIndices[ray] = ray % activeTubeCount;
            rayEnergies[ray] = 1.0f / kRayCount; // Equal initial energy
        }
        raysInitialized = true;
    }

    // Reset tube energies
    for (int i = 0; i < activeTubeCount; ++i)
        tubes[i].currentEnergy = 0.0f;

    const float coupling = couplingSmoother.getNextValue();

    // Propagate rays with coupling between adjacent tubes
    for (int ray = 0; ray < kRayCount; ++ray)
    {
        int currentTube = rayTubeIndices[ray];
        float energy = rayEnergies[ray];

        // Apply absorption (energy loss)
        const float absorption = tubes[currentTube].absorptionPerMeter *
                                tubes[currentTube].lengthMeters;
        energy *= std::exp(-absorption);

        // Accumulate energy in current tube
        tubes[currentTube].currentEnergy += energy;

        // Transfer to adjacent tube (coupling)
        if (coupling > 0.01f)
        {
            // Probabilistic jump to adjacent tube
            const float jumpProbability = coupling * 0.3f; // 30% max jump chance

            // Deterministic "random" based on ray index and tube count
            float pseudoRandom = std::sin(ray * 12.9898f + currentTube * 78.233f);
            pseudoRandom = pseudoRandom - std::floor(pseudoRandom); // [0, 1]

            if (pseudoRandom < jumpProbability && activeTubeCount > 1)
            {
                // Jump to adjacent tube
                int direction = (pseudoRandom < jumpProbability / 2.0f) ? -1 : 1;
                int nextTube = currentTube + direction;

                // Wrap around
                if (nextTube < 0)
                    nextTube = activeTubeCount - 1;
                else if (nextTube >= activeTubeCount)
                    nextTube = 0;

                rayTubeIndices[ray] = nextTube;
            }
        }

        // Update ray energy
        rayEnergies[ray] = energy;
    }

    // Normalize tube energies
    float totalEnergy = 0.0f;
    for (int i = 0; i < activeTubeCount; ++i)
        totalEnergy += tubes[i].currentEnergy;

    if (totalEnergy > 0.001f)
    {
        for (int i = 0; i < activeTubeCount; ++i)
            tubes[i].currentEnergy /= totalEnergy;
    }
}

void TubeRayTracer::applyTubeColoration(juce::AudioBuffer<float>& buffer)
{
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    tubeOutputBuffer.clear();

    // For each active tube, apply resonance filtering weighted by energy
    for (int tubeIdx = 0; tubeIdx < activeTubeCount; ++tubeIdx)
    {
        auto& tube = tubes[tubeIdx];

        if (tube.currentEnergy < 0.001f)
            continue; // Skip tubes with negligible energy

        // Update resonance filter based on current metallic resonance setting
        updateTubeResonanceFilter(tube);

        // Reuse pre-allocated colorationBuffer (no allocation)
        colorationBuffer.clear();

        // Copy input to coloration buffer
        for (int ch = 0; ch < numChannels; ++ch)
            colorationBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);

        // Apply resonance filter
        juce::dsp::AudioBlock<float> block(colorationBuffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        tube.resonanceFilter.process(context);

        // Mix into output buffer with energy-based gain
        for (int ch = 0; ch < numChannels; ++ch)
        {
            tubeOutputBuffer.addFrom(ch, 0, colorationBuffer, ch, 0, numSamples,
                                     tube.currentEnergy);
        }
    }

    // Blend tube coloration with dry signal (50/50 mix for now)
    const float dryGain = 0.5f;
    const float wetGain = 0.5f;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* channelData = buffer.getWritePointer(ch);
        const auto* tubeData = tubeOutputBuffer.getReadPointer(ch);

        for (int sample = 0; sample < numSamples; ++sample)
        {
            channelData[sample] = channelData[sample] * dryGain + tubeData[sample] * wetGain;
        }
    }
}

} // namespace dsp
} // namespace monument
