#include "PresetManager.h"

#include <algorithm>

namespace
{
constexpr int kPresetVersion = 1;

template <typename T>
float readFloatProperty(const juce::DynamicObject* object, const juce::String& key, T fallback)
{
    if (object == nullptr || !object->hasProperty(key))
        return static_cast<float>(fallback);
    return static_cast<float>(object->getProperty(key));
}

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
    float memory = 0.0f,
    float memoryDepth = 0.5f,
    float memoryDecay = 0.4f,
    float memoryDrift = 0.3f)
{
    return {time, mass, density, bloom, gravity, warp, drift, memory, memoryDepth, memoryDecay, memoryDrift, mix};
}
} // namespace

const std::array<PresetManager::Preset, PresetManager::kNumFactoryPresets> PresetManager::kFactoryPresets{{
    {"Init Patch", "Midpoint defaults with no warp or drift.", makePreset(0.50f, 0.50f, 0.50f, 0.50f, 0.50f, 0.00f, 0.00f, 0.50f)},
    {"Cathedral of Glass", "Long, bright reflections with gentle motion.", makePreset(0.82f, 0.25f, 0.80f, 0.55f, 0.15f, 0.00f, 0.20f, 0.60f)},
    {"Event Horizon", "Dark, swelling tail that bends around the source.", makePreset(1.00f, 0.85f, 0.55f, 0.85f, 0.50f, 0.30f, 0.50f, 0.70f)},
    {"Folded Atrium", "Medium time, tight bloom, topology folded inward.", makePreset(0.55f, 0.45f, 0.55f, 0.20f, 0.30f, 0.80f, 0.10f, 0.55f)},
    {"Monumental Void", "Massive, sparse, minimal bloom, no warp.", makePreset(0.90f, 0.35f, 0.10f, 0.00f, 0.00f, 0.00f, 0.00f, 0.65f)},
    {"Zero-G Garden", "Short, lush bloom with weightless diffusion.", makePreset(0.25f, 0.30f, 0.85f, 0.85f, 0.10f, 0.50f, 0.40f, 0.50f)},
    {"Hall of Mirrors", "Maximum warp with reflective, bending echoes.", makePreset(0.60f, 0.40f, 0.60f, 0.40f, 0.20f, 1.00f, 0.30f, 0.55f)},
    {"Tesseract Chamber", "Slow, dimensional motion with extended decay.", makePreset(0.85f, 0.55f, 0.30f, 0.60f, 0.50f, 0.70f, 0.70f, 0.65f)},
    {"Stone Circles", "Tight, dry echoes like ancient stones.", makePreset(0.15f, 0.60f, 0.20f, 0.20f, 1.00f, 0.00f, 0.00f, 0.45f)},
    {"Frozen Monument (Engage Freeze)", "Designed to be held by Freeze.", makePreset(0.70f, 0.50f, 0.50f, 0.50f, 0.50f, 0.00f, 0.00f, 0.60f)},
    {"Ruined Monument (Remembers)", "Faint, aged reflections resurface as the space remembers itself.",
        makePreset(0.85f, 0.60f, 0.40f, 0.45f, 0.45f, 0.10f, 0.15f, 0.60f, 0.55f, 0.75f, 0.60f, 0.35f)},
}};

PresetManager::PresetManager(juce::AudioProcessorValueTreeState& apvts)
    : parameters(apvts)
{
}

int PresetManager::getNumFactoryPresets() const
{
    return static_cast<int>(kFactoryPresets.size());
}

juce::String PresetManager::getFactoryPresetName(int index) const
{
    if (index < 0 || index >= static_cast<int>(kFactoryPresets.size()))
        return {};
    return kFactoryPresets[static_cast<size_t>(index)].name;
}

juce::String PresetManager::getFactoryPresetDescription(int index) const
{
    if (index < 0 || index >= static_cast<int>(kFactoryPresets.size()))
        return {};
    return kFactoryPresets[static_cast<size_t>(index)].description;
}

bool PresetManager::loadFactoryPreset(int index)
{
    if (index < 0 || index >= static_cast<int>(kFactoryPresets.size()))
        return false;
    applyPreset(kFactoryPresets[static_cast<size_t>(index)].values);
    return true;
}

bool PresetManager::loadFactoryPresetByName(const juce::String& name)
{
    auto it = std::find_if(kFactoryPresets.begin(), kFactoryPresets.end(),
        [&name](const Preset& preset) { return preset.name == name; });
    if (it == kFactoryPresets.end())
        return false;
    applyPreset(it->values);
    return true;
}

void PresetManager::saveUserPreset(const juce::String& name, const juce::String& description)
{
    saveUserPreset(juce::File{}, name, description);
}

void PresetManager::saveUserPreset(const juce::File& targetFile,
    const juce::String& name,
    const juce::String& description)
{
    const auto resolvedFile = resolveUserPresetFile(targetFile, name);
    if (resolvedFile == juce::File{})
        return;

    const auto values = captureCurrentValues();

    auto root = std::make_unique<juce::DynamicObject>();
    root->setProperty("formatVersion", kPresetVersion);
    root->setProperty("name", name);
    root->setProperty("description", description);

    auto params = std::make_unique<juce::DynamicObject>();
    params->setProperty("time", values.time);
    params->setProperty("mass", values.mass);
    params->setProperty("density", values.density);
    params->setProperty("bloom", values.bloom);
    params->setProperty("gravity", values.gravity);
    params->setProperty("warp", values.warp);
    params->setProperty("drift", values.drift);
    params->setProperty("memory", values.memory);
    params->setProperty("memoryDepth", values.memoryDepth);
    params->setProperty("memoryDecay", values.memoryDecay);
    params->setProperty("memoryDrift", values.memoryDrift);
    params->setProperty("mix", values.mix);

    root->setProperty("parameters", params.release());

    const juce::var json(root.release());
    const auto jsonText = juce::JSON::toString(json, true);
    resolvedFile.replaceWithText(jsonText);
}

