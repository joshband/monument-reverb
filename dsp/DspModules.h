#pragma once

#include "dsp/Chambers.h"
#include "dsp/DspModule.h"

#include <array>

namespace monument
{
namespace dsp
{
class Foundation final : public DSPModule
{
public:
    void prepare(double sampleRate, int blockSize, int numChannels) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void setInputGainDb(float gainDb);

private:
    double sampleRateHz = 44100.0;
    int maxBlockSize = 0;
    int channels = 0;
    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>,
                                   juce::dsp::IIR::Coefficients<float>>
        dcBlocker;
    juce::dsp::Gain<float> inputGain;
};

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
    void setShape(float shape);
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

    double sampleRateHz = 44100.0;
    int maxBlockSize = 0;
    int channels = 0;

    int tapCount = kMinTaps;
    std::array<int, kMaxTaps> tapSamples{};
    std::array<float, kMaxTaps> tapGains{};
    std::array<float, kMaxTaps> tapAllpassCoeff{};
    juce::AudioBuffer<float> tapAllpassState;

    // Per-sample smoothing for tap coefficients/gains to prevent clicks
    std::array<juce::SmoothedValue<float>, kMaxTaps> tapGainSmoothers;
    std::array<juce::SmoothedValue<float>, kMaxTaps> tapCoeffSmoothers;

    juce::AudioBuffer<float> delayBuffer;
    int delayBufferLength = 0;
    int writePosition = 0;

    float densityAmount = 0.5f;
    float warpAmount = 0.0f;
    float pillarShape = 0.5f;
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

class Weathering final : public DSPModule
{
public:
    void prepare(double sampleRate, int blockSize, int numChannels) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void setWarp(float warp);
    void setDrift(float drift);

private:
    double sampleRateHz = 44100.0;
    int maxBlockSize = 0;
    int channels = 0;
    juce::AudioBuffer<float> delayBuffer;
    int delayBufferLength = 0;
    int writePosition = 0;
    float baseDelaySamples = 0.0f;
    float depthBaseSamples = 0.0f;
    float depthSamples = 0.0f;
    float mix = 0.25f;
    juce::dsp::Oscillator<float> lfo;
    float lfoRateHz = 0.08f;
    float warpAmount = 0.3f;
    float driftAmount = 0.3f;
};

class Buttress final : public DSPModule
{
public:
    void prepare(double sampleRate, int blockSize, int numChannels) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void setDrive(float driveAmount);
    void setFreeze(bool shouldFreeze);

private:
    double sampleRateHz = 44100.0;
    int maxBlockSize = 0;
    int channels = 0;
    float drive = 1.15f;
    bool freezeEnabled = false;
};

class Facade final : public DSPModule
{
public:
    void prepare(double sampleRate, int blockSize, int numChannels) override;
    void reset() override;
    void process(juce::AudioBuffer<float>& buffer) override;
    void setWidth(float widthAmount);
    void setAir(float airAmount);
    void setOutputGain(float gainLinear);

    /**
     * @brief Enable/disable 3D panning mode (Phase 2: Three-System Plan)
     *
     * When enabled, uses azimuth/elevation for spatial positioning.
     * When disabled, uses traditional mid-side stereo width control.
     *
     * @param enable True to use 3D panning, false for stereo width
     */
    void set3DPanning(bool enable) noexcept;

    /**
     * @brief Set 3D spatial position via azimuth and elevation (Phase 2: Three-System Plan)
     *
     * Uses constant power panning law for perceptually smooth positioning.
     *
     * @param azimuth Horizontal angle in degrees: -90° (left) to +90° (right), 0° = center
     * @param elevation Vertical angle in degrees: -90° (down) to +90° (up), 0° = horizontal
     */
    void setSpatialPositions(float azimuthDegrees, float elevationDegrees) noexcept;

private:
    double sampleRateHz = 44100.0;
    int maxBlockSize = 0;
    int channels = 0;
    float width = 1.1f;
    float air = 0.5f;
    float outputGain = 1.0f;
    juce::AudioBuffer<float> airState;
    float airCoefficient = 0.0f;
    juce::SmoothedValue<float> airGainSmoother;  // Per-sample smoothing for airGain

    // Phase 2: Three-System Plan - 3D Panning
    bool use3DPanning = false;
    float azimuthDegrees = 0.0f;     // -90° (left) to +90° (right)
    float elevationDegrees = 0.0f;   // -90° (down) to +90° (up)

    // Constant power panning gains (smoothed for click-free transitions)
    juce::SmoothedValue<float> leftGainSmoother;
    juce::SmoothedValue<float> rightGainSmoother;
};

} // namespace dsp
} // namespace monument
