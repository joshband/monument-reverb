/**
 * Monument Plugin Analyzer
 *
 * Command-line tool for analyzing VST3/AU audio plugins.
 * Captures impulse responses, frequency responses, and other metrics.
 *
 * Usage:
 *   monument_plugin_analyzer --plugin /path/to/plugin.vst3 [options]
 *
 * Options:
 *   --plugin <path>         Path to VST3/AU plugin
 *   --output <dir>          Output directory (default: ./test-results)
 *   --test <type>           Test type: impulse, sweep, noise (default: impulse)
 *   --duration <seconds>    Test duration in seconds (default: 5.0)
 *   --samplerate <hz>       Sample rate (default: 48000)
 *   --channels <num>        Number of channels (default: 2)
 */

#include <JuceHeader.h>
#include <iostream>
#include "PluginLoader.h"
#include "TestSignalGenerator.h"
#include "AudioCapture.h"

using namespace monument::tools;

struct AnalyzerConfig
{
    juce::String pluginPath;
    juce::String outputDir = "./test-results";
    SignalType testType = SignalType::Impulse;
    double duration = 5.0;
    double sampleRate = 48000.0;
    int numChannels = 2;
    int blockSize = 512;
    int presetIndex = -1;  // -1 = don't change preset, use plugin default
    bool runAnalysis = false;  // Run Python analysis scripts after capture
};

void printUsage()
{
    std::cout << "Monument Plugin Analyzer\n";
    std::cout << "=========================\n\n";
    std::cout << "Usage:\n";
    std::cout << "  monument_plugin_analyzer --plugin <path> [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --plugin <path>         Path to VST3/AU plugin (required)\n";
    std::cout << "  --output <dir>          Output directory (default: ./test-results)\n";
    std::cout << "  --preset <index>        Factory preset index (0-based)\n";
    std::cout << "  --test <type>           Test type: impulse|sweep|noise (default: impulse)\n";
    std::cout << "  --duration <seconds>    Test duration (default: 5.0)\n";
    std::cout << "  --samplerate <hz>       Sample rate (default: 48000)\n";
    std::cout << "  --channels <num>        Number of channels (default: 2)\n";
    std::cout << "  --analyze               Run Python analysis (RT60 + frequency) after capture\n\n";
    std::cout << "Examples:\n";
    std::cout << "  # Capture impulse response from Monument\n";
    std::cout << "  monument_plugin_analyzer --plugin ./build/Monument_artefacts/VST3/Monument.vst3\n\n";
    std::cout << "  # Capture preset 7 with analysis\n";
    std::cout << "  monument_plugin_analyzer --plugin Monument.vst3 --preset 7 --analyze\n\n";
    std::cout << "  # Capture with custom duration\n";
    std::cout << "  monument_plugin_analyzer --plugin Monument.vst3 --preset 7 --duration 10\n";
}

bool parseArguments(const juce::ArgumentList& args, AnalyzerConfig& config)
{
    // Debug: print all arguments
    DBG("Total arguments: " << args.size());
    for (int i = 0; i < args.size(); ++i)
    {
        DBG("  Arg[" << i << "]: " << args[i].text);
    }
    DBG("Contains --plugin? " << (args.containsOption("--plugin") ? "YES" : "NO"));

    if (args.containsOption("--help") || args.containsOption("-h"))
    {
        printUsage();
        return false;
    }

    // Required: plugin path
    // Note: getValueForOption doesn't work as expected, so parse manually
    bool foundPlugin = false;
    for (int i = 0; i < args.size() - 1; ++i)
    {
        if (args[i].text == "--plugin")
        {
            config.pluginPath = args[i + 1].text;
            foundPlugin = true;
            break;
        }
    }

    if (!foundPlugin)
    {
        std::cerr << "Error: --plugin option is required\n\n";
        printUsage();
        return false;
    }

    DBG("Parsed plugin path: " << config.pluginPath);

    // Optional arguments - parse manually
    for (int i = 0; i < args.size(); ++i)
    {
        if (args[i].text == "--analyze")
        {
            config.runAnalysis = true;
        }
        else if (i < args.size() - 1)  // Need next arg for value
        {
            if (args[i].text == "--output")
                config.outputDir = args[i + 1].text;
            else if (args[i].text == "--test")
            {
                juce::String testStr = args[i + 1].text.toLowerCase();
                if (testStr == "impulse")
                    config.testType = SignalType::Impulse;
                else if (testStr == "sweep")
                    config.testType = SignalType::SineSweep;
                else if (testStr == "noise" || testStr == "white")
                    config.testType = SignalType::WhiteNoise;
                else if (testStr == "pink")
                    config.testType = SignalType::PinkNoise;
                else
                {
                    std::cerr << "Error: Unknown test type '" << testStr.toStdString() << "'\n";
                    return false;
                }
            }
            else if (args[i].text == "--duration")
                config.duration = args[i + 1].text.getDoubleValue();
            else if (args[i].text == "--samplerate")
                config.sampleRate = args[i + 1].text.getDoubleValue();
            else if (args[i].text == "--channels")
                config.numChannels = args[i + 1].text.getIntValue();
            else if (args[i].text == "--preset")
                config.presetIndex = args[i + 1].text.getIntValue();
        }
    }

    return true;
}

