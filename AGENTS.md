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
- `cmake -S . -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES=arm64` - configure
- `cmake --build build --config Release` - build Release
- `ctest --test-dir build -C Release` - run tests

Include the command list in `README.md` as well if it becomes user-facing.

## Coding Style & Naming Conventions
Formatting uses `.clang-format`. C++ standard is C++17.

## Testing Guidelines
Tests use CTest and live under `tests/`.

## Commit & Pull Request Guidelines
There is no Git history yet, so no existing commit conventions to summarize. Until standards are set, use Conventional Commits (e.g., `feat: add reverb preset loader`).

Pull requests should include:

- A clear description of changes and rationale
- Linked issue or task (if applicable)
- Test results or a note explaining why tests are not applicable
- Screenshots or recordings for UI changes

## Security & Configuration Tips
Avoid committing secrets. Store configuration in `.env` or a local config file and document required keys in `README.md`.
