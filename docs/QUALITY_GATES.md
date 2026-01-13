# Monument Reverb - Quality Gates

**Last Updated:** 2026-01-08
**Phase:** Phase 3 Complete - Production Quality Enforcement
**Status:** All Quality Gates Implemented ✅

---

## Overview

Monument Reverb enforces **production-ready quality** through three automated quality gates integrated into the CI pipeline. These gates ensure numerical stability, performance budgets, and real-time safety before code reaches production.

---

## Philosophy

### Why Quality Gates?

Audio plugins have unique requirements:

1. **Real-Time Constraints** - Must process audio without glitches or dropouts
2. **Numerical Stability** - Cannot produce NaN/Inf values that corrupt audio
3. **Performance Budgets** - Must stay within CPU/memory limits
4. **Production Quality** - Must meet professional DAW standards

### Fail-Fast Principle

Quality gates **fail immediately** when violations are detected, preventing bad code from reaching users. This saves time debugging production issues and maintains user trust.

### Gate Severity Levels

- **🔴 Critical (Error)** - Hard failures that break CI and block merges
- **🟡 Warning** - Soft failures that log but don't block (reviewed manually)
- **⚙️ Optional** - Informational checks enabled on-demand

---

## Gate 1: Audio Stability Check ✅

**Status:** 🔴 Critical - Always Active
**Tool:** [`tools/check_audio_stability.py`](../tools/check_audio_stability.py)
**CI Step:** Step 7 of 9
**Duration:** ~1 second (all 37 presets)

### Purpose

Detect numerical instability in audio output that causes:

- Silent output (NaN/Inf values)
- Audio corruption
- DAW crashes
- User complaints

### What It Checks

#### 1. NaN (Not a Number)

**Cause:** Divide-by-zero, invalid math operations

```cpp
// BAD: Can produce NaN if y == 0
float result = x / y;

// GOOD: Guard against zero
float result = (y != 0.0f) ? x / y : 0.0f;
```

**Detection:** Counts any `isnan()` samples in audio
**Threshold:** Zero tolerance (any NaN fails)

#### 2. Infinity

**Cause:** Numerical overflow or underflow

```cpp
// BAD: Can overflow
float gain = pow(10.0f, largeValue);

// GOOD: Clamp before exponentiation
float gain = pow(10.0f, juce::jlimit(-96.0f, 12.0f, largeValue));
```

**Detection:** Counts any `isinf()` samples
**Threshold:** Zero tolerance (any Inf fails)

#### 3. Denormals

**Cause:** Very small numbers (< ~1e-38) causing CPU spikes

```cpp
// BAD: Allows denormals
float feedback = 0.9999f * state;

// GOOD: Use ScopedNoDenormals
juce::ScopedNoDenormals noDenormals;
float feedback = 0.9999f * state;
```

**Detection:** Counts samples < 1e-30
**Threshold:** Zero tolerance (denormals cause CPU spikes)

#### 4. DC Offset

**Cause:** Unintended DC bias in signal

```cpp
// BAD: Accumulates DC
output = input + 0.001f;  // Small constant offset

// GOOD: Use DC blocker
dcBlocker.processSample(output);
```

**Detection:** Measures |mean| of audio signal
**Threshold:** 0.1% (0.001 absolute)

### Usage

```bash
# Check all presets
python3 tools/check_audio_stability.py test-results/preset-baseline

# Check single preset
python3 tools/check_audio_stability.py test-results/preset-baseline/preset_01

# Custom thresholds
python3 tools/check_audio_stability.py test-results/preset-baseline \
  --dc-threshold 0.01 \
  --denormal-threshold 1e-30
```

### Output Example

```text
Checking preset_01/wet.wav...
  ✓ No NaN values detected
  ✓ No Inf values detected
  ✓ No denormals detected (threshold: 1e-30)
  ✓ DC offset: 0.000123 (threshold: 0.001)

Checking preset_02/wet.wav...
  ✗ NaN detected: 3 samples [ERROR]
  ✓ No Inf values detected
  ✓ No denormals detected
  ✗ DC offset: 0.00234 (threshold: 0.001) [WARNING]

Summary: 1/37 presets failed
Exit code: 1 (FAIL)
```

### Exit Codes

- `0` - All presets passed
- `1` - One or more presets failed (NaN/Inf detected)
- `2` - Invalid input or file read error

### CI Integration

