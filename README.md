# Monument

Monument is an abstract, architectural reverb for impossible-scale space.
It is a compositional tool for massive, slow, dense, evolving ambience.

Monument is not a room simulator, plate, spring, or convolution reverb.
It is a structure sound enters, not a space that reflects sound.

Core priorities:
- Scale over realism
- Density over echo
- Slow tectonic motion
- Controlled instability
- Mono-safe vastness

---

## Project Status

**Current Phase**: Phase 4 - UI Enhancement (✅ Complete) | **Next**: Phase 5 - Polish & Release 🚀

Monument features a memory-free reverb architecture. Memory Echoes lives in a standalone repository with planned v1.6 reintegration.

**Recent Updates** (2026-01-03 Evening):

- ✅ **ModMatrixPanel**: Professional modulation matrix UI with 4×15 grid
- ✅ Color-coded sources (Chaos, Audio, Brownian, Envelope) with visual feedback
- ✅ Interactive connection editing with depth/smoothing controls
- ✅ LayeredKnob rendering system with photorealistic layers
- ✅ Blender procedural knob generation pipeline
- ✅ Standardized build workflow (incremental builds in ~6 seconds)
- ✅ HIGH priority optimizations (parameter cache, smoothing, thread safety)

---

## Quick Start

### Prerequisites

- macOS 12+
- Xcode 15+ and Command Line Tools
- CMake 3.21+
- Git

### Build and Install

**Recommended**: Use standardized incremental builds (see [STANDARD_BUILD_WORKFLOW.md](STANDARD_BUILD_WORKFLOW.md))

```bash
# Initial setup (first time only)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Incremental build (6 seconds, auto-installs)
cmake --build build --target Monument_AU --config Release -j8
```

**Legacy scripts** (still supported):
```bash
./scripts/build_macos.sh
./scripts/install_macos.sh
./scripts/open_xcode.sh
```

**Artifacts**: Auto-installed to `~/Library/Audio/Plug-Ins/{Components,VST3}/`

### Generate UI Knobs

See [docs/development/QUICK_START_BLENDER_KNOBS.md](docs/development/QUICK_START_BLENDER_KNOBS.md) for photorealistic knob generation.

```bash
# Generate knob layers with Blender
./scripts/run_blender_knobs.sh

# Preview composite before building
python3 scripts/preview_knob_composite.py --rotation 45
```

---

## Documentation

**Central Hub**: [docs/INDEX.md](docs/INDEX.md)

### Essential Reading

- [ARCHITECTURE.md](ARCHITECTURE.md) - System architecture overview
- [STANDARD_BUILD_WORKFLOW.md](STANDARD_BUILD_WORKFLOW.md) - Build commands
- [MANIFEST.md](MANIFEST.md) - Project vision & roadmap

### Quick Starts

- [docs/development/QUICK_START_BLENDER_KNOBS.md](docs/development/QUICK_START_BLENDER_KNOBS.md) - Generate knobs
- [docs/development/QUICK_START_MACRO_TESTING.md](docs/development/QUICK_START_MACRO_TESTING.md) - Test macros

### Architecture & DSP

- [ARCHITECTURE_QUICK_REFERENCE.md](ARCHITECTURE_QUICK_REFERENCE.md) - Visual diagrams
- [docs/architecture/DSP_ARCHITECTURE.md](docs/architecture/DSP_ARCHITECTURE.md) - Signal flow
- [docs/architecture/PARAMETER_BEHAVIOR.md](docs/architecture/PARAMETER_BEHAVIOR.md) - Parameter contracts

### UI Design

- [docs/ui/LAYERED_KNOB_DESIGN.md](docs/ui/LAYERED_KNOB_DESIGN.md) - Design system
- [docs/ui/LAYERED_KNOB_WORKFLOW.md](docs/ui/LAYERED_KNOB_WORKFLOW.md) - Asset pipeline

### Testing

- [docs/testing/TESTING.md](docs/testing/TESTING.md) - General testing (pluginval)
- [docs/testing/MODULATION_TESTING_GUIDE.md](docs/testing/MODULATION_TESTING_GUIDE.md) - Modulation tests

---

## Signal Flow

```mermaid
flowchart LR
  Input --> Foundation --> Pillars --> Chambers --> Weathering --> Buttress --> Facade --> Mix --> Output
  Input -. Dry tap .-> Mix
```

![Monument signal flow](docs/assets/monument-signal-flow.svg)

**Modules at a glance**:
- **Foundation**: Input conditioning and headroom
- **Pillars**: Early reflection clusters
- **Chambers**: FDN reverb core
- **Weathering**: Slow motion (warp, drift)
- **Buttress**: Feedback safety and limiting
- **Facade**: Stereo imaging, air, width, and mix

---

## Parameters

All parameters are normalized to [0, 1] unless noted. Mix is [0, 100].

![Monument parameter compass](docs/assets/monument-parameter-compass.svg)

### Primary Controls

