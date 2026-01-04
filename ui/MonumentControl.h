#pragma once

#include <JuceHeader.h>

/**
 * Base class for Monument's photorealistic sprite-based controls.
 *
 * Renders horizontal sprite sheets with smooth frame interpolation.
 * Supports APVTS parameter binding and mouse interaction.
 */
class MonumentControl : public juce::Component,
                        private juce::AudioProcessorValueTreeState::Listener
{
public:
    MonumentControl(juce::AudioProcessorValueTreeState& state,
                    const juce::String& parameterId,
                    const juce::String& labelText);

    ~MonumentControl() override;

    /**
     * Load a horizontal sprite sheet from BinaryData.
     *
     * @param spriteData Raw PNG data from BinaryData
     * @param dataSize Size of sprite data in bytes
     * @param frameCount Number of frames in the sprite sheet
     */
    void setSpriteSheet(const void* spriteData, size_t dataSize, int frameCount);

    /**
     * Set the current control state (normalized 0.0-1.0).
     * Automatically interpolates between sprite frames.
     */
    void setState(float normalizedValue);

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseEnter(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;

protected:
    /**
     * Map vertical mouse drag to normalized parameter value.
     * Override for custom drag sensitivity.
     */
    virtual float mapDragToValue(const juce::MouseEvent& event, float startValue);

private:
    // AudioProcessorValueTreeState::Listener
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // Sprite rendering
    juce::Image spriteSheet;
    std::vector<juce::Image> cachedFrames;  // Pre-extracted frames for fast rendering
    int frameCount = 1;
    float currentState = 0.0f;

    // UI state
    juce::Label label;
    bool isHovered = false;
    bool isDragging = false;
    float dragStartValue = 0.0f;
    int dragStartY = 0;

    // Parameter binding
    juce::AudioProcessorValueTreeState& apvts;
    juce::String paramId;
    juce::RangedAudioParameter* parameter = nullptr;

    // Helpers
    void extractFramesFromSpriteSheet();
    juce::Image getInterpolatedFrame() const;
    void updateParameterFromMouse(const juce::MouseEvent& event);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MonumentControl)
};
