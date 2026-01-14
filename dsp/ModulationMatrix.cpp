#include "ModulationMatrix.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <random>

namespace monument
{
namespace dsp
{

// ============================================================================
// MODULATION SOURCES
// ============================================================================

/**
 * @brief Chaos attractor generator (Lorenz/Rössler).
 *
 * Implements Lorenz strange attractor: deterministic but unpredictable 3D motion.
 * Produces smooth, organic modulation with complex, non-repeating patterns.
 * Output is bipolar [-1, 1] on 3 axes (X, Y, Z).
 */
class ModulationMatrix::ChaosAttractor
{
public:
    void prepare(double sampleRate, int /*blockSize*/)
    {
        sampleRateHz = sampleRate;

        // Lorenz attractor parameters (classic values for chaotic behavior)
        sigma = 10.0f;   // Prandtl number
        rho = 28.0f;     // Rayleigh number (chaos when > 24.74)
        beta = 8.0f / 3.0f;  // Geometric factor

        // Integration time step: smaller = more accurate, larger = faster evolution
        // Calibrated for block-rate updates (~10ms @ 512 samples)
        dt = 0.001f;

        reset();
    }

    void reset()
    {
        // Initialize at a typical point on the attractor
        x = 0.1f;
        y = 0.0f;
        z = 0.0f;
    }

    void process(int numSamples)
    {
        // Block-rate update: iterate the attractor equations multiple times per block
        // More iterations = smoother motion
        juce::ignoreUnused(numSamples);
        constexpr int iterationsPerBlock = 10;

        for (int i = 0; i < iterationsPerBlock; ++i)
        {
            // Lorenz equations (Runge-Kutta 4th order integration)
            const float dx = sigma * (y - x);
            const float dy = x * (rho - z) - y;
            const float dz = x * y - beta * z;

            // Euler integration (simple but stable for these parameters)
            x += dx * dt;
            y += dy * dt;
            z += dz * dt;
        }

        // Normalize outputs to [-1, 1] range
        // Lorenz attractor typically ranges: X,Y ∈ [-20, 20], Z ∈ [0, 50]
        outputX = juce::jlimit(-1.0f, 1.0f, x / 20.0f);
        outputY = juce::jlimit(-1.0f, 1.0f, y / 20.0f);
        outputZ = juce::jlimit(-1.0f, 1.0f, (z - 25.0f) / 25.0f);  // Center Z around 25
    }

    float getValue(int axis) const
    {
        switch (axis)
        {
            case 0: return outputX;
            case 1: return outputY;
            case 2: return outputZ;
            default: return 0.0f;
        }
    }

private:
    double sampleRateHz{48000.0};

    // Lorenz attractor state
    float x{0.1f}, y{0.0f}, z{0.0f};

    // Lorenz parameters
    float sigma{10.0f};
    float rho{28.0f};
    float beta{8.0f / 3.0f};
    float dt{0.001f};

    // Normalized outputs
    float outputX{0.0f}, outputY{0.0f}, outputZ{0.0f};
};

/**
 * @brief Audio follower (RMS envelope tracking).
 *
 * Tracks input signal energy with musical attack/release characteristics.
 * Output is unipolar [0, 1] representing input amplitude envelope.
 */
class ModulationMatrix::AudioFollower
{
public:
    void prepare(double sampleRate, int /*blockSize*/)
    {
        sampleRateHz = sampleRate;

        // Attack: fast response to rising signals (musical: 10ms)
        attackCoeff = std::exp(-1.0f / static_cast<float>(sampleRate * 0.01));

        // Release: slower decay for smooth envelope (musical: 150ms)
        releaseCoeff = std::exp(-1.0f / static_cast<float>(sampleRate * 0.15));

        reset();
    }

    void reset()
    {
        currentEnvelope = 0.0f;
    }

    void process(const juce::AudioBuffer<float>& buffer, int numSamples)
    {
        if (buffer.getNumChannels() == 0 || numSamples == 0)
            return;

        // Compute RMS across all channels (block-rate measurement)
        float sumSquares = 0.0f;
        int totalSamples = 0;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const float* channelData = buffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                const float sample = channelData[i];
                sumSquares += sample * sample;
                ++totalSamples;
            }
        }

        // RMS = sqrt(mean(x^2))
        const float rms = totalSamples > 0
            ? std::sqrt(sumSquares / static_cast<float>(totalSamples))
            : 0.0f;

        // Apply attack/release envelope following
        const float coeff = (rms > currentEnvelope) ? attackCoeff : releaseCoeff;
        currentEnvelope = coeff * currentEnvelope + (1.0f - coeff) * rms;

        // Normalize to [0, 1] range assuming typical audio peaks around 0.5-1.0
        // Apply gentle compression curve for musical response
        constexpr float gain = 2.0f;  // Boost quiet signals
        float normalized = currentEnvelope * gain;

