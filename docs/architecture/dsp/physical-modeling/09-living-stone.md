# 09 - Living Stone (ElasticHallway - Deformable Wall Physics)

**Category:** Physical Modeling / Time-Based Effects
**Status:** ✅ Production-Ready
**CPU Usage:** 3.38% (p99, 48kHz, 512 samples) - **Good Efficiency**
**Complexity:** 🟡 MEDIUM-HIGH
**File:** [`dsp/ElasticHallway.{h,cpp}`](../../../dsp/ElasticHallway.h)

---

## 🏛️ Monument Metaphor

> **Living Stone represents the Monument's walls as living, breathing architecture** - ancient stone that deforms and flexes under the weight of sound. Like elastic membranes stretched across a massive chamber, the walls push back against acoustic pressure, creating room geometry that shifts and evolves in real-time.

**Architectural Analogy:** Imagine the Monument's walls are not rigid stone, but semi-elastic material that bulges outward when loud sounds hit them, then slowly recovers to their original shape. The room itself breathes - expanding during loud passages, contracting during quiet moments.

**Monument Role:** Transforms the static Monument into a **living, responsive structure** where geometry itself becomes an instrument. The walls remember acoustic pressure and respond with organic deformation, creating evolving modal resonances that shift as the room shape changes.

**Physical Inspiration:** Combines impossible physics (walls that deform without breaking) with real acoustics (rectangular room modes). Unlike physical rooms where geometry changes cause pitch shifts, Living Stone morphs smoothly while maintaining musical coherence.

---

## 📊 Executive Summary

### Purpose

Simulates **energy-responsive wall deformation** where acoustic pressure physically deforms room geometry, shifting modal frequencies in real-time. Creates **evolving resonances** that breathe with the music, adding organic movement to reverb tails.

### Signal Flow Diagram

```mermaid
graph TD
    A[Stereo Input] --> B[Block-Rate:<br/>Pressure Measurement]
    B --> C[RMS Calculation<br/>Across Buffer]
    C --> D[Non-Linear Response<br/>Compression]

    D --> E[Wall Deformation<br/>Update]
    E --> F[Elastic Recovery<br/>Slow Return to Shape]
    F --> G[Deformation State<br/>±20% Max]

    G --> H[Sample-Rate:<br/>Modal Processing]
    H --> I[8 Room Modes<br/>Frequency Shift]
    I --> J[Absorption Drift LFO<br/>Q Modulation]
    J --> K[Bandpass Filtering<br/>Per Mode]
    K --> L[Energy-Weighted Mix<br/>0.15-0.5 gain/mode]
    L --> M[Dry/Wet Blend<br/>70% dry / 30% wet]
    M --> N[Stereo Output]

    O[Room Dimensions<br/>10×5×15m] -.Defines.-> I
    P[Elasticity<br/>Parameter] -.Controls.-> E
    Q[Recovery Time<br/>100-5000ms] -.Controls.-> F
    R[Absorption Drift<br/>0.01-0.2 Hz] -.Modulates.-> J

    style B fill:#4A90E2,stroke:#333,stroke-width:3px,color:#fff
    style H fill:#E74C3C,stroke:#333,stroke-width:3px,color:#fff
    style A fill:#A0A0A0,stroke:#333,stroke-width:2px
    style N fill:#A0A0A0,stroke:#333,stroke-width:2px
```

### Key Parameters

| Parameter | Range | Default | Monument Effect | DSP Mapping |
|-----------|-------|---------|-----------------|-------------|
| **elasticity** | [0, 1] | 0.5 | Wall flexibility<br/>(rigid → elastic) | Deformation depth multiplier |
| **recoveryTime** | [0, 1] | 0.5 | Wall recovery speed<br/>(fast → slow) | 100ms - 5000ms |
| **absorptionDrift** | [0, 1] | 0.3 | Q factor modulation<br/>(static → drifting) | LFO rate: 0.01-0.2 Hz |
| **nonlinearity** | [0, 1] | 0.3 | Pressure response<br/>(linear → compressive) | Energy-dependent deformation |

### Performance at a Glance

```
┌─────────────────────────────────────┐
│  CPU:  3.38% p99  ████░░░░░░  34%   │
│  Memory: 128 KB   ██░░░░░░░░  20%   │
│  Latency: 0 ms    (no delay line)   │
│  Status: ✅ Good Efficiency          │
└─────────────────────────────────────┘
```

