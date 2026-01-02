# Changelog

All notable changes to this project will be documented in this file.
The format is based on Keep a Changelog, and this project adheres to Semantic Versioning.

## [Unreleased]

### Added
- Factory preset descriptions, Init Patch, and JSON user preset save/load support.
- Pillars early-reflection upgrades: geometry bending, fractal tap clusters, pseudo-IR loading, and Glass/Stone/Fog modes.
- Testing hooks for peak/CPU logging, plus pluginval runner and testing docs.
- Advanced feature scaffolding notes for algorithm switching, colour modes, tape loop, and sound-on-sound.

### Changed
- Time feedback mapping widened to 0.35-0.995; mass and density ranges extended for darker and sparser tails.
- Drift depth limit set to +/-1 sample for subtle motion without pitch wobble.
- Freeze crossfade timing extended to 100 ms for click-free engage/release.
- Preset loading now applies Init Patch first and schedules a short DSP reset fade.
- Pillars tap energy normalization and output headroom clamp for bounded early-space output.
- Chambers parameter setters now sanitize out-of-range/NaN values (JUCE_DEBUG warnings).

### Fixed
- Preset switches clear freeze state and reset Pillars mutation timers to avoid residual behavior.

## [0.1.7] - 2026-01-01

### Added
- Preset manager with indexed and name-based loading, plus a curated preset list for core parameters.
- Editor preset picker for quick auditioning of curated spaces.
- Visual manual (HTML) and infographic assets for signal flow, control compass, and freeze transitions.

### Changed
- README expanded into a concise user guide with diagrams, parameter tables, and build/install steps.
- Time mapping extended for minute-scale T60 targets.

### Fixed
- Freeze transitions now crossfade between live and frozen wet paths to eliminate clicks.
- Freeze release now ramps early mix and envelope return instead of hard resets.

## [0.1.6] - 2026-01-01

### Added
- Drift micro-modulation with per-line LFOs for subtle, living tails.

## [0.1.5] - 2026-01-01

### Added
- Warp control with orthogonal FDN matrix morphing for slow spatial motion.

## [0.1.4] - 2026-01-01

### Added
- True Freeze semantics in Chambers with state hold and unity feedback.

### Fixed
- Hardened Freeze state hold for stability under sustained feedback.

## [0.1.3] - 2026-01-01

### Changed
- Mono-safe output mixing with constant-power balance across FDN taps.
- Gravity low-end control tuning for tighter decay behavior.

## [0.1.2] - 2026-01-01

### Added
- Per-parameter smoothing in Chambers to prevent zipper noise.

## [0.1.1] - 2026-01-01

### Added
- Monument DSP architecture and tooling scaffold.

## [0.1.0] - 2026-01-01

### Added
- Initial JUCE plugin skeleton with macOS build scripts and CMake workflow.
- CI workflows with cache and build artifacts, plus CTest smoke coverage.
- Baseline documentation: manifesto, DSP architecture, parameter behavior, and C++ standard alignment.

[Unreleased]: https://github.com/joshband/monument-reverb/compare/v0.1.7...HEAD
[0.1.7]: https://github.com/joshband/monument-reverb/releases/tag/v0.1.7
[0.1.6]: https://github.com/joshband/monument-reverb/releases/tag/v0.1.6
[0.1.5]: https://github.com/joshband/monument-reverb/releases/tag/v0.1.5
[0.1.4]: https://github.com/joshband/monument-reverb/releases/tag/v0.1.4
[0.1.3]: https://github.com/joshband/monument-reverb/releases/tag/v0.1.3
[0.1.2]: https://github.com/joshband/monument-reverb/releases/tag/v0.1.2
[0.1.1]: https://github.com/joshband/monument-reverb/releases/tag/v0.1.1
[0.1.0]: https://github.com/joshband/monument-reverb/releases/tag/v0.1.0