        // Soft clipping for natural compression
        if (normalized > 1.0f)
            normalized = 1.0f - std::exp(-(normalized - 1.0f));

        currentEnvelope = juce::jlimit(0.0f, 1.0f, normalized);
    }

    float getValue() const { return currentEnvelope; }

private:
    double sampleRateHz{48000.0};
    float currentEnvelope{0.0f};
    float attackCoeff{0.99f};   // Exponential smoothing coefficient for attack
    float releaseCoeff{0.995f}; // Exponential smoothing coefficient for release
};

/**
 * @brief Brownian motion generator (1/f noise, smooth random walk).
 *
 * Implements a bounded random walk with smooth, organic motion characteristics.
 * Output is bipolar [-1, 1] with automatic boundary reflection to prevent drift.
 */
class ModulationMatrix::BrownianMotion
{
public:
    void prepare(double sampleRate, int /*blockSize*/)
    {
        sampleRateHz = sampleRate;

        // Initialize random number generator with time-based seed
        const auto seed = static_cast<unsigned int>(
            std::chrono::high_resolution_clock::now().time_since_epoch().count());
        rng.seed(seed);

        reset();
    }

    void reset()
    {
        currentValue = 0.0f;
        velocity = 0.0f;
    }

    void process(int numSamples)
    {
        // Block-rate update (called once per audio block, not per sample)
        juce::ignoreUnused(numSamples);

        // Generate random step: uniform distribution [-1, 1]
        std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
        const float randomStep = dist(rng);

        // Step size calibrated for smooth motion at typical block rates (512 samples @ 48kHz = ~10ms updates)
        // Smaller steps = slower drift, larger steps = more erratic motion
        constexpr float baseStepSize = 0.03f;  // ~3% change per block

        // Apply inertia: smooth the random walk with velocity smoothing
        constexpr float inertia = 0.65f;  // Higher = smoother motion
        velocity = velocity * inertia + randomStep * (1.0f - inertia);

        // Update position with velocity-smoothed step
        currentValue += velocity * baseStepSize;

        // Boundary reflection: soft limits with elastic bounce
        if (currentValue > 1.0f)
        {
            currentValue = 1.0f - (currentValue - 1.0f) * 0.5f;  // Reflect with damping
            velocity *= -0.5f;  // Reverse velocity, reduce magnitude
        }
        else if (currentValue < -1.0f)
        {
            currentValue = -1.0f + (-1.0f - currentValue) * 0.5f;
            velocity *= -0.5f;
        }

        // Clamp to guarantee bounds (floating-point safety)
        currentValue = juce::jlimit(-1.0f, 1.0f, currentValue);
    }

    float getValue() const { return currentValue; }

private:
    double sampleRateHz{48000.0};
    float currentValue{0.0f};
    float velocity{0.0f};
    std::mt19937 rng;  // Mersenne Twister PRNG (real-time safe after seeding)
};

/**
 * @brief Envelope tracker (multi-stage attack/release/sustain detection).
 *
 * Detects musical envelope stages: attack (rising), sustain (stable), release (falling).
 * Output is unipolar [0, 1] with enhanced sensitivity to transients and dynamics.
 */
class ModulationMatrix::EnvelopeTracker
{
public:
    enum class Stage { Attack, Sustain, Release };

    void prepare(double sampleRate, int /*blockSize*/)
    {
        sampleRateHz = sampleRate;

        // Very fast attack detection (5ms) to catch transients
        fastAttackCoeff = std::exp(-1.0f / static_cast<float>(sampleRate * 0.005));

        // Medium attack for envelope shaping (20ms)
        mediumAttackCoeff = std::exp(-1.0f / static_cast<float>(sampleRate * 0.02));

        // Slow release for musical decay (300ms)
        releaseCoeff = std::exp(-1.0f / static_cast<float>(sampleRate * 0.3));

        reset();
    }

    void reset()
    {
        currentEnvelope = 0.0f;
        peakEnvelope = 0.0f;
        currentStage = Stage::Release;
    }

    void process(const juce::AudioBuffer<float>& buffer, int numSamples)
    {
        if (buffer.getNumChannels() == 0 || numSamples == 0)
            return;

        // Measure peak and RMS for envelope detection
        float peak = 0.0f;
        float sumSquares = 0.0f;
        int totalSamples = 0;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const float* channelData = buffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
            {
                const float sample = std::abs(channelData[i]);
                peak = std::max(peak, sample);
                sumSquares += sample * sample;
                ++totalSamples;
            }
        }

        const float rms = totalSamples > 0
            ? std::sqrt(sumSquares / static_cast<float>(totalSamples))
            : 0.0f;

        // Combine peak and RMS for musical envelope (peak emphasizes transients, RMS provides body)
        const float instantLevel = peak * 0.6f + rms * 0.4f;

