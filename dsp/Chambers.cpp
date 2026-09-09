#include "Chambers.h"

#include <cmath>

namespace
{
// Feedback matrix order. Must match Chambers::kNumLines.
constexpr size_t kMatrixSize = 12;
using MatrixN = std::array<std::array<float, kMatrixSize>, kMatrixSize>;
constexpr size_t kSimdAlignment = juce::dsp::SIMDRegister<float>::SIMDRegisterSize;

// Householder reflection H = I - (2/N)*ones(N,N) across the all-ones vector.
// Exactly orthogonal for any N (H^T H = I algebraically), so this generalizes
// from 8 to 12 lines without needing a re-derivation.
constexpr float kHouseholderDiag = 1.0f - 2.0f / static_cast<float>(kMatrixSize);
constexpr float kHouseholderOff = -2.0f / static_cast<float>(kMatrixSize);

// Order-12 Hadamard matrix (Paley type-I construction over GF(11), 11 ≡ 3 mod 4),
// normalized by 1/sqrt(12). Verified H*H^T == 12*I. Order-8 Sylvester-style
// Hadamard matrices don't extend to order 12 (12 isn't a power of 2), so this
// is a different, independently-verified construction rather than a resize.
constexpr float kInvSqrt12 = 0.2886751345948129f;
alignas(kSimdAlignment) constexpr MatrixN kMatrixHadamard{{
    {{  kInvSqrt12,  kInvSqrt12,  kInvSqrt12,  kInvSqrt12,  kInvSqrt12,  kInvSqrt12,  kInvSqrt12,  kInvSqrt12,  kInvSqrt12,  kInvSqrt12,  kInvSqrt12,  kInvSqrt12 }},
    {{ -kInvSqrt12,  kInvSqrt12,  kInvSqrt12, -kInvSqrt12,  kInvSqrt12,  kInvSqrt12,  kInvSqrt12, -kInvSqrt12, -kInvSqrt12, -kInvSqrt12,  kInvSqrt12, -kInvSqrt12 }},
    {{ -kInvSqrt12, -kInvSqrt12,  kInvSqrt12,  kInvSqrt12, -kInvSqrt12,  kInvSqrt12,  kInvSqrt12,  kInvSqrt12, -kInvSqrt12, -kInvSqrt12, -kInvSqrt12,  kInvSqrt12 }},
    {{ -kInvSqrt12,  kInvSqrt12, -kInvSqrt12,  kInvSqrt12,  kInvSqrt12, -kInvSqrt12,  kInvSqrt12,  kInvSqrt12,  kInvSqrt12, -kInvSqrt12, -kInvSqrt12, -kInvSqrt12 }},
    {{ -kInvSqrt12, -kInvSqrt12,  kInvSqrt12, -kInvSqrt12,  kInvSqrt12,  kInvSqrt12, -kInvSqrt12,  kInvSqrt12,  kInvSqrt12,  kInvSqrt12, -kInvSqrt12, -kInvSqrt12 }},
    {{ -kInvSqrt12, -kInvSqrt12, -kInvSqrt12,  kInvSqrt12, -kInvSqrt12,  kInvSqrt12,  kInvSqrt12, -kInvSqrt12,  kInvSqrt12,  kInvSqrt12,  kInvSqrt12, -kInvSqrt12 }},
    {{ -kInvSqrt12, -kInvSqrt12, -kInvSqrt12, -kInvSqrt12,  kInvSqrt12, -kInvSqrt12,  kInvSqrt12,  kInvSqrt12, -kInvSqrt12,  kInvSqrt12,  kInvSqrt12,  kInvSqrt12 }},
    {{ -kInvSqrt12,  kInvSqrt12, -kInvSqrt12, -kInvSqrt12, -kInvSqrt12,  kInvSqrt12, -kInvSqrt12,  kInvSqrt12,  kInvSqrt12, -kInvSqrt12,  kInvSqrt12,  kInvSqrt12 }},
    {{ -kInvSqrt12,  kInvSqrt12,  kInvSqrt12, -kInvSqrt12, -kInvSqrt12, -kInvSqrt12,  kInvSqrt12, -kInvSqrt12,  kInvSqrt12,  kInvSqrt12, -kInvSqrt12,  kInvSqrt12 }},
    {{ -kInvSqrt12,  kInvSqrt12,  kInvSqrt12,  kInvSqrt12, -kInvSqrt12, -kInvSqrt12, -kInvSqrt12,  kInvSqrt12, -kInvSqrt12,  kInvSqrt12,  kInvSqrt12, -kInvSqrt12 }},
    {{ -kInvSqrt12, -kInvSqrt12,  kInvSqrt12,  kInvSqrt12,  kInvSqrt12, -kInvSqrt12, -kInvSqrt12, -kInvSqrt12,  kInvSqrt12, -kInvSqrt12,  kInvSqrt12,  kInvSqrt12 }},
    {{ -kInvSqrt12,  kInvSqrt12, -kInvSqrt12,  kInvSqrt12,  kInvSqrt12,  kInvSqrt12, -kInvSqrt12, -kInvSqrt12, -kInvSqrt12,  kInvSqrt12, -kInvSqrt12,  kInvSqrt12 }}
}};

alignas(kSimdAlignment) constexpr MatrixN kMatrixHouseholder{{
    {{ kHouseholderDiag, kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff }},
    {{ kHouseholderOff,  kHouseholderDiag, kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff }},
    {{ kHouseholderOff,  kHouseholderOff,  kHouseholderDiag, kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff }},
    {{ kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderDiag, kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff }},
    {{ kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderDiag, kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff }},
    {{ kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderDiag, kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff }},
    {{ kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderDiag, kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff }},
    {{ kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderDiag, kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff }},
    {{ kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderDiag, kHouseholderOff,  kHouseholderOff,  kHouseholderOff }},
    {{ kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderDiag, kHouseholderOff,  kHouseholderOff }},
    {{ kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderDiag, kHouseholderOff }},
    {{ kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderOff,  kHouseholderDiag }}
}};

// Delay lengths in samples at 48 kHz, spread from ~50ms to ~6.5s.
// Prime-based for incommensurate ratios; harmonic variants computed at runtime.
constexpr std::array<int, 12> kDelaySamples48k{
    2411, 4201, 7001, 11003, 17011, 26003, 39019, 59009, 89003, 135007, 205003, 310019
};

// Harmonic clustering delay ratios: multiply the base delays above to create
// musical relationships between lines, for resonant, pitched reverb character.
//
// Applied against a single shared fundamental (kDelaySamples48k[0], the
// shortest base delay) rather than each line's own already-large,
// widely-varying base delay (which itself spans ~129x across the 12 lines).
// Multiplying per-line bases by these ratios would compound the two ranges
// (e.g. OctaveStack's 2048x against the longest base is ~635M samples --
// no realistic buffer holds that, so nearly every line would clamp to the
// same maximum and lose the intended distinct spacing). A shared fundamental
// is also what "harmonic"/"octave" relationships actually mean musically:
// integer or power-of-2 multiples of one reference pitch, not of twelve
// unrelated ones.
constexpr std::array<float, 12> kHarmonic2xRatios{
    1.0f, 2.0f, 3.0f, 4.0f, 6.0f, 8.0f, 9.0f, 12.0f, 16.0f, 18.0f, 24.0f, 32.0f
};

constexpr std::array<float, 12> kHarmonic3xRatios{
    1.0f, 3.0f, 5.0f, 7.0f, 9.0f, 11.0f, 13.0f, 15.0f, 17.0f, 19.0f, 21.0f, 23.0f
};

// Spans -2 to +9 octaves from the fundamental (some lines shorter than it,
// most longer) rather than 0 to +11 (all longer): capped at 2^9 instead of
// the original 2^11 so the longest resulting delay (fundamental * 512) fits
// a bounded delay-buffer budget (see kHarmonicMaxGrowth) instead of the
// ~635M-sample worst case the uncapped per-line-base scheme produced.
constexpr std::array<float, 12> kOctaveRatios{
    0.25f, 0.5f, 1.0f, 2.0f, 4.0f, 8.0f, 16.0f, 32.0f, 64.0f, 128.0f, 256.0f, 512.0f
};
// Longest delay any harmonic-clustering mode can produce, as a multiple of
// the fundamental (kOctaveRatios' max is the largest of the three tables).
// prepare() sizes delayBufferLength to fit this alongside the Incommensurate
// case, so a mode switch never needs to (RT-unsafe) reallocate the buffer.
constexpr float kHarmonicMaxGrowth = 512.0f;

// Density evolution range: negative shifts density down over the decay
// (grainy -> smooth), positive shifts it up (smooth -> grainy).
constexpr float kDensityEvolutionMin = -1.0f;
constexpr float kDensityEvolutionMax = 1.0f;
constexpr float kDensityEvolutionRate = 0.001f;  // Per-sample smoothing rate at 48kHz

constexpr float kAttackTimeMaxSeconds = 10.0f;    // Longest slow-attack swell time
// Below this normalized value (10ms of the 10s max range), attack time is
// treated as instant. Expressed in normalized units so every call site
// checks the same threshold; a stray 0.01f compared directly against
// normalized attackTimeTarget in one place and against attackSeconds
// (target * kAttackTimeMaxSeconds) in another is a real 10x unit mismatch.
constexpr float kAttackTimeActiveThreshold = 0.001f;
constexpr std::array<int, 2> kInputDiffuserSamples48k{149, 223};

// Output decorrelation delays/coefficients (48 kHz): fixed, incommensurate
// per channel, chosen distinct from every other diffuser stage's values above.
constexpr std::array<int, 2> kOutputDecorrelatorSamples48k{211, 337};
constexpr std::array<float, 2> kOutputDecorrelatorCoeff{0.35f, 0.5f};

// Late diffusion delays (48 kHz), sub-10 ms, incommensurate across lines.
constexpr std::array<int, 12> kLateDiffuserSamples48k{
    157, 173, 197, 223, 251, 281, 313, 347, 383, 421, 463, 509
};

// Feedback diffusion delays (48 kHz), short taps for extra density inside the loop.
constexpr std::array<int, 12> kFeedbackDiffuserSamples48k{
    59, 73, 89, 97, 113, 131, 149, 167, 191, 211, 233, 257
};

// Per-line damping offset from the shared damping base, spreading tonal
// character (brighter/darker) across lines instead of one uniform filter.
constexpr std::array<float, 12> kDampingOffsets{
    -0.045f, -0.035f, -0.025f, -0.015f, -0.005f, 0.005f, 0.015f, 0.025f, 0.035f, 0.045f, 0.055f, 0.065f
};

constexpr std::array<float, 12> kLateDiffuserCoeffOffsets{
    -0.07f, -0.055f, -0.04f, -0.025f, -0.01f, 0.01f, 0.025f, 0.04f, 0.055f, 0.07f, 0.085f, 0.1f
};

constexpr std::array<float, 12> kFeedbackDiffuserCoeffOffsets{
    -0.05f, -0.04f, -0.03f, -0.02f, -0.01f, 0.01f, 0.02f, 0.03f, 0.04f, 0.05f, 0.06f, 0.07f
};

// Mid/side injection sign patterns: which lines receive the mid vs. side
// component in-phase vs. inverted, for stereo image spread across the FDN.
constexpr std::array<float, 12> kInputMid{
    1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f, -1.0f
};

constexpr std::array<float, 12> kInputSide{
    1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f, 1.0f, -1.0f, -1.0f, 1.0f
};

// Constant-power pan weights per line (no sign flips) so mono sum keeps all taps.
// Pan positions: {-0.95, 0.95, -0.85, 0.85, -0.75, 0.75, -0.65, 0.65, -0.55, 0.55, -0.45, 0.45}
constexpr std::array<float, 12> kOutputLeft{
    0.99968f, 0.0316f, 0.99452f, 0.10453f, 0.97629f, 0.21644f,
    0.94388f, 0.33043f, 0.89879f, 0.43837f, 0.84125f, 0.54064f
};

constexpr std::array<float, 12> kOutputRight{
    0.0316f, 0.99968f, 0.10453f, 0.99452f, 0.21644f, 0.97629f,
    0.33043f, 0.94388f, 0.43837f, 0.89879f, 0.54064f, 0.84125f
};

constexpr float kOutputGain = 0.408f; // sum(L^2) == sum(R^2) == 6.0 for 12 lines -> normalize to unity

constexpr float kGravityCutoffMinHz = 20.0f;
constexpr float kGravityCutoffMaxHz = 200.0f;
constexpr float kDCBlockerCutoffHz = 0.5f;  // Very low cutoff for strong DC rejection (<0.001)

// Freeze crossfades are kept long enough to avoid clicks and level jumps.
constexpr float kFreezeReleaseMs = 100.0f;
constexpr float kFreezeOutputFadeMs = 100.0f;
constexpr float kFreezeLimiterCeiling = 0.9f;
constexpr float kWetLimiterCeiling = 0.95f;
// Feedback coefficient is clamped below unity for stability (long tails without runaway).
constexpr float kMaxFeedback = 0.995f;
constexpr float kMinFeedback = 0.35f;
constexpr float kBloomSmoothingMs = 40.0f;
constexpr float kWarpSmoothingMs = 1200.0f;
constexpr float kDriftSmoothingMs = 1500.0f;
constexpr float kAdaptiveWarpSmoothingMs = 400.0f;
constexpr float kDriftRateMinHz = 0.05f;
constexpr float kDriftRateMaxHz = 0.2f;
// Drift depth stays within +/-1.0 sample to avoid audible pitch wobble.
constexpr float kDriftDepthMaxSamples = 1.0f;
constexpr float kJitterDepthMaxSamples = 0.6f;
constexpr float kJitterSmoothingMs = 500.0f;
constexpr float kFeedbackSaturationMaxDrive = 3.0f;
constexpr float kEnvelopeMinTimeSeconds = 1.0f;
constexpr float kEnvelopeMaxTimeSeconds = 12.0f;
constexpr float kBloomPeakGain = 0.5f; // Up to 1.5x at Bloom=1.
constexpr float kWarpMatrixEpsilon = 1.0e-4f;
constexpr float kMatrixNormEpsilon = 1.0e-6f;
constexpr float kMemoryInjectionGain = 1.8f;
constexpr float kMemoryEnvelopeTriggerScale = 1.5f;

inline float onePoleCoeffFromHz(float cutoffHz, double sampleRate)
{
    const double omega = 2.0 * juce::MathConstants<double>::pi
        * static_cast<double>(cutoffHz) / sampleRate;
    return static_cast<float>(std::exp(-omega));
}

inline float freezeHardLimit(float value)
{
    return juce::jlimit(-kFreezeLimiterCeiling, kFreezeLimiterCeiling, value);
}

inline float sanitizeNormalizedParameter(float value, float fallback, const char* label, bool& warned)
{
    juce::ignoreUnused(label, warned);
    if (!std::isfinite(value))
    {
#if JUCE_DEBUG
        if (!warned)
        {
            DBG("Chambers: non-finite " + juce::String(label) + " parameter ignored.");
            warned = true;
        }
#endif
        return fallback;
    }
    if (value < 0.0f || value > 1.0f)
    {
#if JUCE_DEBUG
        if (!warned)
        {
            DBG("Chambers: " + juce::String(label) + " parameter clamped.");
            warned = true;
        }
#endif
        return juce::jlimit(0.0f, 1.0f, value);
    }
    return value;
}

inline void blendMatrices(const MatrixN& a, const MatrixN& b, float blend, MatrixN& dest)
{
    const float invBlend = 1.0f - blend;
    for (size_t row = 0; row < kMatrixSize; ++row)
        for (size_t col = 0; col < kMatrixSize; ++col)
            dest[row][col] = invBlend * a[row][col] + blend * b[row][col];
}

inline void normalizeColumns(MatrixN& matrix)
{
    for (size_t col = 0; col < kMatrixSize; ++col)
    {
        float norm = 0.0f;
        for (size_t row = 0; row < kMatrixSize; ++row)
            norm += matrix[row][col] * matrix[row][col];
        if (norm > kMatrixNormEpsilon)
        {
            const float invNorm = 1.0f / std::sqrt(norm);
            for (size_t row = 0; row < kMatrixSize; ++row)
                matrix[row][col] *= invNorm;
        }
    }
}

inline void computeWarpMatrix(float warp, MatrixN& dest)
{
    // Warp morphs between orthogonal feedback topologies while keeping column energy stable.
    blendMatrices(kMatrixHadamard, kMatrixHouseholder, warp, dest);
    normalizeColumns(dest);
}

inline void applyMatrix(const MatrixN& matrix, const float* input, float* output)
{
    // Plain scalar loop (144 mults/sample) rather than a vectorized kernel.
    // kMatrixSize (12) doesn't divide evenly into an 8-wide SIMD register,
    // though it would into three 4-wide groups (as the 8-line version did for
    // 4-wide targets) -- that path was dropped for simplicity rather than
    // rewritten, since the scalar cost is already within the module's CPU
    // budget; revisit if profiling shows it matters.
    for (size_t row = 0; row < kMatrixSize; ++row)
    {
        float sum = 0.0f;
        for (size_t col = 0; col < kMatrixSize; ++col)
            sum += matrix[row][col] * input[col];
        output[row] = sum;
    }
}

inline float readFractionalDelay(const float* line, int length, int writePos, float delaySamples)
{
    const int delayInt = static_cast<int>(delaySamples);
    const float frac = delaySamples - static_cast<float>(delayInt);

    int readPosA = writePos - delayInt;
    if (readPosA < 0)
        readPosA += length;

    int readPosB = readPosA - 1;
    if (readPosB < 0)
        readPosB += length;

    const float a = line[readPosA];
    const float b = line[readPosB];
    return a * (1.0f - frac) + b * frac;
}
} // namespace

