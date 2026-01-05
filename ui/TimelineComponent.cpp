#include "TimelineComponent.h"

namespace monument
{
namespace ui
{

//==============================================================================
// Constructor / Destructor
//==============================================================================

TimelineComponent::TimelineComponent(dsp::SequenceScheduler& scheduler)
    : sequenceScheduler(scheduler)
{
    // Setup UI sections
    setupTransportControls();
    setupPresetControls();
    setupKeyframeEditor();
    setupParameterEditor();

    // Initialize with current sequence from scheduler
    updateFromScheduler();

    // Start timer for playhead animation (30 FPS)
    startTimerHz(30);

    setSize(800, 500);
}

TimelineComponent::~TimelineComponent()
{
    stopTimer();
}

//==============================================================================
// Paint & Layout
//==============================================================================

void TimelineComponent::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xff1a1d23));

    // Timeline area background
    auto timelineBounds = getLocalBounds()
        .removeFromTop(TIMELINE_HEIGHT)
        .reduced(4);

    g.setColour(juce::Colour(0xff0f1115));
    g.fillRect(timelineBounds);

    // Paint timeline components
    paintTimelineRuler(g, timelineBounds.removeFromTop(RULER_HEIGHT));
    paintParameterLanes(g, timelineBounds);
    paintKeyframeConnections(g, timelineBounds);
    paintPlayheadPosition(g, timelineBounds);

    // Section dividers
    g.setColour(juce::Colour(0xff3a3f46));
    g.drawLine(0, TIMELINE_HEIGHT, getWidth(), TIMELINE_HEIGHT, 2.0f);
}

void TimelineComponent::resized()
{
    auto bounds = getLocalBounds();

    // Timeline viewport at top
    auto timelineArea = bounds.removeFromTop(TIMELINE_HEIGHT);
    // (Timeline is drawn in paint, markers are positioned in rebuildKeyframeMarkers)

    bounds.removeFromTop(10); // Spacing

    // Transport controls
    auto transportArea = bounds.removeFromTop(TRANSPORT_HEIGHT);
    {
        auto left = transportArea.removeFromLeft(transportArea.getWidth() / 2).reduced(10, 5);
        auto right = transportArea.removeFromRight(transportArea.getWidth()).reduced(10, 5);

        // Left side: Transport buttons
        transportLabel.setBounds(left.removeFromTop(20));
        auto buttonArea = left.removeFromTop(30);
        playPauseButton.setBounds(buttonArea.removeFromLeft(80));
        stopButton.setBounds(buttonArea.removeFromLeft(80).reduced(2, 0));
        enabledToggle.setBounds(buttonArea.removeFromLeft(100).reduced(2, 0));

        // Right side: Mode selectors
        auto modeArea = right.removeFromTop(25);
        timingModeBox.setBounds(modeArea.removeFromLeft(140));
        loopModeBox.setBounds(modeArea.removeFromLeft(140).reduced(2, 0));
    }

    bounds.removeFromTop(5);

    // Preset management
    auto presetArea = bounds.removeFromTop(60).reduced(10, 5);
    {
        presetLabel.setBounds(presetArea.removeFromTop(20));
        auto presetRow = presetArea.removeFromTop(30);
        presetSelector.setBounds(presetRow.removeFromLeft(200));
        newSequenceButton.setBounds(presetRow.removeFromLeft(100).reduced(2, 0));
        saveSequenceButton.setBounds(presetRow.removeFromLeft(100).reduced(2, 0));
    }

    bounds.removeFromTop(5);

    // Keyframe editing (left column)
    auto editingArea = bounds.reduced(10, 5);
    auto leftColumn = editingArea.removeFromLeft(editingArea.getWidth() / 2).reduced(5, 0);
    auto rightColumn = editingArea.removeFromRight(editingArea.getWidth()).reduced(5, 0);

    // Left: Keyframe editor
    {
        keyframeLabel.setBounds(leftColumn.removeFromTop(20));
        leftColumn.removeFromTop(5);

        auto timeRow = leftColumn.removeFromTop(25);
        timePositionLabel.setBounds(timeRow.removeFromLeft(80));
        timePositionSlider.setBounds(timeRow.removeFromLeft(timeRow.getWidth()));

        leftColumn.removeFromTop(5);
        auto interpRow = leftColumn.removeFromTop(25);
        interpolationBox.setBounds(interpRow.removeFromLeft(150));

        leftColumn.removeFromTop(5);
        auto buttonRow = leftColumn.removeFromTop(30);
        addKeyframeButton.setBounds(buttonRow.removeFromLeft(100));
        deleteKeyframeButton.setBounds(buttonRow.removeFromLeft(100).reduced(2, 0));
    }

    // Right: Parameter editor
    {
        parameterLabel.setBounds(rightColumn.removeFromTop(20));
        rightColumn.removeFromTop(5);

        parameterSelector.setBounds(rightColumn.removeFromTop(25));
        rightColumn.removeFromTop(5);

        auto valueRow = rightColumn.removeFromTop(25);
        parameterValueLabel.setBounds(valueRow.removeFromLeft(80));
        parameterValueSlider.setBounds(valueRow.removeFromLeft(valueRow.getWidth()));
    }

    // Rebuild keyframe marker positions
    rebuildKeyframeMarkers();
}

