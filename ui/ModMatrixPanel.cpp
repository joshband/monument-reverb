#include "ModMatrixPanel.h"

namespace monument
{
namespace ui
{

ModMatrixPanel::ModMatrixPanel(dsp::ModulationMatrix& matrix)
    : modulationMatrix(matrix)
{
    // Setup labels first
    setupLabels();

    // Setup connection grid
    setupConnectionGrid();

    // Setup connection list display
    connectionsLabel.setText("Active Connections:", juce::dontSendNotification);
    connectionsLabel.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    connectionsLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa8a49c));
    addAndMakeVisible(connectionsLabel);

    connectionListDisplay.setMultiLine(true);
    connectionListDisplay.setReadOnly(true);
    connectionListDisplay.setScrollbarsShown(true);
    connectionListDisplay.setCaretVisible(false);
    connectionListDisplay.setColour(juce::TextEditor::backgroundColourId, juce::Colour(0xff14171b));
    connectionListDisplay.setColour(juce::TextEditor::textColourId, juce::Colour(0xffe6e1d6));
    connectionListDisplay.setColour(juce::TextEditor::outlineColourId, juce::Colour(0xff3a3f46));
    connectionListDisplay.setFont(juce::Font(juce::FontOptions(juce::Font::getDefaultMonospacedFontName(), 11.0f, juce::Font::plain)));
    addAndMakeVisible(connectionListDisplay);

    // Setup control sliders
    setupControlSliders();

    // Initial refresh
    updateFromMatrix();
}

// ConnectionButton paint implementation
void ModMatrixPanel::ConnectionButton::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Get source color
    juce::Colour baseColour;
    switch (source)
    {
        case dsp::ModulationMatrix::SourceType::ChaosAttractor:
            baseColour = juce::Colour(0xffe89547); // Orange
            break;
        case dsp::ModulationMatrix::SourceType::AudioFollower:
            baseColour = juce::Colour(0xff6bc47d); // Green
            break;
        case dsp::ModulationMatrix::SourceType::BrownianMotion:
            baseColour = juce::Colour(0xffa47bd1); // Purple
            break;
        case dsp::ModulationMatrix::SourceType::EnvelopeTracker:
            baseColour = juce::Colour(0xff6b9bd1); // Blue
            break;
        default:
            baseColour = juce::Colour(0xff6b9bd1);
    }

    // Background
    if (isActive)
    {
        g.setColour(baseColour.withAlpha(isSelected ? 0.8f : 0.6f));
        g.fillRoundedRectangle(bounds, 3.0f);
    }
    else if (isHovered)
    {
        g.setColour(baseColour.withAlpha(0.2f));
        g.fillRoundedRectangle(bounds, 3.0f);
    }

    // Border
    g.setColour(isActive ? baseColour : juce::Colour(0xff3a3f46));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 3.0f, isActive ? 2.0f : 1.0f);

    // Selection indicator (dot in center)
    if (isSelected && isActive)
    {
        g.setColour(juce::Colours::white);
        auto center = bounds.getCentre();
        g.fillEllipse(center.x - 2, center.y - 2, 4, 4);
    }
}

void ModMatrixPanel::paint(juce::Graphics& g)
{
    // Background
    g.fillAll(juce::Colour(0xff0d0f12));

    // Title
    g.setFont(juce::Font(juce::FontOptions(16.0f, juce::Font::bold)));
    g.setColour(juce::Colour(0xffe6e1d6));
    g.drawText("MODULATION MATRIX", 10, 5, getWidth() - 20, 25, juce::Justification::centredLeft);

    // Separator line
    g.setColour(juce::Colour(0xff3a3f46));
    g.drawLine(10.0f, 32.0f, static_cast<float>(getWidth() - 10), 32.0f, 1.0f);
}

