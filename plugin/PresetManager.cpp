#include "PresetManager.h"

#include <algorithm>

namespace
{
constexpr int kPresetVersion = 4;  // v4: Added 4 new Ancient Monuments macros (patina, abyss, corona, breath)

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
    float memoryDrift = 0.3f,
    float material = 0.5f,
    float topology = 0.5f,
    float viscosity = 0.5f,
    float evolution = 0.5f,
    float chaosIntensity = 0.0f,
    float elasticityDecay = 0.0f,
    float patina = 0.5f,
    float abyss = 0.5f,
    float corona = 0.5f,
    float breath = 0.0f)
{
    return {time, mass, density, bloom, gravity, warp, drift, memory, memoryDepth, memoryDecay, memoryDrift, mix,
            material, topology, viscosity, evolution, chaosIntensity, elasticityDecay,
            patina, abyss, corona, breath};
}

// Phase 3: Helper to create modulation connections for "Living" presets
monument::dsp::ModulationMatrix::Connection makeModConnection(
    monument::dsp::ModulationMatrix::SourceType source,
    monument::dsp::ModulationMatrix::DestinationType destination,
    float depth,
    int sourceAxis = 0,
    float smoothingMs = 200.0f)
{
    monument::dsp::ModulationMatrix::Connection conn;
    conn.source = source;
    conn.destination = destination;
    conn.sourceAxis = sourceAxis;
    conn.depth = depth;
    conn.smoothingMs = smoothingMs;
    conn.enabled = true;
    return conn;
}

PresetManager::PresetValues makePresetWithMod(
    float time, float mass, float density, float bloom, float gravity,
    float warp, float drift, float mix,
    const std::vector<monument::dsp::ModulationMatrix::Connection>& modulations,
    float memory = 0.0f, float memoryDepth = 0.5f, float memoryDecay = 0.4f, float memoryDrift = 0.3f,
    float material = 0.5f, float topology = 0.5f, float viscosity = 0.5f,
    float evolution = 0.5f, float chaosIntensity = 0.0f, float elasticityDecay = 0.0f,
    float patina = 0.5f, float abyss = 0.5f, float corona = 0.5f, float breath = 0.0f)
{
    PresetManager::PresetValues preset{time, mass, density, bloom, gravity, warp, drift, memory, memoryDepth, memoryDecay, memoryDrift, mix,
                                       material, topology, viscosity, evolution, chaosIntensity, elasticityDecay,
                                       patina, abyss, corona, breath};
    preset.modulationConnections = modulations;
    return preset;
}
} // namespace

