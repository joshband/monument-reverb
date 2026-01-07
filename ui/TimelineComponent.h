#pragma once

#include <JuceHeader.h>
#include "../dsp/SequenceScheduler.h"
#include "../dsp/SequencePresets.h"

namespace monument
{
namespace ui
{

/**
 * @brief Visual timeline editor for SequenceScheduler automation.
 *
 * Provides an interactive timeline interface for creating and editing keyframe-based
 * parameter automation sequences. Features drag-and-drop keyframe editing, real-time
 * preview, multiple parameter lanes, and preset management.
 *
 * Phase 5 - Three-System Plan
 */
class TimelineComponent : public juce::Component,
                          public juce::Timer
{
public:
    /**
     * @brief Construct timeline editor with reference to sequence scheduler.
     * @param scheduler The sequence scheduler to control
     */
    explicit TimelineComponent(dsp::SequenceScheduler& scheduler);
    ~TimelineComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    void visibilityChanged() override;

    /**
     * @brief Refresh UI to match current sequence state.
     * Call after loading presets or external changes to scheduler.
     */
    void updateFromScheduler();

    /**
     * @brief Set timeline zoom level (pixels per beat or second).
     */
    void setZoom(float pixelsPerUnit);

    /**
     * @brief Get current zoom level.
     */
    float getZoom() const noexcept { return pixelsPerUnit; }

private:
    dsp::SequenceScheduler& sequenceScheduler;
    dsp::SequenceScheduler::Sequence currentSequence;

    // UI State
    float pixelsPerUnit{40.0f};     // Zoom: pixels per beat or second
    float scrollOffsetX{0.0f};      // Horizontal scroll position
    int selectedKeyframeIndex{-1};  // Currently selected keyframe (-1 = none)
    bool isDraggingKeyframe{false};
    bool isPlayheadDragging{false};
    juce::Point<int> dragStartPosition;
    double keyframeDragStartTime{0.0};

    // Timeline viewport dimensions
    static constexpr int TIMELINE_HEIGHT = 300;
    static constexpr int RULER_HEIGHT = 30;
    static constexpr int PARAMETER_LANE_HEIGHT = 40;
    static constexpr int TRANSPORT_HEIGHT = 60;
    static constexpr int KEYFRAME_SIZE = 12;

    //==========================================================================
    // Keyframe Visualization
    //==========================================================================

    /**
     * @brief Visual representation of a keyframe marker.
     */
    struct KeyframeMarker : public juce::Component
    {
        int keyframeIndex{-1};
        double time{0.0};
        bool isSelected{false};
        bool isHovered{false};
        dsp::SequenceScheduler::InterpolationType interpolation;
        std::function<void(int)> onSelect;
        std::function<void(int, double)> onDrag;

        KeyframeMarker(int index, double t, dsp::SequenceScheduler::InterpolationType interp)
            : keyframeIndex(index), time(t), interpolation(interp) {}

        void paint(juce::Graphics& g) override;
        void mouseEnter(const juce::MouseEvent&) override;
        void mouseExit(const juce::MouseEvent&) override;
        void mouseDown(const juce::MouseEvent&) override;
        void mouseDrag(const juce::MouseEvent&) override;
    };

    juce::OwnedArray<KeyframeMarker> keyframeMarkers;

    //==========================================================================
    // Timeline Display
    //==========================================================================

    juce::Viewport timelineViewport;
    juce::Component timelineCanvas;

    void paintTimelineRuler(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintParameterLanes(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintPlayheadPosition(juce::Graphics& g, juce::Rectangle<int> bounds);
    void paintKeyframeConnections(juce::Graphics& g, juce::Rectangle<int> bounds);

    //==========================================================================
    // Transport Controls
    //==========================================================================

    juce::Label transportLabel;
    juce::TextButton playPauseButton;
    juce::TextButton stopButton;
    juce::ComboBox loopModeBox;
    juce::ComboBox timingModeBox;
    juce::ToggleButton enabledToggle;

    void onPlayPauseClicked();
    void onStopClicked();
    void onLoopModeChanged();
    void onTimingModeChanged();
    void onEnabledToggled();

    //==========================================================================
    // Preset Management
    //==========================================================================

    juce::Label presetLabel;
    juce::ComboBox presetSelector;
    juce::TextButton saveSequenceButton;
    juce::TextButton newSequenceButton;

    void onPresetSelected();
    void onSaveSequence();
    void onNewSequence();
    void loadFactoryPreset(int presetIndex);
    void refreshPresetList();

    //==========================================================================
    // Keyframe Editing
    //==========================================================================

    juce::Label keyframeLabel;
    juce::Label timePositionLabel;
    juce::Slider timePositionSlider;
    juce::ComboBox interpolationBox;
    juce::TextButton addKeyframeButton;
    juce::TextButton deleteKeyframeButton;

    void onTimePositionChanged();
    void onInterpolationChanged();
    void onAddKeyframe();
    void onDeleteKeyframe();
    void selectKeyframe(int index);
    void updateKeyframeEditor();

    //==========================================================================
    // Parameter Selection
    //==========================================================================

    juce::Label parameterLabel;
    juce::ComboBox parameterSelector;
    juce::Slider parameterValueSlider;
    juce::Label parameterValueLabel;

    void onParameterSelected();
    void onParameterValueChanged();
    void updateParameterEditor();

    //==========================================================================
    // Mouse Interaction
    //==========================================================================

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;

    double pixelToTime(float pixelX) const;
    float timeToPixel(double time) const;
    int getParameterLaneForY(int y) const;

    //==========================================================================
    // Helpers
    //==========================================================================

    void setupTransportControls();
    void setupPresetControls();
    void setupKeyframeEditor();
    void setupParameterEditor();
    void rebuildKeyframeMarkers();
    void syncSequenceToScheduler();

    juce::String getParameterName(dsp::SequenceScheduler::ParameterId param) const;
    juce::Colour getParameterColour(dsp::SequenceScheduler::ParameterId param) const;
    juce::String getInterpolationName(dsp::SequenceScheduler::InterpolationType type) const;
    juce::String getLoopModeName(dsp::SequenceScheduler::PlaybackMode mode) const;
    juce::String getTimingModeName(dsp::SequenceScheduler::TimingMode mode) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TimelineComponent)
};

} // namespace ui
} // namespace monument
