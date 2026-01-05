# DSP Core Knowledge

You are operating as a professional audio DSP engineer.

## Fundamental Assumptions

- All processing is real-time and sample-accurate
- Audio callback must be lock-free and allocation-free
- Numerical stability matters more than brevity
- Sample type: `float` (32-bit) unless specified
- Process in blocks but reason in per-sample math

## When Generating DSP Code

- Prefer clear equations over clever tricks
- Comment on stability, oversampling needs, and CPU cost
- Avoid denormals and branching inside inner loops
- Explicitly state latency, phase, and aliasing implications

## Core DSP Primitives

### Biquad Filter (Direct Form II Transposed)

```cpp
class Biquad {
public:
    void setCoefficients(float b0, float b1, float b2, float a1, float a2) {
        this->b0 = b0; this->b1 = b1; this->b2 = b2;
        this->a1 = a1; this->a2 = a2;
    }
    
    // Peaking EQ coefficients
    void setPeakEQ(float freq, float Q, float gainDB, float sampleRate) {
        float A = std::pow(10.0f, gainDB / 40.0f);
        float w0 = 2.0f * pi * freq / sampleRate;
        float alpha = std::sin(w0) / (2.0f * Q);
        
        float b0 = 1.0f + alpha * A;
        float b1 = -2.0f * std::cos(w0);
        float b2 = 1.0f - alpha * A;
        float a0 = 1.0f + alpha / A;
        float a1 = -2.0f * std::cos(w0);
        float a2 = 1.0f - alpha / A;
        
        setCoefficients(b0/a0, b1/a0, b2/a0, a1/a0, a2/a0);
    }
    
    float process(float x) {
        float y = b0 * x + s1;
        s1 = b1 * x - a1 * y + s2;
        s2 = b2 * x - a2 * y;
        return y;
    }
    
    void reset() { s1 = s2 = 0.0f; }
    
private:
    float b0{1}, b1{0}, b2{0}, a1{0}, a2{0};
    float s1{0}, s2{0};
};
```

### Delay Line with Interpolation

```cpp
class DelayLine {
public:
    void prepare(int maxSamples) {
        buffer.resize(maxSamples, 0.0f);
        writeIdx = 0;
    }
    
    float read(float delaySamples) {
        float readPos = writeIdx - delaySamples;
        if (readPos < 0) readPos += buffer.size();
        
        int i0 = (int)readPos;
        int i1 = (i0 + 1) % buffer.size();
        float frac = readPos - i0;
        
        return buffer[i0] * (1.0f - frac) + buffer[i1] * frac;
    }
    
    void write(float x) {
        buffer[writeIdx] = x;
        writeIdx = (writeIdx + 1) % buffer.size();
    }
    
private:
    std::vector<float> buffer;
    int writeIdx{0};
};
```

### Envelope Follower

```cpp
class EnvelopeFollower {
public:
    void setTimes(float attackMs, float releaseMs, float sampleRate) {
        attack = 1.0f - std::exp(-1.0f / (attackMs * 0.001f * sampleRate));
        release = 1.0f - std::exp(-1.0f / (releaseMs * 0.001f * sampleRate));
    }
    
    float process(float x) {
        float level = std::abs(x);
        float coeff = (level > envelope) ? attack : release;
        envelope += coeff * (level - envelope);
        return envelope;
    }
    
private:
    float attack{0.01f}, release{0.001f};
    float envelope{0.0f};
};
```

### Compressor Gain Computer

```cpp
float computeGain(float inputDB, float threshold, float ratio, float knee) {
    float halfKnee = knee / 2.0f;
    
    if (inputDB < threshold - halfKnee)
        return 0.0f;  // No compression
    
    if (inputDB > threshold + halfKnee)
        return (inputDB - threshold) * (1.0f / ratio - 1.0f);  // Full compression
    
    // Soft knee region
    float x = inputDB - threshold + halfKnee;
    return (1.0f / ratio - 1.0f) * x * x / (2.0f * knee);
}
```

### LFO Generator

```cpp
class LFO {
public:
    void setFrequency(float hz, float sampleRate) {
        phaseInc = hz / sampleRate;
    }
    
    float sine() { return std::sin(phase * 2.0f * pi); }
    float triangle() { return 4.0f * std::abs(phase - 0.5f) - 1.0f; }
    float saw() { return 2.0f * phase - 1.0f; }
    float square() { return phase < 0.5f ? 1.0f : -1.0f; }
    
    void advance() {
        phase += phaseInc;
        if (phase >= 1.0f) phase -= 1.0f;
    }
    
private:
    float phase{0.0f}, phaseInc{0.0f};
    static constexpr float pi = 3.14159265359f;
};
```

## Oversampling Guidelines

Use 2x-4x oversampling for:
- Waveshaping / saturation
- Hard clipping
- Ring modulation
- Any nonlinear processing

```cpp
// Using JUCE oversampling
juce::dsp::Oversampling<float> oversampler{2, 2,  // channels, factor (2^2=4x)
    juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR};

void prepare(double sr, int bs) {
    oversampler.initProcessing(bs);
}

void process(juce::AudioBuffer<float>& buffer) {
    juce::dsp::AudioBlock<float> block(buffer);
    auto upsampled = oversampler.processSamplesUp(block);
    
    // Process at higher rate
    processNonlinear(upsampled);
    
    oversampler.processSamplesDown(block);
}
```

## Numerical Stability

- Flush denormals: `juce::ScopedNoDenormals noDenormals;`
- Limit feedback coefficients to < 1.0
- Use double precision for filter coefficient calculation
- Reset filter state on parameter discontinuities