Runs automatically in `./scripts/run_ci_tests.sh` after preset capture:

```bash
echo "Step 7/9: Audio Stability Check..."
python3 tools/check_audio_stability.py test-results/preset-baseline
if [ $? -ne 0 ]; then
    echo "❌ Audio stability check failed"
    exit 1
fi
```

### Common Failures & Fixes

| Symptom | Root Cause | Fix |
|---------|-----------|-----|
| NaN in reverb tail | Division by zero in feedback path | Add zero check before division |
| Inf after gain | Exponential overflow in dB conversion | Clamp input before `pow()` |
| Denormals in delay | Very small feedback values | Use `ScopedNoDenormals` |
| DC offset | Missing DC blocker | Add highpass filter (20 Hz) |

---

## Gate 2: CPU Performance Thresholds ⚙️

**Status:** ⚙️ Optional - Enabled if CPU profile exists
**Tool:** [`tools/check_cpu_thresholds.py`](../tools/check_cpu_thresholds.py)
**CI Step:** Step 8 of 9 (conditional)
**Duration:** ~1 second (profile analysis)

### Purpose

Enforce **per-module CPU budgets** to prevent performance regressions. Tracks optimization progress and catches CPU spikes before they reach users.

### What It Checks

#### Per-Module CPU Percentage

Each DSP module has a maximum CPU budget (% of total plugin CPU):

```json
{
  "modules": {
    "TubeRayTracer": {
      "max_percent": 25.0,
      "severity": "error",
      "note": "Most expensive module - ray tracing"
    },
    "Chambers": {
      "max_percent": 20.0,
      "severity": "error",
      "note": "FDN matrix operations"
    },
    "ModulationMatrix": {
      "max_percent": 5.0,
      "severity": "warning",
      "note": "Should be minimal - just parameter routing"
    },
    "AllpassDiffuser": {
      "max_percent": 8.0,
      "severity": "error"
    }
  },
  "total_cpu_percent": 60.0
}
```

#### Total Plugin CPU Load

Maximum CPU budget for entire plugin (% of real-time budget):

- **Target:** < 60% CPU at 48kHz, 512 samples
- **Hard Limit:** 80% (fails CI)

### Usage

```bash
# Step 1: Generate CPU profile
./scripts/profile_cpu.sh

# Step 2: Check thresholds
python3 tools/check_cpu_thresholds.py test-results/cpu_profile.json

# Custom threshold file
python3 tools/check_cpu_thresholds.py test-results/cpu_profile.json \
  --thresholds config/cpu_thresholds_strict.json
```

### Output Example

```text
CPU Performance Thresholds Check
================================

Module Performance:
-------------------
✓ TubeRayTracer: 18.2% (threshold: 25.0%)
  Status: PASS (6.8% under budget)

✓ Chambers: 15.4% (threshold: 20.0%)
  Status: PASS (4.6% under budget)

✗ ModulationMatrix: 7.3% (threshold: 5.0%) [ERROR]
  Status: FAIL (2.3% over budget)
  Recommendation: Optimize connection lookup or reduce checks

✓ AllpassDiffuser: 4.8% (threshold: 8.0%)
  Status: PASS (3.2% under budget)

Total Plugin CPU: 52.3% (threshold: 60.0%)
Status: PASS

Summary: 1 module exceeded threshold
Exit code: 1 (FAIL)
```

### Threshold Configuration

Create `config/cpu_thresholds.json`:

```json
{
  "version": "1.0",
  "description": "CPU performance budgets for Monument Reverb",
  "modules": {
    "TubeRayTracer": {
      "max_percent": 25.0,
      "severity": "error",
      "optimization_priority": "high",
      "notes": "Primary CPU bottleneck - consider SIMD"
    },
    "Chambers": {
      "max_percent": 20.0,
      "severity": "error",
      "optimization_priority": "high",
      "notes": "FDN matrix - candidate for SIMD vectorization"
    },
    "ModulationMatrix": {
      "max_percent": 5.0,
      "severity": "warning",
      "optimization_priority": "medium",
      "notes": "Should be lightweight - check for unnecessary allocations"
    },
    "AllpassDiffuser": {
      "max_percent": 8.0,
      "severity": "error",
      "optimization_priority": "low",
      "notes": "Already optimized with JUCE DSP"
    },
    "MemoryEchoes": {
      "max_percent": 3.0,
      "severity": "warning",
      "optimization_priority": "low"
    }
  },
  "total_cpu_percent": 60.0,
  "sample_rate": 48000,
  "buffer_size": 512,
  "notes": "Budgets based on M1 MacBook Pro baseline"
}
```

