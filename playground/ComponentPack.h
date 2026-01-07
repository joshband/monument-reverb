#pragma once

#include <JuceHeader.h>

#include <vector>

namespace monument::playground
{
class ComponentPack
{
public:
    enum class BlendMode
    {
        Normal,
        Add,
        Screen,
        Multiply
    };

    struct Layer
    {
        juce::String name;
        juce::String file;
        BlendMode blend{BlendMode::Normal};
        float opacity{1.0f};
        bool rotates{false};
        bool pulse{false};
        bool glow{false};
        bool indicator{false};
    };

    bool loadFromManifest(const juce::File& manifestFile, juce::String& error);

    int getLogicalSize() const { return logicalSize; }
    juce::Point<float> getPivot() const { return pivot; }
    const std::vector<Layer>& getLayers() const { return layers; }
    const juce::File& getRootDirectory() const { return rootDirectory; }

private:
    static BlendMode parseBlendMode(const juce::String& value);

    int logicalSize{512};
    juce::Point<float> pivot{0.5f, 0.5f};
    juce::File rootDirectory;
    std::vector<Layer> layers;
};
}  // namespace monument::playground