void ModMatrixPanel::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    bounds.removeFromTop(35); // Title area

    // Connection grid area
    auto gridArea = bounds.removeFromTop(220);
    auto sourceLabelsArea = gridArea.removeFromLeft(90);
    gridArea.removeFromLeft(5); // Spacing

    const int numSources = static_cast<int>(dsp::ModulationMatrix::SourceType::Count);
    const int numDestinations = static_cast<int>(dsp::ModulationMatrix::DestinationType::Count);
    const int buttonSize = 20;
    const int spacing = 3;

    // Layout destination labels (header row)
    auto destHeaderArea = gridArea.removeFromTop(35);
    for (int d = 0; d < numDestinations && d < destinationLabels.size(); ++d)
    {
        auto labelBounds = destHeaderArea.removeFromLeft(buttonSize + spacing);
        destinationLabels[d]->setBounds(labelBounds.withSize(buttonSize, 35));
    }

    gridArea.removeFromTop(5); // Spacing

    // Layout source labels and connection buttons
    int buttonIndex = 0;
    for (int s = 0; s < numSources; ++s)
    {
        auto rowBounds = gridArea.removeFromTop(buttonSize + spacing);

        // Source label
        if (s < sourceLabels.size())
        {
            auto labelBounds = sourceLabelsArea.removeFromTop(buttonSize + spacing);
            sourceLabels[s]->setBounds(labelBounds.withTrimmedTop(2).withWidth(85));
        }

        // Connection buttons for this row
        for (int d = 0; d < numDestinations; ++d)
        {
            if (buttonIndex < connectionButtons.size())
            {
                auto buttonBounds = rowBounds.removeFromLeft(buttonSize + spacing);
                connectionButtons[buttonIndex]->setBounds(buttonBounds.withSize(buttonSize, buttonSize));
                buttonIndex++;
            }
        }
    }

    bounds.removeFromTop(15); // Spacing

    // Connection list area
    connectionsLabel.setBounds(bounds.removeFromTop(22));
    connectionListDisplay.setBounds(bounds.removeFromTop(110));

    bounds.removeFromTop(10); // Spacing

    // Control sliders area
    auto controlsArea = bounds.removeFromTop(75);

    depthLabel.setBounds(controlsArea.removeFromTop(18));
    depthSlider.setBounds(controlsArea.removeFromTop(24));

    controlsArea.removeFromTop(4);

    smoothingLabel.setBounds(controlsArea.removeFromTop(18));
    smoothingSlider.setBounds(controlsArea.removeFromTop(24));
}

void ModMatrixPanel::updateFromMatrix()
{
    // Update button states from current connections
    const auto& connections = modulationMatrix.getConnections();

    // Reset all buttons
    for (auto* button : connectionButtons)
    {
        button->isActive = false;
        button->isSelected = false;
        button->repaint();
    }

    // Enable buttons for active connections
    for (const auto& conn : connections)
    {
        if (!conn.enabled)
            continue;

        for (auto* button : connectionButtons)
        {
            if (button->source == conn.source && button->destination == conn.destination)
            {
                button->isActive = true;

                // Check if this is the selected connection
                if (selectedConnection.isValid &&
                    selectedConnection.source == conn.source &&
                    selectedConnection.destination == conn.destination)
                {
                    button->isSelected = true;
                }

                button->repaint();
                break;
            }
        }
    }

    // Refresh connection list
    refreshConnectionList();
}

void ModMatrixPanel::setupConnectionGrid()
{
    const int numSources = static_cast<int>(dsp::ModulationMatrix::SourceType::Count);
    const int numDestinations = static_cast<int>(dsp::ModulationMatrix::DestinationType::Count);

    // Create button for each source/destination combination
    for (int s = 0; s < numSources; ++s)
    {
        auto source = static_cast<dsp::ModulationMatrix::SourceType>(s);

        for (int d = 0; d < numDestinations; ++d)
        {
            auto dest = static_cast<dsp::ModulationMatrix::DestinationType>(d);

            auto* button = new ConnectionButton(source, dest);
            button->onClick = [this, button]() { onConnectionButtonClicked(button); };

            connectionButtons.add(button);
            addAndMakeVisible(button);
        }
    }
}

void ModMatrixPanel::setupLabels()
{
    const int numSources = static_cast<int>(dsp::ModulationMatrix::SourceType::Count);
    const int numDestinations = static_cast<int>(dsp::ModulationMatrix::DestinationType::Count);

    // Source labels (rows)
    for (int s = 0; s < numSources; ++s)
    {
        auto source = static_cast<dsp::ModulationMatrix::SourceType>(s);
        auto* label = new juce::Label();
        label->setText(getSourceName(source), juce::dontSendNotification);
        label->setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
        label->setColour(juce::Label::textColourId, getSourceColour(source));
        label->setJustificationType(juce::Justification::centredRight);
        sourceLabels.add(label);
        addAndMakeVisible(label);
    }

    // Destination labels (columns) - abbreviated for space
    const std::vector<juce::String> abbreviations = {
        "Tim", "Mas", "Den", "Blm", "Air", "Wid", "Mix",
        "Wrp", "Drf", "Grv", "Pil", "Tub", "Met", "Els", "Imp"
    };

    for (int d = 0; d < numDestinations; ++d)
    {
        auto* label = new juce::Label();
        juce::String name = (d < abbreviations.size()) ? abbreviations[d] : "?";

        label->setText(name, juce::dontSendNotification);
        label->setFont(juce::Font(juce::FontOptions(9.0f)));
        label->setColour(juce::Label::textColourId, juce::Colour(0xffa8a49c));
        label->setJustificationType(juce::Justification::centred);
        destinationLabels.add(label);
        addAndMakeVisible(label);
    }
}

