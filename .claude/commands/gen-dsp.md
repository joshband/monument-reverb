# /gen-dsp Command

Generate a DSP algorithm suitable for real-time audio plugins.

## Usage

```
/gen-dsp <algorithm>
```

## Examples

```
/gen-dsp biquad-lowpass
/gen-dsp compressor
/gen-dsp delay-line
/gen-dsp chorus
/gen-dsp reverb-schroeder
/gen-dsp waveshaper-tanh
/gen-dsp envelope-follower
```

## Output Format

For each algorithm, generate:

### 1. Algorithm Description
Brief explanation of what it does and common use cases.

### 2. Mathematical Formulation
- Transfer function or difference equation
- Key parameters and their effects
- Frequency response characteristics (if applicable)

### 3. C++ Implementation
```cpp
class AlgorithmName {
public:
    // Configuration
    void setParameters(...);
    void prepare(double sampleRate);
    void reset();
    
    // Processing
    float processSample(float input);
    void processBlock(float* data, int numSamples);
    
private:
    // State variables
};
```

### 4. Usage Example
```cpp
// In prepareToPlay
algorithm.prepare(sampleRate);

// In processBlock
for (int i = 0; i < numSamples; ++i) {
    output[i] = algorithm.processSample(input[i]);
}
```

### 5. Notes
- **CPU cost**: Approximate operations per sample
- **Latency**: Samples of delay introduced
- **Aliasing**: Whether oversampling is recommended
- **Stability**: Conditions for stable operation
- **Tuning**: Tips for parameter ranges

## Supported Algorithms

### Filters
- `biquad-lowpass`, `biquad-highpass`, `biquad-bandpass`, `biquad-notch`, `biquad-peak`, `biquad-shelf`
- `svf` (state variable filter)
- `onepole-lowpass`, `onepole-highpass`
- `ladder` (Moog-style)
- `comb-feedforward`, `comb-feedback`
- `allpass`

### Dynamics
- `compressor`, `limiter`, `gate`, `expander`
- `envelope-follower`
- `transient-shaper`

### Time-Based
- `delay-line` (with interpolation options)
- `chorus`, `flanger`, `phaser`
- `vibrato`, `tremolo`
- `reverb-schroeder`, `reverb-fdn`

### Distortion
- `waveshaper-tanh`, `waveshaper-clip`, `waveshaper-fold`
- `tube-saturation`
- `bitcrusher`

### Modulation
- `lfo` (sine, tri, saw, square, s&h)
- `adsr-envelope`
- `ring-modulator`

### Analysis
- `peak-meter`, `rms-meter`
- `pitch-detector`
- `onset-detector`

## Code Requirements

All generated code must be:
- **Real-time safe**: No allocations, no locks
- **Self-contained**: Single .h/.cpp pair
- **JUCE-compatible**: Use JUCE types where helpful
- **Well-documented**: Comments on non-obvious operations
- **Tested patterns**: Based on proven implementations

## Example Output

For `/gen-dsp envelope-follower`:

```cpp
/**
 * Envelope Follower
 * 
 * Tracks the amplitude envelope of an audio signal using
 * separate attack and release time constants.
 * 
 * Math: y[n] = y[n-1] + coef * (|x[n]| - y[n-1])
 * where coef = attack_coef if rising, release_coef if falling
 */
class EnvelopeFollower {
public:
    void setTimes(float attackMs, float releaseMs, double sampleRate) {
        attackCoef = 1.0f - std::exp(-1.0f / (attackMs * 0.001f * sampleRate));
        releaseCoef = 1.0f - std::exp(-1.0f / (releaseMs * 0.001f * sampleRate));
    }
    
    void reset() { envelope = 0.0f; }
    
    float processSample(float input) {
        float level = std::abs(input);
        float coef = (level > envelope) ? attackCoef : releaseCoef;
        envelope += coef * (level - envelope);
        return envelope;
    }
    
    float getEnvelope() const { return envelope; }
    
private:
    float attackCoef{0.01f};
    float releaseCoef{0.001f};
    float envelope{0.0f};
};
```

**Notes:**
- CPU: ~5 ops/sample
- Latency: 0 samples
- Aliasing: N/A
- Stability: Always stable (coefficients 0-1)
- Tuning: Attack 0.1-50ms, Release 10-500ms typical
