#pragma once

#include "dsp/DspModule.h"
#include "dsp/ParameterBuffers.h"

#include <array>

namespace monument
{
namespace dsp
{
class Pillars final : public DSPModule
{
public:
    enum class Mode
    {
        Glass = 0,
        Stone = 1,
        Fog = 2
    };

    void prepare(double sampleRate, int blockSize, int numChannels) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void setDensity(float density);
    void setShape(const ParameterBuffer& shape);  // Phase 4: Per-sample parameter
    void setMode(int modeIndex);
    void setWarp(float warp);

    /**
     * @brief Load impulse response for pseudo-convolution of early taps
     *
     * ⚠️ CRITICAL: MUST be called off-audio-thread only
     *
     * Safe to call from:
     * - GUI thread (preset loading)
     * - Background loader thread
     * - Initialization code
     *
     * NEVER call from:
     * - process() method
     * - Audio callback thread
     * - While isProcessing == true
     *
     * Performs file I/O and heap allocations which are not real-time safe.
     * Maximum IR length is 50ms at the current sample rate.
     *
     * @param file Path to audio file (WAV, AIFF, etc.)
     * @return true if load successful, false otherwise
     * @throws std::runtime_error if called from audio thread (debug builds)
     */
    bool loadImpulseResponse(const juce::File& file);
    void clearImpulseResponse();

private:
    static constexpr int kMaxTaps = 32;
    static constexpr int kMinTaps = 16;
    static constexpr float kMaxIrSeconds = 0.05f;

    void updateTapLayout();
    void updateModeTuning();
    float shapePosition(float position01) const;
    float applyAllpass(float input, float coeff, float& state) const;
    float readDelayInterpolated(const float* buffer, int bufferLength,
                                int writePos, float delaySamples) const;  // Phase 5: 4-point Lagrange

    double sampleRateHz = 44100.0;
    int maxBlockSize = 0;
    int channels = 0;

    int tapCount = kMinTaps;
    std::array<float, kMaxTaps> tapSamples{};  // Phase 5: Float for fractional delays
    std::array<float, kMaxTaps> tapGains{};
    std::array<float, kMaxTaps> tapAllpassCoeff{};
    juce::AudioBuffer<float> tapAllpassState;

    // Per-sample smoothing for tap coefficients/gains/positions to prevent clicks
    std::array<juce::SmoothedValue<float>, kMaxTaps> tapGainSmoothers;
    std::array<juce::SmoothedValue<float>, kMaxTaps> tapCoeffSmoothers;
    std::array<juce::SmoothedValue<float>, kMaxTaps> tapPositionSmoothers;  // Phase 5: Smooth position changes

    juce::AudioBuffer<float> delayBuffer;
    int delayBufferLength = 0;
    int writePosition = 0;

    float densityAmount = 0.5f;
    float warpAmount = 0.0f;
    ParameterBuffer pillarShapeBuffer;  // Phase 4: Per-sample parameter buffer (16-byte view)
    float lastShapeValue = 0.5f;
    bool lastShapeValid = false;
    Mode pillarMode = Mode::Glass;
    bool tapsDirty = true;
    int mutationSamplesRemaining = 0;
    int mutationIntervalSamples = 0;
    int mutationSeed = 0;

    float modeLowpassCoeff = 0.0f;
    float modeHighpassCoeff = 0.0f;
    float modeDiffusion = 0.18f;
    float modeTapGain = 1.0f;
    juce::AudioBuffer<float> modeLowState;
    juce::AudioBuffer<float> modeHighState;

    juce::AudioBuffer<float> irBuffer;
    int irLengthSamples = 0;
    bool irLoaded = false;

    // Signal threshold for deferred tap updates (prevents clicks during active audio)
    float inputPeakMagnitude = 0.0f;
    static constexpr float kTapUpdateThreshold = 0.001f;  // ~-60dB

    // Track whether we're processing to catch audio-thread misuse of loadImpulseResponse
    bool isProcessing = false;
};

} // namespace dsp
} // namespace monument
