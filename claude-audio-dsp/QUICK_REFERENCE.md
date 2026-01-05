# Audio DSP Plugin - Quick Reference

## Slash Commands

```
/gen-plugin <name> [--synth] [--midi] [--ui]
    Generate complete VST3/AU/AUv3 plugin project
    Example: /gen-plugin StereoDelay --ui

/gen-dsp <algorithm>
    Generate DSP algorithm with math + optimized C++
    Examples: /gen-dsp biquad-lowpass
              /gen-dsp compressor
              /gen-dsp reverb-schroeder

/bench-dsp <class>
    Generate CPU benchmarking harness
    Example: /bench-dsp MyProcessor

/gen-tests <class>
    Generate audio test suite (impulse, sweep, null)
    Example: /gen-tests MyProcessor

/gen-rgba-component-pack <type>
    Generate layered photorealistic UI component
    Types: knob, toggle, slider
    Example: /gen-rgba-component-pack knob

/gen-golden-tests <componentId>
    Generate golden image render tests
    Example: /gen-golden-tests knob_industrial_01

/review-dsp
    Review current file for real-time safety
```

## Agents (Invoke Explicitly)

```
"Use the DSP Expert agent to design a ladder filter"
"Use the JUCE VST3/AU agent to scaffold a delay plugin"
"Use the DSP Reviewer agent to audit this code"
"Use the SIMD Optimizer agent to vectorize this loop"
"Use the SwiftUI AUv3 UI agent to create a parameter bridge"
"Use the Photoreal RGBA agent to spec a knob component"
"Use the React-JUCE UI agent to create a declarative layout"
```

## React-JUCE Architecture

```
React-JUCE (structure)     →  JUCE (rendering)
<Knob paramId="cutoff" />  →  LayeredRGBAKnob.paint()

React decides WHAT exists
JUCE decides HOW it looks
```

## Real-Time Rules (Always Enforced)

❌ NEVER in processBlock:
- new / delete / malloc
- std::vector::push_back
- std::mutex / locks
- File I/O / logging
- Exceptions

✅ ALWAYS:
- Pre-allocate in prepareToPlay()
- Use SmoothedValue for params
- Use ScopedNoDenormals
- Access params via APVTS atomics

## Common Patterns

```cpp
// Parameter access (audio thread)
float gain = apvts.getRawParameterValue("gain")->load();

// Smoothed parameter
gainSmoothed.setTargetValue(gain);
float smoothed = gainSmoothed.getNextValue();

// Denormal protection
juce::ScopedNoDenormals noDenormals;

// SIMD processing
juce::dsp::AudioBlock<float> block(buffer);
juce::dsp::ProcessContextReplacing<float> ctx(block);
filter.process(ctx);
```

## AUv3 Threading

| Context | Thread | Rules |
|---------|--------|-------|
| renderBlock | Audio (RT) | No ObjC, no alloc, no locks |
| Parameter observer | Arbitrary | Dispatch UI to main |
| allocateRenderResources | Main | Allocations OK |
| UI methods | Main | Normal Cocoa/SwiftUI |

## Asset Pipeline

```
Blender/Midjourney → Layered PNGs → component.manifest.json
    ↓
python tools/validate_component_pack.py Pack/
    ↓
SwiftUI / JUCE / Web compositing
```

## File Locations

| Platform | VST3 | AU |
|----------|------|-----|
| macOS | ~/Library/Audio/Plug-Ins/VST3 | ~/Library/Audio/Plug-Ins/Components |
| Windows | %COMMONPROGRAMFILES%\VST3 | N/A |
| Linux | ~/.vst3 | N/A |
