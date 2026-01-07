#include "ParticleForces.h"

namespace vds::particles
{
    float CurlNoiseForce::hash2(uint32_t x, uint32_t y, uint32_t seed)
    {
        uint32_t h = seed;
        h ^= x + 0x9e3779b9u + (h << 6) + (h >> 2);
        h ^= y + 0x9e3779b9u + (h << 6) + (h >> 2);
        h ^= h << 13;
        h ^= h >> 17;
        h ^= h << 5;
        return (h & 0x00ffffffu) / float(0x01000000u);
    }

    float CurlNoiseForce::noise2(float x, float y, uint32_t seed)
    {
        const int xi = static_cast<int>(std::floor(x));
        const int yi = static_cast<int>(std::floor(y));
        const float tx = x - static_cast<float>(xi);
        const float ty = y - static_cast<float>(yi);

        const auto v00 = hash2(static_cast<uint32_t>(xi), static_cast<uint32_t>(yi), seed);
        const auto v10 = hash2(static_cast<uint32_t>(xi + 1), static_cast<uint32_t>(yi), seed);
        const auto v01 = hash2(static_cast<uint32_t>(xi), static_cast<uint32_t>(yi + 1), seed);
        const auto v11 = hash2(static_cast<uint32_t>(xi + 1), static_cast<uint32_t>(yi + 1), seed);

        const auto sx = tx * tx * (3.f - 2.f * tx);
        const auto sy = ty * ty * (3.f - 2.f * ty);

        const auto a = juce::jmap(sx, v00, v10);
        const auto b = juce::jmap(sx, v01, v11);
        return juce::jmap(sy, a, b);
    }

    Vec2 CurlNoiseForce::gradNoise(float x, float y, uint32_t seed)
    {
        constexpr float eps = 0.01f;
        const float nx1 = noise2(x + eps, y, seed);
        const float nx0 = noise2(x - eps, y, seed);
        const float ny1 = noise2(x, y + eps, seed);
        const float ny0 = noise2(x, y - eps, seed);

        const float dx = (nx1 - nx0) / (2.f * eps);
        const float dy = (ny1 - ny0) / (2.f * eps);
        return { dx, dy };
    }

    Vec2 CurlNoiseForce::compute(const Particle& p, const ForceContext& ctx) const
    {
        const float audioMod = (s.modulateBy == "audio_rms") ? ctx.modAudioRms : 1.f;
        const float strength = s.strength * (0.3f + 0.7f * audioMod);

        const float t = ctx.timeSec * s.timeScale;
        const float x = (p.position.x * s.scale) + t;
        const float y = (p.position.y * s.scale) - t;

        auto g = gradNoise(x, y, p.seed);
        Vec2 curl = { -g.y, g.x };

        curl = normalize(curl) * strength;
        return curl;
    }

    Vec2 CursorFieldForce::compute(const Particle& p, const ForceContext& ctx) const
    {
        if (ctx.signals == nullptr)
            return { 0.f, 0.f };

        const auto cursor = ctx.signals->cursorPosPx;
        Vec2 toCursor = Vec2{ cursor.x, cursor.y } - p.position;
        const float d = length(toCursor);

        const float r = juce::jmax(1.f, s.radiusPx);
        if (d >= r)
            return { 0.f, 0.f };

        const float fall = 1.0f - smoothstep(0.f, r, d);
        const float dirSign = (s.mode == CursorMode::repel) ? -1.f : 1.f;

        const float lagMix = juce::jlimit(0.f, 0.999f, s.lag);
        const float eff = s.strength * fall * (1.0f - lagMix);

        Vec2 dir = normalize(toCursor);
        return dir * (dirSign * eff);
    }

    Vec2 DragForce::compute(const Particle& p, const ForceContext& ctx) const
    {
        const float c = juce::jlimit(0.f, 0.9999f, s.coefficient);
        const float retain = std::pow(c, ctx.dtSec);
        const float k = (1.0f - retain) / juce::jmax(1.0e-6f, ctx.dtSec);

        return p.velocity * (-k);
    }
}