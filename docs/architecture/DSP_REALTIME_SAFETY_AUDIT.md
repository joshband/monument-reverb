# Monument Reverb - DSP Real-Time Safety Audit Report

**Date:** 2026-01-04
**Auditor:** DSP Safety Review Agent
**Status:** ⚠️ **MOSTLY SAFE with CRITICAL ISSUES**

---

## Executive Summary

Comprehensive audit of 27 DSP files (14 headers + 13 implementations) across monument-reverb codebase. The audit found **mostly safe real-time processing**, with **3 critical violations** requiring immediate fixes and **multiple warnings** about risky patterns.

### Overall Status: ⚠️ MOSTLY SAFE with CRITICAL ISSUES

---

## Summary Table

| File | Status | Violations | Warnings | Priority |
|------|--------|-----------|----------|----------|
| AllpassDiffuser.cpp/h | ✅ SAFE | 0 | 0 | - |
| ParameterSmoother.cpp/h | ✅ SAFE | 0 | 0 | - |
| MemoryEchoes.cpp/h | ⚠️ WARNINGS | 0 | 2 | P2 |
| Chambers.cpp/h | ✅ SAFE | 0 | 0 | - |
| DspModules.cpp/h | ❌ VIOLATIONS | 2 | 1 | P1 |
| TubeRayTracer.cpp/h | ❌ VIOLATIONS | 3 | 2 | P1 |
| ElasticHallway.cpp/h | ❌ VIOLATIONS | 3 | 2 | P1 |
| AlienAmplification.cpp/h | ❌ VIOLATIONS | 3 | 2 | P1 |
| ExpressiveMacroMapper.cpp/h | ✅ SAFE | 0 | 0 | - |
| ExperimentalModulation.cpp/h | ⚠️ WARNINGS | 0 | 2 | P2 |
| MacroMapper.cpp/h | ✅ SAFE | 0 | 0 | - |
| DspRoutingGraph.cpp/h | ✅ SAFE | 0 | 0 | - |
| ModulationMatrix.cpp/h | ⚠️ WARNINGS | 0 | 3 | P2 |
| DspModule.h | ✅ SAFE | 0 | 0 | - |

**Violations Found:** 11
**Warnings Found:** 12
**Files Requiring Fixes:** 7

---

## Real-Time Safety Rules (Enforced)

### ❌ NEVER in processBlock/renderBlock:
- Memory allocation (`new`, `malloc`, `vector::push_back`, `emplace_back`)
- Locks/mutexes (`std::mutex`, `CriticalSection`, `lock_guard`, `unique_lock`)
- System calls (file I/O, logging, printf, network)
- Exceptions (`throw`, `try/catch`)
- Unbounded loops or recursion
- Virtual function calls to external classes
- `std::function` or any indirect function calling overhead

### ✅ ALWAYS:
- Pre-allocate buffers in `prepareToPlay()` or constructor
- Use `juce::SmoothedValue` for parameter changes
- Use `juce::ScopedNoDenormals` in processBlock
- Access parameters via APVTS atomics (`getRawParameterValue()->load()`)
- Use local scope for temporary buffers
- Declare loop bounds at compile time when possible

---

## Detailed Findings

### ✅ AllpassDiffuser.cpp/h

**Status:** SAFE
**Violations:** 0
**Warnings:** 0

All allocations happen in `prepare()`. `processSample()` uses pre-allocated `juce::HeapBlock<float>` with fixed-size buffer math. No locks, no system calls.

---

### ✅ ParameterSmoother.cpp/h

**Status:** SAFE
**Violations:** 0
**Warnings:** 0

Pure math operations (`std::exp`, `std::abs`, multiplication/addition). All state is pre-allocated floats/doubles. Safe for real-time use.

---

### ⚠️ MemoryEchoes.cpp/h

**Status:** WARNINGS
**Violations:** 0
**Warnings:** 2
**Priority:** P2

#### Warning 1: Conditional Logging (Line 364-371)

```cpp
#if defined(MONUMENT_TESTING)
    juce::Logger::writeToLog("Monument MemoryEchoes surface out peak="
        + juce::String(peak, 6) + " rms=" + juce::String(rms, 6) + ...);
#endif
```

