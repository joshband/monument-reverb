# Documentation Reorganization Plan

**Date:** 2026-01-09
**Objective:** Systematically clean up, archive, and reorganize all project documentation for optimal clarity and maintainability.

---

## Current State Analysis

### Root Directory (21 markdown files)

**Core Documentation (Keep in Root):**

- ✅ `README.md` (386 lines) - Project overview
- ✅ `ARCHITECTURE.md` (167 lines) - High-level architecture
- ✅ `ARCHITECTURE_QUICK_REFERENCE.md` (439 lines) - Quick reference guide
- ✅ `CHANGELOG.md` (850 lines) - Version history + session summaries
- ✅ `CONTRIBUTING.md` (48 lines) - Contribution guidelines
- ✅ `MANIFEST.md` (60 lines) - Project manifest
- ✅ `NEXT_SESSION_HANDOFF.md` (144 lines) - Current session status
- ✅ `STANDARD_BUILD_WORKFLOW.md` (131 lines) - Build instructions
- ✅ `AGENTS.md` - Agent configurations (DO NOT MODIFY)
- ✅ `CLAUDE.md` - Project instructions (DO NOT MODIFY)
- ✅ `LICENSE` - License file (DO NOT MODIFY)

**Files to Archive (10 files):**

1. **Dated Session Files → docs/archive/sessions/**
   - `01072026-ArchitectureReview.md` (76 lines)
   - `01072026-CodeReview.md` (56 lines)
   - `01072026-Performance.md` (58 lines)
   - `01082026-ArchitectureReview.md` (177 lines)
   - `01082026-CodeReview.md` (373 lines)
   - `01082026-Performance.md` (434 lines)
   - `SESSION_17_SUMMARY.md` (110 lines)

2. **Implementation/Planning Docs → docs/archive/planning/**
   - `ImplementationPlan.md` (133 lines)
   - `Roadmap.md` (estimate: ~50 lines)

3. **Optimization Docs → docs/development/**
   - `CLAUDE_MD_OPTIMIZATION_RESULTS.md` (225 lines)
   - `TOKEN_OPTIMIZATION_STRATEGIES.md` (283 lines)

---

## docs/ Folder Analysis (176 markdown files)

### Files to Archive from docs/ Root

**Phase Completion Docs → docs/archive/phases/**

- `PHASE_1_CONSOLIDATION_COMPLETE.md`
- `PHASE_2_BASELINE_VALIDATION_COMPLETE.md`
- `PHASE_3_COMPLETE.md`
- `PHASE_3_STEP_1_SCHEMAS_COMPLETE.md`
- `PHASE_3_STEP_2_QUALITY_GATES_COMPLETE.md`
- `PHASE_3_TEST_PLAN.md`
- `PHASE_4_COMPLETE_SUMMARY.md`
- `PHASE_4_DESIGN.md`

**Session-Specific Docs → docs/archive/sessions/**

- `SESSION_2026_01_08_PHASE_3_4_COMPLETE.md`
- `UI_FULL_RESET_2026_01_08.md`
- `UI_RESET_2026_01_08.md`

### Redundant Files to Consolidate

**Architecture Documentation:**

- `docs/ARCHITECTURE_REVIEW.md` (829 lines) - **MERGE** into `docs/architecture/ARCHITECTURE_REVIEW.md` (1,465 lines)
- `docs/architecture/DSP_ARCHITECTURE.md` (765 lines) - **REVIEW** against comprehensive DSP module docs

**Testing Documentation:**

- `docs/testing/TESTING_GUIDE.md` → Link from `TESTING.md` / `docs/testing/README.md`
- `docs/testing_audit.md` (1,203 lines) → Move to `docs/testing/TESTING_AUDIT.md`
- `docs/test_output_schemas.md` (737 lines) → Move to `docs/schemas/test_output_schemas.md`

**UI Documentation (docs/ui/ - 11 files):**

- Consolidate multiple roadmap/plan files:
  - `UI_UX_ROADMAP.md` (1,278 lines)
  - `MONUMENT_UI_STRATEGIC_DESIGN_PLAN.md` (765 lines)
  - `PHOTOREALISTIC_UI_IMPLEMENTATION_PLAN.md` (621 lines)
  - `PHOTOREALISTIC_UI_PROGRESS.md`
- **Action:** Create single `docs/ui/UI_MASTER_PLAN.md` with current state + roadmap

---

## Proposed New Structure

### Root Directory (Final: 11 files)

```
monument-reverb/
├── README.md                            # Project overview
├── ARCHITECTURE.md                      # High-level architecture
├── ARCHITECTURE_QUICK_REFERENCE.md      # Quick reference
├── CHANGELOG.md                         # Version history
├── CONTRIBUTING.md                      # Contribution guidelines
├── MANIFEST.md                          # Project manifest
├── NEXT_SESSION_HANDOFF.md              # Session status
├── STANDARD_BUILD_WORKFLOW.md           # Build workflow
├── AGENTS.md                            # (No changes)
├── CLAUDE.md                            # (No changes)
└── LICENSE                              # (No changes)
```

### docs/ Folder Structure (Reorganized)

```
docs/
├── INDEX.md                             # Master documentation index
├── PERFORMANCE_BASELINE.md              # Performance metrics
├── DSP_ARCHITECTURE_COMPREHENSIVE_REVIEW.md  # Complete DSP analysis
├── EXPERIMENTAL_PRESETS.md              # Preset documentation
├── EXPERIMENTAL_PRESETS_ENHANCEMENT_PATCHES.md
├── PRESET_GALLERY.md
├── QUALITY_GATES.md
├── QUICK_FIXES_SUMMARY.md
├── STATUS.md                            # Current project status
├── Monument_User_Manual.html            # User manual
│
├── architecture/
│   ├── README.md                        # Architecture overview
│   ├── ARCHITECTURE_REVIEW.md           # (Consolidated)
│   ├── COMPLETE_EXPERIMENTAL_REDESIGN_PLAN.md
│   ├── DSP_CLICK_ANALYSIS_REPORT.md
│   ├── DSP_REALTIME_SAFETY_AUDIT.md
│   ├── DSP_REALTIME_SAFETY_FIX_PLAN.md
│   ├── EXPERIMENTAL_REDESIGN.md
│   ├── IMPLEMENTATION_GUIDE.md
│   ├── PARAMETER_BEHAVIOR.md
│   └── dsp/                             # Complete DSP module docs (11 modules)
│       ├── 00-index.md
│       ├── 00-monument-theme.md
│       ├── 00-signal-flow-overview.md
│       ├── core-modules/                # Foundation, Pillars, Chambers, etc.
│       ├── physical-modeling/           # Resonance, Living Stone, etc.
│       ├── memory-system/               # Strata (MemoryEchoes)
│       └── supporting-systems/          # (To be created in Phase 4)
│
├── development/
│   ├── README.md                        # Development guide
│   ├── QUICK_START_BLENDER_KNOBS.md
│   ├── QUICK_START_MACRO_TESTING.md
│   ├── CLAUDE_MD_OPTIMIZATION_RESULTS.md    # (Moved from root)
│   ├── TOKEN_OPTIMIZATION_STRATEGIES.md     # (Moved from root)
│   └── BUILD_PATTERNS.md
│
├── testing/
│   ├── README.md                        # Testing overview
│   ├── TESTING.md                       # (Consolidated from root TESTING_GUIDE.md)
│   ├── TESTING_AUDIT.md                 # (Moved from docs/testing_audit.md)
│   ├── MODULATION_TESTING_GUIDE.md
│   ├── PARAMETER_STRESS_RESULTS.md      # (Moved from docs/)
│   ├── STRESS_TEST_PLAN.md              # (Moved from docs/)
│   └── Monument_v1.0_Pre-Memory_Validation.md
│
├── ui/
│   ├── README.md                        # UI overview
│   ├── UI_MASTER_PLAN.md                # (Consolidated roadmap + plans)
│   ├── ENHANCED_UI_SUMMARY.md
│   ├── JUCE_BLEND_MODES_RESEARCH.md
│   ├── LAYERED_KNOB_WORKFLOW.md
│   └── design-references/               # (Keep as-is)
│
├── presets/
│   └── PRESET_FORMAT.md
│
├── schemas/
│   ├── README.md
│   ├── test_output_schemas.md           # (Moved from docs/)
│   └── *.schema.json
│
├── codex/                               # (Keep as-is)
│   └── *.md
│
├── archive/
│   ├── README.md                        # Archive index
│   ├── sessions/                        # Historical session summaries
│   │   ├── 2026-01-07/
│   │   │   ├── ArchitectureReview.md
│   │   │   ├── CodeReview.md
│   │   │   └── Performance.md
│   │   ├── 2026-01-08/
│   │   │   ├── ArchitectureReview.md
│   │   │   ├── CodeReview.md
│   │   │   └── Performance.md
│   │   ├── SESSION_17_SUMMARY.md
│   │   ├── SESSION_2026_01_08_PHASE_3_4_COMPLETE.md
│   │   └── experimental/                # (Moved from docs/experimental/)
│   ├── phases/                          # Phase completion summaries
│   │   ├── PHASE_1_CONSOLIDATION_COMPLETE.md
│   │   ├── PHASE_2_BASELINE_VALIDATION_COMPLETE.md
│   │   ├── PHASE_3_COMPLETE.md
│   │   ├── PHASE_3_STEP_1_SCHEMAS_COMPLETE.md
│   │   ├── PHASE_3_STEP_2_QUALITY_GATES_COMPLETE.md
│   │   ├── PHASE_3_TEST_PLAN.md
│   │   ├── PHASE_4_COMPLETE_SUMMARY.md
│   │   └── PHASE_4_DESIGN.md
│   ├── planning/                        # Historical planning docs
│   │   ├── ImplementationPlan.md
│   │   └── Roadmap.md
│   └── ui/                              # UI reset history
│       ├── UI_FULL_RESET_2026_01_08.md
│       └── UI_RESET_2026_01_08.md
│
└── sessions/                            # (Keep as-is - recent sessions)
    └── *.md
```

---

## Consolidation Actions

### 1. Merge Redundant Architecture Files

**Target:** `docs/architecture/ARCHITECTURE_REVIEW.md`

- Keep the comprehensive 1,465-line version in `docs/architecture/`
- Archive the 829-line version from `docs/` root
- Ensure no unique content is lost

### 2. Consolidate UI Documentation

**Create:** `docs/ui/UI_MASTER_PLAN.md`

**Merge these files:**

- `UI_UX_ROADMAP.md` (current roadmap)
- `MONUMENT_UI_STRATEGIC_DESIGN_PLAN.md` (strategic overview)
- `PHOTOREALISTIC_UI_IMPLEMENTATION_PLAN.md` (implementation details)
- `PHOTOREALISTIC_UI_PROGRESS.md` (progress tracking)

**Structure:**

```markdown
# Monument UI Master Plan

## I. Current State (from ENHANCED_UI_SUMMARY.md)
## II. Strategic Vision (from MONUMENT_UI_STRATEGIC_DESIGN_PLAN.md)
## III. Implementation Plan (from PHOTOREALISTIC_UI_IMPLEMENTATION_PLAN.md)
## IV. Progress Tracking (from PHOTOREALISTIC_UI_PROGRESS.md)
## V. Roadmap (from UI_UX_ROADMAP.md)
```

### 3. Consolidate Testing Documentation

**Actions:**

- Move `docs/testing/TESTING_GUIDE.md` → Link from `TESTING.md`
- Move `docs/testing_audit.md` → `docs/testing/TESTING_AUDIT.md`
- Move `docs/test_output_schemas.md` → `docs/schemas/test_output_schemas.md`
- Move `docs/STRESS_TEST_PLAN.md` → `docs/testing/STRESS_TEST_PLAN.md`
- Move `docs/PARAMETER_STRESS_RESULTS.md` → `docs/testing/PARAMETER_STRESS_RESULTS.md`

### 4. Create READMEs for Major Folders

**New README files:**

- `docs/architecture/README.md` - Architecture documentation guide
- `docs/development/README.md` - Development workflow guide
- `docs/testing/README.md` - Testing documentation index
- `docs/ui/README.md` - UI documentation overview
- `docs/archive/README.md` - Archive contents index

---

## Cross-Reference Updates

### Files Requiring Link Updates

1. `README.md` - Update doc links to new locations
2. `docs/INDEX.md` - Complete rewrite with new structure
3. `NEXT_SESSION_HANDOFF.md` - Update doc references
4. All files linking to moved documentation

### Verification Steps

1. Use grep to find all relative markdown links: `grep -r "\[.*\](.*.md)" docs/`
2. Update broken links to new locations
3. Verify all cross-references resolve correctly
4. Test all documentation links in root README.md

---

## Implementation Checklist

### Phase 0: Pre-Archive Verification (CRITICAL)

**Before archiving ANY file, verify information is captured:**

- [ ] Review each dated session file (01072026-*, 01082026-*) for unique content
- [ ] Verify all session information is in CHANGELOG.md (Sessions 1-24)
- [ ] Check SESSION_17_SUMMARY.md - ensure Memory fix details in CHANGELOG
- [ ] Review ImplementationPlan.md - verify current status in docs/STATUS.md
- [ ] Review Roadmap.md - verify roadmap items in NEXT_SESSION_HANDOFF.md
- [ ] Check all phase completion docs - verify summaries in CHANGELOG.md
- [ ] Create ARCHIVE_VERIFICATION_REPORT.md documenting what was preserved where

**Preservation Strategy:**

1. **Dated Session Files:** All unique content → CHANGELOG.md
2. **Planning Docs:** Current status → docs/STATUS.md, roadmap → NEXT_SESSION_HANDOFF.md
3. **Phase Completions:** Achievements → CHANGELOG.md, status → docs/STATUS.md
4. **Add Archive References:** Update CHANGELOG.md with pointers to archived files

### Phase 1: Preparation

- [ ] Create archive directory structure
- [ ] Create backup of docs/ folder (zip archive)
- [ ] Document all files being moved (this plan)
- [ ] Create docs/archive/README.md with index of archived content

### Phase 2: Root Cleanup

- [ ] Move dated session files → `docs/archive/sessions/YYYY-MM-DD/`
- [ ] Move planning docs → `docs/archive/planning/`
- [ ] Move optimization docs → `docs/development/`
- [ ] Verify root has only 11 essential files

### Phase 3: docs/ Consolidation

- [ ] Move phase completion docs → `docs/archive/phases/`
- [ ] Move session-specific docs → `docs/archive/sessions/`
- [ ] Move experimental docs → `docs/archive/sessions/experimental/`
- [ ] Consolidate architecture files
- [ ] Consolidate UI documentation
- [ ] Consolidate testing documentation

### Phase 4: Structure Enhancement

- [ ] Create new README.md files for major folders
- [ ] Create `docs/ui/UI_MASTER_PLAN.md`
- [ ] Move testing files to proper locations
- [ ] Move schema files to proper locations

### Phase 5: Cross-Reference Fixes

- [ ] Update `docs/INDEX.md` with new structure
- [ ] Update `README.md` doc links
- [ ] Update `NEXT_SESSION_HANDOFF.md` references
- [ ] Find and fix all broken internal links
- [ ] Verify all cross-references resolve

### Phase 6: Validation

- [ ] Verify all markdown files are accessible
- [ ] Check for broken links (markdown link checker)
- [ ] Verify alignment with codebase structure
- [ ] Verify testing documentation matches test suite
- [ ] Confirm no content was lost in consolidation

---

## Benefits

1. **Clarity:** Root directory has only essential, current documentation
2. **Organization:** Clear folder structure by domain (architecture, development, testing, ui)
3. **History:** All historical documents preserved in organized archive
4. **Maintainability:** Easier to find and update documentation
5. **Onboarding:** New contributors can navigate docs easily
6. **Alignment:** Documentation structure matches codebase organization

---

## Token Savings

**Current:** ~180 markdown files across root + docs/
**After Cleanup:** ~120 active files + 60 archived files

**Root directory:** 21 files → 11 files (48% reduction)
**Estimated token savings:** ~5K tokens per session start (consolidated, archived historical docs)

---

## Next Steps

1. Review this plan
2. Approve consolidation strategy
3. Execute phase-by-phase with TodoWrite tracking
4. Validate all cross-references
5. Update NEXT_SESSION_HANDOFF.md to reflect new structure