void TimelineComponent::timerCallback()
{
    // Update playhead position during playback
    if (sequenceScheduler.isEnabled())
    {
        repaint();
    }
}

//==============================================================================
// Timeline Painting
//==============================================================================

void TimelineComponent::paintTimelineRuler(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    g.setColour(juce::Colour(0xff2a2f36));
    g.fillRect(bounds);

    // Draw time markers
    g.setColour(juce::Colour(0xff666666));
    g.setFont(10.0f);

    bool isBeats = (currentSequence.timingMode == dsp::SequenceScheduler::TimingMode::Beats);
    double maxTime = isBeats ? currentSequence.durationBeats : currentSequence.durationSeconds;
    double increment = (pixelsPerUnit < 30.0f) ? 4.0 : 1.0; // Sparser labels when zoomed out

    for (double t = 0; t <= maxTime; t += increment)
    {
        float x = timeToPixel(t);
        if (x >= bounds.getX() && x <= bounds.getRight())
        {
            g.drawLine(x, bounds.getY(), x, bounds.getBottom(), 1.0f);
            juce::String label = isBeats
                ? juce::String(static_cast<int>(t))
                : juce::String(t, 1) + "s";
            g.drawText(label, static_cast<int>(x - 20), bounds.getY() + 5, 40, 20,
                      juce::Justification::centred);
        }
    }
}

void TimelineComponent::paintParameterLanes(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    if (currentSequence.keyframes.empty())
        return;

    // Collect all parameters being automated
    std::set<dsp::SequenceScheduler::ParameterId> automatedParams;
    for (const auto& kf : currentSequence.keyframes)
    {
        for (const auto& [param, value] : kf.parameterValues)
        {
            automatedParams.insert(param);
        }
    }

    // Draw parameter automation curves
    int laneIndex = 0;
    for (auto param : automatedParams)
    {
        int laneY = bounds.getY() + laneIndex * PARAMETER_LANE_HEIGHT;
        if (laneY >= bounds.getBottom())
            break;

        juce::Colour paramColour = getParameterColour(param);

        // Draw lane background
        g.setColour(paramColour.withAlpha(0.1f));
        g.fillRect(bounds.getX(), laneY, bounds.getWidth(), PARAMETER_LANE_HEIGHT);

        // Draw automation curve
        juce::Path curvePath;
        bool firstPoint = true;

        for (const auto& kf : currentSequence.keyframes)
        {
            auto paramValue = kf.getParameter(param);
            if (paramValue.has_value())
            {
                float x = timeToPixel(kf.time);
                float y = laneY + PARAMETER_LANE_HEIGHT - (paramValue.value() * PARAMETER_LANE_HEIGHT);

                if (firstPoint)
                {
                    curvePath.startNewSubPath(x, y);
                    firstPoint = false;
                }
                else
                {
                    curvePath.lineTo(x, y);
                }
            }
        }

        g.setColour(paramColour);
        g.strokePath(curvePath, juce::PathStrokeType(2.0f));

        // Draw parameter name
        g.setColour(paramColour.withAlpha(0.7f));
        g.setFont(11.0f);
        g.drawText(getParameterName(param), bounds.getX() + 5, laneY + 2, 150, 18,
                  juce::Justification::centredLeft);

        laneIndex++;
    }
}

