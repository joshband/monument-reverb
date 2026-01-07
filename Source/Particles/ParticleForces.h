#pragma once

#include <JuceHeader.h>

#include "ParticleBehaviorDSL.h"
#include "ParticleSignals.h"
#include "ParticleTypes.h"

namespace vds::particles
{
    struct ForceContext
    {
        float timeSec = 0.f;
        float dtSec = 1.f / 60.f;

        juce::Rectangle<float> viewportPx;
        ParticleSignals* signals = nullptr;

        float modAudioRms = 0.f;
        float modAudioPeak = 0.f;
    };

    class IForce
    {
    public:
        virtual ~IForce() = default;
        virtual Vec2 compute(const Particle& p, const ForceContext& ctx) const = 0;
    };

    class CurlNoiseForce final : public IForce
    {
    public:
        explicit CurlNoiseForce(CurlNoiseForceSpec spec) : s(std::move(spec)) {}
        Vec2 compute(const Particle& p, const ForceContext& ctx) const override;

        CurlNoiseForceSpec s;

    private:
        static float hash2(uint32_t x, uint32_t y, uint32_t seed);
        static float noise2(float x, float y, uint32_t seed);
        static Vec2 gradNoise(float x, float y, uint32_t seed);
    };

    class CursorFieldForce final : public IForce
    {
    public:
        explicit CursorFieldForce(CursorFieldForceSpec spec) : s(std::move(spec)) {}
        Vec2 compute(const Particle& p, const ForceContext& ctx) const override;

        CursorFieldForceSpec s;
    };

    class DragForce final : public IForce
    {
    public:
        explicit DragForce(DragForceSpec spec) : s(std::move(spec)) {}
        Vec2 compute(const Particle& p, const ForceContext& ctx) const override;

        DragForceSpec s;
    };
}