// Monument Reverb - Memory System
// Infinite feedback and cascading echo system using juce::dsp patterns

#pragma once

#include <JuceHeader.h>
#include <array>

namespace monument {

/**
 * Memory System for infinite feedback and cascading echoes
 *
 * Creates self-sustaining reverb tails that can persist indefinitely
 * Uses feedback limiting and spectral shaping for stability
 */
class MemorySystem
{
public:
    MemorySystem() = default;

    void prepare(double sampleRate, int maxBlockSize, int numChannels)
    {
        juce::(void)maxBlockSize;  // Unused - pre-allocated in constructor
        this->sampleRate = sampleRate;
        this->numChannels = numChannels;

        // Initialize memory buffers (10 seconds per channel)
        const int memoryBufferSize = static_cast<int>(sampleRate * 10.0);
        memoryBuffer.setSize(numChannels, memoryBufferSize);
        memoryBuffer.clear();

        writePosition = 0;

        // Initialize smoothers
        memoryAmountSmoothed.reset(sampleRate, 0.1);    // 100ms
        memoryDecaySmoothed.reset(sampleRate, 0.1);
        memoryDriftSmoothed.reset(sampleRate, 0.1);

        // Initialize feedback limiter using juce::dsp::Limiter
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
        spec.numChannels = static_cast<juce::uint32>(numChannels);

        feedbackLimiter.prepare(spec);
        feedbackLimiter.setThreshold(-3.0f);   // -3dB threshold
        feedbackLimiter.setRelease(50.0f);      // 50ms release

        // Initialize spectral shaper (one-pole lowpass for stability)
        reset();
    }

    /** Set memory amount (0-1)
     *
     * 0 = no infinite feedback
     * 1 = maximum self-sustaining feedback
     */
    void setMemoryAmount(float amount)
    {
        memoryAmountSmoothed.setTargetValue(juce::jlimit(0.0f, 1.0f, amount));
    }

    /** Set memory decay rate (0-1)
     *
     * Controls how quickly the memory tail fades
     * 0 = fast decay, 1 = very slow decay (nearly infinite)
     */
    void setMemoryDecay(float decay)
    {
        memoryDecaySmoothed.setTargetValue(juce::jlimit(0.0f, 1.0f, decay));
    }

    /** Set memory drift (0-1)
     *
     * Adds pitch instability to cascading echoes
     * Creates tape-like wow/flutter effects
     */
    void setMemoryDrift(float drift)
    {
        memoryDriftSmoothed.setTargetValue(juce::jlimit(0.0f, 1.0f, drift));
    }

    /** Process block with memory feedback
     *
     * @param buffer Audio buffer to process (input + output)
     * @param reverbTail Reverb tail from main algorithm to feed into memory
     */
    void process(juce::AudioBuffer<float>& buffer,
                 const juce::AudioBuffer<float>& reverbTail)
    {
        juce::ScopedNoDenormals noDenormals;

        const int numSamples = buffer.getNumSamples();
        const int bufferSize = memoryBuffer.getNumSamples();

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float memoryAmount = memoryAmountSmoothed.getNextValue();
            const float memoryDecay = memoryDecaySmoothed.getNextValue();
            const float memoryDrift = memoryDriftSmoothed.getNextValue();

            // Calculate feedback coefficient (with safety limiting)
            // Maps decay 0-1 to feedback 0.5-0.995
            const float feedbackCoeff = juce::jmap(memoryDecay, 0.5f, 0.995f);

            // Drift modulation for pitch instability
            const float driftAmount = memoryDrift * 0.5f;  // Max ±0.5 samples
            const float driftPhase = driftOscillator.getNextValue() * driftAmount;

            for (int ch = 0; ch < numChannels; ++ch)
            {
                // Read from memory with fractional delay (for drift)
                const float readPos = static_cast<float>(writePosition) + driftPhase;
                const float memorySample = readMemoryInterpolated(ch, readPos, bufferSize);

                // Get reverb tail input
                const float reverbSample = reverbTail.getSample(ch, sample);

                // Feedback path: memory → lowpass → feedback
                const float feedbackSample = applyStabilizingFilter(memorySample, ch);

                // Mix reverb tail + feedback
                const float mixedSample = reverbSample + feedbackSample * feedbackCoeff;

                // Write to memory buffer
                memoryBuffer.setSample(ch, writePosition, mixedSample);

                // Output: dry signal + memory contribution
                const float drySample = buffer.getSample(ch, sample);
                const float outputSample = drySample + memorySample * memoryAmount;
                buffer.setSample(ch, sample, outputSample);
            }

            // Advance write position (circular buffer)
            writePosition = (writePosition + 1) % bufferSize;

            // Update drift oscillator (slow LFO ~ 0.2 Hz)
            driftOscillator.setFrequency(0.2f);
        }

        // Apply safety limiter to output
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        feedbackLimiter.process(context);
    }

    /** Reset memory buffer (clear all stored audio) */
    void reset()
    {
        memoryBuffer.clear();
        writePosition = 0;

        // Reset filter states
        for (auto& state : lowpassStates)
            state = 0.0f;

        // Initialize drift oscillator
        driftOscillator.initialise([](float phase) {
            return std::sin(phase);
        }, 128);
        driftOscillator.prepare({sampleRate, 512, 1});
    }