void TimelineComponent::paintPlayheadPosition(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    double currentPos = sequenceScheduler.getCurrentPosition();
    float x = timeToPixel(currentPos);

    // Draw playhead line
    g.setColour(juce::Colours::white.withAlpha(0.8f));
    g.drawLine(x, bounds.getY(), x, bounds.getBottom(), 2.0f);

    // Draw playhead triangle at top
    juce::Path triangle;
    triangle.addTriangle(x - 6, bounds.getY(), x + 6, bounds.getY(), x, bounds.getY() + 10);
    g.fillPath(triangle);
}

void TimelineComponent::paintKeyframeConnections(juce::Graphics& g, juce::Rectangle<int> bounds)
{
    // Draw lines connecting keyframes in sequence
    if (currentSequence.keyframes.size() < 2)
        return;

    g.setColour(juce::Colour(0xff4a4f56).withAlpha(0.5f));

    for (size_t i = 0; i < currentSequence.keyframes.size() - 1; ++i)
    {
        float x1 = timeToPixel(currentSequence.keyframes[i].time);
        float x2 = timeToPixel(currentSequence.keyframes[i + 1].time);
        int y = bounds.getY() + 20;

        g.drawLine(x1, y, x2, y, 1.0f);
    }
}

//==============================================================================
// Keyframe Marker Implementation
//==============================================================================

void TimelineComponent::KeyframeMarker::paint(juce::Graphics& g)
{
    // Draw diamond shape for keyframe
    juce::Path diamond;
    float cx = getWidth() / 2.0f;
    float cy = getHeight() / 2.0f;
    float size = KEYFRAME_SIZE / 2.0f;

    diamond.startNewSubPath(cx, cy - size);
    diamond.lineTo(cx + size, cy);
    diamond.lineTo(cx, cy + size);
    diamond.lineTo(cx - size, cy);
    diamond.closeSubPath();

    // Color based on state
    juce::Colour colour = juce::Colour(0xff4a9eff); // Blue
    if (isSelected)
        colour = juce::Colours::orange;
    else if (isHovered)
        colour = colour.brighter(0.3f);

    g.setColour(colour);
    g.fillPath(diamond);

    // Outline
    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.strokePath(diamond, juce::PathStrokeType(1.0f));

    // Draw interpolation type indicator (small glyph)
    g.setFont(8.0f);
    juce::String interp;
    switch (interpolation)
    {
        case dsp::SequenceScheduler::InterpolationType::Linear:     interp = "L"; break;
        case dsp::SequenceScheduler::InterpolationType::Exponential: interp = "E"; break;
        case dsp::SequenceScheduler::InterpolationType::SCurve:     interp = "S"; break;
        case dsp::SequenceScheduler::InterpolationType::Step:       interp = "T"; break;
        default: interp = "?"; break;
    }
    g.drawText(interp, getLocalBounds(), juce::Justification::centred);
}

void TimelineComponent::KeyframeMarker::mouseEnter(const juce::MouseEvent&)
{
    isHovered = true;
    repaint();
}

void TimelineComponent::KeyframeMarker::mouseExit(const juce::MouseEvent&)
{
    isHovered = false;
    repaint();
}

void TimelineComponent::KeyframeMarker::mouseDown(const juce::MouseEvent&)
{
    if (onSelect)
        onSelect(keyframeIndex);
}

void TimelineComponent::KeyframeMarker::mouseDrag(const juce::MouseEvent& event)
{
    if (onDrag)
    {
        // Calculate new time based on drag position
        double newTime = time + (event.getDistanceFromDragStartX() / 40.0); // TODO: use actual zoom
        newTime = juce::jmax(0.0, newTime);
        onDrag(keyframeIndex, newTime);
    }
}

//==============================================================================
// Transport Controls
//==============================================================================

