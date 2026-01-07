#pragma once

#include <JuceHeader.h>

namespace vds::particles
{
    struct SignalSpec
    {
        float smoothingMs = 50.f;
        float clampLo = 0.f;
        float clampHi = 1.f;
        float threshold = 0.7f;   // for peak trigger-like signals
    };

    struct SmoothedSignal
    {
        void setSpec(const SignalSpec& s) { spec = s; reset(); }

        void reset(float value = 0.f)
        {
            current = juce::jlimit(spec.clampLo, spec.clampHi, value);
            target = current;
        }

        // dtSec: frame delta
        void setTarget(float v) { target = juce::jlimit(spec.clampLo, spec.clampHi, v); }

        void tick(float dtSec)
        {
            const float tau = juce::jmax(1.0e-4f, spec.smoothingMs / 1000.f);
            const float a = 1.0f - std::exp(-dtSec / tau);
            current += a * (target - current);
            current = juce::jlimit(spec.clampLo, spec.clampHi, current);
        }

        float get() const { return current; }
        bool isAboveThreshold() const { return current >= spec.threshold; }

        SignalSpec spec {};
        float current = 0.f;
        float target = 0.f;
    };

    struct ParticleSignals
    {
        SmoothedSignal rms;
        SmoothedSignal peak;

        juce::Point<float> cursorPosPx { 0.f, 0.f };
        bool cursorDown = false;

        void tick(float dtSec)
        {
            rms.tick(dtSec);
            peak.tick(dtSec);
        }
    };
}