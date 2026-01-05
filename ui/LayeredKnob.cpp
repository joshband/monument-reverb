#include "LayeredKnob.h"

LayeredKnob::LayeredKnob(juce::AudioProcessorValueTreeState& state,
                         const juce::String& parameterId,
                         const juce::String& labelText)
    : apvts(state), paramId(parameterId)
{
    // Label
    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(0xff333333));  // Dark text for white background
    Component::addAndMakeVisible(label);

    // Get parameter
    parameter = apvts.getParameter(parameterId);

    // Initialize state from current parameter value
    if (parameter != nullptr)
        setState(parameter->getValue());

    // Listen for parameter changes (for DAW automation)
    apvts.addParameterListener(parameterId, this);
}

LayeredKnob::~LayeredKnob()
{
    apvts.removeParameterListener(paramId, this);
}

void LayeredKnob::parameterChanged(const juce::String&, float newValue)
{
    setState(newValue);
}

void LayeredKnob::addLayer(const void* imageData, size_t dataSize,
                           bool rotates, float rotationOffset)
{
    // Load image from memory
    juce::Image image = juce::ImageFileFormat::loadFrom(imageData, dataSize);

    if (!image.isValid())
    {
        jassertfalse;  // Invalid image data
        return;
    }

    // Ensure image has alpha channel
    if (image.getFormat() != juce::Image::ARGB)
        image = image.convertedToFormat(juce::Image::ARGB);

    layers.emplace_back(image, rotates, rotationOffset);
}

void LayeredKnob::setRotationRange(float startAngleDegrees, float endAngleDegrees)
{
    angleMin = startAngleDegrees * juce::MathConstants<float>::pi / 180.0f;
    angleMax = endAngleDegrees * juce::MathConstants<float>::pi / 180.0f;
    updateAngleFromState();
    Component::repaint();
}

void LayeredKnob::setState(float normalizedValue)
{
    currentState = juce::jlimit(0.0f, 1.0f, normalizedValue);
    updateAngleFromState();
    Component::repaint();
}

void LayeredKnob::updateAngleFromState()
{
    // Map normalized value (0.0-1.0) to rotation angle
    currentAngle = angleMin + (angleMax - angleMin) * currentState;
}

void LayeredKnob::paint(juce::Graphics& g)
{
    auto bounds = Component::getLocalBounds();

    // Reserve space for label at bottom
    auto controlArea = bounds.removeFromTop(Component::getHeight() - 30);

    if (layers.empty())
    {
        // Fallback: draw placeholder if no layers loaded
        g.setColour(juce::Colour(0xff3a3f46));
        g.fillRoundedRectangle(controlArea.toFloat().reduced(4.0f), 4.0f);
        g.setColour(juce::Colour(0xffe6e1d6));
        g.drawText("No Layers", controlArea, juce::Justification::centred);
        return;
    }

    // Hover effect temporarily disabled
    // if (isHovered)
    // {
    //     g.setColour(juce::Colours::white.withAlpha(0.05f));
    //     g.fillEllipse(controlArea.toFloat().reduced(2.0f));
    // }

    // Calculate rendering bounds (square, centered)
    int size = juce::jmin(controlArea.getWidth(), controlArea.getHeight());
    juce::Rectangle<int> renderBounds(
        controlArea.getCentreX() - size / 2,
        controlArea.getCentreY() - size / 2,
        size,
        size
    );

    float centerX = renderBounds.getCentreX();
    float centerY = renderBounds.getCentreY();

    // Render layers bottom-to-top with alpha blending
    for (const auto& layer : layers)
    {
        if (!layer.image.isValid())
            continue;

        if (layer.rotates)
        {
            // Calculate rotation angle (parameter angle + fixed offset)
            float angle = currentAngle + layer.rotationOffset;

            // Create transform: scale to renderBounds size, rotate around center
            float scale = static_cast<float>(size) / layer.image.getWidth();

            juce::AffineTransform transform =
                juce::AffineTransform::scale(scale)
                .followedBy(juce::AffineTransform::translation(centerX - (layer.image.getWidth() * scale * 0.5f),
                                                                centerY - (layer.image.getHeight() * scale * 0.5f)))
                .followedBy(juce::AffineTransform::rotation(angle, centerX, centerY));

            // Draw rotated layer
            g.drawImageTransformed(layer.image, transform, false);
        }
        else
        {
            // Static layer: draw without rotation (scaled to fit renderBounds)
            g.drawImage(layer.image,
                       renderBounds.toFloat(),
                       juce::RectanglePlacement::centred);
        }
    }
}

void LayeredKnob::resized()
{
    auto area = Component::getLocalBounds();
    label.setBounds(area.removeFromBottom(30));
}

void LayeredKnob::mouseEnter(const juce::MouseEvent&)
{
    isHovered = true;
    Component::repaint();
}

void LayeredKnob::mouseExit(const juce::MouseEvent&)
{
    isHovered = false;
    Component::repaint();
}

void LayeredKnob::mouseDown(const juce::MouseEvent& event)
{
    isDragging = true;
    dragStartY = event.y;
    dragStartValue = currentState;

    // Begin parameter gesture for DAW automation recording
    if (parameter != nullptr)
        parameter->beginChangeGesture();
}

void LayeredKnob::mouseUp(const juce::MouseEvent&)
{
    isDragging = false;

    // End parameter gesture
    if (parameter != nullptr)
        parameter->endChangeGesture();
}

void LayeredKnob::mouseDrag(const juce::MouseEvent& event)
{
    if (!isDragging)
        return;

    updateParameterFromMouse(event);
}

void LayeredKnob::updateParameterFromMouse(const juce::MouseEvent& event)
{
    float newValue = mapDragToValue(event, dragStartValue);
    newValue = juce::jlimit(0.0f, 1.0f, newValue);

    // Update parameter (will trigger setState via listener callback)
    if (parameter != nullptr)
        parameter->setValueNotifyingHost(newValue);
}

float LayeredKnob::mapDragToValue(const juce::MouseEvent& event, float startValue)
{
    // Vertical drag: down = decrease, up = increase
    // Sensitivity: 200 pixels = full range (0.0 to 1.0)
    float dragDelta = (dragStartY - event.y) / 200.0f;
    return startValue + dragDelta;
}
