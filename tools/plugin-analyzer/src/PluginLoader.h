/**
 * PluginLoader.h
 *
 * Dynamically loads VST3/AU/VST2 plugins using JUCE AudioPluginFormatManager.
 * Provides simple interface for loading, initializing, and processing audio.
 */

#pragma once

#include <JuceHeader.h>
#include <memory>
#include <string>

namespace monument {
namespace tools {

class PluginLoader
{
public:
    PluginLoader();
    ~PluginLoader();

    /**
     * Load a plugin from file path.
     * Supports VST3 (.vst3), AU (.component), VST2 (.vst)
     *
     * @param pluginPath Full path to plugin file
     * @return true if loaded successfully
     */
    bool loadPlugin(const juce::String& pluginPath);

    /**
     * Prepare the plugin for processing.
     * Must be called before processBlock().
     *
     * @param sampleRate Sample rate in Hz
     * @param blockSize Maximum samples per block
     * @param numChannels Number of audio channels (1=mono, 2=stereo)
     */
    void prepareToPlay(double sampleRate, int blockSize, int numChannels);

    /**
     * Process audio through the plugin.
     *
     * @param buffer Audio buffer to process in-place
     */
    void processBlock(juce::AudioBuffer<float>& buffer);

    /**
     * Get the loaded plugin instance.
     * Useful for parameter access, state save/load, etc.
     */
    juce::AudioPluginInstance* getPluginInstance() const { return pluginInstance.get(); }

    /**
     * Check if a plugin is currently loaded.
     */
    bool isLoaded() const { return pluginInstance != nullptr; }

    /**
     * Get plugin information.
     */
    juce::String getPluginName() const;
    juce::String getPluginManufacturer() const;
    juce::String getPluginVersion() const;

private:
    juce::AudioPluginFormatManager formatManager;
    std::unique_ptr<juce::AudioPluginInstance> pluginInstance;

    juce::MidiBuffer midiBuffer; // Empty MIDI buffer for processBlock

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginLoader)
};

} // namespace tools
} // namespace monument