### CI Integration

```bash
# Only runs if CPU profile exists (optional check)
if [ -f test-results/cpu_profile.json ]; then
    echo "Step 8/9: CPU Threshold Check..."
    python3 tools/check_cpu_thresholds.py test-results/cpu_profile.json
    if [ $? -ne 0 ]; then
        echo "⚠️  CPU thresholds exceeded (review required)"
        # Don't exit - this is informational
    fi
fi
```

### When to Enable

- **Always:** During optimization work
- **Pre-release:** Before shipping new versions
- **Optional:** In regular CI (adds profiling overhead)

### Optimization Strategy

When a module exceeds its budget:

1. **Profile deeper** - Use Instruments to find hotspots
2. **Consider SIMD** - Vectorize if > 10% CPU
3. **Reduce allocations** - Check for unnecessary memory ops
4. **Cache aggressively** - Avoid redundant calculations
5. **Algorithm change** - Sometimes the approach is wrong

---

## Gate 3: Real-Time Allocation Detection 🔍

**Status:** 🔍 Optional - Environment-controlled (macOS only)
**Tool:** [`tools/check_rt_allocations.sh`](../tools/check_rt_allocations.sh)
**CI Step:** Step 9 of 9 (conditional)
**Duration:** ~60 seconds (30s recording + 30s analysis)

### Purpose

Detect **memory allocations in audio thread** that cause glitches, dropouts, and priority inversion. Audio threads must be **allocation-free** for real-time safety.

### What It Detects

#### Memory Allocations

- `malloc()`, `calloc()`, `realloc()`
- `new` operator
- `delete` operator

#### Container Reallocations

- `std::vector::push_back()` (if reallocation needed)
- `std::vector::resize()`
- `std::map::insert()`
- `std::string` operations

#### JUCE Allocations

- `juce::Array::add()`
- `juce::String` concatenation
- `juce::OwnedArray` operations

### Why Allocations Are Bad

```cpp
// ❌ BAD: Allocates in audio thread
void processBlock(AudioBuffer<float>& buffer) {
    std::vector<float> temp;
    for (int i = 0; i < buffer.getNumSamples(); ++i) {
        temp.push_back(buffer.getSample(0, i));  // Allocates!
    }
}

// ✅ GOOD: Pre-allocated buffer
class MyProcessor {
    std::vector<float> tempBuffer;  // Member variable

    void prepareToPlay(int samplesPerBlock) {
        tempBuffer.resize(samplesPerBlock);  // Allocate once
    }

    void processBlock(AudioBuffer<float>& buffer) {
        // Use pre-allocated buffer (no allocation)
        for (int i = 0; i < buffer.getNumSamples(); ++i) {
            tempBuffer[i] = buffer.getSample(0, i);
        }
    }
};
```

### How It Works

1. **Launch** - Starts Monument Standalone with Instruments
2. **Record** - Captures 30-second System Trace
3. **Analyze** - Parses trace for malloc/free in audio thread
4. **Report** - Shows stack traces for violations

### Usage

```bash
# Enable in CI (optional, slow)
ENABLE_RT_ALLOCATION_CHECK=1 ./scripts/run_ci_tests.sh

# Run standalone
./tools/check_rt_allocations.sh

# Specify custom binary
./tools/check_rt_allocations.sh build/Monument_artefacts/Debug/Standalone/Monument.app
```

### Output Example

#### Success (No Allocations)

```text
Real-Time Allocation Detection
==============================

Starting Monument Standalone...
Recording 30-second trace...
Analyzing trace for allocations in audio thread...

✓ No real-time allocations detected

Thread Analysis:
  Audio thread: 0 malloc/free calls
  Message thread: 142 malloc/free calls (OK)
  Worker threads: 8 malloc/free calls (OK)

Summary: PASS - Audio thread is allocation-free
Exit code: 0 (PASS)
```

#### Failure (Allocations Detected)

