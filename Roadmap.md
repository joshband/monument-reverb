# Monument Roadmap

Date: 2026-01-07
Owner: Codex documentation branch

## Goals
- Stabilize audio-thread safety and testing.
- Consolidate documentation and lock the macro-only UI path.
- Evolve the Monument UI using reactive overlays (particles, cursor/audio cues) and JSON-driven visual profiles.

## Roadmap (Next 6-8 Weeks)

### Now (0-2 weeks)
- Fix critical correctness issues from 01072026 reviews.
- ✅ Make routing preset changes RT-safe and playhead access null-safe.
- Make CTest paths config-aware and ensure smoke tests pass in Release.
- Consolidate docs (README, INDEX, ARCHITECTURE, BUILD/TESTING) and add review/plan docs.
- Land Celestial macro knobs + macro overlay glyph cues in the plugin UI.

### Next (2-6 weeks)
- Confirm host-facing parameter names for macro-only UI.
- Expand `visual_profiles.json` to cover factory preset set.
- Unify asset pipeline strategy (embedded BinaryData vs file-based packs).
- Add fallback behavior for missing asset packs and OpenGL-disabled hosts.

### Later (6-8+ weeks)
- Polish + release milestones: preset browser, export/import, and packaging.
- Performance profiling and regression baselines for UI + DSP.
- Evaluate MemoryEchoes reintegration plan and update roadmap accordingly.

## Risks & Dependencies
- UI direction: unclear canonical UI can fragment development effort.
- Asset pipeline: packaging and deployment strategy required before shipping UI changes.
- RT safety: routing and modulation locks can cause intermittent glitches if not resolved.

## Codex-Ready Prompt Index
- See [ImplementationPlan.md](ImplementationPlan.md) for detailed prompts with acceptance criteria.
