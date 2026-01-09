# Getting Started With Codex

This guide helps you go from zero to your first successful task with Codex.

## What Codex is
Codex is a coding agent that can read your repository, propose changes, edit files, and run commands. You describe the goal, constraints, and context; Codex does the work and reports back.

## Before you start
- Access: Make sure you have access to the Codex CLI in your org.
- Repo: Clone the repository you want Codex to work on.
- Local instructions: Check for `AGENTS.md` or `README.md` in the repo. These often contain important rules.

## First session
1. Open a terminal.
2. Go to your repo: `cd /path/to/repo`.
3. Launch the Codex CLI (often `codex`).
4. Give a focused task with clear constraints.

## How to ask for work
Use this structure:
- Goal: what you want done.
- Constraints: style rules, tests, performance, or compatibility requirements.
- Context: relevant files, background, or related tickets.
- Output: what you want to receive (diff, files changed, test command run).

Example:
"Update the UI labels to match the new naming scheme in `docs/ui-spec.md`. Keep styles consistent and run unit tests. Summarize changes and test results."

## Good habits
- Be explicit about tests you want run.
- Point to a specific file when the change is localized.
- Ask for a short plan when the change is larger.
- Ask for a review if you want risks or regressions identified.

## Approvals and safety
Codex may ask for approval to run commands, especially when network access or non-workspace paths are involved. Read the prompt and approve only if you are comfortable with the command.

## Where results go
Codex edits files in your repo. You can inspect changes with your usual Git tools.

## If you are stuck
- Ask Codex to explain its plan or current status.
- Reduce scope and request a smaller change.
- Provide more context or a failing log.
