#include "dsp/MemoryEchoes.h"

#include <JuceHeader.h>

#include <array>
#include <cmath>
#include <iostream>
#include <vector>

namespace
{
struct HarnessConfig
{
    juce::File inputFile;
    juce::File outputFile;
    double sampleRate = 48000.0;
    double seconds = 40.0;
    int blockSize = 256;
    float memory = 0.7f;
    float depth = 0.6f;
    float decay = 0.5f;
    float drift = 0.3f;
    int64_t seed = 0;
    bool seedSet = false;
    bool mixInput = false;
    bool sampleRateSet = false;
    juce::String signal = "pad";
};

void printUsage()
{
    std::cout << "Usage: monument_memory_echoes_harness [options]\n"
              << "  --input <path.wav>     Optional input wav (default: generated bursts)\n"
              << "  --output <path.wav>    Output wav (default: ./memory_echoes_render.wav)\n"
              << "  --seconds <float>      Duration when no input (default: 40)\n"
              << "  --sample-rate <float>  Sample rate (default: 48000)\n"
              << "  --block-size <int>     Block size (default: 256)\n"
              << "  --memory <float>       Memory amount 0-1 (default: 0.7)\n"
              << "  --depth <float>        Memory depth 0-1 (default: 0.6)\n"
              << "  --decay <float>        Memory decay 0-1 (default: 0.5)\n"
              << "  --drift <float>        Memory drift 0-1 (default: 0.3)\n"
              << "  --signal <pad|pluck|piano> Generated signal type (default: pad)\n"
              << "  --seed <int>           Optional RNG seed\n"
              << "  --mix-input            Mix original input into output\n"
              << "  --help                 Show this help\n";
}

bool parseArgs(int argc, char** argv, HarnessConfig& config)
{
    if (argc <= 1)
        return true;

    for (int i = 1; i < argc; ++i)
    {
        const juce::String arg(argv[i]);
        auto requireValue = [&](const char* label) -> juce::String
        {
            if (i + 1 >= argc)
            {
                std::cerr << "Missing value for " << label << "\n";
                std::exit(1);
            }
            return juce::String(argv[++i]);
        };

        if (arg == "--help" || arg == "-h")
        {
            printUsage();
            std::exit(0);
        }
        else if (arg == "--input")
        {
            config.inputFile = juce::File(requireValue("--input"));
        }
        else if (arg == "--output")
        {
            config.outputFile = juce::File(requireValue("--output"));
        }
        else if (arg == "--seconds")
        {
            config.seconds = requireValue("--seconds").getDoubleValue();
        }
        else if (arg == "--sample-rate")
        {
            config.sampleRate = requireValue("--sample-rate").getDoubleValue();
            config.sampleRateSet = true;
        }
        else if (arg == "--block-size")
        {
            config.blockSize = requireValue("--block-size").getIntValue();
        }
        else if (arg == "--memory")
        {
            config.memory = static_cast<float>(requireValue("--memory").getDoubleValue());
        }
        else if (arg == "--depth")
        {
            config.depth = static_cast<float>(requireValue("--depth").getDoubleValue());
        }
        else if (arg == "--decay")
        {
            config.decay = static_cast<float>(requireValue("--decay").getDoubleValue());
        }
        else if (arg == "--drift")
        {
            config.drift = static_cast<float>(requireValue("--drift").getDoubleValue());
        }
        else if (arg == "--signal")
        {
            config.signal = requireValue("--signal").toLowerCase();
            if (config.signal != "pad" && config.signal != "pluck" && config.signal != "piano")
            {
                std::cerr << "Unknown signal type: " << config.signal << "\n";
                return false;
            }
        }
        else if (arg == "--seed")
        {
            config.seed = requireValue("--seed").getLargeIntValue();
            config.seedSet = true;
        }
        else if (arg == "--mix-input")
        {
            config.mixInput = true;
        }
        else
        {
            std::cerr << "Unknown argument: " << arg << "\n";
            return false;
        }
    }

    return true;
}

void generatePadSignal(juce::AudioBuffer<float>& buffer, double sampleRate)
{
    const int totalSamples = buffer.getNumSamples();
    const int channels = buffer.getNumChannels();
    buffer.clear();

    const double burstSeconds = 1.6;
    const int burstSamples = static_cast<int>(std::round(sampleRate * burstSeconds));
    const int gapSamples = static_cast<int>(std::round(sampleRate * 3.2));
    const std::array<int, 2> starts{0, gapSamples};

    juce::Random random(0xdeadbeef);
    for (size_t burst = 0; burst < starts.size(); ++burst)
    {
        const int start = starts[burst];
        if (start >= totalSamples)
            continue;
        const int length = juce::jmin(burstSamples, totalSamples - start);
        const double baseFreq = burst == 0 ? 220.0 : 330.0;
        const double altFreq = burst == 0 ? 277.18 : 392.0;

        for (int sample = 0; sample < length; ++sample)
        {
            const double t = static_cast<double>(sample) / sampleRate;
            const float env = std::exp(-static_cast<float>(t) / 0.7f);
            const float tone = static_cast<float>(
                std::sin(juce::MathConstants<double>::twoPi * baseFreq * t))
                + 0.7f * static_cast<float>(
                    std::sin(juce::MathConstants<double>::twoPi * altFreq * t));
            const float noise = (random.nextFloat() * 2.0f - 1.0f) * 0.08f;
            const float sampleValue = (0.35f * tone + noise) * env;
            for (int channel = 0; channel < channels; ++channel)
                buffer.setSample(channel, start + sample, sampleValue);
        }
    }
}

void generatePluckSignal(juce::AudioBuffer<float>& buffer, double sampleRate)
{
    const int totalSamples = buffer.getNumSamples();
    const int channels = buffer.getNumChannels();
    buffer.clear();

    const double baseFreq = 220.0;
    const double altFreq = 293.66;
    const double seconds = static_cast<double>(totalSamples) / sampleRate;
    const int burstSamples = static_cast<int>(std::round(sampleRate * 1.4));
    const int gapSamples = static_cast<int>(std::round(sampleRate * 3.0));
    const std::array<int, 2> starts{0, gapSamples};

    for (size_t burst = 0; burst < starts.size(); ++burst)
    {
        const int start = starts[burst];
        if (start >= totalSamples)
            continue;
        const int length = juce::jmin(burstSamples, totalSamples - start);
        const double freq = burst == 0 ? baseFreq : altFreq;
        const int delaySamples = juce::jmax(2, static_cast<int>(std::round(sampleRate / freq)));
        std::vector<float> delay(static_cast<size_t>(delaySamples), 0.0f);
        juce::Random random(0x1234567 + static_cast<int>(burst));
        for (int i = 0; i < delaySamples; ++i)
            delay[static_cast<size_t>(i)] = (random.nextFloat() * 2.0f - 1.0f) * 0.5f;
        int delayIndex = 0;

        for (int sample = 0; sample < length; ++sample)
        {
            const float current = delay[static_cast<size_t>(delayIndex)];
            const int nextIndex = (delayIndex + 1) % delaySamples;
            const float next = delay[static_cast<size_t>(nextIndex)];
            const float averaged = 0.5f * (current + next);
            delay[static_cast<size_t>(delayIndex)] = averaged * 0.996f;
            delayIndex = nextIndex;

            const double t = static_cast<double>(sample) / sampleRate;
            const float env = std::exp(-static_cast<float>(t) / 1.4f);
            const float value = current * env;

            for (int channel = 0; channel < channels; ++channel)
                buffer.setSample(channel, start + sample, value);
        }
    }
    juce::ignoreUnused(seconds);
}

void generatePianoSignal(juce::AudioBuffer<float>& buffer, double sampleRate)
{
    const int totalSamples = buffer.getNumSamples();
    const int channels = buffer.getNumChannels();
    buffer.clear();

    struct Note
    {
        double startSec;
        double freq;
    };

    const std::array<Note, 2> notes{{
        {0.0, 220.0},
        {3.2, 329.63},
    }};

    for (const auto& note : notes)
    {
        const int start = static_cast<int>(std::round(note.startSec * sampleRate));
        if (start >= totalSamples)
            continue;

        const int length = juce::jmin(
            static_cast<int>(std::round(sampleRate * 3.5)), totalSamples - start);

        for (int sample = 0; sample < length; ++sample)
        {
            const double t = static_cast<double>(sample) / sampleRate;
            const float attack = t < 0.01 ? static_cast<float>(t / 0.01) : 1.0f;
            const float env = attack * std::exp(-static_cast<float>(t) / 1.6f);

            const double base = note.freq;
            float value = 0.0f;
            value += static_cast<float>(std::sin(juce::MathConstants<double>::twoPi * base * t)) * 0.6f;
            value += static_cast<float>(std::sin(juce::MathConstants<double>::twoPi * base * 2.0 * t)) * 0.3f;
            value += static_cast<float>(std::sin(juce::MathConstants<double>::twoPi * base * 3.0 * t)) * 0.15f;
            value += static_cast<float>(std::sin(juce::MathConstants<double>::twoPi * base * 4.0 * t)) * 0.08f;

            value *= env;

            for (int channel = 0; channel < channels; ++channel)
                buffer.setSample(channel, start + sample, value);
        }
    }
}
} // namespace