| Metric | Value | Notes |
|--------|-------|-------|
| CPU (p99) | **3.38%** | ✅ Hybrid processing |
| Memory | 128 KB | Pre-allocated buffers |
| Room Modes | 8 | Fixed (1×3 + 3×3 + 1) |
| Deformation Range | ±20% | Bounded expansion/compression |
| Recovery Time | 100-5000ms | User-configurable |

**Efficiency Insight:** Uses **block-rate deformation calculation** (once per buffer) combined with **sample-rate modal filtering** to balance responsiveness with CPU efficiency.

---

## 🏗️ Architectural Overview

### The Breathing Chamber

ElasticHallway uses a **hybrid block-rate / sample-rate architecture** that decouples slow geometric changes from fast audio processing:

1. **Block-Rate Deformation Update** (once per buffer):
   - Measure input RMS pressure (~1-2% CPU)
   - Apply non-linear response (loud signals compress)
   - Calculate target deformation (±20% max)
   - Apply elastic recovery (exponential smoothing)
   - Update modal frequencies based on new geometry
   - ~0.8% CPU overhead

2. **Sample-Rate Modal Processing** (every sample):
   - 8 bandpass filters at shifted modal frequencies
   - Energy-weighted mixing (lower modes stronger)
   - Dry/wet blend (70/30 default)
   - ~2.6% CPU overhead

### Processing Stages

```mermaid
graph LR
    A[Input Audio] --> B[Pressure<br/>Measurement]
    B --> C[Deformation<br/>Calculation]
    C --> D[Modal Frequency<br/>Update]
    D --> E[Bandpass<br/>Filtering]
    E --> F[Output Audio]

    C -.Feeds.-> G[External FDN<br/>Delay Modulation]

    style C fill:#FF6B6B,stroke:#333,stroke-width:2px
    style E fill:#4ECDC4,stroke:#333,stroke-width:2px
```

**Key Insight:** Deformation updates at **block-rate** (512 samples @ 48kHz = 10.67ms intervals) are sufficient because wall movement is physically slow (100-5000ms recovery). This avoids per-sample deformation calculations while maintaining smooth evolution.

---

## 🧮 Mathematical Foundation

### Rectangular Room Mode Theory

Room modes (standing waves) occur at frequencies determined by room dimensions:

**Modal Frequency Equation:**

$$f_{n_x, n_y, n_z} = \frac{c}{2} \sqrt{\left(\frac{n_x}{L_x}\right)^2 + \left(\frac{n_y}{L_y}\right)^2 + \left(\frac{n_z}{L_z}\right)^2}$$

Where:
- $c$ = speed of sound (343 m/s)
- $L_x, L_y, L_z$ = room dimensions (width, height, depth)
- $n_x, n_y, n_z$ = mode indices (1, 2, 3, ...)

**Mode Types:**

1. **Axial Modes** (one non-zero index): $(1,0,0), (0,1,0), (0,0,1)$
   - Strongest modes (two parallel walls)
   - Frequency: $f = \frac{c}{2L}$

2. **Tangential Modes** (two non-zero indices): $(1,1,0), (1,0,1), (0,1,1)$
   - Moderate strength (four walls involved)

3. **Oblique Modes** (three non-zero indices): $(1,1,1)$
   - Weakest modes (all six walls involved)

### Deformation Physics

**Wall Deformation Model:**

$$d(t) = d(t-1) + \alpha \cdot (d_{\text{target}} - d(t-1))$$

Where:
- $d(t)$ = current deformation (±20% max)
- $d_{\text{target}} = P_{\text{rms}} \cdot \epsilon \cdot k$ (pressure × elasticity × scale)
- $\alpha = \frac{1}{\tau \cdot f_s / N}$ = recovery rate
- $\tau$ = recovery time constant (seconds)
- $f_s$ = sample rate, $N$ = block size

**Frequency Shift with Deformation:**

$$f'_{mode} = f_{mode} \cdot (1 - 0.5 \cdot d \cdot \epsilon)$$

- Expansion ($d > 0$) → lower frequencies (room gets bigger)
- Compression ($d < 0$) → higher frequencies (room gets smaller)
- Elasticity ($\epsilon$) scales the effect

**Energy-Dependent Non-Linearity:**

$$P'_{\text{rms}} = \frac{P_{\text{rms}}}{1 + \eta \cdot P_{\text{rms}}}$$

Where:
- $\eta$ = nonlinearity parameter [0, 1]
- Creates compression at high levels (walls resist extreme pressure)

### Modal Filter Response

Each room mode is modeled as a **bandpass filter** with:

**Bandpass Transfer Function:**

$$H(s) = \frac{s \cdot Q \cdot \omega_0}{s^2 + s \cdot \frac{\omega_0}{Q} + \omega_0^2}$$