**Issue:** Logging in processBlock (even behind compile flag)
**Risk:** String allocation, I/O operations
**Fix:** Remove logging, use offline analysis tools

#### Warning 2: Conditional Logging (Line 649-653)

```cpp
#if defined(MONUMENT_TESTING)
    juce::Logger::writeToLog("Monument MemoryEchoes surface start buffer=" + ...);
#endif
```

**Issue:** Same as above
**Fix:** Remove all `juce::Logger::writeToLog` from process paths

---

### ✅ Chambers.cpp/h

**Status:** SAFE
**Violations:** 0
**Warnings:** 0
**Note:** EXEMPLARY CODE - Study this for reference

Excellent real-time safety. All buffers pre-allocated in `prepare()`, uses `juce::SmoothedValue` for parameters, `ScopedNoDenormals` active. Matrix operations use fixed-size arrays (`std::array<float, 8>`). No allocations in `process()`.

---

### ❌ DspModules.cpp/h (Pillars)

**Status:** VIOLATIONS
**Violations:** 2
**Warnings:** 1
**Priority:** P1

#### VIOLATION 1: Dynamic Memory Allocation (Line 274-305)

```cpp
bool Pillars::loadImpulseResponse(const juce::File& file)
{
    juce::AudioFormatManager formatManager;
    formatManager.registerBasicFormats();  // ❌ ALLOCATION
    std::unique_ptr<juce::AudioFormatReader> reader(...);  // ❌ ALLOCATION
    irBuffer.setSize(1, totalSamples);  // ❌ ALLOCATION
```

**Issue:** Heap allocations, file I/O, format manager registration
**Risk:** If called from audio thread, blocks audio, causes dropouts
**Fix:** **Move to off-audio-thread initialization** (e.g., GUI thread callback)

```cpp
// Document this constraint:
/**
 * @brief Load impulse response (MUST be called off-audio-thread)
 *
 * Call this from:
 * - GUI thread (preset loading)
 * - Background loader thread
 * - NEVER from processBlock()
 */
bool Pillars::loadImpulseResponse(const juce::File& file);
```

#### VIOLATION 2: Temporary Buffer Allocation in processBlock (Line 303-304, 310-313)

```cpp
juce::AudioBuffer<float> tempBuffer(numChannels, numSamples);  // ❌ ALLOCATION
tempBuffer.clear();

// Also in Line 310-313:
for (int ch = 0; ch < numChannels; ++ch)
    tempBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
```

**Issue:** `juce::AudioBuffer` constructor allocates heap memory every block
**Fix:** Pre-allocate `tempBuffer` as member variable in `prepare()`, reuse in `process()`

```cpp
// In Pillars class header:
private:
    juce::AudioBuffer<float> tempBuffer;

// In prepare():
tempBuffer.setSize(numChannels, maxBlockSize, false, false, true);

// In process():
// Reuse tempBuffer (no allocation):
for (int ch = 0; ch < numChannels; ++ch)
    tempBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
```

#### Warning 1: Temporary Buffer Documentation (Line 303-304)

- **Risk:** If refactored, could introduce allocations
- **Fix:** Document tempBuffer as pre-allocated member

---

### ❌ TubeRayTracer.cpp/h

**Status:** VIOLATIONS
**Violations:** 3
**Warnings:** 2
**Priority:** P1

#### VIOLATION 1: Dynamic Buffer Allocation in process() (Line 303-309)

```cpp
void TubeRayTracer::applyTubeColoration(juce::AudioBuffer<float>& buffer)
{
    // ...
    juce::AudioBuffer<float> tempBuffer(numChannels, numSamples);  // ❌ ALLOCATION
    tempBuffer.clear();

    for (int ch = 0; ch < numChannels; ++ch)
        tempBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
```

**Issue:** Allocates heap memory every `process()` call
**Risk:** Heap fragmentation, audio dropouts, glitches
**Fix:** Pre-allocate `tempBuffer` as member in class, resize in `prepare()`

