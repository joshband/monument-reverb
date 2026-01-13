#include "AlienAmplification.h"
#include <cmath>

namespace monument
{
namespace dsp
{

AlienAmplification::AlienAmplification() = default;
AlienAmplification::~AlienAmplification() = default;

void AlienAmplification::prepare(double sampleRate, int blockSize, int numChannels)
{
    sampleRateHz = sampleRate;
    maxBlockSizeInternal = blockSize;
    numChannelsInternal = numChannels;

    // Prepare pitch evolution filters (allpass cascade)
    for (auto& filter : pitchEvolutionFilters)
    {
        filter.prepare({sampleRate, static_cast<juce::uint32>(blockSize),
                       static_cast<juce::uint32>(numChannels)});
        filter.reset();
    }

    // Prepare paradox resonance filter
    paradoxResonanceFilter.prepare({sampleRate, static_cast<juce::uint32>(blockSize),
                                   static_cast<juce::uint32>(numChannels)});
    paradoxResonanceFilter.reset();

    // Prepare absorption filter
    absorptionFilter.prepare({sampleRate, static_cast<juce::uint32>(blockSize),
                             static_cast<juce::uint32>(numChannels)});
    absorptionFilter.reset();

    // Pre-allocate wet buffer for applyNonLocalAbsorption
    wetBuffer.setSize(numChannels, blockSize, false, false, true);

    // Initialize parameter smoothers
    impossibilitySmoother.prepare(sampleRate);
    impossibilitySmoother.setSmoothingTimeMs(200.0f); // 200ms smoothing
    pitchEvolutionRateSmoother.prepare(sampleRate);
    pitchEvolutionRateSmoother.setSmoothingTimeMs(150.0f);
    paradoxFreqSmoother.prepare(sampleRate);
    paradoxFreqSmoother.setSmoothingTimeMs(300.0f); // Slow freq changes
    paradoxGainSmoother.prepare(sampleRate);
    paradoxGainSmoother.setSmoothingTimeMs(100.0f);

    impossibilitySmoother.setTarget(impossibilityDegreeTarget);
    pitchEvolutionRateSmoother.setTarget(pitchEvolutionRateTarget);
    paradoxFreqSmoother.setTarget(paradoxResonanceFreqTarget);
    paradoxGainSmoother.setTarget(paradoxGainTarget);

    // Initialize filters
    initializePitchEvolutionFilters();
    updateParadoxResonance();

    // Initialize absorption curve (flat at start)
    absorptionCurve.fill(0.0f);
}

void AlienAmplification::reset()
{
    pitchEvolutionPhase = 0.0f;
    absorptionDriftPhase = 0.0f;
    signalAgeSeconds = 0.0f;

    for (auto& filter : pitchEvolutionFilters)
        filter.reset();

    paradoxResonanceFilter.reset();
    absorptionFilter.reset();
}

void AlienAmplification::process(juce::AudioBuffer<float>& buffer)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();

    // Update parameters (smoothed)
    impossibilitySmoother.setTarget(impossibilityDegreeTarget);
    pitchEvolutionRateSmoother.setTarget(pitchEvolutionRateTarget);
    paradoxFreqSmoother.setTarget(paradoxResonanceFreqTarget);
    paradoxGainSmoother.setTarget(paradoxGainTarget);

    // Update signal age (for pitch evolution)
    signalAgeSeconds += static_cast<float>(numSamples) / static_cast<float>(sampleRateHz);

    // Block-rate: Update filter coefficients
    updatePitchEvolution();
    updateParadoxResonance();

    // Sample-rate: Apply effects in series
    const float impossibility = impossibilitySmoother.getNextValue();

