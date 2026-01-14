#pragma once

#include <JuceHeader.h>

#include <array>
#include <vector>

#include "dsp/ModulationMatrix.h"

class PresetManager
{
public:
    struct PresetValues
    {
        // Base parameters
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

        // Phase 2: Macro parameters (added so UI updates on preset load)
        float material = 0.5f;
        float topology = 0.5f;
        float viscosity = 0.5f;
        float evolution = 0.5f;
        float chaosIntensity = 0.0f;
        float elasticityDecay = 0.0f;

        // Phase 5: Ancient Monuments macros 7-10
        float patina = 0.5f;
        float abyss = 0.5f;
        float corona = 0.5f;
        float breath = 0.0f;

        // Phase 3: Modulation connections for "living" presets
        std::vector<monument::dsp::ModulationMatrix::Connection> modulationConnections;
    };

    struct Preset
    {
        juce::String name;
        juce::String description;
        PresetValues values{};
    };

    explicit PresetManager(juce::AudioProcessorValueTreeState& apvts,
                           monument::dsp::ModulationMatrix* modMatrix = nullptr);

    int getNumFactoryPresets() const;
    juce::String getFactoryPresetName(int index) const;
    juce::String getFactoryPresetDescription(int index) const;
    bool loadFactoryPreset(int index);
    bool loadFactoryPresetByName(const juce::String& name);

    // Phase 3: Get modulation connections from most recently loaded preset
    const std::vector<monument::dsp::ModulationMatrix::Connection>& getLastLoadedModulationConnections() const
    {
        return lastLoadedModulationConnections;
    }

    void saveUserPreset(const juce::String& name, const juce::String& description);
    void saveUserPreset(const juce::File& targetFile,
                        const juce::String& name,
                        const juce::String& description);
    bool loadUserPreset(const juce::File& sourceFile);

    juce::File getDefaultUserPresetDirectory() const;

    static constexpr size_t kNumFactoryPresets = 37;  // 18 original + 5 "Living" (Phase 3) + 5 Physical Modeling (Phase 5) + 9 "Living" (Phase 6/Task 3)
    static const std::array<Preset, kNumFactoryPresets>& getFactoryPresets();

private:
    PresetValues captureCurrentValues() const;
    void applyPreset(const PresetValues& values);
    juce::File resolveUserPresetFile(const juce::File& targetFile, const juce::String& name) const;

    juce::AudioProcessorValueTreeState& parameters;
    monument::dsp::ModulationMatrix* modulationMatrix;  // Phase 3: For capturing modulation state
    static const std::array<Preset, kNumFactoryPresets> kFactoryPresets;

    // Phase 3: Cache modulation connections from last loaded preset
    std::vector<monument::dsp::ModulationMatrix::Connection> lastLoadedModulationConnections;

    // Phase 3: Enum-to-string conversion helpers
    static juce::String sourceTypeToString(monument::dsp::ModulationMatrix::SourceType type);
    static juce::String destinationTypeToString(monument::dsp::ModulationMatrix::DestinationType type);
    static monument::dsp::ModulationMatrix::SourceType stringToSourceType(const juce::String& str);
    static monument::dsp::ModulationMatrix::DestinationType stringToDestinationType(const juce::String& str);
    static juce::String curveTypeToString(monument::dsp::ModulationMatrix::CurveType type);
    static monument::dsp::ModulationMatrix::CurveType stringToCurveType(const juce::String& str);
};
