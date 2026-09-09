#pragma once

#include "dsp/AllpassDiffuser.h"
#include "dsp/DspModule.h"
#include "dsp/ParameterBuffers.h"
#include "dsp/ParameterSmoother.h"
#include "dsp/SpatialProcessor.h"

#include <array>
#include <memory>
#include <optional>
#include <vector>

namespace monument
{
namespace dsp
{
class Chambers final : public DSPModule
{
public:
    // Delay-clustering modes for the warp control: how the twelve FDN delay
    // lengths relate to one another (independent primes vs. harmonically
    // related ratios). Declared first so it's a complete type at the point
    // setWarpClusteringMode() below references it (class member declarations
    // aren't a complete-class context, unlike function bodies/default args,
    // so forward use fails to compile).
    enum class WarpClusteringMode
    {
        Incommensurate = 0,  // Prime-based, non-repeating (default)
        Harmonic2x,          // 2:1 harmonic ratios (clustered delays)
        Harmonic3x,          // 3:1 harmonic ratios (sparser, resonant)
        OctaveStack          // Octave-spaced delays (musical)
    };

    void prepare(double sampleRate, int blockSize, int numChannels) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer) override;

    /**
     * @brief Force a deterministic seed for the late-diffusion drift RNG (test-only).
     *
     * Call before prepare(). Production code never calls this, so production
     * behavior (each instance gets an unseeded, genuinely different drift
     * character) is unchanged unless a test explicitly opts in. Intended for
     * QA-harness scenario captures that need reproducible output run to run.
     */
    void setDeterministicDriftSeedForTesting(juce::int64 seed) noexcept { testDriftSeed = seed; }

    /// Current warp-clustering mode-switch mute gain (1.0 = unmuted). Exposed
    /// so tests can verify the click guard's envelope directly, sample by
    /// sample, instead of inferring it from the reverb tail's own level --
    /// which fluctuates enough between the FDN's 12 delay lines that a
    /// signal-level comparison alone can't reliably distinguish "muted" from
    /// "a naturally quiet moment in the decay."
    float getWarpClusteringMuteGainForTesting() const noexcept { return warpClusteringMuteGain; }

    /// Current per-line FDN delay lengths, in samples. Exposed so tests can
    /// verify a warp-clustering mode actually produces distinct per-line
    /// delays (a std::vector rather than std::array<float, kNumLines> so this
    /// declaration doesn't need kNumLines, a private constant, in scope yet).
    std::vector<float> getDelaySamplesForTesting() const { return { delaySamples.begin(), delaySamples.end() }; }

    // Per-sample parameter setters (accept ParameterBuffer for zipper-free automation)
    void setTime(const ParameterBuffer& time);
    void setMass(const ParameterBuffer& mass);
    void setDensity(const ParameterBuffer& density);
    void setBloom(const ParameterBuffer& bloom);
    void setGravity(const ParameterBuffer& gravity);

    // Block-rate parameters (will be migrated later if needed)
    void setWarp(float warp);
    void setDrift(float drift);
    void setFreeze(bool shouldFreeze);
    void setAdaptiveMatrixAmount(float amount);
    void setFeedbackSaturation(float amount);
    void setDelayJitter(float amount);
    
    // Ambient-reverb shaping controls: how the twelve delay lines relate to
    // each other, how echo density evolves over the decay, and how quickly
    // the reverb swells in after a transient.
    void setWarpClusteringMode(WarpClusteringMode mode);
    void setDensityEvolution(float evolution);  // Controls density buildup/decay over time
    void setAttackTime(float attackTimeNorm);   // Slow-attack reverb swell time

