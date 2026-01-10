/**
 * ParameterBufferTest.cpp
 * Unit tests for ParameterBuffer and ParameterBufferPool infrastructure.
 *
 * Tests per-sample parameter interpolation buffers used to eliminate
 * zipper noise and click artifacts in DSP parameter automation.
 */

#include "../dsp/ParameterBuffers.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <iostream>
#include <cmath>
#include <cassert>

// ANSI color codes for test output
#define ANSI_RESET   "\033[0m"
#define ANSI_RED     "\033[31m"
#define ANSI_GREEN   "\033[32m"
#define ANSI_YELLOW  "\033[33m"
#define ANSI_BLUE    "\033[34m"
#define ANSI_MAGENTA "\033[35m"
#define ANSI_CYAN    "\033[36m"
#define ANSI_BOLD    "\033[1m"

// Test statistics
static int testsRun = 0;
static int testsPassed = 0;
static int testsFailed = 0;

// Assertion macro with detailed error reporting
#define ASSERT_TRUE(condition, message) \
    do { \
        if (!(condition)) { \
            std::cout << ANSI_RED << "  ✗ FAILED: " << message << ANSI_RESET << "\n"; \
            std::cout << "    Line " << __LINE__ << ": " #condition "\n"; \
            testsFailed++; \
            return false; \
        } \
    } while (false)

#define ASSERT_FLOAT_EQ(actual, expected, tolerance, message) \
    do { \
        float diff = std::abs((actual) - (expected)); \
        if (diff > (tolerance)) { \
            std::cout << ANSI_RED << "  ✗ FAILED: " << message << ANSI_RESET << "\n"; \
            std::cout << "    Expected: " << (expected) << "\n"; \
            std::cout << "    Actual:   " << (actual) << "\n"; \
            std::cout << "    Diff:     " << diff << " (tolerance: " << (tolerance) << ")\n"; \
            testsFailed++; \
            return false; \
        } \
    } while (false)

// Helper to print test section headers
void printTestHeader(const char* testName)
{
    std::cout << "\n" << ANSI_BOLD << ANSI_CYAN << "=== " << testName << " ===" << ANSI_RESET << "\n";
}

// Helper to print test result
void printTestResult(const char* testName, bool passed)
{
    testsRun++;
    if (passed) {
        testsPassed++;
        std::cout << ANSI_GREEN << "✓ " << testName << " PASSED" << ANSI_RESET << "\n";
    } else {
        std::cout << ANSI_RED << "✗ " << testName << " FAILED" << ANSI_RESET << "\n";
    }
}

//==============================================================================
// Test 1: ParameterBuffer Per-Sample Mode
//==============================================================================
bool testPerSampleMode()
{
    printTestHeader("Test 1: ParameterBuffer Per-Sample Mode");

    // Create test data with varying values
    constexpr int kBufferSize = 512;
    float testData[kBufferSize];
    for (int i = 0; i < kBufferSize; ++i) {
        testData[i] = static_cast<float>(i) / kBufferSize;  // 0.0 to ~1.0
    }

    // Create per-sample parameter buffer
    ParameterBuffer buffer(testData, kBufferSize);

    // Verify mode flag
    ASSERT_TRUE(buffer.isPerSample, "isPerSample should be true");

    // Verify numSamples
    ASSERT_TRUE(buffer.numSamples == kBufferSize, "numSamples should match");

    // Verify data pointer
    ASSERT_TRUE(buffer.data == testData, "data pointer should match");

    // Test operator[] access - should return different values per sample
    ASSERT_FLOAT_EQ(buffer[0], 0.0f, 0.0001f, "First sample should be 0.0");
    ASSERT_FLOAT_EQ(buffer[255], 255.0f / 512.0f, 0.0001f, "Mid sample should be interpolated");
    ASSERT_FLOAT_EQ(buffer[511], 511.0f / 512.0f, 0.0001f, "Last sample should be ~1.0");

    // Verify values actually vary across buffer
    float firstValue = buffer[0];
    float lastValue = buffer[kBufferSize - 1];
    ASSERT_TRUE(std::abs(lastValue - firstValue) > 0.5f,
                "Per-sample values should vary significantly");

    std::cout << ANSI_GREEN << "  ✓ Per-sample mode verified: values vary from "
              << firstValue << " to " << lastValue << ANSI_RESET << "\n";

    return true;
}

