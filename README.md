# Monument Reverb

**An abstract, architectural reverb for impossible-scale space**

Monument is a compositional tool for massive, slow, dense, evolving ambience. It is not a room simulator, plate, or spring reverb—it is a structure sound enters, not a space that reflects sound.

**Core Priorities:**
- Scale over realism
- Density over echo
- Slow tectonic motion
- Controlled instability
- Mono-safe vastness

---

## Project Status

**Platform:** macOS (arm64/Intel) via Xcode/CMake. Windows/Linux are not built or tested by this repository's workflows.

**Build Status:** VST3/AU/Standalone compile via CMake; see [STANDARD_BUILD_WORKFLOW.md](STANDARD_BUILD_WORKFLOW.md).

**Test Status:** See [TESTING.md](TESTING.md) for the current test matrix, known-red tests, and how to run them — don't infer status from this file.

**Preset Count:** 37 factory presets (see `plugin/PresetManager.cpp`).

**For development history, see [CHANGELOG.md](CHANGELOG.md).**

---

## Quick Start

### Prerequisites

- macOS 12+
- Xcode 15+ and Command Line Tools
- CMake 3.21+
- Git

### Build and Install

```bash
# Configure (Xcode generator)
cmake -S . -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES=arm64

# Build Release
cmake --build build --config Release -j8

# One-shot Debug + Release build
./scripts/build_macos.sh

# Generate and open Xcode project
./scripts/open_xcode.sh

# Run tests
ctest --test-dir build -C Release
```

**Artifacts auto-install to:**
- `~/Library/Audio/Plug-Ins/Components/Monument.component` (AU)
- `~/Library/Audio/Plug-Ins/VST3/Monument.vst3` (VST3)

**For detailed build workflows, see [STANDARD_BUILD_WORKFLOW.md](STANDARD_BUILD_WORKFLOW.md)**

---

## Documentation

**Essential Documents:**

- [AGENTS.md](AGENTS.md) - Repository operating contract and canonical commands
- [ARCHITECTURE.md](ARCHITECTURE.md) - System architecture overview
- [TESTING.md](TESTING.md) - Validation taxonomy, commands, and known limitations
- [ROADMAP.md](ROADMAP.md) - Long-term vision and future enhancements
- [MANIFEST.md](MANIFEST.md) - Project manifesto and design philosophy
- [STANDARD_BUILD_WORKFLOW.md](STANDARD_BUILD_WORKFLOW.md) - Build commands and workflows
- [docs/STATUS.md](docs/STATUS.md) - Implementation status (historical; may lag `git log`)
- [CHANGELOG.md](CHANGELOG.md) - Detailed session history

**Detailed Documentation:**

- [docs/architecture/](docs/architecture/) - DSP architecture (15/17 modules documented)
- [docs/development/](docs/development/) - Development guides and optimization strategies
- [docs/testing/](docs/testing/) - Testing infrastructure and guides
- [docs/ui/](docs/ui/) - UI design and asset workflows

**Architecture Reviews:**

- [docs/archive/reviews/](docs/archive/reviews/) - Code, performance, and architecture reviews (2026-01-07/08)

---

## Architecture Overview

**Core Signal Flow:**

```
Input → Foundation → Pillars → Chambers → Weathering → Physical Modules → Buttress → Facade → Output
          ↓                                                                                    ↑
          └────────────────────────────── Dry Signal ───────────────────────────────────────┘
```

This is the default `AncientWay` fixed chain, the only one reachable through the
current UI/host controls; two other internal chain implementations exist but
have no demonstrated external caller (see `ARCHITECTURE.md`).

**DSP Modules:**

1. **Foundation** - Input conditioning (pre-delay, filtering)
2. **Pillars** - Early reflections (8-tap allpass diffuser)
3. **Chambers** - FDN reverb core (8×8 feedback matrix)
4. **Weathering** - LFO modulation (4 sources × 27 destinations)
5. **Physical Modules** - TubeRayTracer, ElasticHallway, AlienAmplification (MemoryEchoes is prepared but not reached by ordinary processing)
6. **Buttress** - Feedback safety (soft clipping, limiter)
7. **Facade** - Output stage (dry/wet mix, stereo width)

