#include "ParticleSystem.h"

namespace vds::particles
{
    ParticleSystem::ParticleSystem()
    {
        sig.rms.setSpec({});
        sig.peak.setSpec({});
    }

    void ParticleSystem::setViewport(juce::Rectangle<float> viewportPx)
    {
        viewport = viewportPx;
    }

    void ParticleSystem::setBehavior(const ParticleBehaviorSpec& spec)
    {
        behavior = spec;

        sig.rms.setSpec(behavior.modulation.rmsSpec);
        sig.peak.setSpec(behavior.modulation.peakSpec);

        particles.clearQuick();
        particles.ensureStorageAllocated(behavior.stability.maxParticles);

        emitAccumulator = 0.f;
        burstCooldownMsRemaining = 0;
        lastPeakAbove = false;
        modCurlStrengthOverride = -1.f;

        rebuildForceStack();
    }

    void ParticleSystem::rebuildForceStack()
    {
        forceStack.clearQuick();

        for (auto& fs : behavior.forces)
        {
            switch (fs.type)
            {
                case ForceType::curl_noise:
                    forceStack.add(std::make_unique<CurlNoiseForce>(fs.curl));
                    break;
                case ForceType::cursor_field:
                    forceStack.add(std::make_unique<CursorFieldForce>(fs.cursor));
                    break;
                case ForceType::drag:
                    forceStack.add(std::make_unique<DragForce>(fs.drag));
                    break;
                default:
                    break;
            }
        }
    }

    void ParticleSystem::update(float dtSec)
    {
        dtSec = juce::jlimit(1.0e-4f, 0.1f, dtSec);
        timeSec += dtSec;

        sig.tick(dtSec);

        applyBindings();
        emit(dtSec);
        integrate(dtSec);
    }

    void ParticleSystem::applyBindings()
    {
        modCurlStrengthOverride = -1.f;

        const float rms = sig.rms.get();
        const float peak = sig.peak.get();
        const bool peakAbove = sig.peak.isAboveThreshold();

        for (auto& b : behavior.modulation.bindings)
        {
            if (b.mode == "range")
            {
                float sourceValue = 0.f;
                if (b.source == "rms")
                    sourceValue = rms;
                else if (b.source == "peak")
                    sourceValue = peak;

                const float mapped = juce::jmap(juce::jlimit(0.f, 1.f, sourceValue), b.rangeLo, b.rangeHi);

                if (b.target == "curl_noise.strength")
                    modCurlStrengthOverride = mapped;
            }
            else if (b.mode == "trigger")
            {
                if (b.target == "emission.burst" && b.source == "peak")
                {
                    const bool rising = (peakAbove && !lastPeakAbove);
                    if (rising && behavior.emission.burst.enabled)
                    {
                    }
                }
            }
        }

        lastPeakAbove = peakAbove;
    }