int main(int argc, char** argv)
{
    using monument::dsp::MemoryEchoes;

    HarnessConfig config;
    if (!parseArgs(argc, argv, config))
    {
        printUsage();
        return 1;
    }

    if (!config.outputFile.existsAsFile())
    {
        if (config.outputFile.getFullPathName().isEmpty())
            config.outputFile = juce::File::getCurrentWorkingDirectory()
                .getChildFile("memory_echoes_render.wav");
    }

    constexpr int channels = 2;
    juce::AudioBuffer<float> input;
    int totalSamples = 0;

    if (config.inputFile.existsAsFile())
    {
        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();
        std::unique_ptr<juce::AudioFormatReader> reader(
            formatManager.createReaderFor(config.inputFile));
        if (!reader)
        {
            std::cerr << "Failed to read input file: "
                      << config.inputFile.getFullPathName() << "\n";
            return 1;
        }

        if (!config.sampleRateSet)
            config.sampleRate = reader->sampleRate;
        else if (std::abs(reader->sampleRate - config.sampleRate) > 1.0)
        {
            std::cerr << "Input sample rate mismatch. File is "
                      << reader->sampleRate << " Hz.\n";
            return 1;
        }

        totalSamples = static_cast<int>(reader->lengthInSamples);
        const int samplesLimit = config.seconds > 0.0
            ? static_cast<int>(std::round(config.seconds * config.sampleRate))
            : totalSamples;
        totalSamples = juce::jmin(totalSamples, samplesLimit);
        input.setSize(channels, totalSamples);
        input.clear();

        float* dest[channels] = { input.getWritePointer(0), input.getWritePointer(1) };
        reader->read(dest, channels, 0, totalSamples);

        if (reader->numChannels == 1)
            input.copyFrom(1, 0, input, 0, 0, totalSamples);
    }
    else
    {
        totalSamples = static_cast<int>(std::round(config.sampleRate * config.seconds));
        if (totalSamples <= 0)
        {
            std::cerr << "Invalid duration.\n";
            return 1;
        }
        input.setSize(channels, totalSamples);
        if (config.signal == "pluck")
            generatePluckSignal(input, config.sampleRate);
        else if (config.signal == "piano")
            generatePianoSignal(input, config.sampleRate);
        else
            generatePadSignal(input, config.sampleRate);
    }

    if (config.blockSize <= 0)
    {
        std::cerr << "Invalid block size.\n";
        return 1;
    }

    MemoryEchoes memory;
    memory.prepare(config.sampleRate, config.blockSize, channels);
    memory.reset();
    memory.setMemory(config.memory);
    memory.setDepth(config.depth);
    memory.setDecay(config.decay);
    memory.setDrift(config.drift);
    memory.setFreeze(false);
    memory.setInjectToBuffer(false);

#if defined(MONUMENT_TESTING)
    if (config.seedSet)
        memory.setRandomSeed(config.seed);
#endif

    juce::AudioBuffer<float> block(channels, config.blockSize);
    juce::AudioBuffer<float> output(channels, totalSamples);
    output.clear();

    const int totalBlocks = (totalSamples + config.blockSize - 1) / config.blockSize;
    for (int blockIndex = 0; blockIndex < totalBlocks; ++blockIndex)
    {
        const int startSample = blockIndex * config.blockSize;
        const int remaining = totalSamples - startSample;
        const int samplesThisBlock = juce::jmin(config.blockSize, remaining);
        block.clear();

        for (int channel = 0; channel < channels; ++channel)
        {
            block.copyFrom(channel, 0, input, channel, startSample, samplesThisBlock);
        }

        memory.process(block);
        memory.captureWet(block);

        const auto& recall = memory.getRecallBuffer();
        for (int channel = 0; channel < channels; ++channel)
        {
            const auto* recallData = recall.getReadPointer(channel);
            auto* out = output.getWritePointer(channel) + startSample;
            for (int sample = 0; sample < samplesThisBlock; ++sample)
            {
                float value = recallData[sample];
                if (config.mixInput)
                    value += block.getSample(channel, sample);
                out[sample] = value;
            }
        }
    }

#if defined(MONUMENT_TESTING)
    float peak = 0.0f;
    float peakValue = 0.0f;
    int peakIndex = -1;
    int peakChannel = -1;
    double sumSq = 0.0;
    int count = 0;
    int firstNonZero = -1;
    float firstValue = 0.0f;
    for (int channel = 0; channel < channels; ++channel)
    {
        const auto* data = output.getReadPointer(channel);
        for (int sample = 0; sample < totalSamples; ++sample)
        {
            const float value = data[sample];
            if (firstNonZero < 0 && std::abs(value) > 1.0e-9f)
            {
                firstNonZero = sample;
                firstValue = value;
            }
            const float absValue = std::abs(value);
            if (absValue > peak)
            {
                peak = absValue;
                peakValue = value;
                peakIndex = sample;
                peakChannel = channel;
            }
            sumSq += static_cast<double>(value * value);
            ++count;
        }
    }
    const float rms = count > 0 ? static_cast<float>(std::sqrt(sumSq / count)) : 0.0f;
    std::cout << "Harness output peak=" << peak << " rms=" << rms
              << " peakIndex=" << peakIndex
              << " peakChannel=" << peakChannel
              << " peakValue=" << peakValue
              << " firstNonZero=" << firstNonZero
              << " firstValue=" << firstValue << "\n";
#endif

    juce::WavAudioFormat wav;
    std::unique_ptr<juce::FileOutputStream> fileStream(config.outputFile.createOutputStream());
    if (!fileStream)
    {
        std::cerr << "Failed to open output file: "
                  << config.outputFile.getFullPathName() << "\n";
        return 1;
    }

    std::unique_ptr<juce::OutputStream> outputStream(fileStream.release());
    const juce::AudioFormatWriterOptions options = juce::AudioFormatWriterOptions()
        .withSampleRate(config.sampleRate)
        .withNumChannels(channels)
        .withBitsPerSample(24);
    std::unique_ptr<juce::AudioFormatWriter> writer(
        wav.createWriterFor(outputStream, options));
    if (!writer)
    {
        std::cerr << "Failed to create WAV writer.\n";
        return 1;
    }

    const bool writeOk = writer->writeFromAudioSampleBuffer(output, 0, totalSamples);
    if (!writeOk)
        std::cerr << "Failed to write audio samples.\n";
    std::cout << "Wrote " << totalSamples << " samples to "
              << config.outputFile.getFullPathName() << "\n";
    return 0;
}
