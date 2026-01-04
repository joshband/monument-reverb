#include "MonumentControl.h"

MonumentControl::MonumentControl(juce::AudioProcessorValueTreeState& state,
                                 const juce::String& parameterId,
                                 const juce::String& labelText)
    : apvts(state), paramId(parameterId)
{
    // Label
    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centred);
    label.setColour(juce::Label::textColourId, juce::Colour(0xffe6e1d6));
    Component::addAndMakeVisible(label);

    // Get parameter
    parameter = apvts.getParameter(parameterId);

    // Initialize state from current parameter value
    if (parameter != nullptr)
        setState(parameter->getValue());

    // Listen for parameter changes (for DAW automation)
    apvts.addParameterListener(parameterId, this);
}

MonumentControl::~MonumentControl()
{
    apvts.removeParameterListener(paramId, this);
}

void MonumentControl::parameterChanged(const juce::String&, float newValue)
{
    setState(newValue);
}

void MonumentControl::setSpriteSheet(const void* spriteData, size_t dataSize, int numFrames)
{
    frameCount = numFrames;

    // Load sprite sheet from memory
    spriteSheet = juce::ImageFileFormat::loadFrom(spriteData, dataSize);

    if (spriteSheet.isValid())
        extractFramesFromSpriteSheet();
}

void MonumentControl::extractFramesFromSpriteSheet()
{
    cachedFrames.clear();

    if (!spriteSheet.isValid() || frameCount < 1)
        return;

    int frameWidth = spriteSheet.getWidth() / frameCount;
    int frameHeight = spriteSheet.getHeight();

    // Extract each frame
    for (int i = 0; i < frameCount; ++i)
    {
        juce::Image frame = spriteSheet.getClippedImage(
            juce::Rectangle<int>(i * frameWidth, 0, frameWidth, frameHeight)
        );
        cachedFrames.push_back(frame);
    }
}

void MonumentControl::setState(float normalizedValue)
{
    currentState = juce::jlimit(0.0f, 1.0f, normalizedValue);
    Component::repaint();
}

juce::Image MonumentControl::getInterpolatedFrame() const
{
    if (cachedFrames.empty())
        return juce::Image();

    // Map state to frame indices
    float scaledState = currentState * (frameCount - 1);
    int frameA = static_cast<int>(scaledState);
    int frameB = juce::jmin(frameA + 1, frameCount - 1);
    float blend = scaledState - frameA;

    // Simple case: exact frame match
    if (blend < 0.001f || frameA == frameB)
        return cachedFrames[frameA];

    // Interpolate between two frames
    const auto& imgA = cachedFrames[frameA];
    const auto& imgB = cachedFrames[frameB];

    juce::Image result(juce::Image::ARGB, imgA.getWidth(), imgA.getHeight(), true);
    juce::Graphics g(result);

    g.setOpacity(1.0f - blend);
    g.drawImage(imgA, result.getBounds().toFloat());

    g.setOpacity(blend);
    g.drawImage(imgB, result.getBounds().toFloat());

    return result;
}

void MonumentControl::paint(juce::Graphics& g)
{
    // Draw interpolated sprite frame
    auto frame = getInterpolatedFrame();
    auto bounds = Component::getLocalBounds();

    if (frame.isValid())
    {
        // Add subtle hover glow
        if (isHovered)
        {
            g.setColour(juce::Colours::white.withAlpha(0.05f));
            g.fillRoundedRectangle(bounds.toFloat().reduced(2.0f), 4.0f);
        }

        // Draw sprite frame
        auto controlArea = bounds.removeFromTop(Component::getHeight() - 30);  // Leave space for label
        g.drawImage(frame, controlArea.toFloat(),
                    juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize);
    }
    else
    {
        // Fallback: draw placeholder if sprite not loaded
        g.setColour(juce::Colour(0xff3a3f46));
        g.fillRoundedRectangle(bounds.toFloat().reduced(4.0f), 4.0f);
        g.setColour(juce::Colour(0xffe6e1d6));
        g.drawText("Loading...", bounds, juce::Justification::centred);
    }
}

void MonumentControl::resized()
{
    auto area = Component::getLocalBounds();
    label.setBounds(area.removeFromBottom(30));
}

void MonumentControl::mouseEnter(const juce::MouseEvent&)
{
    isHovered = true;
    Component::repaint();
}

void MonumentControl::mouseExit(const juce::MouseEvent&)
{
    isHovered = false;
    Component::repaint();
}

void MonumentControl::mouseDown(const juce::MouseEvent& event)
{
    isDragging = true;
    dragStartY = event.y;
    dragStartValue = currentState;

    // Begin parameter gesture for DAW automation recording
    if (parameter != nullptr)
        parameter->beginChangeGesture();
}

void MonumentControl::mouseUp(const juce::MouseEvent&)
{
    isDragging = false;

    // End parameter gesture
    if (parameter != nullptr)
        parameter->endChangeGesture();
}

void MonumentControl::mouseDrag(const juce::MouseEvent& event)
{
    if (!isDragging)
        return;

    updateParameterFromMouse(event);
}

void MonumentControl::updateParameterFromMouse(const juce::MouseEvent& event)
{
    float newValue = mapDragToValue(event, dragStartValue);
    newValue = juce::jlimit(0.0f, 1.0f, newValue);

    // Update parameter (will trigger setState via listener callback)
    if (parameter != nullptr)
        parameter->setValueNotifyingHost(newValue);
}

float MonumentControl::mapDragToValue(const juce::MouseEvent& event, float startValue)
{
    // Vertical drag: down = decrease, up = increase
    // Sensitivity: 200 pixels = full range (0.0 to 1.0)
    float dragDelta = (dragStartY - event.y) / 200.0f;
    return startValue + dragDelta;
}
