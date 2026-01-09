# Codex Troubleshooting

## Codex command not found
- Ensure the CLI binary is installed.
- Verify it is on your PATH.
- Try `codex --help` to confirm.

## Permission denied or sandbox errors
- You may be in a restricted sandbox.
- Approve the command if appropriate.
- Move the task into the workspace directory if possible.

## Missing skill or asset
- Confirm the skill folder exists in `$CODEX_HOME/skills`.
- Verify `SKILL.md` is present.
- Check any external asset bundle path documented by the skill.

## UI changes do not appear
- Rebuild the plugin or app.
- Confirm the new assets are packaged or embedded.
- Clear any cached resources if your app loads assets at startup.

## Build or tests fail
- Ask Codex to summarize the failure and propose a fix.
- Run the tests with verbose output to capture logs.
- Verify that you are using the correct build config (Debug vs Release).

## Codex looks stuck or idle
- Ask "What are you currently working on?"
- Request a short plan.
- Reduce the task into smaller steps.
