---
name: juce-audio-graphics-architect
description: Advanced JUCE audio DSP, real-time analysis, and interactive graphics for plugin development. Use when building AU/VST3 plugins, adding DSP chains (reverb/delay/physical modeling), implementing FFT or audio-reactive UI, integrating OpenGL/shader visuals or particle systems, or designing layered, animated JUCE interfaces with transparency and shadows. Prefer established third-party JUCE modules or GitHub libraries when they reduce custom code, with license and compatibility checks.
---

# JUCE Audio Graphics Architect

## Overview
Plan and implement JUCE plugins that combine real-time DSP with interactive visuals and layered UI, while keeping the audio thread realtime-safe.

## Core Capabilities
1. Plan plugin architecture, data flow, and thread boundaries.
2. Implement DSP chains with APVTS parameters and smoothing.
3. Build FFT and metering pipelines for audio-reactive UI.
4. Create OpenGL or shader-driven visuals and particle systems.
5. Design layered UI with transparency, shadows, and animation.
6. Integrate third-party modules when they reduce custom work.

## Third-Party Usage
- Prefer proven libraries and JUCE modules over custom re-implementation.
- Verify license, platform support, and JUCE version compatibility.
- Integrate via CMake (submodule, FetchContent, or CPM) and document the steps.

## Workflow Decision Tree
- For new plugin scaffolding: start with `assets/templates/cmake-plugin/` and `assets/templates/Source/`, then follow `references/dsp-recipes.md`.
- For DSP chain or parameter wiring: use `references/dsp-recipes.md` and `references/juce-api-quickref.md`.
- For FFT or spectrum analysis: use `references/dsp-recipes.md` and `references/juce-tutorials-map.md`.
- For OpenGL or shader visuals: use `references/opengl-juce-visuals.md` and `assets/shaders/`.
- For layered UI or animation: use `references/ui-layering-animation.md` and `references/juce-animation.md`.
- For third-party modules: use `references/third-party-modules.md` and `references/third-party/`.
- For real-world DSP ideas: use `references/skill_package/`.
- For blend-mode and layered UI research: use `references/local/`.

## Execution Checklist
- Confirm plugin format(s), target OS, and minimum JUCE version.
- Confirm sample rate, channel layout, and buffer size assumptions.
- Confirm CPU/GPU budget and whether OpenGL is acceptable.
- Define a realtime-safe data bridge from audio thread to UI (lock-free FIFO or atomics).
- Keep DSP allocations out of `processBlock` and avoid locks on the audio thread.
- Provide a minimal validation plan (compile steps, smoke test, or visual check).

## Output Expectations
- Provide code with clear separation between audio thread and UI thread responsibilities.
- Include parameter layout and attachments when UI controls are requested.
- Include fallbacks for hosts without OpenGL or when OpenGL is disabled.
- Note JUCE version-specific APIs that require verification.

## Resources
- `references/juce-tutorials-map.md`: topic map and search keywords for official JUCE docs.
- `references/juce-api-quickref.md`: quick reference for key JUCE classes used by this skill.
- `references/juce-animation.md`: JUCE animation module notes and easing patterns.
- `references/dsp-recipes.md`: DSP chain, parameter, FFT, and audio-to-UI patterns.
- `references/opengl-juce-visuals.md`: OpenGL renderer setup and shader pipeline patterns.
- `references/ui-layering-animation.md`: transparency, shadows, and layered UI composition.
- `references/third-party-modules.md`: integration notes for foleys_gui_magic, JIVE, melatonin_blur, and optional GPU UI.
- `references/juce-upstream/examples/`: JUCE example sources (FFT, DSP, OpenGL, LookAndFeel, animation).
- `references/juce-upstream/api/`: JUCE headers for APVTS, Reverb, DelayLine, AudioVisualiserComponent, Graphics, Component, OpenGL.
- `references/juce-upstream/animation/`: JUCE animation headers (Animator, Easing, ValueAnimatorBuilder).
- `references/third-party/`: README docs for foleys_gui_magic, JIVE, melatonin_blur, juce_murka, imgui_juce, and awesome-juce.
- `references/skill_package/`: DSP algorithm notes and device inspirations from the provided skill package.
- `references/local/`: internal UI research and demo project notes (blend modes, masking, photorealistic UI).
- `assets/templates/cmake-plugin/`: minimal `juce_add_plugin()` CMake template.
- `assets/templates/Source/`: processor/editor templates with FFT and OpenGL hooks.
- `assets/templates/ui/`: layered UI component templates.
- `assets/shaders/`: shader examples for audio-reactive visuals and particles.
- `assets/source/`: raw visual assets (layered RGBA UI, knobs, Midjourney renders).
- `references/juce-upstream/docs/`: JUCE official docs (CMake API, module format, etc.).
- `references/web/`: downloaded CCRMA reverb/delay math and spatialization links.
- `references/external-paths.md`: location of large external assets and references kept outside the skill.
