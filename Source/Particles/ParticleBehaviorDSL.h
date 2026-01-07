#pragma once

#include <JuceHeader.h>

#include "ParticleSignals.h"

namespace vds::particles
{
    enum class ForceType { curl_noise, cursor_field, drag };

    enum class CursorMode { attract, repel };

    struct CurlNoiseForceSpec
    {
        float strength = 0.4f;
        float scale = 0.6f;
        float timeScale = 0.2f;
        juce::String modulateBy; // e.g. "audio_rms"
    };

    struct CursorFieldForceSpec
    {
        CursorMode mode = CursorMode::attract;
        float radiusPx = 140.f;
        float strength = 0.3f;
        float lag = 0.85f; // 0..1, higher = more lag/viscosity
    };

    struct DragForceSpec
    {
        float coefficient = 0.92f;
    };

    struct ForceSpec
    {
        ForceType type {};
        CurlNoiseForceSpec curl {};
        CursorFieldForceSpec cursor {};
        DragForceSpec drag {};
    };

    struct EmissionSpec
    {
        juce::String mode = "continuous";
        int ratePerSec = 40;

        struct Burst
        {
            bool enabled = true;
            juce::String trigger = "audio_peak";
            int count = 12;
            int cooldownMs = 120;
        } burst;

        struct InitialVelocity
        {
            juce::String type = "radial";
            float min = 0.02f;
            float max = 0.08f;
        } velocity;

        float initialEnergy = 1.0f;

        struct InitialSize
        {
            float min = 0.6f;
            float max = 1.2f;
        } size;
    };

    struct LifecycleSpec
    {
        float lifetimeMinMs = 1200.f;
        float lifetimeMaxMs = 2600.f;
        float energyDecayRate = 0.65f;
        float sizeDecayRate = 0.15f;
    };

    struct StabilitySpec
    {
        int maxParticles = 600;
        float maxVelocity = 2.0f;
        float forceClamp = 1.0f;

        struct Bounds
        {
            juce::String mode = "soft";
            float marginPx = 40.f;
        } bounds;
    };

    struct BindingSpec
    {
        juce::String source;  // "rms" | "peak"
        juce::String target;  // e.g. "curl_noise.strength" or "emission.burst"
        juce::String mode;    // "range" | "trigger"
        float rangeLo = 0.f;
        float rangeHi = 1.f;
    };

    struct ModulationSpec
    {
        SignalSpec rmsSpec;
        SignalSpec peakSpec;

        juce::Array<BindingSpec> bindings;
    };

    struct ParticleBehaviorSpec
    {
        juce::String version = "0.1";
        juce::String behaviorId;
        juce::String description;

        EmissionSpec emission;
        juce::Array<ForceSpec> forces;
        ModulationSpec modulation;
        LifecycleSpec lifecycle;
        StabilitySpec stability;
    };

    class ParticleBehaviorDSL
    {
    public:
        static juce::Result parseFromJsonString(const juce::String& json, ParticleBehaviorSpec& outSpec,
                                                juce::String& error);
        static juce::String examplePresetJson();
    };
}