void TimelineComponent::setupTransportControls()
{
    // Transport label
    transportLabel.setText("Transport", juce::dontSendNotification);
    transportLabel.setJustificationType(juce::Justification::centredLeft);
    transportLabel.setColour(juce::Label::textColourId, juce::Colour(0xffaaaaaa));
    addAndMakeVisible(transportLabel);

    // Play/Pause button
    playPauseButton.setButtonText("Play");
    playPauseButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2f36));
    playPauseButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe6e1d6));
    playPauseButton.onClick = [this]() { onPlayPauseClicked(); };
    addAndMakeVisible(playPauseButton);

    // Stop button
    stopButton.setButtonText("Stop");
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2f36));
    stopButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe6e1d6));
    stopButton.onClick = [this]() { onStopClicked(); };
    addAndMakeVisible(stopButton);

    // Enabled toggle
    enabledToggle.setButtonText("Enable");
    enabledToggle.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffe6e1d6));
    enabledToggle.setToggleState(currentSequence.enabled, juce::dontSendNotification);
    enabledToggle.onClick = [this]() { onEnabledToggled(); };
    addAndMakeVisible(enabledToggle);

    // Loop mode selector
    loopModeBox.addItem("One-Shot", 1);
    loopModeBox.addItem("Loop", 2);
    loopModeBox.addItem("Ping-Pong", 3);
    loopModeBox.setSelectedId(static_cast<int>(currentSequence.playbackMode) + 1, juce::dontSendNotification);
    loopModeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff14171b));
    loopModeBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xffe6e1d6));
    loopModeBox.onChange = [this]() { onLoopModeChanged(); };
    addAndMakeVisible(loopModeBox);

    // Timing mode selector
    timingModeBox.addItem("Beats (Tempo Sync)", 1);
    timingModeBox.addItem("Seconds (Free)", 2);
    timingModeBox.setSelectedId(static_cast<int>(currentSequence.timingMode) + 1, juce::dontSendNotification);
    timingModeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff14171b));
    timingModeBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xffe6e1d6));
    timingModeBox.onChange = [this]() { onTimingModeChanged(); };
    addAndMakeVisible(timingModeBox);
}

void TimelineComponent::onPlayPauseClicked()
{
    currentSequence.enabled = !currentSequence.enabled;
    sequenceScheduler.setEnabled(currentSequence.enabled);

    playPauseButton.setButtonText(currentSequence.enabled ? "Pause" : "Play");
    enabledToggle.setToggleState(currentSequence.enabled, juce::dontSendNotification);
}

void TimelineComponent::onStopClicked()
{
    currentSequence.enabled = false;
    sequenceScheduler.setEnabled(false);
    sequenceScheduler.reset();

    playPauseButton.setButtonText("Play");
    enabledToggle.setToggleState(false, juce::dontSendNotification);
    repaint();
}

void TimelineComponent::onLoopModeChanged()
{
    int selectedId = loopModeBox.getSelectedId();
    if (selectedId > 0)
    {
        currentSequence.playbackMode = static_cast<dsp::SequenceScheduler::PlaybackMode>(selectedId - 1);
        syncSequenceToScheduler();
    }
}

void TimelineComponent::onTimingModeChanged()
{
    int selectedId = timingModeBox.getSelectedId();
    if (selectedId > 0)
    {
        currentSequence.timingMode = static_cast<dsp::SequenceScheduler::TimingMode>(selectedId - 1);
        syncSequenceToScheduler();
        repaint(); // Redraw ruler with new timing
    }
}

void TimelineComponent::onEnabledToggled()
{
    currentSequence.enabled = enabledToggle.getToggleState();
    sequenceScheduler.setEnabled(currentSequence.enabled);
    playPauseButton.setButtonText(currentSequence.enabled ? "Pause" : "Play");
}

//==============================================================================
// Preset Management
//==============================================================================

void TimelineComponent::setupPresetControls()
{
    presetLabel.setText("Sequence Presets", juce::dontSendNotification);
    presetLabel.setJustificationType(juce::Justification::centredLeft);
    presetLabel.setColour(juce::Label::textColourId, juce::Colour(0xffaaaaaa));
    addAndMakeVisible(presetLabel);

    // Preset selector
    presetSelector.setTextWhenNothingSelected("Select Preset...");
    presetSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff14171b));
    presetSelector.setColour(juce::ComboBox::textColourId, juce::Colour(0xffe6e1d6));
    presetSelector.onChange = [this]() { onPresetSelected(); };
    addAndMakeVisible(presetSelector);
    refreshPresetList();

    // New sequence button
    newSequenceButton.setButtonText("New");
    newSequenceButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2f36));
    newSequenceButton.onClick = [this]() { onNewSequence(); };
    addAndMakeVisible(newSequenceButton);

    // Save sequence button
    saveSequenceButton.setButtonText("Save");
    saveSequenceButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2f36));
    saveSequenceButton.onClick = [this]() { onSaveSequence(); };
    addAndMakeVisible(saveSequenceButton);
}

