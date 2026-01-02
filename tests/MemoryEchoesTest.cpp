#include "dsp/MemoryEchoes.h"

#include <JuceHeader.h>

#include <cmath>

int main()
{
    using monument::dsp::MemoryEchoes;

    constexpr double sampleRate = 48000.0;
    constexpr int blockSize = 256;
    constexpr int channels = 2;

    MemoryEchoes memory;
    memory.prepare(sampleRate, blockSize, channels);
    memory.reset();

#if defined(MONUMENT_TESTING)
    memory.setRandomSeed(0x12345678);
#endif

    memory.setMemory(1.0f);
    memory.setDepth(0.7f);
    memory.setDecay(0.6f);
    memory.setDrift(0.3f);
    memory.setFreeze(false);

    juce::AudioBuffer<float> buffer(channels, blockSize);

    // Prime capture with steady energy so buffers contain non-zero data.
    for (int block = 0; block < 8; ++block)
    {
        buffer.clear();
        for (int channel = 0; channel < channels; ++channel)
        {
            auto* data = buffer.getWritePointer(channel);
            for (int sample = 0; sample < blockSize; ++sample)
                data[sample] = 0.5f;
        }

        memory.process(buffer);
        memory.captureWet(buffer);
    }

    bool recalled = false;
    const int totalBlocks = static_cast<int>(std::ceil(8.0 * sampleRate / blockSize));

    for (int block = 0; block < totalBlocks && !recalled; ++block)
    {
        buffer.clear();
        memory.process(buffer);

        for (int channel = 0; channel < channels && !recalled; ++channel)
        {
            const auto* data = buffer.getReadPointer(channel);
            for (int sample = 0; sample < blockSize; ++sample)
            {
                if (std::abs(data[sample]) > 1.0e-5f)
                {
                    recalled = true;
                    break;
                }
            }
        }
    }

    return recalled ? 0 : 1;
}
