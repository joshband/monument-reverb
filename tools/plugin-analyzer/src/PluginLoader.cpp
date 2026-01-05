/**
 * PluginLoader.cpp
 */

#include "PluginLoader.h"

namespace monument {
namespace tools {

PluginLoader::PluginLoader()
{
    // Register plugin formats manually (headless mode doesn't have addDefaultFormats)
#if JUCE_PLUGINHOST_VST3
    formatManager.addFormat(new juce::VST3PluginFormat());
#endif
#if JUCE_PLUGINHOST_AU && JUCE_MAC
    formatManager.addFormat(new juce::AudioUnitPluginFormat());
#endif
}

PluginLoader::~PluginLoader()
{
    if (pluginInstance)
    {
        pluginInstance->releaseResources();
        pluginInstance.reset();
    }
}

bool PluginLoader::loadPlugin(const juce::String& pluginPath)
{
    juce::File pluginFile(pluginPath);

    if (!pluginFile.exists())
    {
        DBG("Plugin file not found: " << pluginPath);
        return false;
    }

    // Try to load as a plugin description
    juce::OwnedArray<juce::PluginDescription> descriptions;

    for (int i = 0; i < formatManager.getNumFormats(); ++i)
    {
        auto* format = formatManager.getFormat(i);

        if (format->fileMightContainThisPluginType(pluginPath))
        {
            format->findAllTypesForFile(descriptions, pluginPath);

            if (descriptions.size() > 0)
            {
                // Use first description found
                auto* description = descriptions[0];

                juce::String errorMessage;
                pluginInstance = formatManager.createPluginInstance(
                    *description,
                    44100.0, // Temp sample rate, will be set in prepareToPlay
                    512,     // Temp block size
                    errorMessage);

                if (pluginInstance)
                {
                    DBG("Loaded plugin: " << pluginInstance->getName());
                    return true;
                }
                else
                {
                    DBG("Failed to instantiate plugin: " << errorMessage);
                    return false;
                }
            }
        }
    }

    DBG("No compatible plugin format found for: " << pluginPath);
    return false;
}

void PluginLoader::prepareToPlay(double sampleRate, int blockSize, int numChannels)
{
    if (!pluginInstance)
    {
        jassertfalse; // Must load plugin first
        return;
    }

    pluginInstance->setPlayConfigDetails(numChannels, numChannels, sampleRate, blockSize);
    pluginInstance->prepareToPlay(sampleRate, blockSize);
}

void PluginLoader::processBlock(juce::AudioBuffer<float>& buffer)
{
    if (!pluginInstance)
    {
        jassertfalse;
        return;
    }

    midiBuffer.clear();
    pluginInstance->processBlock(buffer, midiBuffer);
}

juce::String PluginLoader::getPluginName() const
{
    return pluginInstance ? pluginInstance->getName() : "No plugin loaded";
}

juce::String PluginLoader::getPluginManufacturer() const
{
    return pluginInstance ? pluginInstance->getPluginDescription().manufacturerName : "";
}

juce::String PluginLoader::getPluginVersion() const
{
    return pluginInstance ? pluginInstance->getPluginDescription().version : "";
}

} // namespace tools
} // namespace monument
