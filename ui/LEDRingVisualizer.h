// Monument Reverb - LED Ring Visualizer
// Advanced LED ring rendering with animations and effects

#pragma once

#include <JuceHeader.h>

namespace monument {

/**
 * Standalone LED ring component for parameter visualization
 *
 * Can be overlaid on knobs or used independently for meters/displays
 */
class LEDRingVisualizer : public juce::Component,
                          private juce::Timer
{
public:
    enum class Style
    {
        Dots,           // Discrete LED dots
        Arc,            // Continuous arc
        Segments,       // 7-segment style blocks
        Pulse           // Pulsing animation
    };

    LEDRingVisualizer()
    {
        startTimerHz(60);  // 60 FPS animation
    }

    /** Set current value (0-1) */
    void setValue(float newValue)
    {
        targetValue = juce::jlimit(0.0f, 1.0f, newValue);

        // Smooth animation
        if (animationEnabled)
        {
            // Will animate in timerCallback
        }
        else
        {
            currentValue = targetValue;
            repaint();
        }
    }

    /** Set LED ring style */
    void setStyle(Style newStyle)
    {
        style = newStyle;
        repaint();
    }

    /** Set number of LEDs (for Dots and Segments styles) */
    void setNumLEDs(int count)
    {
        numLEDs = juce::jlimit(8, 128, count);
        repaint();
    }

    /** Set LED colors (gradient from min to max value) */
    void setColorGradient(juce::Colour minColor, juce::Colour maxColor)
    {
        colorMin = minColor;
        colorMax = maxColor;
        repaint();
    }

    /** Enable smooth animation */
    void setAnimationEnabled(bool enabled)
    {
        animationEnabled = enabled;
    }

    /** Set ring thickness (0-1, relative to radius) */
    void setThickness(float thickness)
    {
        ringThickness = juce::jlimit(0.1f, 0.5f, thickness);
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        switch (style)
        {
            case Style::Dots:
                paintDots(g);
                break;
            case Style::Arc:
                paintArc(g);
                break;
            case Style::Segments:
                paintSegments(g);
                break;
            case Style::Pulse:
                paintPulse(g);
                break;
        }
    }

private:
    void timerCallback() override
    {
        if (!animationEnabled)
            return;

        // Smooth animation towards target
        const float alpha = 0.15f;
        currentValue = alpha * targetValue + (1.0f - alpha) * currentValue;

        if (std::abs(currentValue - targetValue) > 0.001f)
            repaint();

        // Update pulse animation phase
        if (style == Style::Pulse)
        {
            pulsePhase += 0.05f;
            if (pulsePhase > juce::MathConstants<float>::twoPi)
                pulsePhase -= juce::MathConstants<float>::twoPi;
            repaint();
        }
    }

    void paintDots(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat();
        const float centerX = bounds.getCentreX();
        const float centerY = bounds.getCentreY();
        const float outerRadius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.45f;

        const float startAngle = -juce::MathConstants<float>::pi * 0.75f;  // -135°
        const float endAngle = juce::MathConstants<float>::pi * 0.75f;     // +135°
        const float totalAngle = endAngle - startAngle;

        const int litDots = static_cast<int>(currentValue * numLEDs);

        for (int i = 0; i < numLEDs; ++i)
        {
            const float angle = startAngle + (totalAngle * i) / (numLEDs - 1);
            const float dotX = centerX + outerRadius * std::cos(angle);
            const float dotY = centerY + outerRadius * std::sin(angle);

            const bool isLit = i < litDots;
            const float brightness = isLit ? 1.0f : 0.15f;

            // Color interpolation based on position
            const float colorPos = static_cast<float>(i) / numLEDs;
            auto dotColor = colorMin.interpolatedWith(colorMax, colorPos);

            // Draw LED dot with glow
            const float dotSize = 6.0f;
            const float glowSize = 12.0f;

            if (isLit)
            {
                // Glow effect
                g.setGradientFill(juce::ColourGradient(
                    dotColor.withAlpha(0.6f), dotX, dotY,
                    dotColor.withAlpha(0.0f), dotX, dotY,
                    true));
                g.fillEllipse(dotX - glowSize/2, dotY - glowSize/2, glowSize, glowSize);
            }

            // LED dot
            g.setColour(dotColor.withBrightness(brightness));
            g.fillEllipse(dotX - dotSize/2, dotY - dotSize/2, dotSize, dotSize);
        }
    }

    void paintArc(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat();
        const float centerX = bounds.getCentreX();
        const float centerY = bounds.getCentreY();
        const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.45f;

        const float startAngle = -juce::MathConstants<float>::pi * 0.75f;
        const float endAngle = juce::MathConstants<float>::pi * 0.75f;
        const float totalAngle = endAngle - startAngle;
        const float valueAngle = startAngle + totalAngle * currentValue;

        // Background arc (unlit portion)
        juce::Path backgroundArc;
        backgroundArc.addCentredArc(centerX, centerY, radius, radius,
                                   0.0f, valueAngle, endAngle, true);
        g.setColour(colorMin.withBrightness(0.2f));
        g.strokePath(backgroundArc, juce::PathStrokeType(8.0f * ringThickness));

        // Foreground arc (lit portion)
        juce::Path foregroundArc;
        foregroundArc.addCentredArc(centerX, centerY, radius, radius,
                                   0.0f, startAngle, valueAngle, true);

        // Gradient along the arc
        g.setGradientFill(juce::ColourGradient(
            colorMin, centerX - radius, centerY,
            colorMax, centerX + radius, centerY,
            false));
        g.strokePath(foregroundArc, juce::PathStrokeType(12.0f * ringThickness,
                                                         juce::PathStrokeType::curved,
                                                         juce::PathStrokeType::rounded));

        // Inner glow
        g.setGradientFill(juce::ColourGradient(
            colorMax.withAlpha(0.3f), centerX, centerY,
            colorMax.withAlpha(0.0f), centerX + radius, centerY,
            true));
        g.strokePath(foregroundArc, juce::PathStrokeType(20.0f * ringThickness));
    }

    void paintSegments(juce::Graphics& g)
    {
        auto bounds = getLocalBounds().toFloat();
        const float centerX = bounds.getCentreX();
        const float centerY = bounds.getCentreY();
        const float outerRadius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.45f;
        const float innerRadius = outerRadius * (1.0f - ringThickness);

        const float startAngle = -juce::MathConstants<float>::pi * 0.75f;
        const float endAngle = juce::MathConstants<float>::pi * 0.75f;
        const float totalAngle = endAngle - startAngle;

        const int segmentCount = numLEDs / 2;  // Fewer segments than dots
        const float segmentAngle = totalAngle / segmentCount;
        const float gapAngle = segmentAngle * 0.15f;  // 15% gap between segments

        const int litSegments = static_cast<int>(currentValue * segmentCount);

        for (int i = 0; i < segmentCount; ++i)
        {
            const float angle1 = startAngle + i * segmentAngle + gapAngle / 2;
            const float angle2 = angle1 + segmentAngle - gapAngle;

            const bool isLit = i < litSegments;
            const float brightness = isLit ? 1.0f : 0.1f;

            const float colorPos = static_cast<float>(i) / segmentCount;
            auto segmentColor = colorMin.interpolatedWith(colorMax, colorPos);

            // Create segment path
            juce::Path segment;
            segment.startNewSubPath(centerX + innerRadius * std::cos(angle1),
                                   centerY + innerRadius * std::sin(angle1));
            segment.lineTo(centerX + outerRadius * std::cos(angle1),
                          centerY + outerRadius * std::sin(angle1));
            segment.addArc(centerX - outerRadius, centerY - outerRadius,
                          outerRadius * 2, outerRadius * 2,
                          angle1, angle2, true);
            segment.lineTo(centerX + innerRadius * std::cos(angle2),
                          centerY + innerRadius * std::sin(angle2));
            segment.addArc(centerX - innerRadius, centerY - innerRadius,
                          innerRadius * 2, innerRadius * 2,
                          angle2, angle1, false);
            segment.closeSubPath();

            g.setColour(segmentColor.withBrightness(brightness));
            g.fillPath(segment);

            // Highlight on lit segments
            if (isLit)
            {
                g.setColour(segmentColor.brighter(0.3f).withAlpha(0.6f));
                g.strokePath(segment, juce::PathStrokeType(1.0f));
            }
        }
    }

    void paintPulse(juce::Graphics& g)
    {
        // Base arc rendering
        paintArc(g);

        // Add pulsing glow effect
        auto bounds = getLocalBounds().toFloat();
        const float centerX = bounds.getCentreX();
        const float centerY = bounds.getCentreY();
        const float radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.45f;

        const float pulseIntensity = (std::sin(pulsePhase) + 1.0f) * 0.5f;  // 0-1

        const float startAngle = -juce::MathConstants<float>::pi * 0.75f;
        const float endAngle = juce::MathConstants<float>::pi * 0.75f;
        const float totalAngle = endAngle - startAngle;
        const float valueAngle = startAngle + totalAngle * currentValue;

        juce::Path glowArc;
        glowArc.addCentredArc(centerX, centerY, radius, radius,
                             0.0f, startAngle, valueAngle, true);

        g.setColour(colorMax.withAlpha(pulseIntensity * 0.5f));
        g.strokePath(glowArc, juce::PathStrokeType(25.0f * ringThickness));
    }

    Style style{Style::Dots};
    int numLEDs{32};
    float currentValue{0.0f};
    float targetValue{0.0f};
    bool animationEnabled{true};
    float ringThickness{0.3f};

    juce::Colour colorMin{juce::Colours::blue};
    juce::Colour colorMax{juce::Colours::cyan};

    float pulsePhase{0.0f};
};

/**
 * Multi-ring visualizer for displaying multiple parameters
 *
 * Example: Inner ring = Time, Middle ring = Density, Outer ring = Gravity
 */
class MultiRingVisualizer : public juce::Component
{
public:
    void addRing(const juce::String& name, juce::Colour color)
    {
        auto ring = std::make_unique<LEDRingVisualizer>();
        ring->setColorGradient(color.darker(), color);
        ring->setStyle(LEDRingVisualizer::Style::Arc);
        ring->setThickness(0.2f);

        rings.add(ring.release());
        ringNames.add(name);
        addAndMakeVisible(rings.getLast());

        resized();
    }

    void setRingValue(int ringIndex, float value)
    {
        if (juce::isPositiveAndBelow(ringIndex, rings.size()))
            rings[ringIndex]->setValue(value);
    }

    void resized() override
    {
        const int numRings = rings.size();
        if (numRings == 0)
            return;

        auto bounds = getLocalBounds();
        const int centerX = bounds.getCentreX();
        const int centerY = bounds.getCentreY();
        const int maxRadius = juce::jmin(bounds.getWidth(), bounds.getHeight()) / 2;

        const int ringSpacing = maxRadius / (numRings + 1);

        for (int i = 0; i < numRings; ++i)
        {
            const int radius = maxRadius - i * ringSpacing;
            const int size = radius * 2;
            rings[i]->setBounds(centerX - radius, centerY - radius, size, size);
        }
    }

private:
    juce::OwnedArray<LEDRingVisualizer> rings;
    juce::StringArray ringNames;
};

} // namespace monument