namespace monument
{
namespace dsp
{
void Chambers::prepare(double sampleRate, int blockSize, int numChannels)
{
    sampleRateHz = sampleRate;
    maxBlockSize = blockSize;
    channels = numChannels;

    const float scale = static_cast<float>(sampleRateHz / 48000.0);
    float maxDelay = 0.0f;
    float delaySum = 0.0f;

    for (size_t i = 0; i < kNumLines; ++i)
    {
        delaySamples[i] = juce::jmax(1.0f, static_cast<float>(kDelaySamples48k[i]) * scale);
        maxDelay = juce::jmax(maxDelay, delaySamples[i]);
        delaySum += delaySamples[i];
    }

    // Sized for the longer of the two worst cases -- plain Incommensurate
    // delays, or a harmonic-clustering mode's fundamental * kHarmonicMaxGrowth
    // -- so switching modes later never needs to (RT-unsafe) reallocate.
    const float fundamentalDelay = juce::jmax(1.0f, static_cast<float>(kDelaySamples48k[0]) * scale);
    const float harmonicMaxDelay = fundamentalDelay * kHarmonicMaxGrowth;
    delayBufferLength = juce::jmax(1,
        static_cast<int>(std::ceil(juce::jmax(maxDelay, harmonicMaxDelay))) + 2);
    meanDelaySeconds = (delaySum / static_cast<float>(kNumLines))
        / static_cast<float>(sampleRateHz);
    delayLines.setSize(kNumLines, delayBufferLength);
    delayLines.clear();
    writePositions.fill(0);
    lowpassState.fill(0.0f);
    gravityLowpassState.fill(0.0f);
    dampingCoefficients.fill(1.0f);

    // Input diffusion: short, per-channel allpass before injection to build density
    // without touching the feedback loop (keeps FDN topology unchanged).
    for (size_t i = 0; i < inputDiffusers.size(); ++i)
    {
        const int diffuserDelaySamples = juce::jmax(
            1, static_cast<int>(std::round(kInputDiffuserSamples48k[i] * scale)));
        inputDiffusers[i].setDelaySamples(diffuserDelaySamples);
        inputDiffusers[i].prepare();
    }

    // Late-field diffusion: per-line allpass after delay read, before output mix.
    // This increases echo density and breaks periodicity without affecting feedback stability.
    for (size_t i = 0; i < kNumLines; ++i)
    {
        const int diffuserDelaySamples = juce::jmax(
            1, static_cast<int>(std::round(kLateDiffuserSamples48k[i] * scale)));
        lateDiffusers[i].setDelaySamples(diffuserDelaySamples);
        lateDiffusers[i].prepare();
    }

    // Feedback diffusion: subtle allpass inside feedback loop for extra density.
    for (size_t i = 0; i < kNumLines; ++i)
    {
        const int diffuserDelaySamples = juce::jmax(
            1, static_cast<int>(std::round(kFeedbackDiffuserSamples48k[i] * scale)));
        feedbackDiffusers[i].setDelaySamples(diffuserDelaySamples);
        feedbackDiffusers[i].prepare();
    }

    // Output decorrelation: fixed per-channel allpass on the wet signal (see process()).
    for (size_t i = 0; i < outputDecorrelators.size(); ++i)
    {
        const int diffuserDelaySamples = juce::jmax(
            1, static_cast<int>(std::round(kOutputDecorrelatorSamples48k[i] * scale)));
        outputDecorrelators[i].setDelaySamples(diffuserDelaySamples);
        outputDecorrelators[i].setCoefficient(kOutputDecorrelatorCoeff[i]);
        outputDecorrelators[i].prepare();
    }

    driftDepthMaxSamples = kDriftDepthMaxSamples;
    {
        juce::Random random = testDriftSeed.has_value()
            ? juce::Random(*testDriftSeed)
            : juce::Random();
        for (size_t i = 0; i < kNumLines; ++i)
        {
            driftRateHz[i] = juce::jmap(random.nextFloat(), kDriftRateMinHz, kDriftRateMaxHz);
            driftPhase[i] = random.nextFloat() * juce::MathConstants<float>::twoPi;
        }
    }

    gravityCoeffMin = onePoleCoeffFromHz(kGravityCutoffMinHz, sampleRateHz);
    gravityCoeffMax = onePoleCoeffFromHz(kGravityCutoffMaxHz, sampleRateHz);
    dcBlockerCoeff = onePoleCoeffFromHz(kDCBlockerCutoffHz, sampleRateHz);
    freezeRampSamples = juce::jmax(
        1, static_cast<int>(std::round(sampleRateHz * (kFreezeReleaseMs / 1000.0f))));
    freezeOutputFadeSamples = juce::jmax(
        1, static_cast<int>(std::round(sampleRateHz * (kFreezeOutputFadeMs / 1000.0f))));
    freezeRampStep = 1.0f / static_cast<float>(freezeRampSamples);
    freezeRampRemaining = 0;
    freezeBlend = 1.0f;
    freezeRampingDown = false;
    wasFrozen = isFrozen;

    // Phase 4: Per-sample parameters now smoothed upstream in PluginProcessor
    // No smoothers needed here - eliminate double smoothing for time, mass, density, bloom, gravity

    // Block-rate smoothers (will be migrated later if needed)
    warpSmoother.prepare(sampleRateHz);
    warpSmoother.setSmoothingTimeMs(kWarpSmoothingMs); // Warp is intentionally slow to avoid motion artifacts.
    warpSmoother.setTarget(warpTarget);

    driftSmoother.prepare(sampleRateHz);
    driftSmoother.setSmoothingTimeMs(kDriftSmoothingMs); // Drift stays gentle and motion-safe.
    driftSmoother.setTarget(driftTarget);

    adaptiveWarpOffsetSmoother.reset(sampleRateHz, kAdaptiveWarpSmoothingMs / 1000.0f);
    adaptiveWarpOffsetSmoother.setCurrentAndTargetValue(0.0f);

    // Initialize ambient reverb shaping smoothers
    densityEvolutionSmoother.reset(sampleRateHz, kDensityEvolutionRate);
    densityEvolutionSmoother.setCurrentAndTargetValue(0.0f);
    
    attackTimeSmoother.reset(sampleRateHz, 0.1f);  // 100ms smoothing for attack parameter
    attackTimeSmoother.setCurrentAndTargetValue(0.0f);
    attackEnvelopeValue = 1.0f;
    attackEnvelopeRate = 0.0f;
    densityEvolutionFactor = 1.0f;

    // 5ms half-window (10ms total: fade out, swap, fade in) around a
    // warp-clustering mode change -- long enough to be click-free, short
    // enough that a mode switch still feels immediate.
    warpClusteringMuteHalfSamples = juce::jmax(1, static_cast<int>(sampleRateHz * 0.005));
    warpClusteringMuteCounter = -1;
    warpClusteringMuteGain = 1.0f;
    // Re-apply whatever mode was last selected (defaults to Incommensurate on
    // the very first prepare()) against the delaySamples/delayBufferLength
    // just computed above for the new sample rate -- otherwise a re-prepare
    // (sample rate or block size change) would silently drop back to
    // Incommensurate spacing despite warpClusteringMode still saying
    // otherwise, and a mode-switch mute already in flight would be abandoned
    // mid-fade by the delayLines.clear() above.
    applyWarpClusteringMode(warpClusteringMode);
    lastRequestedWarpClusteringMode = warpClusteringMode;
    pendingWarpClusteringMode = warpClusteringMode;

    // Initialize diffuser coefficient smoothers (8ms = fast but click-free)
    for (auto& smoother : inputDiffuserCoeffSmoothers)
        smoother.reset(sampleRateHz, 0.008);
    for (auto& smoother : lateDiffuserCoeffSmoothers)
        smoother.reset(sampleRateHz, 0.008);
    for (auto& smoother : feedbackDiffuserCoeffSmoothers)
        smoother.reset(sampleRateHz, 0.012);

    for (auto& smoother : jitterSmoothers)
        smoother.reset(sampleRateHz, kJitterSmoothingMs / 1000.0f);
    jitterTargets.fill(0.0f);

    warpSmoothed = warpTarget;
    computeWarpMatrix(warpSmoothed, warpMatrix);
    warpMatrixFrozen = warpMatrix;
    feedbackMatrix = warpMatrix;
    lastMatrixBlend = 1.0f;

    smoothersPrimed = false;
    envelopeTimeSeconds = 0.0f;
    envelopeValue = 1.0f;
    envelopeTriggerArmed = true;

    // Initialize spatial processor (Phase 1: Three-System Plan)
    if (!spatialProcessor)
        spatialProcessor = std::make_unique<SpatialProcessor>();
    spatialProcessor->prepare(sampleRateHz, maxBlockSize, kNumLines);
}

void Chambers::reset()
{
    delayLines.clear();
    writePositions.fill(0);
    lowpassState.fill(0.0f);
    gravityLowpassState.fill(0.0f);
    dcBlockerLowpassState.fill(0.0f);
    inputDCBlockerMidState = 0.0f;
    inputDCBlockerSideState = 0.0f;
    outputDCBlockerLeftState = 0.0f;
    outputDCBlockerRightState = 0.0f;
    for (auto& diffuser : inputDiffusers)
        diffuser.reset();
    for (auto& diffuser : lateDiffusers)
        diffuser.reset();
    for (auto& diffuser : feedbackDiffusers)
        diffuser.reset();
    for (auto& diffuser : outputDecorrelators)
        diffuser.reset();
    smoothersPrimed = false;
    freezeRampRemaining = 0;
    freezeBlend = 1.0f;
    freezeRampingDown = false;
    isFrozen = false;
    wasFrozen = false;
    externalInjection = nullptr;
    envelopeTimeSeconds = 0.0f;
    envelopeValue = 1.0f;
    envelopeTriggerArmed = true;
    warpSmoothed = warpTarget;
    adaptiveWarpOffsetSmoother.setCurrentAndTargetValue(0.0f);
    for (auto& smoother : jitterSmoothers)
        smoother.setCurrentAndTargetValue(0.0f);
    jitterTargets.fill(0.0f);
    
    // Reset ambient reverb shaping state. Preserves the caller's last
    // setDensityEvolution()/setAttackTime() targets (matching warpSmoothed's
    // preservation of warpTarget above) rather than silently reverting them
    // to their defaults on every reset().
    densityEvolutionSmoother.setCurrentAndTargetValue(densityEvolutionTarget);
    attackTimeSmoother.setCurrentAndTargetValue(attackTimeTarget);
    // Abandon any in-flight warp-clustering mode-switch crossfade cleanly
    // rather than leaving it counting down through a reset, and re-sync
    // pending/lastRequested to the mode actually in effect (delaySamples[]
    // itself is untouched here, unlike in prepare(), since reset() doesn't
    // recompute delayBufferLength).
    warpClusteringMuteCounter = -1;
    warpClusteringMuteGain = 1.0f;
    lastRequestedWarpClusteringMode = warpClusteringMode;
    pendingWarpClusteringMode = warpClusteringMode;
    attackEnvelopeValue = 1.0f;
    attackEnvelopeRate = attackTimeTarget > kAttackTimeActiveThreshold
        ? 1.0f / (attackTimeTarget * kAttackTimeMaxSeconds * static_cast<float>(sampleRateHz))
        : 1.0f;
    densityEvolutionFactor = 1.0f;

    // Reset spatial processor
    if (spatialProcessor)
        spatialProcessor->reset();
    computeWarpMatrix(warpSmoothed, warpMatrix);
    warpMatrixFrozen = warpMatrix;
    feedbackMatrix = warpMatrix;
    lastMatrixBlend = 1.0f;
}

void Chambers::process(juce::AudioBuffer<float>& buffer)
{
    juce::ScopedNoDenormals noDenormals;
    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = buffer.getNumChannels();

    // Update spatial attenuation coefficients (block-rate, Phase 1: Three-System Plan)
    if (spatialProcessor)
        spatialProcessor->process(numSamples);

    const float outputScale = kOutputGain; // Normalizes constant-power output mix to unity.

    auto* left = buffer.getWritePointer(0);
    auto* right = numChannels > 1 ? buffer.getWritePointer(1) : nullptr;
    const auto* injectionBuffer = externalInjection;
    const bool hasExternalInjection = injectionBuffer != nullptr
        && injectionBuffer->getNumChannels() >= 2
        && injectionBuffer->getNumSamples() >= numSamples;
    const auto* injectionL = hasExternalInjection ? injectionBuffer->getReadPointer(0) : nullptr;
    const auto* injectionR = hasExternalInjection ? injectionBuffer->getReadPointer(1) : nullptr;

    std::array<float*, kNumLines> lineData{};
    for (size_t i = 0; i < kNumLines; ++i)
        lineData[i] = delayLines.getWritePointer(static_cast<int>(i));

    std::array<float, kNumLines> outputLeftGains{};
    std::array<float, kNumLines> outputRightGains{};
    std::array<float, kNumLines> airAbsorptionGains{};
    if (spatialProcessor)
    {
        for (size_t i = 0; i < kNumLines; ++i)
        {
            float leftGain = 0.0f;
            float rightGain = 0.0f;
            spatialProcessor->getStereoGains(static_cast<int>(i), leftGain, rightGain);
            outputLeftGains[i] = leftGain;
            outputRightGains[i] = rightGain;
            airAbsorptionGains[i] = spatialProcessor->getAirAbsorptionGain(static_cast<int>(i));
        }
    }
    else
    {
        outputLeftGains = kOutputLeft;
        outputRightGains = kOutputRight;
        airAbsorptionGains.fill(1.0f);
    }

    if (adaptiveMatrixAmount > 0.0f)
    {
        float sumSquares = 0.0f;
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float inL = left[sample];
            const float inR = right != nullptr ? right[sample] : inL;
            sumSquares += 0.5f * (inL * inL + inR * inR);
        }
        const float rms = std::sqrt(sumSquares / static_cast<float>(juce::jmax(1, numSamples)));
        const float adaptiveTarget = adaptiveMatrixAmount * juce::jlimit(0.0f, 1.0f, rms * 2.0f);
        adaptiveWarpOffsetSmoother.setTargetValue(adaptiveTarget);
    }
    else
    {
        adaptiveWarpOffsetSmoother.setTargetValue(0.0f);
    }

