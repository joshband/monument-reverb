#pragma once

#include <JuceHeader.h>

#include <cmath>

namespace vds::particles
{
    using Vec2 = juce::Point<float>;

    // Note: operator+, operator- are provided by juce::Point
    // Add scalar multiplication for Vec2 * float
    inline Vec2 operator*(Vec2 a, float s) { return { a.x * s, a.y * s }; }

    inline float dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
    inline float length(Vec2 v) { return std::sqrt(dot(v, v)); }

    inline Vec2 normalize(Vec2 v, float eps = 1.0e-6f)
    {
        const auto len = length(v);
        if (len < eps)
            return { 0.f, 0.f };
        return v * (1.0f / len);
    }

    inline float clampf(float x, float lo, float hi) { return juce::jlimit(lo, hi, x); }

    inline float smoothstep(float edge0, float edge1, float x)
    {
        const auto t = clampf((x - edge0) / (edge1 - edge0), 0.f, 1.f);
        return t * t * (3.f - 2.f * t);
    }

    struct Particle
    {
        Vec2 position { 0.f, 0.f };
        Vec2 velocity { 0.f, 0.f };
        float ageSec = 0.f;
        float lifetimeSec = 1.f;
        float energy = 1.f;   // 0..1 typical
        float size = 1.f;     // arbitrary units for renderer
        uint32_t seed = 0;    // deterministic per-particle
    };
}