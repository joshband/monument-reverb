# /bench-dsp Command

Generate a CPU benchmarking harness for JUCE DSP code.

## Usage

```
/bench-dsp <class-name>
/bench-dsp <class-name> --header <path>
```

## Output

Generates `bench/BenchDSP.cpp`:
- Sample rates: 44100, 48000, 96000
- Block sizes: 64, 128, 256, 512
- Warmup (3s) + measurement (5s)
- CSV output with timing + CPU%

## Generated Code

```cpp
#include <JuceHeader.h>
#include <chrono>
#include <fstream>
#include <iostream>

// TODO: #include "YourProcessor.h"

using Clock = std::chrono::steady_clock;

enum class InputSignal { Silence, Sine1k, WhiteNoise };

static void fillInput(juce::AudioBuffer<float>& buffer, double sr, InputSignal sig) {
    buffer.clear();
    if (sig == InputSignal::Sine1k) {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            float* x = buffer.getWritePointer(ch);
            double phase = 0, inc = 6.283185 * 1000.0 / sr;
            for (int i = 0; i < buffer.getNumSamples(); ++i) {
                x[i] = (float)std::sin(phase);
                phase += inc;
            }
        }
    } else if (sig == InputSignal::WhiteNoise) {
        juce::Random rng(12345);
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
            float* x = buffer.getWritePointer(ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                x[i] = rng.nextFloat() * 2.0f - 1.0f;
        }
    }
}

// TODO: Replace with your processor
static void processBlockUnderTest(juce::AudioBuffer<float>& buf, juce::MidiBuffer& midi) {
    juce::ignoreUnused(buf, midi);
}

struct Result { double avgUs, maxUs; int xruns; };

static Result runOne(int sr, int bs, int ch, InputSignal sig, double warmup, double measure) {
    juce::ScopedNoDenormals noDenormals;
    juce::AudioBuffer<float> buf(ch, bs);
    juce::MidiBuffer midi;
    
    int warmBlocks = (int)(warmup * sr / bs);
    int measBlocks = (int)(measure * sr / bs);
    
    for (int i = 0; i < warmBlocks; ++i) {
        fillInput(buf, sr, sig);
        processBlockUnderTest(buf, midi);
    }
    
    double total = 0, maxUs = 0;
    int xruns = 0;
    double budget = 1e6 * bs / sr;
    
    for (int i = 0; i < measBlocks; ++i) {
        fillInput(buf, sr, sig);
        auto t0 = Clock::now();
        processBlockUnderTest(buf, midi);
        auto t1 = Clock::now();
        double us = std::chrono::duration<double, std::micro>(t1 - t0).count();
        total += us;
        if (us > maxUs) maxUs = us;
        if (us > budget) ++xruns;
    }
    
    return {total / measBlocks, maxUs, xruns};
}

int main() {
    int srs[] = {44100, 48000, 96000};
    int bss[] = {64, 128, 256, 512};
    
    std::ofstream csv("bench_results.csv");
    csv << "sample_rate,block_size,avg_us,max_us,xruns,cpu_pct\n";
    
    for (int sr : srs) {
        for (int bs : bss) {
            auto r = runOne(sr, bs, 2, InputSignal::Sine1k, 3.0, 5.0);
            double cpu = r.avgUs / (1e6 * bs / sr) * 100;
            std::cout << "SR=" << sr << " BS=" << bs << " CPU=" << cpu << "%\n";
            csv << sr << "," << bs << "," << r.avgUs << "," << r.maxUs << "," << r.xruns << "," << cpu << "\n";
        }
    }
    return 0;
}
```

## Build

```bash
# Add to CMakeLists.txt
add_executable(BenchDSP bench/BenchDSP.cpp)
target_link_libraries(BenchDSP PRIVATE juce::juce_audio_basics juce::juce_dsp)

# Build and run
cmake --build build --target BenchDSP
./build/BenchDSP
```
