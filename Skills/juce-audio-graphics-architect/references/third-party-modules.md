# Third-Party Modules and Integration Notes

## Usage guidance
- Prefer established modules and libraries when they cover the requested feature.
- Pull dependencies from GitHub using a submodule, FetchContent, or CPM.
- Document the dependency, version pin, and build/link steps in the output.
- Reference `references/third-party/` for upstream READMEs and backend notes.

## foleys_gui_magic
- Provides a WYSIWYG GUI workflow plus analyser widgets.
- Use when you want rapid UI assembly or built-in visualizers.
- Integration pattern:
  - Add the module to your project (git submodule or FetchContent).
  - Link the target in CMake and include the module header.
  - Consult the module docs for its processor/editor wrappers and analyser components.
- Repo: `https://github.com/ffAudio/foleys_gui_magic`

## JIVE
- Declarative UI and ValueTree-based styling.
- Useful for themeable UI and state-driven layouts.
- Integration pattern:
  - Add the module to your project.
  - Link the target and include JIVE headers.
  - Define UI in ValueTrees and bind to APVTS or custom trees.
- Repo: `https://github.com/ImJimmi/JIVE`

## melatonin_blur
- CPU-optimized blur and shadow helpers.
- Use for soft shadows or glows when you do not want GPU-only effects.
- Integration pattern:
  - Add the module and link the target.
  - Use its blur or shadow helpers in `paint()` or cached layers.
- Repo: `https://github.com/sudara/melatonin_blur`

## Optional GPU UI (juce_murka / ImGui backends)
- Useful for GPU-accelerated widgets or immediate-mode workflows.
- Integration pattern:
  - Add the backend and ensure OpenGL or Metal context is available.
  - Render UI in the OpenGL renderer callback and pass audio-reactive parameters via atomics or FIFOs.
- juce_murka repo: `https://github.com/Kiberchaika/juce_murka`
- ImGui backend repo: `https://github.com/Krasjet/imgui_juce`
- ImGui core docs: `references/third-party/imgui-docs-README.md` and `references/third-party/imgui-docs-BACKENDS.md`

## Additional modules and tooling (from awesome-juce)
- chowdsp_utils: `https://github.com/Chowdhury-DSP/chowdsp_utils` (`references/third-party/chowdsp_utils-README.md`)
- animator: `https://github.com/bgporter/animator` (`references/third-party/animator-README.md`)
- melatonin_inspector: `https://github.com/sudara/melatonin_inspector` (`references/third-party/melatonin_inspector-README.md`)
- juce_nanovg: `https://github.com/timothyschoen/juce_nanovg` (`references/third-party/juce_nanovg-README.md`)
- OpenGLRealtimeVisualization4JUCE: `https://github.com/JanosGit/OpenGLRealtimeVisualization4JUCE` (`references/third-party/OpenGLRealtimeVisualization4JUCE-README.md`)
- LearnJUCEOpenGL: `https://github.com/ianacaburian/LearnJUCEOpenGL` (`references/third-party/LearnJUCEOpenGL-README.md`)
- 3DAudioVisualizers: `https://github.com/TimArt/3DAudioVisualizers` (`references/third-party/3DAudioVisualizers-README.md`)
- Polyline2DPathRenderer: `https://github.com/CrushedPixel/Polyline2DPathRenderer` (`references/third-party/Polyline2DPathRenderer-README.md`)
- pluginval: `https://github.com/Tracktion/pluginval` (`references/third-party/pluginval-README.md`)
- pamplejuce: `https://github.com/sudara/pamplejuce` (`references/third-party/pamplejuce-README.md`)

## Discovery index
- awesome-juce repo: `https://github.com/sudara/awesome-juce`

## Licensing and compatibility
- Verify the license and compatibility of each module before distributing plugins.
- Some modules have platform-specific requirements or additional dependencies.
- `imgui_juce` is LGPL in the awesome-juce index; verify if that works for your project.