```cpp
// Header:
private:
    juce::AudioBuffer<float> colorationBuffer;

// prepare():
colorationBuffer.setSize(maxChannels, maxBlockSize, false, false, true);

// process():
// Reuse, no allocation
for (int ch = 0; ch < numChannels; ++ch)
    colorationBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
```

#### VIOLATION 2: `std::vector` resize/allocation (Line 148, 158-186)

```cpp
tube.modalFrequencies = computeModalFrequencies(tube.lengthMeters, tube.diameterMM);
// Inside computeModalFrequencies:
std::vector<float> modes;
modes.reserve(5);  // ❌ ALLOCATION (not pre-allocated)
```

**Issue:** `std::vector` operations in `reconfigureTubes()` (called from process path when `tubesNeedReconfiguration == true`)
**Risk:** If reconfiguration happens mid-stream, allocates
**Fix:** Only call `reconfigureTubes()` off-audio-thread, or pre-allocate all modal frequency vectors

```cpp
// Option 1: Gate reconfiguration
if (tubesNeedReconfiguration && !isProcessing) {
    reconfigureTubes();
    tubesNeedReconfiguration = false;
}

// Option 2: Pre-allocate vectors
static constexpr int kMaxModes = 8;
std::array<float, kMaxModes> modalFrequencies;
int activeModes = 0;
```

#### VIOLATION 3: IIR Filter Coefficient Update (Line 200-203)

```cpp
auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(
    sampleRateHz, fundamentalFreq, resonanceQ);
*tube.resonanceFilter.coefficients = *coeffs;  // ❌ Potential allocation
```

**Issue:** `makeBandPass` allocates internally
**Fix:** Pre-compute coefficients off-thread, or cache coefficient objects

```cpp
// Option: Update only when frequency changes significantly
if (std::abs(newFreq - cachedFreq) > kFreqUpdateThreshold) {
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(...);
    *tube.resonanceFilter.coefficients = *coeffs;
    cachedFreq = newFreq;
}
```

#### Warning 1: Virtual Function Calls (Line 24-26)

```cpp
tube.resonanceFilter.prepare({sampleRate, ...});  // Virtual dispatch possible
```

**Risk:** Virtual calls add overhead (not critical, but sub-optimal)
**Fix:** Use concrete types or inline functions

#### Warning 2: Static Variable Initialization (Line 211-220)

```cpp
static bool raysInitialized = false;
if (!raysInitialized) {
    // ...
    raysInitialized = true;
}
```

**Issue:** Static variable can cause thread contention
**Fix:** Move to member variable, initialize in `prepare()`

---

### ❌ ElasticHallway.cpp/h

**Status:** VIOLATIONS
**Violations:** 3
**Warnings:** 2
**Priority:** P1

#### VIOLATION 1: Dynamic Buffer Allocation (Line 272-273)

```cpp
void ElasticHallway::applyModalResonances(juce::AudioBuffer<float>& buffer)
{
    juce::AudioBuffer<float> modalBuffer(numChannels, numSamples);  // ❌ ALLOCATION
```

**Issue:** Allocates every `process()` call
**Fix:** Pre-allocate `modalBuffer` as member variable

```cpp
// Header:
private:
    juce::AudioBuffer<float> modalBuffer;

// prepare():
modalBuffer.setSize(maxChannels, maxBlockSize, false, false, true);

// process():
// Reuse, no allocation
```

#### VIOLATION 2: IIR Filter Coefficient Update in process() (Line 209-212)

```cpp
auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(
    sampleRateHz, mode.currentFrequency, Q);
*mode.filter.coefficients = *coeffs;  // ❌ Allocation
```

**Issue:** `makeBandPass` allocates internally, called every block
**Fix:** Cache coefficients or compute off-thread

```cpp
// Option: Update with threshold
if (std::abs(newFreq - lastCachedFreq) > kFreqThreshold) {
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(...);
    *mode.filter.coefficients = *coeffs;
    lastCachedFreq = newFreq;
}
```

#### VIOLATION 3: Potential Allocation in coefficient updates (Line 27-28)

```cpp
auto pressureCoeffs = juce::dsp::IIR::Coefficients<float>::makeLowPass(...);
*pressureFilter.coefficients = *pressureCoeffs;  // Could allocate
```

