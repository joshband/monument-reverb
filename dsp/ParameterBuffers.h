#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <cstddef>

/**
 * @brief Lightweight view into per-sample or block-rate parameter data.
 *
 * Provides efficient access to parameter values that can be either:
 * - Per-sample arrays (for critical parameters requiring smooth automation)
 * - Block-rate constants (for non-critical parameters)
 *
 * Size: 16 bytes (8 pointer + 4 int + 1 bool + 3 padding)
 *
 * Usage:
 * @code
 * // Per-sample parameter
 * float timeData[512];
 * ParameterBuffer timeBuffer(timeData, 512);
 * for (int i = 0; i < 512; ++i) {
 *     float time = timeBuffer[i];  // Access per-sample value
 * }
 *
 * // Block-rate constant
 * ParameterBuffer airBuffer(0.5f, 512);
 * for (int i = 0; i < 512; ++i) {
 *     float air = airBuffer[i];  // Always returns 0.5f
 * }
 * @endcode
 */
struct ParameterBuffer
{
    const float* data;      ///< Pointer to parameter values
    int numSamples;         ///< Buffer length
    bool isPerSample;       ///< True: per-sample array, False: constant

    /**
     * @brief Access parameter value at given sample index.
     *
     * Branchless implementation: compiles to conditional move (cmov) on x86.
     * - Per-sample mode: returns data[index]
     * - Constant mode: returns data[0]
     *
     * @param index Sample index (0 to numSamples-1)
     * @return Parameter value at given sample
     */
    inline float operator[](int index) const noexcept
    {
        // Branchless: uses ternary which compiles to cmov instruction
        // No branch misprediction penalty, CPU-friendly
        return isPerSample ? data[index] : data[0];
    }

    /**
     * @brief Construct per-sample parameter buffer.
     * @param buf Pointer to per-sample parameter array
     * @param samples Number of samples in buffer
     */
    ParameterBuffer(const float* buf, int samples) noexcept
        : data(buf)
        , numSamples(samples)
        , isPerSample(true)
    {
    }

    /**
     * @brief Construct block-rate constant parameter.
     * @param constant Constant value for entire block
     * @param samples Number of samples (for interface consistency)
     */
    ParameterBuffer(float constant, int samples) noexcept
        : data(&constantStorage)
        , numSamples(samples)
        , isPerSample(false)
    {
        constantStorage = constant;
    }

    /**
     * @brief Default constructor (invalid buffer).
     */
    ParameterBuffer() noexcept
        : data(nullptr)
        , numSamples(0)
        , isPerSample(false)
    {
    }

private:
    mutable float constantStorage = 0.0f;  ///< Storage for block-rate constants
};

/**
 * @brief Pre-allocated buffer pool for critical parameters.
 *
 * Stack-allocated in PluginProcessor to avoid real-time allocations.
 * Buffers are 64-byte aligned for cache efficiency and SIMD-readiness.
 *
 * Size: 8 buffers × 2048 samples × 4 bytes = 64KB
 * Alignment: 64 bytes (cache-line size, prevents false sharing)
 *
 * Critical parameters requiring per-sample smoothing:
 * - time, mass, density, bloom, gravity (Chambers reverb)
 * - pillarShape (Pillars early reflections)
 * - warp, drift (Weathering modulation)
 *
 * Non-critical parameters (air, width, etc.) use block-rate averaging.
 */
struct ParameterBufferPool
{
    /// Maximum samples per block (2048 supports even extreme buffer sizes)
    static constexpr int kMaxSamples = 2048;

    // Critical parameters (per-sample smoothing)
    // 64-byte alignment prevents false sharing between CPU cores
    alignas(64) float timeBuffer[kMaxSamples];
    alignas(64) float massBuffer[kMaxSamples];
    alignas(64) float densityBuffer[kMaxSamples];
    alignas(64) float bloomBuffer[kMaxSamples];
    alignas(64) float gravityBuffer[kMaxSamples];
    alignas(64) float pillarShapeBuffer[kMaxSamples];
    alignas(64) float warpBuffer[kMaxSamples];
    alignas(64) float driftBuffer[kMaxSamples];

    /**
     * @brief Fill buffer with per-sample smoothed values from JUCE SmoothedValue.
     *
     * Advances the smoother and fills the destination buffer with
     * per-sample interpolated values.
     *
     * @param dest Destination buffer (must have space for numSamples)
     * @param smoother JUCE SmoothedValue to read from
     * @param numSamples Number of samples to generate
     *
     * @note This advances the smoother's internal state. Don't call multiple times
     *       per block for the same smoother unless you want double-advancement.
     */
    static void fillBuffer(float* dest,
                          juce::SmoothedValue<float>& smoother,
                          int numSamples) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
        {
            dest[i] = smoother.getNextValue();
        }
    }

    /**
     * @brief Get ParameterBuffer view for a given buffer.
     *
     * Helper to create ParameterBuffer views from pool buffers.
     *
     * @param buffer Pointer to buffer in pool
     * @param numSamples Number of valid samples
     * @return ParameterBuffer view (per-sample mode)
     */
    static ParameterBuffer makeView(const float* buffer, int numSamples) noexcept
    {
        return ParameterBuffer(buffer, numSamples);
    }
};
