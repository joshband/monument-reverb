#include "DspModules.h"

#include <cmath>

namespace
{
float onePoleCoeffFromHz(float cutoffHz, double sampleRate)
{
    const double omega = 2.0 * juce::MathConstants<double>::pi
        * static_cast<double>(cutoffHz) / sampleRate;
    return static_cast<float>(1.0 - std::exp(-omega));
}

// RMS tap gain target (pre-density) to keep Pillars energy bounded before Chambers.
constexpr float kPillarsTapEnergyTarget = 1.6f;
constexpr float kPillarsOutputCeiling = 1.25f;
} // namespace

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

    tapAllpassState.setSize(channels, kMaxTaps);
    tapAllpassState.clear();

    modeLowState.setSize(channels, 1);
    modeHighState.setSize(channels, 1);
    modeLowState.clear();
    modeHighState.clear();

    updateModeTuning();
    updateTapLayout();

    // Initialize tap coefficient/gain smoothers (15ms = responsive but click-free)
    for (size_t i = 0; i < kMaxTaps; ++i)
    {
        tapGainSmoothers[i].reset(sampleRateHz, 0.015);
        tapCoeffSmoothers[i].reset(sampleRateHz, 0.015);
        tapGainSmoothers[i].setCurrentAndTargetValue(tapGains[i]);
        tapCoeffSmoothers[i].setCurrentAndTargetValue(tapAllpassCoeff[i]);
    }
}

void Pillars::reset()
{
    delayBuffer.clear();
    writePosition = 0;
    tapAllpassState.clear();
    modeLowState.clear();
    modeHighState.clear();
    mutationSamplesRemaining = 0;
    mutationIntervalSamples = 0;
    mutationSeed = 0;
    tapsDirty = true;
}

void Pillars::process(juce::AudioBuffer<float>& buffer)
{
    juce::ScopedNoDenormals noDenormals;
    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();
    const auto densityScale = juce::jmap(juce::jlimit(0.0f, 1.0f, densityAmount), 0.25f, 0.85f);

    if (warpAmount > 0.0f && sampleRateHz > 0.0)
    {
        const float intervalSeconds = juce::jmap(warpAmount, 6.0f, 2.0f);
        mutationIntervalSamples = juce::jmax(1, static_cast<int>(intervalSeconds * sampleRateHz));
        if (mutationSamplesRemaining <= 0)
            mutationSamplesRemaining = mutationIntervalSamples;
        mutationSamplesRemaining -= numSamples;
        if (mutationSamplesRemaining <= 0)
        {
            mutationSamplesRemaining = mutationIntervalSamples;
            ++mutationSeed;
            tapsDirty = true;
        }
    }
    else
    {
        mutationSamplesRemaining = 0;
        mutationIntervalSamples = 0;
    }

    // Track peak input magnitude to defer tap updates during active audio.
    // This prevents clicks from tap position discontinuities when Topology/Shape changes.
    // Only update tap layout when signal is below threshold (~-60dB).
    inputPeakMagnitude = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* channelData = buffer.getReadPointer(ch);
        for (int sample = 0; sample < numSamples; ++sample)
        {
            inputPeakMagnitude = juce::jmax(inputPeakMagnitude, std::abs(channelData[sample]));
        }
    }

    // Defer tap layout updates until input is quiet to avoid clicks
    if (tapsDirty && inputPeakMagnitude < kTapUpdateThreshold)
    {
        updateTapLayout();
        // Update smoother targets after tap layout recalculation
        for (int tap = 0; tap < tapCount; ++tap)
        {
            const auto tapIndex = static_cast<size_t>(tap);
            tapGainSmoothers[tapIndex].setTargetValue(tapGains[tapIndex]);
            tapCoeffSmoothers[tapIndex].setTargetValue(tapAllpassCoeff[tapIndex]);
        }
        tapsDirty = false;
    }

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Advance smoothers once per sample (not per channel) to avoid fast ramps
        std::array<float, kMaxTaps> smoothedCoeffs;
        std::array<float, kMaxTaps> smoothedGains;
        for (int tap = 0; tap < tapCount; ++tap)
        {
            const auto tapIndex = static_cast<size_t>(tap);
            smoothedCoeffs[tapIndex] = tapCoeffSmoothers[tapIndex].getNextValue();
            smoothedGains[tapIndex] = tapGainSmoothers[tapIndex].getNextValue();
        }

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = buffer.getWritePointer(channel);
            auto* delayData = delayBuffer.getWritePointer(channel);
            auto* apState = tapAllpassState.getWritePointer(channel);
            float lowState = modeLowState.getSample(channel, 0);
            float highState = modeHighState.getSample(channel, 0);
            const float input = channelData[sample];
            float acc = input;

            for (int tap = 0; tap < tapCount; ++tap)
            {
                const auto tapIndex = static_cast<size_t>(tap);
                int readPos = writePosition - tapSamples[tapIndex];
                if (readPos < 0)
                    readPos += delayBufferLength;

                const float tapIn = delayData[readPos];
                // Use pre-computed smoothed values from outer loop
                const float tapOut = applyAllpass(tapIn, smoothedCoeffs[tapIndex], apState[tap]);
                acc += tapOut * smoothedGains[tapIndex] * densityScale;
            }

            delayData[writePosition] = input;

            float filtered = acc;
            if (modeLowpassCoeff > 0.0f)
            {
                lowState += modeLowpassCoeff * (filtered - lowState);
                filtered = lowState;
            }

            if (modeHighpassCoeff > 0.0f)
            {
                // High-pass removes DC from IR-mapped taps and keeps low-end energy in check.
                highState += modeHighpassCoeff * (filtered - highState);
                filtered = filtered - highState;
            }

            // Pillars is a creative early-space generator; clamp output to keep Chambers input bounded.
            filtered = juce::jlimit(-kPillarsOutputCeiling, kPillarsOutputCeiling, filtered);

            channelData[sample] = filtered;
            modeLowState.setSample(channel, 0, lowState);
            modeHighState.setSample(channel, 0, highState);
        }

        ++writePosition;
        if (writePosition >= delayBufferLength)
            writePosition = 0;
    }
}

