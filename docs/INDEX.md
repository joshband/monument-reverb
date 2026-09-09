# Monument Reverb - Documentation Index

> **Central Hub**: Navigate all project documentation from here. Each
> category below links to a canonical hub doc where one exists — start
> there rather than assuming this index enumerates every file.

---

## Quick Start

### For Developers
1. [README.md](../README.md) - Project overview & setup
2. [STANDARD_BUILD_WORKFLOW.md](../STANDARD_BUILD_WORKFLOW.md) - Build commands & workflow
3. [TESTING.md](../TESTING.md) - Canonical CI/QA entrypoint

### For Contributors
1. [CONTRIBUTING.md](../CONTRIBUTING.md) - Contribution guidelines
2. [CHANGELOG.md](../CHANGELOG.md) - Version history
3. [AGENTS.md](../AGENTS.md) - Repository operating contract (for AI agents and humans alike)

---

## Architecture & Design

- [ARCHITECTURE.md](../ARCHITECTURE.md) - **Start here** - system architecture, reachable signal chain, macro/modulation control
- [architecture/README.md](architecture/README.md) - Architecture docs hub (DSP deep dives, quick reference)
- [architecture/ARCHITECTURE_QUICK_REFERENCE.md](architecture/ARCHITECTURE_QUICK_REFERENCE.md) - Visual diagrams & fast navigation
- [MANIFEST.md](../MANIFEST.md) - Project manifesto & sonic vision
- [DSP_SIGNAL_FLOW_BASICS.md](DSP_SIGNAL_FLOW_BASICS.md) - DSP signal flow basics
- [architecture/PARAMETER_BEHAVIOR.md](architecture/PARAMETER_BEHAVIOR.md) - Parameter behavior documentation
- [architecture/EDITOR_PARITY_FINDINGS.md](architecture/EDITOR_PARITY_FINDINGS.md) - What the shipping editor actually exposes vs. the host parameter surface

### DSP Module Documentation

- [architecture/dsp/00-index.md](architecture/dsp/00-index.md) - DSP documentation index
- [architecture/dsp/00-signal-flow-overview.md](architecture/dsp/00-signal-flow-overview.md) - Signal flow overview
- [architecture/dsp/core-modules/](architecture/dsp/core-modules/) - Foundation, Pillars, Chambers, Weathering, Buttress, Facade, routing graph
- [architecture/dsp/physical-modeling/](architecture/dsp/physical-modeling/) - Resonance, Living Stone, Impossible Geometry
- [architecture/dsp/memory-system/](architecture/dsp/memory-system/) - Strata (MemoryEchoes)
- [architecture/dsp/supporting-systems/](architecture/dsp/supporting-systems/) - Parameter Buffers, Spatial Processor, Allpass Diffuser, Modulation Sources
- [architecture/dsp/control-systems/00-ancient-monuments.md](architecture/dsp/control-systems/00-ancient-monuments.md) - Macro control system

---

## UI Design

- [ui/README.md](ui/README.md) - UI docs hub (flags which sections below it are superseded)
- [ui/UI_DESIGN_HISTORY.md](ui/UI_DESIGN_HISTORY.md) - How the UI direction got from the original macro-only plan to what ships today
- [ui/design-references/README.md](ui/design-references/README.md) - Visual design references

For the superseded macro-only UI roadmap (strategic plan, layered knob design/workflow, photorealistic implementation plan) see [archive/ui/](archive/ui/) — `UI_DESIGN_HISTORY.md` above is the short version of why it changed.

---

## Presets

- [PRESET_GALLERY.md](PRESET_GALLERY.md) - Preset showcase
- [presets/PRESET_FORMAT.md](presets/PRESET_FORMAT.md) - User preset JSON file format specification

---

## Development Guides

- [development/README.md](development/README.md) - Development hub: build workflows, coding standards, real-time safety rules, asset management

---

## Testing & Validation