bool runPythonAnalysis(const juce::File& wetFile, const juce::File& outputDir)
{
    std::cout << "\n▸ Running Python analysis...\n";

    // Get project root (assuming we're running from build/ directory)
    juce::File projectRoot = juce::File::getCurrentWorkingDirectory().getParentDirectory();

    // RT60 Analysis
    std::cout << "  • RT60 analysis...";
    std::flush(std::cout);

    juce::String rt60Script = projectRoot.getChildFile("tools/plugin-analyzer/python/rt60_analysis_robust.py").getFullPathName();
    juce::String rt60Cmd = "python3 \"" + rt60Script + "\" "
                         + "\"" + wetFile.getFullPathName() + "\" "
                         + "--output \"" + outputDir.getFullPathName() + "\"";

    int rt60Result = std::system(rt60Cmd.toRawUTF8());
    if (rt60Result != 0)
    {
        std::cerr << " ✗ Failed (exit code " << rt60Result << ")\n";
        return false;
    }
    std::cout << " ✓\n";

    // Frequency Response Analysis
    std::cout << "  • Frequency response...";
    std::flush(std::cout);

    juce::String freqScript = projectRoot.getChildFile("tools/plugin-analyzer/python/frequency_response.py").getFullPathName();
    juce::String freqCmd = "python3 \"" + freqScript + "\" "
                         + "\"" + wetFile.getFullPathName() + "\" "
                         + "--output \"" + outputDir.getFullPathName() + "\"";

    int freqResult = std::system(freqCmd.toRawUTF8());
    if (freqResult != 0)
    {
        std::cerr << " ✗ Failed (exit code " << freqResult << ")\n";
        return false;
    }
    std::cout << " ✓\n";

    // Check that output files were created
    juce::File rt60File = outputDir.getChildFile("rt60_metrics.json");
    juce::File freqFile = outputDir.getChildFile("frequency_response.json");

    if (!rt60File.existsAsFile())
    {
        std::cerr << "  ✗ RT60 metrics file not created\n";
        return false;
    }

    if (!freqFile.existsAsFile())
    {
        std::cerr << "  ✗ Frequency response file not created\n";
        return false;
    }

    std::cout << "  ✓ Generated: rt60_metrics.json\n";
    std::cout << "  ✓ Generated: frequency_response.json\n";

    return true;
}