        // Stage detection and coefficient selection
        const float threshold = 0.01f;  // Minimum level for sustain detection

        if (instantLevel > currentEnvelope + threshold)
        {
            // Rising signal = Attack stage
            currentStage = Stage::Attack;
            currentEnvelope = fastAttackCoeff * currentEnvelope
                            + (1.0f - fastAttackCoeff) * instantLevel;
            peakEnvelope = std::max(peakEnvelope, currentEnvelope);
        }
        else if (instantLevel > threshold && std::abs(instantLevel - currentEnvelope) < threshold)
        {
            // Stable signal = Sustain stage
            currentStage = Stage::Sustain;
            currentEnvelope = mediumAttackCoeff * currentEnvelope
                            + (1.0f - mediumAttackCoeff) * instantLevel;
        }
        else
        {
            // Falling signal = Release stage
            currentStage = Stage::Release;
            currentEnvelope = releaseCoeff * currentEnvelope
                            + (1.0f - releaseCoeff) * instantLevel;

            // Decay peak envelope slowly
            peakEnvelope *= 0.999f;
        }

        // Normalize output: boost quiet signals, compress loud signals
        constexpr float gain = 2.5f;
        float output = currentEnvelope * gain;

        // Soft compression for natural dynamics
        if (output > 1.0f)
            output = 1.0f - std::exp(-(output - 1.0f) * 0.5f);

        currentEnvelope = juce::jlimit(0.0f, 1.0f, output);
    }

    float getValue() const { return currentEnvelope; }

    Stage getCurrentStage() const { return currentStage; }

private:
    double sampleRateHz{48000.0};
    float currentEnvelope{0.0f};
    float peakEnvelope{0.0f};
    float fastAttackCoeff{0.99f};
    float mediumAttackCoeff{0.995f};
    float releaseCoeff{0.998f};
    Stage currentStage{Stage::Release};
};

/**
 * @brief User-configurable LFO (block-rate).
 *
 * Produces bipolar [-1, 1] modulation values with multiple shapes.
 */
void ModulationMatrix::Lfo::prepare(double sampleRate)
{
    sampleRateHz = sampleRate;
    reset();
}

void ModulationMatrix::Lfo::reset()
{
    phase = 0.0f;
    currentValue = 0.0f;
    randomStart = nextRandom();
    randomTarget = nextRandom();
}

float ModulationMatrix::Lfo::nextRandom()
{
    return rng.nextFloat() * 2.0f - 1.0f;
}

void ModulationMatrix::Lfo::setConfig(const LfoConfig& newConfig)
{
    config = newConfig;
    config.rateHz = juce::jlimit(0.01f, 20.0f, config.rateHz);
    config.pulseWidth = juce::jlimit(0.05f, 0.95f, config.pulseWidth);
    config.skew = juce::jlimit(0.05f, 0.95f, config.skew);
    config.phaseOffset = juce::jlimit(0.0f, 1.0f, config.phaseOffset);
}

void ModulationMatrix::Lfo::process(int numSamples)
{
    if (sampleRateHz <= 0.0 || numSamples <= 0)
        return;

    const float increment = config.rateHz > 0.0f
        ? static_cast<float>(config.rateHz * numSamples / sampleRateHz)
        : 0.0f;

    float newPhase = phase + increment;
    bool wrapped = false;
    if (newPhase >= 1.0f)
    {
        newPhase -= std::floor(newPhase);
        wrapped = true;
    }
    phase = newPhase;

    if (wrapped && (config.shape == LfoShape::RandomHold || config.shape == LfoShape::SmoothRandom))
    {
        randomStart = randomTarget;
        randomTarget = nextRandom();
    }

    float phaseValue = phase + config.phaseOffset;
    if (phaseValue >= 1.0f)
        phaseValue -= std::floor(phaseValue);

    switch (config.shape)
    {
        case LfoShape::Sine:
            currentValue = std::sin(phaseValue * juce::MathConstants<float>::twoPi);
            break;
        case LfoShape::Triangle:
        {
            const float tri = 2.0f * std::abs(2.0f * phaseValue - 1.0f) - 1.0f;
            currentValue = tri;
            break;
        }
        case LfoShape::SawUp:
            currentValue = 2.0f * phaseValue - 1.0f;
            break;
        case LfoShape::SawDown:
            currentValue = 1.0f - 2.0f * phaseValue;
            break;
        case LfoShape::Square:
            currentValue = (phaseValue < config.pulseWidth) ? 1.0f : -1.0f;
            break;
        case LfoShape::RandomHold:
            currentValue = randomTarget;
            break;
        case LfoShape::SmoothRandom:
            currentValue = juce::jmap(phaseValue, randomStart, randomTarget);
            break;
        case LfoShape::SkewedTriangle:
        {
            const float skew = config.skew;
            if (phaseValue < skew)
                currentValue = (phaseValue / skew) * 2.0f - 1.0f;
            else
                currentValue = (1.0f - (phaseValue - skew) / (1.0f - skew)) * 2.0f - 1.0f;
            break;
        }
        default:
            currentValue = 0.0f;
            break;
    }
}

