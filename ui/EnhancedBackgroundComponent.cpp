#include "EnhancedBackgroundComponent.h"

EnhancedBackgroundComponent::EnhancedBackgroundComponent()
{
    initializeWisps();
    startTimerHz(30); // 30 FPS for smooth wisp animation
}

EnhancedBackgroundComponent::~EnhancedBackgroundComponent()
{
    stopTimer();
}

void EnhancedBackgroundComponent::paint(juce::Graphics& g)
{
    // Layer 1: Dark stone base texture
    paintStoneTexture(g);

    // Layer 2: Ethereal blue wisps (animated)
    if (animationEnabled)
        paintEtherealWisps(g);

    // Layer 3: Panel dividers
    paintPanelDividers(g);
}

void EnhancedBackgroundComponent::resized()
{
    needsTextureRegen = true;
    repaint();
}

void EnhancedBackgroundComponent::setPanelDividers(const std::vector<float>& dividerPositions)
{
    panelDividers = dividerPositions;
    repaint();
}

void EnhancedBackgroundComponent::setAnimationEnabled(bool enabled)
{
    animationEnabled = enabled;

    if (animationEnabled && !isTimerRunning())
        startTimerHz(30);
    else if (!animationEnabled && isTimerRunning())
        stopTimer();
}

void EnhancedBackgroundComponent::timerCallback()
{
    if (!animationEnabled)
        return;

    float deltaTime = 1.0f / 30.0f; // 30 FPS
    animationTime += deltaTime;

    updateWisps(deltaTime);
    repaint();
}

void EnhancedBackgroundComponent::paintStoneTexture(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    // Regenerate texture if needed
    if (needsTextureRegen || !stoneTexture.isValid())
    {
        stoneTexture = generateStoneTexture(bounds.getWidth(), bounds.getHeight());
        needsTextureRegen = false;
    }

    // Base dark gradient (top to bottom lighting)
    juce::ColourGradient gradient(
        juce::Colour(DARK_MID), 0.0f, 0.0f,
        juce::Colour(DARK_BASE), 0.0f, static_cast<float>(bounds.getHeight()),
        false);
    gradient.addColour(0.5, juce::Colour(DARK_HIGHLIGHT));

    g.setGradientFill(gradient);
    g.fillRect(bounds);

    // Overlay stone texture (subtle noise)
    g.setOpacity(0.08f); // Very subtle
    g.drawImage(stoneTexture, bounds.toFloat());
    g.setOpacity(1.0f);
}

void EnhancedBackgroundComponent::paintEtherealWisps(juce::Graphics& g)
{
    // Draw animated blue/cyan wisps
    for (const auto& wisp : wisps)
    {
        // Outer glow - use point at edge for radial gradient
        juce::Point<float> center(wisp.x, wisp.y);
        juce::Point<float> edge(wisp.x + wisp.radius * 2.0f, wisp.y);

        juce::ColourGradient gradient(
            wisp.color.withAlpha(wisp.alpha * 0.3f), center,
            wisp.color.withAlpha(0.0f), edge,
            true);

        g.setGradientFill(gradient);
        g.fillEllipse(
            wisp.x - wisp.radius * 2.0f,
            wisp.y - wisp.radius * 2.0f,
            wisp.radius * 4.0f,
            wisp.radius * 4.0f);

        // Inner bright core
        juce::Point<float> coreEdge(wisp.x + wisp.radius, wisp.y);

        juce::ColourGradient coreGradient(
            wisp.color.withAlpha(wisp.alpha * 0.6f), center,
            wisp.color.withAlpha(0.0f), coreEdge,
            true);

        g.setGradientFill(coreGradient);
        g.fillEllipse(
            wisp.x - wisp.radius,
            wisp.y - wisp.radius,
            wisp.radius * 2.0f,
            wisp.radius * 2.0f);
    }
}

void EnhancedBackgroundComponent::paintPanelDividers(juce::Graphics& g)
{
    auto bounds = getLocalBounds();

    for (float y : panelDividers)
    {
        // Dark shadow line
        g.setColour(juce::Colour(DARK_BASE).darker(0.3f));
        g.drawHorizontalLine(static_cast<int>(y), 0.0f, static_cast<float>(bounds.getWidth()));

        // Light highlight line (1px below)
        g.setColour(juce::Colour(DARK_HIGHLIGHT).brighter(0.2f));
        g.drawHorizontalLine(static_cast<int>(y + 1), 0.0f, static_cast<float>(bounds.getWidth()));
    }
}

juce::Image EnhancedBackgroundComponent::generateStoneTexture(int width, int height)
{
    juce::Image texture(juce::Image::ARGB, width, height, true);

    juce::Random random(12345); // Fixed seed for consistency

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            // Multi-octave noise for stone texture
            float noise = 0.0f;
            float amplitude = 1.0f;
            float frequency = 1.0f;

            for (int octave = 0; octave < 4; ++octave)
            {
                float nx = x * frequency * 0.01f;
                float ny = y * frequency * 0.01f;

                // Simple pseudo-noise
                float n = random.nextFloat() * amplitude;
                noise += n;

                amplitude *= 0.5f;
                frequency *= 2.0f;
            }

            // Normalize noise to 0-1 range
            noise = juce::jlimit(0.0f, 1.0f, noise * 0.5f);

            // Convert to grayscale
            juce::uint8 value = static_cast<juce::uint8>(noise * 255);
            texture.setPixelAt(x, y, juce::Colour(value, value, value));
        }
    }

    return texture;
}

void EnhancedBackgroundComponent::initializeWisps()
{
    juce::Random random(67890);

    // Create 8-12 ethereal wisps
    int numWisps = random.nextInt(5) + 8;
    wisps.clear();

    for (int i = 0; i < numWisps; ++i)
    {
        Wisp wisp;
        wisp.x = random.nextFloat() * 900.0f; // Assume ~900px width
        wisp.y = random.nextFloat() * 800.0f;
        wisp.radius = random.nextFloat() * 80.0f + 40.0f; // 40-120px radius
        wisp.alpha = random.nextFloat() * 0.4f + 0.2f; // 0.2-0.6 alpha
        wisp.vx = (random.nextFloat() - 0.5f) * 4.0f; // Slow horizontal drift
        wisp.vy = (random.nextFloat() - 0.5f) * 3.0f; // Slow vertical drift

        // Alternate between blue and cyan tints
        wisp.color = (i % 2 == 0)
            ? juce::Colour(BLUE_WISP)
            : juce::Colour(CYAN_WISP);

        wisps.push_back(wisp);
    }
}

void EnhancedBackgroundComponent::updateWisps(float deltaTime)
{
    auto bounds = getLocalBounds();

    for (auto& wisp : wisps)
    {
        // Update position
        wisp.x += wisp.vx * deltaTime;
        wisp.y += wisp.vy * deltaTime;

        // Wrap around screen edges
        if (wisp.x < -wisp.radius * 2)
            wisp.x = bounds.getWidth() + wisp.radius * 2;
        else if (wisp.x > bounds.getWidth() + wisp.radius * 2)
            wisp.x = -wisp.radius * 2;

        if (wisp.y < -wisp.radius * 2)
            wisp.y = bounds.getHeight() + wisp.radius * 2;
        else if (wisp.y > bounds.getHeight() + wisp.radius * 2)
            wisp.y = -wisp.radius * 2;

        // Subtle alpha pulsing
        float pulse = std::sin(animationTime * 0.5f + wisp.x * 0.01f) * 0.1f + 0.9f;
        wisp.alpha = juce::jlimit(0.2f, 0.6f, wisp.alpha * pulse);
    }
}