    /// Sets an external audio buffer for memory injection into the reverb network.
    ///
    /// LIFETIME GUARANTEES:
    /// - The pointer must remain valid until the next call to process() completes
    /// - Typical usage: Set immediately before calling process() in the same call stack
    /// - The buffer is accessed only during process() execution (same audio thread)
    /// - Set to nullptr to disable external injection
    ///
    /// THREAD SAFETY:
    /// - Must be called from the audio processing thread only
    /// - No synchronization needed as long as set-then-process is atomic
    ///
    /// Example usage (from PluginProcessor::processBlock):
    /// ```cpp
    /// chambers.setExternalInjection(&memoryEchoes.getRecallBuffer());
    /// chambers.process(buffer);  // Pointer valid during this call
    /// ```
    void setExternalInjection(const juce::AudioBuffer<float>* injectionBuffer);

    /// Returns the spatial processor for modulation control
    SpatialProcessor* getSpatialProcessor() noexcept { return spatialProcessor.get(); }

private:
    // Test-only deterministic seed for the drift RNG in prepare(); nullopt in
    // production, so juce::Random's own unseeded default constructor is used
    // (see setDeterministicDriftSeedForTesting()).
    std::optional<juce::int64> testDriftSeed;

    // Actually recomputes delaySamples[] for the given mode (clamped to
    // delayBufferLength so a large harmonic ratio, e.g. OctaveStack's 2048x,
    // can't push a read past the allocated delay-line buffer). Called from the
    // middle of the mute window set up by setWarpClusteringMode(), not directly.
    void applyWarpClusteringMode(WarpClusteringMode mode);

    // Number of parallel delay lines in the feedback delay network.
    static constexpr int kNumLines = 12;
    double sampleRateHz = 44100.0;
    int maxBlockSize = 0;
    int channels = 0;
    juce::AudioBuffer<float> delayLines;
    std::array<float, kNumLines> delaySamples{};
    std::array<int, kNumLines> writePositions{};
    std::array<float, kNumLines> lowpassState{};
    std::array<float, kNumLines> gravityLowpassState{};
    std::array<float, kNumLines> dcBlockerLowpassState{};  // DC blocker state (<5Hz)
    std::array<float, kNumLines> dampingCoefficients{};
    float inputDCBlockerMidState = 0.0f;     // Input DC blocker for mid signal
    float inputDCBlockerSideState = 0.0f;    // Input DC blocker for side signal
    float outputDCBlockerLeftState = 0.0f;   // Output DC blocker for left channel
    float outputDCBlockerRightState = 0.0f;  // Output DC blocker for right channel
    int delayBufferLength = 0;
    float meanDelaySeconds = 0.0f;

    // Per-sample parameter buffers (eliminate double smoothing): lightweight
    // 16-byte views set via setXXX() and consumed in process()
    ParameterBuffer timeBuffer;
    ParameterBuffer massBuffer;
    ParameterBuffer densityBuffer;
    ParameterBuffer bloomBuffer;
    ParameterBuffer gravityBuffer;

    // Block-rate smoothers (will be migrated later if needed)
    ParameterSmoother warpSmoother;
    ParameterSmoother driftSmoother;

    // Per-sample smoothing for diffuser coefficients to prevent clicks
    std::array<juce::SmoothedValue<float>, 2> inputDiffuserCoeffSmoothers;
    std::array<juce::SmoothedValue<float>, kNumLines> lateDiffuserCoeffSmoothers;
    std::array<juce::SmoothedValue<float>, kNumLines> feedbackDiffuserCoeffSmoothers;
    float lastInputCoeffTarget = 0.0f;
    float lastLateCoeffBase = 0.0f;
    float lastFeedbackCoeffBase = 0.0f;

    // Target values are no longer needed for per-sample parameters (stored in ParameterBuffer)
    // Kept for block-rate parameters only
    float warpTarget = 0.0f;
    float driftTarget = 0.0f;
    float warpSmoothed = 0.0f;
    float adaptiveMatrixAmount = 0.0f;
    float feedbackSaturationAmount = 0.0f;
    float delayJitterAmount = 0.0f;
    
