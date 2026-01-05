#include "PluginEditor.h"

MonumentAudioProcessorEditor::MonumentAudioProcessorEditor(MonumentAudioProcessor& p)
    : juce::AudioProcessorEditor(&p),
      processorRef(p),
      // Macro Controls - All using HeroKnob (codex brushed aluminum)
      materialKnob(processorRef.getAPVTS(), "material", "Material"),
      topologyKnob(processorRef.getAPVTS(), "topology", "Topology"),
      viscosityKnob(processorRef.getAPVTS(), "viscosity", "Viscosity"),
      evolutionKnob(processorRef.getAPVTS(), "evolution", "Evolution"),
      chaosKnob(processorRef.getAPVTS(), "chaosIntensity", "Chaos"),
      elasticityKnob(processorRef.getAPVTS(), "elasticityDecay", "Elasticity"),
      patinaKnob(processorRef.getAPVTS(), "patina", "Patina"),
      abyssKnob(processorRef.getAPVTS(), "abyss", "Abyss"),
      coronaKnob(processorRef.getAPVTS(), "corona", "Corona"),
      breathKnob(processorRef.getAPVTS(), "breath", "Breath"),
      // Base Parameters - All using HeroKnob (codex brushed aluminum)
      mixKnob(processorRef.getAPVTS(), "mix", "Mix"),
      timeKnob(processorRef.getAPVTS(), "time", "Time"),
      sizeHeroKnob(processorRef.getAPVTS(), "size", "Size"),
      massKnob(processorRef.getAPVTS(), "mass", "Mass"),
      densityKnob(processorRef.getAPVTS(), "density", "Density"),
      bloomKnob(processorRef.getAPVTS(), "bloom", "Bloom"),
      airKnob(processorRef.getAPVTS(), "air", "Air"),
      widthKnob(processorRef.getAPVTS(), "width", "Width"),
      warpKnob(processorRef.getAPVTS(), "warp", "Warp"),
      driftKnob(processorRef.getAPVTS(), "drift", "Drift"),
      gravityKnob(processorRef.getAPVTS(), "gravity", "Gravity"),
      freezeToggle(processorRef.getAPVTS(), "freeze", "Freeze")
{
    // Add macro controls (primary interface)
    addAndMakeVisible(materialKnob);
    addAndMakeVisible(topologyKnob);
    addAndMakeVisible(viscosityKnob);
    addAndMakeVisible(evolutionKnob);
    addAndMakeVisible(chaosKnob);
    addAndMakeVisible(elasticityKnob);
    addAndMakeVisible(patinaKnob);
    addAndMakeVisible(abyssKnob);
    addAndMakeVisible(coronaKnob);
    addAndMakeVisible(breathKnob);

    // Add base parameters (hidden by default - macros are primary interface)
    addChildComponent(mixKnob);
    addChildComponent(timeKnob);
    addChildComponent(sizeHeroKnob);
    addChildComponent(massKnob);
    addChildComponent(densityKnob);
    addChildComponent(bloomKnob);
    addChildComponent(airKnob);
    addChildComponent(widthKnob);
    addChildComponent(warpKnob);
    addChildComponent(driftKnob);
    addChildComponent(gravityKnob);
    addChildComponent(freezeToggle);

    // Preset controls always visible
    addAndMakeVisible(presetBox);
    addAndMakeVisible(savePresetButton);

    // Routing Architecture Selector (Phase 1.5)
    routingPresetLabel.setText("Architecture", juce::dontSendNotification);
    routingPresetLabel.setJustificationType(juce::Justification::centred);
    routingPresetLabel.setColour(juce::Label::textColourId, juce::Colour(0xff666666));
    addAndMakeVisible(routingPresetLabel);

    routingPresetBox.setTextWhenNothingSelected("Traditional Cathedral");
    routingPresetBox.setJustificationType(juce::Justification::centred);
    routingPresetBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff14171b));
    routingPresetBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xffe6e1d6));
    routingPresetBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff3a3f46));
    routingPresetBox.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xffe6e1d6));
    routingPresetBox.setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff14171b));
    routingPresetBox.setColour(juce::PopupMenu::textColourId, juce::Colour(0xffe6e1d6));
    routingPresetBox.setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff242833));
    routingPresetBox.setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(0xffe6e1d6));
    addAndMakeVisible(routingPresetBox);

    // Create APVTS attachment for routing preset
    routingPresetAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        processorRef.getAPVTS(), "routingPreset", routingPresetBox);

    // Processing Mode Selector (Ancient Monuments Routing)
    processingModeLabel.setText("Mode:", juce::dontSendNotification);
    processingModeLabel.setJustificationType(juce::Justification::centredRight);
    processingModeLabel.setColour(juce::Label::textColourId, juce::Colour(0xff666666));
    addAndMakeVisible(processingModeLabel);

    processingModeBox.addItem("Ancient Way", 1);
    processingModeBox.addItem("Resonant Halls", 2);
    processingModeBox.addItem("Breathing Stone", 3);
    processingModeBox.setSelectedId(1, juce::dontSendNotification);
    processingModeBox.setTextWhenNothingSelected("Ancient Way");
    processingModeBox.setJustificationType(juce::Justification::centred);
    processingModeBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff14171b));
    processingModeBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xffe6e1d6));
    processingModeBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff3a3f46));
    processingModeBox.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xffe6e1d6));
    processingModeBox.setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff14171b));
    processingModeBox.setColour(juce::PopupMenu::textColourId, juce::Colour(0xffe6e1d6));
    processingModeBox.setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff242833));
    processingModeBox.setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(0xffe6e1d6));
    processingModeBox.onChange = [this]()
    {
        const int selectedId = processingModeBox.getSelectedId();
        ProcessingMode mode = ProcessingMode::AncientWay;

        switch (selectedId)
        {
            case 1: mode = ProcessingMode::AncientWay; break;
            case 2: mode = ProcessingMode::ResonantHalls; break;
            case 3: mode = ProcessingMode::BreathingStone; break;
        }

        processorRef.setProcessingMode(mode);
    };
    addAndMakeVisible(processingModeBox);

    // Preset browser styling
    presetBox.setTextWhenNothingSelected("Presets");
    presetBox.setJustificationType(juce::Justification::centred);
    presetBox.setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff14171b));
    presetBox.setColour(juce::ComboBox::textColourId, juce::Colour(0xffe6e1d6));
    presetBox.setColour(juce::ComboBox::outlineColourId, juce::Colour(0xff3a3f46));
    presetBox.setColour(juce::ComboBox::arrowColourId, juce::Colour(0xffe6e1d6));
    presetBox.setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff14171b));
    presetBox.setColour(juce::PopupMenu::textColourId, juce::Colour(0xffe6e1d6));
    presetBox.setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff242833));
    presetBox.setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(0xffe6e1d6));

    // Save button styling
    savePresetButton.setButtonText("Save");
    savePresetButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff242833));
    savePresetButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe6e1d6));
    savePresetButton.setColour(juce::TextButton::textColourOnId, juce::Colour(0xffe6e1d6));
    savePresetButton.onClick = [this]() { showSavePresetDialog(); };

    // Base Parameters toggle button
    baseParamsToggleButton.setButtonText("BASE PARAMS");
    baseParamsToggleButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff242833));
    baseParamsToggleButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff8b7355));
    baseParamsToggleButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe6e1d6));
    baseParamsToggleButton.setColour(juce::TextButton::textColourOnId, juce::Colour(0xff0d0f12));
    baseParamsToggleButton.setClickingTogglesState(true);
    baseParamsToggleButton.onClick = [this]()
    {
        baseParamsVisible = baseParamsToggleButton.getToggleState();

        // Toggle visibility of all base parameter controls
        mixKnob.setVisible(baseParamsVisible);
        timeKnob.setVisible(baseParamsVisible);
        sizeHeroKnob.setVisible(baseParamsVisible);
        massKnob.setVisible(baseParamsVisible);
        densityKnob.setVisible(baseParamsVisible);
        bloomKnob.setVisible(baseParamsVisible);
        airKnob.setVisible(baseParamsVisible);
        widthKnob.setVisible(baseParamsVisible);
        warpKnob.setVisible(baseParamsVisible);
        driftKnob.setVisible(baseParamsVisible);
        gravityKnob.setVisible(baseParamsVisible);
        freezeToggle.setVisible(baseParamsVisible);

        resized();
    };
    addAndMakeVisible(baseParamsToggleButton);

    // Modulation Matrix toggle button
    modMatrixToggleButton.setButtonText("MODULATION");
    modMatrixToggleButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff242833));
    modMatrixToggleButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xff6b9bd1));
    modMatrixToggleButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe6e1d6));
    modMatrixToggleButton.setColour(juce::TextButton::textColourOnId, juce::Colour(0xff0d0f12));
    modMatrixToggleButton.setClickingTogglesState(true);
    modMatrixToggleButton.onClick = [this]()
    {
        modMatrixVisible = modMatrixToggleButton.getToggleState();
        if (modMatrixPanel)
            modMatrixPanel->setVisible(modMatrixVisible);
        resized();
    };
    addAndMakeVisible(modMatrixToggleButton);

    // Create modulation matrix panel
    modMatrixPanel = std::make_unique<monument::ui::ModMatrixPanel>(processorRef.getModulationMatrix());
    modMatrixPanel->setVisible(false);
    addAndMakeVisible(modMatrixPanel.get());

    // Timeline Editor toggle button (Phase 5)
    timelineToggleButton.setButtonText("TIMELINE");
    timelineToggleButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff242833));
    timelineToggleButton.setColour(juce::TextButton::buttonOnColourId, juce::Colour(0xffff9e4a));
    timelineToggleButton.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe6e1d6));
    timelineToggleButton.setColour(juce::TextButton::textColourOnId, juce::Colour(0xff0d0f12));
    timelineToggleButton.setClickingTogglesState(true);
    timelineToggleButton.onClick = [this]()
    {
        timelineVisible = timelineToggleButton.getToggleState();
        if (timelinePanel)
            timelinePanel->setVisible(timelineVisible);
        resized();
    };
    addAndMakeVisible(timelineToggleButton);

    // Create timeline panel (Phase 5)
    timelinePanel = std::make_unique<monument::ui::TimelineComponent>(processorRef.getSequenceScheduler());
    timelinePanel->setVisible(false);
    addAndMakeVisible(timelinePanel.get());

    // Populate preset list (factory + user)
    scanUserPresets();
    refreshPresetList();

    setSize(900, 580);
}

