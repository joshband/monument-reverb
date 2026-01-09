# Codex Skills Guide

## What is a skill?
A skill is a bundle of instructions and assets that teach Codex how to work in a specialized domain. Each skill has a `SKILL.md` file that explains how it should be used.

## Where skills live
- Default: `$CODEX_HOME/skills` or `~/.codex/skills`.
- Each skill has its own folder.

## How to use a skill
- Mention the skill name in your prompt.
- If multiple skills apply, mention all of them.
- Follow any rules described in the skill's `SKILL.md`.

## Installing a skill
1. Copy the skill folder into your skills directory.
2. Verify the folder contains `SKILL.md`.
3. Restart Codex or start a new session.

## External assets
Some skills reference large assets stored outside the repo. These should be documented in the skill README or install guide.

## Best practices
- Keep skill repos small and versioned.
- Store large assets in a separate archive or LFS.
- Add an `INSTALL.md` that explains external dependencies.
