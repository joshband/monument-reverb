# Monument Reverb - Architecture Overview

> **Quick Reference**: See [ARCHITECTURE_QUICK_REFERENCE.md](ARCHITECTURE_QUICK_REFERENCE.md) for visual diagrams and fast navigation.
>
> **Detailed Review**: See [docs/architecture/ARCHITECTURE_REVIEW.md](docs/architecture/ARCHITECTURE_REVIEW.md) for comprehensive architectural analysis.

---

## System Architecture

Monument is a memory-based reverb plugin built on JUCE 8.0, featuring physical modeling, chaotic modulation, and elastic acoustic spaces.

### Core DSP Chain

```
Input → Chambers (FDN) → Weathering → Tube Ray Tracer → Output
         ↑
    Elastic Hallway (modulates chamber geometry)
         ↑
    Modulation Matrix → Chaos/Audio/Brownian/Envelope
         ↑
    Macro Controls (6 high-level parameters)
```

### Three Pillars of Innovation

#### 1. Macro Control System
6 high-level parameters that musically morph multiple DSP parameters:
- **MATERIAL** (Soft ↔ Hard) - Density, absorption, reflectivity
- **TOPOLOGY** (Regular ↔ Non-Euclidean) - Space geometry warping
- **VISCOSITY** (Airy ↔ Thick) - Air density, frequency behavior
- **EVOLUTION** (Static ↔ Blooming) - Time-varying characteristics
- **CHAOS** (Stable ↔ Unstable) - Predictability vs. complexity
- **ELASTICITY** (Instant ↔ Slow Recovery) - Wall response time

**Implementation**: `dsp/MacroMapper.h/cpp`

#### 2. Modulation Matrix
4 modulation sources can route to 16+ parameter destinations with 64+ simultaneous connections:
- **Chaos Attractor** - Lorenz/Rössler strange attractors
- **Audio Follower** - Input-reactive modulation
- **Brownian Motion** - Smooth random walk
- **Envelope Tracker** - Multi-stage amplitude following

**Visual UI**: Interactive 4×15 grid panel with color-coded sources, depth/smoothing controls, and real-time connection editing (Phase 4).

**Implementation**: `dsp/ModulationMatrix.h/cpp`, `ui/ModMatrixPanel.h/cpp`, `dsp/ChaosAttractor.h/cpp`, etc.

#### 3. Physical/Algorithmic Modules

**Tube Ray Tracer**: Metal tube resonance modeling with ray propagation
- 8-16 virtual tubes with modal resonances
- Distance-based absorption
- Inter-tube coupling

**Elastic Hallway**: Walls that deform under acoustic pressure
- Room geometry (width, height, depth)
- Non-linear reflections
- Slow recovery creates evolving timbre

**Implementation**: `dsp/TubeRayTracer.h/cpp`, `dsp/ElasticHallway.h/cpp`

---

## Project Structure

```
monument-reverb/
├── plugin/                 # JUCE plugin wrapper
│   ├── PluginProcessor.cpp # Audio processing & parameter management
│   └── PluginEditor.cpp    # UI (knobs, controls)
├── ui/                     # Custom UI components
│   ├── LayeredKnob.h/cpp   # Photorealistic layered knobs
│   ├── ModMatrixPanel.h/cpp # Modulation matrix visual editor (NEW)
│   └── Monument*.h         # Parameter-specific knob wrappers
├── dsp/                    # DSP algorithms (to be created)
│   ├── MacroMapper.h/cpp
│   ├── ModulationMatrix.h/cpp
│   ├── ChaosAttractor.h/cpp
│   ├── TubeRayTracer.h/cpp
│   └── ElasticHallway.h/cpp
├── assets/ui/              # Knob layer PNGs
├── scripts/                # Blender knob generation
├── docs/                   # Documentation
│   ├── ui/                 # UI design docs
│   ├── development/        # Quick start guides
│   ├── architecture/       # Technical architecture
│   └── testing/            # Test plans & validation
└── build/                  # CMake build output (gitignored)
```

---

## Build System

**Standard Build**: See [STANDARD_BUILD_WORKFLOW.md](STANDARD_BUILD_WORKFLOW.md)

```bash
# Incremental build (6 seconds)
cmake --build build --target Monument_AU --config Release -j8
```

**Auto-installs to**:
- `~/Library/Audio/Plug-Ins/Components/Monument.component` (AU)
- `~/Library/Audio/Plug-Ins/VST3/Monument.vst3` (VST3)

---

## Key Technologies

- **JUCE 8.0** - Audio plugin framework
- **CMake** - Build system with incremental compilation
- **Blender** - Procedural knob layer generation (Python scripts)
- **C++17** - Language standard

---

## Development Phases

**Phase 1**: ✅ Foundation (JUCE setup, FDN reverb, base parameters)
**Phase 2**: ✅ Macro system (6 high-level controls integrated)
**Phase 3**: ✅ Modulation sources (4 sources, 16 destinations, living presets)
**Phase 4**: 🚀 UI Enhancement (90% complete)

- ✅ ModMatrix visual panel with interactive 4×15 grid
- ✅ LayeredKnob rendering system
- ✅ Blender knob generation pipeline
- ⏳ Enhanced knob integration (final step)

**Phase 5**: 📋 Polish & release (preset browser, export/import)
**Phase 6**: 📋 Physical modules (Tubes, Elastic spaces)

**Note**: Memory Echoes was extracted to standalone repository with planned v1.6 reintegration.

---

## Documentation Index

**Getting Started**:
- [README.md](README.md) - Project overview
- [docs/development/QUICK_START_BLENDER_KNOBS.md](docs/development/QUICK_START_BLENDER_KNOBS.md) - Generate knobs with Blender
- [docs/development/QUICK_START_MACRO_TESTING.md](docs/development/QUICK_START_MACRO_TESTING.md) - Test macro controls

**Architecture**:
- [ARCHITECTURE_QUICK_REFERENCE.md](ARCHITECTURE_QUICK_REFERENCE.md) - Visual diagrams & fast navigation
- [docs/architecture/ARCHITECTURE_REVIEW.md](docs/architecture/ARCHITECTURE_REVIEW.md) - Detailed review
- [docs/architecture/DSP_CLICK_ANALYSIS_REPORT.md](docs/architecture/DSP_CLICK_ANALYSIS_REPORT.md) - DSP debugging

**UI Design**:
- [docs/ui/LAYERED_KNOB_DESIGN.md](docs/ui/LAYERED_KNOB_DESIGN.md) - Knob design system
- [docs/ui/LAYERED_KNOB_WORKFLOW.md](docs/ui/LAYERED_KNOB_WORKFLOW.md) - Asset pipeline
- [docs/ui/MONUMENT_UI_STRATEGIC_DESIGN_PLAN.md](docs/ui/MONUMENT_UI_STRATEGIC_DESIGN_PLAN.md) - UI strategy

**Testing**:
- [docs/testing/MODULATION_TESTING_GUIDE.md](docs/testing/MODULATION_TESTING_GUIDE.md) - Test modulation system
- [docs/testing/PHASE_2_VALIDATION_TEST.md](docs/testing/PHASE_2_VALIDATION_TEST.md) - Phase 2 validation
- [docs/testing/PHASE_3_COMPLETE_SUMMARY.md](docs/testing/PHASE_3_COMPLETE_SUMMARY.md) - Phase 3 results

**Process**:
- [CONTRIBUTING.md](CONTRIBUTING.md) - Contribution guidelines
- [CHANGELOG.md](CHANGELOG.md) - Version history
- [MANIFEST.md](MANIFEST.md) - Project manifest