    /** Get current memory buffer for visualization
     *
     * Useful for UI display of memory tail
     */
    const juce::AudioBuffer<float>& getMemoryBuffer() const
    {
        return memoryBuffer;
    }

private:
    /** Read from memory buffer with linear interpolation */
    float readMemoryInterpolated(int channel, float position, int bufferSize) const
    {
        // Wrap position
        while (position < 0.0f)
            position += static_cast<float>(bufferSize);
        while (position >= static_cast<float>(bufferSize))
            position -= static_cast<float>(bufferSize);

        const int idx0 = static_cast<int>(position);
        const int idx1 = (idx0 + 1) % bufferSize;
        const float frac = position - static_cast<float>(idx0);

        const float y0 = memoryBuffer.getSample(channel, idx0);
        const float y1 = memoryBuffer.getSample(channel, idx1);

        return y0 + frac * (y1 - y0);
    }

    /** Apply stabilizing lowpass filter to prevent buildup
     *
     * One-pole filter: y[n] = a*x[n] + (1-a)*y[n-1]
     * Cutoff ~4kHz to gradually darken infinite tail
     */
    float applyStabilizingFilter(float input, int channel)
    {
        // Calculate coefficient for ~4kHz cutoff
        const float cutoffHz = 4000.0f;
        const float rc = 1.0f / (juce::MathConstants<float>::twoPi * cutoffHz);
        const float dt = 1.0f / static_cast<float>(sampleRate);
        const float alpha = dt / (rc + dt);

        // One-pole lowpass
        lowpassStates[channel] = alpha * input + (1.0f - alpha) * lowpassStates[channel];
        return lowpassStates[channel];
    }

    // Audio buffers
    juce::AudioBuffer<float> memoryBuffer;
    int writePosition{0};

    // Parameters
    juce::SmoothedValue<float> memoryAmountSmoothed;
    juce::SmoothedValue<float> memoryDecaySmoothed;
    juce::SmoothedValue<float> memoryDriftSmoothed;

    // Processing components
    juce::dsp::Limiter<float> feedbackLimiter;
    juce::dsp::Oscillator<float> driftOscillator;

    // Filter states
    std::array<float, 2> lowpassStates{};

    // State
    double sampleRate{48000.0};
    int numChannels{2};
};

/**
 * Cascading Echo System
 *
 * Creates rhythmic cascading echoes with feedback
 * Can be used standalone or integrated with MemorySystem
 */
class CascadingEchos
{
public:
    static constexpr size_t kMaxEchoes = 16;

    struct EchoTap
    {
        float delaySeconds{0.0f};
        float feedback{0.5f};
        float panPosition{0.0f};  // -1 (left) to +1 (right)
        float filterCutoff{10000.0f};  // Hz
    };

    void prepare(double sampleRate, int maxBlockSize, int numChannels)
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
        spec.numChannels = static_cast<juce::uint32>(numChannels);

        this->sampleRate = sampleRate;

        // Initialize delay lines (max 2 seconds per tap)
        const int maxDelaySamples = static_cast<int>(sampleRate * 2.0);
        for (auto& delay : delayLines)
        {
            delay.setMaximumDelayInSamples(maxDelaySamples);
            delay.prepare(spec);
            delay.reset();
        }

        // Initialize filters
        for (auto& filter : filters)
        {
            filter.prepare(spec);
            filter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
            filter.setCutoffFrequency(10000.0f);
            filter.setResonance(0.7f);
        }
    }

    /** Configure echo taps */
    void setEchoTap(size_t tapIndex, const EchoTap& tap)
    {
        if (tapIndex < kMaxEchoes)
        {
            echoTaps[tapIndex] = tap;

            // Update delay line
            const float delaySamples = tap.delaySeconds * static_cast<float>(sampleRate);
            delayLines[tapIndex].setDelay(delaySamples);

            // Update filter
            filters[tapIndex].setCutoffFrequency(tap.filterCutoff);
        }
    }

    /** Process block with cascading echoes */
    void process(juce::AudioBuffer<float>& buffer)
    {
        juce::ScopedNoDenormals noDenormals;

        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);

        // Process each echo tap
        for (size_t i = 0; i < kMaxEchoes; ++i)
        {
            if (echoTaps[i].delaySeconds > 0.0f)
            {
                // Process delay
                delayLines[i].process(context);

                // Apply filtering
                filters[i].process(context);

                // Apply panning (simple constant power pan)
                applyPanning(buffer, echoTaps[i].panPosition);
            }
        }
    }

    void reset()
    {
        for (auto& delay : delayLines)
            delay.reset();
        for (auto& filter : filters)
            filter.reset();
    }

private:
    void applyPanning(juce::AudioBuffer<float>& buffer, float panPosition)
    {
        if (buffer.getNumChannels() < 2)
            return;

        // Constant power panning
        const float panRadians = (panPosition + 1.0f) * 0.25f * juce::MathConstants<float>::pi;
        const float leftGain = std::cos(panRadians);
        const float rightGain = std::sin(panRadians);

        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            const float mono = (buffer.getSample(0, sample) + buffer.getSample(1, sample)) * 0.5f;
            buffer.setSample(0, sample, mono * leftGain);
            buffer.setSample(1, sample, mono * rightGain);
        }
    }

    std::array<juce::dsp::DelayLine<float>, kMaxEchoes> delayLines;
    std::array<juce::dsp::StateVariableTPTFilter<float>, kMaxEchoes> filters;
    std::array<EchoTap, kMaxEchoes> echoTaps;

    double sampleRate{48000.0};
};

} // namespace monument
