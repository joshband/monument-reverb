# Monument Roadmap

Date: 2026-01-07
Owner: Codex documentation branch

## Goals
- Stabilize audio-thread safety and testing.
- Consolidate documentation and clarify the canonical UI path.
- Evolve the Monument UI using the Playground learnings (particles, layered PBR UI, cursor-reactive elements, audio-reactive visuals).

## Roadmap (Next 6-8 Weeks)

### Now (0-2 weeks)
- Fix critical correctness issues from 01072026 reviews.
- Make routing preset changes RT-safe and playhead access null-safe.
- Make CTest paths config-aware and ensure smoke tests pass in Release.
- Consolidate docs (README, INDEX, ARCHITECTURE, BUILD/TESTING) and add review/plan docs.

### Next (2-6 weeks)
- Decide and document the canonical UI path (plugin UI vs Playground as experimental surface).
- Unify asset pipeline strategy (embedded BinaryData vs file-based packs).
- Integrate particle rendering as an optional UI layer (behind feature flag or build option).
- Build a component architecture that supports layered PBR controls + audio reactive cues.

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
