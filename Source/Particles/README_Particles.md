# Particle Behavior Module (behavior-only)

## What this is
- Parses Particle Behavior JSON DSL v0.1
- Runs a deterministic particle simulation suitable for UI overlays
- Audio/cursor inputs modulate behavior safely

## What this is NOT
- No rendering backend
- No shaders
- No GPU compute assumptions

## Typical usage
1) Create a ParticleSystem
2) Load a behavior JSON, parse it to ParticleBehaviorSpec
3) Feed audio features (RMS/peak) each frame/timer tick
4) Feed cursor position and mouse down state
5) Draw particles in your JUCE Component paint()

## Example sketch

```cpp
vds::particles::ParticleSystem ps;
vds::particles::ParticleBehaviorSpec spec;
juce::String err;
auto json = vds::particles::ParticleBehaviorDSL::examplePresetJson();
auto res = vds::particles::ParticleBehaviorDSL::parseFromJsonString(json, spec, err);
jassert(res.wasOk());
ps.setBehavior(spec);

void resized() override
{
  ps.setViewport(getLocalBounds().toFloat());
  ps.setEmitterPosition(getLocalBounds().getCentre().toFloat());
}

void mouseMove(const juce::MouseEvent& e) override
{
  ps.signals().cursorPosPx = e.position;
}

void timerCallback() override
{
  ps.setAudioRms(currentRms01);   // computed elsewhere
  ps.setAudioPeak(currentPeak01); // computed elsewhere
  ps.update(1.0f / 60.0f);
  repaint();
}

void paint(juce::Graphics& g) override
{
  for (auto& p : ps.getParticles())
  {
    // render as circles, sprites, or layer-aligned glows
    g.setOpacity(p.energy);
    g.fillEllipse(p.position.x - p.size, p.position.y - p.size, p.size * 2.f, p.size * 2.f);
  }
}
```

---

## What you’ll likely want next
- **JSON Schema (draft-07)** for validation + editor autocomplete
- a **behavior preset pack**: `smoke`, `dust`, `sparks`, `halo`, `cursor_trail`
- a **renderer stub** that composites layered PBR UI passes + particle overlay cleanly
- a **GPU path** (Metal/OpenGL) behind an interface, still driven by this DSL