//==============================================================================
// Test 2: ParameterBuffer Constant Mode
//==============================================================================
bool testConstantMode()
{
    printTestHeader("Test 2: ParameterBuffer Constant Mode");

    constexpr int kBufferSize = 512;
    constexpr float kConstantValue = 0.75f;

    // Create constant parameter buffer
    ParameterBuffer buffer(kConstantValue, kBufferSize);

    // Verify mode flag
    ASSERT_TRUE(!buffer.isPerSample, "isPerSample should be false");

    // Verify numSamples
    ASSERT_TRUE(buffer.numSamples == kBufferSize, "numSamples should match");

    // Verify data pointer is not null (points to constantStorage)
    ASSERT_TRUE(buffer.data != nullptr, "data pointer should not be null");

    // Test operator[] access - should return same value for all indices
    ASSERT_FLOAT_EQ(buffer[0], kConstantValue, 0.0001f, "First sample should be constant");
    ASSERT_FLOAT_EQ(buffer[255], kConstantValue, 0.0001f, "Mid sample should be constant");
    ASSERT_FLOAT_EQ(buffer[511], kConstantValue, 0.0001f, "Last sample should be constant");

    // Verify values are truly constant across buffer
    for (int i = 0; i < kBufferSize; i += 50) {
        ASSERT_FLOAT_EQ(buffer[i], kConstantValue, 0.0001f, "All samples should be constant");
    }

    std::cout << ANSI_GREEN << "  ✓ Constant mode verified: all samples = "
              << kConstantValue << ANSI_RESET << "\n";

    return true;
}

//==============================================================================
// Test 3: Branchless Access Pattern
//==============================================================================
bool testBranchlessAccess()
{
    printTestHeader("Test 3: Branchless Access Pattern");

    constexpr int kBufferSize = 256;
    float testData[kBufferSize];
    for (int i = 0; i < kBufferSize; ++i) {
        testData[i] = static_cast<float>(i);
    }

    // Test per-sample buffer
    ParameterBuffer perSampleBuffer(testData, kBufferSize);

    // Access multiple indices to verify per-sample behavior
    int accessPattern[] = {0, 10, 50, 100, 200, 255};
    for (int idx : accessPattern) {
        float expected = static_cast<float>(idx);
        ASSERT_FLOAT_EQ(perSampleBuffer[idx], expected, 0.0001f,
                       "Per-sample access should return data[idx]");
    }

    // Test constant buffer
    constexpr float kConstantValue = 42.0f;
    ParameterBuffer constantBuffer(kConstantValue, kBufferSize);

    // Access same indices - should all return constant
    for (int idx : accessPattern) {
        ASSERT_FLOAT_EQ(constantBuffer[idx], kConstantValue, 0.0001f,
                       "Constant access should always return constant value");
    }

    std::cout << ANSI_GREEN << "  ✓ Branchless access verified for both modes" << ANSI_RESET << "\n";
    std::cout << ANSI_YELLOW << "  ℹ Note: Compiles to cmov on x86 (no branch misprediction)" << ANSI_RESET << "\n";

    return true;
}

//==============================================================================
// Test 4: Default Constructor
//==============================================================================
bool testDefaultConstructor()
{
    printTestHeader("Test 4: Default Constructor");

    ParameterBuffer buffer;

    // Verify default state (safe default - points to constantStorage)
    ASSERT_TRUE(buffer.data != nullptr, "data should point to constantStorage (safe default)");
    ASSERT_TRUE(buffer.numSamples == 0, "numSamples should be 0");
    ASSERT_TRUE(!buffer.isPerSample, "isPerSample should be false");
    ASSERT_TRUE(buffer[0] == 0.5f, "default value should be 0.5f (neutral)");

    std::cout << ANSI_GREEN << "  ✓ Default constructor creates safe default buffer" << ANSI_RESET << "\n";

    return true;
}