MonumentAudioProcessorEditor::~MonumentAudioProcessorEditor() = default;

void MonumentAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::white);  // Changed from black to white
    g.setColour(juce::Colour(0xff333333));  // Darker text for white background

    // Title
    g.setFont(juce::Font(juce::FontOptions(20.0f, juce::Font::bold)));
    g.drawFittedText("Monument", getLocalBounds().removeFromTop(35), juce::Justification::centred, 1);

    // Macro section label
    g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
    g.setColour(juce::Colour(0xff666666));  // Medium gray for white background
    g.drawFittedText("MACRO CONTROLS", juce::Rectangle<int>(24, 45, getWidth() - 48, 20),
                     juce::Justification::centredLeft, 1);

    // Base parameters section (only show if visible)
    if (baseParamsVisible)
    {
        // Separator line after macros
        g.setColour(juce::Colour(0xffcccccc));  // Light gray separator for white background
        g.drawLine(24.0f, 185.0f, static_cast<float>(getWidth() - 24), 185.0f, 1.0f);

        // Base parameters label
        g.setFont(juce::Font(juce::FontOptions(14.0f, juce::Font::bold)));
        g.setColour(juce::Colour(0xff666666));  // Medium gray for white background
        g.drawFittedText("BASE PARAMETERS", juce::Rectangle<int>(24, 195, getWidth() - 48, 20),
                         juce::Justification::centredLeft, 1);
    }
}

void MonumentAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(24);

    // Top Bar: Title + Architecture Selector
    auto topBar = area.removeFromTop(35);
    // Architecture dropdown in top-right
    const int archLabelWidth = 90;
    const int archDropdownWidth = 200;
    topBar.removeFromRight(10);  // Right margin
    auto archDropdown = topBar.removeFromRight(archDropdownWidth);
    auto archLabel = topBar.removeFromRight(archLabelWidth);
    routingPresetLabel.setBounds(archLabel);
    routingPresetBox.setBounds(archDropdown);

    // Processing Mode Selector (next to routing architecture)
    topBar.removeFromRight(10);  // Small gap
    const auto modeLabelWidth = 50;
    const auto modeDropdownWidth = 150;
    auto modeDropdown = topBar.removeFromRight(modeDropdownWidth);
    auto modeLabel = topBar.removeFromRight(modeLabelWidth);
    processingModeLabel.setBounds(modeLabel);
    processingModeBox.setBounds(modeDropdown);

    // Macro Controls Section
    area.removeFromTop(25);  // Label space
    // Ancient Monuments Phase 5 - 10 macro controls
    auto macroArea = area.removeFromTop(115);
    const auto macroWidth = macroArea.getWidth() / 10;  // Was /6, now /10

    materialKnob.setBounds(macroArea.removeFromLeft(macroWidth).reduced(6));
    topologyKnob.setBounds(macroArea.removeFromLeft(macroWidth).reduced(6));
    viscosityKnob.setBounds(macroArea.removeFromLeft(macroWidth).reduced(6));
    evolutionKnob.setBounds(macroArea.removeFromLeft(macroWidth).reduced(6));
    chaosKnob.setBounds(macroArea.removeFromLeft(macroWidth).reduced(6));
    elasticityKnob.setBounds(macroArea.removeFromLeft(macroWidth).reduced(6));
    patinaKnob.setBounds(macroArea.removeFromLeft(macroWidth).reduced(6));
    abyssKnob.setBounds(macroArea.removeFromLeft(macroWidth).reduced(6));
    coronaKnob.setBounds(macroArea.removeFromLeft(macroWidth).reduced(6));
    breathKnob.setBounds(macroArea.removeFromLeft(macroWidth).reduced(6));

    // Control buttons area (below macros)
    area.removeFromTop(10);  // Spacing
    auto controlsArea = area.removeFromTop(100);
    const int buttonWidth = 150;
    const int buttonHeight = 35;
    const int buttonSpacing = 10;

    // Center the control buttons horizontally
    auto buttonRow = controlsArea.withSizeKeepingCentre(
        (buttonWidth * 3) + (buttonSpacing * 2), buttonHeight);

    presetBox.setBounds(buttonRow.removeFromLeft(buttonWidth));
    buttonRow.removeFromLeft(buttonSpacing);
    savePresetButton.setBounds(buttonRow.removeFromLeft(buttonWidth));
    buttonRow.removeFromLeft(buttonSpacing);
    baseParamsToggleButton.setBounds(buttonRow.removeFromLeft(buttonWidth));

    // Modulation and Timeline toggle buttons below
    auto modTimelineButtonsArea = controlsArea.removeFromTop(50);
    auto modTimelineRow = modTimelineButtonsArea.withSizeKeepingCentre(
        (buttonWidth * 2) + buttonSpacing, buttonHeight);
    modMatrixToggleButton.setBounds(modTimelineRow.removeFromLeft(buttonWidth));
    modTimelineRow.removeFromLeft(buttonSpacing);
    timelineToggleButton.setBounds(modTimelineRow.removeFromLeft(buttonWidth));

    // Base Parameters Section (only layout if visible)
    if (baseParamsVisible)
    {
        area.removeFromTop(10);  // Separator space
        area.removeFromTop(25);  // Base params label space

        // Base Parameters Grid (4x3)
        auto gridArea = area.reduced(10);
        const auto columnWidth = gridArea.getWidth() / 4;
        const auto rowHeight = gridArea.getHeight() / 3;

        auto cell = [&](int row, int column)
        {
            return juce::Rectangle<int>(gridArea.getX() + column * columnWidth,
                                        gridArea.getY() + row * rowHeight,
                                        columnWidth,
                                        rowHeight)
                .reduced(6);
        };

        mixKnob.setBounds(cell(0, 0));
        timeKnob.setBounds(cell(0, 1));
        sizeHeroKnob.setBounds(cell(0, 2));
        massKnob.setBounds(cell(0, 3));

        densityKnob.setBounds(cell(1, 0));
        bloomKnob.setBounds(cell(1, 1));
        airKnob.setBounds(cell(1, 2));
        widthKnob.setBounds(cell(1, 3));

        warpKnob.setBounds(cell(2, 0));
        driftKnob.setBounds(cell(2, 1));
        freezeToggle.setBounds(cell(2, 2));
        gravityKnob.setBounds(cell(2, 3));
    }

    // Dynamic window sizing based on visibility
    int targetHeight = 260;  // Compact size (macros + controls only)
    if (baseParamsVisible)
        targetHeight = 580;  // Full size with base parameters
    if (modMatrixVisible)
        targetHeight = 1080;  // Expanded with mod matrix
    if (timelineVisible)
        targetHeight = 800;   // Expanded with timeline editor

    if (getHeight() != targetHeight)
        setSize(900, targetHeight);

    // Modulation Matrix Panel (if visible)
    if (modMatrixVisible && modMatrixPanel)
    {
        auto panelBounds = getLocalBounds();
        panelBounds.removeFromTop(baseParamsVisible ? 580 : 260);
        modMatrixPanel->setBounds(panelBounds.reduced(10));
    }

    // Timeline Panel (if visible) (Phase 5)
    if (timelineVisible && timelinePanel)
    {
        auto panelBounds = getLocalBounds();
        panelBounds.removeFromTop(baseParamsVisible ? 580 : 260);
        timelinePanel->setBounds(panelBounds.reduced(10));
    }
}

