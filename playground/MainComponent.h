// PHASE 7: Main component with Audio → Visual + Particle rendering
#pragma once
#include <JuceHeader.h>
#include "LayerCompositor.h"
#include "ComponentPack.h"
#include "AudioEngine.h"
#include "../Source/Particles/ParticleSystem.h"

namespace monument::playground
{

class MainComponent : public juce::Component, private juce::Timer
{
public:
    MainComponent();
    ~MainComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    bool keyPressed(const juce::KeyPress& key) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseMove(const juce::MouseEvent& event) override;

private:
    // PHASE 5: Timer callback for audio metric polling
    void timerCallback() override;

private:
    // UI elements
    juce::Label titleLabel;
    juce::Label statusLabel;

    // PHASE 3: Layer compositor for RGBA rendering
    LayerCompositor compositor;
    juce::Image compositedImage;
    bool showDebugAlpha = false;

    // PHASE 4: Component pack for asset loading
    ComponentPack componentPack;
    bool assetLoadSuccess = false;

    // Demo: Multiple component packs
    std::vector<juce::String> availablePacks;
    int currentPackIndex = 0;

    // Create a test pattern for Phase 3 validation
    void createTestPattern();

    // PHASE 4: Load assets by pack name
    bool loadComponentPack(const juce::String& packName);

    // Legacy method (now calls loadComponentPack)
    bool loadKnobGeodeAssets();

    // Convert ComponentPack blend mode to LayerCompositor blend mode
    static LayerCompositor::BlendMode convertBlendMode(ComponentPack::BlendMode mode);

    // Update status label with current pack info
    void updateStatusLabel();

    // PHASE 5: Audio engine and device manager
    juce::AudioDeviceManager audioDeviceManager;
    AudioEngine audioEngine;

    // PHASE 5: Audio-reactive parameters
    juce::SmoothedValue<float> smoothedGlow{0.0f};
    bool audioEnabled = true;  // Default ON for interactive demo

    // PHASE 6: Particle system for audio-reactive visuals
    vds::particles::ParticleSystem particleSystem;

    // PHASE 7: Knob interaction
    float knobRotation = 0.0f;  // -150° to +150° range
    juce::Point<int> lastMousePos;
    juce::Rectangle<int> knobBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};

} // namespace monument::playground