    uint32_t ParticleSystem::nextSeed(uint32_t& state)
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }

    float ParticleSystem::rand01(uint32_t& state)
    {
        return (nextSeed(state) & 0x00ffffffu) / float(0x01000000u);
    }

    void ParticleSystem::emit(float dtSec)
    {
        burstCooldownMsRemaining = juce::jmax(0, burstCooldownMsRemaining
                                                 - static_cast<int>(std::lround(dtSec * 1000.0f)));

        auto spawnOne = [&]()
        {
            if (particles.size() >= behavior.stability.maxParticles)
                return;

            Particle p;
            p.seed = nextSeed(rngState);
            p.position = emitterPos;

            const float ltMs = juce::jmap(rand01(rngState), behavior.lifecycle.lifetimeMinMs,
                                          behavior.lifecycle.lifetimeMaxMs);
            p.lifetimeSec = juce::jmax(0.1f, ltMs / 1000.f);
            p.ageSec = 0.f;

            Vec2 dir { rand01(rngState) * 2.f - 1.f, rand01(rngState) * 2.f - 1.f };
            dir = normalize(dir);
            const float speed = juce::jmap(rand01(rngState), behavior.emission.velocity.min,
                                           behavior.emission.velocity.max);
            p.velocity = dir * speed;

            p.energy = behavior.emission.initialEnergy;
            p.size = juce::jmap(rand01(rngState), behavior.emission.size.min,
                                behavior.emission.size.max);

            particles.add(p);
        };

        if (behavior.emission.mode == "continuous" && behavior.emission.ratePerSec > 0)
        {
            emitAccumulator += dtSec * static_cast<float>(behavior.emission.ratePerSec);
            while (emitAccumulator >= 1.f)
            {
                emitAccumulator -= 1.f;
                spawnOne();
            }
        }

        const bool peakAbove = sig.peak.isAboveThreshold();
        const bool rising = peakAbove && !lastPeakAbove;

        if (behavior.emission.burst.enabled && rising && burstCooldownMsRemaining == 0)
        {
            for (int i = 0; i < behavior.emission.burst.count; ++i)
                spawnOne();

            burstCooldownMsRemaining = behavior.emission.burst.cooldownMs;
        }
    }

    void ParticleSystem::integrate(float dtSec)
    {
        ForceContext ctx;
        ctx.timeSec = timeSec;
        ctx.dtSec = dtSec;
        ctx.viewportPx = viewport;
        ctx.signals = &sig;
        ctx.modAudioRms = sig.rms.get();
        ctx.modAudioPeak = sig.peak.get();

        for (int i = particles.size(); --i >= 0;)
        {
            auto& p = particles.getReference(i);

            p.ageSec += dtSec;

            p.energy *= std::exp(-behavior.lifecycle.energyDecayRate * dtSec);
            p.energy = juce::jlimit(0.f, 1.f, p.energy);

            p.size = juce::jmax(0.f, p.size - behavior.lifecycle.sizeDecayRate * dtSec);

            Vec2 force { 0.f, 0.f };

            for (auto& f : forceStack)
            {
                auto contrib = f->compute(p, ctx);

                const float contribLen = length(contrib);
                if (contribLen > behavior.stability.forceClamp)
                    contrib = normalize(contrib) * behavior.stability.forceClamp;

                if (modCurlStrengthOverride >= 0.f)
                {
                    if (auto* cn = dynamic_cast<CurlNoiseForce*>(f.get()))
                    {
                        const float base = juce::jmax(1.0e-4f, cn->s.strength);
                        const float scale = modCurlStrengthOverride / base;
                        contrib = contrib * scale;
                    }
                }

                force = force + contrib;
            }

            const float fLen = length(force);
            if (fLen > behavior.stability.forceClamp)
                force = normalize(force) * behavior.stability.forceClamp;

            p.velocity = p.velocity + (force * dtSec);

            const float vLen = length(p.velocity);
            if (vLen > behavior.stability.maxVelocity)
                p.velocity = normalize(p.velocity) * behavior.stability.maxVelocity;

            p.position = p.position + (p.velocity * dtSec);

            enforceBounds(p);

            const bool dead = (p.ageSec >= p.lifetimeSec)
                              || (p.energy <= 0.001f)
                              || (p.size <= 0.001f);
            if (dead)
                particles.remove(i);
        }
    }

    void ParticleSystem::enforceBounds(Particle& p)
    {
        if (behavior.stability.bounds.mode != "soft")
            return;

        const auto rect = viewport.expanded(behavior.stability.bounds.marginPx);

        float push = 0.0f;
        Vec2 dir { 0.f, 0.f };

        if (p.position.x < rect.getX())
        {
            push = rect.getX() - p.position.x;
            dir = dir + Vec2{ 1.f, 0.f };
        }
        if (p.position.x > rect.getRight())
        {
            push = p.position.x - rect.getRight();
            dir = dir + Vec2{ -1.f, 0.f };
        }
        if (p.position.y < rect.getY())
        {
            push = juce::jmax(push, rect.getY() - p.position.y);
            dir = dir + Vec2{ 0.f, 1.f };
        }
        if (p.position.y > rect.getBottom())
        {
            push = juce::jmax(push, p.position.y - rect.getBottom());
            dir = dir + Vec2{ 0.f, -1.f };
        }

        if (push > 0.f)
        {
            dir = normalize(dir);
            const float k = 0.15f;
            p.velocity = p.velocity + dir * (k * push);
        }
    }
}