**Supporting Systems:**
- ParameterBuffers - Zipper noise elimination
- SpatialProcessor - 3D positioning with Doppler shift
- AllpassDiffuser - Schroeder topology diffusion
- ModulationMatrix - 4 sources × 27 destinations

**For detailed architecture, see [ARCHITECTURE.md](ARCHITECTURE.md)**

---

## Parameters

**Macro System** (3 high-level controls):

| Macro | Meaning | Controls |
| --- | --- | --- |
| Scale | Overall size and time | Time, decay, pre-delay |
| Character | Tonal quality | Damping, diffusion, modulation |
| Breath | Movement and life | LFO rates, drift, evolution |

**Core Parameters** (25+ internal parameters driven by macros):

- **Time** - Reverb tail duration
- **Mass** - Weight and darkness
- **Density** - Reflection complexity
- **Bloom** - Late energy swell
- **Air** - High-frequency lift
- **Width** - Stereo spread
- **Mix** - Dry/wet balance

**Advanced Parameters** - Warp, Drift, Gravity, Freeze, and more

**For complete parameter reference, see [docs/architecture/](docs/architecture/)**

---

## Presets

Monument includes **37 factory presets**, spanning plain architectural spaces
through physically-modeled and modulation-driven ("Living") variants. A few
representative examples:

- **Init Patch** - a clean, even hall with no motion
- **Cathedral of Glass** - bright surfaces with long, fragile light trails
- **Breathing Stone** - the hall expands and contracts with the input signal
- **Metallic Corridor** - sound routed through resonant metal tubes (TubeRayTracer)
- **Fractal Space** - topology morphs through a chaotic attractor

See `plugin/PresetManager.cpp` for the complete, authoritative list and descriptions.

User presets can be saved as JSON in `~/Documents/MonumentPresets/`.

---

## Testing

```bash
# Build analyzer (needed for audio regression + preset capture)
cmake --build build --config Release --target monument_plugin_analyzer

# Use a non-default build directory (e.g., Ninja)
BUILD_DIR=build-ninja cmake --build build-ninja --config Release --target monument_plugin_analyzer

# Install Python deps for analysis (one-time)
python3 -m pip install -r tools/plugin-analyzer/python/requirements.txt

# Run C++ tests
ctest --test-dir build -C Release

# Run C++ tests from a Ninja build
ctest --test-dir build-ninja -C Release

# Run comprehensive CI/QA harness (CTest + audio regression + quality gates)
./scripts/run_ci_tests.sh
```

Set `TEST_CONFIG=Debug` to point the harness at Debug builds and test binaries.
Set `BUILD_DIR=build-ninja` to point the harness at a Ninja build directory.

**Testing hub:** [TESTING.md](TESTING.md) (canonical) and [docs/testing/README.md](docs/testing/README.md) (index).

---

## UI Prototype

```bash
cmake --build build --config Debug --target monument_ui_prototype
```

Run the app at:

```
build/monument_ui_prototype_artefacts/Debug/Monument UI Prototype.app
```

---

## Project Structure

```
monument-reverb/
├── plugin/           # JUCE processor and editor
├── dsp/              # DSP modules
├── ui/               # UI components + UI prototype entrypoint
├── assets/ui/        # Knob layer PNGs
├── scripts/          # Build scripts, Blender knob generation
├── tests/            # CTest coverage
├── docs/             # Documentation (see docs/INDEX.md)
├── build/            # CMake build output (Xcode, gitignored)
└── build-ninja/      # Ninja build output (optional, gitignored)
```

---

## Development Roadmap

Development history lives in `git log` and [CHANGELOG.md](CHANGELOG.md), not a
phase checklist. Known open realtime-safety work — timeline-preset and
TubeRayTracer allocation on the audio thread — is tracked as a deliberately
failing characterization test; see [TESTING.md](TESTING.md).

**For long-term vision, see [ROADMAP.md](ROADMAP.md)**

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for contribution guidelines.

---

## License

MIT. See [LICENSE](LICENSE).
