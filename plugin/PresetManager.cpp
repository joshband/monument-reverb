#include "PresetManager.h"

#include <algorithm>

namespace
{
void setParamNormalized(juce::AudioProcessorValueTreeState& apvts, const juce::String& id, float value)
{
    if (auto* param = apvts.getParameter(id))
        param->setValueNotifyingHost(juce::jlimit(0.0f, 1.0f, value));
}

PresetManager::PresetValues makePreset(float time,
    float mass,
    float density,
    float bloom,
    float gravity,
    float warp,
    float drift,
    float mix,
    float air,
    float width,
    float freeze)
{
    return {time, mass, density, bloom, gravity, warp, drift, mix, air, width, freeze};
}
} // namespace

const std::array<PresetManager::Preset, PresetManager::kNumPresets> PresetManager::kPresets{{
    // Long, shimmering reflections with gentle motion.
    {"Cathedral of Glass", makePreset(0.82f, 0.25f, 0.80f, 0.55f, 0.15f, 0.00f, 0.20f, 0.60f, 0.70f, 0.75f, 0.0f)},
    // Dark, swelling tail that bends around the source.
    {"Event Horizon", makePreset(1.00f, 0.85f, 0.55f, 0.85f, 0.50f, 0.30f, 0.50f, 0.70f, 0.35f, 0.65f, 0.0f)},
    // The topology folds on itself with a tight bloom.
    {"Folded Atrium", makePreset(0.55f, 0.45f, 0.55f, 0.20f, 0.30f, 0.80f, 0.10f, 0.55f, 0.50f, 0.70f, 0.0f)},
    // Massive but sparse, with minimal bloom and no warp.
    {"Monumental Void", makePreset(0.90f, 0.35f, 0.10f, 0.00f, 0.00f, 0.00f, 0.00f, 0.65f, 0.45f, 0.80f, 0.0f)},
    // Short, lush bloom with weightless diffusion.
    {"Zero-G Garden", makePreset(0.25f, 0.30f, 0.85f, 0.85f, 0.10f, 0.50f, 0.40f, 0.50f, 0.70f, 0.90f, 0.0f)},
    // Maximum warp with reflective, bending echoes.
    {"Hall of Mirrors", makePreset(0.60f, 0.40f, 0.60f, 0.40f, 0.20f, 1.00f, 0.30f, 0.55f, 0.60f, 0.85f, 0.0f)},
    // Slow, dimensional motion with extended decay.
    {"Tesseract Chamber", makePreset(0.85f, 0.55f, 0.30f, 0.60f, 0.50f, 0.70f, 0.70f, 0.65f, 0.50f, 0.75f, 0.0f)},
    // Tight and dry, like ancient stones breathing lightly.
    {"Stone Circles", makePreset(0.15f, 0.60f, 0.20f, 0.20f, 1.00f, 0.00f, 0.00f, 0.45f, 0.35f, 0.40f, 0.0f)},
    // Use Freeze to capture a moment and let it hover.
    {"Frozen Monument (Engage Freeze)", makePreset(0.70f, 0.50f, 0.50f, 0.50f, 0.50f, 0.00f, 0.00f, 0.60f, 0.50f, 0.70f, 0.0f)},
}};

PresetManager::PresetManager(juce::AudioProcessorValueTreeState& apvts)
    : parameters(apvts)
{
}

int PresetManager::getNumPresets() const
{
    return static_cast<int>(kPresets.size());
}

std::string PresetManager::getPresetName(int index) const
{
    if (index < 0 || index >= static_cast<int>(kPresets.size()))
        return {};
    return kPresets[static_cast<size_t>(index)].name;
}

int PresetManager::findPresetIndex(const std::string& name) const
{
    auto it = std::find_if(kPresets.begin(), kPresets.end(),
        [&name](const Preset& preset) { return name == preset.name; });
    if (it == kPresets.end())
        return -1;
    return static_cast<int>(std::distance(kPresets.begin(), it));
}

bool PresetManager::loadPreset(int index)
{
    if (index < 0 || index >= static_cast<int>(kPresets.size()))
        return false;
    applyPreset(kPresets[static_cast<size_t>(index)].values);
    return true;
}

bool PresetManager::loadPresetByName(const std::string& name)
{
    const int index = findPresetIndex(name);
    if (index < 0)
        return false;
    return loadPreset(index);
}

const std::array<PresetManager::Preset, PresetManager::kNumPresets>& PresetManager::getPresets()
{
    return kPresets;
}

void PresetManager::applyPreset(const PresetValues& values)
{
    setParamNormalized(parameters, "time", values.time);
    setParamNormalized(parameters, "mass", values.mass);
    setParamNormalized(parameters, "density", values.density);
    setParamNormalized(parameters, "bloom", values.bloom);
    setParamNormalized(parameters, "gravity", values.gravity);
    setParamNormalized(parameters, "warp", values.warp);
    setParamNormalized(parameters, "drift", values.drift);
    setParamNormalized(parameters, "mix", values.mix);
    setParamNormalized(parameters, "air", values.air);
    setParamNormalized(parameters, "width", values.width);
    setParamNormalized(parameters, "freeze", values.freeze);
}
