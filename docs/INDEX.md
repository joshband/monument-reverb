# Monument Reverb - Documentation Index

> **Central Hub**: Navigate all project documentation from here

---

## Quick Start

### For Developers
1. [README.md](../README.md) - Project overview & setup
2. [STANDARD_BUILD_WORKFLOW.md](../STANDARD_BUILD_WORKFLOW.md) - Build commands & workflow
3. [development/QUICK_START_BLENDER_KNOBS.md](development/QUICK_START_BLENDER_KNOBS.md) - Generate UI knobs

### For Contributors
1. [CONTRIBUTING.md](../CONTRIBUTING.md) - Contribution guidelines
2. [CHANGELOG.md](../CHANGELOG.md) - Version history
3. [AGENTS.md](../AGENTS.md) - AI agent documentation

---

## Architecture & Design

### System Architecture
- [ARCHITECTURE.md](../ARCHITECTURE.md) - **Start Here** - Main architecture overview
- [ARCHITECTURE_QUICK_REFERENCE.md](../ARCHITECTURE_QUICK_REFERENCE.md) - Visual diagrams & fast navigation
- [architecture/ARCHITECTURE_REVIEW.md](architecture/ARCHITECTURE_REVIEW.md) - Detailed architectural analysis
- [MANIFEST.md](../MANIFEST.md) - Project manifest & vision

### Technical Deep Dives
- [architecture/DSP_CLICK_ANALYSIS_REPORT.md](architecture/DSP_CLICK_ANALYSIS_REPORT.md) - DSP debugging & click analysis

---

## UI Design

### Design System
- [ui/LAYERED_KNOB_DESIGN.md](ui/LAYERED_KNOB_DESIGN.md) - Layered knob design principles
- [ui/LAYERED_KNOB_WORKFLOW.md](ui/LAYERED_KNOB_WORKFLOW.md) - Asset generation pipeline (Blender + Midjourney)
- [ui/MONUMENT_UI_STRATEGIC_DESIGN_PLAN.md](ui/MONUMENT_UI_STRATEGIC_DESIGN_PLAN.md) - Overall UI strategy

### Recent Handoffs
- [ui/MVP_UI_HANDOFF_2026_01_03.md](ui/MVP_UI_HANDOFF_2026_01_03.md) - Latest UI implementation status (2026-01-03)

---

## Development Guides

### Quick Starts
- [development/QUICK_START_BLENDER_KNOBS.md](development/QUICK_START_BLENDER_KNOBS.md) - Generate photorealistic knobs (5 min)
- [development/QUICK_START_MACRO_TESTING.md](development/QUICK_START_MACRO_TESTING.md) - Test macro control system
- [STANDARD_BUILD_WORKFLOW.md](../STANDARD_BUILD_WORKFLOW.md) - Build system commands

---

## Testing & Validation

### Testing Guides
- [testing/MODULATION_TESTING_GUIDE.md](testing/MODULATION_TESTING_GUIDE.md) - Test modulation system
- [testing/PHASE_2_VALIDATION_TEST.md](testing/PHASE_2_VALIDATION_TEST.md) - Phase 2 memory system validation
- [testing/PHASE_3_COMPLETE_SUMMARY.md](testing/PHASE_3_COMPLETE_SUMMARY.md) - Phase 3 modulation complete

---

## Project Organization

### Root Documentation (Always in Root)
```
monument-reverb/
├── README.md                           # Project overview
├── ARCHITECTURE.md                     # System architecture (NEW)
├── ARCHITECTURE_QUICK_REFERENCE.md     # Visual reference
├── STANDARD_BUILD_WORKFLOW.md          # Build commands
├── CHANGELOG.md                        # Version history
├── CONTRIBUTING.md                     # Contribution guide
├── MANIFEST.md                         # Project manifest
└── AGENTS.md                           # AI agent docs
```

### Documentation Tree
```
docs/
├── INDEX.md (this file)                # Central navigation hub
├── ui/                                 # UI design & strategy
│   ├── LAYERED_KNOB_DESIGN.md
│   ├── LAYERED_KNOB_WORKFLOW.md
│   ├── MONUMENT_UI_STRATEGIC_DESIGN_PLAN.md
│   └── MVP_UI_HANDOFF_2026_01_03.md
├── development/                        # Development guides
│   ├── QUICK_START_BLENDER_KNOBS.md
│   └── QUICK_START_MACRO_TESTING.md
├── architecture/                       # Technical architecture
│   ├── ARCHITECTURE_REVIEW.md
│   └── DSP_CLICK_ANALYSIS_REPORT.md
└── testing/                            # Testing & validation
    ├── MODULATION_TESTING_GUIDE.md
    ├── PHASE_2_VALIDATION_TEST.md
    └── PHASE_3_COMPLETE_SUMMARY.md
```

