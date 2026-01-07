#include "ComponentPack.h"

namespace monument::playground
{
namespace
{
bool hasKey(const juce::var& object, const juce::Identifier& key)
{
    if (!object.isObject())
        return false;

    return object.getDynamicObject()->hasProperty(key);
}

juce::String getStringOrDefault(const juce::var& object, const juce::Identifier& key,
                                const juce::String& fallback)
{
    if (!hasKey(object, key))
        return fallback;

    return object.getProperty(key, fallback).toString();
}

float getFloatOrDefault(const juce::var& object, const juce::Identifier& key, float fallback)
{
    if (!hasKey(object, key))
        return fallback;

    return static_cast<float>(object.getProperty(key, fallback));
}

bool getBoolOrDefault(const juce::var& object, const juce::Identifier& key, bool fallback)
{
    if (!hasKey(object, key))
        return fallback;

    return static_cast<bool>(object.getProperty(key, fallback));
}
}

ComponentPack::BlendMode ComponentPack::parseBlendMode(const juce::String& value)
{
    if (value == "add")
        return BlendMode::Add;
    if (value == "screen")
        return BlendMode::Screen;
    if (value == "multiply")
        return BlendMode::Multiply;

    return BlendMode::Normal;
}

bool ComponentPack::loadFromManifest(const juce::File& manifestFile, juce::String& error)
{
    error.clear();
    layers.clear();

    if (!manifestFile.existsAsFile())
    {
        error = "Manifest file does not exist.";
        return false;
    }

    const auto manifestText = manifestFile.loadFileAsString();
    const auto parsed = juce::JSON::parse(manifestText);

    if (parsed.isVoid())
    {
        error = "Failed to parse manifest JSON.";
        return false;
    }

    rootDirectory = manifestFile.getParentDirectory();

    logicalSize = static_cast<int>(getFloatOrDefault(parsed, "logicalSize", 512.0f));

    if (hasKey(parsed, "pivot"))
    {
        const auto pivotVar = parsed.getProperty("pivot", {});
        if (pivotVar.isObject())
        {
            const auto pivotX = getFloatOrDefault(pivotVar, "x", 0.5f);
            const auto pivotY = getFloatOrDefault(pivotVar, "y", 0.5f);
            pivot = {pivotX, pivotY};
        }
    }

    if (!hasKey(parsed, "layers"))
    {
        error = "Manifest is missing 'layers'.";
        return false;
    }

    const auto layersVar = parsed.getProperty("layers", {});
    if (!layersVar.isArray())
    {
        error = "Manifest 'layers' must be an array.";
        return false;
    }

    const auto* array = layersVar.getArray();
    for (const auto& entry : *array)
    {
        if (!entry.isObject())
            continue;

        Layer layer;
        layer.name = getStringOrDefault(entry, "name", "layer");
        layer.file = getStringOrDefault(entry, "file", "");
        layer.opacity = getFloatOrDefault(entry, "opacity", 1.0f);
        layer.blend = parseBlendMode(getStringOrDefault(entry, "blend", "normal"));
        layer.rotates = getBoolOrDefault(entry, "rotates", false);
        layer.pulse = getBoolOrDefault(entry, "pulse", false);
        layer.glow = getBoolOrDefault(entry, "glow", false);
        layer.indicator = getBoolOrDefault(entry, "indicator", false);

        if (layer.file.isNotEmpty())
            layers.push_back(layer);
    }

    if (layers.empty())
    {
        error = "Manifest contains no layers.";
        return false;
    }

    return true;
}
}  // namespace monument::playground
