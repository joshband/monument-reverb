#pragma once

#include "LayeredKnob.h"
#include "BinaryData.h"

/**
 * Monument TIME knob using layered rendering.
 *
 * Renders 4 layers with industrial/brutalist aesthetic:
 * - Layer 0 (bottom): Concrete base body with radial segments (rotates)
 * - Layer 1: Engraved detail ring with scale markings (static)
 * - Layer 2: Metal indicator bar (rotates)
 * - Layer 3 (top): Brushed metal center cap (static)
 *
 * Standard 270° rotation range (-135° to +135°).
 */
class MonumentTimeKnob : public LayeredKnob
{
public:
    /**
     * Create TIME knob bound to the "time" parameter.
     *
     * @param state AudioProcessorValueTreeState from processor
     */
    explicit MonumentTimeKnob(juce::AudioProcessorValueTreeState& state)
        : LayeredKnob(state, "time", "TIME")
    {
        // Layer 0 (bottom): Base body - rotates with parameter
        addLayer(
            BinaryData::base_body_concrete_png,
            BinaryData::base_body_concrete_pngSize,
            true  // rotates
        );

        // Layer 1: Detail ring - static (scale markings)
        addLayer(
            BinaryData::detail_ring_engraved_png,
            BinaryData::detail_ring_engraved_pngSize,
            false  // stays fixed
        );

        // Layer 2: Indicator - rotates with parameter
        addLayer(
            BinaryData::indicator_metal_png,
            BinaryData::indicator_metal_pngSize,
            true  // rotates
        );

        // Layer 3 (top): Center cap - static
        addLayer(
            BinaryData::center_cap_brushed_metal_png,
            BinaryData::center_cap_brushed_metal_pngSize,
            false  // stays fixed
        );

        // Standard audio knob rotation: 270° sweep
        // -135° (7:30 position) to +135° (4:30 position)
        setRotationRange(-135.0f, +135.0f);
    }

    ~MonumentTimeKnob() override = default;

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MonumentTimeKnob)
};