void TimelineComponent::refreshPresetList()
{
    presetSelector.clear();

    // Add factory presets
    for (int i = 0; i < dsp::SequencePresets::getNumPresets(); ++i)
    {
        presetSelector.addItem(dsp::SequencePresets::getPresetName(i), i + 1);
    }

    // TODO: Add user presets from file system
}

void TimelineComponent::onPresetSelected()
{
    int selectedId = presetSelector.getSelectedId();
    if (selectedId > 0)
    {
        loadFactoryPreset(selectedId - 1);
    }
}

void TimelineComponent::loadFactoryPreset(int presetIndex)
{
    if (presetIndex >= 0 && presetIndex < dsp::SequencePresets::getNumPresets())
    {
        currentSequence = dsp::SequencePresets::getPreset(presetIndex);
        syncSequenceToScheduler();
        updateFromScheduler();
    }
}

void TimelineComponent::onNewSequence()
{
    currentSequence = dsp::SequenceScheduler::Sequence("New Sequence");
    currentSequence.timingMode = dsp::SequenceScheduler::TimingMode::Beats;
    currentSequence.playbackMode = dsp::SequenceScheduler::PlaybackMode::Loop;
    currentSequence.durationBeats = 16.0;
    currentSequence.enabled = false;

    // Add initial keyframe at time 0
    dsp::SequenceScheduler::Keyframe initialKeyframe(0.0);
    currentSequence.addKeyframe(initialKeyframe);

    syncSequenceToScheduler();
    updateFromScheduler();
}

void TimelineComponent::onSaveSequence()
{
    // TODO: Implement save to file system
    juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon,
        "Save Sequence",
        "User sequence saving will be implemented in future update.",
        "OK");
}

//==============================================================================
// Keyframe Editing
//==============================================================================

void TimelineComponent::setupKeyframeEditor()
{
    keyframeLabel.setText("Keyframe Editor", juce::dontSendNotification);
    keyframeLabel.setJustificationType(juce::Justification::centredLeft);
    keyframeLabel.setColour(juce::Label::textColourId, juce::Colour(0xffaaaaaa));
    addAndMakeVisible(keyframeLabel);

    // Time position label
    timePositionLabel.setText("Time:", juce::dontSendNotification);
    timePositionLabel.setJustificationType(juce::Justification::centredRight);
    timePositionLabel.setColour(juce::Label::textColourId, juce::Colour(0xffaaaaaa));
    addAndMakeVisible(timePositionLabel);

    // Time position slider
    timePositionSlider.setRange(0.0, 32.0, 0.1);
    timePositionSlider.setValue(0.0);
    timePositionSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    timePositionSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    timePositionSlider.onValueChange = [this]() { onTimePositionChanged(); };
    addAndMakeVisible(timePositionSlider);

    // Interpolation selector
    interpolationBox.addItem("Linear", 1);
    interpolationBox.addItem("Exponential", 2);
    interpolationBox.addItem("S-Curve", 3);
    interpolationBox.addItem("Step", 4);
    interpolationBox.setSelectedId(1, juce::dontSendNotification);
    interpolationBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff14171b));
    interpolationBox.onChange = [this]() { onInterpolationChanged(); };
    addAndMakeVisible(interpolationBox);

    // Add keyframe button
    addKeyframeButton.setButtonText("Add");
    addKeyframeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a7a46));
    addKeyframeButton.onClick = [this]() { onAddKeyframe(); };
    addAndMakeVisible(addKeyframeButton);

    // Delete keyframe button
    deleteKeyframeButton.setButtonText("Delete");
    deleteKeyframeButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff7a2a2a));
    deleteKeyframeButton.onClick = [this]() { onDeleteKeyframe(); };
    addAndMakeVisible(deleteKeyframeButton);

    updateKeyframeEditor();
}

void TimelineComponent::onTimePositionChanged()
{
    if (selectedKeyframeIndex >= 0 &&
        selectedKeyframeIndex < static_cast<int>(currentSequence.keyframes.size()))
    {
        currentSequence.keyframes[selectedKeyframeIndex].time = timePositionSlider.getValue();

        // Re-sort keyframes by time
        std::sort(currentSequence.keyframes.begin(), currentSequence.keyframes.end(),
            [](const auto& a, const auto& b) { return a.time < b.time; });

        syncSequenceToScheduler();
        rebuildKeyframeMarkers();
        repaint();
    }
}