| Control | Meaning | Behavior notes |
| --- | --- | --- |
| Time | Tail duration | Maps to feedback gain for long T60 values |
| Mass | Weight and darkness | Increases damping and low-end persistence |
| Density | Reflection complexity | Higher values increase diffusion and continuity |
| Bloom | Late swell | Shapes a slow envelope on late field injection |
| Air | Upper-band lift | Adds openness without harshness |
| Width | Stereo spread | Applies to wet only, keeps mono-safe center |
| Mix (0-100) | Wet/dry blend | Constant-power crossfade |

### Advanced Controls

| Control | Meaning | Behavior notes |
| --- | --- | --- |
| Warp | Space topology bend | Morphs orthogonal FDN matrices at sub-Hz rates |
| Drift | Micro motion | Per-line slow LFO delay offsets, ultra subtle |
| Gravity | Spectral tilt | Frequency-dependent decay and high-pass gravity |
| Freeze | Infinite hold | Locks FDN state, mutes input, smooth crossfade |
| Pillar Shape | Early reflection spacing | Compresses or expands tap intervals |
| Pillar Mode | Early reflection palette | Glass, Stone, Fog tap profiles |

### Macro Controls & Modulation (Phase 2-4 Complete ✅)

Monument includes a high-level macro system and visual modulation matrix for deep sound design.

**Macro Controls:**

| Macro | Meaning | Influences |
| --- | --- | --- |
| Material | Surface character (soft → hard) | Time, Mass, Density |
| Topology | Spatial geometry (regular → non-Euclidean) | Warp, Drift |
| Viscosity | Medium resistance (airy → thick) | Time, Air, Mass |
| Evolution | Temporal change (static → evolving) | Bloom, Drift |
| Chaos | Unpredictable motion (stable → chaotic) | Warp, Drift |
| Elasticity | Deformation speed (instant → slow) | Reserved for future |

**Modulation Matrix** (Phase 3-4 Complete ✅):

- 4 source types: Chaos Attractor, Audio Follower, Brownian Motion, Envelope Tracker
- 16 parameter destinations with 64+ simultaneous connections
- **Visual UI Panel**: Interactive 4×15 grid for creating/editing connections
- Per-connection depth (-1 to +1) and smoothing (20-1000ms) controls
- Color-coded sources with hover effects and selection states
- Thread-safe real-time updates via SpinLock
- See [docs/testing/MODULATION_TESTING_GUIDE.md](docs/testing/MODULATION_TESTING_GUIDE.md)

---

## Presets

Monument ships with curated, whimsical presets that explore extremes and hybrids.

- Init Patch: neutral starting point for new spaces
- Cathedral of Glass: long, bright, high density shimmer
- Event Horizon: dark, swelling tail with bent geometry
- Folded Atrium: medium time, strong warp, controlled bloom
- Monumental Void: massive, sparse, and hollow
- Zero-G Garden: short, blooming, weightless room
- Hall of Mirrors: warped reflections, medium density
- Tesseract Chamber: long, low density, high drift
- Stone Circles: tight, dry, and grounded
- Frozen Monument (Engage Freeze): tuned for Freeze captures

User presets can be saved as JSON in `~/Documents/MonumentPresets/`.

See [docs/PRESET_GALLERY.md](docs/PRESET_GALLERY.md) for full descriptions.

---

## Testing and Validation

- **Automated**: pluginval via `./scripts/run_pluginval.sh` (see [docs/testing/TESTING.md](docs/testing/TESTING.md))
- **Optional logging**: Enable `MONUMENT_TESTING` for peak and block timing
- **Performance**: REAPER Performance Monitor with 50-100 instances
- **Instrumentation**: Instruments, AddressSanitizer, or Valgrind for leak checks

---

## Project Structure

```
monument-reverb/
├── plugin/           # JUCE processor and editor
├── dsp/              # DSP modules (to be created)
├── ui/               # Custom UI components (LayeredKnob)
├── assets/ui/        # Knob layer PNGs
├── scripts/          # Build scripts, Blender knob generation
├── tests/            # CTest coverage
├── docs/             # Documentation (see docs/INDEX.md)
└── build/            # CMake build output (gitignored)
```

---

## Development Phases

- **Phase 1**: ✅ Foundation (JUCE setup, FDN reverb, base parameters)
- **Phase 2**: ✅ Macro system (6 high-level controls integrated)
- **Phase 3**: ✅ Modulation sources (4 sources, 16 destinations, living presets)
- **Phase 4**: 🚀 UI Enhancement (90% complete)
  - ✅ ModMatrix visual panel with interactive grid
  - ✅ LayeredKnob rendering system
  - ✅ Blender knob generation pipeline
  - ⏳ Enhanced knob integration (final step)
- **Phase 5**: 📋 Polish & release (preset browser, export/import)
- **Phase 6**: 📋 Physical modules (Tubes, Elastic spaces)

See [ARCHITECTURE.md](ARCHITECTURE.md) for detailed roadmap.

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for contribution guidelines.

---

## License

MIT. See [LICENSE](LICENSE).