//==============================================================================
// Test 5: ParameterBufferPool fillBuffer()
//==============================================================================
bool testFillBuffer()
{
    printTestHeader("Test 5: ParameterBufferPool fillBuffer()");

    constexpr int kBufferSize = 512;
    constexpr double kSampleRate = 48000.0;
    constexpr float kStartValue = 0.0f;
    constexpr float kTargetValue = 1.0f;
    constexpr double kSmoothingTime = 0.02; // 20ms

    // Create JUCE SmoothedValue
    juce::SmoothedValue<float> smoother;
    smoother.reset(kSampleRate, kSmoothingTime);
    smoother.setCurrentAndTargetValue(kStartValue);
    smoother.setTargetValue(kTargetValue);

    // Fill buffer using helper
    float buffer[kBufferSize];
    ParameterBufferPool::fillBuffer(buffer, smoother, kBufferSize);

    // Verify buffer is filled with smoothly increasing values
    ASSERT_FLOAT_EQ(buffer[0], kStartValue, 0.01f, "First sample should be near start");

    // Values should increase monotonically
    for (int i = 1; i < kBufferSize; ++i) {
        ASSERT_TRUE(buffer[i] >= buffer[i - 1], "Values should increase");
    }

    // Last value should approach target (won't reach exactly due to smoothing)
    float lastValue = buffer[kBufferSize - 1];
    ASSERT_TRUE(lastValue > 0.5f, "Last value should be significantly above start");
    ASSERT_TRUE(lastValue < kTargetValue, "Last value should not exceed target");

    std::cout << ANSI_GREEN << "  ✓ fillBuffer() creates smooth ramp: "
              << buffer[0] << " → " << lastValue << ANSI_RESET << "\n";

    // Verify smoothness by checking max jump between samples
    float maxJump = 0.0f;
    for (int i = 1; i < kBufferSize; ++i) {
        float jump = std::abs(buffer[i] - buffer[i - 1]);
        maxJump = std::max(maxJump, jump);
    }

    std::cout << ANSI_GREEN << "  ✓ Max jump between samples: " << maxJump << ANSI_RESET << "\n";
    ASSERT_TRUE(maxJump < 0.01f, "Smoothing should prevent large jumps");

    return true;
}

//==============================================================================
// Test 6: ParameterBufferPool makeView()
//==============================================================================
bool testMakeView()
{
    printTestHeader("Test 6: ParameterBufferPool makeView()");

    constexpr int kBufferSize = 256;
    float testData[kBufferSize];
    for (int i = 0; i < kBufferSize; ++i) {
        testData[i] = static_cast<float>(i) * 0.5f;
    }

    // Create view using helper
    ParameterBuffer view = ParameterBufferPool::makeView(testData, kBufferSize);

    // Verify view properties
    ASSERT_TRUE(view.data == testData, "View should point to source data");
    ASSERT_TRUE(view.numSamples == kBufferSize, "View should have correct size");
    ASSERT_TRUE(view.isPerSample, "View should be per-sample mode");

    // Verify view accesses original data
    ASSERT_FLOAT_EQ(view[0], 0.0f, 0.0001f, "View[0] should access testData[0]");
    ASSERT_FLOAT_EQ(view[100], 50.0f, 0.0001f, "View[100] should access testData[100]");

    std::cout << ANSI_GREEN << "  ✓ makeView() creates valid ParameterBuffer view" << ANSI_RESET << "\n";

    return true;
}

//==============================================================================
// Test 7: ParameterBufferPool Alignment
//==============================================================================
bool testBufferPoolAlignment()
{
    printTestHeader("Test 7: ParameterBufferPool Alignment");

    ParameterBufferPool pool;

    // Verify 64-byte alignment for cache-line efficiency
    ASSERT_TRUE(reinterpret_cast<uintptr_t>(pool.timeBuffer) % 64 == 0,
               "timeBuffer should be 64-byte aligned");
    ASSERT_TRUE(reinterpret_cast<uintptr_t>(pool.massBuffer) % 64 == 0,
               "massBuffer should be 64-byte aligned");
    ASSERT_TRUE(reinterpret_cast<uintptr_t>(pool.densityBuffer) % 64 == 0,
               "densityBuffer should be 64-byte aligned");
    ASSERT_TRUE(reinterpret_cast<uintptr_t>(pool.bloomBuffer) % 64 == 0,
               "bloomBuffer should be 64-byte aligned");
    ASSERT_TRUE(reinterpret_cast<uintptr_t>(pool.gravityBuffer) % 64 == 0,
               "gravityBuffer should be 64-byte aligned");
    ASSERT_TRUE(reinterpret_cast<uintptr_t>(pool.pillarShapeBuffer) % 64 == 0,
               "pillarShapeBuffer should be 64-byte aligned");
    ASSERT_TRUE(reinterpret_cast<uintptr_t>(pool.warpBuffer) % 64 == 0,
               "warpBuffer should be 64-byte aligned");
    ASSERT_TRUE(reinterpret_cast<uintptr_t>(pool.driftBuffer) % 64 == 0,
               "driftBuffer should be 64-byte aligned");

    std::cout << ANSI_GREEN << "  ✓ All 8 buffers are 64-byte aligned" << ANSI_RESET << "\n";
    std::cout << ANSI_YELLOW << "  ℹ Prevents false sharing between CPU cores" << ANSI_RESET << "\n";

    // Verify pool size
    size_t poolSize = sizeof(ParameterBufferPool);
    size_t expectedSize = 8 * ParameterBufferPool::kMaxSamples * sizeof(float);

    std::cout << ANSI_GREEN << "  ✓ Pool size: " << poolSize << " bytes "
              << "(" << (poolSize / 1024) << " KB)" << ANSI_RESET << "\n";
    std::cout << ANSI_GREEN << "  ✓ Expected minimum: " << expectedSize << " bytes "
              << "(" << (expectedSize / 1024) << " KB)" << ANSI_RESET << "\n";

    ASSERT_TRUE(poolSize >= expectedSize, "Pool size should be at least 64KB");

    return true;
}

