# JUCE API Quick Reference (Audio + UI + OpenGL)

## AudioProcessorValueTreeState (APVTS)
Use APVTS for parameter definition, state serialization, and UI attachments.

```cpp
static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    using Param = juce::AudioParameterFloat;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<Param>(
        "mix",
        "Mix",
        juce::NormalisableRange<float>(0.0f, 1.0f),
        0.5f));

    return { params.begin(), params.end() };
}

juce::AudioProcessorValueTreeState apvts {
    *this,
    nullptr,
    "PARAMS",
    createParameterLayout()
};
```

Attachments in the editor:

```cpp
using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

SliderAttachment mixAttachment { apvts, "mix", mixSlider };
```

## juce::dsp::ProcessSpec
Prepare DSP objects with consistent spec values.

```cpp
juce::dsp::ProcessSpec spec;
spec.sampleRate = getSampleRate();
spec.maximumBlockSize = static_cast<juce::uint32>(getBlockSize());
spec.numChannels = static_cast<juce::uint32>(getTotalNumOutputChannels());

chain.prepare(spec);
chain.reset();
```

## juce::dsp::Reverb
Use a single reverb instance or place it in a `ProcessorChain`.

```cpp
juce::dsp::Reverb reverb;
juce::dsp::Reverb::Parameters params;
params.roomSize = 0.6f;
params.damping = 0.4f;
params.wetLevel = 0.35f;
params.dryLevel = 0.7f;
params.width = 1.0f;
reverb.setParameters(params);
```

## juce::dsp::DelayLine
Construct with a maximum delay buffer, then call `setDelay()` in samples.

```cpp
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear>
    delay { 48000 };

delay.setDelay(1200.0f);
```

## AudioVisualiserComponent
Use for quick waveform or level display on the UI thread.

```cpp
visualiser.setBufferSize(128);
visualiser.setSamplesPerBlock(16);
visualiser.setRepaintRate(60);
visualiser.pushBuffer(bufferToVisualise);
```

## Graphics::beginTransparencyLayer
Use to group layered alpha and shadow effects in `paint()`.

```cpp
g.beginTransparencyLayer(0.75f);
// Draw translucent layers here.
g.endTransparencyLayer();
```

## Component::setAlpha
Use to fade an entire component, including its children.

```cpp
component.setAlpha(0.8f);
```

## OpenGLContext and OpenGLRenderer
Attach an OpenGL context to a component and implement renderer callbacks.

```cpp
openGLContext.setRenderer(this);
openGLContext.attachTo(*this);
openGLContext.setContinuousRepainting(true);

void newOpenGLContextCreated() override {}
void renderOpenGL() override {}
void openGLContextClosing() override {}
```

## Animator and Easing (JUCE 7/8)
Use the animation module if available; otherwise fall back to `ComponentAnimator` or a `Timer`.

```cpp
// Verify the exact API for your JUCE version.
// Typical pattern: animate a value with an easing function on the message thread.
```

## Version notes
- Verify API names and module targets for your JUCE version.
- If `juce_animation` is unavailable, use `juce::ComponentAnimator` and easing math manually.

## Local header references
- See `references/juce-upstream/api/` in the external stash (see `references/external-paths.md`).