// ============================================================================
// MODULATION MATRIX IMPLEMENTATION
// ============================================================================

ModulationMatrix::ModulationMatrix()
{
    // Initialize smoothers with default time constants
    for (auto& smoother : smoothers)
    {
        smoother.reset(48000.0, kDefaultSmoothingMs);  // 200ms default smoothing
        smoother.setCurrentAndTargetValue(0.0f);
    }

    modulationValues.fill(0.0f);

    // Default LFO configurations (disabled until connections are made)
    lfoConfigs[0] = {0.05f, LfoShape::Sine, 0.5f, 0.5f, 0.0f};
    lfoConfigs[1] = {0.10f, LfoShape::Triangle, 0.5f, 0.5f, 0.0f};
    lfoConfigs[2] = {0.20f, LfoShape::SawUp, 0.5f, 0.5f, 0.0f};
    lfoConfigs[3] = {0.35f, LfoShape::SawDown, 0.5f, 0.5f, 0.0f};
    lfoConfigs[4] = {0.60f, LfoShape::Square, 0.5f, 0.5f, 0.0f};
    lfoConfigs[5] = {0.15f, LfoShape::SmoothRandom, 0.5f, 0.5f, 0.0f};

    // Initialize RNG for probability gating
    std::random_device rd;
    probabilityRng.seed(rd());

    midiCCValues.fill(0.0f);
    midiPitchBend = 0.0f;
    midiChannelPressure = 0.0f;

    for (auto& snapshot : smoothingSnapshots)
        snapshot.fill(kDefaultSmoothingMs);

    snapshotCounts.fill(0);
    activeSnapshotIndex.store(0, std::memory_order_relaxed);
    publishConnectionsSnapshot();

    activeLfoConfigIndex.store(0, std::memory_order_relaxed);
    publishLfoConfigSnapshot();
}

ModulationMatrix::~ModulationMatrix() = default;

void ModulationMatrix::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    sampleRateHz = sampleRate;
    maxBlockSizeInternal = maxBlockSize;
    numChannelsInternal = numChannels;

    // Allocate modulation sources (stub implementations for Phase 1)
    chaosGen = std::make_unique<ChaosAttractor>();
    audioFollower = std::make_unique<AudioFollower>();
    brownianGen = std::make_unique<BrownianMotion>();
    envTracker = std::make_unique<EnvelopeTracker>();

    // Prepare all sources
    chaosGen->prepare(sampleRate, maxBlockSize);
    audioFollower->prepare(sampleRate, maxBlockSize);
    brownianGen->prepare(sampleRate, maxBlockSize);
    envTracker->prepare(sampleRate, maxBlockSize);

    for (size_t i = 0; i < lfos.size(); ++i)
    {
        lfos[i].prepare(sampleRate);
    }

    // Re-initialize smoothers with correct sample rate
    for (auto& smoother : smoothers)
    {
        smoother.reset(sampleRate, kDefaultSmoothingMs);  // 200ms default
        smoother.setCurrentAndTargetValue(0.0f);
    }

    appliedSmoothingSnapshotIndex = -1;
    appliedLfoConfigIndex = -1;

    const int smoothingSnapshotIndex =
        activeSnapshotIndex.load(std::memory_order_acquire);
    applySmootherSnapshot(smoothingSnapshotIndex);

    const int lfoConfigIndex =
        activeLfoConfigIndex.load(std::memory_order_acquire);
    applyLfoConfigSnapshot(lfoConfigIndex);

    modulationValues.fill(0.0f);
    midiCCValues.fill(0.0f);
    midiPitchBend = 0.0f;
    midiChannelPressure = 0.0f;
}

void ModulationMatrix::reset()
{
    if (chaosGen) chaosGen->reset();
    if (audioFollower) audioFollower->reset();
    if (brownianGen) brownianGen->reset();
    if (envTracker) envTracker->reset();
    for (auto& lfo : lfos)
        lfo.reset();

    for (auto& smoother : smoothers)
        smoother.setCurrentAndTargetValue(0.0f);

    modulationValues.fill(0.0f);
    midiCCValues.fill(0.0f);
    midiPitchBend = 0.0f;
    midiChannelPressure = 0.0f;
}

void ModulationMatrix::processMidi(const juce::MidiBuffer& midiMessages)
{
    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();
        if (msg.isController())
        {
            const int cc = msg.getControllerNumber();
            if (cc >= 0 && cc < static_cast<int>(midiCCValues.size()))
                midiCCValues[static_cast<size_t>(cc)] = msg.getControllerValue() / 127.0f;
        }
        else if (msg.isPitchWheel())
        {
            const int value = msg.getPitchWheelValue(); // 0..16383
            midiPitchBend = juce::jlimit(-1.0f, 1.0f, (value - 8192) / 8192.0f);
        }
        else if (msg.isChannelPressure())
        {
            midiChannelPressure = msg.getChannelPressureValue() / 127.0f;
        }
    }
}