    if (delayJitterAmount > 0.0f)
    {
        const float jitterStep = 0.15f * delayJitterAmount;
        for (size_t i = 0; i < kNumLines; ++i)
        {
            jitterTargets[i] = juce::jlimit(-1.0f, 1.0f,
                jitterTargets[i] + (jitterRandom.nextFloat() * 2.0f - 1.0f) * jitterStep);
            const float jitterSamples = jitterTargets[i] * delayJitterAmount * kJitterDepthMaxSamples;
            jitterSmoothers[i].setTargetValue(jitterSamples);
        }
    }
    else
    {
        for (auto& smoother : jitterSmoothers)
            smoother.setTargetValue(0.0f);
    }

    if (!smoothersPrimed)
    {
        // Phase 4: Per-sample parameters no longer need priming (smoothed upstream)
        // Only initialize block-rate smoothers and diffuser coefficients

        warpSmoother.reset(warpTarget);
        driftSmoother.reset(driftTarget);
        // Prime to the caller's already-set target instead of ramping in
        // from the prepare()-time default (0) -- matches warp/drift above, and
        // avoids a spurious multi-hundred-ms delay before setDensityEvolution()/
        // setAttackTime() take effect on the first block after prepare().
        densityEvolutionSmoother.setCurrentAndTargetValue(densityEvolutionTarget);
        attackTimeSmoother.setCurrentAndTargetValue(attackTimeTarget);

        // Initialize diffuser coefficient smoothers with current density from first buffer sample
        // Default to 0.5 if buffer is empty (will be set properly on first setDensity call)
        const float initialDensityNorm = densityBuffer.numSamples > 0 ? densityBuffer[0] : 0.5f;
        const float initialDensityShaped = juce::jmap(initialDensityNorm, 0.05f, 1.0f);
        const float initialInputCoeff = juce::jmap(initialDensityShaped, 0.12f, 0.6f);
        const float initialLateCoeffBase = juce::jmap(initialDensityShaped, 0.18f, 0.7f);
        const float initialFeedbackCoeffBase = juce::jmap(initialDensityShaped, 0.08f, 0.35f);
        for (auto& smoother : inputDiffuserCoeffSmoothers)
            smoother.setCurrentAndTargetValue(initialInputCoeff);
        for (size_t i = 0; i < kNumLines; ++i)
        {
            const float initialCoeff = initialLateCoeffBase * (1.0f + kLateDiffuserCoeffOffsets[i]);
            lateDiffuserCoeffSmoothers[i].setCurrentAndTargetValue(juce::jlimit(0.05f, 0.74f, initialCoeff));
            const float initialFeedbackCoeff = initialFeedbackCoeffBase * (1.0f + kFeedbackDiffuserCoeffOffsets[i]);
            feedbackDiffuserCoeffSmoothers[i].setCurrentAndTargetValue(juce::jlimit(0.05f, 0.6f, initialFeedbackCoeff));
            jitterSmoothers[i].setCurrentAndTargetValue(0.0f);
        }
        lastInputCoeffTarget = initialInputCoeff;
        lastLateCoeffBase = initialLateCoeffBase;
        lastFeedbackCoeffBase = initialFeedbackCoeffBase;

        warpSmoothed = warpTarget;
        computeWarpMatrix(warpSmoothed, warpMatrix);
        warpMatrixFrozen = warpMatrix;
        feedbackMatrix = warpMatrix;
        lastMatrixBlend = 1.0f;
        smoothersPrimed = true;
    }