void Pillars::setDensity(float density)
{
    if (!std::isfinite(density))
        return;
    const float clamped = juce::jlimit(0.0f, 1.0f, density);
    if (std::abs(clamped - densityAmount) > 1.0e-3f)
    {
        densityAmount = clamped;
        tapsDirty = true;
    }
}

void Pillars::setShape(float shape)
{
    if (!std::isfinite(shape))
        return;
    const float clamped = juce::jlimit(0.0f, 1.0f, shape);
    if (std::abs(clamped - pillarShape) > 1.0e-3f)
    {
        pillarShape = clamped;
        tapsDirty = true;
    }
}

void Pillars::setMode(int modeIndex)
{
    const int clamped = juce::jlimit(0, 2, modeIndex);
    if (static_cast<int>(pillarMode) != clamped)
    {
        pillarMode = static_cast<Mode>(clamped);
        updateModeTuning();
        tapsDirty = true;
    }
}

void Pillars::setWarp(float warp)
{
    if (!std::isfinite(warp))
        return;
    const float clamped = juce::jlimit(0.0f, 1.0f, warp);
    if (std::abs(clamped - warpAmount) > 1.0e-3f)
    {
        warpAmount = clamped;
        tapsDirty = true;
    }
}

bool Pillars::loadImpulseResponse(const juce::File& file)
{
    if (!file.existsAsFile() || sampleRateHz <= 0.0)
        return false;

    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (!reader)
        return false;

    const int maxSamples = static_cast<int>(std::round(sampleRateHz * kMaxIrSeconds));
    const int totalSamples = static_cast<int>(juce::jmin<int64>(reader->lengthInSamples, maxSamples));
    if (totalSamples <= 0)
        return false;

    irBuffer.setSize(1, totalSamples);
    irBuffer.clear();
    reader->read(&irBuffer, 0, totalSamples, 0, true, true);

    auto* irData = irBuffer.getWritePointer(0);
    float peak = 0.0f;
    for (int i = 0; i < totalSamples; ++i)
        peak = juce::jmax(peak, std::abs(irData[i]));
    if (peak > 0.0f)
        irBuffer.applyGain(1.0f / peak);

    irLengthSamples = totalSamples;
    irLoaded = true;
    tapsDirty = true;
    return true;
}

void Pillars::clearImpulseResponse()
{
    irLoaded = false;
    irBuffer.setSize(0, 0);
    irLengthSamples = 0;
    tapsDirty = true;
}