void ModulationMatrix::publishConnectionsSnapshot() noexcept
{
    const int nextIndex =
        1 - activeSnapshotIndex.load(std::memory_order_relaxed);

    snapshotCounts[nextIndex] = connectionCount;
    for (int i = 0; i < connectionCount; ++i)
        connectionSnapshots[nextIndex][i] = connections[i];

    auto& smoothingSnapshot = smoothingSnapshots[nextIndex];
    smoothingSnapshot.fill(kDefaultSmoothingMs);
    for (int i = 0; i < connectionCount; ++i)
    {
        const auto& conn = connections[i];
        if (!conn.enabled)
            continue;
        const size_t destIdx = static_cast<size_t>(conn.destination);
        if (destIdx < smoothingSnapshot.size())
            smoothingSnapshot[destIdx] = conn.smoothingMs;
    }

    activeSnapshotIndex.store(nextIndex, std::memory_order_release);
}

void ModulationMatrix::publishLfoConfigSnapshot() noexcept
{
    const int nextIndex =
        1 - activeLfoConfigIndex.load(std::memory_order_relaxed);

    lfoConfigSnapshots[nextIndex] = lfoConfigs;
    activeLfoConfigIndex.store(nextIndex, std::memory_order_release);
}

void ModulationMatrix::applySmootherSnapshot(int snapshotIndex) noexcept
{
    if (snapshotIndex < 0 || snapshotIndex >= static_cast<int>(smoothingSnapshots.size()))
        return;

    const auto& smoothingSnapshot = smoothingSnapshots[snapshotIndex];
    for (size_t i = 0; i < smoothers.size(); ++i)
        smoothers[i].reset(sampleRateHz, smoothingSnapshot[i]);

    appliedSmoothingSnapshotIndex = snapshotIndex;
}

void ModulationMatrix::applyLfoConfigSnapshot(int snapshotIndex) noexcept
{
    if (snapshotIndex < 0 || snapshotIndex >= static_cast<int>(lfoConfigSnapshots.size()))
        return;

    const auto& configSnapshot = lfoConfigSnapshots[snapshotIndex];
    for (size_t i = 0; i < lfos.size(); ++i)
        lfos[i].setConfig(configSnapshot[i]);

    appliedLfoConfigIndex = snapshotIndex;
}

