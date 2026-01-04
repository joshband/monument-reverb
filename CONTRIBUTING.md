# Contributing to Monument

Thanks for your interest in Monument. This project is early and evolving. Keep changes focused
and communicate intent clearly.

## Getting started

- Fork the repo and create a feature branch.
- Follow the build steps in [README.md](README.md) and [STANDARD_BUILD_WORKFLOW.md](STANDARD_BUILD_WORKFLOW.md).
- Review [ARCHITECTURE.md](ARCHITECTURE.md) and [docs/INDEX.md](docs/INDEX.md) for project structure.
- Keep changes scoped and readable.

## Coding guidelines

- Use modern C++ (C++17) and keep functions small.
- Format code with the repository `.clang-format`.
- Prefer explicit names over abbreviations.

## Commit guidelines

Use Conventional Commits, for example:

- `feat: add diffusion stage`
- `fix: clamp modulation rate`
- `docs: clarify macOS build steps`

## Documentation guidelines

When adding or updating documentation:

- **Root docs** (8 files only): AGENTS, ARCHITECTURE, CHANGELOG, CONTRIBUTING, MANIFEST, README, STANDARD_BUILD_WORKFLOW, ARCHITECTURE_QUICK_REFERENCE
- **Categorized docs** go in `docs/` subdirectories:
  - `docs/ui/` - UI/UX design documents
  - `docs/development/` - Quick starts, guides, tutorials
  - `docs/architecture/` - Technical architecture, reviews
  - `docs/testing/` - Test plans, validation docs
- Update [docs/INDEX.md](docs/INDEX.md) when adding new docs
- Use markdown links for cross-references
- See [docs/INDEX.md](docs/INDEX.md) for full documentation structure

## Pull requests

Include:

- A brief description of the change and why it is needed
- Build or test results, or a note explaining why they are not applicable
- Screenshots or short clips for UI changes
- Documentation updates if adding features or changing behavior