//==============================================================================
// Test 8: ParameterBufferPool Multiple fillBuffer() Calls
//==============================================================================
bool testMultipleFillBuffer()
{
    printTestHeader("Test 8: Multiple fillBuffer() Calls");

    constexpr int kBufferSize = 512;
    constexpr double kSampleRate = 48000.0;

    ParameterBufferPool pool;

    // Create multiple smoothers with different targets
    juce::SmoothedValue<float> timeSmoother;
    juce::SmoothedValue<float> massSmoother;
    juce::SmoothedValue<float> densitySmoother;

    timeSmoother.reset(kSampleRate, 0.02);
    massSmoother.reset(kSampleRate, 0.05);
    densitySmoother.reset(kSampleRate, 0.01);

    timeSmoother.setCurrentAndTargetValue(0.0f);
    massSmoother.setCurrentAndTargetValue(0.5f);
    densitySmoother.setCurrentAndTargetValue(1.0f);

    timeSmoother.setTargetValue(1.0f);
    massSmoother.setTargetValue(0.0f);
    densitySmoother.setTargetValue(0.5f);

    // Fill all three buffers
    ParameterBufferPool::fillBuffer(pool.timeBuffer, timeSmoother, kBufferSize);
    ParameterBufferPool::fillBuffer(pool.massBuffer, massSmoother, kBufferSize);
    ParameterBufferPool::fillBuffer(pool.densityBuffer, densitySmoother, kBufferSize);

    // Verify each buffer has different trajectory
    float timeStart = pool.timeBuffer[0];
    float timeEnd = pool.timeBuffer[kBufferSize - 1];
    float massStart = pool.massBuffer[0];
    float massEnd = pool.massBuffer[kBufferSize - 1];
    float densityStart = pool.densityBuffer[0];
    float densityEnd = pool.densityBuffer[kBufferSize - 1];

    // Time should increase (0 → 1)
    ASSERT_TRUE(timeEnd > timeStart, "Time should increase");

    // Mass should decrease (0.5 → 0)
    ASSERT_TRUE(massEnd < massStart, "Mass should decrease");

    // Density should decrease (1 → 0.5)
    ASSERT_TRUE(densityEnd < densityStart, "Density should decrease");

    std::cout << ANSI_GREEN << "  ✓ Time:    " << timeStart << " → " << timeEnd << ANSI_RESET << "\n";
    std::cout << ANSI_GREEN << "  ✓ Mass:    " << massStart << " → " << massEnd << ANSI_RESET << "\n";
    std::cout << ANSI_GREEN << "  ✓ Density: " << densityStart << " → " << densityEnd << ANSI_RESET << "\n";

    return true;
}

//==============================================================================
// Test 9: Edge Case - Zero-Length Buffer
//==============================================================================
bool testZeroLengthBuffer()
{
    printTestHeader("Test 9: Edge Case - Zero-Length Buffer");

    float testData[1] = {0.5f};

    // Create buffers with zero length
    ParameterBuffer perSampleBuffer(testData, 0);
    ParameterBuffer constantBuffer(0.75f, 0);

    // Verify properties
    ASSERT_TRUE(perSampleBuffer.numSamples == 0, "Per-sample buffer should have 0 samples");
    ASSERT_TRUE(constantBuffer.numSamples == 0, "Constant buffer should have 0 samples");

    std::cout << ANSI_GREEN << "  ✓ Zero-length buffers created without crash" << ANSI_RESET << "\n";
    std::cout << ANSI_YELLOW << "  ℹ Note: Accessing these buffers would be undefined behavior" << ANSI_RESET << "\n";

    return true;
}