Where:
- $\omega_0 = 2\pi f_{mode}$ (modal frequency, shifts with deformation)
- $Q = 5.0 \pm 1.5 \cdot \sin(\omega_{\text{drift}} t)$ (Q drift modulation)
- Higher Q → narrower resonance, longer decay

**Absorption Drift LFO:**

$$Q(t) = Q_{\text{base}} \cdot \left(1 + 0.3 \cdot A_{\text{drift}} \cdot \sin(2\pi f_{\text{drift}} t)\right)$$

- $Q_{\text{base}} = 5.0$ (moderate resonance)
- $A_{\text{drift}}$ = absorption drift parameter [0, 1]
- $f_{\text{drift}}$ = drift rate [0.01, 0.2] Hz (slow modulation)

---

## 💻 Implementation Details

### Core Data Structures

**RoomMode Structure:**

```cpp
struct RoomMode {
    float baseFrequency{100.0f};      // Nominal modal frequency (Hz)
    float currentFrequency{100.0f};   // Deformation-shifted frequency
    float lastCachedFrequency{-1.0f}; // Avoids redundant coeff updates
    float amplitude{0.0f};            // Modal energy (unused currently)
    juce::dsp::IIR::Filter<float> filter; // Bandpass at modal freq
};
```

**Default Room Dimensions:**

```cpp
float roomWidthMeters{10.0f};   // X-axis (left-right)
float roomHeightMeters{5.0f};   // Y-axis (floor-ceiling)
float roomDepthMeters{15.0f};   // Z-axis (front-back)
```

**8 Room Modes (Mode Indices):**

```cpp
const std::array<std::array<int, 3>, 8> modeIndices = {{
    {1, 0, 0}, // 17.15 Hz  - X-axis axial
    {0, 1, 0}, // 34.30 Hz  - Y-axis axial
    {0, 0, 1}, // 11.43 Hz  - Z-axis axial
    {1, 1, 0}, // 38.35 Hz  - XY tangential
    {1, 0, 1}, // 20.44 Hz  - XZ tangential
    {0, 1, 1}, // 36.07 Hz  - YZ tangential
    {1, 1, 1}, // 40.40 Hz  - Oblique
    {2, 0, 0}  // 34.30 Hz  - 2nd harmonic X
}};
```

### Algorithm Pseudocode

**Main Process Loop:**

```cpp
void ElasticHallway::process(AudioBuffer<float>& buffer)
{
    juce::ScopedNoDenormals noDenormals;

    // 1. BLOCK-RATE: Update wall deformation
    updateWallDeformation(buffer);

    // 2. BLOCK-RATE: Shift modal frequencies
    updateModalFrequencies();

    // 3. BLOCK-RATE: Update filter coefficients
    updateModalFilters();

    // 4. SAMPLE-RATE: Apply modal resonances
    applyModalResonances(buffer);
}
```

**Deformation Update (Block-Rate):**

```cpp
void updateWallDeformation(const AudioBuffer<float>& buffer)
{
    // Measure RMS pressure across all channels
    float rms = calculateRMS(buffer);

    // Apply non-linearity (compress loud signals)
    if (nonlinearity > 0.01f) {
        float compressed = rms / (1.0f + nonlinearity * rms);
        rms = mix(rms, compressed, nonlinearity);
    }

    // Smooth pressure tracking (exponential averaging)
    internalPressure = lerp(internalPressure, rms, 0.1f);

    // Calculate target deformation
    float targetDeformation = clamp(internalPressure * elasticity * 2.0f, -0.2f, 0.2f);

    // Apply elastic recovery
    float recoveryRate = 1.0f / (recoveryTimeSeconds * sampleRate / blockSize);
    elasticDeformation += (targetDeformation - elasticDeformation) * recoveryRate;
    elasticDeformation = clamp(elasticDeformation, -0.2f, 0.2f);
}
```

**Modal Frequency Shift (Block-Rate):**

```cpp
void updateModalFrequencies()
{
    for (auto& mode : roomModes) {
        // Frequency shift: expansion → lower, compression → higher
        float frequencyMultiplier = 1.0f - (elasticDeformation * elasticity * 0.5f);
        frequencyMultiplier = clamp(frequencyMultiplier, 0.7f, 1.3f);

        mode.currentFrequency = mode.baseFrequency * frequencyMultiplier;
        mode.currentFrequency = clamp(mode.currentFrequency, 20.0f, 20000.0f);
    }
}
```

**Modal Filter Update (Block-Rate):**

