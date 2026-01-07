#include "MainComponent.h"

MainComponent::MainComponent()
{
    // Test asset loading by printing available keys
    auto& am = Monument::AssetManager::instance();

    DBG("=== Monument UI Demo Started ===");
    DBG("Available assets:");
    for (auto& key : am.getAvailableKeys())
    {
        DBG("  - " << key);
    }

    // Add monument body
    addAndMakeVisible(body);

    // Add CPU-based PBR knob demo - using Geode variant
    cpuBlendKnob = std::make_unique<Monument::StoneKnobDemo>(
        "CPU Blend (Geode)",
        Monument::StoneKnobDemo::KnobType::Geode
    );
    addAndMakeVisible(cpuBlendKnob.get());

    // Add filmstrip knob demo - zero CPU cost, perfect blend modes
    filmstripKnob = std::make_unique<Monument::FilmstripKnobDemo>(
        "Filmstrip (Zero CPU)",
        Monument::FilmstripKnobDemo::KnobType::Geode
    );
    addAndMakeVisible(filmstripKnob.get());

    setSize(1400, 900);
}

MainComponent::~MainComponent()
{
}

void MainComponent::paint(juce::Graphics& g)
{
    auto& am = Monument::AssetManager::instance();

    // Dark background
    g.fillAll(juce::Colour(25, 27, 30));

    // Draw title
    g.setColour(juce::Colours::white);
    g.setFont(28.0f);
    g.drawText("Monument UI Demo - PBR Knob Asset Showcase",
               getLocalBounds().removeFromTop(70),
               juce::Justification::centred, true);

    // === PBR KNOB LAYER SHOWCASE ===
    int x = 50;
    int y = 100;
    int knobSize = 200;
    int spacing = 250;

    // Draw 4 knob variants with their layers
    juce::StringArray knobTypes = {"geode", "obsidian", "marble", "weathered"};
    juce::StringArray knobNames = {"Geode Crystal", "Black Obsidian", "Pale Marble", "Weathered Basalt"};

    for (int i = 0; i < knobTypes.size(); ++i)
    {
        juce::String type = knobTypes[i];
        juce::String prefix = "knob." + type + ".";

        // Draw knob label
        g.setColour(juce::Colours::lightgrey);
        g.setFont(18.0f);
        g.drawText(knobNames[i], x, y - 30, knobSize, 25, juce::Justification::centred);

        // Layer stack (bottom to top)
        auto albedo = am.getImage(prefix + "albedo");
        auto ao = am.getImage(prefix + "ao");
        auto glow_core = am.getImage(prefix + "glow_core");
        auto glow_crystal = am.getImage(prefix + "glow_crystal");
        auto indicator = am.getImage(prefix + "indicator");
        auto contact_shadow = am.getImage(prefix + "contact_shadow");

        if (albedo.isValid())
        {
            // 1. Contact shadow (beneath knob)
            if (contact_shadow.isValid())
            {
                g.setOpacity(0.5f);
                g.drawImage(contact_shadow, x, y + 5, knobSize, knobSize,
                           0, 0, contact_shadow.getWidth(), contact_shadow.getHeight());
            }

            // 2. Base albedo
            g.setOpacity(1.0f);
            g.drawImage(albedo, x, y, knobSize, knobSize,
                       0, 0, albedo.getWidth(), albedo.getHeight());

            // 3. AO for depth (multiply blend approximation - darken)
            if (ao.isValid())
            {
                g.setOpacity(0.3f);
                g.drawImage(ao, x, y, knobSize, knobSize,
                           0, 0, ao.getWidth(), ao.getHeight());
            }

            // 4. Center LED glow (additive - brightens)
            if (glow_core.isValid())
            {
                g.setOpacity(0.7f);
                g.drawImage(glow_core, x, y, knobSize, knobSize,
                           0, 0, glow_core.getWidth(), glow_core.getHeight());
            }

            // 5. Crystal glow overlay
            if (glow_crystal.isValid())
            {
                g.setOpacity(0.6f);
                g.drawImage(glow_crystal, x, y, knobSize, knobSize,
                           0, 0, glow_crystal.getWidth(), glow_crystal.getHeight());
            }

            // 6. Rotation indicator
            if (indicator.isValid())
            {
                g.setOpacity(1.0f);
                g.drawImage(indicator, x, y, knobSize, knobSize,
                           0, 0, indicator.getWidth(), indicator.getHeight());
            }
        }
        else
        {
            // Asset not found - draw error
            g.setColour(juce::Colours::red);
            g.drawRect(x, y, knobSize, knobSize, 2);
            g.drawText("Asset Not Found", x, y, knobSize, knobSize, juce::Justification::centred);
        }

        x += spacing;
    }

    // Draw layer legend
    y += knobSize + 50;
    g.setColour(juce::Colours::white.withAlpha(0.7f));
    g.setFont(14.0f);
    g.drawText("Each knob has 11 PBR layers:", 50, y, 600, 25, juce::Justification::left);

    juce::StringArray layers = {
        "1. Albedo (base color) - AI generated via DALL-E 3",
        "2. AO (ambient occlusion) - depth shadows",
        "3. Roughness (surface variation) - micro-detail",
        "4. Normal (bump mapping) - surface geometry",
        "5. Glow Core (center LED) - blue radial glow",
        "6. Glow Crystal (material glow) - crystal shine",
        "7. Bloom (post-process) - soft halo",
        "8. Light Wrap (rim lighting) - edge highlights",
        "9. Highlight (specular) - surface reflections",
        "10. Indicator (rotation pointer) - white line",
        "11. Contact Shadow (ground) - soft drop shadow"
    };

    int ly = y + 30;
    g.setFont(12.0f);
    for (auto& layer : layers)
    {
        g.drawText(layer, 50, ly, 800, 20, juce::Justification::left);
        ly += 20;
    }

    // Cost info
    ly += 20;
    g.setColour(juce::Colours::lightgreen);
    g.setFont(14.0f);
    g.drawText("Total Generation Cost: $0.16 USD (4× DALL-E 3 HD + 40× derived layers)",
               50, ly, 800, 25, juce::Justification::left);
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(70); // Space for title

    body.setBounds(bounds);

    // Position knobs in top-right corner for side-by-side comparison
    int knobSize = 200;
    int knobHeight = 240;
    int spacing = 30;  // Gap between knobs
    int rightMargin = 50;
    int topMargin = 100;

    // Calculate total width needed for both knobs
    int totalKnobWidth = (knobSize * 2) + spacing;
    int startX = getWidth() - totalKnobWidth - rightMargin;

    // CPU blend knob (left)
    if (cpuBlendKnob)
    {
        cpuBlendKnob->setBounds(
            startX,
            topMargin,
            knobSize,
            knobHeight
        );
    }

    // Filmstrip knob (right) - zero CPU cost
    if (filmstripKnob)
    {
        filmstripKnob->setBounds(
            startX + knobSize + spacing,
            topMargin,
            knobSize,
            knobHeight
        );
    }
}
