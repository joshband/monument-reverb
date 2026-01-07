# /gen-tests Command

Generate audio DSP test harness (impulse, sweep, null tests).

## Usage

```
/gen-tests <class-name>
/gen-tests <class-name> --framework catch2|gtest
```

## Output Files

```
tests/
├── AudioTestHarness.h
├── TestImpulse.cpp
├── TestSweep.cpp
└── TestNull.cpp
```

## AudioTestHarness.h

```cpp
#pragma once
#include <JuceHeader.h>
#include <cmath>

namespace audiotest {

inline juce::AudioBuffer<float> makeImpulse(int ch, int n) {
    juce::AudioBuffer<float> b(ch, n);
    b.clear();
    for (int c = 0; c < ch; ++c) b.setSample(c, 0, 1.0f);
    return b;
}

inline juce::AudioBuffer<float> makeLogSweep(int ch, int n, double sr,
                                              double f0 = 20, double f1 = 20000) {
    juce::AudioBuffer<float> b(ch, n);
    double T = n / sr;
    double K = std::log(f1 / f0);
    double a = (2.0 * 3.14159265 * f0 * T) / K;
    
    for (int c = 0; c < ch; ++c) {
        float* x = b.getWritePointer(c);
        for (int i = 0; i < n; ++i) {
            double t = i / sr;
            x[i] = (float)std::sin(a * (std::exp(K * t / T) - 1.0));
        }
    }
    return b;
}

inline double rms(const juce::AudioBuffer<float>& b) {
    double acc = 0;
    int total = b.getNumChannels() * b.getNumSamples();
    for (int c = 0; c < b.getNumChannels(); ++c) {
        const float* x = b.getReadPointer(c);
        for (int i = 0; i < b.getNumSamples(); ++i)
            acc += x[i] * x[i];
    }
    return std::sqrt(acc / total);
}

inline double dbfs(double lin) {
    return lin > 0 ? 20.0 * std::log10(lin) : -200.0;
}

inline double diffRmsDb(const juce::AudioBuffer<float>& a,
                        const juce::AudioBuffer<float>& b) {
    juce::AudioBuffer<float> d(a.getNumChannels(), a.getNumSamples());
    for (int c = 0; c < d.getNumChannels(); ++c) {
        const float* xa = a.getReadPointer(c);
        const float* xb = b.getReadPointer(c);
        float* xd = d.getWritePointer(c);
        for (int i = 0; i < d.getNumSamples(); ++i)
            xd[i] = xa[i] - xb[i];
    }
    return dbfs(rms(d));
}

inline bool allFinite(const juce::AudioBuffer<float>& b) {
    for (int c = 0; c < b.getNumChannels(); ++c) {
        const float* x = b.getReadPointer(c);
        for (int i = 0; i < b.getNumSamples(); ++i)
            if (!std::isfinite(x[i])) return false;
    }
    return true;
}

} // namespace audiotest
```

## TestImpulse.cpp

```cpp
#include "AudioTestHarness.h"
// #include "YourProcessor.h"

int main() {
    const int srs[] = {44100, 48000, 96000};
    const int bss[] = {64, 256, 512};
    
    for (int sr : srs) {
        for (int bs : bss) {
            auto impulse = audiotest::makeImpulse(2, sr);  // 1 second
            
            // TODO: Process through your DSP
            // processor.prepareToPlay(sr, bs);
            // processOffline(processor, impulse, bs);
            
            if (!audiotest::allFinite(impulse)) {
                std::cerr << "FAIL: Non-finite output (SR=" << sr << " BS=" << bs << ")\n";
                return 1;
            }
            
            std::cout << "PASS: Impulse test SR=" << sr << " BS=" << bs << "\n";
        }
    }
    return 0;
}
```

## TestSweep.cpp

```cpp
#include "AudioTestHarness.h"

int main() {
    const int srs[] = {44100, 48000, 96000};
    
    for (int sr : srs) {
        auto sweep = audiotest::makeLogSweep(2, sr * 2, sr);  // 2 seconds
        
        // TODO: Process through your DSP
        
        if (!audiotest::allFinite(sweep)) {
            std::cerr << "FAIL: Non-finite output (SR=" << sr << ")\n";
            return 1;
        }
        
        double peakDb = audiotest::dbfs(sweep.getMagnitude(0, sweep.getNumSamples()));
        if (peakDb > 6.0) {
            std::cerr << "FAIL: Output too hot " << peakDb << " dB (SR=" << sr << ")\n";
            return 1;
        }
        
        std::cout << "PASS: Sweep test SR=" << sr << " peak=" << peakDb << " dB\n";
    }
    return 0;
}
```

## TestNull.cpp

```cpp
#include "AudioTestHarness.h"

int main() {
    const int srs[] = {44100, 48000, 96000};
    const int bss[] = {64, 256, 512};
    const double threshold = -120.0;  // dBFS
    
    for (int sr : srs) {
        for (int bs : bss) {
            // Generate test signal
            juce::AudioBuffer<float> input(2, sr * 2);
            juce::Random rng(123);
            for (int c = 0; c < 2; ++c) {
                float* x = input.getWritePointer(c);
                for (int i = 0; i < input.getNumSamples(); ++i)
                    x[i] = rng.nextFloat() * 2.0f - 1.0f;
            }
            
            juce::AudioBuffer<float> output = input;
            
            // TODO: Set processor to bypass/identity mode
            // processor.prepareToPlay(sr, bs);
            // processOffline(processor, output, bs);
            
            double errDb = audiotest::diffRmsDb(output, input);
            
            if (errDb > threshold) {
                std::cerr << "FAIL: Null error " << errDb << " dB (SR=" << sr << " BS=" << bs << ")\n";
                return 1;
            }
            
            std::cout << "PASS: Null test SR=" << sr << " BS=" << bs << " err=" << errDb << " dB\n";
        }
    }
    return 0;
}
```

## Build

```cmake
add_executable(TestImpulse tests/TestImpulse.cpp)
add_executable(TestSweep tests/TestSweep.cpp)
add_executable(TestNull tests/TestNull.cpp)

target_link_libraries(TestImpulse PRIVATE juce::juce_audio_basics)
target_link_libraries(TestSweep PRIVATE juce::juce_audio_basics)
target_link_libraries(TestNull PRIVATE juce::juce_audio_basics)
```