- [TESTING.md](../TESTING.md) - **Canonical hub** (repo root) - DSP QA authority policy, quick start, CI/CD architecture, tooling catalog
- [testing/README.md](testing/README.md) - Deep-dive index (points back to the canonical hub)
- [testing/TESTING_GUIDE.md](testing/TESTING_GUIDE.md) - Audio regression workflow
- [testing/UI_TESTING.md](testing/UI_TESTING.md) - UI capture + visual regression
- [testing/TESTING.md](testing/TESTING.md) - Pluginval and manual validation checks
- [testing/MODULATION_TESTING_GUIDE.md](testing/MODULATION_TESTING_GUIDE.md) - Modulation system test coverage
- [testing/STRESS_TEST_PLAN.md](testing/STRESS_TEST_PLAN.md) - Long-form stress testing plan
- [testing/PARAMETER_STRESS_RESULTS.md](testing/PARAMETER_STRESS_RESULTS.md) - Parameter stress test results
- [CTEST_FAILURES.md](CTEST_FAILURES.md) - Current CTest pass/fail state, with root causes for known failures

### Test Schemas

- [schemas/README.md](schemas/README.md) - Schema catalog
- [schemas/test_output_schemas.md](schemas/test_output_schemas.md) - Test output schema definitions

---

## Historical Documentation (Archive)

Superseded docs are archived rather than deleted, so the reasoning behind past
decisions stays traceable. Each cluster below has an index or summary that's
worth reading before diving into individual files.

- [archive/reviews/INDEX.md](archive/reviews/INDEX.md) - **Timeline** of every archived code/architecture/DSP review and audit (2026-01-03 through 01-09), with what each concluded
- [ui/UI_DESIGN_HISTORY.md](ui/UI_DESIGN_HISTORY.md) - Summary of the UI pivot away from the docs in [archive/ui/](archive/ui/)
- [archive/sessions/](archive/sessions/) - Session summaries, status snapshots, and quick-fix logs from earlier development phases (includes [archive/sessions/experimental/](archive/sessions/experimental/), an abandoned experimental-preset redesign)
- [archive/phases/](archive/phases/) - Phase completion summaries (Phases 1-4)
- [archive/plans/](archive/plans/) - Completed implementation plans (e.g. the QA-truth/RT-characterization work)
- [archive/NEXT_SESSION_*.md](archive/) - Loose historical session-handoff notes

---

## Project Organization

### Root Documentation (Essential Files Only)

```
monument-reverb/
├── README.md                           # Project overview
├── ARCHITECTURE.md                     # System architecture
├── TESTING.md                          # Canonical CI/QA hub
├── ROADMAP.md                          # Project roadmap
├── CHANGELOG.md                        # Version history
├── CONTRIBUTING.md                     # Contribution guide
├── MANIFEST.md                         # Project manifesto
├── STANDARD_BUILD_WORKFLOW.md          # Build commands
├── AGENTS.md                           # Repository operating contract
├── CLAUDE.md                           # Project instructions
└── LICENSE                             # License file
```

### Documentation Tree

```
docs/
├── INDEX.md (this file)                # Central navigation hub
├── DSP_SIGNAL_FLOW_BASICS.md
├── CTEST_FAILURES.md
├── PRESET_GALLERY.md
│
├── architecture/                       # Technical architecture
│   ├── README.md
│   ├── ARCHITECTURE_QUICK_REFERENCE.md
│   ├── PARAMETER_BEHAVIOR.md
│   ├── EDITOR_PARITY_FINDINGS.md
│   ├── ADVANCED_FEATURES.md
│   ├── DSP_ARCHITECTURE.md
│   └── dsp/                            # DSP module docs, by category
│       ├── core-modules/
│       ├── physical-modeling/
│       ├── memory-system/
│       ├── supporting-systems/
│       └── control-systems/
│
├── development/                        # Development guide hub
│   └── README.md
│
├── testing/                             # Testing & validation
│   ├── README.md
│   ├── TESTING_GUIDE.md
│   ├── TESTING.md
│   ├── UI_TESTING.md
│   ├── MODULATION_TESTING_GUIDE.md
│   ├── STRESS_TEST_PLAN.md
│   └── PARAMETER_STRESS_RESULTS.md
│
├── ui/                                  # UI design & strategy
│   ├── README.md
│   ├── UI_DESIGN_HISTORY.md
│   └── design-references/
│
├── presets/                             # Preset formats
│   └── PRESET_FORMAT.md
│
├── schemas/                             # Schema definitions
│   ├── README.md
│   └── test_output_schemas.md
│
└── archive/                             # Historical docs (see above)
    ├── reviews/                         # Dated reviews & audits, with INDEX.md timeline
    ├── phases/                          # Phase completions
    ├── sessions/                        # Session summaries (+ experimental/ subcluster)
    ├── plans/                           # Completed implementation plans
    ├── ui/                              # Superseded UI roadmap docs
    └── NEXT_SESSION_*.md                # Loose handoff notes
```

