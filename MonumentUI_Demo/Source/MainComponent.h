#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "UI/AssetManager.h"
#include "UI/MonumentBodyComponent.h"
#include "Components/StoneKnobDemo.h"
#include "Components/FilmstripKnobDemo.h"

class MainComponent : public juce::Component
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    Monument::MonumentBodyComponent body;
    std::unique_ptr<Monument::StoneKnobDemo> cpuBlendKnob;
    std::unique_ptr<Monument::FilmstripKnobDemo> filmstripKnob;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