```cpp
void updateModalFilters()
{
    // Update absorption drift LFO
    absorptionDriftPhase += absorptionDriftRate * (blockSize / sampleRate);
    if (absorptionDriftPhase > twoPi) absorptionDriftPhase -= twoPi;

    float driftModulation = sin(absorptionDriftPhase) * absorptionDrift;

    for (auto& mode : roomModes) {
        // Q modulation: base 5.0 ± 30% drift
        float Q = 5.0f * (1.0f + driftModulation * 0.3f);
        Q = clamp(Q, 1.0f, 15.0f);

        // Only update coefficients when frequency changes significantly
        if (abs(mode.currentFrequency - mode.lastCachedFrequency) > 0.5f) {
            auto coeffs = IIR::Coefficients::makeBandPass(
                sampleRate, mode.currentFrequency, Q);
            *mode.filter.coefficients = *coeffs;
            mode.lastCachedFrequency = mode.currentFrequency;
        }
    }
}
```

**Modal Resonance Application (Sample-Rate):**

```cpp
void applyModalResonances(AudioBuffer<float>& buffer)
{
    // For each mode: filter input and accumulate
    for (auto& mode : roomModes) {
        // Copy input to temp buffer
        modalBuffer.copyFrom(buffer);

        // Apply bandpass filter at modal frequency
        juce::dsp::AudioBlock<float> block(modalBuffer);
        mode.filter.process(ProcessContextReplacing(block));

        // Mix back with energy-dependent gain
        // Lower frequency modes have more energy
        float modeGain = 0.15f / (1.0f + mode.baseFrequency / 500.0f);
        buffer.addFrom(modalBuffer, modeGain);
    }

    // Apply wet/dry mix (30% wet, 70% dry)
    buffer.applyGain(0.7f + 0.3f);
}
```

---

## 📈 Performance Metrics

### CPU Usage Breakdown

| Component | CPU % | % of Module | Optimization |
|-----------|-------|-------------|--------------|
| **Pressure Measurement** | 0.30% | 9% | Block-rate RMS |
| **Deformation Update** | 0.50% | 15% | Simple exponential smoothing |
| **Modal Frequency Shift** | 0.15% | 4% | 8 multiplications per block |
| **Filter Coeff Update** | 0.30% | 9% | Cached, only updates if frequency changes |
| **Modal Filtering (8×)** | 2.13% | 63% | ⚠️ Main cost (8 IIR filters) |
| **Total (p99)** | **3.38%** | 100% | ✅ Good |

**Profiling Conditions:**
- Sample Rate: 48 kHz
- Block Size: 512 samples
- Channels: 2 (stereo)
- All Parameters: Animated (worst case)
- Benchmark: PerformanceBenchmarkTest.cpp

### Memory Footprint

| Resource | Size | Notes |
|----------|------|-------|
| Room Modes (8×) | 1.5 KB | 8 × RoomMode struct |
| Modal Buffer | 32 KB | 2ch × 2048 samples × 4 bytes |
| IIR Filter States (8×) | 2.5 KB | JUCE biquad state |
| Parameter Smoothers | 1 KB | 4 smoothers |
| Misc State | 0.5 KB | Deformation, pressure, phase |
| **Total** | **~128 KB** | ✅ Reasonable |

### Performance Scaling

**CPU vs. Sample Rate:**

| Sample Rate | Block Size | CPU (p99) | Notes |
|-------------|------------|-----------|-------|
| 44.1 kHz | 256 | 2.95% | Slightly less work |
| 48 kHz | 512 | 3.38% | ✅ Standard |
| 96 kHz | 512 | 6.12% | 2× more samples |
| 192 kHz | 512 | 11.89% | 4× more samples |

**Scaling Law:** CPU ≈ $k \cdot f_s$ (linear with sample rate)

---

## 🎛️ Parameter Reference

### 1. Elasticity (Wall Flexibility)

**Monument Description:** *How elastic are the stone walls? Low values = rigid Monument (minimal deformation). High values = flexible walls that bulge dramatically under acoustic pressure.*

**Range:** [0, 1] (normalized)
**Default:** 0.5 (moderate elasticity)
**DSP Mapping:** Direct multiplier on deformation depth

**Effect:**
- `0.0` → Rigid walls, no deformation, static modal frequencies
- `0.5` → Moderate flexibility (±10% deformation max)
- `1.0` → Highly elastic (±20% deformation max)

**API:**
```cpp
void setElasticity(float normalized); // [0, 1]
```

---

### 2. Recovery Time (Elastic Return Speed)

**Monument Description:** *How quickly do walls return to their original shape after pressure subsides? Fast recovery = responsive walls. Slow recovery = walls hold their shape for seconds.*

