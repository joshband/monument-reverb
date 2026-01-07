# React-JUCE Core Knowledge

You are using React-JUCE as a declarative UI layer on top of JUCE.

## Hard Boundaries

- React code NEVER touches DSP
- React code NEVER runs on the audio thread
- React components express structure and intent, not heavy rendering

## Division of Responsibility

| React-JUCE | JUCE |
|------------|------|
| Component hierarchy | Drawing |
| Layout intent | Image compositing |
| UI state | Performance-critical rendering |
| Parameter binding (via bridges) | Hit testing |

## The Rule

```
React-JUCE owns structure and state flow.
JUCE owns rendering, assets, and performance.
```

## Correct Architecture

```
React-JUCE (structure)
└── <Knob value={v} state={s} />

JUCE Component
└── LayeredRGBAKnobView
    ├── loads component.manifest.json
    ├── composites RGBA layers
    ├── rotates indicator
    └── handles hit-testing
```

## Rules

- All DSP parameters flow through APVTS or AUParameterTree
- React state mirrors parameter state, not vice versa
- No dynamic allocation in paint paths
- No timers or polling for parameter sync

## When Generating React-JUCE Code

1. Use minimal component trees
2. Prefer controlled components
3. Explicitly document parameter bindings
4. Delegate all rendering to JUCE

## Correct Pattern

```jsx
// React side - structure only, NO rendering
export function Knob({ paramId, value, state }) {
  return (
    <juce-component
      type="LayeredRGBAKnob"
      paramId={paramId}
      value={value}
      state={state}
    />
  );
}
```

```cpp
// JUCE side - all rendering
class LayeredRGBAKnobComponent : public juce::Component {
public:
    void setNormalizedValue(float v) { value = v; repaint(); }
    void setState(ComponentState s) { state = s; repaint(); }
    
    void paint(juce::Graphics& g) override {
        renderer.render(g, getLocalBounds(), value, state);
    }
};
```

## Anti-Patterns

```jsx
// WRONG: Drawing in React
function Knob({ value }) {
  return (
    <canvas onDraw={(ctx) => {
      // NO! Never render photoreal assets in React
      ctx.drawImage(knobImage, ...);
    }} />
  );
}

// WRONG: Reimplementing controls in JS
function Knob({ value }) {
  return <div style={{ transform: `rotate(${value * 270}deg)` }}>...</div>;
}

// WRONG: Storing DSP objects
function Panel() {
  const [processor] = useState(new AudioProcessor()); // NO!
}
```

## React-JUCE is NOT

- A DSP solution
- A layout engine for photoreal rendering
- Safe to touch the audio thread
- A replacement for JUCE's rendering constraints

## React-JUCE IS

- A UI orchestration layer
- A declarative scene graph feeding JUCE
- Fast iteration on layout and hierarchy
- Clean separation from DSP and rendering