---

## Learning Paths

### Path 1: New Developer Onboarding
1. [README.md](../README.md) - Understand project goals
2. [ARCHITECTURE.md](../ARCHITECTURE.md) - Learn system design
3. [STANDARD_BUILD_WORKFLOW.md](../STANDARD_BUILD_WORKFLOW.md) - Build the plugin
4. [development/QUICK_START_BLENDER_KNOBS.md](development/QUICK_START_BLENDER_KNOBS.md) - Generate assets
5. [CONTRIBUTING.md](../CONTRIBUTING.md) - Start contributing

### Path 2: UI/UX Designer
1. [ui/MONUMENT_UI_STRATEGIC_DESIGN_PLAN.md](ui/MONUMENT_UI_STRATEGIC_DESIGN_PLAN.md) - UI strategy
2. [ui/LAYERED_KNOB_DESIGN.md](ui/LAYERED_KNOB_DESIGN.md) - Design system
3. [ui/LAYERED_KNOB_WORKFLOW.md](ui/LAYERED_KNOB_WORKFLOW.md) - Asset pipeline
4. [development/QUICK_START_BLENDER_KNOBS.md](development/QUICK_START_BLENDER_KNOBS.md) - Generate assets

### Path 3: DSP Engineer
1. [ARCHITECTURE.md](../ARCHITECTURE.md) - System overview
2. [ARCHITECTURE_QUICK_REFERENCE.md](../ARCHITECTURE_QUICK_REFERENCE.md) - DSP chain diagrams
3. [architecture/ARCHITECTURE_REVIEW.md](architecture/ARCHITECTURE_REVIEW.md) - Deep dive
4. [architecture/DSP_CLICK_ANALYSIS_REPORT.md](architecture/DSP_CLICK_ANALYSIS_REPORT.md) - Debugging techniques
5. [testing/MODULATION_TESTING_GUIDE.md](testing/MODULATION_TESTING_GUIDE.md) - Test approaches

### Path 4: QA/Testing
1. [testing/MODULATION_TESTING_GUIDE.md](testing/MODULATION_TESTING_GUIDE.md) - Modulation testing
2. [testing/PHASE_2_VALIDATION_TEST.md](testing/PHASE_2_VALIDATION_TEST.md) - Memory system tests
3. [testing/PHASE_3_COMPLETE_SUMMARY.md](testing/PHASE_3_COMPLETE_SUMMARY.md) - Phase 3 results
4. [development/QUICK_START_MACRO_TESTING.md](development/QUICK_START_MACRO_TESTING.md) - Macro testing

---

## Document Status

### Up to Date ✅
- Root docs (README, ARCHITECTURE, CHANGELOG, etc.)
- [development/QUICK_START_BLENDER_KNOBS.md](development/QUICK_START_BLENDER_KNOBS.md) - Updated 2026-01-03 with LayeredKnob fix
- [STANDARD_BUILD_WORKFLOW.md](../STANDARD_BUILD_WORKFLOW.md) - Updated 2026-01-03

### Needs Review 🔍
- [ui/MVP_UI_HANDOFF_2026_01_03.md](ui/MVP_UI_HANDOFF_2026_01_03.md) - Check if still relevant after recent fixes

### Historical 📦
- [testing/PHASE_2_VALIDATION_TEST.md](testing/PHASE_2_VALIDATION_TEST.md) - Phase 2 complete
- [testing/PHASE_3_COMPLETE_SUMMARY.md](testing/PHASE_3_COMPLETE_SUMMARY.md) - Phase 3 complete

---

## Contributing to Documentation

When adding new documentation:

1. **Root-level docs** - Only for:
   - README.md
   - ARCHITECTURE.md
   - CHANGELOG.md
   - CONTRIBUTING.md
   - MANIFEST.md
   - AGENTS.md
   - STANDARD_BUILD_WORKFLOW.md
   - ARCHITECTURE_QUICK_REFERENCE.md (exception for visual reference)

2. **Categorized docs** - Place in appropriate subdirectory:
   - `docs/ui/` - UI/UX design documents
   - `docs/development/` - Quick starts, guides, tutorials
   - `docs/architecture/` - Technical architecture, reviews, analyses
   - `docs/testing/` - Test plans, validation, phase summaries

3. **Update this index** - Add new docs to relevant sections

4. **Cross-reference** - Link related docs together

---

## Support & Contact

- Issues: See [CONTRIBUTING.md](../CONTRIBUTING.md)
- Architecture questions: See [ARCHITECTURE.md](../ARCHITECTURE.md)
- Build problems: See [STANDARD_BUILD_WORKFLOW.md](../STANDARD_BUILD_WORKFLOW.md)
