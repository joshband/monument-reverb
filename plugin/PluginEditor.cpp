#include "PluginEditor.h"

MonumentAudioProcessorEditor::MonumentAudioProcessorEditor(MonumentAudioProcessor& p)
    : juce::AudioProcessorEditor(&p),
      processorRef(p),
      // Macro Controls - All PhotorealisticKnob with strategic stone variant distribution
      // Material/topology/viscosity → StoneType1 (irregular cosmic stone)
      materialKnob(processorRef.getAPVTS(), "material", "Material", PhotorealisticKnob::Style::StoneType2_Variant0),
      topologyKnob(processorRef.getAPVTS(), "topology", "Topology", PhotorealisticKnob::Style::StoneType1_Variant0),
      viscosityKnob(processorRef.getAPVTS(), "viscosity", "Viscosity", PhotorealisticKnob::Style::StoneType1_Variant1),
      // Evolution/chaos/elasticity → StoneType2 (polished LED stone)
      evolutionKnob(processorRef.getAPVTS(), "evolution", "Evolution", PhotorealisticKnob::Style::StoneType2_Variant1),
      chaosKnob(processorRef.getAPVTS(), "chaosIntensity", "Chaos", PhotorealisticKnob::Style::StoneType2_Variant2),
      elasticityKnob(processorRef.getAPVTS(), "elasticityDecay", "Elasticity", PhotorealisticKnob::Style::StoneType2_Variant3),
      // Ancient params → StoneType3 (weathered stone)
      patinaKnob(processorRef.getAPVTS(), "patina", "Patina", PhotorealisticKnob::Style::StoneType3_Variant0),
      abyssKnob(processorRef.getAPVTS(), "abyss", "Abyss", PhotorealisticKnob::Style::StoneType3_Variant1),
      coronaKnob(processorRef.getAPVTS(), "corona", "Corona", PhotorealisticKnob::Style::StoneType3_Variant2),
      breathKnob(processorRef.getAPVTS(), "breath", "Breath", PhotorealisticKnob::Style::StoneType3_Variant3),
      // Base Parameters - PhotorealisticKnob with distributed stone variants
      mixKnob(processorRef.getAPVTS(), "mix", "Mix", PhotorealisticKnob::Style::StoneType1_Variant2),
      timeKnob(processorRef.getAPVTS(), "time", "Time", PhotorealisticKnob::Style::StoneType1_Variant3),
      sizeHeroKnob(processorRef.getAPVTS(), "size", "Size", PhotorealisticKnob::Style::StoneType2_Variant0),
      massKnob(processorRef.getAPVTS(), "mass", "Mass", PhotorealisticKnob::Style::StoneType2_Variant1),
      densityKnob(processorRef.getAPVTS(), "density", "Density", PhotorealisticKnob::Style::StoneType2_Variant2),
      bloomKnob(processorRef.getAPVTS(), "bloom", "Bloom", PhotorealisticKnob::Style::StoneType2_Variant3),
      airKnob(processorRef.getAPVTS(), "air", "Air", PhotorealisticKnob::Style::StoneType3_Variant0),
      widthKnob(processorRef.getAPVTS(), "width", "Width", PhotorealisticKnob::Style::StoneType3_Variant1),
      warpKnob(processorRef.getAPVTS(), "warp", "Warp", PhotorealisticKnob::Style::StoneType3_Variant2),
      driftKnob(processorRef.getAPVTS(), "drift", "Drift", PhotorealisticKnob::Style::StoneType3_Variant3),
      gravityKnob(processorRef.getAPVTS(), "gravity", "Gravity", PhotorealisticKnob::Style::StoneType1_Variant0),
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

    // Create modulation matrix panel (kept separate for now)
    modMatrixPanel = std::make_unique<monument::ui::ModMatrixPanel>(processorRef.getModulationMatrix());
    modMatrixPanel->setVisible(false);
    addAndMakeVisible(modMatrixPanel.get());

    // Populate preset list (factory + user)
    scanUserPresets();
    refreshPresetList();

    // Create HeaderBar (will be added to top of layout)
    headerBar = std::make_unique<HeaderBar>(processorRef.getAPVTS());
    addAndMakeVisible(headerBar.get());

    // Create CollapsiblePanels for the three main sections
    // Panel 1: THE MACRO CONTROL (10 knobs in 2 rows of 5)
    macroControlPanel = std::make_unique<CollapsiblePanel>("THE MACRO CONTROL");
    macroControlPanel->setExpandedHeight(280); // Two rows of knobs + padding
    macroControlPanel->setExpanded(true, false); // Start expanded, no animation
    macroControlPanel->onExpandedChanged = [this]() { resized(); };
    addAndMakeVisible(macroControlPanel.get());

    // Panel 2: THE FOUNDATION (11 base params + freeze toggle)
    foundationPanel = std::make_unique<CollapsiblePanel>("THE FOUNDATION");
    foundationPanel->setExpandedHeight(300); // Grid of base parameters
    foundationPanel->setExpanded(false, false); // Start collapsed
    foundationPanel->onExpandedChanged = [this]() { resized(); };
    addAndMakeVisible(foundationPanel.get());

    // Panel 3: THE MODULATION NEXUS (timeline component)
    modulationNexusPanel = std::make_unique<CollapsiblePanel>("THE MODULATION NEXUS");
    modulationNexusPanel->setExpandedHeight(560); // Timeline height
    modulationNexusPanel->setExpanded(false, false); // Start collapsed
    modulationNexusPanel->onExpandedChanged = [this]() { resized(); };

    // Create and set timeline as the panel content
    auto* timeline = new monument::ui::TimelineComponent(processorRef.getSequenceScheduler());
    modulationNexusPanel->setContentComponent(timeline);

    addAndMakeVisible(modulationNexusPanel.get());

    // Add enhanced dark stone background with animated wisps (bottom-most component)
    addAndMakeVisible(enhancedBackground);
    enhancedBackground.toBack();

    setSize(900, 600);
}

