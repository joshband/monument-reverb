#include "DspModules.h"

#include <cmath>

namespace monument
{
namespace dsp
{
void Foundation::prepare(double sampleRate, int blockSize, int numChannels)
{
    sampleRateHz = sampleRate;
    maxBlockSize = blockSize;
    channels = numChannels;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRateHz;
    spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
    spec.numChannels = static_cast<juce::uint32>(channels);

    dcBlocker.state = juce::dsp::IIR::Coefficients<float>::makeHighPass(sampleRateHz, 20.0);
    dcBlocker.prepare(spec);
    dcBlocker.reset();

    inputGain.prepare(spec);
    inputGain.setRampDurationSeconds(0.02);
    inputGain.setGainLinear(1.0f);
}

void Foundation::reset()
{
    dcBlocker.reset();
    inputGain.reset();
}

void Foundation::process(juce::AudioBuffer<float>& buffer)
{
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    dcBlocker.process(context);
    inputGain.process(context);
}

void Foundation::setInputGainDb(float gainDb)
{
    if (!std::isfinite(gainDb))
        return;
    inputGain.setGainDecibels(gainDb);
}

void Weathering::prepare(double sampleRate, int blockSize, int numChannels)
{
    sampleRateHz = sampleRate;
    maxBlockSize = blockSize;
    channels = numChannels;

    const auto maxDelaySeconds = 0.05;
    delayBufferLength = juce::jmax(1, static_cast<int>(sampleRateHz * maxDelaySeconds));
    delayBuffer.setSize(channels, delayBufferLength);
    delayBuffer.clear();
    writePosition = 0;

    baseDelaySamples = static_cast<float>(sampleRateHz * 0.015);
    depthBaseSamples = static_cast<float>(sampleRateHz * 0.005);
    depthSamples = depthBaseSamples;

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRateHz;
    spec.maximumBlockSize = static_cast<juce::uint32>(maxBlockSize);
    spec.numChannels = 1;

    lfo.initialise([](float x) { return std::sin(x); });
    lfo.prepare(spec);
    lfo.setFrequency(lfoRateHz);

    setWarp(warpAmount);
    setDrift(driftAmount);
}

void Weathering::reset()
{
    delayBuffer.clear();
    writePosition = 0;
    lfo.reset();
}

void Weathering::process(juce::AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();
    const auto depthLocal = depthSamples;
    const auto mixLocal = juce::jlimit(0.0f, 1.0f, mix);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float mod = lfo.processSample(0.0f);
        const float delaySamples = baseDelaySamples + depthLocal * mod;
        const int delayInt = static_cast<int>(delaySamples);
        const float frac = delaySamples - static_cast<float>(delayInt);

        int readPosA = writePosition - delayInt;
        if (readPosA < 0)
            readPosA += delayBufferLength;
        int readPosB = readPosA - 1;
        if (readPosB < 0)
            readPosB += delayBufferLength;

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            auto* delayData = delayBuffer.getWritePointer(channel);
            const float input = channelData[sample];

            const float delayed = delayData[readPosA] * (1.0f - frac) + delayData[readPosB] * frac;
            delayData[writePosition] = input;
            channelData[sample] = input * (1.0f - mixLocal) + delayed * mixLocal;
        }

        ++writePosition;
        if (writePosition >= delayBufferLength)
            writePosition = 0;
    }
}

void Weathering::setWarp(float warp)
{
    if (!std::isfinite(warp))
        return;
    warpAmount = juce::jlimit(0.0f, 1.0f, warp);
    depthSamples = depthBaseSamples * juce::jmap(warpAmount, 0.25f, 1.2f);
    mix = juce::jmap(warpAmount, 0.1f, 0.4f);
}

void Weathering::setDrift(float drift)
{
    if (!std::isfinite(drift))
        return;
    driftAmount = juce::jlimit(0.0f, 1.0f, drift);
    lfoRateHz = juce::jmap(driftAmount, 0.02f, 0.2f);
    lfo.setFrequency(lfoRateHz);
}

void Buttress::prepare(double sampleRate, int blockSize, int numChannels)
{
    sampleRateHz = sampleRate;
    maxBlockSize = blockSize;
    channels = numChannels;
}

void Buttress::reset()
{
}

void Buttress::process(juce::AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();
    auto driveLocal = juce::jlimit(0.5f, 3.0f, drive);
    if (freezeEnabled)
        driveLocal = juce::jmin(3.0f, driveLocal * 1.25f);
    const auto norm = juce::dsp::FastMathApproximations::tanh(driveLocal);
    const auto normSafe = norm > 0.0f ? norm : 1.0f;

    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float driven = channelData[sample] * driveLocal;
            channelData[sample] = juce::dsp::FastMathApproximations::tanh(driven) / normSafe;
        }
    }
}

void Buttress::setDrive(float driveAmount)
{
    if (!std::isfinite(driveAmount))
        return;
    drive = juce::jlimit(0.5f, 3.0f, driveAmount);
}

void Buttress::setFreeze(bool shouldFreeze)
{
    freezeEnabled = shouldFreeze;
}

