# /gen-plugin Command

Generate a complete JUCE VST3/AU plugin project.

## Usage

```
/gen-plugin <plugin-name> [options]
```

## Options

- `--synth` - Generate synthesizer (default: effect)
- `--midi` - Enable MIDI input
- `--stereo` - Stereo I/O (default)
- `--mono` - Mono I/O
- `--ui` - Include basic UI (default: no UI)
- `--params <list>` - Comma-separated parameters

## Examples

```
/gen-plugin StereoDelay
/gen-plugin MonoComp --mono --params threshold,ratio,attack,release
/gen-plugin MySynth --synth --midi --ui
```

## Output Structure

```
<PluginName>/
├── CMakeLists.txt
├── Source/
│   ├── PluginProcessor.h
│   ├── PluginProcessor.cpp
│   ├── PluginEditor.h      (if --ui)
│   ├── PluginEditor.cpp    (if --ui)
│   └── DSP/
│       └── (placeholder)
└── README.md
```

## Generated CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.22)
project({{PluginName}} VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add JUCE (adjust path or use FetchContent)
add_subdirectory(JUCE)

juce_add_plugin({{PluginName}}
    COMPANY_NAME "{{CompanyName}}"
    PLUGIN_MANUFACTURER_CODE {{ManuCode}}
    PLUGIN_CODE {{PluginCode}}
    
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "{{PluginName}}"
    
    IS_SYNTH {{IsSynth}}
    NEEDS_MIDI_INPUT {{NeedsMidi}}
    NEEDS_MIDI_OUTPUT FALSE
    
    AU_MAIN_TYPE "{{AUType}}"
    VST3_CATEGORIES "{{VST3Categories}}"
)

target_sources({{PluginName}} PRIVATE
    Source/PluginProcessor.cpp
    {{EditorSource}}
)

target_compile_definitions({{PluginName}} PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries({{PluginName}} PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_basics
PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
```

## Generated PluginProcessor

Include:
- APVTS with requested parameters
- Proper `prepareToPlay` / `processBlock` / `releaseResources`
- State serialization
- Real-time safe by default
- `ScopedNoDenormals` in processBlock

## Parameter Types (auto-detected)

| Name pattern | Type | Range |
|--------------|------|-------|
| `gain`, `volume`, `level` | Float | -60 to +12 dB |
| `frequency`, `freq`, `cutoff` | Float | 20-20000 Hz (skewed) |
| `q`, `resonance` | Float | 0.1-10 |
| `ratio` | Float | 1-20 |
| `threshold` | Float | -60 to 0 dB |
| `attack`, `release` | Float | 0.1-1000 ms |
| `mix`, `blend`, `wet` | Float | 0-100% |
| `bypass`, `enabled` | Bool | |
| `type`, `mode` | Choice | |

## Behavior

1. Parse plugin name and options
2. Generate 4-char codes from name
3. Create directory structure
4. Generate CMakeLists.txt
5. Generate PluginProcessor.h/cpp
6. If `--ui`: Generate PluginEditor.h/cpp
7. Generate README.md with build instructions

## Build Instructions (in README)

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build
cmake --build build --config Release

# Plugin locations
# macOS: build/{{PluginName}}_artefacts/Release/AU/
# macOS: build/{{PluginName}}_artefacts/Release/VST3/
# Windows: build/{{PluginName}}_artefacts/Release/VST3/
```