void TimelineComponent::onInterpolationChanged()
{
    if (selectedKeyframeIndex >= 0 &&
        selectedKeyframeIndex < static_cast<int>(currentSequence.keyframes.size()))
    {
        int selectedId = interpolationBox.getSelectedId();
        if (selectedId > 0)
        {
            currentSequence.keyframes[selectedKeyframeIndex].interpolation =
                static_cast<dsp::SequenceScheduler::InterpolationType>(selectedId - 1);
            syncSequenceToScheduler();
            rebuildKeyframeMarkers();
            repaint();
        }
    }
}

void TimelineComponent::onAddKeyframe()
{
    double time = timePositionSlider.getValue();
    dsp::SequenceScheduler::Keyframe newKeyframe(time);

    // If there's a selected parameter, add it with current slider value
    int paramId = parameterSelector.getSelectedId();
    if (paramId > 0)
    {
        auto param = static_cast<dsp::SequenceScheduler::ParameterId>(paramId - 1);
        float value = static_cast<float>(parameterValueSlider.getValue());
        newKeyframe.setParameter(param, value);
    }

    currentSequence.addKeyframe(newKeyframe);
    syncSequenceToScheduler();
    rebuildKeyframeMarkers();

    // Select the new keyframe
    selectedKeyframeIndex = static_cast<int>(currentSequence.keyframes.size()) - 1;
    updateKeyframeEditor();
    repaint();
}

void TimelineComponent::onDeleteKeyframe()
{
    if (selectedKeyframeIndex >= 0 &&
        selectedKeyframeIndex < static_cast<int>(currentSequence.keyframes.size()))
    {
        currentSequence.removeKeyframe(static_cast<size_t>(selectedKeyframeIndex));
        syncSequenceToScheduler();
        rebuildKeyframeMarkers();

        selectedKeyframeIndex = -1;
        updateKeyframeEditor();
        repaint();
    }
}

void TimelineComponent::selectKeyframe(int index)
{
    selectedKeyframeIndex = index;
    updateKeyframeEditor();
    repaint();
}

void TimelineComponent::updateKeyframeEditor()
{
    bool hasSelection = (selectedKeyframeIndex >= 0 &&
                        selectedKeyframeIndex < static_cast<int>(currentSequence.keyframes.size()));

    timePositionSlider.setEnabled(hasSelection);
    interpolationBox.setEnabled(hasSelection);
    deleteKeyframeButton.setEnabled(hasSelection);

    if (hasSelection)
    {
        const auto& kf = currentSequence.keyframes[selectedKeyframeIndex];
        timePositionSlider.setValue(kf.time, juce::dontSendNotification);
        interpolationBox.setSelectedId(static_cast<int>(kf.interpolation) + 1, juce::dontSendNotification);
    }
}

//==============================================================================
// Parameter Editing
//==============================================================================

void TimelineComponent::setupParameterEditor()
{
    parameterLabel.setText("Parameter Editor", juce::dontSendNotification);
    parameterLabel.setJustificationType(juce::Justification::centredLeft);
    parameterLabel.setColour(juce::Label::textColourId, juce::Colour(0xffaaaaaa));
    addAndMakeVisible(parameterLabel);

    // Parameter selector
    parameterSelector.setTextWhenNothingSelected("Select Parameter...");
    parameterSelector.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff14171b));

    // Add all parameter types
    for (int i = 0; i < static_cast<int>(dsp::SequenceScheduler::ParameterId::Count); ++i)
    {
        auto param = static_cast<dsp::SequenceScheduler::ParameterId>(i);
        parameterSelector.addItem(getParameterName(param), i + 1);
    }

    parameterSelector.onChange = [this]() { onParameterSelected(); };
    addAndMakeVisible(parameterSelector);

    // Parameter value label
    parameterValueLabel.setText("Value:", juce::dontSendNotification);
    parameterValueLabel.setJustificationType(juce::Justification::centredRight);
    parameterValueLabel.setColour(juce::Label::textColourId, juce::Colour(0xffaaaaaa));
    addAndMakeVisible(parameterValueLabel);

    // Parameter value slider
    parameterValueSlider.setRange(0.0, 1.0, 0.01);
    parameterValueSlider.setValue(0.5);
    parameterValueSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    parameterValueSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    parameterValueSlider.onValueChange = [this]() { onParameterValueChanged(); };
    addAndMakeVisible(parameterValueSlider);

    updateParameterEditor();
}

