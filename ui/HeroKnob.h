#pragma once

#include "LayeredKnob.h"
#include "BinaryData.h"

/**
 * Hero knob using PBR albedo texture from materialize pipeline.
 *
 * Uses high-quality photorealistic stone knob texture with LED center.
 * Single rotating layer approach for initial integration.
 *
 * Future enhancements could include:
 * - Normal mapping for dynamic lighting
 * - Roughness-based material properties
 * - Multiple rotation frames (filmstrip) for smoother rotation
 *
 * Source: Series 1 from hero knob processing pipeline (2026-01-03)
 * Texture: 57-67% coverage, rich stone detail, warm amber LED glow
 */
class HeroKnob : public LayeredKnob
{
public:
    /**
     * Create hero knob bound to specified parameter.
     *
     * @param state AudioProcessorValueTreeState from processor
     * @param parameterId Parameter ID to bind to (e.g., "time", "size")
     * @param labelText Label text displayed below knob
     */
    HeroKnob(juce::AudioProcessorValueTreeState& state,
             const juce::String& parameterId,
             const juce::String& labelText)
        : LayeredKnob(state, parameterId, labelText)
    {
        // Layer 0: Hero knob RGBA albedo texture with alpha mask - rotates with parameter
        addLayer(
            BinaryData::albedo_rgba_png,
            BinaryData::albedo_rgba_pngSize,
            true  // rotates
        );

        // Standard audio knob rotation: 270° sweep
        // -135° (7:30 position) to +135° (4:30 position)
        setRotationRange(-135.0f, +135.0f);
    }

    ~HeroKnob() override = default;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeroKnob)
};