    if (impossibility > 0.01f)
    {
        // Apply pitch evolution (spectral rotation)
        applyPitchEvolution(buffer);

        // Apply paradox resonance (impossible amplification)
        applyParadoxResonance(buffer);

        // Apply non-local absorption (drifting frequency-dependent damping)
        applyNonLocalAbsorption(buffer);
    }
}

void AlienAmplification::setImpossibilityDegree(float normalized)
{
    impossibilityDegreeTarget = juce::jlimit(0.0f, 1.0f, normalized);
}

void AlienAmplification::setPitchEvolutionRate(float normalized)
{
    pitchEvolutionRateTarget = juce::jlimit(0.0f, 1.0f, normalized);
}

void AlienAmplification::setParadoxResonanceFreq(float normalized)
{
    paradoxResonanceFreqTarget = juce::jlimit(0.0f, 1.0f, normalized);

    // Map [0, 1] → [50 Hz, 5000 Hz] (logarithmic)
    float logMin = std::log(50.0f);
    float logMax = std::log(5000.0f);
    paradoxFrequencyHz = std::exp(logMin + normalized * (logMax - logMin));
}

void AlienAmplification::setParadoxGain(float normalized)
{
    paradoxGainTarget = juce::jlimit(0.0f, 1.0f, normalized);

    // Map [0, 1] → [1.0, 1.05] (careful: >1.0 can cause instability)
    paradoxGain = 1.0f + normalized * 0.05f;
}

void AlienAmplification::initializePitchEvolutionFilters()
{
    // Create allpass filters at different frequency bands
    // These create frequency-dependent phase shifts for spectral rotation

    const std::array<float, kNumPitchBands> centerFrequencies = {
        100.0f, 200.0f, 400.0f, 800.0f, 1600.0f, 3200.0f, 6400.0f, 12800.0f
    };

    for (size_t i = 0; i < kNumPitchBands; ++i)
    {
        float freq = centerFrequencies[i];
        float Q = 0.707f; // Butterworth response

        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeAllPass(
            sampleRateHz, freq, Q);

        *pitchEvolutionFilters[i].state = *coeffs;
    }
}

void AlienAmplification::updatePitchEvolution()
{
    const float pitchRate = pitchEvolutionRateSmoother.getNextValue();
    const float impossibility = impossibilitySmoother.getNextValue();

    // Update phase for slow LFO modulation of allpass frequencies
    // Rate: 0.01-0.2 Hz (very slow spectral morphing)
    float lfoRate = 0.01f + pitchRate * 0.19f;
    pitchEvolutionPhase += lfoRate * (static_cast<float>(maxBlockSizeInternal) / static_cast<float>(sampleRateHz));

    if (pitchEvolutionPhase > juce::MathConstants<float>::twoPi)
        pitchEvolutionPhase -= juce::MathConstants<float>::twoPi;

    // Modulate allpass frequencies with LFO
    // Higher impossibility → more dramatic frequency shifts
    const std::array<float, kNumPitchBands> baseFrequencies = {
        100.0f, 200.0f, 400.0f, 800.0f, 1600.0f, 3200.0f, 6400.0f, 12800.0f
    };

    for (size_t i = 0; i < kNumPitchBands; ++i)
    {
        // Each band gets a different phase offset for complex spectral motion
        float phaseOffset = i * juce::MathConstants<float>::pi / 4.0f;
        float modulation = std::sin(pitchEvolutionPhase + phaseOffset);

        // Frequency modulation: ±30% at maximum impossibility
        float freqMultiplier = 1.0f + modulation * impossibility * 0.3f;
        float modulatedFreq = baseFrequencies[i] * freqMultiplier;
        modulatedFreq = juce::jlimit(20.0f, 20000.0f, modulatedFreq);

        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeAllPass(
            sampleRateHz, modulatedFreq, 0.707f);

        *pitchEvolutionFilters[i].state = *coeffs;
    }
}

void AlienAmplification::updateParadoxResonance()
{
    // Update paradox resonance filter: narrow peak with gain > 1.0

    const float impossibility = impossibilitySmoother.getNextValue();

    // Q factor: higher impossibility → narrower peak (more pronounced effect)
    float Q = 5.0f + impossibility * 15.0f; // [5, 20]
    Q = juce::jlimit(5.0f, 20.0f, Q);

    // Only update coefficients when gain changes significantly (avoid unnecessary allocations)
    static constexpr float kGainUpdateThreshold = 0.5f;  // 0.5 dB threshold
    if (std::abs(paradoxGain - lastCachedParadoxGain) > kGainUpdateThreshold)
    {
        // Create resonant peak at paradox frequency
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(
            sampleRateHz, paradoxFrequencyHz, Q,
            juce::Decibels::decibelsToGain((paradoxGain - 1.0f) * 100.0f)); // Convert to dB gain

        *paradoxResonanceFilter.state = *coeffs;
        lastCachedParadoxGain = paradoxGain;
    }
}

void AlienAmplification::applyPitchEvolution(juce::AudioBuffer<float>& buffer)
{
    const float pitchRate = pitchEvolutionRateSmoother.getNextValue();

    if (pitchRate < 0.01f)
        return; // Skip if pitch evolution is disabled

    // Apply allpass cascade for spectral rotation
    // Each filter adds frequency-dependent phase shift
    for (auto& filter : pitchEvolutionFilters)
    {
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        filter.process(context);
    }

    // Mix with dry signal to control effect intensity
    // Higher pitch rate → more wet signal
    const float wetGain = pitchRate * 0.3f; // Max 30% wet
    const float dryGain = 1.0f - wetGain * 0.5f; // Slight dry reduction for balance

    buffer.applyGain(dryGain + wetGain);
}

void AlienAmplification::applyParadoxResonance(juce::AudioBuffer<float>& buffer)
{
    const float impossibility = impossibilitySmoother.getNextValue();

    if (impossibility < 0.01f || paradoxGain <= 1.001f)
        return; // Skip if effect is minimal

    // Apply paradox resonance: amplifies specific frequency
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    paradoxResonanceFilter.process(context);

    // Safety limiter: prevent runaway amplification
    // Soft clip peaks above 0.95 to maintain stability
    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* channelData = buffer.getWritePointer(ch);
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float s = channelData[sample];

            // Soft clipping (tanh-style) using fast approximation for real-time safety
            if (std::abs(s) > 0.95f)
            {
                s = 0.95f * juce::dsp::FastMathApproximations::tanh(s / 0.95f);
            }

            channelData[sample] = s;
        }
    }
}