void ModMatrixPanel::setupControlSliders()
{
    // Depth slider
    depthLabel.setText("Depth: (select connection)", juce::dontSendNotification);
    depthLabel.setFont(juce::Font(juce::FontOptions(12.0f)));
    depthLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa8a49c));
    addAndMakeVisible(depthLabel);

    depthSlider.setRange(-1.0, 1.0, 0.01);
    depthSlider.setValue(0.5);
    depthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    depthSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    depthSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff14171b));
    depthSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xff6b9bd1));
    depthSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xffe6e1d6));
    depthSlider.setEnabled(false);
    depthSlider.onValueChange = [this]() { onDepthChanged(); };
    addAndMakeVisible(depthSlider);

    // Smoothing slider
    smoothingLabel.setText("Smoothing (ms): (select connection)", juce::dontSendNotification);
    smoothingLabel.setFont(juce::Font(juce::FontOptions(12.0f)));
    smoothingLabel.setColour(juce::Label::textColourId, juce::Colour(0xffa8a49c));
    addAndMakeVisible(smoothingLabel);

    smoothingSlider.setRange(20.0, 1000.0, 1.0);
    smoothingSlider.setValue(200.0);
    smoothingSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    smoothingSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    smoothingSlider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff14171b));
    smoothingSlider.setColour(juce::Slider::trackColourId, juce::Colour(0xff6b9bd1));
    smoothingSlider.setColour(juce::Slider::thumbColourId, juce::Colour(0xffe6e1d6));
    smoothingSlider.setEnabled(false);
    smoothingSlider.onValueChange = [this]() { onSmoothingChanged(); };
    addAndMakeVisible(smoothingSlider);
}

void ModMatrixPanel::onConnectionButtonClicked(ConnectionButton* button)
{
    if (!button->isActive)
    {
        // Create new connection with default values
        modulationMatrix.setConnection(
            button->source,
            button->destination,
            0, // sourceAxis (0 for single-axis sources)
            0.5f, // default depth
            200.0f // default smoothing
        );

        // Select this connection for editing
        selectedConnection.source = button->source;
        selectedConnection.destination = button->destination;
        selectedConnection.isValid = true;

        // Update sliders
        depthSlider.setValue(0.5, juce::dontSendNotification);
        smoothingSlider.setValue(200.0, juce::dontSendNotification);
        depthSlider.setEnabled(true);
        smoothingSlider.setEnabled(true);

        depthLabel.setText("Depth: " + getSourceName(button->source) + " → " +
                          getDestinationName(button->destination), juce::dontSendNotification);
        smoothingLabel.setText("Smoothing (ms): " + getSourceName(button->source) + " → " +
                               getDestinationName(button->destination), juce::dontSendNotification);
    }
    else
    {
        // If already active and selected, remove connection
        if (button->isSelected)
        {
            modulationMatrix.removeConnection(button->source, button->destination, 0);

            selectedConnection.isValid = false;
            depthSlider.setEnabled(false);
            smoothingSlider.setEnabled(false);
            depthLabel.setText("Depth: (select connection)", juce::dontSendNotification);
            smoothingLabel.setText("Smoothing (ms): (select connection)", juce::dontSendNotification);
        }
        else
        {
            // Select this connection for editing
            selectedConnection.source = button->source;
            selectedConnection.destination = button->destination;
            selectedConnection.isValid = true;

            // Load current values from connection
            const auto& connections = modulationMatrix.getConnections();
            for (const auto& conn : connections)
            {
                if (conn.source == button->source && conn.destination == button->destination)
                {
                    depthSlider.setValue(conn.depth, juce::dontSendNotification);
                    smoothingSlider.setValue(conn.smoothingMs, juce::dontSendNotification);
                    break;
                }
            }

            depthSlider.setEnabled(true);
            smoothingSlider.setEnabled(true);

            depthLabel.setText("Depth: " + getSourceName(button->source) + " → " +
                              getDestinationName(button->destination), juce::dontSendNotification);
            smoothingLabel.setText("Smoothing (ms): " + getSourceName(button->source) + " → " +
                                   getDestinationName(button->destination), juce::dontSendNotification);
        }
    }

    updateFromMatrix();
}

