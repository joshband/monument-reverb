# Repository Guidelines

## Project Structure & Module Organization
Current layout:

- `plugin/` for JUCE processor/editor sources
- `dsp/` for DSP modules
- `ui/` for UI components
- `tests/` for automated tests
- `scripts/` for maintenance or tooling
- `docs/` for documentation

## Build, Test, and Development Commands
Canonical commands:

- `./scripts/build_macos.sh` - configure and build Debug + Release
- `./scripts/open_xcode.sh` - generate and open the Xcode project
- `cmake -S . -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES=arm64` - configure (add `-DMONUMENT_ENABLE_TESTS=ON -DBUILD_TESTING=ON` to register any CTest target; both are OFF by default, and `enable_testing()` itself lives inside the `MONUMENT_ENABLE_TESTS` guard, so a plain configure registers zero tests)
- `cmake --build build --config Release` - build Release
- `ctest --test-dir build -C Release` - run registered tests (empty unless the test flags above were set at configure time)

Include the command list in `README.md` as well if it becomes user-facing.

Only macOS is built and tested by this repository's scripts/workflows; JUCE's
format declarations are not evidence of supported Windows/Linux releases.

## Coding Style & Naming Conventions
Formatting uses `.clang-format`. C++ standard is C++17.

## Testing Guidelines
Tests use CTest and live under `tests/`.
Canonical QA/CI/testing hub: `TESTING.md` (authoritative DSP gate: `audio-dsp-qa-harness` scenario suites; full local diagnostic: `./scripts/run_ci_tests.sh`, non-authoritative). Index: `docs/testing/README.md`.

## Invariants (do not change during structural cleanup or doc/tooling work)
- Rendered DSP algorithms, the default `AncientWay` fixed signal chain and its
  order, routing-preset bypass behavior, both macro mappers (`MacroMapper`,
  `ExpressiveMacroMapper`).
- Host parameter IDs, ranges, and declaration order in `createParameterLayout()`.
- Factory preset order/indices in `plugin/PresetManager.cpp` and the user
  preset JSON format version.
- CI policy (`.github/workflows/qa_harness.yml` is the authoritative DSP gate).

Reconnecting a disconnected feature (e.g. Memory Echoes), switching the
processor to the generic routing-graph executor, or changing smoothing/mix law
are product decisions, not cleanup — they need their own review.

## Audio-Thread Restrictions
Never in `processBlock`/audio-callback paths: heap allocation, locks/mutexes,
file I/O or logging, exceptions, or unbounded loops. `MONUMENT_TESTING` is
test/editor-suppression scaffolding, not a production-equivalent flag — it
also enables string construction/logging in the callback, which distorts
allocation measurements taken under it. See `TESTING.md` for the known-red
`monument_realtime_allocation_characterization_test`, which proves (without
`MONUMENT_TESTING`) that the timeline-preset and TubeRayTracer paths still
allocate.

## Definition of Done
A bounded, intended change; explicit preserved contracts (see Invariants
above); the canonical check for the touched area passes with reported counts,
not just a green exit code; no unexplained audio/state delta; no new
audio-thread hazard; docs updated where they made a claim the change
contradicts; diff inspected; no generated build/capture artifacts staged;
remaining known limitations stated in the PR/commit. A pre-existing failing
test is a finding to name, not license to weaken a gate or add
`WILL_FAIL`/skip annotations to make it disappear.

## Commit & Pull Request Guidelines
Follow the existing commit style in `git log` (Conventional Commits, e.g., `feat: add reverb preset loader`).

Pull requests should include:

- A clear description of changes and rationale
- Linked issue or task (if applicable)
- Test results or a note explaining why tests are not applicable
- Screenshots or recordings for UI changes

## Security & Configuration Tips
Avoid committing secrets. Store configuration in `.env` or a local config file and document required keys in `README.md`.