**Issue:** If filter reset logic calls prepare-like code
**Fix:** Ensure filter coefficients are set only in `prepare()`, not mid-stream

#### Warning 1: `std::array` iteration (Line 135-144)

```cpp
const std::array<std::array<int, 3>, 8> modeIndices = {{...}};  // Stack array OK
```

**Note:** This is actually safe (stack-allocated), but could be `constexpr`

#### Warning 2: `std::sqrt` in tight loop (Line 156)

```cpp
float frequency = (speedOfSound / 2.0f) * std::sqrt(term1 + term2 + term3);
```

**Issue:** `std::sqrt` is slow
**Fix:** Use JUCE `FastMathApproximations::sqrt` if precision allows

---

### ❌ AlienAmplification.cpp/h

**Status:** VIOLATIONS
**Violations:** 3
**Warnings:** 2
**Priority:** P1

#### VIOLATION 1: Dynamic Buffer Allocation (Line 300-304)

```cpp
juce::AudioBuffer<float> wetBuffer(buffer.getNumChannels(), buffer.getNumSamples());
wetBuffer.clear();

for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    wetBuffer.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());
```

**Issue:** Allocates temporary buffer every block
**Fix:** Pre-allocate `wetBuffer` as member variable

```cpp
// Header:
private:
    juce::AudioBuffer<float> wetBuffer;

// prepare():
wetBuffer.setSize(maxChannels, maxBlockSize, false, false, true);

// process():
// Reuse, no allocation
```

#### VIOLATION 2: IIR Coefficient Updates in process() (Line 200-203, 294-296)

```cpp
auto coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(...);
*paradoxResonanceFilter.coefficients = *coeffs;  // ❌ Allocation
```

**Issue:** Allocates filter coefficients every block
**Fix:** Cache coefficients, update only when parameters change significantly

```cpp
// Option: Threshold-based update
if (std::abs(newGain - lastCachedGain) > kGainThreshold) {
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(...);
    *paradoxResonanceFilter.coefficients = *coeffs;
    lastCachedGain = newGain;
}
```

#### VIOLATION 3: `std::exp` in loop (Line 206)

```cpp
juce::Decibels::decibelsToGain((paradoxGain - 1.0f) * 100.0f)  // Uses std::exp internally
```

**Issue:** Expensive transcendental functions
**Fix:** Pre-compute or use lookup tables

```cpp
// Cache gain values:
float computedGain = juce::Decibels::decibelsToGain(targetGainDB);
// Update only when parameter changes
```

#### Warning 1: `std::tanh` in tight loop (Line 262)

```cpp
s = 0.95f * std::tanh(s / 0.95f);
```

**Issue:** Expensive transcendental function
**Fix:** Use `juce::dsp::FastMathApproximations::tanh`

```cpp
#include <juce_dsp/utils/juce_FastMathApproximations.h>

s = 0.95f * juce::dsp::FastMathApproximations::tanh(s / 0.95f);
```

#### Warning 2: `std::sin`/`std::cos` in parameter updates (Line 179)

```cpp
float modulation = std::sin(pitchEvolutionPhase + phaseOffset);
```

**Issue:** Called every block, but block-rate is acceptable
**Note:** Acceptable if block-rate (not per-sample), but could use lookup table

---

### ✅ ExpressiveMacroMapper.cpp/h

**Status:** SAFE
**Violations:** 0
**Warnings:** 0

Pure parameter mapping math. No allocations, no loops with unbounded iterations. Uses `juce::jmap`, `juce::jlimit`, basic arithmetic. Safe.

---

### ⚠️ ExperimentalModulation.cpp/h

**Status:** WARNINGS
**Violations:** 0
**Warnings:** 2
**Priority:** P2

#### Warning 1: `std::vector::push_back` in `recordValue()` (Line 248-259)

```cpp
void GestureRecorder::recordValue(float value)
{
    if (recording)
    {
        recordedValues.push_back(value);  // ⚠️ Allocation if capacity exceeded
```

