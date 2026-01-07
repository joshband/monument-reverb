#include "ParticleBehaviorDSL.h"

namespace vds::particles
{
    static juce::var getObjProp(const juce::var& obj, const char* name)
    {
        if (auto* o = obj.getDynamicObject())
            return o->getProperty(name);
        return {};
    }

    static float getFloat(const juce::var& v, float fallback)
    {
        if (v.isDouble() || v.isInt())
            return static_cast<float>(v);
        return fallback;
    }

    static int getInt(const juce::var& v, int fallback)
    {
        if (v.isInt())
            return static_cast<int>(v);
        if (v.isDouble())
            return static_cast<int>(std::lround(static_cast<double>(v)));
        return fallback;
    }

    static bool getBool(const juce::var& v, bool fallback)
    {
        if (v.isBool())
            return static_cast<bool>(v);
        return fallback;
    }

    static juce::String getString(const juce::var& v, const juce::String& fallback = {})
    {
        if (v.isString())
            return v.toString();
        return fallback;
    }

    static SignalSpec parseSignalSpec(const juce::var& node, const SignalSpec& defaults)
    {
        SignalSpec s = defaults;
        if (auto* o = node.getDynamicObject())
        {
            s.smoothingMs = getFloat(o->getProperty("smoothing_ms"), s.smoothingMs);
            if (auto clampV = o->getProperty("clamp"); clampV.isArray())
            {
                auto* arr = clampV.getArray();
                if (arr->size() >= 2)
                {
                    s.clampLo = getFloat(arr->getReference(0), s.clampLo);
                    s.clampHi = getFloat(arr->getReference(1), s.clampHi);
                }
            }
            s.threshold = getFloat(o->getProperty("threshold"), s.threshold);
        }
        return s;
    }

    static CursorMode parseCursorMode(const juce::String& s)
    {
        if (s == "repel")
            return CursorMode::repel;
        return CursorMode::attract;
    }

    static ForceType parseForceType(const juce::String& s)
    {
        if (s == "cursor_field")
            return ForceType::cursor_field;
        if (s == "drag")
            return ForceType::drag;
        return ForceType::curl_noise;
    }

