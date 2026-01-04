#pragma once

#include <JuceHeader.h>
#include "../dsp/ModulationMatrix.h"

namespace monument
{
namespace ui
{

/**
 * @brief Visual modulation matrix panel with connection grid and controls.
 *
 * Provides a 4×15 grid interface for routing modulation sources to destinations.
 * Shows active connections with depth/smoothing controls for editing.
 */
class ModMatrixPanel : public juce::Component
{
public:
    /**
     * @brief Construct panel with reference to modulation matrix.
     * @param matrix The modulation matrix to control
     */
    explicit ModMatrixPanel(dsp::ModulationMatrix& matrix);
    ~ModMatrixPanel() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    /**
     * @brief Refresh UI to match current modulation connections.
     * Call after loading presets or external changes to matrix.
     */
    void updateFromMatrix();

private:
    dsp::ModulationMatrix& modulationMatrix;

    // Connection grid (4 sources × 15 destinations)
    struct ConnectionButton : public juce::Component
    {
        dsp::ModulationMatrix::SourceType source;
        dsp::ModulationMatrix::DestinationType destination;
        bool isActive{false};
        bool isHovered{false};
        bool isSelected{false};
        std::function<void()> onClick;

        ConnectionButton(dsp::ModulationMatrix::SourceType src,
                        dsp::ModulationMatrix::DestinationType dst)
            : source(src), destination(dst) {}

        void paint(juce::Graphics& g) override;
        void mouseEnter(const juce::MouseEvent&) override { isHovered = true; repaint(); }
        void mouseExit(const juce::MouseEvent&) override { isHovered = false; repaint(); }
        void mouseDown(const juce::MouseEvent&) override { if (onClick) onClick(); }
    };

    juce::OwnedArray<ConnectionButton> connectionButtons;

    // Source and destination labels
    juce::OwnedArray<juce::Label> sourceLabels;
    juce::OwnedArray<juce::Label> destinationLabels;

    // Active connection list display
    juce::Label connectionsLabel;
    juce::TextEditor connectionListDisplay;

    // Connection editing controls (for selected connection)
    juce::Label depthLabel;
    juce::Slider depthSlider;
    juce::Label smoothingLabel;
    juce::Slider smoothingSlider;

    // Currently selected connection (for editing)
    struct SelectedConnection
    {
        dsp::ModulationMatrix::SourceType source;
        dsp::ModulationMatrix::DestinationType destination;
        bool isValid{false};
    };
    SelectedConnection selectedConnection;

    // Callbacks
    void onConnectionButtonClicked(ConnectionButton* button);
    void onDepthChanged();
    void onSmoothingChanged();

    // Helpers
    void setupConnectionGrid();
    void setupControlSliders();
    void setupLabels();
    void refreshConnectionList();
    juce::String getSourceName(dsp::ModulationMatrix::SourceType source) const;
    juce::String getDestinationName(dsp::ModulationMatrix::DestinationType dest) const;
    juce::Colour getSourceColour(dsp::ModulationMatrix::SourceType source) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModMatrixPanel)
};

} // namespace ui
} // namespace monument
