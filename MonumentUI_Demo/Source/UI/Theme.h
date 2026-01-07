#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_graphics/juce_graphics.h>

namespace Monument
{

/**
 * Theme defines the visual material palette for the Monument UI.
 * Supports layered PBR rendering with separate stone/crystal/core assets.
 */
struct Theme
{
    juce::String name;
    juce::Colour background;
    float glowIntensity;           // 0.0 - 1.0
    juce::String stoneSet;          // "ancient", "void", "cathedral"
    juce::String crystalSet;        // "blue", "white", "gold"

    // Audio-reactive response weights
    float panelResponseWeight = 0.15f;
    float knobResponseWeight = 0.35f;
};

namespace Themes
{
    /**
     * ANCIENT THEME
     * - Warm weathered stone
     * - Deep blue crystals
     * - Low glow intensity
     * - Heavy textures
     */
    static const Theme Ancient {
        "Ancient",
        juce::Colour(0xff0e0e10),      // Very dark grey
        0.4f,                           // Subdued glow
        "stone",                        // Default stone set
        "crystal_blue",                 // Blue crystals
        0.15f,                          // Panel response
        0.35f                           // Knob response
    };

    /**
     * VOID THEME
     * - Black obsidian
     * - Blue-white glow
     * - High intensity
     * - Minimal texture
     */
    static const Theme Void {
        "Void",
        juce::Colours::black,           // Pure black
        0.8f,                           // Bright glow
        "obsidian",                     // Glossy obsidian
        "crystal_white",                // White/cyan crystals
        0.25f,                          // Strong panel response
        0.50f                           // Strong knob response
    };

    /**
     * CATHEDRAL THEME
     * - Pale marble stone
     * - Gold crystal accents
     * - Medium glow
     * - Vertical emphasis
     */
    static const Theme Cathedral {
        "Cathedral",
        juce::Colour(0xff1a1a1a),       // Dark grey
        0.6f,                           // Medium glow
        "marble",                       // Polished marble
        "crystal_gold",                 // Gold crystals
        0.10f,                          // Subtle panel response
        0.30f                           // Moderate knob response
    };
}

} // namespace Monument
