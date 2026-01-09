Perfect. Here’s a **clean, production-oriented expansion** covering **1, 3, 4, and 5**, designed so you can implement incrementally without painting yourself into a corner.

I’ll be direct and opinionated where it matters.

---

# 1. JUCE MASKING + ASYMMETRIC SILHOUETTE

*(Rectangular window, non-rectangular presence)*

You **must** keep the plugin window rectangular for host compatibility. The illusion comes from **masking, shadows, and negative space**, not window shape.

## Core technique

* Black (or near-black) background fills the entire editor
* The “Monument body” is a **masked shape** floating above it
* Everything outside the mask is visual void

### Implementation strategy

* Render the UI into a **Path-based mask**
* Draw drop shadow + ambient occlusion outside the shape
* Clip all panel rendering to the shape

### `MonumentBodyComponent.h`

```cpp
class MonumentBodyComponent : public juce::Component
{
public:
    void paint (juce::Graphics&) override;

private:
    juce::Path bodyMask;
};
```

### `MonumentBodyComponent.cpp`

```cpp
void MonumentBodyComponent::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Build asymmetrical silhouette
    bodyMask.clear();
    bodyMask.startNewSubPath(20.f, 10.f);
    bodyMask.quadraticTo(bounds.getWidth() * 0.6f, -10.f,
                          bounds.getWidth() - 20.f, 30.f);
    bodyMask.lineTo(bounds.getWidth() - 10.f, bounds.getHeight() - 40.f);
    bodyMask.quadraticTo(bounds.getWidth() * 0.8f, bounds.getHeight() + 10.f,
                          30.f, bounds.getHeight() - 20.f);
    bodyMask.closeSubPath();

    // Shadow
    juce::DropShadow shadow(juce::Colours::black.withAlpha(0.6f),
                            30, {0, 8});
    shadow.drawForPath(g, bodyMask);

    // Clip to body
    g.reduceClipRegion(bodyMask);

    // Body fill (stone texture later)
    g.setColour(juce::Colour(0xff0f0f10));
    g.fillPath(bodyMask);
}
```

**Key rule:**
All panels, knobs, and headers are children of this component. They never draw outside the mask.

This is how you get the **sculptural, altar-like presence** without breaking hosts.

---

# 3. ANIMATED MODULATION MATRIX

*(Replaces play/pause entirely)*

The modulation nexus should feel **alive, not interactive transport UI**.

## Visual language

* Nodes = destinations
* Vertical columns = sources
* Lines = modulation strength
* Motion = low-frequency breathing, not twitchy EDM lines

### Data model

```cpp
struct ModConnection
{
    int source;
    int dest;
    float depth;   // 0..1
};
```

### Rendering

```cpp
void ModulationMatrixComponent::paint(juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat();

    constexpr int sources = 6;
    constexpr int dests   = 8;

    juce::Random rng(42);

    for (int s = 0; s < sources; ++s)
    {
        for (int d = 0; d < dests; ++d)
        {
            float x1 = juce::jmap((float)s, 0.f, sources - 1.f,
                                  r.getX() + 20.f, r.getRight() - 20.f);
            float y2 = juce::jmap((float)d, 0.f, dests - 1.f,
                                  r.getY() + 10.f, r.getBottom() - 10.f);

            float alpha = 0.15f + 0.15f * std::sin(juce::Time::getMillisecondCounterHiRes() * 0.0004 + s + d);

            g.setColour(juce::Colours::deepskyblue.withAlpha(alpha));
            g.drawLine(x1, r.getCentreY(), x1, y2, 1.2f);
        }
    }
}
```

### Animation

* Use `Timer` at **30–45 Hz**, not 60
* Avoid sharp motion
* Think: breathing mineral lattice

This instantly reads as *“modulation topology”* without explanation.

---

# 4. SLIDER → KNOB MIGRATION PLAN

*(Without breaking automation or presets)*

This is where people screw up. Don’t.

## Rule

**Parameters never change. Controls do.**

### Step-by-step

1. **Keep all parameters exactly as-is**

   ```cpp
   AudioParameterFloat* size;
   AudioParameterFloat* density;
   ```

2. **Replace Slider UI only**

   ```cpp
   auto sizeKnob = std::make_unique<StoneKnob>("size");
   sizeKnob->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
   sizeKnob->setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
   ```

3. **Attach normally**

   ```cpp
   sizeAttachment = std::make_unique<SliderAttachment>(
       apvts, "size", *sizeKnob);
   ```

4. **Optional hybrid**

   * Keep timeline sliders in Temporal Vault
   * Add *ghosted rotary overlays* that modulate them

### Result

* Automation stays intact
* Presets load safely
* You can A/B UI styles without touching DSP

---

# 5. SWIFTUI AUv3 PARALLEL UI

*(Same assets, same identity)*

This is absolutely doable — and smart.

## Shared asset philosophy

* One **authoritative asset pack**
* Export to:

  * JUCE BinaryData
  * iOS asset catalog

### SwiftUI knob (Canvas-based, performant)

```swift
struct StoneKnobView: View {
    let value: Double
    let stone: Image
    let crystal: Image
    let core: Image

    var body: some View {
        Canvas { ctx, size in
            let rect = CGRect(origin: .zero, size: size)

            ctx.draw(stone, in: rect)
            ctx.draw(crystal, in: rect)
            ctx.draw(core, in: rect)

            let angle = Angle.degrees(-135 + 270 * value)
            ctx.rotate(by: angle, around: rect.center)

            ctx.stroke(
                Path { p in
                    p.move(to: CGPoint(x: rect.midX, y: rect.minY + 6))
                    p.addLine(to: CGPoint(x: rect.midX, y: rect.midY))
                },
                with: .color(.white.opacity(0.8)),
                lineWidth: 2
            )
        }
    }
}
```

### Why Canvas

* GPU-accelerated
* Layered composition
* Matches JUCE mental model

### AUv3 structure

```
SharedAssets/
iOS/
macOS/
JUCE/
```

Same knobs. Same panels. Same philosophy.

---

# Where You Are Now (Reality Check)

You now have:

* A **sculptural, non-rectangular illusion** that hosts accept
* A **living modulation matrix**, not UI chrome
* A **safe migration path** from sliders to knobs
* A **future-proof AUv3 UI** that doesn’t fork your design language

This is not a mockup anymore.
This is an interface architecture.

---

## Next logical steps

If you want, the *correct* next moves are:

* Deterministic **knob taxonomy** (which parameters get which stone)
* **Asymmetric hit-testing** rules
* **Performance audit** (JUCE repaint cost vs animation)
* **Theme system** (Ancient / Void / Cathedral)

Say which one you want to lock next, and we’ll keep building this like it deserves.