void ModulationMatrix::process(const juce::AudioBuffer<float>& audioBuffer, int numSamples)
{
    // Phase 3: Full modulation implementation
    // 1. Update all modulation sources
    // 2. Compute per-destination modulation sums from active connections
    // 3. Apply smoothing and store in modulationValues[]

    if (resetPending.exchange(false, std::memory_order_acq_rel))
    {
        for (size_t i = 0; i < smoothers.size(); ++i)
        {
            smoothers[i].setCurrentAndTargetValue(0.0f);
            modulationValues[i] = 0.0f;
        }
    }

    const int snapshotIndex = activeSnapshotIndex.load(std::memory_order_acquire);
    if (snapshotIndex != appliedSmoothingSnapshotIndex)
        applySmootherSnapshot(snapshotIndex);

    const int lfoConfigIndex = activeLfoConfigIndex.load(std::memory_order_acquire);
    if (lfoConfigIndex != appliedLfoConfigIndex)
        applyLfoConfigSnapshot(lfoConfigIndex);

    // Update modulation sources (block-rate)
    if (chaosGen)
        chaosGen->process(numSamples);

    if (audioFollower)
        audioFollower->process(audioBuffer, numSamples);

    if (brownianGen)
        brownianGen->process(numSamples);

    if (envTracker)
        envTracker->process(audioBuffer, numSamples);

    for (auto& lfo : lfos)
        lfo.process(numSamples);

    // Initialize per-destination accumulators
    std::array<float, static_cast<size_t>(DestinationType::Count)> destinationSums{};

    const int snapshotCount = snapshotCounts[snapshotIndex];

    // Early exit optimization: no connections = no processing needed
    if (snapshotCount == 0)
    {
        for (size_t i = 0; i < smoothers.size(); ++i)
            modulationValues[i] = smoothers[i].getNextValue();
        return;
    }

    const auto& snapshotConnections = connectionSnapshots[snapshotIndex];

    // Accumulate modulation from all active connections
    for (int connIdx = 0; connIdx < snapshotCount; ++connIdx)
    {
        const auto& conn = snapshotConnections[connIdx];
        if (!conn.enabled)
            continue;

        // Probability gating: randomly skip modulation based on probability value
        // 1.0 = always apply, 0.5 = apply 50% of the time, 0.0 = never apply
        if (conn.probability < 1.0f)
        {
            // Generate random number [0, 1] and compare to probability
            const float randomValue = probabilityDist(probabilityRng);
            if (randomValue > conn.probability)
                continue;  // Skip this connection for this block
        }

        // Get source value
        float sourceValue = 0.0f;
        switch (conn.source)
        {
            case SourceType::ChaosAttractor:
                sourceValue = chaosGen ? chaosGen->getValue(conn.sourceAxis) : 0.0f;
                break;
            case SourceType::AudioFollower:
                sourceValue = audioFollower ? audioFollower->getValue() : 0.0f;
                break;
            case SourceType::BrownianMotion:
                sourceValue = brownianGen ? brownianGen->getValue() : 0.0f;
                break;
            case SourceType::EnvelopeTracker:
                sourceValue = envTracker ? envTracker->getValue() : 0.0f;
                break;
            case SourceType::Lfo1:
            case SourceType::Lfo2:
            case SourceType::Lfo3:
            case SourceType::Lfo4:
            case SourceType::Lfo5:
            case SourceType::Lfo6:
            {
                const int index = static_cast<int>(conn.source) - static_cast<int>(SourceType::Lfo1);
                if (index >= 0 && index < static_cast<int>(lfos.size()))
                    sourceValue = lfos[static_cast<size_t>(index)].getValue();
                break;
            }
            case SourceType::MidiCC:
            {
                const int cc = juce::jlimit(0, 127, conn.sourceAxis);
                sourceValue = midiCCValues[static_cast<size_t>(cc)];
                break;
            }
            case SourceType::MidiPitchBend:
                sourceValue = midiPitchBend;
                break;
            case SourceType::MidiChannelPressure:
                sourceValue = midiChannelPressure;
                break;
            default:
                break;
        }

        const float shaped = applyCurve(sourceValue, conn.curveType, conn.curveAmount);
        // Scale by connection depth (bipolar: -1 to +1)
        const float modulation = shaped * conn.depth;

        // Accumulate to destination
        const size_t destIdx = static_cast<size_t>(conn.destination);
        if (destIdx < destinationSums.size())
            destinationSums[destIdx] += modulation;
    }

    // Apply smoothing and clamp to valid range
    for (size_t i = 0; i < smoothers.size(); ++i)
    {
        // Clamp accumulated modulation to [-1, 1]
        const float targetValue = juce::jlimit(-1.0f, 1.0f, destinationSums[i]);
        smoothers[i].setTargetValue(targetValue);

        // Get smoothed value for this block (prevents zipper noise)
        modulationValues[i] = smoothers[i].skip(numSamples);
    }
}

float ModulationMatrix::getModulation(DestinationType destination) const noexcept
{
    const size_t idx = static_cast<size_t>(destination);
    if (idx < modulationValues.size())
        return modulationValues[idx];
    return 0.0f;
}

void ModulationMatrix::setConnection(
    SourceType source,
    DestinationType destination,
    int sourceAxis,
    float depth,
    float smoothingMs,
    float probability,
    CurveType curveType,
    float curveAmount)
{
    // Sanitize inputs
    depth = juce::jlimit(-1.0f, 1.0f, depth);
    smoothingMs = juce::jlimit(20.0f, 1000.0f, smoothingMs);
    probability = juce::jlimit(0.0f, 1.0f, probability);
    curveAmount = juce::jlimit(0.0f, 1.0f, curveAmount);

    // Thread-safe: Only called from message thread (JUCE serialization)
    bool connectionChanged = false;

    // Find existing connection or create new one
    const int existingIdx = findConnectionIndex(source, destination, sourceAxis);

    if (existingIdx >= 0)
    {
        // Update existing connection
        auto& conn = connections[static_cast<size_t>(existingIdx)];
        conn.depth = depth;
        conn.smoothingMs = smoothingMs;
        conn.probability = probability;
        conn.curveType = curveType;
        conn.curveAmount = curveAmount;
        conn.enabled = true;
        connectionChanged = true;
    }
    else if (connectionCount < kMaxConnections)
    {
        // Create new connection in the fixed-size array
        auto& conn = connections[connectionCount];
        conn.source = source;
        conn.destination = destination;
        conn.sourceAxis = sourceAxis;
        conn.depth = depth;
        conn.smoothingMs = smoothingMs;
        conn.probability = probability;
        conn.curveType = curveType;
        conn.curveAmount = curveAmount;
        conn.enabled = true;
        ++connectionCount;
        connectionChanged = true;
    }
    // else: connection limit reached, silently ignore

    if (connectionChanged)
        publishConnectionsSnapshot();
}