    juce::Result ParticleBehaviorDSL::parseFromJsonString(const juce::String& json,
                                                          ParticleBehaviorSpec& outSpec,
                                                          juce::String& error)
    {
        error.clear();
        juce::var v;
        auto result = juce::JSON::parse(json, v);
        if (result.failed())
        {
            error = result.getErrorMessage();
            return result;
        }

        auto* root = v.getDynamicObject();
        if (root == nullptr)
            return juce::Result::fail("Root must be a JSON object");

        ParticleBehaviorSpec spec;

        spec.version = getString(root->getProperty("version"), "0.1");
        spec.behaviorId = getString(root->getProperty("behavior_id"));
        spec.description = getString(root->getProperty("description"));

        if (auto e = root->getProperty("emission"); e.isObject())
        {
            auto* eo = e.getDynamicObject();
            spec.emission.mode = getString(eo->getProperty("mode"), spec.emission.mode);
            spec.emission.ratePerSec = getInt(eo->getProperty("rate"), spec.emission.ratePerSec);

            if (auto b = eo->getProperty("burst"); b.isObject())
            {
                auto* bo = b.getDynamicObject();
                spec.emission.burst.enabled = getBool(bo->getProperty("enabled"), spec.emission.burst.enabled);
                spec.emission.burst.trigger = getString(bo->getProperty("trigger"), spec.emission.burst.trigger);
                spec.emission.burst.count = getInt(bo->getProperty("count"), spec.emission.burst.count);
                spec.emission.burst.cooldownMs = getInt(bo->getProperty("cooldown_ms"),
                                                        spec.emission.burst.cooldownMs);
            }

            if (auto init = eo->getProperty("initial_state"); init.isObject())
            {
                auto* io = init.getDynamicObject();

                if (auto vel = io->getProperty("velocity"); vel.isObject())
                {
                    auto* vo = vel.getDynamicObject();
                    spec.emission.velocity.type = getString(vo->getProperty("type"), spec.emission.velocity.type);
                    spec.emission.velocity.min = getFloat(vo->getProperty("min"), spec.emission.velocity.min);
                    spec.emission.velocity.max = getFloat(vo->getProperty("max"), spec.emission.velocity.max);
                }

                spec.emission.initialEnergy = getFloat(io->getProperty("energy"), spec.emission.initialEnergy);

                if (auto sz = io->getProperty("size"); sz.isObject())
                {
                    auto* so = sz.getDynamicObject();
                    spec.emission.size.min = getFloat(so->getProperty("min"), spec.emission.size.min);
                    spec.emission.size.max = getFloat(so->getProperty("max"), spec.emission.size.max);
                }
            }
        }

        if (auto f = root->getProperty("forces"); f.isArray())
        {
            for (auto& item : *f.getArray())
            {
                if (!item.isObject())
                    continue;
                auto* fo = item.getDynamicObject();

                ForceSpec fs;
                fs.type = parseForceType(getString(fo->getProperty("type")));

                if (fs.type == ForceType::curl_noise)
                {
                    fs.curl.strength = getFloat(fo->getProperty("strength"), fs.curl.strength);
                    fs.curl.scale = getFloat(fo->getProperty("scale"), fs.curl.scale);
                    fs.curl.timeScale = getFloat(fo->getProperty("time_scale"), fs.curl.timeScale);
                    fs.curl.modulateBy = getString(fo->getProperty("modulate_by"));
                }
                else if (fs.type == ForceType::cursor_field)
                {
                    fs.cursor.mode = parseCursorMode(getString(fo->getProperty("mode"), "attract"));
                    fs.cursor.radiusPx = getFloat(fo->getProperty("radius"), fs.cursor.radiusPx);
                    fs.cursor.strength = getFloat(fo->getProperty("strength"), fs.cursor.strength);
                    fs.cursor.lag = getFloat(fo->getProperty("lag"), fs.cursor.lag);
                }
                else if (fs.type == ForceType::drag)
                {
                    fs.drag.coefficient = getFloat(fo->getProperty("coefficient"), fs.drag.coefficient);
                }

                spec.forces.add(fs);
            }
        }

        if (auto m = root->getProperty("modulation"); m.isObject())
        {
            auto* mo = m.getDynamicObject();

            if (auto ai = mo->getProperty("audio_inputs"); ai.isObject())
            {
                auto* aio = ai.getDynamicObject();
                spec.modulation.rmsSpec = parseSignalSpec(aio->getProperty("rms"), spec.modulation.rmsSpec);
                spec.modulation.peakSpec = parseSignalSpec(aio->getProperty("peak"), spec.modulation.peakSpec);
            }

            if (auto binds = mo->getProperty("bindings"); binds.isArray())
            {
                for (auto& b : *binds.getArray())
                {
                    if (!b.isObject())
                        continue;
                    auto* bo = b.getDynamicObject();

                    BindingSpec bs;
                    bs.source = getString(bo->getProperty("source"));
                    bs.target = getString(bo->getProperty("target"));
                    bs.mode = getString(bo->getProperty("mode"), "range");

                    if (auto r = bo->getProperty("range"); r.isArray())
                    {
                        auto* arr = r.getArray();
                        if (arr->size() >= 2)
                        {
                            bs.rangeLo = getFloat(arr->getReference(0), bs.rangeLo);
                            bs.rangeHi = getFloat(arr->getReference(1), bs.rangeHi);
                        }
                    }

                    spec.modulation.bindings.add(bs);
                }
            }
        }

        if (auto l = root->getProperty("lifecycle"); l.isObject())
        {
            auto* lo = l.getDynamicObject();

            if (auto lt = lo->getProperty("lifetime_ms"); lt.isObject())
            {
                auto* lto = lt.getDynamicObject();
                spec.lifecycle.lifetimeMinMs = getFloat(lto->getProperty("min"), spec.lifecycle.lifetimeMinMs);
                spec.lifecycle.lifetimeMaxMs = getFloat(lto->getProperty("max"), spec.lifecycle.lifetimeMaxMs);
            }

            if (auto ed = lo->getProperty("energy_decay"); ed.isObject())
            {
                auto* edo = ed.getDynamicObject();
                spec.lifecycle.energyDecayRate = getFloat(edo->getProperty("rate"), spec.lifecycle.energyDecayRate);
            }

            if (auto sd = lo->getProperty("size_decay"); sd.isObject())
            {
                auto* sdo = sd.getDynamicObject();
                spec.lifecycle.sizeDecayRate = getFloat(sdo->getProperty("rate"), spec.lifecycle.sizeDecayRate);
            }
        }

        if (auto s = root->getProperty("stability"); s.isObject())
        {
            auto* so = s.getDynamicObject();
            spec.stability.maxParticles = getInt(so->getProperty("max_particles"), spec.stability.maxParticles);
            spec.stability.maxVelocity = getFloat(so->getProperty("max_velocity"), spec.stability.maxVelocity);
            spec.stability.forceClamp = getFloat(so->getProperty("force_clamp"), spec.stability.forceClamp);

            if (auto b = so->getProperty("bounds"); b.isObject())
            {
                auto* bo = b.getDynamicObject();
                spec.stability.bounds.mode = getString(bo->getProperty("mode"), spec.stability.bounds.mode);
                spec.stability.bounds.marginPx = getFloat(bo->getProperty("margin"), spec.stability.bounds.marginPx);
            }
        }

        outSpec = spec;
        return juce::Result::ok();
    }

    juce::String ParticleBehaviorDSL::examplePresetJson()
    {
        return R"json(
{
  "version": "0.1",
  "behavior_id": "smoke_glow_idle",
  "description": "Soft smoke-like field with audio-reactive energy",
  "emission": {
    "mode": "continuous",
    "rate": 40,
    "burst": { "enabled": true, "trigger": "audio_peak", "count": 12, "cooldown_ms": 120 },
    "initial_state": {
      "velocity": { "type": "radial", "min": 0.02, "max": 0.08 },
      "energy": 1.0,
      "size": { "min": 0.6, "max": 1.2 }
    }
  },
  "forces": [
    { "type": "curl_noise", "strength": 0.4, "scale": 0.6, "time_scale": 0.2, "modulate_by": "audio_rms" },
    { "type": "cursor_field", "mode": "attract", "radius": 140, "falloff": "smoothstep", "strength": 0.3, "lag": 0.85 },
    { "type": "drag", "coefficient": 0.92 }
  ],
  "modulation": {
    "audio_inputs": {
      "rms": { "smoothing_ms": 60, "clamp": [0.0, 1.0] },
      "peak": { "smoothing_ms": 20, "threshold": 0.7 }
    },
    "bindings": [
      { "source": "rms", "target": "curl_noise.strength", "mode": "range", "range": [0.2, 0.7] },
      { "source": "peak", "target": "emission.burst", "mode": "trigger" }
    ]
  },
  "lifecycle": {
    "lifetime_ms": { "min": 1200, "max": 2600 },
    "energy_decay": { "type": "exponential", "rate": 0.65 },
    "size_decay": { "type": "linear", "rate": 0.15 }
  },
  "stability": {
    "max_particles": 600,
    "max_velocity": 2.0,
    "force_clamp": 1.0,
    "bounds": { "mode": "soft", "margin": 40 }
  }
}
)json";
    }
}