    const bool freezeActive = isFrozen;
    const float driftPhaseStep = juce::MathConstants<float>::twoPi
        / static_cast<float>(sampleRateHz);
    if (freezeActive && !wasFrozen)
    {
        // Capture the active topology so Freeze holds the current spatial mapping.
        warpMatrixFrozen = feedbackMatrix;
        lastMatrixBlend = 0.0f;
    }

    for (int sample = 0; sample < numSamples; ++sample)
    {
        if (freezeRampRemaining > 0)
        {
            if (freezeRampingDown)
                freezeBlend = juce::jmax(0.0f, freezeBlend - freezeRampStep);
            else
                freezeBlend = juce::jmin(1.0f, freezeBlend + freezeRampStep);
            --freezeRampRemaining;
        }
        else
        {
            freezeBlend = freezeActive ? 0.0f : 1.0f;
        }
        bool warpMatrixDirty = false;
        if (!freezeActive)
        {
            const float adaptiveOffset = adaptiveWarpOffsetSmoother.getNextValue();
            const float warpNext = juce::jlimit(0.0f, 1.0f, warpSmoother.getNextValue() + adaptiveOffset);
            if (std::abs(warpNext - warpSmoothed) > kWarpMatrixEpsilon)
            {
                warpSmoothed = warpNext;
                computeWarpMatrix(warpSmoothed, warpMatrix);
                warpMatrixDirty = true;
            }
        }

        const float matrixBlend = freezeActive ? 0.0f : freezeBlend;
        if (freezeActive)
        {
            if (lastMatrixBlend != 0.0f)
            {
                feedbackMatrix = warpMatrixFrozen;
                lastMatrixBlend = 0.0f;
            }
        }
        else if (warpMatrixDirty || std::abs(matrixBlend - lastMatrixBlend) > kWarpMatrixEpsilon)
        {
            if (matrixBlend < 1.0f - kWarpMatrixEpsilon)
            {
                // Crossfade topologies during freeze release to avoid spatial jumps.
                blendMatrices(warpMatrixFrozen, warpMatrix, matrixBlend, feedbackMatrix);
                normalizeColumns(feedbackMatrix);
            }
            else
            {
                feedbackMatrix = warpMatrix;
            }
            lastMatrixBlend = matrixBlend;
        }

        // Phase 4: Direct per-sample buffer access (no smoothers, already smoothed upstream)
        // Eliminates double smoothing and reduces CPU overhead
        const float timeNorm = juce::jlimit(0.0f, 1.0f, timeBuffer[sample]);
        const float massNorm = juce::jlimit(0.0f, 1.0f, massBuffer[sample]);
        const float densityNormRaw = juce::jlimit(0.0f, 1.0f, densityBuffer[sample]);
        // Apply the density evolution factor computed from the previous
        // sample's envelope time (see below) -- this sample's diffusion-strength
        // calculations are the first consumer of densityNorm, so the factor must
        // already be known by this point; a strict same-sample dependency would
        // require reordering the freeze/envelope-trigger logic that decides it.
        const float densityNorm = juce::jlimit(0.0f, 1.0f, densityNormRaw * densityEvolutionFactor);
        const float gravityNorm = juce::jlimit(0.0f, 1.0f, gravityBuffer[sample]);
        const float bloomNorm = juce::jlimit(0.0f, 1.0f, bloomBuffer[sample]);
        // Drift subtly modulates delay lengths; depth ramps with freezeBlend and phases pause on freeze/ramp.
        const float driftNorm = juce::jlimit(0.0f, 1.0f, driftSmoother.getNextValue());
        const float driftDepthBase = driftNorm * driftDepthMaxSamples;
        const float driftDepth = freezeActive ? 0.0f : (driftDepthBase * freezeBlend);
        const bool advanceDrift = !freezeActive && (freezeRampRemaining == 0);

        // Time maps directly to feedback coefficient for long-tail control.
        float feedbackBaseLocal = juce::jmap(timeNorm, kMinFeedback, kMaxFeedback);
        if (feedbackBaseLocal > kMaxFeedback)
        {
#if JUCE_DEBUG
            static bool warned = false;
            if (!warned)
            {
                DBG("Chambers: feedback clamped for safety.");
                warned = true;
            }
#endif
            feedbackBaseLocal = kMaxFeedback;
        }
        // Mass darkens the tail by increasing HF damping up to 0.95.
        const float dampingBaseLocal = juce::jmap(massNorm, 0.1f, 0.95f);

        for (size_t i = 0; i < kNumLines; ++i)
        {
            const float damping = juce::jlimit(0.0f, 0.98f, dampingBaseLocal + kDampingOffsets[i]);
            const float targetCoeff = 1.0f - damping;
            dampingCoefficients[i] = 1.0f + freezeBlend * (targetCoeff - 1.0f);
        }

        // Density extends down to 0.05 for grainier, sparser ambience.
        const float densityShaped = juce::jmap(densityNorm, 0.05f, 1.0f);
        const float densityInputGain = juce::jmap(densityShaped, 0.18f, 0.32f);
        const float densityEarlyMix = juce::jmap(densityShaped, 0.45f, 0.25f);
        const float feedbackDiffusionMix = juce::jmap(densityShaped, 0.0f, 0.6f);

        // Density drives diffusion strength; coefficients stay below 0.75 for stability.
        // Per-sample smoothing prevents clicks in feedback path.
        // Only update targets when they change significantly to avoid constant ramp interruption.
        const float inputCoeffTarget = juce::jmap(densityShaped, 0.12f, 0.6f);
        const float lateCoeffBase = juce::jmap(densityShaped, 0.18f, 0.7f);
        const float feedbackCoeffBase = juce::jmap(densityShaped, 0.08f, 0.35f);

        constexpr float kCoeffTargetEpsilon = 0.001f;
        if (std::abs(inputCoeffTarget - lastInputCoeffTarget) > kCoeffTargetEpsilon)
        {
            inputDiffuserCoeffSmoothers[0].setTargetValue(inputCoeffTarget);
            inputDiffuserCoeffSmoothers[1].setTargetValue(inputCoeffTarget);
            lastInputCoeffTarget = inputCoeffTarget;
        }
        inputDiffusers[0].setCoefficient(inputDiffuserCoeffSmoothers[0].getNextValue());
        inputDiffusers[1].setCoefficient(inputDiffuserCoeffSmoothers[1].getNextValue());

        if (std::abs(lateCoeffBase - lastLateCoeffBase) > kCoeffTargetEpsilon)
        {
            for (size_t i = 0; i < kNumLines; ++i)
            {
                const float coeffTarget = lateCoeffBase * (1.0f + kLateDiffuserCoeffOffsets[i]);
                lateDiffuserCoeffSmoothers[i].setTargetValue(juce::jlimit(0.05f, 0.74f, coeffTarget));
            }
            lastLateCoeffBase = lateCoeffBase;
        }
        for (size_t i = 0; i < kNumLines; ++i)
            lateDiffusers[i].setCoefficient(lateDiffuserCoeffSmoothers[i].getNextValue());

        if (std::abs(feedbackCoeffBase - lastFeedbackCoeffBase) > kCoeffTargetEpsilon)
        {
            for (size_t i = 0; i < kNumLines; ++i)
            {
                const float coeffTarget = feedbackCoeffBase * (1.0f + kFeedbackDiffuserCoeffOffsets[i]);
                feedbackDiffuserCoeffSmoothers[i].setTargetValue(juce::jlimit(0.05f, 0.6f, coeffTarget));
            }
            lastFeedbackCoeffBase = feedbackCoeffBase;
        }
        for (size_t i = 0; i < kNumLines; ++i)
            feedbackDiffusers[i].setCoefficient(feedbackDiffuserCoeffSmoothers[i].getNextValue());

        // Freeze feedback below unity for stability (compensates for allpass gain and matrix non-idealities)
        // Reduced to 0.85 to prevent energy accumulation from numerical precision in diffusers/matrix
        // This provides adequate headroom for matrix non-orthogonality, floating-point errors,
        // and initial transient redistribution when freeze is engaged early
        constexpr float kFreezeFeedback = 0.85f;
        const float feedbackLocal = freezeActive
            ? kFreezeFeedback
            : 1.0f + freezeBlend * (feedbackBaseLocal - 1.0f);
        const float inputGainLocal = densityInputGain;
        const float earlyMixLocal = juce::jlimit(0.0f, 0.7f, densityEarlyMix * freezeBlend);

        const float inputScale = inputGainLocal * kInvSqrt12;
        const float gravityCoeff = juce::jlimit(
            0.0f, 1.0f, juce::jmap(gravityNorm, gravityCoeffMin, gravityCoeffMax));

        const float inL = left[sample];
        const float inR = right != nullptr ? right[sample] : inL;
        const float inputMagnitude = juce::jmax(std::abs(inL), std::abs(inR));
        const float memoryMagnitude = hasExternalInjection
            ? juce::jmax(std::abs(injectionL[sample]), std::abs(injectionR[sample]))
            : 0.0f;
        const float envelopeInputMagnitude = juce::jmax(
            inputMagnitude, memoryMagnitude * kMemoryEnvelopeTriggerScale);

        if (!freezeActive)
        {
            if (envelopeInputMagnitude > envelopeResetThreshold && envelopeTriggerArmed)
            {
                envelopeTimeSeconds = 0.0f;
                envelopeValue = 1.0f;
                envelopeTriggerArmed = false;
                // Start the slow-attack swell from silence on each new
                // transient. Without this, attackEnvelopeValue never leaves its
                // initial 1.0 and the attackTime parameter has no audible effect
                // (the per-sample ramp below only fires while it's still < 1.0).
                attackEnvelopeValue = 0.0f;
            }
            else if (envelopeInputMagnitude <= envelopeResetThreshold)
            {
                envelopeTriggerArmed = true;
            }

            envelopeTimeSeconds += static_cast<float>(1.0 / sampleRateHz);
            
            // Density evolution over time: shifts echo density up or down as the
            // decay progresses, rather than staying constant. Computes the factor
            // densityNorm will be multiplied by on the *next*
            // sample (see the top of this loop) -- this sample's own densityNorm
            // was already consumed by the diffusion-strength calculations above
            // before envelopeTimeSeconds was known for this sample.
            const float densityEvolution = densityEvolutionSmoother.getNextValue();
            // Clamped to keep the factor from growing unbounded (or flipping sign)
            // on very long, un-retriggered decays/freezes; the final densityNorm
            // is clamped to [0,1] separately where the factor is applied above.
            densityEvolutionFactor = juce::jlimit(0.0f, 2.0f,
                1.0f + densityEvolution * (envelopeTimeSeconds / kEnvelopeMaxTimeSeconds));
            
            // Slow-attack envelope: ramps the wet signal in gradually instead
            // of letting it hit full level instantly.
            const float attackNorm = attackTimeSmoother.getNextValue();
            if (attackNorm > kAttackTimeActiveThreshold && attackEnvelopeValue < 1.0f)
            {
                attackEnvelopeValue = juce::jmin(1.0f, attackEnvelopeValue + attackEnvelopeRate);
            }
            else if (attackNorm <= kAttackTimeActiveThreshold)
            {
                attackEnvelopeValue = 1.0f;  // Instant attack
            }
        }

        // Warp-clustering mode-switch click guard: ramps the output down,
        // swaps delaySamples[] to the pending mode at the window's midpoint
        // (when fully muted), then ramps back up. Runs regardless of freeze,
        // since frozen playback reads the same delaySamples[] array.
        if (warpClusteringMuteCounter >= 0)
        {
            const int totalWindow = warpClusteringMuteHalfSamples * 2;
            if (warpClusteringMuteCounter == warpClusteringMuteHalfSamples)
                applyWarpClusteringMode(pendingWarpClusteringMode);

            const int elapsed = totalWindow - warpClusteringMuteCounter;
            warpClusteringMuteGain = elapsed <= warpClusteringMuteHalfSamples
                ? 1.0f - static_cast<float>(elapsed) / static_cast<float>(warpClusteringMuteHalfSamples)
                : static_cast<float>(elapsed - warpClusteringMuteHalfSamples)
                    / static_cast<float>(warpClusteringMuteHalfSamples);
            warpClusteringMuteGain = juce::jlimit(0.0f, 1.0f, warpClusteringMuteGain);
            --warpClusteringMuteCounter;
        }
        else
        {
            warpClusteringMuteGain = 1.0f;
        }

        // Input diffusion is pre-FDN to build density without altering the feedback topology.
        float diffL = inL;
        float diffR = inR;
        if (!freezeActive)
        {
            const float processedL = inputDiffusers[0].processSample(inL);
            const float processedR = inputDiffusers[1].processSample(inR);
            diffL = inL + freezeBlend * (processedL - inL);
            diffR = inR + freezeBlend * (processedR - inR);
        }
        float mid = 0.5f * (diffL + diffR);
        float side = 0.5f * (diffL - diffR);
        // Apply DC blocker to input before injection to prevent DC accumulation
        const float inputDCMidLow = inputDCBlockerMidState
            + (1.0f - dcBlockerCoeff) * (mid - inputDCBlockerMidState);
        inputDCBlockerMidState = inputDCMidLow;
        mid = mid - inputDCMidLow;
        const float inputDCSideLow = inputDCBlockerSideState
            + (1.0f - dcBlockerCoeff) * (side - inputDCBlockerSideState);
        inputDCBlockerSideState = inputDCSideLow;
        side = side - inputDCSideLow;
        float memoryMid = 0.0f;
        float memorySide = 0.0f;
        if (hasExternalInjection)
        {
            float memL = injectionL[sample];
            float memR = injectionR[sample];
            if (!std::isfinite(memL) || !std::isfinite(memR))
            {
                memL = 0.0f;
                memR = 0.0f;
            }
            memoryMid = 0.5f * (memL + memR);
            memorySide = 0.5f * (memL - memR);
        }

        const float outputBlend = freezeBlend;
        alignas(kSimdAlignment) float outLive[kNumLines];
        alignas(kSimdAlignment) float outFrozen[kNumLines];
        for (size_t i = 0; i < kNumLines; ++i)
        {
            const int readPos = writePositions[i];
            if (advanceDrift)
            {
                driftPhase[i] += driftRateHz[i] * driftPhaseStep;
                if (driftPhase[i] >= juce::MathConstants<float>::twoPi)
                    driftPhase[i] -= juce::MathConstants<float>::twoPi;
            }
            const float modOffset = driftDepth != 0.0f
                ? std::sin(driftPhase[i]) * driftDepth
                : 0.0f;
            const float jitterOffset = jitterSmoothers[i].getNextValue();
            const float driftedDelay = juce::jmax(1.0f, delaySamples[i] + modOffset + jitterOffset);

            // Apply Doppler shift to delay time (Phase 3: Three-System Plan)
            float finalDelay = driftedDelay;
            if (spatialProcessor)
            {
                const float dopplerShift = spatialProcessor->getDopplerShift(static_cast<int>(i));
                finalDelay += dopplerShift;
                finalDelay = juce::jmax(1.0f, finalDelay); // Ensure positive delay
            }

            float delayedSample = readFractionalDelay(lineData[i], delayBufferLength, readPos, finalDelay);

            // Apply spatial distance attenuation (Phase 1: Three-System Plan)
            if (spatialProcessor)
            {
                const float spatialAttenuation = spatialProcessor->getAttenuationGain(static_cast<int>(i));
                delayedSample *= spatialAttenuation;
            }

            outLive[i] = delayedSample;
            if (driftDepth == 0.0f)
            {
                outFrozen[i] = outLive[i];
            }
            else
            {
                outFrozen[i] = readFractionalDelay(lineData[i], delayBufferLength, readPos, delaySamples[i]);
            }
        }

        alignas(kSimdAlignment) float feedback[kNumLines];
        applyMatrix(feedbackMatrix, outLive, feedback);

        alignas(kSimdAlignment) float lateOutLive[kNumLines];
        for (size_t i = 0; i < kNumLines; ++i)
        {
            // Late diffusion is post-read and pre-output mix to increase density
            // without placing allpass recursion inside the feedback loop.
            const float processed = lateDiffusers[i].processSample(outLive[i]);
            lateOutLive[i] = outLive[i] + freezeBlend * (processed - outLive[i]);
        }

        if (!freezeActive)
        {
            // Bloom shapes the late-field envelope by blending exponential decay with a plateau.
            const float envelopeTime = envelopeTimeSeconds;
            const float decayTimeSeconds = juce::jmap(
                timeNorm, kEnvelopeMinTimeSeconds, kEnvelopeMaxTimeSeconds);
            const float expEnv = std::exp(-envelopeTime / decayTimeSeconds);
            const float plateauFraction = 0.25f + 0.35f * bloomNorm;
            const float plateauTime = decayTimeSeconds * plateauFraction;
            const float plateauEnv = envelopeTime < plateauTime
                ? 1.0f
                : std::exp(-(envelopeTime - plateauTime) / decayTimeSeconds);
            const float bloomGain = 1.0f + kBloomPeakGain * (bloomNorm * bloomNorm);
            const float targetEnvelope = expEnv + bloomNorm * ((plateauEnv * bloomGain) - expEnv);
            envelopeValue = juce::jlimit(0.0f, 1.5f, targetEnvelope);
        }

        float wetLiveL = 0.0f;
        float wetLiveR = 0.0f;
        float wetFrozenL = 0.0f;
        float wetFrozenR = 0.0f;
        for (size_t i = 0; i < kNumLines; ++i)
        {
            const float airGain = airAbsorptionGains[i];
            wetLiveL += (lateOutLive[i] * airGain) * outputLeftGains[i];
            wetLiveR += (lateOutLive[i] * airGain) * outputRightGains[i];
            wetFrozenL += (outFrozen[i] * airGain) * outputLeftGains[i];
            wetFrozenR += (outFrozen[i] * airGain) * outputRightGains[i];
        }
        wetLiveL *= outputScale * envelopeValue * attackEnvelopeValue * warpClusteringMuteGain;
        wetLiveR *= outputScale * envelopeValue * attackEnvelopeValue * warpClusteringMuteGain;
        // Preserve the captured Bloom envelope during freeze crossfades.
        wetFrozenL *= outputScale * envelopeValue * attackEnvelopeValue * warpClusteringMuteGain;
        wetFrozenR *= outputScale * envelopeValue * attackEnvelopeValue * warpClusteringMuteGain;

        // Decorrelate the live wet signal only, before the live/frozen blend,
        // and only while fully unfrozen. Deliberately excluded from freeze
        // (including its ramp: freezeActive flips true on the very first block
        // after setFreeze(true), before freezeBlend finishes ramping down, and
        // lateOutLive[] itself still blends toward the frozen snapshot during
        // that ramp): the frozen tail converges toward a sustained, near-periodic
        // resonance, and this allpass's group delay beats against that
        // periodicity and against the still-morphing ramp, producing slow
        // amplitude modulation over tens of seconds (verified: applying it
        // unconditionally, and even applying it to wetLiveL/R only without this
        // gate, both broke Freeze Mode Stability's long-window RMS check).
        // Stereo Decorrelation never engages freeze, so this gate doesn't
        // affect it either way.
        if (right != nullptr && !freezeActive)
        {
            wetLiveL = outputDecorrelators[0].processSample(wetLiveL);
            wetLiveR = outputDecorrelators[1].processSample(wetLiveR);
        }

        float wetL = outputBlend * wetLiveL + (1.0f - outputBlend) * wetFrozenL;
        float wetR = outputBlend * wetLiveR + (1.0f - outputBlend) * wetFrozenR;
        wetL = juce::jlimit(-kWetLimiterCeiling, kWetLimiterCeiling, wetL);
        wetR = juce::jlimit(-kWetLimiterCeiling, kWetLimiterCeiling, wetR);

        for (size_t i = 0; i < kNumLines; ++i)
        {
            const float injection = (mid * kInputMid[i] + side * kInputSide[i])
                * inputScale * freezeBlend;
            const float memoryInjection = hasExternalInjection
                ? (memoryMid * kInputMid[i] + memorySide * kInputSide[i])
                    * kInvSqrt12 * kMemoryInjectionGain * freezeBlend
                : 0.0f;
            float feedbackSample = feedback[i] * feedbackLocal;
            if (feedbackDiffusionMix > 0.0f)
            {
                const float diffused = feedbackDiffusers[i].processSample(feedbackSample);
                feedbackSample += feedbackDiffusionMix * (diffused - feedbackSample);
            }
            if (feedbackSaturationAmount > 0.0f)
            {
                const float drive = 1.0f + feedbackSaturationAmount * (kFeedbackSaturationMaxDrive - 1.0f);
                const float norm = juce::dsp::FastMathApproximations::tanh(drive);
                feedbackSample = juce::dsp::FastMathApproximations::tanh(feedbackSample * drive)
                    / (norm > 0.0f ? norm : 1.0f);
            }
            const float writeValue = injection + memoryInjection + feedbackSample;
            const int writePos = writePositions[i];
            const float damped = lowpassState[i] + dampingCoefficients[i] * (writeValue - lowpassState[i]);
            lowpassState[i] = damped;
            // Gravity is a low-end containment high-pass inside the loop, after HF damping.
            // Always apply gravity filter to prevent DC accumulation (not just during unfrozen state)
            const float gravityLow = gravityLowpassState[i]
                + (1.0f - gravityCoeff) * (damped - gravityLowpassState[i]);
            gravityLowpassState[i] = gravityLow;
            const float gravityOut = damped - gravityLow;
            // Apply dedicated DC blocker (5Hz) for DC rejection below 0.001
            const float dcBlockerLow = dcBlockerLowpassState[i]
                + (1.0f - dcBlockerCoeff) * (gravityOut - dcBlockerLowpassState[i]);
            dcBlockerLowpassState[i] = dcBlockerLow;
            const float writeSample = gravityOut - dcBlockerLow;
            lineData[i][writePos] = freezeActive ? freezeHardLimit(writeSample) : writeSample;

            ++writePositions[i];
            if (writePositions[i] >= delayBufferLength)
                writePositions[i] = 0;
        }

        const float wetBlend = 1.0f - earlyMixLocal;
        float outL, outR;
        if (right != nullptr)
        {
            outL = inL * earlyMixLocal + wetL * wetBlend;
            outR = inR * earlyMixLocal + wetR * wetBlend;
            // Apply DC blocker to final output to remove DC from early mix passthrough
            const float outDCLeftLow = outputDCBlockerLeftState
                + (1.0f - dcBlockerCoeff) * (outL - outputDCBlockerLeftState);
            outputDCBlockerLeftState = outDCLeftLow;
            const float outDCRightLow = outputDCBlockerRightState
                + (1.0f - dcBlockerCoeff) * (outR - outputDCBlockerRightState);
            outputDCBlockerRightState = outDCRightLow;
            left[sample] = outL - outDCLeftLow;
            right[sample] = outR - outDCRightLow;
        }
        else
        {
            outL = mid * earlyMixLocal + (wetL + wetR) * 0.5f * wetBlend;
            const float outDCLeftLow = outputDCBlockerLeftState
                + (1.0f - dcBlockerCoeff) * (outL - outputDCBlockerLeftState);
            outputDCBlockerLeftState = outDCLeftLow;
            left[sample] = outL - outDCLeftLow;
        }
    }