void ModulationMatrix::removeConnection(
    SourceType source,
    DestinationType destination,
    int sourceAxis)
{
    // Thread-safe: Only called from message thread (JUCE serialization)
    const int idx = findConnectionIndex(source, destination, sourceAxis);
    if (idx >= 0 && idx < connectionCount)
    {
        // Shift remaining elements down to fill the gap
        for (int i = idx; i < connectionCount - 1; ++i)
            connections[i] = connections[i + 1];
        --connectionCount;
        publishConnectionsSnapshot();
    }
}

void ModulationMatrix::clearConnections()
{
    // Thread-safe: Only called from message thread (JUCE serialization)
    connectionCount = 0;  // No allocation, just reset counter
    publishConnectionsSnapshot();
    resetPending.store(true, std::memory_order_release);
}

void ModulationMatrix::setConnections(const std::vector<Connection>& newConnections)
{
    // Thread-safe: Only called from message thread (JUCE serialization)
    // Copy connections from vector to fixed-size array, respecting the max size
    connectionCount = std::min(static_cast<int>(newConnections.size()), kMaxConnections);
    for (int i = 0; i < connectionCount; ++i)
        connections[i] = newConnections[i];

    publishConnectionsSnapshot();
}

std::vector<ModulationMatrix::Connection> ModulationMatrix::getConnections() const noexcept
{
    // Lock-free: Read from active snapshot instead of master array
    // This ensures consistent view without blocking the audio thread
    const int snapshotIndex = activeSnapshotIndex.load(std::memory_order_acquire);
    const int snapshotCount = snapshotCounts[snapshotIndex];

    std::vector<Connection> result;
    result.reserve(snapshotCount);
    for (int i = 0; i < snapshotCount; ++i)
        result.push_back(connectionSnapshots[snapshotIndex][i]);
    return result;
}

float ModulationMatrix::getSourceValue(SourceType source, int axis) const noexcept
{
    // Phase 1: Stub (all sources return 0)
    // Phase 2: Query actual source values
    juce::ignoreUnused(source, axis);

    switch (source)
    {
        case SourceType::ChaosAttractor:
            return chaosGen ? chaosGen->getValue(axis) : 0.0f;
        case SourceType::AudioFollower:
            return audioFollower ? audioFollower->getValue() : 0.0f;
        case SourceType::BrownianMotion:
            return brownianGen ? brownianGen->getValue() : 0.0f;
        case SourceType::EnvelopeTracker:
            return envTracker ? envTracker->getValue() : 0.0f;
        case SourceType::Lfo1:
        case SourceType::Lfo2:
        case SourceType::Lfo3:
        case SourceType::Lfo4:
        case SourceType::Lfo5:
        case SourceType::Lfo6:
        {
            const int index = static_cast<int>(source) - static_cast<int>(SourceType::Lfo1);
            if (index >= 0 && index < static_cast<int>(lfos.size()))
                return lfos[static_cast<size_t>(index)].getValue();
            return 0.0f;
        }
        case SourceType::MidiCC:
        {
            const int cc = juce::jlimit(0, 127, axis);
            return midiCCValues[static_cast<size_t>(cc)];
        }
        case SourceType::MidiPitchBend:
            return midiPitchBend;
        case SourceType::MidiChannelPressure:
            return midiChannelPressure;
        default:
            return 0.0f;
    }
}

void ModulationMatrix::setLfoConfig(int index, const LfoConfig& config)
{
    if (index < 0 || index >= static_cast<int>(lfos.size()))
        return;

    lfoConfigs[static_cast<size_t>(index)] = config;
    publishLfoConfigSnapshot();
}

ModulationMatrix::LfoConfig ModulationMatrix::getLfoConfig(int index) const noexcept
{
    if (index < 0 || index >= static_cast<int>(lfoConfigs.size()))
        return {};
    return lfoConfigs[static_cast<size_t>(index)];
}

int ModulationMatrix::findConnectionIndex(
    SourceType source,
    DestinationType destination,
    int axis) const noexcept
{
    for (int i = 0; i < connectionCount; ++i)
    {
        const auto& conn = connections[i];
        if (conn.source == source &&
            conn.destination == destination &&
            conn.sourceAxis == axis)
        {
            return i;
        }
    }
    return -1;
}

