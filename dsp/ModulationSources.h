// Monument Reverb - Modulation Sources
// LFOs, Envelope Followers, and modulation routing using juce::dsp

#pragma once

#include <JuceHeader.h>
#include <array>

namespace monument {

/**
 * Multi-waveform LFO using juce::dsp::Oscillator
 * Real-time safe with smooth parameter changes
 */
class ModulationLFO
{
public:
    enum class Waveform
    {
        Sine,
        Triangle,
        Sawtooth,
        Square,
        Random  // Sample & hold
    };

    ModulationLFO()
    {
        // Initialize with sine wave
        oscillator.initialise([](float phase) {
            return std::sin(phase);
        }, 128);  // Lookup table size
    }

    void prepare(double sampleRate, int maxBlockSize)
    {
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
        spec.numChannels = 1;

        oscillator.prepare(spec);

        // Initialize smoothers for parameter changes
        rateSmoothed.reset(sampleRate, 0.05);  // 50ms smoothing
        depthSmoothed.reset(sampleRate, 0.05);

        this->sampleRate = sampleRate;
    }

    void setWaveform(Waveform waveform)
    {
        currentWaveform = waveform;

        switch (waveform)
        {
            case Waveform::Sine:
                oscillator.initialise([](float phase) {
                    return std::sin(phase);
                }, 128);
                break;

            case Waveform::Triangle:
                oscillator.initialise([](float phase) {
                    return std::asin(std::sin(phase)) / (juce::MathConstants<float>::pi / 2.0f);
                }, 128);
                break;

            case Waveform::Sawtooth:
                oscillator.initialise([](float phase) {
                    return phase / juce::MathConstants<float>::pi;
                }, 128);
                break;

            case Waveform::Square:
                oscillator.initialise([](float phase) {
                    return phase < juce::MathConstants<float>::pi ? 1.0f : -1.0f;
                }, 128);
                break;

            case Waveform::Random:
                // Sample & hold - updated per cycle
                break;
        }
    }

    /** Set LFO rate in Hz (0.01 - 20 Hz typical) */
    void setRate(float rateHz)
    {
        rateSmoothed.setTargetValue(rateHz);
    }

    /** Set modulation depth (0-1) */
    void setDepth(float depth)
    {
        depthSmoothed.setTargetValue(juce::jlimit(0.0f, 1.0f, depth));
    }

    /** Get next modulation value (-depth to +depth) */
    float getNextValue()
    {
        const float rate = rateSmoothed.getNextValue();
        const float depth = depthSmoothed.getNextValue();

        oscillator.setFrequency(rate);

        float value = 0.0f;

        if (currentWaveform == Waveform::Random)
        {
            // Sample & hold - update at rate frequency
            sampleHoldPhase += rate / static_cast<float>(sampleRate);
            if (sampleHoldPhase >= 1.0f)
            {
                sampleHoldPhase -= 1.0f;
                sampleHoldValue = juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
            }
            value = sampleHoldValue;
        }
        else
        {
            value = oscillator.processSample(0.0f);  // Generate waveform
        }

        return value * depth;
    }

    /** Process entire block (more efficient for buffer filling) */
    void processBlock(float* output, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            output[i] = getNextValue();
        }
    }

    void reset()
    {
        oscillator.reset();
        sampleHoldPhase = 0.0f;
        sampleHoldValue = 0.0f;
    }

private:
    juce::dsp::Oscillator<float> oscillator;
    juce::SmoothedValue<float> rateSmoothed;
    juce::SmoothedValue<float> depthSmoothed;

    Waveform currentWaveform{Waveform::Sine};
    double sampleRate{48000.0};

    // For sample & hold
    float sampleHoldPhase{0.0f};
    float sampleHoldValue{0.0f};
};

/**
 * Audio envelope follower for dynamic modulation
 * Tracks amplitude envelope of input signal
 */
class EnvelopeFollower
{
public:
    EnvelopeFollower() = default;

    void prepare(double sampleRate)
    {
        this->sampleRate = sampleRate;
        setAttackTime(5.0f);   // 5ms attack
        setReleaseTime(100.0f); // 100ms release
    }

    /** Set attack time in milliseconds */
    void setAttackTime(float timeMs)
    {
        attackCoeff = calculateCoeff(timeMs);
    }

    /** Set release time in milliseconds */
    void setReleaseTime(float timeMs)
    {
        releaseCoeff = calculateCoeff(timeMs);
    }

    /** Process single sample
     *
     * @param input Input audio sample
     * @return Envelope value (0-1)
     */
    float processSample(float input)
    {
        const float inputAbs = std::abs(input);

        // Choose coefficient based on if envelope is rising or falling
        const float coeff = inputAbs > envelope ? attackCoeff : releaseCoeff;

        // One-pole filter
        envelope = coeff * inputAbs + (1.0f - coeff) * envelope;

        return envelope;
    }

    /** Process block of samples
     *
     * @param input Input buffer
     * @param output Output envelope buffer
     * @param numSamples Number of samples
     */
    void processBlock(const float* input, float* output, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            output[i] = processSample(input[i]);
        }
    }

    void reset()
    {
        envelope = 0.0f;
    }

    float getCurrentEnvelope() const { return envelope; }

