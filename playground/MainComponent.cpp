// PHASE 4: Main component with ComponentPack integration
#include "MainComponent.h"

namespace monument::playground
{

MainComponent::MainComponent()
{
    // Set up title label
    titleLabel.setText("Monument UI Playground", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(titleLabel);

    // Set up status label - will be updated based on asset load result
    statusLabel.setFont(juce::FontOptions(14.0f));
    statusLabel.setJustificationType(juce::Justification::centred);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(statusLabel);

    // Enable keyboard focus for this component
    setWantsKeyboardFocus(true);

    // Enable mouse move tracking for cursor-reactive particles
    setMouseCursor(juce::MouseCursor::NormalCursor);

    // DEMO: Register available component packs
    availablePacks.push_back("knob_geode");
    availablePacks.push_back("knob_metal");
    availablePacks.push_back("knob_industrial");

    // Try to load first pack
    currentPackIndex = 0;
    assetLoadSuccess = loadComponentPack(availablePacks[currentPackIndex]);

    if (!assetLoadSuccess)
    {
        // Fall back to test pattern if assets fail to load
        DBG("MainComponent: Failed to load assets, using test pattern");
        createTestPattern();
    }

    updateStatusLabel();

    // PHASE 5: Initialize audio system
    auto result = audioDeviceManager.initialiseWithDefaultDevices(0, 2);  // 0 inputs, 2 outputs
    if (result.isNotEmpty())
    {
        DBG("MainComponent: Audio device init failed: " + result);
    }
    audioDeviceManager.addAudioCallback(&audioEngine);

    // PHASE 5: Configure audio parameters
    audioEngine.setGain(0.5f);         // Higher gain for audible sound and RMS
    audioEngine.setFrequency(220.0f);  // A3 note (starting frequency)
    audioEngine.setNoiseAmount(0.08f); // Slightly more noise for texture
    audioEngine.setEnabled(audioEnabled);  // Now defaults to true

    // PHASE 5: Initialize smoothing for audio-reactive parameters
    smoothedGlow.reset(60.0, 0.18);  // 60Hz update rate, 180ms smoothing

    // PHASE 5: Start timer for audio metric polling (60Hz)
    startTimerHz(60);

    // PHASE 6: Initialize particle system with embers preset
    {
        using namespace vds::particles;

        // Load embers.json preset (high-velocity, fluid motion)
        juce::File presetFile = juce::File::getCurrentWorkingDirectory()
            .getChildFile("Source/Particles/presets/embers.json");

        if (presetFile.existsAsFile())
        {
            auto jsonString = presetFile.loadFileAsString();
            ParticleBehaviorSpec spec;
            juce::String error;

            auto result = ParticleBehaviorDSL::parseFromJsonString(jsonString, spec, error);
            if (result.wasOk())
            {
                particleSystem.setBehavior(spec);
                DBG("MainComponent: Particle system initialized with embers preset (high-velocity)");
            }
            else
            {
                DBG("MainComponent: Failed to parse embers.json - " + error);
            }
        }
        else
        {
            DBG("MainComponent: embers.json not found at " + presetFile.getFullPathName());
        }
    }

    // Set initial size
    setSize(800, 600);
}

MainComponent::~MainComponent()
{
    // PHASE 5: Clean up audio system
    stopTimer();
    audioDeviceManager.removeAudioCallback(&audioEngine);
}

void MainComponent::createTestPattern()
{
    // Create test pattern with multiple layers to validate:
    // 1. Straight-alpha blending
    // 2. Different blend modes
    // 3. Opacity control
    // 4. Alpha channel preservation

    const int size = 256;

    // === LAYER 1: Base gradient (bottom layer) ===
    juce::Image baseLayer(juce::Image::ARGB, size, size, true);
    {
        juce::Graphics g(baseLayer);

        // Radial gradient from center (dark blue to transparent)
        juce::ColourGradient gradient(
            juce::Colours::darkblue.withAlpha(0.8f),
            size * 0.5f, size * 0.5f,
            juce::Colours::transparentBlack,
            size * 0.5f, 0.0f,
            true
        );

        g.setGradientFill(gradient);
        g.fillEllipse(0, 0, (float)size, (float)size);
    }
    compositor.addLayer(baseLayer, "Base Gradient", LayerCompositor::BlendMode::Normal, 1.0f);

    // === LAYER 2: Circle pattern with multiply blend ===
    juce::Image circlePattern(juce::Image::ARGB, size, size, true);
    {
        juce::Graphics g(circlePattern);
        g.setColour(juce::Colours::orange.withAlpha(0.7f));

        // Draw multiple circles
        for (int i = 0; i < 5; ++i)
        {
            float radius = 30.0f + i * 20.0f;
            float x = size * 0.5f - radius;
            float y = size * 0.5f - radius;
            g.drawEllipse(x, y, radius * 2, radius * 2, 3.0f);
        }
    }
    compositor.addLayer(circlePattern, "Circle Pattern", LayerCompositor::BlendMode::Normal, 0.8f);

    // === LAYER 3: Highlight with screen blend ===
    juce::Image highlight(juce::Image::ARGB, size, size, true);
    {
        juce::Graphics g(highlight);

        // Top-left highlight
        juce::ColourGradient gradient(
            juce::Colours::white.withAlpha(0.9f),
            size * 0.3f, size * 0.3f,
            juce::Colours::transparentBlack,
            size * 0.6f, size * 0.6f,
            false
        );

        g.setGradientFill(gradient);
        g.fillEllipse(size * 0.1f, size * 0.1f, size * 0.5f, size * 0.5f);
    }
    compositor.addLayer(highlight, "Highlight", LayerCompositor::BlendMode::Screen, 0.6f);

    // === LAYER 4: Shadow with multiply blend ===
    juce::Image shadow(juce::Image::ARGB, size, size, true);
    {
        juce::Graphics g(shadow);

        // Bottom-right shadow
        juce::ColourGradient gradient(
            juce::Colours::black.withAlpha(0.8f),
            size * 0.7f, size * 0.7f,
            juce::Colours::transparentBlack,
            size * 0.4f, size * 0.4f,
            false
        );

        g.setGradientFill(gradient);
        g.fillEllipse(size * 0.5f, size * 0.5f, size * 0.5f, size * 0.5f);
    }
    compositor.addLayer(shadow, "Shadow", LayerCompositor::BlendMode::Multiply, 0.7f);

    // === LAYER 5: Additive glow ===
    juce::Image glow(juce::Image::ARGB, size, size, true);
    {
        juce::Graphics g(glow);

        // Center glow
        juce::ColourGradient gradient(
            juce::Colours::cyan.withAlpha(0.7f),
            size * 0.5f, size * 0.5f,
            juce::Colours::transparentBlack,
            size * 0.5f, size * 0.3f,
            true
        );

        g.setGradientFill(gradient);
        g.fillEllipse(size * 0.35f, size * 0.35f, size * 0.3f, size * 0.3f);
    }
    compositor.addLayer(glow, "Glow", LayerCompositor::BlendMode::Additive, 0.5f);

    // Composite all layers
    compositedImage = compositor.composite();

    DBG("MainComponent: Created test pattern with " +
        juce::String(compositor.getLayerCount()) + " layers, " +
        juce::String(compositedImage.getWidth()) + "x" +
        juce::String(compositedImage.getHeight()));
}

void MainComponent::paint(juce::Graphics& g)
{
    // Dark background
    g.fillAll(juce::Colour(0xff0b0d10));

    // Draw a simple frame to show the window is active
    g.setColour(juce::Colours::darkgrey);
    g.drawRect(getLocalBounds(), 2);

    // Calculate display area for composited image (centered)
    if (compositedImage.isValid())
    {
        auto bounds = getLocalBounds();
        bounds.removeFromTop(100);    // Space for title
        bounds.removeFromBottom(60);  // Space for status + info

        // Center the image
        int imgSize = juce::jmin(bounds.getWidth(), bounds.getHeight()) - 40;
        int x = bounds.getCentreX() - imgSize / 2;
        int y = bounds.getCentreY() - imgSize / 2;

        juce::Rectangle<int> imgBounds(x, y, imgSize, imgSize);

        // Draw composited result
        if (showDebugAlpha)
        {
            // Show alpha visualization
            juce::Image alphaVis = compositor.getAlphaVisualization();
            g.drawImage(alphaVis, imgBounds.toFloat());

            // Label
            g.setColour(juce::Colours::yellow);
            g.setFont(juce::FontOptions(12.0f));
            g.drawText("ALPHA VISUALIZATION", imgBounds.getX(), imgBounds.getBottom() + 5,
                      imgBounds.getWidth(), 20, juce::Justification::centred);
        }
        else
        {
            // Show composited image
            g.drawImage(compositedImage, imgBounds.toFloat());
        }

        // Draw border around image
        g.setColour(juce::Colours::grey);
        g.drawRect(imgBounds, 1);

        // Draw layer info below image
        g.setColour(juce::Colours::lightgrey);
        g.setFont(juce::FontOptions(11.0f));

        int infoY = imgBounds.getBottom() + 25;
        g.drawText(juce::String(compositor.getLayerCount()) + " layers composited | " +
                   juce::String(compositedImage.getWidth()) + "x" +
                   juce::String(compositedImage.getHeight()) + " ARGB",
                   imgBounds.getX(), infoY, imgBounds.getWidth(), 20,
                   juce::Justification::centred);

        // Draw instructions
        g.setColour(juce::Colours::darkgrey);
        g.setFont(juce::FontOptions(10.0f));
        g.drawText("Press 'D' to toggle alpha debug visualization",
                   imgBounds.getX(), infoY + 20, imgBounds.getWidth(), 20,
                   juce::Justification::centred);
    }
    else
    {
        // Error message if compositor failed
        g.setColour(juce::Colours::red);
        g.setFont(juce::FontOptions(16.0f));
        g.drawText("Failed to composite layers",
                   getLocalBounds(), juce::Justification::centred);
    }

    // PHASE 7: Render particles with additive blending
    const auto& particles = particleSystem.getParticles();
    if (!particles.isEmpty())
    {
        // Use additive blend mode for glow effect
        // Note: JUCE doesn't have direct additive blend, so we use transparency
        // and saturated colors for a glow-like appearance

        for (const auto& particle : particles)
        {
            // Calculate opacity based on energy (0..1) and lifetime progress
            float lifetimeProgress = particle.ageSec / particle.lifetimeSec;
            float fadeOut = 1.0f - lifetimeProgress;  // Fade out over lifetime
            float opacity = particle.energy * fadeOut;
            opacity = juce::jlimit(0.0f, 1.0f, opacity);

            // Calculate particle color with energy-based intensity
            // Use bright glowing ember colors for maximum visibility
            float energyGlow = particle.energy;
            auto particleColor = juce::Colour::fromFloatRGBA(
                1.0f,                              // R: 1.0 (bright orange-red)
                0.5f + energyGlow * 0.4f,         // G: 0.5-0.9 (bright orange)
                0.15f + energyGlow * 0.25f,       // B: 0.15-0.4 (warm glow)
                opacity                            // A: full opacity from energy calc
            );

            // Scale particle size - DRAMATICALLY LARGER for visibility
            float visualSize = particle.size * 20.0f;  // Increased from 8.0f to 20.0f!

            // Draw particle as a filled circle with glow
            g.setColour(particleColor);
            g.fillEllipse(
                particle.position.x - visualSize,
                particle.position.y - visualSize,
                visualSize * 2.0f,
                visualSize * 2.0f
            );

            // Add outer glow ring for enhanced effect (when energy is high)
            if (particle.energy > 0.3f)
            {
                float glowSize = visualSize * 2.0f;  // Larger glow
                auto glowColor = particleColor.withAlpha(opacity * 0.4f);
                g.setColour(glowColor);
                g.drawEllipse(
                    particle.position.x - glowSize,
                    particle.position.y - glowSize,
                    glowSize * 2.0f,
                    glowSize * 2.0f,
                    2.0f
                );
            }
        }

        // Draw particle count on screen for debugging
        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.setFont(juce::FontOptions(14.0f, juce::Font::bold));
        g.drawText("Particles: " + juce::String(particles.size()),
                   10, 10, 150, 30, juce::Justification::left);
    }
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();

    // Title at top
    titleLabel.setBounds(bounds.removeFromTop(80).reduced(20));

    // Status at bottom
    statusLabel.setBounds(bounds.removeFromBottom(40).reduced(20));

    // Calculate knob bounds for interaction (centered in remaining space)
    auto remainingBounds = bounds;
    remainingBounds.removeFromBottom(60);  // Space for info text
    int knobSize = juce::jmin(remainingBounds.getWidth(), remainingBounds.getHeight()) - 40;
    int knobX = remainingBounds.getCentreX() - knobSize / 2;
    int knobY = remainingBounds.getCentreY() - knobSize / 2;
    knobBounds = juce::Rectangle<int>(knobX, knobY, knobSize, knobSize);

    // PHASE 6: Update particle system viewport
    particleSystem.setViewport(getLocalBounds().toFloat());
    particleSystem.setEmitterPosition(getLocalBounds().getCentre().toFloat());
}

bool MainComponent::loadComponentPack(const juce::String& packName)
{
    // Load a component pack by name from assets directory

    // Get manifest file path (relative to executable)
    auto executableDir = juce::File::getSpecialLocation(juce::File::currentExecutableFile).getParentDirectory();

    // Try multiple paths for the manifest (Debug build vs installed location)
    std::vector<juce::File> searchPaths = {
        // Path 1: Relative to build directory (during development)
        executableDir.getParentDirectory().getParentDirectory().getParentDirectory()
            .getChildFile("assets/" + packName + "/manifest.json"),

        // Path 2: Absolute path (development environment)
        juce::File("/Users/noisebox/Documents/3_Development/Repos/monument-reverb/assets/" + packName + "/manifest.json")
    };

    juce::File manifestFile;
    for (const auto& path : searchPaths)
    {
        if (path.existsAsFile())
        {
            manifestFile = path;
            DBG("MainComponent: Found manifest at: " + manifestFile.getFullPathName());
            break;
        }
    }

    if (!manifestFile.existsAsFile())
    {
        DBG("MainComponent: Could not find manifest.json for pack: " + packName);
        return false;
    }

    // Load the component pack
    juce::String error;
    if (!componentPack.loadFromManifest(manifestFile, error))
    {
        DBG("MainComponent: Failed to load ComponentPack: " + error);
        return false;
    }

    DBG("MainComponent: Loaded ComponentPack '" + packName + "' with " +
        juce::String(componentPack.getLayers().size()) + " layers");

    // Clear compositor and load all layers from the pack
    compositor.clear();

    const auto& layers = componentPack.getLayers();
    const auto& rootDir = componentPack.getRootDirectory();

    int loadedCount = 0;
    for (const auto& layer : layers)
    {
        // Load the image file
        auto imageFile = rootDir.getChildFile(layer.file);
        if (!imageFile.existsAsFile())
        {
            DBG("MainComponent: Missing layer file: " + layer.file);
            continue;
        }

        // Convert blend mode from ComponentPack to LayerCompositor
        auto blendMode = convertBlendMode(layer.blend);

        // Add to compositor
        if (compositor.loadImage(imageFile, layer.name, blendMode, layer.opacity))
        {
            loadedCount++;
            DBG("MainComponent: Loaded layer '" + layer.name + "' (" + layer.file + ") " +
                "blend=" + juce::String((int)blendMode) + " opacity=" + juce::String(layer.opacity));
        }
        else
        {
            DBG("MainComponent: Failed to load image: " + imageFile.getFullPathName());
        }
    }

    if (loadedCount == 0)
    {
        DBG("MainComponent: No layers were loaded successfully");
        return false;
    }

    // Composite all layers
    compositedImage = compositor.composite();

    DBG("MainComponent: Successfully composited " + juce::String(loadedCount) + " layers, " +
        juce::String(compositedImage.getWidth()) + "x" +
        juce::String(compositedImage.getHeight()));

    return true;
}

bool MainComponent::loadKnobGeodeAssets()
{
    // Legacy method - now calls loadComponentPack
    return loadComponentPack("knob_geode");
}

bool MainComponent::keyPressed(const juce::KeyPress& key)
{
    // Handle keyboard input for demo navigation
    if (key == juce::KeyPress::leftKey || key == juce::KeyPress::rightKey)
    {
        // Switch between component packs
        if (key == juce::KeyPress::leftKey)
        {
            currentPackIndex = (currentPackIndex - 1 + availablePacks.size()) % availablePacks.size();
        }
        else
        {
            currentPackIndex = (currentPackIndex + 1) % availablePacks.size();
        }

        // Load new pack
        assetLoadSuccess = loadComponentPack(availablePacks[currentPackIndex]);
        if (!assetLoadSuccess)
        {
            DBG("MainComponent: Failed to load pack: " + availablePacks[currentPackIndex]);
        }

        updateStatusLabel();
        repaint();
        return true;
    }
    else if (key == juce::KeyPress('d') || key == juce::KeyPress('D'))
    {
        // Toggle debug alpha visualization
        showDebugAlpha = !showDebugAlpha;
        repaint();
        return true;
    }
    else if (key == juce::KeyPress('a') || key == juce::KeyPress('A'))
    {
        // PHASE 5: Toggle audio on/off
        audioEnabled = !audioEnabled;
        audioEngine.setEnabled(audioEnabled);
        DBG("Audio " + juce::String(audioEnabled ? "enabled" : "disabled"));
        updateStatusLabel();
        return true;
    }

    return false;
}

void MainComponent::updateStatusLabel()
{
    if (assetLoadSuccess && currentPackIndex < availablePacks.size())
    {
        auto packName = availablePacks[currentPackIndex];

        // Calculate current frequency from knob rotation
        float normalized = (knobRotation + 150.0f) / 300.0f;
        float logMin = std::log(40.0f);
        float logMax = std::log(2000.0f);
        float frequency = std::exp(logMin + normalized * (logMax - logMin));

        juce::String audioState = audioEnabled ? "🔊" : "🔇";
        statusLabel.setText(
            packName + " (" + juce::String(currentPackIndex + 1) + "/" +
            juce::String(availablePacks.size()) + ") | " +
            audioState + " " + juce::String(frequency, 0) + " Hz | " +
            "Drag knob for frequency, ← → packs, A audio",
            juce::dontSendNotification
        );
    }
    else
    {
        statusLabel.setText("Asset load failed - Using test pattern | Drag knob, A audio, D debug", juce::dontSendNotification);
    }
}

LayerCompositor::BlendMode MainComponent::convertBlendMode(ComponentPack::BlendMode mode)
{
    switch (mode)
    {
        case ComponentPack::BlendMode::Normal:   return LayerCompositor::BlendMode::Normal;
        case ComponentPack::BlendMode::Add:      return LayerCompositor::BlendMode::Additive;
        case ComponentPack::BlendMode::Screen:   return LayerCompositor::BlendMode::Screen;
        case ComponentPack::BlendMode::Multiply: return LayerCompositor::BlendMode::Multiply;
        default:                                 return LayerCompositor::BlendMode::Normal;
    }
}

void MainComponent::timerCallback()
{
    // PHASE 5: Poll audio metrics and update visual parameters
    if (audioEnabled)
    {
        auto metrics = audioEngine.getMetrics();

        // Update smoothed glow intensity based on RMS
        smoothedGlow.setTargetValue(metrics.rms * 10.0f);  // Scale RMS for visibility
        smoothedGlow.skip(1);

        // PHASE 6: Feed audio metrics to particle system
        particleSystem.setAudioRms(metrics.rms);
        particleSystem.setAudioPeak(metrics.peak);

        // Log metrics periodically
        static int frameCount = 0;
        if (++frameCount % 60 == 0)  // Log once per second
        {
            DBG("Audio Metrics - RMS: " + juce::String(metrics.rms, 3) +
                " Peak: " + juce::String(metrics.peak, 3) +
                " Centroid: " + juce::String(metrics.centroid, 3) +
                " Glow: " + juce::String(smoothedGlow.getCurrentValue(), 3) +
                " Particles: " + juce::String(particleSystem.getParticles().size()));
        }
    }
    else
    {
        // Fade out glow when audio is disabled
        smoothedGlow.setTargetValue(0.0f);
        smoothedGlow.skip(1);

        // PHASE 6: Still update particle system with zero audio input
        particleSystem.setAudioRms(0.0f);
        particleSystem.setAudioPeak(0.0f);
    }

    // PHASE 6: Update particle simulation (60Hz)
    particleSystem.update(1.0f / 60.0f);

    // PHASE 7: Trigger repaint for particle rendering (when particles exist)
    if (!particleSystem.getParticles().isEmpty())
    {
        repaint();
    }
}

void MainComponent::mouseMove(const juce::MouseEvent& event)
{
    // PHASE 7: Cursor-reactive particles - emitter follows mouse
    particleSystem.setEmitterPosition(event.getPosition().toFloat());
}

void MainComponent::mouseDown(const juce::MouseEvent& event)
{
    if (knobBounds.contains(event.getPosition()))
    {
        lastMousePos = event.getPosition();
    }
}

void MainComponent::mouseDrag(const juce::MouseEvent& event)
{
    if (!knobBounds.contains(event.getMouseDownPosition()))
        return;

    // Calculate vertical drag distance (up = increase frequency, down = decrease)
    int dragDelta = lastMousePos.y - event.getPosition().y;
    lastMousePos = event.getPosition();

    // Update knob rotation (-150° to +150°, 300° range)
    float rotationDelta = dragDelta * 0.5f;  // Sensitivity factor
    knobRotation = juce::jlimit(-150.0f, 150.0f, knobRotation + rotationDelta);

    // Map rotation to frequency (40Hz - 2000Hz, logarithmic scale)
    // knobRotation: -150 to +150
    // normalized: 0.0 to 1.0
    float normalized = (knobRotation + 150.0f) / 300.0f;

    // Logarithmic mapping for musical frequency range
    float minFreq = 40.0f;
    float maxFreq = 2000.0f;
    float logMin = std::log(minFreq);
    float logMax = std::log(maxFreq);
    float frequency = std::exp(logMin + normalized * (logMax - logMin));

    // Update audio engine
    audioEngine.setFrequency(frequency);

    // Update status label to show current frequency
    updateStatusLabel();
    repaint();
}

} // namespace monument::playground