```text
Real-Time Allocation Detection
==============================

Starting Monument Standalone...
Recording 30-second trace...
Analyzing trace for allocations in audio thread...

✗ Real-time allocations detected: 5 malloc calls

Violation #1: malloc(320 bytes)
  Stack trace:
    1. malloc()
    2. std::vector<Connection>::push_back()
    3. ModulationMatrix::getConnections() [ModulationMatrix.cpp:658]
    4. PluginProcessor::processBlock() [PluginProcessor.cpp:224]
  Fix: Use pre-allocated array or lock-free snapshot

Violation #2: malloc(128 bytes)
  Stack trace:
    1. malloc()
    2. std::vector<RoutingConnection>::resize()
    3. DspRoutingGraph::loadRoutingPreset() [DspRoutingGraph.cpp:389]
    4. PluginProcessor::processBlock() [PluginProcessor.cpp:312]
  Fix: Use atomic index swap instead of vector reallocation

Summary: FAIL - Fix allocations before shipping
Exit code: 1 (FAIL)
```

### CI Integration

```bash
# Optional - controlled by environment variable
if [ "$ENABLE_RT_ALLOCATION_CHECK" = "1" ]; then
    echo "Step 9/9: Real-Time Allocation Detection..."
    ./tools/check_rt_allocations.sh
    if [ $? -ne 0 ]; then
        echo "❌ Real-time allocations detected"
        exit 1
    fi
fi
```

### When to Enable

- **Pre-release** - Always before shipping
- **After DSP changes** - When modifying audio callback
- **After refactoring** - When changing data structures
- **Never in regular CI** - Too slow (1 minute overhead)

### Common Violations & Fixes

| Violation | Root Cause | Fix |
|-----------|-----------|-----|
| `std::vector::push_back()` | Dynamic array growth | Pre-allocate in `prepareToPlay()` |
| `std::vector::resize()` | Preset changes in callback | Use atomic index swap |
| `juce::SpinLock` | Lock contention | Use lock-free snapshots |
| `std::map::insert()` | Dynamic map growth | Use fixed-size array |
| `std::string` concat | String operations | Use `const char*` or pre-allocated |

### Real-Time Safety Patterns

#### Pattern 1: Pre-Allocation

```cpp
class MyProcessor {
    std::array<float, kMaxConnections> connectionBuffer;  // Fixed-size

    void processBlock() {
        // No allocation - fixed size
        for (size_t i = 0; i < activeConnections; ++i) {
            connectionBuffer[i] = /* ... */;
        }
    }
};
```

#### Pattern 2: Lock-Free Snapshots

```cpp
class MyProcessor {
    std::array<Connection, kMaxConnections> connections[2];  // Double-buffer
    std::atomic<int> readIndex{0};

    void updateConnections() {  // Called from message thread
        int writeIndex = 1 - readIndex.load();
        // Update connections[writeIndex]...
        readIndex.store(writeIndex);  // Atomic swap
    }

    void processBlock() {
        int idx = readIndex.load(std::memory_order_acquire);
        // Read from connections[idx] (no allocation)
    }
};
```

#### Pattern 3: Atomic Flags

```cpp
class MyProcessor {
    std::atomic<bool> bypassEnabled{false};

    void setBypass(bool enabled) {  // Message thread
        bypassEnabled.store(enabled, std::memory_order_release);
    }

    void processBlock() {
        if (bypassEnabled.load(std::memory_order_acquire)) {
            // Bypass processing (no allocation)
        }
    }
};
```

---

## Quality Gate Summary

| Gate | Status | CI Mode | Duration | Severity | Exit on Fail |
|------|--------|---------|----------|----------|--------------|
| Audio Stability | ✅ Active | Always | ~1s | 🔴 Critical | Yes |
| CPU Thresholds | ⚙️ Optional | If profile exists | ~1s | 🟡 Warning | Optional |
| RT Allocations | 🔍 Optional | Env-controlled | ~60s | 🔴 Critical | Yes (if enabled) |

### CI Pipeline Integration

```bash
#!/bin/bash
# scripts/run_ci_tests.sh

# ... (steps 1-6: build, test, capture, analyze, regression) ...

# Step 7: Audio Stability (ALWAYS)
echo "Step 7/9: Audio Stability Check..."
python3 tools/check_audio_stability.py test-results/preset-baseline || exit 1

# Step 8: CPU Thresholds (OPTIONAL - if profile exists)
if [ -f test-results/cpu_profile.json ]; then
    echo "Step 8/9: CPU Threshold Check..."
    python3 tools/check_cpu_thresholds.py test-results/cpu_profile.json
fi

# Step 9: RT Allocations (OPTIONAL - if enabled)
if [ "$ENABLE_RT_ALLOCATION_CHECK" = "1" ]; then
    echo "Step 9/9: Real-Time Allocation Detection..."
    ./tools/check_rt_allocations.sh || exit 1
fi

echo "✅ All quality gates passed"
```