    // Ambient reverb shaping state
    WarpClusteringMode warpClusteringMode = WarpClusteringMode::Incommensurate;
    // Recomputing delaySamples[] on a mode change is a discontinuous jump in a
    // live feedback delay network, so the swap is deferred to the middle of a
    // brief mute window instead of applied instantly (see applyWarpClusteringMode
    // and the per-sample mute-gain logic in process()).
    WarpClusteringMode lastRequestedWarpClusteringMode = WarpClusteringMode::Incommensurate;
    WarpClusteringMode pendingWarpClusteringMode = WarpClusteringMode::Incommensurate;
    int warpClusteringMuteHalfSamples = 0;   // half-window length in samples, set in prepare()
    int warpClusteringMuteCounter = -1;      // -1 = idle; else counts down from 2*halfSamples
    float warpClusteringMuteGain = 1.0f;     // current output multiplier (triangular fade)
    float densityEvolutionTarget = 0.0f;  // -1 (decreasing) to +1 (increasing) density over time
    float attackTimeTarget = 0.0f;         // 0 (instant) to 1 (slow ~10s attack)
    juce::SmoothedValue<float> densityEvolutionSmoother;
    juce::SmoothedValue<float> attackTimeSmoother;
    float attackEnvelopeValue = 1.0f;      // Current attack envelope state
    float attackEnvelopeRate = 0.0f;       // Attack envelope increment per sample
    // Multiplies densityNorm on the *next* sample (one-sample lag; updated from
    // the current sample's envelopeTimeSeconds, which the diffusion-strength
    // calculations for this sample have already consumed by the time it's known).
    float densityEvolutionFactor = 1.0f;
    
    juce::SmoothedValue<float> adaptiveWarpOffsetSmoother;
    juce::Random jitterRandom;
    float lastMatrixBlend = 1.0f;
    float driftDepthMaxSamples = 1.0f;
    float gravityCoeffMin = 1.0f;
    float gravityCoeffMax = 1.0f;
    float dcBlockerCoeff = 1.0f;  // DC blocker coefficient (5Hz cutoff)
    std::array<juce::SmoothedValue<float>, kNumLines> jitterSmoothers;
    std::array<float, kNumLines> jitterTargets{};
    bool smoothersPrimed = false;
    bool isFrozen = false;
    bool wasFrozen = false;
    bool freezeRampingDown = false;
    int freezeRampSamples = 0;
    int freezeOutputFadeSamples = 0;
    int freezeRampRemaining = 0;
    float freezeRampStep = 1.0f;
    float freezeBlend = 1.0f;
    const juce::AudioBuffer<float>* externalInjection = nullptr;

    float envelopeTimeSeconds = 0.0f;
    float envelopeValue = 1.0f;
    float envelopeResetThreshold = 1.0e-4f;
    bool envelopeTriggerArmed = true;

    std::array<AllpassDiffuser, 2> inputDiffusers;
    std::array<AllpassDiffuser, kNumLines> lateDiffusers;
    std::array<AllpassDiffuser, kNumLines> feedbackDiffusers;
    // Output stereo decorrelation: one fixed allpass per channel, applied to the
    // wet signal only (see process()). Phase-only, so it doesn't affect RT60,
    // frequency response, or DC offset — it exists solely to break the
    // linear-combination relationship a fully-mixing feedback matrix otherwise
    // preserves between L and R for mono/dual-mono input.
    std::array<AllpassDiffuser, 2> outputDecorrelators;
    std::array<float, kNumLines> driftPhase{};
    std::array<float, kNumLines> driftRateHz{};
    alignas(juce::dsp::SIMDRegister<float>::SIMDRegisterSize)
        std::array<std::array<float, kNumLines>, kNumLines> warpMatrix{};
    alignas(juce::dsp::SIMDRegister<float>::SIMDRegisterSize)
        std::array<std::array<float, kNumLines>, kNumLines> warpMatrixFrozen{};
    alignas(juce::dsp::SIMDRegister<float>::SIMDRegisterSize)
        std::array<std::array<float, kNumLines>, kNumLines> feedbackMatrix{};

    // Spatial positioning system shared with the Ancient Monuments module
    std::unique_ptr<SpatialProcessor> spatialProcessor;
};

} // namespace dsp
} // namespace monument
