#pragma once

#include <JuceHeader.h>

#include <array>
#include <string>

class PresetManager
{
public:
    struct PresetValues
    {
        float time = 0.0f;
        float mass = 0.0f;
        float density = 0.0f;
        float bloom = 0.0f;
        float gravity = 0.0f;
        float warp = 0.0f;
        float drift = 0.0f;
        float mix = 0.5f;
        float air = 0.5f;
        float width = 0.5f;
        float freeze = 0.0f;
    };

    struct Preset
    {
        const char* name = "";
        PresetValues values{};
    };

    explicit PresetManager(juce::AudioProcessorValueTreeState& apvts);

    int getNumPresets() const;
    std::string getPresetName(int index) const;
    int findPresetIndex(const std::string& name) const;
    bool loadPreset(int index);
    bool loadPresetByName(const std::string& name);

    static constexpr size_t kNumPresets = 9;
    static const std::array<Preset, kNumPresets>& getPresets();

private:
    void applyPreset(const PresetValues& values);

    juce::AudioProcessorValueTreeState& parameters;
    static const std::array<Preset, kNumPresets> kPresets;
};