const std::array<PresetManager::Preset, PresetManager::kNumFactoryPresets> PresetManager::kFactoryPresets{{
    {"Init Patch", "A clean, even hall with no motion, ready to be shaped.",
        makePreset(0.50f, 0.50f, 0.50f, 0.50f, 0.50f, 0.00f, 0.00f, 0.50f)},
    {"Stone Hall", "Hard surfaces and steady air hold the sound in place, with no movement.",
        makePreset(0.45f, 0.55f, 0.45f, 0.10f, 0.65f, 0.00f, 0.05f, 0.55f)},
    {"High Vault", "Tall ceilings lift the sound upward, bright and still.",
        makePreset(0.75f, 0.35f, 0.65f, 0.30f, 0.25f, 0.10f, 0.15f, 0.60f)},
    {"Cold Chamber", "A cool, heavy room where the sound settles quickly and stays put.",
        makePreset(0.55f, 0.70f, 0.35f, 0.20f, 0.75f, 0.00f, 0.00f, 0.55f)},
    {"Night Atrium", "Wide and quiet, with a soft roof of darkness that keeps everything calm.",
        makePreset(0.65f, 0.45f, 0.55f, 0.25f, 0.35f, 0.15f, 0.10f, 0.60f)},
    {"Monumental Void", "Immense and sparse, the space feels carved out of silence.",
        makePreset(0.90f, 0.35f, 0.10f, 0.00f, 0.00f, 0.00f, 0.00f, 0.65f)},
    {"Stone Circles", "Short, grounded rings gather in layers, with a faint sense of earlier steps lingering.",
        makePreset(0.15f, 0.60f, 0.20f, 0.20f, 1.00f, 0.00f, 0.00f, 0.45f, 0.20f, 0.40f, 0.45f, 0.20f)},
    {"Cathedral of Glass", "Bright surfaces carry long light trails; a few remnants drift back, fragile and high.",
        makePreset(0.82f, 0.25f, 0.80f, 0.55f, 0.15f, 0.00f, 0.20f, 0.60f, 0.25f, 0.45f, 0.45f, 0.25f)},
    {"Zero-G Garden", "Light and buoyant, the space breathes; soft afterimages barely return.",
        makePreset(0.25f, 0.30f, 0.85f, 0.85f, 0.10f, 0.50f, 0.40f, 0.50f, 0.22f, 0.50f, 0.40f, 0.25f)},
    {"Weathered Nave", "The hall seems to absorb what passes through it, letting a softened trace rise later.",
        makePreset(0.70f, 0.55f, 0.50f, 0.35f, 0.45f, 0.20f, 0.20f, 0.60f, 0.30f, 0.55f, 0.55f, 0.30f)},
    {"Dust in the Columns", "Fine dust hangs in the air; sounds leave a faint residue that settles slowly.",
        makePreset(0.40f, 0.40f, 0.60f, 0.30f, 0.35f, 0.25f, 0.25f, 0.55f, 0.28f, 0.50f, 0.50f, 0.35f)},
    {"Frozen Monument (Engage Freeze)", "A still, glassy hold that waits in place, with only the quietest return.",
        makePreset(0.70f, 0.50f, 0.50f, 0.50f, 0.50f, 0.00f, 0.00f, 0.60f, 0.15f, 0.45f, 0.40f, 0.20f)},
    {"Ruined Monument (Remembers)", "The space remembers what touched it, releasing darkened pieces long after the moment passes.",
        makePreset(0.85f, 0.60f, 0.40f, 0.45f, 0.45f, 0.10f, 0.15f, 0.60f, 0.70f, 0.70f, 0.70f, 0.45f)},
    {"What the Hall Kept", "What passes through is kept and released later, quieter and weathered.",
        makePreset(0.75f, 0.60f, 0.45f, 0.50f, 0.55f, 0.20f, 0.25f, 0.60f, 0.80f, 0.80f, 0.75f, 0.50f)},
    {"Event Horizon", "The room bends toward a heavy center, then sends back shadows of what fell in.",
        makePreset(1.00f, 0.85f, 0.55f, 0.85f, 0.50f, 0.30f, 0.50f, 0.70f, 0.75f, 0.70f, 0.70f, 0.50f)},
    {"Folded Atrium", "A folded space where entrances and exits blur, and earlier notes reappear like mistaken doors.",
        makePreset(0.55f, 0.45f, 0.55f, 0.20f, 0.30f, 0.80f, 0.10f, 0.55f, 0.60f, 0.55f, 0.55f, 0.40f)},
    {"Hall of Mirrors", "Reflections lose their order as the hall folds inward, returning softened images out of sequence.",
        makePreset(0.60f, 0.40f, 0.60f, 0.40f, 0.20f, 1.00f, 0.30f, 0.55f, 0.65f, 0.55f, 0.55f, 0.50f)},
    {"Tesseract Chamber", "The room turns in on itself; distant traces drift back, slightly misplaced in time.",
        makePreset(0.85f, 0.55f, 0.30f, 0.60f, 0.50f, 0.70f, 0.70f, 0.65f, 0.70f, 0.65f, 0.65f, 0.55f)},

    // Phase 3: "Living" Presets with Modulation (Discovery-Focused, No UI Controls)
    {"Breathing Stone", "The hall expands and contracts with your signal, as if the walls themselves are alive.",
        makePresetWithMod(0.55f, 0.60f, 0.50f, 0.50f, 0.65f, 0.00f, 0.05f, 0.55f,
            {makeModConnection(monument::dsp::ModulationMatrix::SourceType::AudioFollower,
                              monument::dsp::ModulationMatrix::DestinationType::Bloom, 0.30f, 0, 250.0f)})},

    {"Drifting Cathedral", "The space wanders slowly, its character shifting like clouds overhead.",
        makePresetWithMod(0.70f, 0.50f, 0.55f, 0.40f, 0.50f, 0.10f, 0.15f, 0.60f,
            {makeModConnection(monument::dsp::ModulationMatrix::SourceType::BrownianMotion,
                              monument::dsp::ModulationMatrix::DestinationType::Drift, 0.35f, 0, 400.0f),
             makeModConnection(monument::dsp::ModulationMatrix::SourceType::BrownianMotion,
                              monument::dsp::ModulationMatrix::DestinationType::Gravity, 0.18f, 0, 600.0f)})},

    {"Chaos Hall", "The room breathes with strange, organic patterns—alive but unknowable.",
        makePresetWithMod(0.60f, 0.55f, 0.60f, 0.35f, 0.45f, 0.20f, 0.25f, 0.55f,
            {makeModConnection(monument::dsp::ModulationMatrix::SourceType::ChaosAttractor,
                              monument::dsp::ModulationMatrix::DestinationType::Warp, 0.45f, 0, 300.0f),
             makeModConnection(monument::dsp::ModulationMatrix::SourceType::ChaosAttractor,
                              monument::dsp::ModulationMatrix::DestinationType::Density, 0.25f, 1, 350.0f)})},

    {"Living Pillars", "The columns reshape themselves to the music, dancing in place.",
        makePresetWithMod(0.50f, 0.50f, 0.65f, 0.45f, 0.55f, 0.15f, 0.10f, 0.55f,
            {makeModConnection(monument::dsp::ModulationMatrix::SourceType::EnvelopeTracker,
                              monument::dsp::ModulationMatrix::DestinationType::PillarShape, 0.35f, 0, 200.0f),
             makeModConnection(monument::dsp::ModulationMatrix::SourceType::AudioFollower,
                              monument::dsp::ModulationMatrix::DestinationType::Width, 0.22f, 0, 300.0f)})},

    {"Event Horizon Evolved", "The gravitational center shifts and wobbles, pulling the sound into ever-changing orbits.",
        makePresetWithMod(1.00f, 0.85f, 0.55f, 0.85f, 0.50f, 0.30f, 0.50f, 0.70f,
            {makeModConnection(monument::dsp::ModulationMatrix::SourceType::ChaosAttractor,
                              monument::dsp::ModulationMatrix::DestinationType::Mass, 0.18f, 2, 500.0f),
             makeModConnection(monument::dsp::ModulationMatrix::SourceType::BrownianMotion,
                              monument::dsp::ModulationMatrix::DestinationType::Drift, 0.50f, 0, 800.0f)},
            0.75f, 0.70f, 0.70f, 0.70f)},

    // Phase 5: Physical Modeling Presets
    {"Metallic Corridor", "Sound travels through a network of resonant metal tubes, each ringing with its own harmonic character.",
        makePreset(0.65f, 0.55f, 0.60f, 0.35f, 0.50f, 0.20f, 0.15f, 0.60f, 0.0f, 0.5f, 0.4f, 0.3f,
            0.85f,  // material: hard/metallic → strong metallic resonance, uniform tubes
            0.60f,  // topology: moderate → complex tube network
            0.45f,  // viscosity: moderate
            0.30f,  // evolution: subtle
            0.0f,   // chaos: stable
            0.0f)}, // elasticity: instant recovery

    {"Elastic Cathedral", "The walls pulse and breathe with the music, deforming under acoustic pressure and slowly returning to shape.",
        makePreset(0.75f, 0.50f, 0.55f, 0.50f, 0.55f, 0.15f, 0.20f, 0.65f, 0.0f, 0.5f, 0.4f, 0.3f,
            0.40f,  // material: soft → varied tubes
            0.45f,  // topology: moderate
            0.75f,  // viscosity: thick → slow wall recovery
            0.60f,  // evolution: evolving → absorption drift
            0.0f,   // chaos: stable
            0.80f)},// elasticity: slow deformation → high wall elasticity

    {"Impossible Chamber", "Physics breaks down—frequencies amplify impossibly, pitches drift through dimensions, reality bends.",
        makePreset(0.70f, 0.60f, 0.50f, 0.60f, 0.45f, 0.50f, 0.40f, 0.65f, 0.0f, 0.5f, 0.4f, 0.3f,
            0.50f,  // material: neutral
            0.75f,  // topology: non-Euclidean → complex tube network, paradox freq variation
            0.50f,  // viscosity: moderate
            0.70f,  // evolution: high → pitch evolution, absorption drift
            0.75f,  // chaos: very high → impossibility physics, nonlinearity, paradox gain
            0.40f)},// elasticity: moderate

    {"Breathing Tubes", "Organic metal pipes expand and contract like lungs, creating a living acoustic environment.",
        makePresetWithMod(0.55f, 0.60f, 0.65f, 0.55f, 0.60f, 0.10f, 0.25f, 0.60f,
            {makeModConnection(monument::dsp::ModulationMatrix::SourceType::AudioFollower,
                              monument::dsp::ModulationMatrix::DestinationType::RadiusVariation, 0.35f, 0, 300.0f),
             makeModConnection(monument::dsp::ModulationMatrix::SourceType::BrownianMotion,
                              monument::dsp::ModulationMatrix::DestinationType::Elasticity, 0.25f, 0, 500.0f)},
            0.0f, 0.5f, 0.4f, 0.3f,
            0.65f,  // material: moderate hard → some metallic character
            0.50f,  // topology: moderate
            0.80f,  // viscosity: thick → slow recovery
            0.45f,  // evolution: moderate
            0.20f,  // chaos: subtle instability
            0.65f)},// elasticity: high → walls deform significantly

    {"Quantum Hall", "A non-Euclidean space where tubes fold through higher dimensions and sound obeys impossible laws.",
        makePresetWithMod(0.80f, 0.65f, 0.45f, 0.70f, 0.50f, 0.70f, 0.60f, 0.70f,
            {makeModConnection(monument::dsp::ModulationMatrix::SourceType::ChaosAttractor,
                              monument::dsp::ModulationMatrix::DestinationType::CouplingStrength, 0.40f, 0, 250.0f),
             makeModConnection(monument::dsp::ModulationMatrix::SourceType::ChaosAttractor,
                              monument::dsp::ModulationMatrix::DestinationType::ImpossibilityDegree, 0.30f, 1, 400.0f)},
            0.0f, 0.5f, 0.4f, 0.3f,
            0.55f,  // material: moderate
            0.90f,  // topology: very non-Euclidean → max tube network complexity and coupling
            0.50f,  // viscosity: moderate
            0.65f,  // evolution: high → pitch morphing
            0.85f,  // chaos: very high → alien physics
            0.50f)},// elasticity: moderate
}};

