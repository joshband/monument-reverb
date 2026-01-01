# Repository Guidelines

## Project Structure & Module Organization
This repository is currently empty, so there is no established layout yet. When adding code, use a conventional layout to keep navigation predictable:

- `src/` for application or library source
- `tests/` for automated tests mirroring `src/`
- `assets/` for static files (images, audio, etc.)
- `scripts/` for one-off maintenance or tooling

Document any deviations here as the structure solidifies.

## Build, Test, and Development Commands
No build or test tooling is configured yet. Once a toolchain is chosen, list the canonical commands here. Examples (update to match actual tooling):

- `npm run dev` - start the local dev server
- `npm test` - run the full test suite
- `npm run build` - produce production artifacts

Include the command list in `README.md` as well if it becomes user-facing.

## Coding Style & Naming Conventions
No style rules are enforced yet. When you introduce a language/tooling stack, add the exact formatting and linting commands and key conventions. Recommended defaults until then:

- Indentation: 2 spaces for JS/TS, 4 spaces for Python
- File naming: `kebab-case` for filenames, `PascalCase` for components/classes
- Lint/format: add Prettier, ESLint, or equivalent as appropriate

## Testing Guidelines
No testing framework is configured yet. When tests are added, specify:

- Framework (e.g., Jest, Vitest, pytest)
- Test file naming (e.g., `*.test.ts`, `test_*.py`)
- Minimum coverage targets (if any)

Include a short “how to run tests” section once tooling exists.

## Commit & Pull Request Guidelines
There is no Git history yet, so no existing commit conventions to summarize. Until standards are set, use Conventional Commits (e.g., `feat: add reverb preset loader`).

Pull requests should include:

- A clear description of changes and rationale
- Linked issue or task (if applicable)
- Test results or a note explaining why tests are not applicable
- Screenshots or recordings for UI changes

## Security & Configuration Tips
Avoid committing secrets. Store configuration in `.env` or a local config file and document required keys in `README.md`.