void AlienAmplification::applyNonLocalAbsorption(juce::AudioBuffer<float>& buffer)
{
    const float impossibility = impossibilitySmoother.getNextValue();

    if (impossibility < 0.01f)
        return;

    // Update absorption drift phase (very slow, 0.02-0.1 Hz)
    float driftRate = 0.02f + impossibility * 0.08f;
    absorptionDriftPhase += driftRate * (static_cast<float>(maxBlockSizeInternal) / static_cast<float>(sampleRateHz));

    if (absorptionDriftPhase > juce::MathConstants<float>::twoPi)
        absorptionDriftPhase -= juce::MathConstants<float>::twoPi;

    // Modulate absorption curve with slow LFO
    // This creates time-varying frequency-dependent damping
    float absorption = 0.5f + 0.5f * std::sin(absorptionDriftPhase);
    absorption *= impossibility; // Scale by impossibility degree

    // Apply low-pass filter with drifting cutoff
    // Cutoff drifts between 2kHz and 10kHz
    float cutoffHz = 2000.0f + absorption * 8000.0f;
    cutoffHz = juce::jlimit(500.0f, 15000.0f, cutoffHz);

    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(
        sampleRateHz, cutoffHz, 0.707f);

    *absorptionFilter.state = *coeffs;

    // Apply filter with wet/dry mix using pre-allocated buffer
    wetBuffer.clear();

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        wetBuffer.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());

    juce::dsp::AudioBlock<float> block(wetBuffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    absorptionFilter.process(context);

    // Mix: 20% wet at maximum impossibility
    float wetGain = impossibility * 0.2f;
    float dryGain = 1.0f - wetGain;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* channelData = buffer.getWritePointer(ch);
        const auto* wetData = wetBuffer.getReadPointer(ch);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            channelData[sample] = channelData[sample] * dryGain + wetData[sample] * wetGain;
        }
    }
}

} // namespace dsp
} // namespace monument