int runAnalysis(const AnalyzerConfig& config)
{
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "Monument Plugin Analyzer\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

    // 1. Load plugin
    std::cout << "▸ Loading plugin...\n";
    std::cout << "  Path: " << config.pluginPath.toStdString() << "\n";

    // Verify file exists
    juce::File pluginFile(config.pluginPath);
    if (!pluginFile.exists())
    {
        std::cerr << "  ✗ Plugin file not found: " << config.pluginPath.toStdString() << "\n";
        std::cerr << "  ✗ Absolute path: " << pluginFile.getFullPathName().toStdString() << "\n";
        return 1;
    }
    std::cout << "  ✓ File exists: " << pluginFile.getFullPathName().toStdString() << "\n";

    PluginLoader loader;
    if (!loader.loadPlugin(config.pluginPath))
    {
        std::cerr << "\n✗ Failed to load plugin\n";
        return 1;
    }

    std::cout << "  ✓ Loaded: " << loader.getPluginName().toStdString() << "\n";
    std::cout << "    Manufacturer: " << loader.getPluginManufacturer().toStdString() << "\n";
    std::cout << "    Version: " << loader.getPluginVersion().toStdString() << "\n";

    // 2. Prepare plugin
    std::cout << "\n▸ Preparing plugin...\n";
    std::cout << "  Sample rate: " << config.sampleRate << " Hz\n";
    std::cout << "  Channels: " << config.numChannels << "\n";
    std::cout << "  Block size: " << config.blockSize << "\n";

    loader.prepareToPlay(config.sampleRate, config.blockSize, config.numChannels);

    // Load factory preset if specified
    auto* plugin = loader.getPluginInstance();
    if (plugin && config.presetIndex >= 0)
    {
        int numPresets = plugin->getNumPrograms();
        if (config.presetIndex < numPresets)
        {
            plugin->setCurrentProgram(config.presetIndex);
            juce::String presetName = plugin->getProgramName(config.presetIndex);
            std::cout << "  ✓ Loaded preset " << config.presetIndex << ": "
                     << presetName.toStdString() << "\n";
        }
        else
        {
            std::cerr << "  ✗ Warning: Preset index " << config.presetIndex
                     << " out of range (0-" << (numPresets-1) << ")\n";
        }
    }

    // Set mix to 100% for wet signal capture
    if (plugin)
    {
        for (int i = 0; i < plugin->getParameters().size(); ++i)
        {
            auto* param = plugin->getParameters()[i];
            if (param->getName(32).toLowerCase().contains("mix"))
            {
                param->setValue(1.0f); // 100%
                std::cout << "  ✓ Set Mix parameter to 100%\n";
                break;
            }
        }
    }

    // 3. Generate test signal
    std::cout << "\n▸ Generating test signal...\n";

    const char* testTypeNames[] = {"Impulse", "Sine Sweep", "White Noise", "Pink Noise"};
    std::cout << "  Type: " << testTypeNames[static_cast<int>(config.testType)] << "\n";
    std::cout << "  Duration: " << config.duration << " seconds\n";

    auto inputSignal = TestSignalGenerator::generate(
        config.testType,
        config.duration,
        config.sampleRate,
        config.numChannels);

    std::cout << "  ✓ Generated " << inputSignal.getNumSamples() << " samples\n";

    // 4. Process through plugin (block by block)
    std::cout << "\n▸ Processing audio...\n";

    AudioCapture dryCapture, wetCapture;

    dryCapture.startCapture(config.sampleRate, config.numChannels, config.duration);
    wetCapture.startCapture(config.sampleRate, config.numChannels, config.duration);

    int totalSamples = inputSignal.getNumSamples();
    int samplesProcessed = 0;

    while (samplesProcessed < totalSamples)
    {
        int samplesToProcess = juce::jmin(config.blockSize, totalSamples - samplesProcessed);

        // Create buffer view for this block
        juce::AudioBuffer<float> blockBuffer(config.numChannels, samplesToProcess);

        for (int ch = 0; ch < config.numChannels; ++ch)
        {
            blockBuffer.copyFrom(ch, 0, inputSignal, ch, samplesProcessed, samplesToProcess);
        }

        // Capture dry signal
        dryCapture.appendAudio(blockBuffer);

        // Process through plugin
        loader.processBlock(blockBuffer);

        // Capture wet signal
        wetCapture.appendAudio(blockBuffer);

        samplesProcessed += samplesToProcess;

        // Progress indicator
        int percent = (samplesProcessed * 100) / totalSamples;
        if (percent % 10 == 0 && samplesProcessed > 0)
        {
            std::cout << "  Processing: " << percent << "%\r" << std::flush;
        }
    }

    dryCapture.stopCapture();
    wetCapture.stopCapture();

    std::cout << "  ✓ Processed " << samplesProcessed << " samples\n";

    // 5. Export WAV files
    std::cout << "\n▸ Exporting audio files...\n";

    juce::File outputFolder(config.outputDir);
    outputFolder.createDirectory();

    juce::String dryPath = outputFolder.getChildFile("dry.wav").getFullPathName();
    juce::String wetPath = outputFolder.getChildFile("wet.wav").getFullPathName();

    if (!dryCapture.exportToWav(dryPath, 24))
    {
        std::cerr << "  ✗ Failed to export dry signal\n";
        return 1;
    }
    std::cout << "  ✓ Dry: " << dryPath.toStdString() << "\n";

    if (!wetCapture.exportToWav(wetPath, 24))
    {
        std::cerr << "  ✗ Failed to export wet signal\n";
        return 1;
    }
    std::cout << "  ✓ Wet: " << wetPath.toStdString() << "\n";

    // 6. Run Python Analysis (if requested)
    bool analysisSuccess = true;
    if (config.runAnalysis)
    {
        juce::File wetFile(wetPath);
        if (!runPythonAnalysis(wetFile, outputFolder))
        {
            std::cerr << "\n✗ Python analysis failed\n";
            analysisSuccess = false;
        }
    }

    // 7. Summary
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    if (config.runAnalysis && analysisSuccess)
    {
        std::cout << "✓ Capture and analysis complete!\n";
    }
    else if (config.runAnalysis && !analysisSuccess)
    {
        std::cout << "⚠ Capture complete, but analysis failed\n";
    }
    else
    {
        std::cout << "✓ Capture complete!\n";
    }
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";

    if (config.runAnalysis && analysisSuccess)
    {
        std::cout << "Generated files:\n";
        std::cout << "  • " << dryPath.toStdString() << "\n";
        std::cout << "  • " << wetPath.toStdString() << "\n";
        std::cout << "  • " << outputFolder.getChildFile("rt60_metrics.json").getFullPathName().toStdString() << "\n";
        std::cout << "  • " << outputFolder.getChildFile("frequency_response.json").getFullPathName().toStdString() << "\n\n";
    }
    else if (!config.runAnalysis)
    {
        std::cout << "Next steps:\n";
        std::cout << "  • Listen to dry.wav and wet.wav to verify processing\n";
        std::cout << "  • Run with --analyze flag to generate RT60 and frequency metrics\n";
        std::cout << "  • Or manually run: python3 tools/plugin-analyzer/python/rt60_analysis_robust.py " << wetPath.toStdString() << "\n\n";
    }

    return (config.runAnalysis && !analysisSuccess) ? 1 : 0;
}

int main(int argc, char* argv[])
{
    // Initialize JUCE
    juce::ScopedJuceInitialiser_GUI juceInit;

    // Parse arguments
    juce::ArgumentList args(argc, argv);

    AnalyzerConfig config;
    if (!parseArguments(args, config))
        return 1;

    // Run analysis
    return runAnalysis(config);
}
