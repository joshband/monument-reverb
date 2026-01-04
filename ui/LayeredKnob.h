#pragma once

#include <JuceHeader.h>

/**
 * Layered knob control with rotation and alpha compositing.
 *
 * Renders multiple PNG layers with independent rotation:
 * - Layer 0 (bottom): Base body (rotates with parameter)
 * - Layer 1: Detail ring (static, scale markings)
 * - Layer 2: Indicator (rotates with parameter)
 * - Layer 3 (top): Center cap (static)
 *
 * All layers must be round PNGs with alpha channels (512×512px recommended).
 */
class LayeredKnob : public juce::Component,
                    private juce::AudioProcessorValueTreeState::Listener
{
public:
    /**
     * Layer specification for knob rendering.
     */
    struct Layer
    {
        juce::Image image;       // RGBA image with alpha channel
        bool rotates;            // Whether layer rotates with parameter
        float rotationOffset;    // Fixed rotation offset in radians (default: 0)

        Layer() : rotates(false), rotationOffset(0.0f) {}
        Layer(const juce::Image& img, bool rot, float offset = 0.0f)
            : image(img), rotates(rot), rotationOffset(offset) {}
    };

    /**
     * Create a layered knob bound to an APVTS parameter.
     *
     * @param state AudioProcessorValueTreeState for parameter binding
     * @param parameterId Parameter ID to bind to
     * @param labelText Label text displayed below knob
     */
    LayeredKnob(juce::AudioProcessorValueTreeState& state,
                const juce::String& parameterId,
                const juce::String& labelText);

    ~LayeredKnob() override;

    /**
     * Add a layer to the knob.
     * Layers are rendered bottom-to-top in order of addition.
     *
     * @param imageData PNG data from BinaryData
     * @param dataSize Size of PNG data in bytes
     * @param rotates Whether this layer rotates with parameter value
     * @param rotationOffset Fixed rotation offset in radians (optional)
     */
    void addLayer(const void* imageData, size_t dataSize,
                  bool rotates, float rotationOffset = 0.0f);

    /**
     * Set the rotation range mapping.
     *
     * Maps normalized parameter value (0.0-1.0) to rotation angle.
     * Default: -135° to +135° (270° sweep, standard audio knob).
     *
     * @param startAngleDegrees Start angle at value=0.0 (degrees)
     * @param endAngleDegrees End angle at value=1.0 (degrees)
     */
    void setRotationRange(float startAngleDegrees, float endAngleDegrees);

    /**
     * Set the current knob state (normalized 0.0-1.0).
     * Automatically updates rotation angle for rendering.
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
     *
     * @param event Mouse event
     * @param startValue Value when drag began
     * @return New normalized value (0.0-1.0)
     */
    virtual float mapDragToValue(const juce::MouseEvent& event, float startValue);

private:
    // AudioProcessorValueTreeState::Listener
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    // Layer management
    std::vector<Layer> layers;
    float currentState = 0.0f;
    float currentAngle = 0.0f;  // Current rotation angle in radians

    // Rotation mapping
    float angleMin = -135.0f * juce::MathConstants<float>::pi / 180.0f;  // -135° (7:30 position)
    float angleMax = +135.0f * juce::MathConstants<float>::pi / 180.0f;  // +135° (4:30 position)

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
    void updateAngleFromState();
    void updateParameterFromMouse(const juce::MouseEvent& event);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LayeredKnob)
};
