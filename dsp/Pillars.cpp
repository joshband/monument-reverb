#include "Pillars.h"

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
    lastShapeValue = pillarShapeBuffer.numSamples > 0
        ? juce::jlimit(0.0f, 1.0f, pillarShapeBuffer[0])
        : 0.5f;
    lastShapeValid = true;

    // Initialize tap coefficient/gain/position smoothers
    // Gains/coeffs: 15ms = responsive but click-free
    // Positions: 500ms = zipper-free tap layout changes (Phase 5)
    for (size_t i = 0; i < kMaxTaps; ++i)
    {
        tapGainSmoothers[i].reset(sampleRateHz, 0.015);
        tapCoeffSmoothers[i].reset(sampleRateHz, 0.015);
        tapPositionSmoothers[i].reset(sampleRateHz, 0.5);  // Phase 5: 500ms for smooth position changes
        tapGainSmoothers[i].setCurrentAndTargetValue(tapGains[i]);
        tapCoeffSmoothers[i].setCurrentAndTargetValue(tapAllpassCoeff[i]);
        tapPositionSmoothers[i].setCurrentAndTargetValue(tapSamples[i]);
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
    lastShapeValue = 0.5f;
    lastShapeValid = false;
}

void Pillars::process(juce::AudioBuffer<float>& buffer)
{
    juce::ScopedNoDenormals noDenormals;

    // Mark that we're processing (used to catch misuse of loadImpulseResponse)
    juce::ScopedValueSetter<bool> processingScope(isProcessing, true);

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
        // Update smoother targets after tap layout recalculation (Phase 5: includes positions)
        for (int tap = 0; tap < tapCount; ++tap)
        {
            const auto tapIndex = static_cast<size_t>(tap);
            tapGainSmoothers[tapIndex].setTargetValue(tapGains[tapIndex]);
            tapCoeffSmoothers[tapIndex].setTargetValue(tapAllpassCoeff[tapIndex]);
            tapPositionSmoothers[tapIndex].setTargetValue(tapSamples[tapIndex]);  // Phase 5: Smooth positions
        }
        tapsDirty = false;
    }

    std::array<float, kMaxTaps> smoothedCoeffs{};
    std::array<float, kMaxTaps> smoothedGains{};
    std::array<float, kMaxTaps> smoothedPositions{};

    static constexpr int kMaxChannels = 2;
    jassert(numChannels <= kMaxChannels);

    std::array<float*, kMaxChannels> channelDataPtrs{};
    std::array<float*, kMaxChannels> delayDataPtrs{};
    std::array<float*, kMaxChannels> apStatePtrs{};
    std::array<float, kMaxChannels> lowStates{};
    std::array<float, kMaxChannels> highStates{};

    for (int channel = 0; channel < numChannels; ++channel)
    {
        channelDataPtrs[channel] = buffer.getWritePointer(channel);
        delayDataPtrs[channel] = delayBuffer.getWritePointer(channel);
        apStatePtrs[channel] = tapAllpassState.getWritePointer(channel);
        lowStates[channel] = modeLowState.getSample(channel, 0);
        highStates[channel] = modeHighState.getSample(channel, 0);
    }

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Advance smoothers once per sample (not per channel) to avoid fast ramps
        // Phase 5: Now includes position smoothing for zipper-free tap changes
        for (int tap = 0; tap < tapCount; ++tap)
        {
            const auto tapIndex = static_cast<size_t>(tap);
            smoothedCoeffs[tapIndex] = tapCoeffSmoothers[tapIndex].getNextValue();
            smoothedGains[tapIndex] = tapGainSmoothers[tapIndex].getNextValue();
            smoothedPositions[tapIndex] = tapPositionSmoothers[tapIndex].getNextValue();
        }

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* channelData = channelDataPtrs[channel];
            auto* delayData = delayDataPtrs[channel];
            auto* apState = apStatePtrs[channel];
            float lowState = lowStates[channel];
            float highState = highStates[channel];
            const float input = channelData[sample];
            float acc = input;

            for (int tap = 0; tap < tapCount; ++tap)
            {
                const auto tapIndex = static_cast<size_t>(tap);
                // Phase 5: Use interpolated read with smoothed fractional position
                const float tapIn = readDelayInterpolated(delayData, delayBufferLength,
                                                         writePosition, smoothedPositions[tapIndex]);
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
            lowStates[channel] = lowState;
            highStates[channel] = highState;
        }

        ++writePosition;
        if (writePosition >= delayBufferLength)
            writePosition = 0;
    }

    for (int channel = 0; channel < numChannels; ++channel)
    {
        modeLowState.setSample(channel, 0, lowStates[channel]);
        modeHighState.setSample(channel, 0, highStates[channel]);
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

void Pillars::setShape(const ParameterBuffer& shape)
{
    pillarShapeBuffer = shape;
    float shapeValue = lastShapeValid ? lastShapeValue : 0.5f;
    if (pillarShapeBuffer.numSamples > 0)
        shapeValue = pillarShapeBuffer[0];
    if (!std::isfinite(shapeValue))
        shapeValue = lastShapeValid ? lastShapeValue : 0.5f;
    shapeValue = juce::jlimit(0.0f, 1.0f, shapeValue);

    if (!lastShapeValid || std::abs(shapeValue - lastShapeValue) > 1.0e-3f)
    {
        tapsDirty = true;
        lastShapeValue = shapeValue;
        lastShapeValid = true;
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
    // Debug check: catch audio-thread misuse
    if (isProcessing)
    {
        jassertfalse;  // Triggers in debug builds
        return false;  // Fail safely in release builds
    }

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
        // Phase 5: Use fractional delays for smooth, zipper-free tap position changes
        const float exactDelay = sampleRateHz * (delayMs / 1000.0f);
        tapSamples[tapIndex] = juce::jlimit(2.0f, static_cast<float>(delayBufferLength - 3), exactDelay);

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
    // Phase 4: Sample from parameter buffer (eliminates double smoothing)
    // Handle uninitialized buffer with sensible default (0.5 = neutral)
    const float pillarShapeValue = pillarShapeBuffer.numSamples > 0
        ? juce::jlimit(0.0f, 1.0f, pillarShapeBuffer[0])
        : 0.5f;

    // Shape interpolates between compressed (short intervals) and expanded (long intervals).
    const float shape = juce::jmap(pillarShapeValue, -1.0f, 1.0f);
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

float Pillars::readDelayInterpolated(const float* buffer, int bufferLength,
                                     int writePos, float delaySamples) const
{
    // Phase 5: Linear interpolation for fractional delay
    // Optimized: branchless wrapping, minimal operations

    // Calculate read position with fractional part
    float readPosFloat = static_cast<float>(writePos) - delaySamples;
    if (readPosFloat < 0.0f)
        readPosFloat += static_cast<float>(bufferLength);

    // Integer and fractional parts
    const int readPos0 = static_cast<int>(readPosFloat);
    const float frac = readPosFloat - static_cast<float>(readPos0);

    // Get 2 samples with branchless wrapping (avoid modulo)
    const int idx_0 = readPos0;
    int idx_1 = readPos0 + 1;
    if (idx_1 >= bufferLength)
        idx_1 = 0;

    const float y_0 = buffer[idx_0];
    const float y_1 = buffer[idx_1];

    // Linear interpolation: y = y0 + frac * (y1 - y0)
    return y_0 + frac * (y_1 - y_0);
}

} // namespace dsp
} // namespace monument
