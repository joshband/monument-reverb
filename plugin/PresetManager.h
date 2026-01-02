#pragma once

#include <JuceHeader.h>

#include <array>

class PresetManager
{
public:
    struct PresetValues
    {
        float time = 0.5f;
        float mass = 0.5f;
        float density = 0.5f;
        float bloom = 0.5f;
        float gravity = 0.5f;
        float warp = 0.0f;
        float drift = 0.0f;
        float memory = 0.0f;
        float memoryDepth = 0.5f;
        float memoryDecay = 0.4f;
        float memoryDrift = 0.3f;
        float mix = 0.5f;
    };

    struct Preset
    {
        juce::String name;
        juce::String description;
        PresetValues values{};
    };

    explicit PresetManager(juce::AudioProcessorValueTreeState& apvts);

    int getNumFactoryPresets() const;
    juce::String getFactoryPresetName(int index) const;
    juce::String getFactoryPresetDescription(int index) const;
    bool loadFactoryPreset(int index);
    bool loadFactoryPresetByName(const juce::String& name);

    void saveUserPreset(const juce::String& name, const juce::String& description);
    void saveUserPreset(const juce::File& targetFile,
                        const juce::String& name,
                        const juce::String& description);
    bool loadUserPreset(const juce::File& sourceFile);

    juce::File getDefaultUserPresetDirectory() const;

    static constexpr size_t kNumFactoryPresets = 18;
    static const std::array<Preset, kNumFactoryPresets>& getFactoryPresets();

private:
    PresetValues captureCurrentValues() const;
    void applyPreset(const PresetValues& values);
    juce::File resolveUserPresetFile(const juce::File& targetFile, const juce::String& name) const;

    juce::AudioProcessorValueTreeState& parameters;
    static const std::array<Preset, kNumFactoryPresets> kFactoryPresets;
};
