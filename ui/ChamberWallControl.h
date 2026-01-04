#pragma once

#include "MonumentControl.h"

/**
 * Photorealistic chamber wall control for Time parameter.
 *
 * Displays a beautiful granite stone texture with depth and shadows.
 * Uses a 4-frame horizontal sprite sheet for smooth interpolation.
 */
class ChamberWallControl : public MonumentControl
{
public:
    ChamberWallControl(juce::AudioProcessorValueTreeState& state);

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChamberWallControl)
};