MonumentAudioProcessorEditor::~MonumentAudioProcessorEditor() = default;

void MonumentAudioProcessorEditor::paint(juce::Graphics&)
{
    // Background is handled by EnhancedBackgroundComponent
    // All other UI elements are handled by child components
}

void MonumentAudioProcessorEditor::resized()
{
    // Fill background to entire component
    enhancedBackground.setBounds(getLocalBounds());

    const int margin = 10;
    const int headerHeight = 70;
    auto area = getLocalBounds();

    // Header Bar at top
    if (headerBar)
    {
        headerBar->setBounds(area.removeFromTop(headerHeight));
    }

    // Preset controls area (below header)
    area.removeFromTop(margin);
    auto presetArea = area.removeFromTop(40);
    const int buttonWidth = 150;
    const int buttonSpacing = 10;
    auto presetRow = presetArea.withSizeKeepingCentre((buttonWidth * 3) + (buttonSpacing * 2), 35);
    presetBox.setBounds(presetRow.removeFromLeft(buttonWidth));
    presetRow.removeFromLeft(buttonSpacing);
    savePresetButton.setBounds(presetRow.removeFromLeft(buttonWidth));
    presetRow.removeFromLeft(buttonSpacing);

    // Modulation Matrix toggle button
    auto modButtonArea = presetRow.removeFromLeft(buttonWidth);
    modMatrixToggleButton.setBounds(modButtonArea);

    area.removeFromTop(margin);

    // Panel 1: THE MACRO CONTROL (10 knobs, 2 rows of 5)
    if (macroControlPanel)
    {
        const int panelHeight = macroControlPanel->isExpanded() ?
            macroControlPanel->getExpandedHeight() : macroControlPanel->getCollapsedHeight();
        auto panelBounds = area.removeFromTop(panelHeight);
        macroControlPanel->setBounds(panelBounds);

        // Position macro knobs inside the panel if expanded
        if (macroControlPanel->isExpanded())
        {
            auto macroArea = panelBounds.withTrimmedTop(45).reduced(margin); // Skip header
            const int knobSize = 100;
            const int knobsPerRow = 5;
            const int spacing = (macroArea.getWidth() - (knobsPerRow * knobSize)) / (knobsPerRow + 1);

            // Row 1: Material, Topology, Viscosity, Evolution, Chaos
            int x = macroArea.getX() + spacing;
            int y = macroArea.getY() + 10;
            materialKnob.setBounds(x, y, knobSize, knobSize); x += knobSize + spacing;
            topologyKnob.setBounds(x, y, knobSize, knobSize); x += knobSize + spacing;
            viscosityKnob.setBounds(x, y, knobSize, knobSize); x += knobSize + spacing;
            evolutionKnob.setBounds(x, y, knobSize, knobSize); x += knobSize + spacing;
            chaosKnob.setBounds(x, y, knobSize, knobSize);

            // Row 2: Elasticity, Patina, Abyss, Corona, Breath
            x = macroArea.getX() + spacing;
            y += knobSize + 20;
            elasticityKnob.setBounds(x, y, knobSize, knobSize); x += knobSize + spacing;
            patinaKnob.setBounds(x, y, knobSize, knobSize); x += knobSize + spacing;
            abyssKnob.setBounds(x, y, knobSize, knobSize); x += knobSize + spacing;
            coronaKnob.setBounds(x, y, knobSize, knobSize); x += knobSize + spacing;
            breathKnob.setBounds(x, y, knobSize, knobSize);

            // Show macro knobs
            materialKnob.setVisible(true);
            topologyKnob.setVisible(true);
            viscosityKnob.setVisible(true);
            evolutionKnob.setVisible(true);
            chaosKnob.setVisible(true);
            elasticityKnob.setVisible(true);
            patinaKnob.setVisible(true);
            abyssKnob.setVisible(true);
            coronaKnob.setVisible(true);
            breathKnob.setVisible(true);
        }
        else
        {
            // Hide macro knobs when panel collapsed
            materialKnob.setVisible(false);
            topologyKnob.setVisible(false);
            viscosityKnob.setVisible(false);
            evolutionKnob.setVisible(false);
            chaosKnob.setVisible(false);
            elasticityKnob.setVisible(false);
            patinaKnob.setVisible(false);
            abyssKnob.setVisible(false);
            coronaKnob.setVisible(false);
            breathKnob.setVisible(false);
        }

        area.removeFromTop(margin);
    }

    // Panel 2: THE FOUNDATION (11 base params + freeze)
    if (foundationPanel)
    {
        const int panelHeight = foundationPanel->isExpanded() ?
            foundationPanel->getExpandedHeight() : foundationPanel->getCollapsedHeight();
        auto panelBounds = area.removeFromTop(panelHeight);
        foundationPanel->setBounds(panelBounds);

        // Position base param knobs inside the panel if expanded
        if (foundationPanel->isExpanded())
        {
            auto gridArea = panelBounds.withTrimmedTop(45).reduced(margin * 2);
            const int columns = 4;
            const int rows = 3;
            const int cellWidth = gridArea.getWidth() / columns;
            const int cellHeight = gridArea.getHeight() / rows;

            auto cell = [&](int row, int col)
            {
                return juce::Rectangle<int>(
                    gridArea.getX() + col * cellWidth,
                    gridArea.getY() + row * cellHeight,
                    cellWidth,
                    cellHeight
                ).reduced(6);
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

            // Show base param knobs
            mixKnob.setVisible(true);
            timeKnob.setVisible(true);
            sizeHeroKnob.setVisible(true);
            massKnob.setVisible(true);
            densityKnob.setVisible(true);
            bloomKnob.setVisible(true);
            airKnob.setVisible(true);
            widthKnob.setVisible(true);
            warpKnob.setVisible(true);
            driftKnob.setVisible(true);
            gravityKnob.setVisible(true);
            freezeToggle.setVisible(true);
        }
        else
        {
            // Hide base param knobs when panel collapsed
            mixKnob.setVisible(false);
            timeKnob.setVisible(false);
            sizeHeroKnob.setVisible(false);
            massKnob.setVisible(false);
            densityKnob.setVisible(false);
            bloomKnob.setVisible(false);
            airKnob.setVisible(false);
            widthKnob.setVisible(false);
            warpKnob.setVisible(false);
            driftKnob.setVisible(false);
            gravityKnob.setVisible(false);
            freezeToggle.setVisible(false);
        }

        area.removeFromTop(margin);
    }

    // Panel 3: THE MODULATION NEXUS (timeline)
    if (modulationNexusPanel)
    {
        const int panelHeight = modulationNexusPanel->isExpanded() ?
            modulationNexusPanel->getExpandedHeight() : modulationNexusPanel->getCollapsedHeight();
        auto panelBounds = area.removeFromTop(panelHeight);
        modulationNexusPanel->setBounds(panelBounds);

        area.removeFromTop(margin);
    }

    // Modulation Matrix Panel (separate, toggle-able)
    if (modMatrixVisible && modMatrixPanel)
    {
        const int modMatrixHeight = 500;
        modMatrixPanel->setBounds(area.removeFromTop(modMatrixHeight));
    }

    // Calculate total required height and resize window if needed
    int totalHeight = headerHeight + margin + 40 + margin; // Header + preset area

    totalHeight += macroControlPanel ?
        (macroControlPanel->isExpanded() ? macroControlPanel->getExpandedHeight() : macroControlPanel->getCollapsedHeight()) + margin : 0;

    totalHeight += foundationPanel ?
        (foundationPanel->isExpanded() ? foundationPanel->getExpandedHeight() : foundationPanel->getCollapsedHeight()) + margin : 0;

    totalHeight += modulationNexusPanel ?
        (modulationNexusPanel->isExpanded() ? modulationNexusPanel->getExpandedHeight() : modulationNexusPanel->getCollapsedHeight()) + margin : 0;

    if (modMatrixVisible)
        totalHeight += 500;

    totalHeight += margin; // Bottom margin

    // Update window size if needed
    if (getHeight() != totalHeight)
        setSize(900, totalHeight);

    // Update background panel dividers
    std::vector<float> dividers;
    dividers.push_back(static_cast<float>(headerHeight));
    dividers.push_back(static_cast<float>(headerHeight + margin + 40 + margin));
    enhancedBackground.setPanelDividers(dividers);
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
