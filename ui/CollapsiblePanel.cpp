#include "CollapsiblePanel.h"

CollapsiblePanel::CollapsiblePanel(const juce::String& title)
    : panelTitle(title)
{
    currentHeight = collapsedHeight;
    targetHeight = collapsedHeight;
}

CollapsiblePanel::~CollapsiblePanel()
{
    stopTimer();
}

void CollapsiblePanel::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds();
    auto headerBounds = bounds.removeFromTop(headerHeight);

    // Header background
    g.setColour(juce::Colour(headerHovered ? HEADER_HOVER : HEADER_BG));
    g.fillRect(headerBounds);

    // Arrow indicator (▶ or ▼)
    auto arrowBounds = headerBounds.removeFromLeft(headerHeight).reduced(12);
    g.setColour(juce::Colour(ARROW_COLOR));

    juce::Path arrow;
    if (expanded)
    {
        // Down arrow (▼)
        arrow.addTriangle(
            arrowBounds.getX() + 4, arrowBounds.getY() + 4,
            arrowBounds.getRight() - 4, arrowBounds.getY() + 4,
            arrowBounds.getCentreX(), arrowBounds.getBottom() - 4);
    }
    else
    {
        // Right arrow (▶)
        arrow.addTriangle(
            arrowBounds.getX() + 4, arrowBounds.getY() + 4,
            arrowBounds.getX() + 4, arrowBounds.getBottom() - 4,
            arrowBounds.getRight() - 4, arrowBounds.getCentreY());
    }
    g.fillPath(arrow);

    // Title text
    g.setColour(juce::Colour(TEXT_COLOR));
    g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
    g.drawText(panelTitle, headerBounds.reduced(4, 0),
               juce::Justification::centredLeft);

    // Bottom border
    g.setColour(juce::Colour(0xff0d0d0d));
    g.drawHorizontalLine(headerBounds.getBottom(), 0.0f,
                         static_cast<float>(bounds.getWidth()));
}

void CollapsiblePanel::resized()
{
    updateContentBounds();
}

void CollapsiblePanel::mouseDown(const juce::MouseEvent& event)
{
    // Check if click was in header area
    if (event.y <= headerHeight)
    {
        setExpanded(!expanded, true);
    }
}

void CollapsiblePanel::setContentComponent(juce::Component* newContent)
{
    if (contentComponent.get() != nullptr)
        removeChildComponent(contentComponent.get());

    contentComponent.reset(newContent);

    if (contentComponent)
    {
        addAndMakeVisible(contentComponent.get());
        updateContentBounds();
    }
}

void CollapsiblePanel::setExpanded(bool shouldBeExpanded, bool animate)
{
    if (expanded == shouldBeExpanded)
        return;

    expanded = shouldBeExpanded;
    targetHeight = expanded ? expandedHeight : collapsedHeight;

    if (animate)
    {
        animating = true;
        animationProgress = 0.0f;
        startTimerHz(60); // 60 FPS for smooth animation
    }
    else
    {
        currentHeight = targetHeight;
        setSize(getWidth(), static_cast<int>(currentHeight));
        updateContentBounds();
    }

    if (onExpandedChanged)
        onExpandedChanged();

    repaint();
}

void CollapsiblePanel::timerCallback()
{
    if (!animating)
        return;

    animationProgress += 1.0f / 18.0f; // 300ms at 60 FPS

    if (animationProgress >= 1.0f)
    {
        animationProgress = 1.0f;
        animating = false;
        stopTimer();
    }

    // Ease out cubic
    float eased = 1.0f - std::pow(1.0f - animationProgress, 3.0f);

    float startHeight = expanded ? collapsedHeight : expandedHeight;
    currentHeight = startHeight + (targetHeight - startHeight) * eased;

    setSize(getWidth(), static_cast<int>(currentHeight));
    updateContentBounds();

    if (auto* parent = getParentComponent())
        parent->resized();

    repaint();
}

void CollapsiblePanel::updateContentBounds()
{
    if (!contentComponent)
        return;

    auto bounds = getLocalBounds();
    bounds.removeFromTop(headerHeight);

    // Only show content if panel is expanding or expanded
    if (currentHeight > collapsedHeight + 5)
    {
        contentComponent->setBounds(bounds);
        contentComponent->setVisible(true);
    }
    else
    {
        contentComponent->setVisible(false);
    }
}