void TimelineComponent::onParameterSelected()
{
    updateParameterEditor();
}

void TimelineComponent::onParameterValueChanged()
{
    if (selectedKeyframeIndex >= 0 &&
        selectedKeyframeIndex < static_cast<int>(currentSequence.keyframes.size()))
    {
        int paramId = parameterSelector.getSelectedId();
        if (paramId > 0)
        {
            auto param = static_cast<dsp::SequenceScheduler::ParameterId>(paramId - 1);
            float value = static_cast<float>(parameterValueSlider.getValue());

            currentSequence.keyframes[selectedKeyframeIndex].setParameter(param, value);
            syncSequenceToScheduler();
            repaint();
        }
    }
}

void TimelineComponent::updateParameterEditor()
{
    bool hasSelection = (selectedKeyframeIndex >= 0 &&
                        selectedKeyframeIndex < static_cast<int>(currentSequence.keyframes.size()));

    parameterValueSlider.setEnabled(hasSelection && parameterSelector.getSelectedId() > 0);

    if (hasSelection && parameterSelector.getSelectedId() > 0)
    {
        auto param = static_cast<dsp::SequenceScheduler::ParameterId>(parameterSelector.getSelectedId() - 1);
        const auto& kf = currentSequence.keyframes[selectedKeyframeIndex];
        auto value = kf.getParameter(param);

        if (value.has_value())
        {
            parameterValueSlider.setValue(value.value(), juce::dontSendNotification);
        }
        else
        {
            parameterValueSlider.setValue(0.5, juce::dontSendNotification);
        }
    }
}

//==============================================================================
// Mouse Interaction
//==============================================================================

void TimelineComponent::mouseDown(const juce::MouseEvent& event)
{
    // Check if clicking on playhead to scrub
    double currentPos = sequenceScheduler.getCurrentPosition();
    float playheadX = timeToPixel(currentPos);

    if (std::abs(event.position.x - playheadX) < 10.0f && event.position.y < TIMELINE_HEIGHT)
    {
        isPlayheadDragging = true;
        return;
    }

    // Otherwise, deselect keyframe
    if (event.mods.isLeftButtonDown())
    {
        selectedKeyframeIndex = -1;
        updateKeyframeEditor();
        repaint();
    }
}

void TimelineComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (isPlayheadDragging)
    {
        double newPosition = pixelToTime(event.position.x);
        bool isBeats = (currentSequence.timingMode == dsp::SequenceScheduler::TimingMode::Beats);
        double maxTime = isBeats ? currentSequence.durationBeats : currentSequence.durationSeconds;

        newPosition = juce::jlimit(0.0, maxTime, newPosition);
        sequenceScheduler.setCurrentPosition(newPosition);
        repaint();
    }
}

void TimelineComponent::mouseUp(const juce::MouseEvent&)
{
    isPlayheadDragging = false;
    isDraggingKeyframe = false;
}

void TimelineComponent::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    // Zoom with mouse wheel
    if (event.mods.isCommandDown() || event.mods.isCtrlDown())
    {
        float zoomDelta = wheel.deltaY * 5.0f;
        setZoom(pixelsPerUnit + zoomDelta);
        repaint();
    }
}

//==============================================================================
// Helpers
//==============================================================================

void TimelineComponent::updateFromScheduler()
{
    currentSequence = sequenceScheduler.getSequence();

    // Update UI controls
    enabledToggle.setToggleState(currentSequence.enabled, juce::dontSendNotification);
    playPauseButton.setButtonText(currentSequence.enabled ? "Pause" : "Play");
    loopModeBox.setSelectedId(static_cast<int>(currentSequence.playbackMode) + 1, juce::dontSendNotification);
    timingModeBox.setSelectedId(static_cast<int>(currentSequence.timingMode) + 1, juce::dontSendNotification);

    // Update timeline range
    double maxTime = (currentSequence.timingMode == dsp::SequenceScheduler::TimingMode::Beats)
        ? currentSequence.durationBeats
        : currentSequence.durationSeconds;
    timePositionSlider.setRange(0.0, maxTime, 0.1);

    rebuildKeyframeMarkers();
    repaint();
}