bool PresetManager::loadUserPreset(const juce::File& sourceFile)
{
    if (!sourceFile.existsAsFile())
        return false;

    const auto jsonText = sourceFile.loadFileAsString();
    const auto json = juce::JSON::parse(jsonText);
    auto* rootObject = json.getDynamicObject();
    if (rootObject == nullptr)
        return false;

    auto paramsVar = rootObject->getProperty("parameters");
    auto* paramsObject = paramsVar.getDynamicObject();
    if (paramsObject == nullptr)
        paramsObject = rootObject;

    PresetValues values{};
    values.time = readFloatProperty(paramsObject, "time", values.time);
    values.mass = readFloatProperty(paramsObject, "mass", values.mass);
    values.density = readFloatProperty(paramsObject, "density", values.density);
    values.bloom = readFloatProperty(paramsObject, "bloom", values.bloom);
    values.gravity = readFloatProperty(paramsObject, "gravity", values.gravity);
    values.warp = readFloatProperty(paramsObject, "warp", values.warp);
    values.drift = readFloatProperty(paramsObject, "drift", values.drift);
    values.memory = readFloatProperty(paramsObject, "memory", values.memory);
    values.memoryDepth = readFloatProperty(paramsObject, "memoryDepth", values.memoryDepth);
    values.memoryDecay = readFloatProperty(paramsObject, "memoryDecay", values.memoryDecay);
    values.memoryDrift = readFloatProperty(paramsObject, "memoryDrift", values.memoryDrift);
    values.mix = readFloatProperty(paramsObject, "mix", values.mix);

    applyPreset(values);
    return true;
}

juce::File PresetManager::getDefaultUserPresetDirectory() const
{
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("MonumentPresets");
}

const std::array<PresetManager::Preset, PresetManager::kNumFactoryPresets>& PresetManager::getFactoryPresets()
{
    return kFactoryPresets;
}

PresetManager::PresetValues PresetManager::captureCurrentValues() const
{
    PresetValues values{};

    auto readParam = [this](const juce::String& id, float fallback)
    {
        if (auto* param = parameters.getParameter(id))
            return param->getValue();
        return fallback;
    };

    values.time = readParam("time", values.time);
    values.mass = readParam("mass", values.mass);
    values.density = readParam("density", values.density);
    values.bloom = readParam("bloom", values.bloom);
    values.gravity = readParam("gravity", values.gravity);
    values.warp = readParam("warp", values.warp);
    values.drift = readParam("drift", values.drift);
    values.memory = readParam("memory", values.memory);
    values.memoryDepth = readParam("memoryDepth", values.memoryDepth);
    values.memoryDecay = readParam("memoryDecay", values.memoryDecay);
    values.memoryDrift = readParam("memoryDrift", values.memoryDrift);
    values.mix = readParam("mix", values.mix);

    return values;
}

void PresetManager::applyPreset(const PresetValues& values)
{
    const auto& init = kFactoryPresets.front().values;
    // Always apply Init Patch first so preset switching clears residual state.
    setParamNormalized(parameters, "time", init.time);
    setParamNormalized(parameters, "mass", init.mass);
    setParamNormalized(parameters, "density", init.density);
    setParamNormalized(parameters, "bloom", init.bloom);
    setParamNormalized(parameters, "gravity", init.gravity);
    setParamNormalized(parameters, "warp", init.warp);
    setParamNormalized(parameters, "drift", init.drift);
    setParamNormalized(parameters, "memory", init.memory);
    setParamNormalized(parameters, "memoryDepth", init.memoryDepth);
    setParamNormalized(parameters, "memoryDecay", init.memoryDecay);
    setParamNormalized(parameters, "memoryDrift", init.memoryDrift);
    setParamNormalized(parameters, "mix", init.mix);
    setParamNormalized(parameters, "time", values.time);
    setParamNormalized(parameters, "mass", values.mass);
    setParamNormalized(parameters, "density", values.density);
    setParamNormalized(parameters, "bloom", values.bloom);
    setParamNormalized(parameters, "gravity", values.gravity);
    setParamNormalized(parameters, "warp", values.warp);
    setParamNormalized(parameters, "drift", values.drift);
    setParamNormalized(parameters, "memory", values.memory);
    setParamNormalized(parameters, "memoryDepth", values.memoryDepth);
    setParamNormalized(parameters, "memoryDecay", values.memoryDecay);
    setParamNormalized(parameters, "memoryDrift", values.memoryDrift);
    setParamNormalized(parameters, "mix", values.mix);
}

juce::File PresetManager::resolveUserPresetFile(const juce::File& targetFile, const juce::String& name) const
{
    auto presetDir = getDefaultUserPresetDirectory();
    presetDir.createDirectory();

    if (targetFile == juce::File{})
    {
        const auto fileName = juce::File::createLegalFileName(name.isNotEmpty() ? name : "UserPreset")
            .replaceCharacter(' ', '_');
        return presetDir.getChildFile(fileName + ".json");
    }

    if (targetFile.isDirectory())
    {
        const auto fileName = juce::File::createLegalFileName(name.isNotEmpty() ? name : "UserPreset")
            .replaceCharacter(' ', '_');
        return targetFile.getChildFile(fileName + ".json");
    }

    return targetFile;
}