private:
    float calculateCoeff(float timeMs) const
    {
        const float timeSeconds = timeMs / 1000.0f;
        const float timeSamples = timeSeconds * static_cast<float>(sampleRate);
        return 1.0f - std::exp(-1.0f / timeSamples);
    }

    double sampleRate{48000.0};
    float attackCoeff{0.0f};
    float releaseCoeff{0.0f};
    float envelope{0.0f};
};

/**
 * Modulation matrix for routing sources to destinations
 * Real-time safe with smooth transitions
 */
class ModulationMatrix
{
public:
    enum class Source
    {
        LFO1,
        LFO2,
        EnvFollower,
        Random,
        NumSources
    };

    enum class Destination
    {
        Time,
        Density,
        Gravity,
        Bloom,
        Mass,
        PillarShape,
        Warp,
        Drift,
        NumDestinations
    };

    struct Modulation
    {
        Source source{Source::LFO1};
        Destination destination{Destination::Time};
        float depth{0.0f};  // -1 to +1
        bool bipolar{false};  // true = -depth to +depth, false = 0 to +depth
    };

    void prepare(double sampleRate)
    {
        for (auto& smoother : depthSmoothers)
        {
            smoother.reset(sampleRate, 0.05);  // 50ms smoothing
        }
    }

    /** Add or update modulation routing */
    void setModulation(size_t slotIndex, const Modulation& mod)
    {
        if (slotIndex < kMaxModulations)
        {
            modulations[slotIndex] = mod;
            depthSmoothers[slotIndex].setTargetValue(mod.depth);
        }
    }

    /** Apply modulation to parameter value
     *
     * @param destination Which parameter to modulate
     * @param baseValue Base parameter value (0-1)
     * @param sourceValues Current modulation source values
     * @return Modulated value (clamped 0-1)
     */
    float applyModulation(
        Destination destination,
        float baseValue,
        const std::array<float, static_cast<size_t>(Source::NumSources)>& sourceValues)
    {
        float totalModulation = 0.0f;

        for (size_t i = 0; i < kMaxModulations; ++i)
        {
            const auto& mod = modulations[i];
            if (mod.destination != destination)
                continue;

            const float depth = depthSmoothers[i].getNextValue();
            const float sourceValue = sourceValues[static_cast<size_t>(mod.source)];

            if (mod.bipolar)
            {
                // Bipolar: -depth to +depth
                totalModulation += sourceValue * depth;
            }
            else
            {
                // Unipolar: 0 to +depth (convert -1..1 to 0..1)
                const float unipolar = (sourceValue + 1.0f) * 0.5f;
                totalModulation += unipolar * depth;
            }
        }

        return juce::jlimit(0.0f, 1.0f, baseValue + totalModulation);
    }

private:
    static constexpr size_t kMaxModulations = 16;
    std::array<Modulation, kMaxModulations> modulations;
    std::array<juce::SmoothedValue<float>, kMaxModulations> depthSmoothers;
};

/**
 * Complete modulation system integrating all sources
 */
class ModulationSystem
{
public:
    void prepare(double sampleRate, int maxBlockSize)
    {
        lfo1.prepare(sampleRate, maxBlockSize);
        lfo2.prepare(sampleRate, maxBlockSize);
        envFollower.prepare(sampleRate);
        modulationMatrix.prepare(sampleRate);

        this->sampleRate = sampleRate;

        // Configure default LFO settings
        lfo1.setWaveform(ModulationLFO::Waveform::Sine);
        lfo1.setRate(0.5f);  // 0.5 Hz
        lfo1.setDepth(0.5f);

        lfo2.setWaveform(ModulationLFO::Waveform::Triangle);
        lfo2.setRate(2.0f);  // 2 Hz
        lfo2.setDepth(0.3f);
    }

    /** Process modulation for one sample
     *
     * Call this once per sample to update all modulation sources
     *
     * @param audioInput Audio signal for envelope follower
     */
    void processSample(float audioInput)
    {
        // Update all sources
        sourceValues[static_cast<size_t>(ModulationMatrix::Source::LFO1)] = lfo1.getNextValue();
        sourceValues[static_cast<size_t>(ModulationMatrix::Source::LFO2)] = lfo2.getNextValue();
        sourceValues[static_cast<size_t>(ModulationMatrix::Source::EnvFollower)] =
            envFollower.processSample(audioInput) * 2.0f - 1.0f;  // Convert to -1..1
        sourceValues[static_cast<size_t>(ModulationMatrix::Source::Random)] =
            juce::Random::getSystemRandom().nextFloat() * 2.0f - 1.0f;
    }

    /** Get modulated parameter value */
    float getModulatedValue(ModulationMatrix::Destination dest, float baseValue)
    {
        return modulationMatrix.applyModulation(dest, baseValue, sourceValues);
    }

    // Access to components for configuration
    ModulationLFO& getLFO1() { return lfo1; }
    ModulationLFO& getLFO2() { return lfo2; }
    EnvelopeFollower& getEnvelopeFollower() { return envFollower; }
    ModulationMatrix& getMatrix() { return modulationMatrix; }

private:
    ModulationLFO lfo1;
    ModulationLFO lfo2;
    EnvelopeFollower envFollower;
    ModulationMatrix modulationMatrix;

    std::array<float, static_cast<size_t>(ModulationMatrix::Source::NumSources)> sourceValues{};
    double sampleRate{48000.0};
};

} // namespace monument