void TimelineComponent::setZoom(float pixelsPerUnit)
{
    this->pixelsPerUnit = juce::jlimit(10.0f, 200.0f, pixelsPerUnit);
    rebuildKeyframeMarkers();
}

void TimelineComponent::rebuildKeyframeMarkers()
{
    keyframeMarkers.clear();

    for (size_t i = 0; i < currentSequence.keyframes.size(); ++i)
    {
        const auto& kf = currentSequence.keyframes[i];

        auto* marker = new KeyframeMarker(
            static_cast<int>(i),
            kf.time,
            kf.interpolation
        );

        marker->isSelected = (static_cast<int>(i) == selectedKeyframeIndex);
        marker->onSelect = [this](int index) { selectKeyframe(index); };
        marker->onDrag = [this](int index, double newTime) {
            if (index >= 0 && index < static_cast<int>(currentSequence.keyframes.size()))
            {
                currentSequence.keyframes[index].time = newTime;
                syncSequenceToScheduler();
                rebuildKeyframeMarkers();
                repaint();
            }
        };

        float x = timeToPixel(kf.time);
        int y = RULER_HEIGHT + 10;
        marker->setBounds(static_cast<int>(x - KEYFRAME_SIZE / 2), y, KEYFRAME_SIZE, KEYFRAME_SIZE);

        addAndMakeVisible(marker);
        keyframeMarkers.add(marker);
    }
}

void TimelineComponent::syncSequenceToScheduler()
{
    sequenceScheduler.loadSequence(currentSequence);
}

double TimelineComponent::pixelToTime(float pixelX) const
{
    return static_cast<double>(pixelX) / pixelsPerUnit;
}

float TimelineComponent::timeToPixel(double time) const
{
    return static_cast<float>(time) * pixelsPerUnit;
}

juce::String TimelineComponent::getParameterName(dsp::SequenceScheduler::ParameterId param) const
{
    return dsp::SequenceScheduler::parameterIdToString(param);
}

juce::Colour TimelineComponent::getParameterColour(dsp::SequenceScheduler::ParameterId param) const
{
    // Color-code parameters by category
    int paramIndex = static_cast<int>(param);

    // Spatial parameters (green)
    if (paramIndex >= static_cast<int>(dsp::SequenceScheduler::ParameterId::PositionX))
        return juce::Colour(0xff4aff4a);

    // Macro parameters (blue)
    if (paramIndex >= static_cast<int>(dsp::SequenceScheduler::ParameterId::Material))
        return juce::Colour(0xff4a9eff);

    // Memory parameters (purple)
    if (paramIndex >= static_cast<int>(dsp::SequenceScheduler::ParameterId::Memory))
        return juce::Colour(0xffb44aff);

    // Base parameters (orange)
    return juce::Colour(0xffff9e4a);
}

juce::String TimelineComponent::getInterpolationName(dsp::SequenceScheduler::InterpolationType type) const
{
    switch (type)
    {
        case dsp::SequenceScheduler::InterpolationType::Linear:     return "Linear";
        case dsp::SequenceScheduler::InterpolationType::Exponential: return "Exponential";
        case dsp::SequenceScheduler::InterpolationType::SCurve:     return "S-Curve";
        case dsp::SequenceScheduler::InterpolationType::Step:       return "Step";
        default: return "Unknown";
    }
}

juce::String TimelineComponent::getLoopModeName(dsp::SequenceScheduler::PlaybackMode mode) const
{
    switch (mode)
    {
        case dsp::SequenceScheduler::PlaybackMode::OneShot:  return "One-Shot";
        case dsp::SequenceScheduler::PlaybackMode::Loop:     return "Loop";
        case dsp::SequenceScheduler::PlaybackMode::PingPong: return "Ping-Pong";
        default: return "Unknown";
    }
}

juce::String TimelineComponent::getTimingModeName(dsp::SequenceScheduler::TimingMode mode) const
{
    switch (mode)
    {
        case dsp::SequenceScheduler::TimingMode::Beats:   return "Beats";
        case dsp::SequenceScheduler::TimingMode::Seconds: return "Seconds";
        default: return "Unknown";
    }
}

} // namespace ui
} // namespace monument
