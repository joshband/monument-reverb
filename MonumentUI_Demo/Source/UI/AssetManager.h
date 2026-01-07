#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>
#include "Theme.h"

namespace Monument
{

/**
 * AssetManager - Centralized asset loading and caching
 *
 * Supports layered PBR rendering:
 * - Stone base layers (albedo, normal, roughness)
 * - Crystal overlays (RGBA with alpha for glow)
 * - Core metal layers
 * - Indicator elements
 *
 * All assets loaded from BinaryData with deterministic keys.
 * No runtime file I/O - everything is embedded.
 */
class AssetManager
{
public:
    static AssetManager& instance();

    /**
     * Get an image by semantic key
     * @param key Format: "category.variant" e.g. "knob.stone.01", "panel.macro.bg"
     * @return Cached image or null image if not found
     */
    juce::Image getImage(const juce::String& key);

    /**
     * Set current theme - affects which asset variants are returned
     * @param stone Stone set name ("stone", "obsidian", "marble")
     * @param crystal Crystal set name ("crystal_blue", "crystal_white", "crystal_gold")
     */
    void setTheme(const juce::String& stone, const juce::String& crystal);

    /**
     * Apply theme directly
     */
    void setTheme(const Theme& theme);

    /**
     * Get current theme settings
     */
    juce::String getStoneSet() const { return stoneSet; }
    juce::String getCrystalSet() const { return crystalSet; }

    /**
     * Check if an asset key exists
     */
    bool hasImage(const juce::String& key) const;

    /**
     * Get all available keys (useful for debugging)
     */
    juce::StringArray getAvailableKeys() const;

private:
    AssetManager();
    ~AssetManager() = default;

    // No copying
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    void loadAllAssets();
    void loadAsset(const juce::String& key, const void* data, int size);

    juce::HashMap<juce::String, juce::Image> cache;
    juce::String stoneSet;
    juce::String crystalSet;
};

} // namespace Monument