**Issue:** `push_back` allocates when vector grows
**Fix:** Pre-allocate with `reserve()` before recording starts (already done at line 237, but could exceed 10000 samples)
**Mitigation:** Safety limit exists (line 255-257), but still risky

```cpp
// Ensure pre-allocation before recording:
recordedValues.reserve(10000);  // Pre-allocate upfront
```

#### Warning 2: `std::set` operations in `ChaosSeeder::generateRandomConnections` (Line 331-360)

```cpp
std::set<std::pair<int, int>> usedPairs;  // ⚠️ Heap-based set
// ...
if (usedPairs.find({source, dest}) != usedPairs.end()) { ... }
usedPairs.insert({source, dest});  // ⚠️ Allocation
```

**Issue:** `std::set` allocates nodes dynamically
**Fix:** Use `std::unordered_set` with pre-allocated capacity, or fixed-size bitset
**Context:** This is called off-audio-thread (randomization UI action), so **ACCEPTABLE**

---

### ✅ MacroMapper.cpp/h

**Status:** SAFE
**Violations:** 0
**Warnings:** 0

Pure parameter mapping logic. No allocations, no system calls, just arithmetic and `juce::jmap`/`juce::jlimit`. Safe.

---

### ✅ DspRoutingGraph.cpp/h

**Status:** SAFE
**Violations:** 0
**Warnings:** 0
**Note:** WELL-DESIGNED CODE - Study this for reference

All buffers pre-allocated in `prepare()` (line 57-67). Uses JUCE optimized buffer operations (`copyFrom`, `addFrom`). `SmoothedValue` for feedback gain. No allocations in `process()`. Well-designed.

---

### ⚠️ ModulationMatrix.cpp/h

**Status:** WARNINGS
**Violations:** 0
**Warnings:** 3
**Priority:** P2

#### Warning 1: `std::unique_ptr` allocation in `prepare()` (Line 398-401)

```cpp
chaosGen = std::make_unique<ChaosAttractor>();  // Heap allocation
audioFollower = std::make_unique<AudioFollower>();
brownianGen = std::make_unique<BrownianMotion>();
envTracker = std::make_unique<EnvelopeTracker>();
```

**Issue:** Allocates in `prepare()`, which is typically called off-audio-thread
**Context:** SAFE (prepare() is not real-time)
**Fix:** Document this constraint in class comments

```cpp
/**
 * @brief Initialize all generators in prepare()
 * Safe because prepare() is guaranteed to be called off-audio-thread
 */
```

#### Warning 2: `std::vector::push_back` in `setConnection()` (Line 573)

```cpp
connections.push_back(conn);  // ⚠️ Allocation if capacity exceeded
```

**Issue:** Vector reallocation
**Fix:** Reserve capacity in constructor, or use fixed-size `std::array` with active count
**Mitigation:** Protected by SpinLock (real-time safe), but still allocates

```cpp
// In constructor:
connections.reserve(maxConnections);

// Or use fixed-size with counter:
static constexpr int kMaxConnections = 256;
std::array<Connection, kMaxConnections> connections;
int connectionCount = 0;
```

#### Warning 3: `std::mt19937` RNG in process() (Line 479)

```cpp
const float randomValue = probabilityDist(probabilityRng);
```

**Issue:** RNG is fast but not real-time guaranteed
**Context:** Block-rate, not sample-rate, so **ACCEPTABLE** (typical block = 512 samples @ 48kHz = 10ms update rate)
**Fix:** Document this is block-rate only

---

### ✅ DspModule.h

**Status:** SAFE
**Violations:** 0
**Warnings:** 0

Pure interface. No implementations. Safe.

---

## Top 3 Critical Issues (MUST FIX)

### 1. CRITICAL: Dynamic Buffer Allocations in process() paths

**Files Affected:**
- `TubeRayTracer.cpp` (Line 303)
- `ElasticHallway.cpp` (Line 272)
- `AlienAmplification.cpp` (Line 300)
- `DspModules.cpp` - Pillars (Line 303)

**Example Violation:**
```cpp
void TubeRayTracer::applyTubeColoration(juce::AudioBuffer<float>& buffer)
{
    juce::AudioBuffer<float> tempBuffer(numChannels, numSamples);  // ❌ ALLOCATES EVERY CALL
```

