#include "AssetManager.h"
#include "BinaryData.h"

namespace Monument
{

AssetManager& AssetManager::instance()
{
    static AssetManager inst;
    return inst;
}

AssetManager::AssetManager()
    : stoneSet("stone")
    , crystalSet("crystal_blue")
{
    loadAllAssets();
}

void AssetManager::loadAllAssets()
{
    // PANEL BACKGROUNDS
    // These are the 4 main panel backgrounds for each section
    loadAsset("panel.macro.bg",
              BinaryData::macro_cosmos_bg_png,
              BinaryData::macro_cosmos_bg_pngSize);

    loadAsset("panel.foundation.bg",
              BinaryData::foundation_bg_png,
              BinaryData::foundation_bg_pngSize);

    loadAsset("panel.modulation.bg",
              BinaryData::modulation_nexus_bg_png,
              BinaryData::modulation_nexus_bg_pngSize);

    loadAsset("panel.temporal.bg",
              BinaryData::temporal_vault_bg_png,
              BinaryData::temporal_vault_bg_pngSize);

    // HEADER BARS
    // Decorative header bars for each panel section
    loadAsset("header.macro",
              BinaryData::macro_cosmos_header_png,
              BinaryData::macro_cosmos_header_pngSize);

    loadAsset("header.foundation",
              BinaryData::foundation_header_png,
              BinaryData::foundation_header_pngSize);

    loadAsset("header.modulation",
              BinaryData::modulation_nexus_header_png,
              BinaryData::modulation_nexus_header_pngSize);

    loadAsset("header.temporal",
              BinaryData::temporal_vault_header_png,
              BinaryData::temporal_vault_header_pngSize);

    // KNOB LAYERS - STONE BASE
    // Stone exterior layer (albedo + roughness baked)
    // Format: knob.stone.XX where XX is variant number (01-12)
    loadAsset("knob.stone.01",
              BinaryData::knob_stone_01_png,
              BinaryData::knob_stone_01_pngSize);

    loadAsset("knob.stone.02",
              BinaryData::knob_stone_02_png,
              BinaryData::knob_stone_02_pngSize);

    loadAsset("knob.stone.03",
              BinaryData::knob_stone_03_png,
              BinaryData::knob_stone_03_pngSize);

    loadAsset("knob.stone.04",
              BinaryData::knob_stone_04_png,
              BinaryData::knob_stone_04_pngSize);

    // KNOB LAYERS - CRYSTAL GLOW
    // Crystal interior with LED glow (RGBA with alpha channel)
    // Format: knob.crystal.XX
    loadAsset("knob.crystal.01",
              BinaryData::crystal_glow_01_png,
              BinaryData::crystal_glow_01_pngSize);

    loadAsset("knob.crystal.02",
              BinaryData::crystal_glow_02_png,
              BinaryData::crystal_glow_02_pngSize);

    loadAsset("knob.crystal.warm",
              BinaryData::crystal_glow_warm_png,
              BinaryData::crystal_glow_warm_pngSize);

    loadAsset("knob.crystal.gold",
              BinaryData::crystal_glow_gold_png,
              BinaryData::crystal_glow_gold_pngSize);

    loadAsset("knob.crystal.amber",
              BinaryData::crystal_glow_amber_png,
              BinaryData::crystal_glow_amber_pngSize);

    // KNOB LAYERS - METAL CORE
    // Metal center cap (brushed aluminum, brass, copper)
    // Format: knob.core.material
    loadAsset("knob.core.brushed",
              BinaryData::core_metal_brushed_generated_png,
              BinaryData::core_metal_brushed_generated_pngSize);

    loadAsset("knob.core.brass",
              BinaryData::core_metal_brass_generated_png,
              BinaryData::core_metal_brass_generated_pngSize);

    loadAsset("knob.core.copper",
              BinaryData::core_metal_copper_generated_png,
              BinaryData::core_metal_copper_generated_pngSize);

    // KNOB LAYERS - ROTATION INDICATOR
    // Pointer/marker showing knob rotation angle
    // Format: knob.indicator.type
    loadAsset("knob.indicator.line",
              BinaryData::indicator_line_generated_png,
              BinaryData::indicator_line_generated_pngSize);

    loadAsset("knob.indicator.dot",
              BinaryData::indicator_dot_generated_png,
              BinaryData::indicator_dot_generated_pngSize);

    // === NEW PBR KNOB ASSETS (Complete layer stacks) ===
    // Format: knob.<type>.<layer> (e.g., knob.geode.albedo)
    // Each knob has 11 layers for full PBR rendering

    // GEODE KNOB (dark crystal with blue interior)
    loadAsset("knob.geode.albedo", BinaryData::albedo_png, BinaryData::albedo_pngSize);
    loadAsset("knob.geode.ao", BinaryData::ao_png, BinaryData::ao_pngSize);
    loadAsset("knob.geode.roughness", BinaryData::roughness_png, BinaryData::roughness_pngSize);
    loadAsset("knob.geode.normal", BinaryData::normal_png, BinaryData::normal_pngSize);
    loadAsset("knob.geode.glow_core", BinaryData::glow_core_png, BinaryData::glow_core_pngSize);
    loadAsset("knob.geode.glow_crystal", BinaryData::glow_crystal_png, BinaryData::glow_crystal_pngSize);
    loadAsset("knob.geode.bloom", BinaryData::bloom_png, BinaryData::bloom_pngSize);
    loadAsset("knob.geode.light_wrap", BinaryData::light_wrap_png, BinaryData::light_wrap_pngSize);
    loadAsset("knob.geode.highlight", BinaryData::highlight_png, BinaryData::highlight_pngSize);
    loadAsset("knob.geode.indicator", BinaryData::indicator_png, BinaryData::indicator_pngSize);
    loadAsset("knob.geode.contact_shadow", BinaryData::contact_shadow_png, BinaryData::contact_shadow_pngSize);

    // OBSIDIAN KNOB (polished black volcanic glass)
    loadAsset("knob.obsidian.albedo", BinaryData::albedo_png2, BinaryData::albedo_png2Size);
    loadAsset("knob.obsidian.ao", BinaryData::ao_png2, BinaryData::ao_png2Size);
    loadAsset("knob.obsidian.roughness", BinaryData::roughness_png2, BinaryData::roughness_png2Size);
    loadAsset("knob.obsidian.normal", BinaryData::normal_png2, BinaryData::normal_png2Size);
    loadAsset("knob.obsidian.glow_core", BinaryData::glow_core_png2, BinaryData::glow_core_png2Size);
    loadAsset("knob.obsidian.glow_crystal", BinaryData::glow_crystal_png2, BinaryData::glow_crystal_png2Size);
    loadAsset("knob.obsidian.bloom", BinaryData::bloom_png2, BinaryData::bloom_png2Size);
    loadAsset("knob.obsidian.light_wrap", BinaryData::light_wrap_png2, BinaryData::light_wrap_png2Size);
    loadAsset("knob.obsidian.highlight", BinaryData::highlight_png2, BinaryData::highlight_png2Size);
    loadAsset("knob.obsidian.indicator", BinaryData::indicator_png2, BinaryData::indicator_png2Size);
    loadAsset("knob.obsidian.contact_shadow", BinaryData::contact_shadow_png2, BinaryData::contact_shadow_png2Size);

    // MARBLE KNOB (pale marble with veining)
    loadAsset("knob.marble.albedo", BinaryData::albedo_png3, BinaryData::albedo_png3Size);
    loadAsset("knob.marble.ao", BinaryData::ao_png3, BinaryData::ao_png3Size);
    loadAsset("knob.marble.roughness", BinaryData::roughness_png3, BinaryData::roughness_png3Size);
    loadAsset("knob.marble.normal", BinaryData::normal_png3, BinaryData::normal_png3Size);
    loadAsset("knob.marble.glow_core", BinaryData::glow_core_png3, BinaryData::glow_core_png3Size);
    loadAsset("knob.marble.glow_crystal", BinaryData::glow_crystal_png3, BinaryData::glow_crystal_png3Size);
    loadAsset("knob.marble.bloom", BinaryData::bloom_png3, BinaryData::bloom_png3Size);
    loadAsset("knob.marble.light_wrap", BinaryData::light_wrap_png3, BinaryData::light_wrap_png3Size);
    loadAsset("knob.marble.highlight", BinaryData::highlight_png3, BinaryData::highlight_png3Size);
    loadAsset("knob.marble.indicator", BinaryData::indicator_png3, BinaryData::indicator_png3Size);
    loadAsset("knob.marble.contact_shadow", BinaryData::contact_shadow_png3, BinaryData::contact_shadow_png3Size);

    // WEATHERED STONE KNOB (ancient weathered basalt)
    loadAsset("knob.weathered.albedo", BinaryData::albedo_png4, BinaryData::albedo_png4Size);
    loadAsset("knob.weathered.ao", BinaryData::ao_png4, BinaryData::ao_png4Size);
    loadAsset("knob.weathered.roughness", BinaryData::roughness_png4, BinaryData::roughness_png4Size);
    loadAsset("knob.weathered.normal", BinaryData::normal_png4, BinaryData::normal_png4Size);
    loadAsset("knob.weathered.glow_core", BinaryData::glow_core_png4, BinaryData::glow_core_png4Size);
    loadAsset("knob.weathered.glow_crystal", BinaryData::glow_crystal_png4, BinaryData::glow_crystal_png4Size);
    loadAsset("knob.weathered.bloom", BinaryData::bloom_png4, BinaryData::bloom_png4Size);
    loadAsset("knob.weathered.light_wrap", BinaryData::light_wrap_png4, BinaryData::light_wrap_png4Size);
    loadAsset("knob.weathered.highlight", BinaryData::highlight_png4, BinaryData::highlight_png4Size);
    loadAsset("knob.weathered.indicator", BinaryData::indicator_png4, BinaryData::indicator_png4Size);
    loadAsset("knob.weathered.contact_shadow", BinaryData::contact_shadow_png4, BinaryData::contact_shadow_png4Size);
}

void AssetManager::loadAsset(const juce::String& key, const void* data, int size)
{
    auto image = juce::ImageCache::getFromMemory(data, size);

    if (image.isValid())
    {
        cache.set(key, image);
        DBG("✓ Loaded asset: " << key << " (" << image.getWidth() << "x" << image.getHeight() << ")");
    }
    else
    {
        DBG("✗ FAILED to load asset: " << key);
        jassertfalse; // Asset load failed - check BinaryData
    }
}

juce::Image AssetManager::getImage(const juce::String& key)
{
    if (cache.contains(key))
        return cache[key];

    // Asset not found - return null image
    DBG("⚠ Asset not found: " << key);
    return juce::Image();
}

void AssetManager::setTheme(const juce::String& stone, const juce::String& crystal)
{
    stoneSet = stone;
    crystalSet = crystal;

    DBG("Theme changed: stone=" << stone << ", crystal=" << crystal);
    // In full implementation, this would trigger asset swapping
}

void AssetManager::setTheme(const Theme& theme)
{
    setTheme(theme.stoneSet, theme.crystalSet);
}

bool AssetManager::hasImage(const juce::String& key) const
{
    return cache.contains(key);
}

juce::StringArray AssetManager::getAvailableKeys() const
{
    juce::StringArray keys;
    for (auto it = cache.begin(); it != cache.end(); ++it)
        keys.add(it.getKey());

    keys.sort(false);
    return keys;
}

} // namespace Monument