void Pillars::updateTapLayout()
{
    tapsDirty = false;

    const float densityClamped = juce::jlimit(0.0f, 1.0f, densityAmount);
    const float warpClamped = juce::jlimit(0.0f, 1.0f, warpAmount);

    int baseCount = 22;
    switch (pillarMode)
    {
        case Mode::Glass: baseCount = 26; break;
        case Mode::Stone: baseCount = 20; break;
        case Mode::Fog: baseCount = 30; break;
        default: break;
    }

    tapCount = juce::jlimit(kMinTaps, kMaxTaps,
        static_cast<int>(std::round(baseCount + densityClamped * 6.0f)));

    // Fractal clusters: randomized tap positions/gains seeded by density, warp, and a slow mutation seed.
    juce::Random random(static_cast<int>(densityClamped * 10000.0f)
        ^ static_cast<int>(warpClamped * 5000.0f)
        ^ (static_cast<int>(pillarMode) << 6)
        ^ (mutationSeed << 12));

    const float minDelayMs = 4.0f;
    const float maxDelayMs = 50.0f;
    const float warpJitter = juce::jmap(warpClamped, 0.0f, 0.35f);

    tapAllpassState.clear();

    for (int tap = 0; tap < tapCount; ++tap)
    {
        const auto tapIndex = static_cast<size_t>(tap);
        float position01 = random.nextFloat();
        if (warpJitter > 0.0f)
            position01 = juce::jlimit(0.0f, 1.0f,
                position01 + (random.nextFloat() - 0.5f) * warpJitter);

        const float shaped = shapePosition(position01);
        const float delayMs = juce::jmap(shaped, minDelayMs, maxDelayMs);
        // Keep tap delays integer-sample for stability during geometry bending.
        const int samples = static_cast<int>(std::round(sampleRateHz * (delayMs / 1000.0f)));
        tapSamples[tapIndex] = juce::jlimit(1, delayBufferLength - 1, samples);

        const float gainBase = juce::jmap(random.nextFloat(), 0.08f, 0.42f);
        tapGains[tapIndex] = gainBase * modeTapGain;

        // Coefficients stay below 0.3 to keep allpass diffusion stable.
        const float diffusion = juce::jmap(random.nextFloat(), 0.05f, modeDiffusion);
        tapAllpassCoeff[tapIndex] = diffusion;
    }

    // If an IR is loaded, use its amplitudes; otherwise keep algorithmic gains.
    if (irLoaded && irLengthSamples > 0)
    {
        auto* irData = irBuffer.getReadPointer(0);
        for (int tap = 0; tap < tapCount; ++tap)
        {
            const auto tapIndex = static_cast<size_t>(tap);
            const float position01 = static_cast<float>(tapSamples[tapIndex])
                / static_cast<float>(juce::jmax(1, delayBufferLength - 1));
            const int irIndex = juce::jlimit(0, irLengthSamples - 1,
                static_cast<int>(std::round(position01 * (irLengthSamples - 1))));
            tapGains[tapIndex] = irData[irIndex] * modeTapGain;
        }
    }

    // Normalize tap energy so early clusters stay punchy but bounded (no runaway IR gains).
    float energy = 0.0f;
    for (int tap = 0; tap < tapCount; ++tap)
    {
        const auto tapIndex = static_cast<size_t>(tap);
        energy += tapGains[tapIndex] * tapGains[tapIndex];
    }
    if (energy > 0.0f)
    {
        const float rms = std::sqrt(energy);
        if (rms > kPillarsTapEnergyTarget)
        {
            const float scale = kPillarsTapEnergyTarget / rms;
            for (int tap = 0; tap < tapCount; ++tap)
            {
                const auto tapIndex = static_cast<size_t>(tap);
                tapGains[tapIndex] *= scale;
            }
        }
    }
}

void Pillars::updateModeTuning()
{
    // Mode palettes set early reflection coloration and diffusion strength.
    float lowpassHz = 10000.0f;
    float highpassHz = 80.0f;
    modeDiffusion = 0.18f;
    modeTapGain = 1.0f;

    switch (pillarMode)
    {
        case Mode::Glass:
            lowpassHz = 14000.0f;
            highpassHz = 60.0f;
            modeDiffusion = 0.14f;
            modeTapGain = 1.05f;
            break;
        case Mode::Stone:
            lowpassHz = 7200.0f;
            highpassHz = 160.0f;
            modeDiffusion = 0.22f;
            modeTapGain = 0.85f;
            break;
        case Mode::Fog:
            lowpassHz = 11000.0f;
            highpassHz = 40.0f;
            modeDiffusion = 0.26f;
            modeTapGain = 0.95f;
            break;
        default:
            break;
    }

    modeLowpassCoeff = onePoleCoeffFromHz(lowpassHz, sampleRateHz);
    modeHighpassCoeff = onePoleCoeffFromHz(highpassHz, sampleRateHz);
}

float Pillars::shapePosition(float position01) const
{
    // Shape interpolates between compressed (short intervals) and expanded (long intervals).
    const float shape = juce::jmap(pillarShape, -1.0f, 1.0f);
    const float exponent = shape < 0.0f
        ? (1.0f + (-shape) * 2.0f)
        : (1.0f / (1.0f + shape * 1.5f));
    return std::pow(juce::jlimit(0.0f, 1.0f, position01), exponent);
}

float Pillars::applyAllpass(float input, float coeff, float& state) const
{
    const float output = -coeff * input + state;
    state = input + coeff * output;
    return output;
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
            const float currentAirGain = airGainSmoother.getNextValue();  // Per-sample interpolation
            channelData[sample] = input + high * currentAirGain;
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
    outputGain = juce::jmax(0.0f, gainLinear);
}

} // namespace dsp
} // namespace monument