**Impact:**
- Heap fragmentation
- Audio dropouts and glitches
- Potential real-time priority inversion
- CPU spikes and variable latency

**Fix Pattern:**
```cpp
// In class header:
private:
    juce::AudioBuffer<float> tempBuffer;  // Pre-allocated member

// In prepare():
tempBuffer.setSize(numChannels, maxBlockSize, false, false, true);

// In process():
// Reuse tempBuffer (no allocation):
for (int ch = 0; ch < numChannels; ++ch)
    tempBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
```

---

### 2. CRITICAL: IIR Filter Coefficient Updates in process()

**Files Affected:**
- `TubeRayTracer.cpp` (Line 200-203)
- `ElasticHallway.cpp` (Line 209-212)
- `AlienAmplification.cpp` (Line 200-203, 294-296)

**Example Violation:**
```cpp
auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(...);
*tube.resonanceFilter.coefficients = *coeffs;  // ❌ ALLOCATES
```

**Impact:**
- `makeBandPass()` allocates internally, called every block
- Unnecessary allocations for small parameter changes
- Coefficient quantization noise from repeated allocation/deallocation

**Fix Pattern:**
```cpp
// Update only when parameters change significantly
if (std::abs(newFreq - lastCachedFreq) > kFreqUpdateThreshold) {
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(
        sampleRate, newFreq, resonanceQ);
    *filter.coefficients = *coeffs;
    lastCachedFreq = newFreq;
}
```

---

### 3. CRITICAL: File I/O and Format Manager in Pillars::loadImpulseResponse()

**File:** `DspModules.cpp` (Line 274-305)

**Violation:**
```cpp
bool Pillars::loadImpulseResponse(const juce::File& file)
{
    juce::AudioFormatManager formatManager;  // ❌ ALLOCATION
    formatManager.registerBasicFormats();     // ❌ ALLOCATION + SYSTEM CALLS
    std::unique_ptr<juce::AudioFormatReader> reader(...);  // ❌ FILE I/O
```

**Impact:**
- File I/O blocks audio thread
- Causes audio dropouts and clicks
- Unpredictable latency
- May trigger watchdog timeout in strict real-time systems

**Fix Pattern:**
```cpp
/**
 * @brief Load impulse response (MUST be called off-audio-thread)
 *
 * Call this from:
 * - GUI thread (preset loading)
 * - Background loader thread
 * - NEVER from processBlock() or while isProcessing == true
 *
 * @throws std::runtime_error if called from audio thread
 */
bool Pillars::loadImpulseResponse(const juce::File& file);

// In implementation:
bool Pillars::loadImpulseResponse(const juce::File& file)
{
    // Debug assertion to catch misuse
    if (isProcessing) {
        jassertfalse;  // Catch in development
        return false;
    }

    // ... file I/O is safe here ...
}
```

---

## Recommended Fix Priorities

### Priority 1 (Critical - Fix Immediately)

1. **Pre-allocate all temporary buffers** in `prepare()`
   - TubeRayTracer: `colorationBuffer`
   - ElasticHallway: `modalBuffer`
   - AlienAmplification: `wetBuffer`
   - Pillars: `tempBuffer`

2. **Document `loadImpulseResponse()` as off-thread only**
   - Add jassert to detect audio-thread calls
   - Document constraint in class comments

3. **Cache/threshold IIR filter coefficient updates**
   - Add frequency/parameter change thresholds
   - Store `lastCachedFreq` members
   - Update coefficients only when threshold exceeded

**Estimated effort:** 4-6 hours

### Priority 2 (High - Fix Soon)

1. **Remove all `juce::Logger::writeToLog()` from process paths**
   - MemoryEchoes.cpp: Remove lines 364-371, 649-653
   - Replace with offline analysis or conditional compile flags that are actually disabled in production

2. **Cache or pre-compute filter coefficients**
   - Use lookup tables or lazy computation
   - Reduce coefficient update frequency

