# Codex Installation Manual

This document is intentionally conservative because Codex setups vary by org. If your team has an internal installer or wrapper, follow that first.

## Requirements
- A terminal (macOS, Linux, or Windows shell).
- Git installed and configured.
- Access to your org's Codex distribution or the official Codex CLI package.

## Install steps (generic)
1. Get the Codex CLI package from your org or the official source.
2. Install it using the provided installer or by placing the binary on your PATH.
3. Verify installation:
   - Run `codex --version` or `codex --help`.
4. Configure environment:
   - Set `CODEX_HOME` if your org uses a non-default location.
   - If required, set credentials via your org-approved method (for example, `.env`, keychain, or SSO).

## Skills installation
Codex skills are small bundles of instructions and assets.
- Default location: `$CODEX_HOME/skills` or `~/.codex/skills`.
- Each skill lives in its own folder with a `SKILL.md` file.
- Copy or install skills into that folder.

Example layout:
```
~/.codex/skills/
  juce-audio-graphics-architect/
    SKILL.md
    references/
    assets/
    scripts/
```

## Verify skills
- Start Codex and ask it to list available skills, or mention a skill by name in your prompt.
- If a skill is not found, confirm the folder name and `SKILL.md` path.

## Troubleshooting install
See `Troubleshooting.md` for common issues (PATH, permissions, missing skills).
