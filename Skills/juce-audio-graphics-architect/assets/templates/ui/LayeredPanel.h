#pragma once

#include <JuceHeader.h>

class LayeredPanel : public juce::Component, private juce::Timer
{
public:
    void setTargetAlpha(float newAlpha)
    {
        targetAlpha = juce::jlimit(0.0f, 1.0f, newAlpha);
        startTimerHz(60);
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        g.beginTransparencyLayer(currentAlpha);
        g.setColour(juce::Colours::black.withAlpha(0.25f));
        g.fillRoundedRectangle(bounds, 16.0f);

        g.setColour(juce::Colours::white.withAlpha(0.08f));
        g.fillRoundedRectangle(bounds.reduced(6.0f), 12.0f);
        g.endTransparencyLayer();
    }

private:
    void timerCallback() override
    {
        currentAlpha += (targetAlpha - currentAlpha) * 0.15f;
        repaint();

        if (std::abs(targetAlpha - currentAlpha) < 0.01f)
            stopTimer();
    }

    float currentAlpha = 1.0f;
    float targetAlpha = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LayeredPanel)
};