PresetManager::PresetManager(juce::AudioProcessorValueTreeState& apvts,
                             monument::dsp::ModulationMatrix* modMatrix)
    : parameters(apvts), modulationMatrix(modMatrix)
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

    // Phase 2: Save macro parameters
    params->setProperty("material", values.material);
    params->setProperty("topology", values.topology);
    params->setProperty("viscosity", values.viscosity);
    params->setProperty("evolution", values.evolution);
    params->setProperty("chaosIntensity", values.chaosIntensity);
    params->setProperty("elasticityDecay", values.elasticityDecay);

    // Phase 5: Save Ancient Monuments macros 7-10
    params->setProperty("patina", values.patina);
    params->setProperty("abyss", values.abyss);
    params->setProperty("corona", values.corona);
    params->setProperty("breath", values.breath);

    root->setProperty("parameters", params.release());

    // Phase 3: Save modulation connections
    if (modulationMatrix != nullptr)
    {
        const auto& connections = modulationMatrix->getConnections();
        juce::Array<juce::var> modulationArray;

        for (const auto& conn : connections)
        {
            if (!conn.enabled)
                continue;  // Skip disabled connections

            auto connObj = std::make_unique<juce::DynamicObject>();
            connObj->setProperty("source", sourceTypeToString(conn.source));
            connObj->setProperty("destination", destinationTypeToString(conn.destination));
            connObj->setProperty("sourceAxis", conn.sourceAxis);
            connObj->setProperty("depth", conn.depth);
            connObj->setProperty("smoothingMs", conn.smoothingMs);
            connObj->setProperty("enabled", conn.enabled);

            modulationArray.add(juce::var(connObj.release()));
        }

        root->setProperty("modulation", modulationArray);
    }

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

    // Phase 2: Load macro parameters
    values.material = readFloatProperty(paramsObject, "material", values.material);
    values.topology = readFloatProperty(paramsObject, "topology", values.topology);
    values.viscosity = readFloatProperty(paramsObject, "viscosity", values.viscosity);
    values.evolution = readFloatProperty(paramsObject, "evolution", values.evolution);
    values.chaosIntensity = readFloatProperty(paramsObject, "chaosIntensity", values.chaosIntensity);
    values.elasticityDecay = readFloatProperty(paramsObject, "elasticityDecay", values.elasticityDecay);

    // Phase 5: Load Ancient Monuments macros 7-10 (with v3→v4 migration defaults)
    values.patina = readFloatProperty(paramsObject, "patina", 0.5f);
    values.abyss = readFloatProperty(paramsObject, "abyss", 0.5f);
    values.corona = readFloatProperty(paramsObject, "corona", 0.5f);
    values.breath = readFloatProperty(paramsObject, "breath", 0.0f);

    // Phase 3: Load modulation connections
    values.modulationConnections.clear();
    auto modulationVar = rootObject->getProperty("modulation");
    if (modulationVar.isArray())
    {
        const auto* modulationArray = modulationVar.getArray();
        for (const auto& connVar : *modulationArray)
        {
            auto* connObj = connVar.getDynamicObject();
            if (connObj == nullptr)
                continue;

            monument::dsp::ModulationMatrix::Connection conn;
            conn.source = stringToSourceType(connObj->getProperty("source").toString());
            conn.destination = stringToDestinationType(connObj->getProperty("destination").toString());
            conn.sourceAxis = static_cast<int>(connObj->getProperty("sourceAxis"));
            conn.depth = static_cast<float>(connObj->getProperty("depth"));
            conn.smoothingMs = static_cast<float>(connObj->getProperty("smoothingMs"));
            conn.enabled = static_cast<bool>(connObj->getProperty("enabled"));

            values.modulationConnections.push_back(conn);
        }
    }

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

    // Phase 2: Capture macro parameters (so UI updates on preset load)
    values.material = readParam("material", values.material);
    values.topology = readParam("topology", values.topology);
    values.viscosity = readParam("viscosity", values.viscosity);
    values.evolution = readParam("evolution", values.evolution);
    values.chaosIntensity = readParam("chaosIntensity", values.chaosIntensity);
    values.elasticityDecay = readParam("elasticityDecay", values.elasticityDecay);

    // Phase 5: Capture Ancient Monuments macros 7-10
    values.patina = readParam("patina", values.patina);
    values.abyss = readParam("abyss", values.abyss);
    values.corona = readParam("corona", values.corona);
    values.breath = readParam("breath", values.breath);

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

    // Phase 3: Cache modulation connections for PluginProcessor to apply
    lastLoadedModulationConnections = values.modulationConnections;
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

    // Phase 2: Apply macro parameters (so UI updates on preset load)
    setParamNormalized(parameters, "material", values.material);
    setParamNormalized(parameters, "topology", values.topology);
    setParamNormalized(parameters, "viscosity", values.viscosity);
    setParamNormalized(parameters, "evolution", values.evolution);
    setParamNormalized(parameters, "chaosIntensity", values.chaosIntensity);
    setParamNormalized(parameters, "elasticityDecay", values.elasticityDecay);

    // Phase 5: Apply Ancient Monuments macros 7-10
    setParamNormalized(parameters, "patina", values.patina);
    setParamNormalized(parameters, "abyss", values.abyss);
    setParamNormalized(parameters, "corona", values.corona);
    setParamNormalized(parameters, "breath", values.breath);
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

