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

**Current Phase:** Phase 4 Complete - Supporting Systems Documentation (2026-01-09)

**Build Status:** ✅ VST3/AU compiling and installing successfully

**Test Status:** See `TESTING.md` for the current test matrix and run results.

**Preset Count:** 8 presets (3 original + 5 experimental)

**Documentation:** 15/17 DSP architecture modules documented (~184,500 words)

**For detailed status, see [docs/STATUS.md](docs/STATUS.md)**

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

- [ARCHITECTURE.md](ARCHITECTURE.md) - System architecture overview
- [ROADMAP.md](ROADMAP.md) - Long-term vision and future enhancements
- [MANIFEST.md](MANIFEST.md) - Project manifesto and design philosophy
- [STANDARD_BUILD_WORKFLOW.md](STANDARD_BUILD_WORKFLOW.md) - Build commands and workflows
- [docs/STATUS.md](docs/STATUS.md) - Current implementation status
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
Input → Foundation → Pillars → Chambers → Physical Modules → Weathering → Buttress → Facade → Output
          ↓                                                                                    ↑
          └────────────────────────────── Dry Signal ───────────────────────────────────────┘
```

**DSP Modules:**

1. **Foundation** - Input conditioning (pre-delay, filtering)
2. **Pillars** - Early reflections (8-tap allpass diffuser)
3. **Chambers** - FDN reverb core (8×8 feedback matrix)
4. **Physical Modules** - TubeRayTracer, ElasticHallway, AlienAmplification, MemoryEchoes
5. **Weathering** - LFO modulation (4 sources × 27 destinations)
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

Monument includes **8 factory presets:**

**Original Presets:**
1. Cathedral Space - Large reverb with 8.5s decay
2. Spring Chamber - Vintage spring simulation
3. Infinite Abyss - 20s decay with memory system

**Experimental Presets:**
4. Elastic Hall - Living Stone algorithm
5. Alien Amplification - Impossible Geometry algorithm
6. Memory Lane - Memory system focus
7. Parallel Universe - Parallel routing configuration
8. Feedback Loop - Series routing with high feedback

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

**Current Status:** Phase 4 Complete - Supporting Systems Documentation

**Completed Phases:**
- ✅ Phase 1: Foundation (JUCE setup, FDN reverb core)
- ✅ Phase 2: Core Signal Flow (6 modules documented)
- ✅ Phase 3: Physical Modeling + Memory (4 modules)
- ✅ Phase 4: Supporting Systems (4 modules)

**Next Steps:**
- Complete remaining 2 DSP documentation modules
- Address critical RT-safety issues (routing allocation, SpinLock)
- Performance optimization (SIMD, memory ordering)
- UI enhancement planning

**For long-term vision, see [ROADMAP.md](ROADMAP.md)**

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for contribution guidelines.

---

## License

MIT. See [LICENSE](LICENSE).