void MonumentAudioProcessorEditor::scanUserPresets()
{
    userPresetFiles.clear();

    const auto presetDir = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("MonumentPresets");

    if (!presetDir.exists())
        return;

    juce::Array<juce::File> results;
    presetDir.findChildFiles(results, juce::File::findFiles, false, "*.json");

    for (const auto& file : results)
        userPresetFiles.push_back(file);

    // Sort alphabetically by filename
    std::sort(userPresetFiles.begin(), userPresetFiles.end(),
        [](const juce::File& a, const juce::File& b)
        {
            return a.getFileNameWithoutExtension().compareIgnoreCase(b.getFileNameWithoutExtension()) < 0;
        });
}

void MonumentAudioProcessorEditor::refreshPresetList()
{
    presetBox.clear();

    // Factory presets
    const int factoryCount = processorRef.getNumFactoryPresets();
    auto addSection = [&](const juce::String& title, int start, int end)
    {
        if (start >= factoryCount)
            return;
        const int clampedEnd = juce::jmin(end, factoryCount - 1);
        if (clampedEnd < start)
            return;

        presetBox.addSectionHeading(title);
        for (int index = start; index <= clampedEnd; ++index)
            presetBox.addItem(processorRef.getFactoryPresetName(index), index + 1);
    };

    addSection("Foundational Spaces", 0, 5);
    addSection("Living Spaces", 6, 11);
    addSection("Remembering Spaces", 12, 14);
    addSection("Time-Bent / Abstract", 15, 17);
    addSection("Evolving Spaces", 18, factoryCount - 1);

    // User presets section
    if (!userPresetFiles.empty())
    {
        presetBox.addSectionHeading("User Presets");
        const int userStartId = factoryCount + 100;  // Offset to avoid conflicts
        for (size_t i = 0; i < userPresetFiles.size(); ++i)
        {
            const auto name = userPresetFiles[i].getFileNameWithoutExtension();
            presetBox.addItem(name, userStartId + static_cast<int>(i));
        }
    }

    // Preset selection handler
    presetBox.onChange = [this]()
    {
        const int selectedId = presetBox.getSelectedId();
        if (selectedId <= 0)
            return;

        const int factoryCount = processorRef.getNumFactoryPresets();
        const int userStartId = factoryCount + 100;

        if (selectedId < userStartId)
        {
            // Factory preset
            processorRef.loadFactoryPreset(selectedId - 1);
        }
        else
        {
            // User preset
            const int userIndex = selectedId - userStartId;
            if (userIndex >= 0 && userIndex < static_cast<int>(userPresetFiles.size()))
            {
                processorRef.loadUserPreset(userPresetFiles[static_cast<size_t>(userIndex)]);
            }
        }
    };
}

