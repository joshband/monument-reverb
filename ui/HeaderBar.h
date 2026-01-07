#pragma once

#include <JuceHeader.h>

/**
 * Header bar component for Monument Reverb UI
 *
 * Features:
 * - MONUMENT logo/title (left)
 * - Preset selector dropdown (center-left)
 * - Hall/Wall selector (center)
 * - Architecture dropdown (center-right)
 * - Input/output level meters (right)
 * - Dark theme styling to match brutalist aesthetic
 */
class HeaderBar : public juce::Component
{
public:
    HeaderBar(juce::AudioProcessorValueTreeState& apvts);
    ~HeaderBar() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // Preset management
    juce::ComboBox& getPresetSelector() { return presetSelector; }

    // Level meter updates (called from audio thread via timer)
    void setInputLevel(float level);
    void setOutputLevel(float level);

private:
    juce::AudioProcessorValueTreeState& valueTreeState;

    // UI Components
    juce::Label titleLabel;
    juce::ComboBox presetSelector;
    juce::ComboBox hallWallSelector;
    juce::ComboBox architectureSelector;

    // Level meters
    float inputLevel{0.0f};
    float outputLevel{0.0f};

    // Draw level meter
    void drawLevelMeter(juce::Graphics& g, juce::Rectangle<float> bounds, float level);

    // Styling constants
    static constexpr juce::uint32 BG_COLOR = 0xff1a1a1a;
    static constexpr juce::uint32 TEXT_COLOR = 0xffc0c0c0;
    static constexpr juce::uint32 ACCENT_COLOR = 0xff4488ff;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderBar)
};
