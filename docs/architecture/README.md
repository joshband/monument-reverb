# Architecture Documentation

Comprehensive architecture documentation for Monument Reverb's DSP system and implementation patterns.

## Quick Navigation

### Overview Documents
- [ARCHITECTURE_REVIEW.md](ARCHITECTURE_REVIEW.md) - Complete architecture analysis and review
- [ARCHITECTURE_QUICK_REFERENCE.md](ARCHITECTURE_QUICK_REFERENCE.md) - Quick reference guide (also in repo root)
- [IMPLEMENTATION_GUIDE.md](IMPLEMENTATION_GUIDE.md) - Implementation patterns and guidelines

### DSP Architecture (Complete Module Documentation)

All 17 DSP modules fully documented in [dsp/](dsp/) subfolder:

**Core Modules:** (6 modules)
- 00: [Routing Graph Overview](dsp/overview/00-routing-graph.md)
- 01: [Foundation](dsp/core-modules/01-foundation.md) - Input stage
- 02: [Pillars](dsp/core-modules/02-pillars.md) - Early reflections
- 03: [Chambers](dsp/core-modules/03-chambers.md) - FDN reverb core (*to be extracted*)
- 04: [Weathering](dsp/core-modules/04-weathering.md) - LFO modulation
- 05: [Buttress](dsp/core-modules/05-buttress.md) - Feedback safety
- 06: [Facade](dsp/core-modules/06-facade.md) - Output stage
- 07: [Ancient Monuments](dsp/control-systems/00-ancient-monuments.md) - Macro system (*to be created*)

**Physical Modeling:** (3 modules)
- 08: [Resonance](dsp/physical-modeling/08-resonance.md) - TubeRayTracer
- 09: [Living Stone](dsp/physical-modeling/09-living-stone.md) - ElasticHallway
- 10: [Impossible Geometry](dsp/physical-modeling/10-impossible-geometry.md) - AlienAmplification

**Memory System:** (1 module)
- 11: [Strata](dsp/memory-system/11-strata.md) - MemoryEchoes

**Supporting Systems:** (4 modules)
- 12: [Parameter Buffers](dsp/supporting-systems/12-parameter-buffers.md) - Zipper noise elimination
- 13: [Spatial Processor](dsp/supporting-systems/13-spatial-processor.md) - 3D positioning
- 14: [Allpass Diffuser](dsp/supporting-systems/14-allpass-diffuser.md) - Network topology
- 15: [Modulation Sources](dsp/supporting-systems/15-modulation-sources.md) - ModulationMatrix

**Control Systems:** (1 module)
- 00: [Ancient Monuments](dsp/control-systems/00-ancient-monuments.md) - Thematic macro controls

### Real-Time Safety & Performance
- [DSP_REALTIME_SAFETY_AUDIT.md](DSP_REALTIME_SAFETY_AUDIT.md) - RT-safety audit
- [DSP_REALTIME_SAFETY_FIX_PLAN.md](DSP_REALTIME_SAFETY_FIX_PLAN.md) - Fix plan for RT issues
- [DSP_CLICK_ANALYSIS_REPORT.md](DSP_CLICK_ANALYSIS_REPORT.md) - Audio glitch analysis
- [PARAMETER_BEHAVIOR.md](PARAMETER_BEHAVIOR.md) - Parameter smoothing and behavior

### Experimental & Design
- [EXPERIMENTAL_REDESIGN.md](EXPERIMENTAL_REDESIGN.md) - Experimental features and designs
- [COMPLETE_EXPERIMENTAL_REDESIGN_PLAN.md](COMPLETE_EXPERIMENTAL_REDESIGN_PLAN.md) - Complete redesign proposals

## Documentation Stats

**Total:** ~200,000 words across 17 comprehensive modules
**Lines:** ~18,000+ lines of technical documentation
**Visual Elements:**
- 48+ Mermaid diagrams
- 217+ LaTeX equations
- 173+ code examples

## Contributing

When creating or updating architecture documentation:

1. **Follow the 12-section template:**
   - Overview, Monument Metaphor, Architecture, DSP Theory, Implementation, Performance, Parameters, Usage Examples, Integration, Test Coverage, Future Enhancements, References

2. **Include visual elements:**
   - Mermaid diagrams for signal flow
   - LaTeX equations for mathematical foundations
   - Code examples for implementation patterns

3. **Maintain quality standards:**
   - All diagrams must render correctly
   - Cross-references must resolve
   - Monument theme must be consistent
   - Technical accuracy verified against source code
   - Performance metrics documented with benchmarks

## Related Documentation

- [../README.md](../../README.md) - Project overview
- [../testing/README.md](../testing/README.md) - Testing documentation
- [../ui/README.md](../ui/README.md) - UI documentation
- [../development/README.md](../development/README.md) - Development guides