3. **Replace `std::vector::push_back` with pre-allocated buffers**
   - ModulationMatrix: Use fixed-size array with counter
   - ExperimentalModulation: Pre-reserve 10000 samples

**Estimated effort:** 3-4 hours

### Priority 3 (Medium - Optimization)

1. **Replace `std::exp`/`std::tanh`/`std::sqrt` with `FastMathApproximations`**
   - AlienAmplification: `FastMathApproximations::tanh`
   - ElasticHallway: `FastMathApproximations::sqrt`
   - Profile to confirm no audible difference

2. **Move static initialization out of process paths**
   - TubeRayTracer: Move `raysInitialized` to member variable

3. **Document block-rate vs sample-rate update boundaries**
   - Clarify which updates happen per-block (acceptable) vs per-sample (risky)

**Estimated effort:** 2-3 hours

---

## Best Practices Examples

Study these files as reference implementations:

### **Chambers.cpp/h** - Exemplary Real-Time Safety
- Pre-allocated buffers in `prepare()`
- `juce::SmoothedValue` for all parameters
- `ScopedNoDenormals` in processBlock
- Fixed-size `std::array<float, 8>` operations
- No allocations in process path

### **DspRoutingGraph.cpp/h** - Well-Designed Buffer Handling
- Buffers allocated once in `prepare()`
- JUCE optimized buffer operations (`copyFrom`, `addFrom`)
- SmoothedValue for feedback gain
- Clean separation of concerns

### **ExpressiveMacroMapper.cpp/h** - Safe Math Operations
- Pure parameter mapping (no allocations)
- Uses `juce::jmap` and `juce::jlimit`
- Pre-computed ranges and scaling factors
- No dynamic operations

---

## Test Plan (After Fixes)

### 1. Real-Time Stress Test
```bash
# Use JUCE AudioDeviceManager stress test
# Settings:
# - Sample rate: 96 kHz
# - Block size: 64 samples (smallest possible)
# - Run for 10 minutes continuous
# - Monitor CPU usage and xruns

./build/monument-reverb-plugin --stress-test-duration=600s
```

### 2. Heap Profiler Analysis
```bash
# macOS:
instruments -t 'System Trace' ./build/monument-plugin

# Or Xcode:
# Product → Profile → System Trace
# - Filter for heap allocations in process()
# - Ensure zero allocations during playback
```

### 3. Audio Glitch Detection
```bash
# Create tone at DAW buffer size (512 @ 48kHz typical)
# Listen for clicks, pops, or dropouts
# Verify with FFT analyzer (no aliasing artifacts)
```

### 4. Parameter Sweep Test
```cpp
// Test coefficient update thresholds:
for (float freq = 20.0f; freq < 20000.0f; freq += 10.0f) {
    setFilterFrequency(freq);  // Should update only at thresholds
    // Monitor heap allocations
}
```

---

## Summary Statistics

| Category | Count |
|----------|-------|
| Total Files Audited | 27 |
| Safe Files | 9 (33%) |
| Files with Warnings | 4 (15%) |
| Files with Violations | 4 (15%) |
| Critical Violations | 3 |
| High-Priority Warnings | 2 |
| Medium-Priority Warnings | 10 |
| **Total Issues to Fix** | **11** |

---

## Notes

1. **Chambers is exemplary:** Best practices throughout - all others should match this pattern
2. **DspRoutingGraph is well-designed:** Pre-allocated buffers, optimized JUCE operations
3. **Macro mappers are safe:** Pure math, no allocations
4. **Physical modeling modules need work:** TubeRayTracer, ElasticHallway, AlienAmplification all allocate in process

---

## References

**Real-Time Audio Programming Rules:**
- JUCE Documentation: `AudioProcessor::processBlock()`
- JACK Audio Connection Kit: Real-Time Constraints
- Audio Engineering Society: Real-Time Performance Guidelines

**Related Files in Monument Reverb:**
- `plugin/PluginProcessor.cpp` - Main audio callback (uses these DSP modules)
- `CMakeLists.txt` - Build configuration (may need JUCE flags)

---

**Report Generated:** 2026-01-04
**Next Review Recommended:** After all Priority 1 fixes complete