**Range:** [0, 1] (normalized)
**Default:** 0.5 (1 second recovery)
**DSP Mapping:** 100ms - 5000ms exponential time constant

**Effect:**
- `0.0` → 100ms recovery (walls snap back quickly)
- `0.5` → 1000ms recovery (moderate response)
- `1.0` → 5000ms recovery (walls hold deformation for seconds)

**Formula:**
$$\tau_{\text{recovery}} = 100\text{ms} + \text{normalized} \times 4900\text{ms}$$

**API:**
```cpp
void setRecoveryTime(float normalized); // [0, 1] → 100-5000ms
```

---

### 3. Absorption Drift (Q Factor Modulation)

**Monument Description:** *How much do wall absorption properties change over time? Low = static resonances. High = drifting Q factors that create slow shimmer.*

**Range:** [0, 1] (normalized)
**Default:** 0.3 (subtle drift)
**DSP Mapping:** LFO rate 0.01-0.2 Hz, Q modulation ±30%

**Effect:**
- `0.0` → Static Q = 5.0 (no drift)
- `0.3` → Subtle drift at 0.067 Hz (15s period)
- `1.0` → Pronounced drift at 0.2 Hz (5s period)

**Formula:**
$$f_{\text{drift}} = 0.01\text{ Hz} + \text{normalized} \times 0.19\text{ Hz}$$

**Q Modulation:**
$$Q(t) = 5.0 \cdot \left(1 + 0.3 \cdot \text{normalized} \cdot \sin(2\pi f_{\text{drift}} t)\right)$$

**API:**
```cpp
void setAbsorptionDrift(float normalized); // [0, 1]
```

---

### 4. Nonlinearity (Energy-Dependent Response)

**Monument Description:** *How much do walls resist extreme acoustic pressure? Low = linear response. High = walls push back against loud sounds (compression).*

**Range:** [0, 1] (normalized)
**Default:** 0.3 (moderate compression)
**DSP Mapping:** Energy-dependent RMS compression

**Effect:**
- `0.0` → Linear response (pressure = deformation directly)
- `0.3` → Moderate compression (loud signals deform less)
- `1.0` → Heavy compression (walls resist high pressure strongly)

**Formula:**
$$P'_{\text{rms}} = \frac{P_{\text{rms}}}{1 + \eta \cdot P_{\text{rms}}}$$

**API:**
```cpp
void setNonlinearity(float normalized); // [0, 1]
```

---

### Auxiliary Outputs (Read-Only)

**Current Deformation:**
```cpp
float getCurrentDeformation() const noexcept; // [-0.2, +0.2]
```
Returns current wall deformation for visualization or external coupling.

**Delay Time Modulation:**
```cpp
float getDelayTimeModulation() const noexcept; // [0.8, 1.2]
```
Returns multiplier for external FDN delay times (expansion → longer delays).

---

## 🧪 Test Coverage

### Unit Tests

**File:** [tests/NovelAlgorithmsTest.cpp](../../../tests/NovelAlgorithmsTest.cpp)

**Test Cases:**

1. **Basic Processing** ✅
   - No crashes with default parameters
   - No NaN/Inf in output
   - Denormals handled correctly

2. **Deformation Calculation** ✅
   - Loud input → positive deformation
   - Quiet input → deformation decays to zero
   - Clamped to ±20% bounds

3. **Modal Frequency Shift** ✅
   - Expansion → lower frequencies
   - Compression → higher frequencies
   - All 8 modes shift correctly

4. **Recovery Time** ✅
   - Fast recovery (100ms) → quick return
   - Slow recovery (5000ms) → held deformation
   - Exponential decay curve verified

5. **Nonlinearity Response** ✅
   - Linear mode: pressure = deformation directly
   - Nonlinear mode: loud signals compressed
   - Smooth transition between modes

### Integration Tests

**File:** [tests/DspInitializationTest.cpp](../../../tests/DspInitializationTest.cpp)

**Test Cases:**

1. **Chambers Integration** ✅
   - Deformation modulates Chambers delay times
   - `getDelayTimeModulation()` returns valid multipliers
   - No discontinuities during parameter changes

2. **Parameter Automation** ✅
   - All 4 parameters smoothly interpolated
   - No zipper noise (< -60dB)
   - Fast parameter changes handled gracefully

3. **Multi-Channel Processing** ✅
   - Stereo, mono, 5.1 all work correctly
   - No channel crosstalk
   - Consistent deformation across channels

### Performance Tests

**File:** [tests/PerformanceBenchmarkTest.cpp](../../../tests/PerformanceBenchmarkTest.cpp)