void MonumentAudioProcessorEditor::showSavePresetDialog()
{
    // FIXED: Memory-safe AlertWindow pattern using fully async callbacks
    // This prevents memory leaks and follows JUCE best practices for modal dialogs
    auto alertWindow = std::make_unique<juce::AlertWindow>("Save Preset",
                                                             "Enter a name for this preset:",
                                                             juce::MessageBoxIconType::QuestionIcon,
                                                             this);
    alertWindow->addTextEditor("name", "", "Preset Name:");
    alertWindow->addTextEditor("description", "", "Description (optional):");
    alertWindow->addButton("Save", 1, juce::KeyPress(juce::KeyPress::returnKey));
    alertWindow->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

    // Use SafePointer to prevent dangling references if component is deleted
    alertWindow->enterModalState(true,
        juce::ModalCallbackFunction::create([this, safeThis = juce::Component::SafePointer<MonumentAudioProcessorEditor>(this)](int result) mutable
        {
            if (safeThis == nullptr)
                return;  // Editor was deleted

            if (result == 1)  // Save clicked
            {
                // Create a new AlertWindow for text input (must be on message thread)
                auto textWindow = std::make_unique<juce::AlertWindow>("Preset Details",
                                                                        "Enter preset information:",
                                                                        juce::MessageBoxIconType::NoIcon);
                textWindow->addTextEditor("name", "", "Preset Name:");
                textWindow->addTextEditor("description", "", "Description (optional):");
                textWindow->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
                textWindow->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

                auto* textPtr = textWindow.get();
                textWindow->enterModalState(true,
                    juce::ModalCallbackFunction::create([this, safeThis2 = juce::Component::SafePointer<MonumentAudioProcessorEditor>(this), textPtr](int innerResult)
                    {
                        if (safeThis2 == nullptr || textPtr == nullptr)
                            return;

                        if (innerResult == 1)  // OK clicked
                        {
                            const auto name = textPtr->getTextEditorContents("name");
                            const auto description = textPtr->getTextEditorContents("description");

                            if (name.isNotEmpty())
                            {
                                processorRef.saveUserPreset(name, description.isEmpty() ? "User preset" : description);

                                // Refresh the preset list to include the new preset
                                scanUserPresets();
                                refreshPresetList();
                            }
                        }
                    }),
                    true);

                textWindow.release();  // Transfer ownership to modal state
            }
        }),
        true);

    alertWindow.release();  // Transfer ownership to modal state
}