void Facade::prepare(double sampleRate, int blockSize, int numChannels)
{
    sampleRateHz = sampleRate;
    maxBlockSize = blockSize;
    channels = numChannels;
    airState.setSize(channels, 1, false, false, true);
    airState.clear();

    const auto cutoffHz = 6500.0f;
    const auto x = std::exp(-2.0f * juce::MathConstants<float>::pi
                            * cutoffHz / static_cast<float>(sampleRateHz));
    airCoefficient = 1.0f - x;

    // Initialize airGain smoother for per-sample interpolation (10ms ramp)
    airGainSmoother.reset(sampleRate, 0.01);  // 10ms smoothing
    setAir(air);
    airGainSmoother.setCurrentAndTargetValue(juce::jmap(air, -0.3f, 0.35f));

    // Initialize output gain smoother for feedback safety (20ms ramp)
    outputGainSmoother.reset(sampleRate, 0.02);
    outputGainSmoother.setCurrentAndTargetValue(outputGain);

    // Phase 2: Three-System Plan - Initialize 3D panning gain smoothers (20ms ramp for smooth transitions)
    leftGainSmoother.reset(sampleRate, 0.02);
    rightGainSmoother.reset(sampleRate, 0.02);
    leftGainSmoother.setCurrentAndTargetValue(0.707f);   // sqrt(0.5) for center position
    rightGainSmoother.setCurrentAndTargetValue(0.707f);
}

void Facade::reset()
{
    airState.clear();
}

void Facade::process(juce::AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();
    const auto widthLocal = juce::jlimit(0.0f, 2.0f, width);

    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer(channel);
        auto* stateData = airState.getWritePointer(channel);
        float state = stateData[0];

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float input = channelData[sample];
            state += airCoefficient * (input - state);
            const float high = input - state;
            const float currentAirGain = airGainSmoother.getNextValue();  // Per-sample interpolation
            channelData[sample] = input + high * currentAirGain;
        }

        stateData[0] = state;
    }

    if (numChannels < 2)
    {
        // Apply smoothed gain per-sample (mono)
        auto* data = buffer.getWritePointer(0);
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float gain = outputGainSmoother.getNextValue();
            data[sample] *= gain;
        }
        return;
    }

    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);

    // Phase 2: Three-System Plan - Choose between 3D panning and traditional stereo width
    if (use3DPanning)
    {
        // 3D azimuth/elevation panning using constant power law
        for (int sample = 0; sample < numSamples; ++sample)
        {
            // Mix to mono first (sum L+R)
            const float mono = 0.5f * (left[sample] + right[sample]);

            // Apply smoothed panning gains (constant power law, calculated in setSpatialPositions)
            const float leftGain = leftGainSmoother.getNextValue();
            const float rightGain = rightGainSmoother.getNextValue();
            const float outputGain = outputGainSmoother.getNextValue();  // Per-sample smoothed output gain

            left[sample] = mono * leftGain * outputGain;
            right[sample] = mono * rightGain * outputGain;
        }
    }
    else
    {
        // Traditional mid-side stereo width processing
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float mid = 0.5f * (left[sample] + right[sample]);
            float side = 0.5f * (left[sample] - right[sample]);
            side *= widthLocal;
            const float outputGain = outputGainSmoother.getNextValue();  // Per-sample smoothed output gain

            left[sample] = (mid + side) * outputGain;
            right[sample] = (mid - side) * outputGain;
        }
    }
}

void Facade::setWidth(float widthAmount)
{
    if (!std::isfinite(widthAmount))
        return;
    width = juce::jlimit(0.0f, 2.0f, widthAmount);
}

void Facade::setAir(float airAmount)
{
    if (!std::isfinite(airAmount))
        return;
    air = juce::jlimit(0.0f, 1.0f, airAmount);
    const float targetGain = juce::jmap(air, -0.3f, 0.35f);
    airGainSmoother.setTargetValue(targetGain);
}

void Facade::setOutputGain(float gainLinear)
{
    if (!std::isfinite(gainLinear))
        return;
    outputGain = juce::jmax(0.0f, gainLinear);
    outputGainSmoother.setTargetValue(outputGain);  // Smooth transitions for feedback safety
}

void Facade::set3DPanning(bool enable) noexcept
{
    use3DPanning = enable;
}

void Facade::setSpatialPositions(float azimuthDegreesInput, float elevationDegreesInput) noexcept
{
    // Phase 2: Three-System Plan - 3D panning using constant power law
    // Store raw degrees for potential UI display
    azimuthDegrees = juce::jlimit(-90.0f, 90.0f, azimuthDegreesInput);
    elevationDegrees = juce::jlimit(-90.0f, 90.0f, elevationDegreesInput);

    // Convert azimuth to pan angle θ: -90° (left) → 0, 0° (center) → π/2, +90° (right) → π
    const float azimuthRadians = (azimuthDegrees + 90.0f) * juce::MathConstants<float>::pi / 180.0f;
    const float panAngle = azimuthRadians;  // 0 to π

    // Constant power panning law: L = cos(θ/2), R = sin(θ/2)
    // This ensures L² + R² = 1, maintaining constant perceived loudness
    const float leftGain = std::cos(panAngle * 0.5f);
    const float rightGain = std::sin(panAngle * 0.5f);

    // Elevation scaling: higher elevation = quieter (simulating distance/height)
    // cos(0°) = 1.0 (horizontal plane), cos(±90°) = 0.0 (directly above/below)
    const float elevationRadians = elevationDegrees * juce::MathConstants<float>::pi / 180.0f;
    const float elevationScale = std::cos(elevationRadians);
    const float elevationScaleClamped = juce::jmax(0.0f, elevationScale);  // Ensure non-negative

    // Apply elevation scaling to both channels
    const float finalLeftGain = leftGain * elevationScaleClamped;
    const float finalRightGain = rightGain * elevationScaleClamped;

    // Update smoothed gain targets (smoothers will interpolate per-sample in process())
    leftGainSmoother.setTargetValue(finalLeftGain);
    rightGainSmoother.setTargetValue(finalRightGain);
}

} // namespace dsp
} // namespace monument