---

## Performance Impact

| Gate | CI Overhead | When to Run | Skip Condition |
|------|-------------|-------------|----------------|
| Audio Stability | +1s | Always | Never |
| CPU Thresholds | +1s | Optional | No CPU profile |
| RT Allocations | +60s | Pre-release only | Env var not set |

**Total CI Time:**

- Minimal: +1s (audio stability only)
- Standard: +2s (audio + CPU)
- Full: +61s (audio + CPU + RT allocations)

---

## Best Practices

### 1. Run Locally Before Pushing

```bash
# Quick check before git push
./scripts/run_ci_tests.sh

# Full check before release
ENABLE_RT_ALLOCATION_CHECK=1 ./scripts/run_ci_tests.sh
```

### 2. Fix Issues Immediately

Don't accumulate quality gate failures. Fix immediately:

- **NaN/Inf** - Usually divide-by-zero (add guards)
- **CPU regression** - Profile and optimize hotspot
- **RT allocations** - Pre-allocate or use lock-free patterns

### 3. Update Thresholds Carefully

Only relax thresholds with justification:

```json
{
  "TubeRayTracer": {
    "max_percent": 30.0,  // Raised from 25.0
    "note": "Added ray reflection feature - acceptable CPU increase"
  }
}
```

### 4. Monitor Trends

Track CPU usage over time:

```bash
# Generate historical report
python3 tools/analyze_cpu_trends.py \
  --baseline test-results/cpu_profile_v1.0.json \
  --current test-results/cpu_profile.json
```

---

## Troubleshooting

### Audio Stability Check Fails

**Symptom:** NaN/Inf detected in presets

**Debug:**

```bash
# Check specific preset
python3 tools/check_audio_stability.py test-results/preset-baseline/preset_05

# Inspect audio file
open test-results/preset-baseline/preset_05/wet.wav

# Profile with Instruments to find source
./scripts/profile_with_audio.sh
```

**Common Fixes:**

- Add zero checks before division
- Clamp values before exponentiation
- Use `ScopedNoDenormals`
- Add highpass DC blocker

### CPU Threshold Check Fails

**Symptom:** Module exceeds CPU budget

**Debug:**

```bash
# Generate detailed profile
./scripts/profile_cpu.sh

# Analyze hotspots
python3 scripts/analyze_profile.py monument_profile_export.xml

# Profile in DAW
./scripts/profile_in_reaper.sh
```

**Common Fixes:**

- Vectorize with SIMD
- Reduce allocations
- Cache expensive calculations
- Use lookup tables

### RT Allocation Check Fails

**Symptom:** Allocations detected in audio thread

**Debug:**

```bash
# Run with detailed output
./tools/check_rt_allocations.sh --verbose

# Inspect stack traces
# Look for std::vector, std::map, std::string operations
```

**Common Fixes:**

- Pre-allocate in `prepareToPlay()`
- Use lock-free snapshots
- Replace SpinLock with atomics
- Use fixed-size arrays

---

## Future Enhancements

### Planned Quality Gates

1. **Parameter Smoothing Test** - Click/pop detection (<-60dB THD+N)
2. **Stereo Width Test** - Spatial correctness validation
3. **Latency Test** - DAW PDC compatibility
4. **State Save/Recall Test** - Automation compatibility

### Automation Improvements

1. **Automatic Threshold Tuning** - Machine learning-based budget optimization
2. **Historical Trending** - Track CPU/stability over git commits
3. **Visual Dashboard** - Web UI showing quality gate status
4. **Slack Integration** - Notify team of quality gate failures

---

## References

- **Implementation:** [docs/PHASE_3_STEP_2_QUALITY_GATES_COMPLETE.md](PHASE_3_STEP_2_QUALITY_GATES_COMPLETE.md)
- **Tool Docs:** [scripts/README.md#quality-gate-scripts](../scripts/README.md#quality-gate-scripts)
- **Testing Guide:** [testing/TESTING_GUIDE.md#quality-gates](testing/TESTING_GUIDE.md#quality-gates)

---

**Document Version:** 1.0
**Phase:** Phase 3 Complete
**Status:** Production-Ready ✅
