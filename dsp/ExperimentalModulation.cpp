#include "ExperimentalModulation.h"

namespace monument
{
namespace dsp
{

//==============================================================================
// ModulationQuantizer Implementation
//==============================================================================

void ModulationQuantizer::setSteps(int numSteps) noexcept
{
    steps = juce::jlimit(2, 64, numSteps);
}

float ModulationQuantizer::quantize(float smoothValue) const noexcept
{
    if (steps <= 1)
        return smoothValue;

    // Snap to discrete steps
    const int stepIndex = static_cast<int>(smoothValue * static_cast<float>(steps));
    const int clampedIndex = juce::jlimit(0, steps - 1, stepIndex);

    // Map back to [0, 1] range
    return static_cast<float>(clampedIndex) / static_cast<float>(steps - 1);
}

//==============================================================================
// ProbabilityGate Implementation
//==============================================================================

ProbabilityGate::ProbabilityGate()
    : rng(std::random_device{}())
{
}

void ProbabilityGate::setProbability(float prob) noexcept
{
    probability = juce::jlimit(0.0f, 1.0f, prob);
}

void ProbabilityGate::setSmoothingMs(float ms, double sampleRate) noexcept
{
    const double smoothingTime = juce::jmax(0.001, ms / 1000.0);
    gateEnvelope.reset(sampleRate, smoothingTime);
}

void ProbabilityGate::prepare(double sampleRate)
{
    gateEnvelope.reset(sampleRate, 0.05);  // 50ms default smoothing
    gateEnvelope.setCurrentAndTargetValue(0.0f);
    currentlyActive = false;
}

bool ProbabilityGate::shouldBeActive()
{
    // Per-block decision to avoid zipper noise
    return dist(rng) < probability;
}

float ProbabilityGate::process(float inputModulation)
{
    // Check if gate should open/close (per-block decision)
    const bool targetState = shouldBeActive();

    if (targetState != currentlyActive)
    {
        currentlyActive = targetState;
        gateEnvelope.setTargetValue(currentlyActive ? 1.0f : 0.0f);
    }

    // Apply smoothed gate envelope to prevent clicks
    const float gateGain = gateEnvelope.getNextValue();
    return inputModulation * gateGain;
}

//==============================================================================
// SpringMassModulator Implementation
//==============================================================================

SpringMassModulator::SpringMassModulator() = default;

void SpringMassModulator::setSpringConstant(float k) noexcept
{
    springConstant = juce::jmax(0.01f, k);  // Prevent division by zero
}

void SpringMassModulator::setMass(float m) noexcept
{
    mass = juce::jmax(0.01f, m);  // Prevent division by zero
}

void SpringMassModulator::setDamping(float c) noexcept
{
    damping = juce::jmax(0.0f, c);
}

void SpringMassModulator::applyForce(float force) noexcept
{
    externalForce = force;
}

void SpringMassModulator::prepare(double sampleRate)
{
    dt = 1.0 / sampleRate;
    reset();
}

void SpringMassModulator::updatePhysics()
{
    // Semi-implicit Euler integration (more stable than explicit Euler for stiff springs)
    // F = ma = -kx - cv + F_ext
    // a = (-kx - cv + F_ext) / m

    const float springForce = -springConstant * position;
    const float dampingForce = -damping * velocity;
    const float totalForce = springForce + dampingForce + externalForce;

    const float acceleration = totalForce / mass;

    // Update velocity first (semi-implicit method)
    velocity += acceleration * static_cast<float>(dt);

    // Then update position using new velocity
    position += velocity * static_cast<float>(dt);

    // Soft limit position to prevent runaway oscillation
    position = juce::jlimit(-10.0f, 10.0f, position);
}

float SpringMassModulator::processSample()
{
    updatePhysics();
    return position;
}

void SpringMassModulator::reset() noexcept
{
    position = 0.0f;
    velocity = 0.0f;
    externalForce = 0.0f;
}

//==============================================================================
// PresetMorpher Implementation
//==============================================================================

PresetMorpher::PresetMorpher()
{
    // Initialize with empty parameter vectors
    for (auto& preset : presetParameters)
        preset.clear();
}

void PresetMorpher::setCornerPresets(int topLeft, int topRight,
                                      int bottomLeft, int bottomRight)
{
    cornerPresets[0] = topLeft;
    cornerPresets[1] = topRight;
    cornerPresets[2] = bottomLeft;
    cornerPresets[3] = bottomRight;
}

void PresetMorpher::setMorphPosition(float x, float y) noexcept
{
    morphX = juce::jlimit(0.0f, 1.0f, x);
    morphY = juce::jlimit(0.0f, 1.0f, y);
}

void PresetMorpher::loadPresetStates(const std::vector<std::vector<float>>& presetParams)
{
    jassert(presetParams.size() == 4);  // Must have exactly 4 presets

    for (size_t i = 0; i < 4; ++i)
    {
        presetParameters[i] = presetParams[i];

        // All presets must have same parameter count
        if (i > 0)
        {
            jassert(presetParameters[i].size() == presetParameters[0].size());
        }
    }
}

float PresetMorpher::bilinearInterpolate(float topLeft, float topRight,
                                          float bottomLeft, float bottomRight,
                                          float x, float y) const noexcept
{
    // Bilinear interpolation formula:
    // f(x,y) = (1-x)(1-y)·TL + x(1-y)·TR + (1-x)y·BL + xy·BR

    const float invX = 1.0f - x;
    const float invY = 1.0f - y;

    const float weightTL = invX * invY;
    const float weightTR = x * invY;
    const float weightBL = invX * y;
    const float weightBR = x * y;

    return weightTL * topLeft + weightTR * topRight +
           weightBL * bottomLeft + weightBR * bottomRight;
}

float PresetMorpher::getMorphedParameter(int parameterIndex) const noexcept
{
    // Validate parameter index
    if (parameterIndex < 0 || presetParameters[0].empty() ||
        parameterIndex >= static_cast<int>(presetParameters[0].size()))
    {
        return 0.0f;
    }

    // Get parameter value from each corner preset (cast to avoid sign-conversion warning)
    const auto index = static_cast<size_t>(parameterIndex);
    const float topLeft = presetParameters[0][index];
    const float topRight = presetParameters[1][index];
    const float bottomLeft = presetParameters[2][index];
    const float bottomRight = presetParameters[3][index];

    // Perform bilinear interpolation
    return bilinearInterpolate(topLeft, topRight, bottomLeft, bottomRight,
                                morphX, morphY);
}

//==============================================================================
// GestureRecorder Implementation
//==============================================================================

GestureRecorder::GestureRecorder() = default;

void GestureRecorder::startRecording()
{
    recordedValues.clear();
    recordedValues.reserve(10000);  // Pre-allocate for ~3 seconds at 48kHz/512 blocks
    recording = true;
    playing = false;
}

void GestureRecorder::stopRecording()
{
    recording = false;
}

void GestureRecorder::recordValue(float value)
{
    if (recording)
    {
        recordedValues.push_back(value);

        // Safety limit: max 60 seconds at 48kHz/512 blocks = ~5625 samples
        // But be conservative and cap at 180k for extreme cases
        if (recordedValues.size() > 180000)
        {
            stopRecording();
        }
    }
}

void GestureRecorder::startPlayback(float speed, bool loop)
{
    if (recordedValues.empty())
        return;

    playbackSpeed = juce::jlimit(0.1f, 10.0f, speed);
    looping = loop;
    playbackIndex = 0;
    playing = true;
    recording = false;
}

void GestureRecorder::stopPlayback()
{
    playing = false;
    playbackIndex = 0;
}

float GestureRecorder::getSample()
{
    if (!playing || recordedValues.empty())
        return 0.0f;

    // Get current sample (cast to avoid sign-conversion warning)
    const float sample = recordedValues[static_cast<size_t>(playbackIndex)];

    // Advance playback index with speed control
    playbackIndex += static_cast<int>(playbackSpeed);

    // Handle end of recording
    if (playbackIndex >= static_cast<int>(recordedValues.size()))
    {
        if (looping)
        {
            playbackIndex = playbackIndex % static_cast<int>(recordedValues.size());
        }
        else
        {
            playing = false;
            playbackIndex = 0;
            return 0.0f;
        }
    }

    return sample;
}

//==============================================================================
// ChaosSeeder Implementation
//==============================================================================

std::mt19937& ChaosSeeder::getRng()
{
    static std::mt19937 rng{std::random_device{}()};
    return rng;
}

std::vector<std::tuple<int, int, float>> ChaosSeeder::generateRandomConnections(
    int numConnections, int numSources, int numDestinations)
{
    std::vector<std::tuple<int, int, float>> connections;
    connections.reserve(static_cast<size_t>(numConnections));

    auto& rng = getRng();
    std::uniform_int_distribution<> sourceDist(0, numSources - 1);
    std::uniform_int_distribution<> destDist(0, numDestinations - 1);
    std::uniform_real_distribution<float> depthDist(0.2f, 0.6f);  // Musical range

    // Track used combinations to avoid duplicates
    std::set<std::pair<int, int>> usedPairs;

    int attempts = 0;
    const int maxAttempts = numConnections * 10;  // Safety limit

    while (connections.size() < static_cast<size_t>(numConnections) &&
           attempts < maxAttempts)
    {
        const int source = sourceDist(rng);
        const int dest = destDist(rng);

        // Skip if already used
        if (usedPairs.find({source, dest}) != usedPairs.end())
        {
            ++attempts;
            continue;
        }

        // 70% positive depth bias (more musical)
        float depth = depthDist(rng);
        std::uniform_real_distribution<float> biasDist(0.0f, 1.0f);
        if (biasDist(rng) > 0.7f)
        {
            depth = -depth;
        }

        connections.emplace_back(source, dest, depth);
        usedPairs.insert({source, dest});
        ++attempts;
    }

    return connections;
}

std::vector<float> ChaosSeeder::generateRandomProbabilities(int numConnections)
{
    std::vector<float> probabilities;
    probabilities.reserve(static_cast<size_t>(numConnections));

    auto& rng = getRng();
    std::uniform_real_distribution<float> dist(0.3f, 1.0f);  // 30-100% range

    for (int i = 0; i < numConnections; ++i)
    {
        probabilities.push_back(dist(rng));
    }

    return probabilities;
}

std::vector<int> ChaosSeeder::generateRandomQuantization(int numConnections)
{
    std::vector<int> steps;
    steps.reserve(static_cast<size_t>(numConnections));

    auto& rng = getRng();
    std::uniform_int_distribution<> dist(2, 16);  // 2-16 steps

    for (int i = 0; i < numConnections; ++i)
    {
        steps.push_back(dist(rng));
    }

    return steps;
}

} // namespace dsp
} // namespace monument