---

## Learning Paths

### Path 1: New Developer Onboarding

1. [README.md](../README.md) - Understand project goals
2. [ARCHITECTURE.md](../ARCHITECTURE.md) - Learn system design
3. [STANDARD_BUILD_WORKFLOW.md](../STANDARD_BUILD_WORKFLOW.md) - Build the plugin
4. [development/README.md](development/README.md) - Development workflows and coding standards
5. [CONTRIBUTING.md](../CONTRIBUTING.md) - Start contributing

### Path 2: UI/UX Designer

1. [ui/README.md](ui/README.md) - UI docs hub
2. [ui/UI_DESIGN_HISTORY.md](ui/UI_DESIGN_HISTORY.md) - How the current direction was reached
3. [ARCHITECTURE.md](../ARCHITECTURE.md) - "UI ownership" section for what ships today
4. [architecture/EDITOR_PARITY_FINDINGS.md](architecture/EDITOR_PARITY_FINDINGS.md) - Editor vs. host parameter surface

### Path 3: DSP Engineer

1. [ARCHITECTURE.md](../ARCHITECTURE.md) - System overview
2. [architecture/ARCHITECTURE_QUICK_REFERENCE.md](architecture/ARCHITECTURE_QUICK_REFERENCE.md) - DSP chain diagrams
3. [architecture/dsp/00-index.md](architecture/dsp/00-index.md) - DSP module index
4. [architecture/PARAMETER_BEHAVIOR.md](architecture/PARAMETER_BEHAVIOR.md) - Parameter behavior
5. [CTEST_FAILURES.md](CTEST_FAILURES.md) - Known test failures and root causes

### Path 4: QA/Testing

1. [TESTING.md](../TESTING.md) - Canonical CI/QA hub
2. [testing/README.md](testing/README.md) - Deep-dive index
3. [testing/TESTING_GUIDE.md](testing/TESTING_GUIDE.md) - Audio regression workflow
4. [testing/MODULATION_TESTING_GUIDE.md](testing/MODULATION_TESTING_GUIDE.md) - Modulation testing
5. [testing/STRESS_TEST_PLAN.md](testing/STRESS_TEST_PLAN.md) - Stress testing

---

## Contributing to Documentation

When adding new documentation:

1. **Root-level docs** - Only for essential project files:
   - README.md, ARCHITECTURE.md, TESTING.md, ROADMAP.md, CHANGELOG.md, CONTRIBUTING.md
   - MANIFEST.md, AGENTS.md, CLAUDE.md, STANDARD_BUILD_WORKFLOW.md
   - LICENSE

2. **Categorized docs** - Place in the appropriate subdirectory:
   - `docs/ui/` - UI/UX design documents
   - `docs/development/` - Build workflows, coding standards, guides
   - `docs/architecture/` - Technical architecture and DSP module docs
   - `docs/testing/` - Test plans, validation, results
   - `docs/archive/` - Historical documents only, once superseded

3. **Update this index** - Add new docs to the relevant section above.

4. **Cross-reference** - Link related docs together, and prefer linking to a
   category's hub README over duplicating its content here.

---

## Support & Contact

- Issues: See [CONTRIBUTING.md](../CONTRIBUTING.md)
- Architecture questions: See [ARCHITECTURE.md](../ARCHITECTURE.md)
- Build problems: See [STANDARD_BUILD_WORKFLOW.md](../STANDARD_BUILD_WORKFLOW.md)
- Testing/CI questions: See [TESTING.md](../TESTING.md)