float ModulationMatrix::applyCurve(float value, CurveType curveType, float curveAmount)
{
    curveAmount = juce::jlimit(0.0f, 1.0f, curveAmount);
    if (curveType == CurveType::Linear || curveAmount <= 0.0f)
        return value;

    const float sign = value >= 0.0f ? 1.0f : -1.0f;
    float x = std::abs(value);

    switch (curveType)
    {
        case CurveType::EaseIn:
        {
            const float exponent = juce::jmap(curveAmount, 1.0f, 4.0f);
            x = std::pow(x, exponent);
            break;
        }
        case CurveType::EaseOut:
        {
            const float exponent = juce::jmap(curveAmount, 1.0f, 4.0f);
            x = 1.0f - std::pow(1.0f - x, exponent);
            break;
        }
        case CurveType::Sine:
            x = std::sin(x * juce::MathConstants<float>::halfPi);
            break;
        case CurveType::SCurve:
        {
            const float k = juce::jmap(curveAmount, 0.5f, 3.5f);
            x = std::tanh(k * (x * 2.0f - 1.0f));
            x = (x + 1.0f) * 0.5f;
            break;
        }
        case CurveType::Steps:
        {
            const int steps = juce::jlimit(2, 16, 2 + static_cast<int>(curveAmount * 14.0f));
            const float stepped = std::round(x * static_cast<float>(steps - 1))
                / static_cast<float>(steps - 1);
            x = stepped;
            break;
        }
        default:
            break;
    }

    return sign * x;
}

// Helper function for randomization with configurable parameters
namespace {
    void randomizeConnectionsHelper(
        ModulationMatrix* matrix,
        int minConnections, int maxConnections,
        float minDepth, float maxDepth)
    {
        // Thread-safe: Clear all existing connections first
        matrix->clearConnections();

        // Initialize random number generator with time-based seed for variety
        std::random_device rd;
        std::mt19937 rng(rd());

        const std::array<ModulationMatrix::SourceType, 10> kRandomSources{
            ModulationMatrix::SourceType::ChaosAttractor,
            ModulationMatrix::SourceType::AudioFollower,
            ModulationMatrix::SourceType::BrownianMotion,
            ModulationMatrix::SourceType::EnvelopeTracker,
            ModulationMatrix::SourceType::Lfo1,
            ModulationMatrix::SourceType::Lfo2,
            ModulationMatrix::SourceType::Lfo3,
            ModulationMatrix::SourceType::Lfo4,
            ModulationMatrix::SourceType::Lfo5,
            ModulationMatrix::SourceType::Lfo6
        };

        // Distribution for number of connections
        std::uniform_int_distribution<int> numConnectionsDist(minConnections, maxConnections);
        const int targetConnections = numConnectionsDist(rng);

        // Distributions for random parameters
        std::uniform_int_distribution<int> sourceDist(0, static_cast<int>(kRandomSources.size()) - 1);
        std::uniform_int_distribution<int> destDist(0, static_cast<int>(ModulationMatrix::DestinationType::Count) - 1);
        std::uniform_real_distribution<float> depthDist(minDepth, maxDepth);
        std::uniform_int_distribution<int> smoothingDist(100, 500);   // 100-500ms smoothing

        // Bias: 70% positive depth, 30% negative depth (more musical)
        std::uniform_real_distribution<float> signDist(0.0f, 1.0f);

        // Create random connections, avoiding duplicates
        int attempts = 0;
        const int maxAttempts = targetConnections * 10;  // Prevent infinite loops

        int connectionCount = 0;
        while (connectionCount < targetConnections && attempts < maxAttempts)
        {
            attempts++;

            // Generate random source and destination
            auto source = kRandomSources[static_cast<size_t>(sourceDist(rng))];
            auto dest = static_cast<ModulationMatrix::DestinationType>(destDist(rng));

            // Determine source axis (chaos has 3 axes X/Y/Z, others have 1)
            int sourceAxis = 0;
            if (source == ModulationMatrix::SourceType::ChaosAttractor)
            {
                std::uniform_int_distribution<int> axisDist(0, 2);  // X, Y, or Z
                sourceAxis = axisDist(rng);
            }

            // Skip if connection already exists (check via matrix)
            const auto& connections = matrix->getConnections();
            bool exists = false;
            for (const auto& conn : connections)
            {
                if (conn.source == source && conn.destination == dest && conn.sourceAxis == sourceAxis)
                {
                    exists = true;
                    break;
                }
            }
            if (exists)
                continue;

            // Generate random depth with sign bias (70% positive, 30% negative)
            float depth = depthDist(rng);
            if (signDist(rng) < 0.3f)  // 30% chance of negative
                depth = -depth;

            // Generate random smoothing
            float smoothing = static_cast<float>(smoothingDist(rng));

            // Create the connection (uses setConnection which handles thread safety)
            matrix->setConnection(source, dest, sourceAxis, depth, smoothing);
            connectionCount++;
        }

        // If we couldn't create enough connections due to collisions, that's okay
        // We'll have at least a few interesting connections
    }
}

void ModulationMatrix::randomizeAll()
{
    randomizeConnectionsHelper(this, 4, 8, 0.2f, 0.6f);
}

void ModulationMatrix::randomizeSparse()
{
    randomizeConnectionsHelper(this, 2, 3, 0.2f, 0.4f);
}

void ModulationMatrix::randomizeDense()
{
    randomizeConnectionsHelper(this, 8, 12, 0.4f, 0.8f);
}

} // namespace dsp
} // namespace monument