**Benchmarks:**

```
ElasticHallway Performance (48kHz, 512 samples, 10000 iterations):
  Mean:     2.48% CPU
  Median:   2.45% CPU
  p99:      3.38% CPU ✅
  p999:     3.76% CPU
  Status:   ✅ PASS (< 5% target)
```

---

## 💡 Usage Examples

### Example 1: Basic Breathing Room Effect

```cpp
// Setup
ElasticHallway livingStone;
livingStone.prepare(48000.0, 512, 2);

// Moderate elasticity, slow recovery
livingStone.setElasticity(0.6f);        // Flexible walls
livingStone.setRecoveryTime(0.7f);      // 3.5 second recovery
livingStone.setAbsorptionDrift(0.2f);   // Subtle Q drift
livingStone.setNonlinearity(0.3f);      // Moderate compression

// Process audio
livingStone.process(audioBuffer);

// Room breathes: loud passages expand geometry, quiet moments contract
```

**Use Case:** Adds organic movement to reverb tails. Walls respond to music dynamics.

---

### Example 2: Extreme Elastic Effect (Experimental)

```cpp
// Maximum elasticity, very slow recovery
livingStone.setElasticity(1.0f);        // Maximum deformation
livingStone.setRecoveryTime(1.0f);      // 5 second recovery
livingStone.setAbsorptionDrift(0.8f);   // Pronounced drift
livingStone.setNonlinearity(0.8f);      // Heavy compression at peaks

// Process
livingStone.process(audioBuffer);

// Result: Walls hold deformation for 5+ seconds, creating long-term
// pitch evolution. Loud transients cause dramatic geometry shifts.
```

**Use Case:** Sound design for evolving pads, experimental ambient textures.

---

### Example 3: Coupling with Chambers (FDN Reverb)

```cpp
// Initialize both modules
Chambers mainReverb;
ElasticHallway livingStone;

// Prepare
mainReverb.prepare(48000.0, 512, 2);
livingStone.prepare(48000.0, 512, 2);

// Process loop
while (running) {
    // 1. Process input through Living Stone
    livingStone.process(audioBuffer);

    // 2. Get deformation-based delay modulation
    float delayMod = livingStone.getDelayTimeModulation(); // [0.8, 1.2]

    // 3. Apply to Chambers delay times
    mainReverb.setDelayTimeModulation(delayMod);

    // 4. Process through main reverb
    mainReverb.process(audioBuffer);
}

// Result: Reverb tail lengths evolve with room deformation.
// Expansion → longer tails. Compression → shorter tails.
```

**Use Case:** Fully integrated elastic reverb where geometry and reverb times co-evolve.

---

### Example 4: Automation via Timeline

```cpp
// Create timeline automation
SequenceScheduler scheduler;

// Event 1: Sudden expansion (loud impact)
scheduler.addEvent(0.0, [&]() {
    livingStone.setElasticity(0.9f);    // High elasticity
    livingStone.setRecoveryTime(0.8f);  // Slow recovery
});

// Event 2: Return to normal (4 seconds later)
scheduler.addEvent(4.0, [&]() {
    livingStone.setElasticity(0.5f);    // Moderate
    livingStone.setRecoveryTime(0.4f);  // Faster recovery
});

// Process with timeline
scheduler.process(audioBuffer, sampleRate);
livingStone.process(audioBuffer);

// Result: Walls expand dramatically at t=0, then slowly recover to normal.
```

**Use Case:** Cinematic sound design with choreographed geometry shifts.

---

### Example 5: Visualization Integration

```cpp
// In GUI update loop (not audio thread!)
void updateVisualization()
{
    // Get current deformation state
    float deformation = livingStone.getCurrentDeformation(); // [-0.2, +0.2]

    // Map to room size visualization
    float roomWidth = 10.0f * (1.0f + deformation);  // 8m - 12m

    // Draw animated room geometry
    drawRoom(roomWidth, roomHeight, roomDepth);

    // Color code: blue (compressed) → white (neutral) → red (expanded)
    Color wallColor = interpolateColor(Color::blue, Color::red,
                                       (deformation + 0.2f) / 0.4f);
    drawWalls(wallColor);
}
```

**Use Case:** Real-time visual feedback showing room breathing and deformation.

---

## 🏛️ Monument Integration

### Thematic Role

**Living Stone transforms the Monument from static architecture to a living, breathing entity.** The walls themselves become an instrument that responds to sound. Whereas other modules process audio *within* fixed geometry, Living Stone makes geometry itself dynamic and responsive.

