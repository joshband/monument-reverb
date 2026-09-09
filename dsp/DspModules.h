#pragma once

#include "dsp/Chambers.h"
#include "dsp/DspModule.h"

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
    juce::SmoothedValue<float> airGainSmoother;      // Per-sample smoothing for airGain
    juce::SmoothedValue<float> outputGainSmoother;   // Per-sample smoothing for feedback-safe output gain

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