// Phase 3: Enum-to-string conversion for modulation serialization
juce::String PresetManager::sourceTypeToString(monument::dsp::ModulationMatrix::SourceType type)
{
    using SourceType = monument::dsp::ModulationMatrix::SourceType;
    switch (type)
    {
        case SourceType::ChaosAttractor:  return "ChaosAttractor";
        case SourceType::AudioFollower:   return "AudioFollower";
        case SourceType::BrownianMotion:  return "BrownianMotion";
        case SourceType::EnvelopeTracker: return "EnvelopeTracker";
        default:                          return "Unknown";
    }
}

juce::String PresetManager::destinationTypeToString(monument::dsp::ModulationMatrix::DestinationType type)
{
    using DestinationType = monument::dsp::ModulationMatrix::DestinationType;
    switch (type)
    {
        case DestinationType::Time:                return "Time";
        case DestinationType::Mass:                return "Mass";
        case DestinationType::Density:             return "Density";
        case DestinationType::Bloom:               return "Bloom";
        case DestinationType::Air:                 return "Air";
        case DestinationType::Width:               return "Width";
        case DestinationType::Mix:                 return "Mix";
        case DestinationType::Warp:                return "Warp";
        case DestinationType::Drift:               return "Drift";
        case DestinationType::Gravity:             return "Gravity";
        case DestinationType::PillarShape:         return "PillarShape";
        case DestinationType::TubeCount:           return "TubeCount";
        case DestinationType::MetallicResonance:   return "MetallicResonance";
        case DestinationType::Elasticity:          return "Elasticity";
        case DestinationType::ImpossibilityDegree: return "ImpossibilityDegree";
        default:                                   return "Unknown";
    }
}

