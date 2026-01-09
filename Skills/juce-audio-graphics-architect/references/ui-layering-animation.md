# UI Layering and Animation Patterns

## Layered composition
- Separate background, mid-layer, and overlay components.
- Order matters: add background first, then overlays, then call `toFront()` when needed.

```cpp
addAndMakeVisible(background);
addAndMakeVisible(visualLayer);
addAndMakeVisible(overlay);
```

## Transparency layers
Use transparency layers for grouped alpha and soft shadows.

```cpp
void paint(juce::Graphics& g) override
{
    g.beginTransparencyLayer(0.85f);
    g.setColour(juce::Colours::black.withAlpha(0.25f));
    g.fillRoundedRectangle(bounds.toFloat(), 12.0f);
    g.endTransparencyLayer();
}
```

## Drop shadows
- `juce::DropShadow` and `juce::DropShadowEffect` work for simple cases.
- For higher-quality CPU shadows, prefer `melatonin_blur` (see `references/third-party-modules.md`).

## Component alpha
Use `Component::setAlpha()` for global fades or pulsing.

```cpp
overlay.setAlpha(0.6f);
```

## Animation approaches
- Use `juce_animation` if available (see `references/juce-animation.md`).
- Use `juce::ComponentAnimator` for simple fades and movement.
- Use a `juce::Timer` for custom property interpolation.

## Audio-reactive UI
- Store RMS or peak values in atomics or a lock-free FIFO.
- Map these values to UI properties (alpha, glow size, motion amplitude).
- Clamp and smooth to avoid jitter.

## Local UI references
- `references/local/juce-masking-animation.md` for masking and modulation matrix patterns.
- `references/local/juce-demo-project.md` for a full UI component layout example.
- `references/local/juce-blend-modes.md` for blend mode limitations and workarounds.
