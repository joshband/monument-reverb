# Monument Reverb - DSP Real-Time Safety Fix Implementation Plan

**Status:** Ready for Implementation
**Estimated Duration:** 9-13 hours across 3 priority levels
**Test Plan:** See Section 5

---

## Table of Contents

1. [Priority 1 - Critical Fixes (4-6 hours)](#priority-1)
2. [Priority 2 - High Priority Fixes (3-4 hours)](#priority-2)
3. [Priority 3 - Optimization (2-3 hours)](#priority-3)
4. [Implementation Checklist](#checklist)
5. [Test & Verification Plan](#verification)
6. [Code Review Checklist](#review)

---

## Priority 1 - Critical Fixes

**Impact:** Eliminates heap allocations in audio callbacks
**Risk if not fixed:** Audio dropouts, glitches, real-time deadline misses
**Files:** TubeRayTracer, ElasticHallway, AlienAmplification, Pillars, all IIR updates

### Fix 1.1: TubeRayTracer - Pre-allocate colorationBuffer

**File:** `dsp/TubeRayTracer.h`

**Current Code (WRONG):**
```cpp
class TubeRayTracer {
private:
    // ... other members
};
```

**New Code:**
```cpp
class TubeRayTracer {
private:
    // ... other members
    juce::AudioBuffer<float> colorationBuffer;  // ADD THIS
};
```

**File:** `dsp/TubeRayTracer.cpp`

**In `prepare()` method, add after line 95:**
```cpp
void TubeRayTracer::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    // ... existing code ...

    // ADD: Pre-allocate coloration buffer
    colorationBuffer.setSize(numChannels, maxBlockSize, false, false, true);
}
```

**In `applyTubeColoration()` method, replace lines 303-309:**

**Current (WRONG):**
```cpp
void TubeRayTracer::applyTubeColoration(juce::AudioBuffer<float>& buffer)
{
    // ...
    juce::AudioBuffer<float> tempBuffer(numChannels, numSamples);  // ❌ ALLOCATION
    tempBuffer.clear();

    for (int ch = 0; ch < numChannels; ++ch)
        tempBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
```

**New (CORRECT):**
```cpp
void TubeRayTracer::applyTubeColoration(juce::AudioBuffer<float>& buffer)
{
    // ...
    // ✅ Reuse pre-allocated colorationBuffer (no allocation)
    for (int ch = 0; ch < numChannels; ++ch)
        colorationBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
```

**Then update all subsequent references from `tempBuffer` to `colorationBuffer`**

**Verification:**
- [ ] Code compiles without errors
- [ ] No allocations in `applyTubeColoration()` call path
- [ ] Heap profiler shows zero allocations during audio playback

---

### Fix 1.2: TubeRayTracer - Cache IIR Filter Coefficients

**File:** `dsp/TubeRayTracer.h`

**Current Code (WRONG):**
```cpp
class TubeRayTracer {
private:
    std::vector<Tube> tubes;
    // ... other members
};
```

**New Code:**
```cpp
class TubeRayTracer {
private:
    std::vector<Tube> tubes;
    float lastCachedFundamentalFreq = -1.0f;  // ADD THIS
    // ... other members
};
```

**File:** `dsp/TubeRayTracer.cpp`

**In line 200-203, replace:**

**Current (WRONG):**
```cpp
auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(
    sampleRateHz, fundamentalFreq, resonanceQ);
*tube.resonanceFilter.coefficients = *coeffs;
```

**New (CORRECT):**
```cpp
// Only update coefficients when frequency changes significantly
static constexpr float kFreqUpdateThreshold = 1.0f;  // Hz
if (std::abs(fundamentalFreq - lastCachedFundamentalFreq) > kFreqUpdateThreshold)
{
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(
        sampleRateHz, fundamentalFreq, resonanceQ);
    *tube.resonanceFilter.coefficients = *coeffs;
    lastCachedFundamentalFreq = fundamentalFreq;
}
```

**Verification:**
- [ ] Coefficient updates only on significant frequency changes
- [ ] No redundant allocations per sample
- [ ] Audible behavior unchanged (frequency response identical)

---

### Fix 1.3: ElasticHallway - Pre-allocate modalBuffer

**File:** `dsp/ElasticHallway.h`

**Current Code (WRONG):**
```cpp
class ElasticHallway {
private:
    std::array<ModalFilter, 8> modalFilters;
    // ... other members
};
```

**New Code:**
```cpp
class ElasticHallway {
private:
    std::array<ModalFilter, 8> modalFilters;
    juce::AudioBuffer<float> modalBuffer;  // ADD THIS
    // ... other members
};
```

**File:** `dsp/ElasticHallway.cpp`

**In `prepare()` method, add:**
```cpp
void ElasticHallway::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    // ... existing code ...

    // ADD: Pre-allocate modal buffer
    modalBuffer.setSize(numChannels, maxBlockSize, false, false, true);
}
```

**In `applyModalResonances()` method, replace lines 272-273:**

**Current (WRONG):**
```cpp
void ElasticHallway::applyModalResonances(juce::AudioBuffer<float>& buffer)
{
    juce::AudioBuffer<float> modalBuffer(numChannels, numSamples);  // ❌ ALLOCATION
```

**New (CORRECT):**
```cpp
void ElasticHallway::applyModalResonances(juce::AudioBuffer<float>& buffer)
{
    // ✅ Reuse pre-allocated modalBuffer (no allocation)
```

**Then update all `modalBuffer` references (no name changes needed)**

**Verification:**
- [ ] Code compiles
- [ ] No allocations in `applyModalResonances()` path
- [ ] Audio output unchanged

---

### Fix 1.4: ElasticHallway - Cache IIR Filter Coefficients

**File:** `dsp/ElasticHallway.h`

**Current Code (WRONG):**
```cpp
struct ModalFilter {
    juce::dsp::IIR::Filter<float> filter;
    float currentFrequency = 0.0f;
    // ... other members
};
```

**New Code:**
```cpp
struct ModalFilter {
    juce::dsp::IIR::Filter<float> filter;
    float currentFrequency = 0.0f;
    float lastCachedFrequency = -1.0f;  // ADD THIS
    // ... other members
};
```

**File:** `dsp/ElasticHallway.cpp`

**In line 209-212, replace:**

**Current (WRONG):**
```cpp
auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(
    sampleRateHz, mode.currentFrequency, Q);
*mode.filter.coefficients = *coeffs;
```

**New (CORRECT):**
```cpp
static constexpr float kFreqUpdateThreshold = 0.5f;  // Hz
if (std::abs(mode.currentFrequency - mode.lastCachedFrequency) > kFreqUpdateThreshold)
{
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass(
        sampleRateHz, mode.currentFrequency, Q);
    *mode.filter.coefficients = *coeffs;
    mode.lastCachedFrequency = mode.currentFrequency;
}
```

**Verification:**
- [ ] Code compiles
- [ ] Frequency changes are smooth (below 1Hz difference between updates)
- [ ] Reduced allocation frequency

---

### Fix 1.5: AlienAmplification - Pre-allocate wetBuffer

**File:** `dsp/AlienAmplification.h`

**Current Code (WRONG):**
```cpp
class AlienAmplification {
private:
    juce::dsp::IIR::Filter<float> paradoxResonanceFilter;
    // ... other members
};
```

**New Code:**
```cpp
class AlienAmplification {
private:
    juce::dsp::IIR::Filter<float> paradoxResonanceFilter;
    juce::AudioBuffer<float> wetBuffer;  // ADD THIS
    // ... other members
};
```

**File:** `dsp/AlienAmplification.cpp`

**In `prepare()` method, add:**
```cpp
void AlienAmplification::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    // ... existing code ...

    // ADD: Pre-allocate wet buffer
    wetBuffer.setSize(numChannels, maxBlockSize, false, false, true);
}
```

**In the process method around line 300-304, replace:**

**Current (WRONG):**
```cpp
juce::AudioBuffer<float> wetBuffer(buffer.getNumChannels(), buffer.getNumSamples());
wetBuffer.clear();

for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    wetBuffer.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());
```

**New (CORRECT):**
```cpp
// ✅ Reuse pre-allocated wetBuffer (no allocation)
wetBuffer.clear();  // Clear is still needed

for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    wetBuffer.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());
```

**Verification:**
- [ ] Code compiles
- [ ] No allocations in process path
- [ ] Wet/dry mix works as expected

---

### Fix 1.6: AlienAmplification - Cache IIR Filter Coefficients

**File:** `dsp/AlienAmplification.h`

**Current Code (WRONG):**
```cpp
class AlienAmplification {
private:
    float paradoxGain = 0.0f;
    // ... other members
};
```

**New Code:**
```cpp
class AlienAmplification {
private:
    float paradoxGain = 0.0f;
    float lastCachedParadoxGain = -1.0f;  // ADD THIS
    // ... other members
};
```

**File:** `dsp/AlienAmplification.cpp`

**In line 200-203 and 294-296, replace both instances:**

**Current (WRONG):**
```cpp
auto coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(...);
*paradoxResonanceFilter.coefficients = *coeffs;
```

**New (CORRECT):**
```cpp
static constexpr float kGainUpdateThreshold = 0.5f;  // dB
if (std::abs(paradoxGain - lastCachedParadoxGain) > kGainUpdateThreshold)
{
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(...);
    *paradoxResonanceFilter.coefficients = *coeffs;
    lastCachedParadoxGain = paradoxGain;
}
```

**Verification:**
- [ ] Code compiles
- [ ] Both filter update locations use same threshold
- [ ] Parameter changes are still responsive (0.5dB threshold is imperceptible)

---

### Fix 1.7: Pillars - Document loadImpulseResponse as Off-Thread

**File:** `dsp/DspModules.h`

**Current Code (WRONG):**
```cpp
class Pillars : public DspModule {
public:
    bool loadImpulseResponse(const juce::File& file);
    // ... other methods
};
```

**New Code (ADD DOCUMENTATION):**
```cpp
class Pillars : public DspModule {
public:
    /**
     * @brief Load impulse response for convolutional reverb
     *
     * ⚠️ CRITICAL: MUST be called off-audio-thread only
     *
     * Safe to call from:
     * - GUI thread (preset loading)
     * - Background loader thread
     * - Initialization code
     *
     * NEVER call from:
     * - processBlock() or process() method
     * - Audio callback thread
     * - While isProcessing == true
     *
     * Performs file I/O and heap allocations which are not real-time safe.
     *
     * @param file Path to audio file (WAV, AIFF, etc.)
     * @return true if load successful, false otherwise
     * @throws std::runtime_error if called from audio thread (debug builds)
     */
    bool loadImpulseResponse(const juce::File& file);
    // ... other methods
};
```

**File:** `dsp/DspModules.cpp`

**At the top of `loadImpulseResponse()` method, add debug check:**

```cpp
bool Pillars::loadImpulseResponse(const juce::File& file)
{
    // ✅ Debug check to catch audio-thread misuse
    if (isProcessing) {
        jassertfalse;  // Catches in debug builds
        return false;
    }

    // ... existing file I/O code ...
}
```

**Also add member variable to track processing state:**

**In `dsp/DspModules.h`:**
```cpp
class Pillars : public DspModule {
private:
    bool isProcessing = false;  // ADD THIS
    // ... other members
};
```

**In `process()` method:**
```cpp
void Pillars::process(juce::AudioBuffer<float>& buffer, ...)
{
    ScopedValueSetter<bool> scope(isProcessing, true);  // Set to true during processing

    // ... existing process code ...
}
```

**Verification:**
- [ ] Documentation is clear and visible to developers
- [ ] Debug assertion fires if called from audio thread
- [ ] File loading still works normally from GUI thread

---

### Fix 1.8: Pillars - Pre-allocate tempBuffer

**File:** `dsp/DspModules.h`

**Current Code (WRONG):**
```cpp
class Pillars : public DspModule {
private:
    juce::AudioBuffer<float> irBuffer;
    // ... other members
};
```

**New Code:**
```cpp
class Pillars : public DspModule {
private:
    juce::AudioBuffer<float> irBuffer;
    juce::AudioBuffer<float> tempBuffer;  // ADD THIS
    // ... other members
};
```

**File:** `dsp/DspModules.cpp`

**In `prepare()` method, add:**
```cpp
void Pillars::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    // ... existing code ...

    // ADD: Pre-allocate temp buffer
    tempBuffer.setSize(numChannels, maxBlockSize, false, false, true);
}
```

**In `process()` method around line 303-304, replace:**

**Current (WRONG):**
```cpp
juce::AudioBuffer<float> tempBuffer(numChannels, numSamples);  // ❌ ALLOCATION
tempBuffer.clear();

for (int ch = 0; ch < numChannels; ++ch)
    tempBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
```

**New (CORRECT):**
```cpp
// ✅ Reuse pre-allocated tempBuffer (no allocation)
tempBuffer.clear();

for (int ch = 0; ch < numChannels; ++ch)
    tempBuffer.copyFrom(ch, 0, buffer, ch, 0, numSamples);
```

**Verification:**
- [ ] Code compiles
- [ ] No allocations in process path
- [ ] Impulse response convolution works as expected

---

**Priority 1 Summary:**
- [ ] TubeRayTracer: colorationBuffer pre-allocated, frequency caching added
- [ ] ElasticHallway: modalBuffer pre-allocated, frequency caching added
- [ ] AlienAmplification: wetBuffer pre-allocated, gain caching added
- [ ] Pillars: tempBuffer pre-allocated, loadImpulseResponse documented
- [ ] All IIR coefficient updates use thresholds
- [ ] Total files modified: 4
- [ ] New compile errors: 0
- [ ] New heap allocations in process: 0

---

## Priority 2 - High Priority Fixes

**Impact:** Reduces logging overhead, fixes remaining allocations
**Risk if not fixed:** Potential allocation during edge cases, log spam in testing
**Files:** MemoryEchoes, ExperimentalModulation, ModulationMatrix

### Fix 2.1: MemoryEchoes - Remove Process-Path Logging

**File:** `dsp/MemoryEchoes.cpp`

**Lines 364-371, remove or gate this logging:**

**Current (WRONG):**
```cpp
#if defined(MONUMENT_TESTING)
    juce::Logger::writeToLog("Monument MemoryEchoes surface out peak="
        + juce::String(peak, 6) + " rms=" + juce::String(rms, 6) + ...);
#endif
```

**Option 1: Remove completely**
```cpp
// REMOVED: Process-path logging (not real-time safe even in testing)
```

**Option 2: Move to separate debug callback (if metrics needed)**
```cpp
// Instead of logging in process(), collect metrics:
struct DebugMetrics {
    float peakOut = 0.0f;
    float rmsOut = 0.0f;
    // ... other metrics
};

// Then provide a separate getter (called off-thread):
const DebugMetrics& getDebugMetrics() const { return metrics; }
```

**Lines 649-653, remove similarly:**

**Current (WRONG):**
```cpp
#if defined(MONUMENT_TESTING)
    juce::Logger::writeToLog("Monument MemoryEchoes surface start buffer=" + ...);
#endif
```

**New (CORRECT):**
```cpp
// REMOVED: Process-path logging (not real-time safe)
```

**Verification:**
- [ ] Code compiles without warnings
- [ ] No logging in process() call path
- [ ] If metrics needed, metrics are collected instead of logged

---

### Fix 2.2: ExperimentalModulation - Pre-reserve GestureRecorder

**File:** `dsp/ExperimentalModulation.h`

**Current Code (WRONG):**
```cpp
class GestureRecorder {
private:
    std::vector<float> recordedValues;  // Dynamic allocation
    // ... other members
};
```

**New Code:**
```cpp
class GestureRecorder {
private:
    static constexpr int kMaxRecordedSamples = 10000;
    std::vector<float> recordedValues;  // Now bounded
    // ... other members
};
```

**File:** `dsp/ExperimentalModulation.cpp`

**In constructor or `resetRecording()` method, add:**
```cpp
GestureRecorder::GestureRecorder()
{
    // Pre-allocate vector to avoid growth during recording
    recordedValues.reserve(kMaxRecordedSamples);
}
```

**In `recordValue()` method, add safety check:**

**Current (Line 248-259):**
```cpp
void GestureRecorder::recordValue(float value)
{
    if (recording)
    {
        recordedValues.push_back(value);  // ⚠️ Could allocate
        // ...
        if (recordedValues.size() > 10000) {
            recording = false;
        }
    }
}
```

**New (BETTER):**
```cpp
void GestureRecorder::recordValue(float value)
{
    if (recording)
    {
        // Safety check to prevent allocation
        if (recordedValues.size() >= kMaxRecordedSamples) {
            recording = false;
            return;
        }

        recordedValues.push_back(value);  // ✅ Safe now (capacity guaranteed)
        // ...
    }
}
```

**Verification:**
- [ ] Vector is pre-allocated to max size
- [ ] No allocations during recording
- [ ] Recording stops gracefully at limit

---

### Fix 2.3: ModulationMatrix - Fix vector::push_back

**File:** `dsp/ModulationMatrix.h`

**Current Code (WRONG):**
```cpp
class ModulationMatrix {
private:
    std::vector<Connection> connections;  // Dynamic
    // ... other members
};
```

**Option 1: Fixed-size array with counter (RECOMMENDED)**
```cpp
class ModulationMatrix {
private:
    static constexpr int kMaxConnections = 256;
    std::array<Connection, kMaxConnections> connections;
    int connectionCount = 0;
    // ... other members
};
```

**Option 2: Pre-reserved vector**
```cpp
class ModulationMatrix {
private:
    static constexpr int kMaxConnections = 256;
    std::vector<Connection> connections;
    // ... other members

public:
    ModulationMatrix() {
        connections.reserve(kMaxConnections);
    }
};
```

**File:** `dsp/ModulationMatrix.cpp`

**In `setConnection()` method around line 573:**

**Current (WRONG) - using vector:**
```cpp
void ModulationMatrix::setConnection(const Connection& conn)
{
    connections.push_back(conn);  // ⚠️ Could allocate
}
```

**New - using fixed array:**
```cpp
void ModulationMatrix::setConnection(const Connection& conn)
{
    jassert(connectionCount < kMaxConnections);

    connections[connectionCount++] = conn;  // ✅ No allocation
}
```

**Also update `getConnections()` to use counter:**
```cpp
const Connection* ModulationMatrix::getConnections(int& outCount) const
{
    outCount = connectionCount;
    return connections.data();
}
```

**Verification:**
- [ ] Code compiles
- [ ] No allocations in setConnection()
- [ ] Connection limit is enforced
- [ ] All connection getters updated

---

**Priority 2 Summary:**
- [ ] MemoryEchoes: Logging removed from process paths
- [ ] ExperimentalModulation: GestureRecorder pre-allocated
- [ ] ModulationMatrix: Connections use fixed-size array
- [ ] Total files modified: 3
- [ ] Edge case allocations eliminated

---

## Priority 3 - Optimization

**Impact:** Reduces CPU overhead, improves performance headroom
**Risk if not fixed:** Higher CPU cost, less optimization headroom
**Files:** AlienAmplification, ElasticHallway, TubeRayTracer

### Fix 3.1: AlienAmplification - Use FastMathApproximations

**File:** `dsp/AlienAmplification.cpp`

**At top, add include:**
```cpp
#include <juce_dsp/utils/juce_FastMathApproximations.h>
```

**In line 262, replace:**

**Current (WRONG):**
```cpp
s = 0.95f * std::tanh(s / 0.95f);
```

**New (CORRECT):**
```cpp
s = 0.95f * juce::dsp::FastMathApproximations::tanh(s / 0.95f);
```

**In line 206 and similar gain calculations, consider lookup table:**
```cpp
// Instead of repeated std::exp:
static constexpr int kGainTableSize = 256;
static std::array<float, kGainTableSize> gainTable;

// Pre-compute table once
static void initGainTable() {
    for (int i = 0; i < kGainTableSize; ++i) {
        float gainDB = -120.0f + (i / (kGainTableSize - 1)) * 120.0f;
        gainTable[i] = juce::Decibels::decibelsToGain(gainDB);
    }
}

// Use table with interpolation
float lookupGain(float gainDB) {
    // Quantize to table index
    float normalized = (gainDB + 120.0f) / 240.0f;
    int index = (int)(normalized * (kGainTableSize - 1));
    return gainTable[juce::jlimit(0, kGainTableSize - 1, index)];
}
```

**Verification:**
- [ ] Code compiles
- [ ] CPU usage reduces by ~20-30% (profile before/after)
- [ ] Audio output quality unchanged or improved

---

### Fix 3.2: ElasticHallway - Use FastMathApproximations

**File:** `dsp/ElasticHallway.cpp`

**At top, add include:**
```cpp
#include <juce_dsp/utils/juce_FastMathApproximations.h>
```

**In line 156, replace:**

**Current (WRONG):**
```cpp
float frequency = (speedOfSound / 2.0f) * std::sqrt(term1 + term2 + term3);
```

**New (CORRECT):**
```cpp
float frequency = (speedOfSound / 2.0f) * juce::dsp::FastMathApproximations::sqrt(term1 + term2 + term3);
```

**Verification:**
- [ ] Code compiles
- [ ] Frequency calculation unchanged
- [ ] CPU usage slightly reduced

---

### Fix 3.3: TubeRayTracer - Remove Static Initialization

**File:** `dsp/TubeRayTracer.cpp`

**Current Code (WRONG) - Line 211-220:**
```cpp
static bool raysInitialized = false;
if (!raysInitialized) {
    // ... initialization code ...
    raysInitialized = true;
}
```

**New Code:**
```cpp
// REMOVED: Static initialization (move to prepare())
```

**File:** `dsp/TubeRayTracer.h`

**Add member variable:**
```cpp
class TubeRayTracer {
private:
    bool raysInitialized = false;  // ADD THIS
    // ... other members
};
```

**In `prepare()` method, add initialization:**
```cpp
void TubeRayTracer::prepare(double sampleRate, int maxBlockSize, int numChannels)
{
    if (!raysInitialized) {
        // ... initialization code from static block ...
        raysInitialized = true;
    }

    // ... rest of prepare() ...
}
```

**Verification:**
- [ ] Code compiles
- [ ] Initialization happens once in prepare()
- [ ] No static variables in process path

---

**Priority 3 Summary:**
- [ ] AlienAmplification: FastMathApproximations for tanh
- [ ] ElasticHallway: FastMathApproximations for sqrt
- [ ] TubeRayTracer: Static initialization moved to prepare()
- [ ] Total files modified: 3
- [ ] Expected CPU reduction: 5-15%

---

## Implementation Checklist

### Pre-Implementation
- [ ] Read full audit report: `docs/architecture/DSP_REALTIME_SAFETY_AUDIT.md`
- [ ] Back up current code: `git stash` or create a branch
- [ ] Create feature branch: `git checkout -b fix/realtime-safety`

### Priority 1 Implementation
- [ ] TubeRayTracer.h - Add colorationBuffer member
- [ ] TubeRayTracer.cpp - Pre-allocate in prepare()
- [ ] TubeRayTracer.cpp - Reuse in applyTubeColoration()
- [ ] TubeRayTracer.cpp - Cache fundamental frequency for IIR updates
- [ ] TubeRayTracer.cpp - Add frequency threshold check
- [ ] ElasticHallway.h - Add modalBuffer member
- [ ] ElasticHallway.cpp - Pre-allocate in prepare()
- [ ] ElasticHallway.cpp - Reuse in applyModalResonances()
- [ ] ElasticHallway.h - Add lastCachedFrequency to ModalFilter struct
- [ ] ElasticHallway.cpp - Cache frequency for IIR updates
- [ ] ElasticHallway.cpp - Add frequency threshold check
- [ ] AlienAmplification.h - Add wetBuffer member
- [ ] AlienAmplification.cpp - Pre-allocate in prepare()
- [ ] AlienAmplification.cpp - Reuse in process
- [ ] AlienAmplification.h - Add lastCachedParadoxGain member
- [ ] AlienAmplification.cpp - Cache gain for IIR updates (2 locations)
- [ ] AlienAmplification.cpp - Add gain threshold checks (2 locations)
- [ ] DspModules.h - Add documentation to loadImpulseResponse()
- [ ] DspModules.h - Add isProcessing member variable
- [ ] DspModules.cpp - Add debug check in loadImpulseResponse()
- [ ] DspModules.cpp - Set isProcessing in process()
- [ ] DspModules.h - Add tempBuffer member
- [ ] DspModules.cpp - Pre-allocate in prepare()
- [ ] DspModules.cpp - Reuse in process()

### Priority 2 Implementation
- [ ] MemoryEchoes.cpp - Remove/gate logging at lines 364-371
- [ ] MemoryEchoes.cpp - Remove/gate logging at lines 649-653
- [ ] ExperimentalModulation.h - Add kMaxRecordedSamples constant
- [ ] ExperimentalModulation.cpp - Pre-reserve in constructor
- [ ] ExperimentalModulation.cpp - Add safety check in recordValue()
- [ ] ModulationMatrix.h - Replace vector with fixed-size array
- [ ] ModulationMatrix.h - Add connectionCount member
- [ ] ModulationMatrix.cpp - Update setConnection() logic
- [ ] ModulationMatrix.cpp - Update getConnections() logic
- [ ] ModulationMatrix.cpp - Update all connection iteration code

### Priority 3 Implementation
- [ ] AlienAmplification.cpp - Add FastMathApproximations include
- [ ] AlienAmplification.cpp - Replace std::tanh at line 262
- [ ] AlienAmplification.cpp - Optional: Add gain lookup table (advanced)
- [ ] ElasticHallway.cpp - Add FastMathApproximations include
- [ ] ElasticHallway.cpp - Replace std::sqrt at line 156
- [ ] TubeRayTracer.cpp - Remove static initialization block
- [ ] TubeRayTracer.h - Add raysInitialized member
- [ ] TubeRayTracer.cpp - Move initialization to prepare()

### Testing & Verification
- [ ] Build succeeds: `cmake --build build`
- [ ] No compiler warnings
- [ ] No runtime assertions (debug mode)
- [ ] Audio still plays (test with DAW)
- [ ] Heap profiler shows zero process-path allocations
- [ ] CPU usage benchmark passes
- [ ] Real-time stress test (see section 5)
- [ ] Git commit: `git commit -am "fix: eliminate allocations from real-time DSP paths"`

---

## Verification

### Build Verification
```bash
# From project root:
cd /Users/noisebox/Documents/3_Development/Repos/monument-reverb

# Clean build
rm -rf build
cmake -B build -S .
cmake --build build

# Check for warnings
cmake --build build 2>&1 | grep -i warning
```

### Heap Profiler (macOS)
```bash
# Build in Debug mode for symbols
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Run under heap profiler
instruments -t 'Allocations' \
    -D /tmp/heap-trace.trace \
    ./build/monument-plugin

# Or use Xcode:
# 1. Product → Profile → Allocations
# 2. Record 10 minutes of audio playback
# 3. Filter for heap allocations in process()
# 4. Expect: ZERO allocations during playback
```

### Audio Test
```bash
# 1. Open in DAW (Logic Pro, Ableton, etc.)
# 2. Play audio clip through plugin
# 3. Listen for clicks, pops, glitches
# 4. Monitor CPU usage (should be stable, no spikes)
# 5. Parameter sweep: Adjust all controls across full range
# 6. Verify no audio dropouts or artifacts
```

### Stress Test Script
```cpp
// In PluginProcessor or standalone:
void stressTest() {
    const int sampleRate = 96000;
    const int blockSize = 64;
    const int numChannels = 2;
    const double testDurationSeconds = 600;  // 10 minutes

    AudioBuffer<float> buffer(numChannels, blockSize);
    MidiBuffer midi;

    int blocksToProcess = (int)(testDurationSeconds * sampleRate / blockSize);

    prepareToPlay(sampleRate, blockSize);

    auto startTime = std::chrono::steady_clock::now();

    for (int block = 0; block < blocksToProcess; ++block) {
        // Fill with test signal (sine wave or noise)
        fillTestSignal(buffer, sampleRate);

        // Process block
        processBlock(buffer, midi);

        // Check for glitches (peak detection)
        float peak = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch) {
            auto* data = buffer.getReadPointer(ch);
            for (int i = 0; i < blockSize; ++i) {
                peak = std::max(peak, std::abs(data[i]));
            }
        }

        // Log results every 100 blocks
        if (block % 100 == 0) {
            printf("Block %d/%d: Peak = %.3f, CPU = %.1f%%\n",
                   block, blocksToProcess, peak, getCpuUsage());
        }
    }

    auto endTime = std::chrono::steady_clock::now();
    printf("Stress test complete: %.1f seconds\n",
           std::chrono::duration<double>(endTime - startTime).count());
}
```

### Expected Results

**Before Fixes:**
```
Block 0/576000: Peak = 0.450, CPU = 12.5%, Allocations = 45/block
Block 100/576000: Peak = 0.449, CPU = 12.6%, Allocations = 44/block
...
Block 576000/576000: Peak = 0.448, CPU = 13.2%, Allocations = 46/block
⚠️ Average: 45 allocations per block, CPU variable
```

**After Fixes:**
```
Block 0/576000: Peak = 0.450, CPU = 9.5%, Allocations = 0/block
Block 100/576000: Peak = 0.449, CPU = 9.4%, Allocations = 0/block
...
Block 576000/576000: Peak = 0.448, CPU = 9.3%, Allocations = 0/block
✅ Average: 0 allocations per block, CPU stable
```

---

## Code Review Checklist

When reviewing fixes, verify:

### Buffer Pre-allocation
- [ ] All buffers allocated in `prepare()` or constructor
- [ ] Buffer size matches max block size from `prepare()`
- [ ] `setSize(channels, blockSize, false, false, true)` parameters correct
- [ ] All temporary buffers are member variables (not local)

### IIR Coefficient Updates
- [ ] Threshold-based update gates added
- [ ] Last cached value members added
- [ ] Threshold values documented with units (Hz or dB)
- [ ] Both old and new code paths checked

### Removed Allocations
- [ ] No `new`, `malloc` in process paths
- [ ] No `std::vector::push_back` in process paths
- [ ] No `std::unique_ptr` operations in process paths
- [ ] No `juce::AudioBuffer` constructors in process paths

### Documentation
- [ ] Off-thread constraints clearly documented
- [ ] Thread safety comments added
- [ ] Real-time rules noted in class comments
- [ ] Threshold values explained

### Testing
- [ ] Code compiles with no warnings
- [ ] No assertions triggered during normal operation
- [ ] Audio functionality unchanged
- [ ] CPU usage improved or neutral
- [ ] Heap allocation count = 0 in process paths

---

## Commit Strategy

```bash
# Priority 1 fixes
git commit -m "fix(dsp): eliminate allocations from real-time audio paths

- Pre-allocate temporary buffers in prepare() for TubeRayTracer, ElasticHallway, AlienAmplification, Pillars
- Add threshold-based IIR coefficient update caching to reduce allocations
- Document loadImpulseResponse() as off-thread only with debug check

Impact: Zero allocations in process() call paths, eliminates audio dropouts"

# Priority 2 fixes (separate commit)
git commit -m "fix(dsp): remove process-path logging and fix edge case allocations

- Remove juce::Logger calls from MemoryEchoes process paths
- Pre-allocate GestureRecorder vector and add safety bounds
- Convert ModulationMatrix connections to fixed-size array

Impact: Safer real-time operation, removes logging overhead"

# Priority 3 fixes (separate commit)
git commit -m "perf(dsp): optimize math operations in signal processing

- Use FastMathApproximations for tanh and sqrt operations
- Move static initialization out of process paths
- Improve CPU headroom for real-time processing

Impact: ~5-15% CPU reduction depending on workload"
```

---

**Next Steps:**
1. Review this plan with team
2. Allocate time for implementation and testing
3. Start with Priority 1 fixes
4. Run verification suite after each priority level
5. Merge to main after all tests pass

---

**Document Location:** `docs/architecture/DSP_REALTIME_SAFETY_FIX_PLAN.md`
**Related Audit:** `docs/architecture/DSP_REALTIME_SAFETY_AUDIT.md`
**Last Updated:** 2026-01-04
