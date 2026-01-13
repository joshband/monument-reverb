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
     * @brief Copy constructor (safe for constant buffers).
     *
     * Repoints constant-mode data to this instance's storage to avoid dangling pointers.
     */
    ParameterBuffer(const ParameterBuffer& other) noexcept
        : data(other.data)
        , numSamples(other.numSamples)
        , isPerSample(other.isPerSample)
        , constantStorage(other.constantStorage)
    {
        if (!isPerSample)
            data = &constantStorage;
    }

    /**
     * @brief Copy assignment (safe for constant buffers).
     *
     * Repoints constant-mode data to this instance's storage to avoid dangling pointers.
     */
    ParameterBuffer& operator=(const ParameterBuffer& other) noexcept
    {
        if (this == &other)
            return *this;

        numSamples = other.numSamples;
        isPerSample = other.isPerSample;
        constantStorage = other.constantStorage;
        data = isPerSample ? other.data : &constantStorage;
        return *this;
    }

    /**
     * @brief Move constructor (safe for constant buffers).
     */
    ParameterBuffer(ParameterBuffer&& other) noexcept
        : data(other.data)
        , numSamples(other.numSamples)
        , isPerSample(other.isPerSample)
        , constantStorage(other.constantStorage)
    {
        if (!isPerSample)
            data = &constantStorage;
    }

    /**
     * @brief Move assignment (safe for constant buffers).
     */
    ParameterBuffer& operator=(ParameterBuffer&& other) noexcept
    {
        if (this == &other)
            return *this;

        numSamples = other.numSamples;
        isPerSample = other.isPerSample;
        constantStorage = other.constantStorage;
        data = isPerSample ? other.data : &constantStorage;
        return *this;
    }

    /**
     * @brief Default constructor (safe default - points to constant storage).
     */
    ParameterBuffer() noexcept
        : data(&constantStorage)
        , numSamples(0)
        , isPerSample(false)
    {
        constantStorage = 0.5f;  // Neutral default
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
     * @brief Ensure the pool can handle a given block size.
     *
     * Uses stack buffers for <= kMaxSamples, otherwise allocates heap buffers
     * during prepare time (not real-time critical).
     */
    void prepare(int numSamples) noexcept
    {
        if (numSamples <= kMaxSamples)
        {
            heapSamples = 0;
            return;
        }

        if (numSamples <= heapSamples)
            return;

        heapSamples = numSamples;
        timeHeap.calloc(static_cast<size_t>(heapSamples));
        massHeap.calloc(static_cast<size_t>(heapSamples));
        densityHeap.calloc(static_cast<size_t>(heapSamples));
        bloomHeap.calloc(static_cast<size_t>(heapSamples));
        gravityHeap.calloc(static_cast<size_t>(heapSamples));
        pillarShapeHeap.calloc(static_cast<size_t>(heapSamples));
        warpHeap.calloc(static_cast<size_t>(heapSamples));
        driftHeap.calloc(static_cast<size_t>(heapSamples));
    }

    /**
     * @brief Maximum buffer size supported by the pool.
     */
    int capacity() const noexcept
    {
        return heapSamples > 0 ? heapSamples : kMaxSamples;
    }

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

    float* getTimeBuffer(int numSamples) noexcept
    {
        return selectBuffer(timeBuffer, timeHeap, numSamples);
    }

    float* getMassBuffer(int numSamples) noexcept
    {
        return selectBuffer(massBuffer, massHeap, numSamples);
    }

    float* getDensityBuffer(int numSamples) noexcept
    {
        return selectBuffer(densityBuffer, densityHeap, numSamples);
    }

    float* getBloomBuffer(int numSamples) noexcept
    {
        return selectBuffer(bloomBuffer, bloomHeap, numSamples);
    }

    float* getGravityBuffer(int numSamples) noexcept
    {
        return selectBuffer(gravityBuffer, gravityHeap, numSamples);
    }

    float* getPillarShapeBuffer(int numSamples) noexcept
    {
        return selectBuffer(pillarShapeBuffer, pillarShapeHeap, numSamples);
    }

    float* getWarpBuffer(int numSamples) noexcept
    {
        return selectBuffer(warpBuffer, warpHeap, numSamples);
    }

    float* getDriftBuffer(int numSamples) noexcept
    {
        return selectBuffer(driftBuffer, driftHeap, numSamples);
    }

    const float* getTimeBuffer(int numSamples) const noexcept
    {
        return selectBuffer(timeBuffer, timeHeap, numSamples);
    }

    const float* getMassBuffer(int numSamples) const noexcept
    {
        return selectBuffer(massBuffer, massHeap, numSamples);
    }

    const float* getDensityBuffer(int numSamples) const noexcept
    {
        return selectBuffer(densityBuffer, densityHeap, numSamples);
    }

    const float* getBloomBuffer(int numSamples) const noexcept
    {
        return selectBuffer(bloomBuffer, bloomHeap, numSamples);
    }

    const float* getGravityBuffer(int numSamples) const noexcept
    {
        return selectBuffer(gravityBuffer, gravityHeap, numSamples);
    }

    const float* getPillarShapeBuffer(int numSamples) const noexcept
    {
        return selectBuffer(pillarShapeBuffer, pillarShapeHeap, numSamples);
    }

    const float* getWarpBuffer(int numSamples) const noexcept
    {
        return selectBuffer(warpBuffer, warpHeap, numSamples);
    }

    const float* getDriftBuffer(int numSamples) const noexcept
    {
        return selectBuffer(driftBuffer, driftHeap, numSamples);
    }

private:
    static float* selectBuffer(float* stackBuffer,
                               juce::HeapBlock<float>& heapBuffer,
                               int numSamples) noexcept
    {
        if (numSamples > kMaxSamples)
            return heapBuffer.get();

        return stackBuffer;
    }

    static const float* selectBuffer(const float* stackBuffer,
                                     const juce::HeapBlock<float>& heapBuffer,
                                     int numSamples) noexcept
    {
        if (numSamples > kMaxSamples)
            return heapBuffer.get();

        return stackBuffer;
    }

    int heapSamples{0};
    juce::HeapBlock<float> timeHeap;
    juce::HeapBlock<float> massHeap;
    juce::HeapBlock<float> densityHeap;
    juce::HeapBlock<float> bloomHeap;
    juce::HeapBlock<float> gravityHeap;
    juce::HeapBlock<float> pillarShapeHeap;
    juce::HeapBlock<float> warpHeap;
    juce::HeapBlock<float> driftHeap;
};
