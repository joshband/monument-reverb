#pragma once

#include <JuceHeader.h>

#include <memory>

#include "ParticleBehaviorDSL.h"
#include "ParticleForces.h"
#include "ParticleSignals.h"
#include "ParticleTypes.h"

namespace vds::particles
{
    class ParticleSystem
    {
    public:
        ParticleSystem();

        void setViewport(juce::Rectangle<float> viewportPx);
        void setBehavior(const ParticleBehaviorSpec& spec);

        ParticleSignals& signals() { return sig; }

        void setAudioRms(float v)  { sig.rms.setTarget(v); }
        void setAudioPeak(float v) { sig.peak.setTarget(v); }

        void update(float dtSec);

        const juce::Array<Particle>& getParticles() const { return particles; }

        void setEmitterPosition(Vec2 p) { emitterPos = p; }

    private:
        void rebuildForceStack();
        void applyBindings();
        void emit(float dtSec);
        void integrate(float dtSec);
        void enforceBounds(Particle& p);

        static uint32_t nextSeed(uint32_t& state);
        static float rand01(uint32_t& state);

        ParticleBehaviorSpec behavior;
        ParticleSignals sig;

        juce::Array<std::unique_ptr<IForce>> forceStack;
        juce::Array<Particle> particles;

        juce::Rectangle<float> viewport { 0.f, 0.f, 400.f, 300.f };
        Vec2 emitterPos { 200.f, 150.f };

        float timeSec = 0.f;

        float emitAccumulator = 0.f;
        int burstCooldownMsRemaining = 0;
        bool lastPeakAbove = false;

        float modCurlStrengthOverride = -1.f;

        uint32_t rngState = 0x12345678u;
    };
}