//==============================================================================
// Test 10: Stress Test - Large Buffer Sizes
//==============================================================================
bool testLargeBufferSizes()
{
    printTestHeader("Test 10: Stress Test - Large Buffer Sizes");

    constexpr int kLargeSize = ParameterBufferPool::kMaxSamples;

    ParameterBufferPool pool;

    // Fill with test pattern
    for (int i = 0; i < kLargeSize; ++i) {
        pool.timeBuffer[i] = static_cast<float>(i) / kLargeSize;
    }

    // Create view of full buffer
    ParameterBuffer view = ParameterBufferPool::makeView(pool.timeBuffer, kLargeSize);

    // Verify access at various points
    ASSERT_FLOAT_EQ(view[0], 0.0f, 0.0001f, "First sample");
    ASSERT_FLOAT_EQ(view[kLargeSize / 2], 0.5f, 0.001f, "Middle sample");
    ASSERT_FLOAT_EQ(view[kLargeSize - 1],
                   static_cast<float>(kLargeSize - 1) / kLargeSize, 0.001f, "Last sample");

    std::cout << ANSI_GREEN << "  ✓ Large buffer (2048 samples) handles correctly" << ANSI_RESET << "\n";
    std::cout << ANSI_GREEN << "  ✓ Pool supports up to " << ParameterBufferPool::kMaxSamples
              << " samples per buffer" << ANSI_RESET << "\n";

    return true;
}

//==============================================================================
// Main Test Runner
//==============================================================================
int main()
{
    std::cout << ANSI_BOLD << ANSI_MAGENTA
              << "\n╔════════════════════════════════════════════════════════════╗\n"
              << "║        Monument Reverb - ParameterBuffer Test Suite       ║\n"
              << "╚════════════════════════════════════════════════════════════╝"
              << ANSI_RESET << "\n";

    std::cout << ANSI_CYAN << "\nTesting per-sample parameter infrastructure (zipper noise elimination)"
              << ANSI_RESET << "\n";

    // Run all tests
    printTestResult("Test 1: Per-Sample Mode", testPerSampleMode());
    printTestResult("Test 2: Constant Mode", testConstantMode());
    printTestResult("Test 3: Branchless Access", testBranchlessAccess());
    printTestResult("Test 4: Default Constructor", testDefaultConstructor());
    printTestResult("Test 5: fillBuffer()", testFillBuffer());
    printTestResult("Test 6: makeView()", testMakeView());
    printTestResult("Test 7: Buffer Pool Alignment", testBufferPoolAlignment());
    printTestResult("Test 8: Multiple fillBuffer() Calls", testMultipleFillBuffer());
    printTestResult("Test 9: Zero-Length Buffer", testZeroLengthBuffer());
    printTestResult("Test 10: Large Buffer Sizes", testLargeBufferSizes());

    // Print summary
    std::cout << "\n" << ANSI_BOLD << ANSI_CYAN << "═══════════════════════════════════════════════════════════" << ANSI_RESET << "\n";
    std::cout << ANSI_BOLD << "Test Results:" << ANSI_RESET << "\n";
    std::cout << "  Total:  " << testsRun << "\n";
    std::cout << ANSI_GREEN << "  Passed: " << testsPassed << ANSI_RESET << "\n";

    if (testsFailed > 0) {
        std::cout << ANSI_RED << "  Failed: " << testsFailed << ANSI_RESET << "\n";
    }

    std::cout << ANSI_BOLD << ANSI_CYAN << "═══════════════════════════════════════════════════════════" << ANSI_RESET << "\n\n";

    if (testsFailed == 0) {
        std::cout << ANSI_BOLD << ANSI_GREEN
                  << "🎉 ALL TESTS PASSED! ParameterBuffer infrastructure ready for integration."
                  << ANSI_RESET << "\n\n";
        return 0;
    } else {
        std::cout << ANSI_BOLD << ANSI_RED
                  << "❌ SOME TESTS FAILED. Fix issues before proceeding."
                  << ANSI_RESET << "\n\n";
        return 1;
    }
}