void ModMatrixPanel::onDepthChanged()
{
    if (!selectedConnection.isValid)
        return;

    const float newDepth = static_cast<float>(depthSlider.getValue());

    // Update the connection (preserves existing smoothing)
    const auto& connections = modulationMatrix.getConnections();
    for (const auto& conn : connections)
    {
        if (conn.source == selectedConnection.source &&
            conn.destination == selectedConnection.destination)
        {
            modulationMatrix.setConnection(
                selectedConnection.source,
                selectedConnection.destination,
                0,
                newDepth,
                conn.smoothingMs
            );
            break;
        }
    }

    refreshConnectionList();
}

void ModMatrixPanel::onSmoothingChanged()
{
    if (!selectedConnection.isValid)
        return;

    const float newSmoothing = static_cast<float>(smoothingSlider.getValue());

    // Update the connection (preserves existing depth)
    const auto& connections = modulationMatrix.getConnections();
    for (const auto& conn : connections)
    {
        if (conn.source == selectedConnection.source &&
            conn.destination == selectedConnection.destination)
        {
            modulationMatrix.setConnection(
                selectedConnection.source,
                selectedConnection.destination,
                0,
                conn.depth,
                newSmoothing
            );
            break;
        }
    }

    refreshConnectionList();
}

void ModMatrixPanel::refreshConnectionList()
{
    const auto& connections = modulationMatrix.getConnections();

    juce::String listText;
    int activeCount = 0;

    for (const auto& conn : connections)
    {
        if (!conn.enabled)
            continue;

        listText += "• " + getSourceName(conn.source) + " → " +
                    getDestinationName(conn.destination) +
                    " (depth: " + juce::String(conn.depth, 2) +
                    ", smooth: " + juce::String(conn.smoothingMs, 0) + "ms)\n";
        activeCount++;
    }

    if (activeCount == 0)
    {
        listText = "No active connections.\nClick grid buttons above to create connections.";
    }

    connectionListDisplay.setText(listText, false);
}

juce::String ModMatrixPanel::getSourceName(dsp::ModulationMatrix::SourceType source) const
{
    switch (source)
    {
        case dsp::ModulationMatrix::SourceType::ChaosAttractor: return "Chaos";
        case dsp::ModulationMatrix::SourceType::AudioFollower: return "Audio";
        case dsp::ModulationMatrix::SourceType::BrownianMotion: return "Brownian";
        case dsp::ModulationMatrix::SourceType::EnvelopeTracker: return "Envelope";
        default: return "Unknown";
    }
}

juce::String ModMatrixPanel::getDestinationName(dsp::ModulationMatrix::DestinationType dest) const
{
    switch (dest)
    {
        case dsp::ModulationMatrix::DestinationType::Time: return "Time";
        case dsp::ModulationMatrix::DestinationType::Mass: return "Mass";
        case dsp::ModulationMatrix::DestinationType::Density: return "Density";
        case dsp::ModulationMatrix::DestinationType::Bloom: return "Bloom";
        case dsp::ModulationMatrix::DestinationType::Air: return "Air";
        case dsp::ModulationMatrix::DestinationType::Width: return "Width";
        case dsp::ModulationMatrix::DestinationType::Mix: return "Mix";
        case dsp::ModulationMatrix::DestinationType::Warp: return "Warp";
        case dsp::ModulationMatrix::DestinationType::Drift: return "Drift";
        case dsp::ModulationMatrix::DestinationType::Gravity: return "Gravity";
        case dsp::ModulationMatrix::DestinationType::PillarShape: return "Pillar";
        case dsp::ModulationMatrix::DestinationType::TubeCount: return "Tubes";
        case dsp::ModulationMatrix::DestinationType::MetallicResonance: return "Metallic";
        case dsp::ModulationMatrix::DestinationType::Elasticity: return "Elasticity";
        case dsp::ModulationMatrix::DestinationType::ImpossibilityDegree: return "Impossible";
        default: return "Unknown";
    }
}

juce::Colour ModMatrixPanel::getSourceColour(dsp::ModulationMatrix::SourceType source) const
{
    switch (source)
    {
        case dsp::ModulationMatrix::SourceType::ChaosAttractor:
            return juce::Colour(0xffe89547); // Orange
        case dsp::ModulationMatrix::SourceType::AudioFollower:
            return juce::Colour(0xff6bc47d); // Green
        case dsp::ModulationMatrix::SourceType::BrownianMotion:
            return juce::Colour(0xffa47bd1); // Purple
        case dsp::ModulationMatrix::SourceType::EnvelopeTracker:
            return juce::Colour(0xff6b9bd1); // Blue
        default:
            return juce::Colour(0xff6b9bd1);
    }
}

} // namespace ui
} // namespace monument