    externalInjection = nullptr;
    wasFrozen = freezeActive;
}

void Chambers::setExternalInjection(const juce::AudioBuffer<float>* injectionBuffer)
{
    externalInjection = injectionBuffer;
}

void Chambers::setTime(const ParameterBuffer& time)
{
    // Phase 4: Store per-sample buffer reference (no validation needed - upstream already smoothed)
    timeBuffer = time;
}

void Chambers::setMass(const ParameterBuffer& mass)
{
    // Phase 4: Store per-sample buffer reference (no validation needed - upstream already smoothed)
    massBuffer = mass;
}

void Chambers::setDensity(const ParameterBuffer& density)
{
    // Phase 4: Store per-sample buffer reference (no validation needed - upstream already smoothed)
    densityBuffer = density;
}

void Chambers::setBloom(const ParameterBuffer& bloom)
{
    // Phase 4: Store per-sample buffer reference (no validation needed - upstream already smoothed)
    bloomBuffer = bloom;
}

void Chambers::setGravity(const ParameterBuffer& gravity)
{
    // Phase 4: Store per-sample buffer reference (no validation needed - upstream already smoothed)
    gravityBuffer = gravity;
}

void Chambers::setWarp(float warp)
{
    // Warp is clamped to [0, 1] and the blended matrix is re-normalized per column for stability.
    static bool warned = false;
    warpTarget = sanitizeNormalizedParameter(warp, warpTarget, "warp", warned);
    warpSmoother.setTarget(warpTarget);
}

void Chambers::setDrift(float drift)
{
    static bool warned = false;
    driftTarget = sanitizeNormalizedParameter(drift, driftTarget, "drift", warned);
    driftSmoother.setTarget(driftTarget);
}

void Chambers::setFreeze(bool shouldFreeze)
{
    if (shouldFreeze && !isFrozen)
    {
        isFrozen = true;
        freezeRampingDown = true;
        freezeRampRemaining = juce::jmax(1, freezeOutputFadeSamples);
        freezeRampStep = 1.0f / static_cast<float>(freezeRampRemaining);
        // Bypassed while frozen (see process()); reset so it doesn't replay
        // stale pre-freeze content into a click when unfreezing resumes it.
        for (auto& diffuser : outputDecorrelators)
            diffuser.reset();
    }
    else if (!shouldFreeze && isFrozen)
    {
        isFrozen = false;
        freezeRampingDown = false;
        freezeRampRemaining = juce::jmax(1, freezeRampSamples);
        freezeRampStep = 1.0f / static_cast<float>(freezeRampRemaining);
    }

}

void Chambers::setAdaptiveMatrixAmount(float amount)
{
    adaptiveMatrixAmount = juce::jlimit(0.0f, 1.0f, amount);
}

void Chambers::setFeedbackSaturation(float amount)
{
    feedbackSaturationAmount = juce::jlimit(0.0f, 1.0f, amount);
}

void Chambers::setDelayJitter(float amount)
{
    delayJitterAmount = juce::jlimit(0.0f, 1.0f, amount);
}

// ============================================================================
// Ambient reverb shaping controls
// ============================================================================

void Chambers::applyWarpClusteringMode(WarpClusteringMode mode)
{
    warpClusteringMode = mode;
    const float scale = static_cast<float>(sampleRateHz / 48000.0);
    // delayBufferLength is sized in prepare() to fit either case; -2 matches
    // its own margin. The clamp is a last-resort safety net (it should never
    // actually bind given kHarmonicMaxGrowth), not the mechanism keeping
    // lines distinct -- that's the fundamental-based scheme below.
    const float maxDelaySamples = static_cast<float>(juce::jmax(1, delayBufferLength - 2));

    if (warpClusteringMode == WarpClusteringMode::Incommensurate)
    {
        // Each line keeps its own prime-based base delay, unrelated to the
        // others -- the non-repeating, diffuse default.
        for (size_t i = 0; i < kNumLines; ++i)
        {
            delaySamples[i] = juce::jlimit(1.0f, maxDelaySamples,
                static_cast<float>(kDelaySamples48k[i]) * scale);
        }
        return;
    }

    // Harmonic/octave modes: every line is a ratio of one shared fundamental
    // (the shortest base delay), not of its own base -- see kOctaveRatios'
    // comment for why compounding against twelve already-different bases
    // doesn't work. This is also what "harmonic"/"octave" actually means:
    // multiples of a single reference, not of unrelated references.
    const float fundamentalDelay = juce::jmax(1.0f, static_cast<float>(kDelaySamples48k[0]) * scale);
    const std::array<float, 12>* ratios = nullptr;
    switch (warpClusteringMode)
    {
        case WarpClusteringMode::Harmonic2x:  ratios = &kHarmonic2xRatios; break;
        case WarpClusteringMode::Harmonic3x:  ratios = &kHarmonic3xRatios; break;
        case WarpClusteringMode::OctaveStack: ratios = &kOctaveRatios;     break;
        default: break; // unreachable: Incommensurate returned above
    }
    for (size_t i = 0; i < kNumLines; ++i)
    {
        delaySamples[i] = juce::jlimit(1.0f, maxDelaySamples,
            fundamentalDelay * (*ratios)[i]);
    }
}

void Chambers::setWarpClusteringMode(WarpClusteringMode mode)
{
    if (mode == lastRequestedWarpClusteringMode)
        return;
    lastRequestedWarpClusteringMode = mode;
    pendingWarpClusteringMode = mode;
    // Restart the mute window even if one is already in progress (e.g. rapid
    // automation): the in-flight target is superseded by this newer request.
    warpClusteringMuteCounter = warpClusteringMuteHalfSamples * 2;
}

void Chambers::setDensityEvolution(float evolution)
{
    // Density evolution controls how echo density changes over time
    // evolution = -1: decreasing density (smooth → grainy)
    // evolution = 0: constant density (traditional reverb)
    // evolution = +1: increasing density (grainy → smooth)
    densityEvolutionTarget = juce::jlimit(kDensityEvolutionMin, kDensityEvolutionMax, evolution);
    densityEvolutionSmoother.setTargetValue(densityEvolutionTarget);
}

void Chambers::setAttackTime(float attackTimeNorm)
{
    // Slow-attack reverb: a gradual ambient swell instead of an instant onset.
    // attackTimeNorm = 0: instant attack (traditional)
    // attackTimeNorm = 1: ~10 second slow attack (ambient swell)
    attackTimeTarget = juce::jlimit(0.0f, 1.0f, attackTimeNorm);
    attackTimeSmoother.setTargetValue(attackTimeTarget);
    
    // Compute attack envelope rate
    const float attackSeconds = attackTimeTarget * kAttackTimeMaxSeconds;
    if (attackTimeTarget > kAttackTimeActiveThreshold)
        attackEnvelopeRate = 1.0f / (attackSeconds * sampleRateHz);
    else
        attackEnvelopeRate = 1.0f;  // Instant attack
}

} // namespace dsp
} // namespace monument