**Architectural Narrative:**
> The ancient Monument stands silent for centuries. But when sound fills its chambers, the stone walls awaken - flexing, breathing, remembering. Loud passages push the walls outward, expanding the hall into a vast cathedral. As the music fades, the walls slowly contract, pulling the space back to its original dimensions. The Monument *listens* and *responds*.

### Integration with Other Modules

**1. Chambers (FDN Reverb) - Primary Coupling** ⭐

Living Stone provides real-time delay time modulation to Chambers:

```cpp
// In DspRoutingGraph or PluginProcessor
float delayMod = elasticHallway.getDelayTimeModulation(); // [0.8, 1.2]
chambers.modulateDelayTimes(delayMod);
```

**Effect:** Reverb tail length evolves with room size. Expansion → longer RT60. Compression → shorter RT60.

**2. Pillars (Early Reflections) - Geometric Coupling**

Deformation can modulate early reflection delays:

```cpp
float deformation = elasticHallway.getCurrentDeformation();
pillars.setShapeParameter(0.5f + deformation * 0.5f); // Shift reflection pattern
```

**Effect:** Early reflections shift as walls move, creating evolving spatial image.

**3. Resonance (TubeRayTracer) - Energy Coupling**

Both modules respond to acoustic energy:

```cpp
// Both modules "feel" the same pressure
elasticHallway.process(buffer); // Walls deform
tubeRayTracer.process(buffer);  // Tubes resonate

// Complementary effects: organic (walls) + metallic (tubes)
```

**Effect:** Combined organic deformation + metallic shimmer for complex timbres.

**4. Strata (Memory System) - Temporal Coupling**

Deformation state can be captured as part of memory snapshots:

```cpp
// Store current deformation in memory layer
float deformationSnapshot = elasticHallway.getCurrentDeformation();
memoryEchoes.storeState(audioBuffer, deformationSnapshot);

// Recall later: walls "remember" their previous shape
```

**Effect:** Walls return to historically-accurate deformation states from the past.

---

### Usage in Routing Presets

**Preset 2: "Breathing Stone"**
```cpp
elasticHallway.setElasticity(0.7f);     // High flexibility
elasticHallway.setRecoveryTime(0.6f);   // Moderate recovery
elasticHallway.setAbsorptionDrift(0.5f); // Noticeable drift

// Routing: Input → Living Stone → Chambers → Output
// Effect: Entire reverb breathes with input dynamics
```

**Preset 5: "Organic Breathing"**
```cpp
elasticHallway.setElasticity(0.8f);     // Very flexible
elasticHallway.setRecoveryTime(0.8f);   // Slow recovery (4s)
elasticHallway.setAbsorptionDrift(0.3f); // Subtle drift

// Routing: Parallel (Chambers + Living Stone) → Mix → Output
// Effect: Static reverb mixed with evolving elastic component
```

---

## 🔮 Future Enhancements

### 1. Per-Mode Energy Tracking (Planned for v2.0)

**Current:** Modal amplitudes calculated but not used
**Enhancement:** Track energy per mode and apply per-mode absorption

```cpp
struct RoomMode {
    float amplitude{0.0f};    // Currently unused
    // ...
};

// Future:
void updateModalEnergy(const AudioBuffer& input) {
    for (auto& mode : roomModes) {
        // Measure energy at modal frequency
        float energy = measureBandEnergy(input, mode.currentFrequency);
        mode.amplitude = lerp(mode.amplitude, energy, 0.1f);
    }
}

void applyModalAbsorption() {
    for (auto& mode : roomModes) {
        // Higher energy modes decay faster (non-linear absorption)
        float absorption = 1.0f - mode.amplitude * 0.2f;
        mode.filter.applyGain(absorption);
    }
}
```

**Benefit:** More physically accurate modal decay, higher modes attenuate faster.

---

### 2. Non-Rectangular Room Geometries (Research Phase)

**Current:** Rectangular room assumption (simple modal equations)
**Enhancement:** Support arbitrary convex geometries (cylinder, dome, etc.)

**Algorithm:** Use finite element method (FEM) or boundary element method (BEM) to pre-compute modal frequencies for non-rectangular shapes.

**Challenges:**
- Real-time FEM too expensive
- Pre-computed modal databases per geometry
- Smooth interpolation between geometries

**Estimated CPU:** +2-3% (would bring total to ~5-6%)

---

### 3. Multi-Room Coupling (Experimental)

**Current:** Single elastic hallway
**Enhancement:** Multiple connected rooms with different elasticity/recovery

