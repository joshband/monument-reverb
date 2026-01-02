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
    inputGain.setGainDecibels(gainDb);
}

void Pillars::prepare(double sampleRate, int blockSize, int numChannels)
{
    sampleRateHz = sampleRate;
    maxBlockSize = blockSize;
    channels = numChannels;

    const auto maxDelaySeconds = 0.09;
    delayBufferLength = juce::jmax(1, static_cast<int>(sampleRateHz * maxDelaySeconds));
    delayBuffer.setSize(channels, delayBufferLength);
    delayBuffer.clear();
    writePosition = 0;

    const std::array<float, kNumTaps> tapTimesMs{7.0f, 11.0f, 17.0f, 23.0f, 31.0f, 41.0f};
    const std::array<float, kNumTaps> baseGains{0.45f, 0.38f, 0.31f, 0.26f, 0.22f, 0.18f};

    for (size_t i = 0; i < kNumTaps; ++i)
    {
        const auto samples = static_cast<int>(std::round(sampleRateHz * (tapTimesMs[i] / 1000.0f)));
        tapSamples[i] = juce::jlimit(1, delayBufferLength - 1, samples);
        tapGains[i] = baseGains[i];
    }
}

void Pillars::reset()
{
    delayBuffer.clear();
    writePosition = 0;
}

void Pillars::process(juce::AudioBuffer<float>& buffer)
{
    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();
    const auto densityScale = juce::jmap(juce::jlimit(0.0f, 1.0f, densityAmount), 0.35f, 0.75f);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            auto* delayData = delayBuffer.getWritePointer(channel);
            const float input = channelData[sample];
            float acc = input;

            for (size_t tap = 0; tap < kNumTaps; ++tap)
            {
                int readPos = writePosition - tapSamples[tap];
                if (readPos < 0)
                    readPos += delayBufferLength;

                acc += delayData[readPos] * tapGains[tap] * densityScale;
            }

            delayData[writePosition] = input;
            channelData[sample] = acc;
        }

        ++writePosition;
        if (writePosition >= delayBufferLength)
            writePosition = 0;
    }
}

void Pillars::setDensity(float density)
{
    densityAmount = juce::jlimit(0.0f, 1.0f, density);
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
    warpAmount = juce::jlimit(0.0f, 1.0f, warp);
    depthSamples = depthBaseSamples * juce::jmap(warpAmount, 0.25f, 1.2f);
    mix = juce::jmap(warpAmount, 0.1f, 0.4f);
}

void Weathering::setDrift(float drift)
{
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
    setAir(air);
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
    const auto gainLocal = outputGain;

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
            channelData[sample] = input + high * airGain;
        }

        stateData[0] = state;
    }

    if (numChannels < 2)
    {
        buffer.applyGain(gainLocal);
        return;
    }

    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getWritePointer(1);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const float mid = 0.5f * (left[sample] + right[sample]);
        float side = 0.5f * (left[sample] - right[sample]);
        side *= widthLocal;

        left[sample] = (mid + side) * gainLocal;
        right[sample] = (mid - side) * gainLocal;
    }
}

void Facade::setWidth(float widthAmount)
{
    width = juce::jlimit(0.0f, 2.0f, widthAmount);
}

void Facade::setAir(float airAmount)
{
    air = juce::jlimit(0.0f, 1.0f, airAmount);
    airGain = juce::jmap(air, -0.3f, 0.35f);
}

void Facade::setOutputGain(float gainLinear)
{
    outputGain = juce::jmax(0.0f, gainLinear);
}

} // namespace dsp
} // namespace monument