monument::dsp::ModulationMatrix::SourceType PresetManager::stringToSourceType(const juce::String& str)
{
    using SourceType = monument::dsp::ModulationMatrix::SourceType;
    if (str == "ChaosAttractor")  return SourceType::ChaosAttractor;
    if (str == "AudioFollower")   return SourceType::AudioFollower;
    if (str == "BrownianMotion")  return SourceType::BrownianMotion;
    if (str == "EnvelopeTracker") return SourceType::EnvelopeTracker;
    return SourceType::ChaosAttractor;  // Default fallback
}

monument::dsp::ModulationMatrix::DestinationType PresetManager::stringToDestinationType(const juce::String& str)
{
    using DestinationType = monument::dsp::ModulationMatrix::DestinationType;
    if (str == "Time")                return DestinationType::Time;
    if (str == "Mass")                return DestinationType::Mass;
    if (str == "Density")             return DestinationType::Density;
    if (str == "Bloom")               return DestinationType::Bloom;
    if (str == "Air")                 return DestinationType::Air;
    if (str == "Width")               return DestinationType::Width;
    if (str == "Mix")                 return DestinationType::Mix;
    if (str == "Warp")                return DestinationType::Warp;
    if (str == "Drift")               return DestinationType::Drift;
    if (str == "Gravity")             return DestinationType::Gravity;
    if (str == "PillarShape")         return DestinationType::PillarShape;
    if (str == "TubeCount")           return DestinationType::TubeCount;
    if (str == "MetallicResonance")   return DestinationType::MetallicResonance;
    if (str == "Elasticity")          return DestinationType::Elasticity;
    if (str == "ImpossibilityDegree") return DestinationType::ImpossibilityDegree;
    return DestinationType::Warp;  // Default fallback
}