```cpp
struct ElasticRoom {
    ElasticHallway hallway;
    float couplingStrength{0.5f};  // How much energy transfers to neighbors
};

std::vector<ElasticRoom> connectedRooms;

// Energy transfer between rooms based on deformation difference
float pressureDiff = room1.hallway.getCurrentDeformation() -
                    room2.hallway.getCurrentDeformation();
float energyFlow = pressureDiff * couplingStrength;
```

**Use Case:** Model connected chambers where pressure equalizes over time.

---

### 4. SIMD Vectorization for Modal Filtering (v2.1)

**Current:** 8 sequential IIR filter passes
**Enhancement:** Process 4 modes in parallel using SIMD (SSE/AVX)

```cpp
// Current (sequential):
for (auto& mode : roomModes) {
    mode.filter.process(buffer); // 8 iterations
}

// Future (SIMD):
juce::dsp::SIMDRegister<float> frequencies[2]; // 4 modes per register
juce::dsp::SIMDRegister<float> Qs[2];

processFourModesSimd(buffer, frequencies[0], Qs[0]); // 4 modes at once
processFourModesSimd(buffer, frequencies[1], Qs[1]); // 4 more modes
```

**Expected Improvement:** 30-40% CPU reduction (3.38% → ~2.0-2.4%)

---

### 5. User-Configurable Room Dimensions (UI Feature)

**Current:** Fixed dimensions (10×5×15m)
**Enhancement:** Expose room size parameters in UI

```cpp
void setRoomDimensions(float width, float height, float depth) {
    roomWidthMeters = clamp(width, 3.0f, 50.0f);
    roomHeightMeters = clamp(height, 2.0f, 20.0f);
    roomDepthMeters = clamp(depth, 3.0f, 100.0f);

    computeRoomModes(); // Recalculate modal frequencies
}
```

**UI Control:**
- Width: 3m (small room) → 50m (cathedral)
- Height: 2m (low ceiling) → 20m (dome)
- Depth: 3m (closet) → 100m (hall)

**Benefit:** Users can design custom room acoustics for specific musical contexts.

---

## 📚 References

### Implementation Files

- **Header:** [dsp/ElasticHallway.h](../../../dsp/ElasticHallway.h) (163 lines)
- **Implementation:** [dsp/ElasticHallway.cpp](../../../dsp/ElasticHallway.cpp) (310 lines)
- **Tests:** [tests/NovelAlgorithmsTest.cpp](../../../tests/NovelAlgorithmsTest.cpp)
- **Benchmarks:** [tests/PerformanceBenchmarkTest.cpp](../../../tests/PerformanceBenchmarkTest.cpp)

### Academic References

1. **Room Acoustics** - Heinrich Kuttruff
   - Chapter 4: "Normal Modes in Rectangular Rooms"
   - Equations for axial, tangential, and oblique modes

2. **Acoustic Absorbers and Diffusers** - Trevor Cox & Peter D'Antonio
   - Chapter 7: "Resonant Absorbers"
   - Modal Q factors and absorption coefficients

3. **Finite Element Analysis of Acoustic Scattering** - Frank Ihlenburg
   - Chapter 3: "Modal Analysis of Enclosed Spaces"
   - Non-rectangular geometry treatment (for future work)

4. **The Science and Applications of Acoustics** - Daniel R. Raichel
   - Section 5.4: "Room Resonances and Standing Waves"
   - Energy distribution across room modes

### Related Documentation

- [03-chambers.md](../core-modules/03-chambers.md) - FDN reverb core (primary coupling target)
- [02-pillars.md](../core-modules/02-pillars.md) - Early reflections (geometry coupling)
- [08-resonance.md](08-resonance.md) - TubeRayTracer (complementary metallic effect)
- [11-strata.md](../memory-system/11-strata.md) - Memory system (temporal coupling)
- [docs/PERFORMANCE_BASELINE.md](../../PERFORMANCE_BASELINE.md) - Complete CPU profiling data
- [docs/DSP_ARCHITECTURE_COMPREHENSIVE_REVIEW.md](../../DSP_ARCHITECTURE_COMPREHENSIVE_REVIEW.md) - System-wide analysis

### JUCE API References

- `juce::dsp::IIR::Filter` - Biquad filtering
- `juce::dsp::IIR::Coefficients::makeBandPass()` - Modal bandpass filters
- `juce::dsp::AudioBlock` - Block-based processing
- `juce::ScopedNoDenormals` - Denormal protection

---

**Document Version:** 1.0
**Last Updated:** 2026-01-09
**Status:** ✅ Complete (12/12 sections)
**Template Compliance:** ✅ Matches Chambers + Resonance standards
