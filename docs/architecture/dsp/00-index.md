# Monument Reverb: DSP Architecture Documentation

**Version:** 1.0
**Last Updated:** 2026-01-09
**Status:** In Progress

---

## Welcome to the Monument

> "Monument is not a room simulator, plate, spring, or convolution reverb. It is a **structure sound enters**, not a space that reflects sound."

This documentation explores Monument Reverb's DSP architecture through the lens of **architectural structures, stone, and geological time**. Each module represents a physical element of an impossible-scale monument where sound reverberates through stone, metal, and non-Euclidean geometry.

---

## Documentation Philosophy

Monument's documentation balances **three pillars**:

1. **Technical Rigor** - Mathematical foundations, DSP algorithms, performance metrics
2. **Visual Accessibility** - Diagrams, infographics, charts that make complex concepts clear
3. **Thematic Coherence** - Architectural metaphors that unify the design vision

Each module document includes:
- Monument metaphor (architectural description)
- Mathematical foundation (LaTeX equations)
- Implementation details (C++ code, pseudocode)
- Visual infographics (signal flow, parameter response)
- Performance metrics (CPU, memory, optimization history)
- Test coverage and usage examples

---

## Quick Start

**New to Monument?** Start here:

1. [Monument Theme](00-monument-theme.md) - Understand the architectural metaphor
2. [Signal Flow Overview](00-signal-flow-overview.md) - High-level architecture
3. [Chambers](core-modules/03-chambers.md) - The heart of Monument (FDN reverb core)

**Implementing DSP?** Go directly to:

- Module-specific docs (see [Core Modules](#core-modules) below)
- [Routing Graph](core-modules/07-routing-graph.md) - Orchestration and signal routing

**Understanding Performance?** Check:

- Performance sections in each module doc
- `docs/PERFORMANCE_BASELINE.md` (project root)

---

## Monument DSP Modules

Monument consists of **11 DSP modules** organized by sonic function. All modules are equal architectural elements - no hierarchy, just different roles in the signal flow.

### Overview Documents

| Document | Description | Status |
|----------|-------------|--------|
| [00-monument-theme.md](00-monument-theme.md) | Thematic foundation and architectural metaphor | ✅ Complete |
| [00-signal-flow-overview.md](00-signal-flow-overview.md) | High-level architecture and signal routing | ✅ Complete |

---

### Input Stage (1 module)

Sound enters the monument through a solid foundation:

| # | Module | Monument Metaphor | CPU (p99) | Status |
|---|--------|-------------------|-----------|--------|
| [01](core-modules/01-foundation.md) | **Foundation** | Heavy stone doors | ~0.1% | 📝 Draft |

---

### Early Reflections (1 module)

Initial scattering off vertical architectural elements:

| # | Module | Monument Metaphor | CPU (p99) | Status |
|---|--------|-------------------|-----------|--------|
| [02](core-modules/02-pillars.md) | **Pillars** | 32 stone columns (Glass/Stone/Fog) | 5.38% | 📝 Draft |

---

### Reverb Core (1 module)

The heart of Monument - an 8-chambered feedback delay network:

| # | Module | Monument Metaphor | CPU (p99) | Status |
|---|--------|-------------------|-----------|--------|
| [03](core-modules/03-chambers.md) | **Chambers** | Vaulted catacombs with interconnected passages | 7.22% | ✅ Complete |

---

### Coloration & Time (4 modules)

Modules that color, age, and transform the reverb character:

| # | Module | Monument Metaphor | CPU (p99) | Status |
|---|--------|-------------------|-----------|--------|
| [04](core-modules/04-weathering.md) | **Weathering** | Erosion over geological time | ~0.5% | 📝 Draft |
| [05](physical-modeling/08-resonance.md) | **Resonance** | Metal pipes embedded in stone | 0.03% | 🔄 In Progress |
| [06](physical-modeling/09-living-stone.md) | **Living Stone** | Breathing walls responding to pressure | 3.38% | 🔄 In Progress |
| [07](physical-modeling/10-impossible-geometry.md) | **Impossible Geometry** | Non-Euclidean space where physics breaks | 4.05% | 🔄 In Progress |

**Technical Names (Code):**
- Resonance = `TubeRayTracer`
- Living Stone = `ElasticHallway`
- Impossible Geometry = `AlienAmplification`

---

### Memory (1 module)

Temporal feedback system preserving acoustic history in geological layers:

| # | Module | Monument Metaphor | CPU (p99) | Status |
|---|--------|-------------------|-----------|--------|
| [08](memory-system/11-strata.md) | **Strata** | Sedimentary layers (2-4s sediment, 20-60s stone) | 0.15% | 🔄 In Progress |

**Technical Name (Code):** `MemoryEchoes` (scheduled for rename to `Strata`)

---

### Output Stage (2 modules)

Structural support and external presentation:

| # | Module | Monument Metaphor | CPU (p99) | Status |
|---|--------|-------------------|-----------|--------|
| [09](core-modules/05-buttress.md) | **Buttress** | Structural support prevents collapse | ~0.2% | 📝 Draft |
| [10](core-modules/06-facade.md) | **Facade** | The monument's external face | ~0.8% | 📝 Draft |

---

### Orchestration (1 module)

Signal routing and module coordination:

| # | Module | Monument Metaphor | CPU (p99) | Status |
|---|--------|-------------------|-----------|--------|
| [11](core-modules/07-routing-graph.md) | **Routing Graph** | Architectural blueprint (3 routing modes) | ~0.5% | ✅ Complete |

---

### Performance Summary

| Category | Modules | CPU (p99) | % of Budget |
|----------|---------|-----------|-------------|
| Input | 1 | 0.1% | 0.3% |
| Early Reflections | 1 | 5.38% | 18% |
| Reverb Core | 1 | 7.22% | 24% |
| Coloration & Time | 4 | 8.0% | 27% |
| Memory | 1 | 0.15% | 0.5% |
| Output | 2 | 1.0% | 3% |
| Orchestration | 1 | 0.5% | 2% |
| **Total** | **11** | **~22%** | **73%** |
| **Headroom** | — | **~8%** | **27%** |

**Target Budget:** <30% CPU (48kHz, 512 samples)
**Status:** ✅ Within budget (Phase 4 optimization complete)

---

### Control Systems (Parameter Mapping)

High-level macro controls that translate user intent into coordinated DSP parameter changes:

| #    | Module               | Purpose                                | Status       |
|------|----------------------|----------------------------------------|--------------|
| [00](control-systems/00-ancient-monuments.md) | **Ancient Monuments** | 10 thematic macros → 40+ DSP parameters | ✅ Complete |

---

### Supporting Systems (Infrastructure)

Critical infrastructure that powers Monument's DSP:

| # | Module | Purpose | Status |
|---|--------|---------|--------|
| [12](supporting-systems/12-parameter-buffers.md) | **Parameter Buffers** | Per-sample automation (zero-copy, 16-byte views) | ✅ Complete |
| [13](supporting-systems/13-spatial-processor.md) | **Spatial Processor** | 3D positioning (azimuth/elevation) | ✅ Complete |
| [14](supporting-systems/14-allpass-diffuser.md) | **Allpass Diffuser** | Diffusion networks (used in Chambers, Pillars) | ✅ Complete |
| [15](supporting-systems/15-modulation-sources.md) | **Modulation Sources** | LFO waveforms + envelope followers | ✅ Complete |

---

## Reading Paths

### For DSP Engineers

**Recommended Order:**

1. [Signal Flow Overview](00-signal-flow-overview.md) - Understand the architecture
2. [Chambers](core-modules/03-chambers.md) - Study the FDN reverb core (most complex)
3. [Routing Graph](core-modules/07-routing-graph.md) - Learn signal routing
4. [Parameter Buffers](supporting-systems/12-parameter-buffers.md) - Per-sample automation
5. [Physical Modeling Modules](physical-modeling/) - Novel algorithms

**Focus:** Mathematical foundations, real-time safety, performance optimization

---

### For Sound Designers

**Recommended Order:**

1. [Monument Theme](00-monument-theme.md) - Understand the metaphor
2. [Signal Flow Overview](00-signal-flow-overview.md) - How modules connect
3. [Pillars](core-modules/02-pillars.md) - Early reflections (3 modes: Glass/Stone/Fog)
4. [Chambers](core-modules/03-chambers.md) - Reverb core parameters (time, mass, density, bloom, gravity)
5. [Weathering](core-modules/04-weathering.md) - Modulation effects (warp, drift)
6. [Strata](memory-system/11-strata.md) - Memory feedback system

**Focus:** Parameter descriptions, preset examples, sonic character

---

### For Plugin Developers

**Recommended Order:**

1. [Signal Flow Overview](00-signal-flow-overview.md) - Architecture overview
2. [Routing Graph](core-modules/07-routing-graph.md) - Signal routing patterns
3. [Foundation](core-modules/01-foundation.md) → [Facade](core-modules/06-facade.md) - Full signal path
4. [Supporting Systems](supporting-systems/) - Infrastructure modules

**Focus:** Code examples, usage patterns, integration points

---

## Performance Budget

Monument targets **<30% CPU usage** at 48kHz, 512 samples:

| Category | CPU (p99) | % of Budget |
|----------|-----------|-------------|
| **Core Modules** | 14.5% | 48% |
| **Physical Modeling** | 7.5% | 25% |
| **Memory System** | 0.15% | 0.5% |
| **Total** | ~22% | 73% |
| **Headroom** | ~8% | 27% |

**Optimization Status:** ✅ Within budget (Phase 4 complete, 31% improvement in Chambers)

---

## Visual Assets

### Global Architecture Diagrams

- [Complete Signal Flow](assets/diagrams/monument-complete-signal-flow.svg) - All 11 modules interconnected (🔄 In Progress)
- [Performance Dashboard](assets/infographics/performance-dashboard.png) - CPU/memory overview (🔄 In Progress)
- [Parameter Map](assets/infographics/parameter-map.svg) - Macro → module mappings (🔄 In Progress)
- [Monument Architecture Concept](assets/monument-architecture-concept.png) - Artist's rendering (🔄 In Progress)

### Per-Module Assets

Each module includes:
- **Signal Flow Diagram** (`diagrams/[module]-signal-flow.svg`)
- **Parameter Response Chart** (`infographics/[module]-parameter-response.png`)
- **CPU Usage Visualization** (`charts/[module]-cpu-over-time.png`)
- **Module Icon** (`icons/[module]-icon.svg`)

**Design Aesthetic:** Brutalist (stone gray, concrete white, steel blue)

---

## Monument Routing Modes

Monument supports **3 core routing topologies** for sonic diversity:

### 1. AncientWay (Traditional - Default)
```
Foundation → Pillars → Chambers → Weathering → [Physical Modeling] → Buttress → Facade
```
**Character:** Classic reverb with physical modeling enhancements

### 2. ResonantHalls (Metallic First)
```
Foundation → Pillars → Resonance → Chambers → Weathering → [Physical] → Buttress → Facade
```
**Character:** Bright metallic coloration BEFORE reverb diffusion

### 3. BreathingStone (Elastic Core)
```
Foundation → Pillars → Living Stone → Chambers → Living Stone → [Physical] → Buttress → Facade
```
**Character:** Chambers sandwiched between deformable walls for organic breathing

**More Routing Presets:** See [Routing Graph](core-modules/07-routing-graph.md) documentation

---

## Strata Integration (Upcoming)

Monument's **Strata** (memory system) will be fully integrated into the routing graph as the **11th routable module**:

### New Routing Presets with Strata

1. **Ghostly Cathedral** - Strata AFTER Chambers (captures reverb tail)
2. **Fragmented Reality** - Strata BEFORE Chambers (memory-driven early reflections)
3. **Recursive Haunt** - Strata in feedback loop (infinite temporal regression)
4. **Metallic Memory** - Strata after Resonance (tube resonance memories)

**Status:** 🔄 Planned for Phase 6 (Code Refactoring)

---

## Development Phases

### Phase 1: Documentation Foundation (Current)
- [ ] Directory structure ✅
- [ ] 00-index.md ✅
- [ ] 00-monument-theme.md
- [ ] 00-signal-flow-overview.md
- [ ] 03-chambers.md
- [ ] 07-routing-graph.md

### Phase 2: Core Signal Flow
- [ ] 01-foundation.md
- [ ] 02-pillars.md
- [ ] 04-weathering.md
- [ ] 06-facade.md
- [ ] 05-buttress.md

### Phase 3: Advanced Features
- [ ] 08-resonance.md
- [ ] 09-living-stone.md
- [ ] 10-impossible-geometry.md
- [ ] 11-strata.md

### Phase 4: Supporting Systems
- [ ] 12-parameter-buffers.md
- [ ] 13-spatial-processor.md
- [ ] 14-allpass-diffuser.md
- [ ] 15-modulation-sources.md

### Phase 5: Visual Assets
- [ ] 50+ diagrams, charts, infographics
- [ ] Brutalist design aesthetic

### Phase 6: Code Refactoring
- [ ] Rename physical modeling classes
- [ ] Rename MemoryEchoes → Strata
- [ ] Integrate Strata into routing graph
- [ ] Add Strata to ModulationMatrix

---

## Related Documentation

### Project Root Documentation
- `README.md` - Project overview and Monument philosophy
- `docs/DSP_ARCHITECTURE_COMPREHENSIVE_REVIEW.md` - Existing architecture analysis
- `docs/MEMORY_ECHOES_INTEGRATION.md` - Strata integration proposal
- `docs/PERFORMANCE_BASELINE.md` - Performance metrics and benchmarks
- `docs/TESTING_GUIDE.md` - Test suite documentation
- `docs/BUILD_PATTERNS.md` - Build system and CMake patterns

### Source Code Reference
- `dsp/DspModules.h` - Core module class definitions
- `dsp/Chambers.{h,cpp}` - FDN reverb core implementation
- `dsp/DspRoutingGraph.{h,cpp}` - Signal routing and orchestration
- `dsp/MemoryEchoes.{h,cpp}` - Memory system (to be renamed Strata)
- `plugin/PluginProcessor.{h,cpp}` - Plugin integration and processBlock

---

## Contributing to Documentation

### Quality Gates

Before publishing module documentation:
- [ ] All LaTeX equations render correctly
- [ ] Code examples compile without errors
- [ ] Cross-references resolve correctly
- [ ] Visual assets load properly (no broken links)
- [ ] Monument theme is consistent throughout
- [ ] Technical accuracy verified against source code
- [ ] Performance metrics match benchmark data
- [ ] Spelling and grammar checked
- [ ] Brutalist aesthetic maintained in visuals

### Visual Asset Standards

**Color Palette (Brutalist):**
- Stone Gray: `#A0A0A0`
- Concrete White: `#F0F0F0`
- Steel Blue: `#4A90E2`
- Accent Red: `#E74C3C` (errors/warnings)
- Accent Green: `#27AE60` (success/passing)

**Typography:**
- Headings: Bold sans-serif (Monument Grotesk, Roboto)
- Body: Regular sans-serif (Inter, Open Sans)
- Code: Monospace (Roboto Mono, Fira Code)

**Diagram Standards:**
- SVG format (vector, scalable)
- 1200px width maximum
- Clean geometric shapes (rectangles, arrows)
- Grid background for blueprint aesthetic

---

## Credits

**DSP Architecture:** Monument Development Team
**Documentation:** AI-assisted technical writing
**Visual Assets:** Brutalist design aesthetic
**Monument Philosophy:** "A structure sound enters, not a space that reflects sound"

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-01-09 | Initial documentation structure, Phase 1 foundation |

---

**Next:** [Monument Theme](00-monument-theme.md) - Explore the architectural metaphor
