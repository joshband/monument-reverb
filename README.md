# Monument

Monument is an abstract, architectural reverb for impossible-scale space. This repository is a
macOS-first JUCE plugin project using a modern CMake workflow (no Projucer).

## Build (macOS, Apple Silicon)

### Prerequisites

- macOS 12+ on Apple Silicon
- Xcode 15+ and Xcode Command Line Tools
- CMake 3.21+
- Git

### Configure and build

Generate an Xcode project and build an AU/VST3:

```sh
cmake -S . -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES=arm64
cmake --build build --config Release
```

Artifacts are placed under `build/Monument_artefacts/Release`.

### Tests

After building, run:

```sh
ctest --test-dir build -C Release
```

### Using a local JUCE checkout

By default, JUCE is fetched with CMake FetchContent. To use a local JUCE checkout:

```sh
cmake -S . -B build -G Xcode \
  -DMONUMENT_USE_LOCAL_JUCE=ON \
  -DJUCE_SOURCE_DIR=/path/to/JUCE
cmake --build build --config Release
```

## Project layout

- `plugin/` - JUCE processor/editor sources
- `dsp/` - DSP building blocks
- `ui/` - UI assets and layout references
- `tests/` - test scaffolding
- `scripts/` - helper scripts
- `docs/` - documentation

## License

MIT. See `LICENSE`.
