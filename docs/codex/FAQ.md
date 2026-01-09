# Codex FAQ

## Does Codex run commands automatically?
Codex can run commands, but you can require approval for safety. If you see an approval prompt, you decide whether to allow it.

## Will Codex modify files without asking?
It will propose or apply changes based on your request. You can always inspect changes before committing.

## How do I get a code review instead of edits?
Ask explicitly for a review. Example: "Review this change for regressions and missing tests."

## Where do skills live?
Usually in `$CODEX_HOME/skills` or `~/.codex/skills`. Each skill has a `SKILL.md` file.

## How do I use a skill?
Mention the skill name in your prompt. If multiple skills apply, mention all of them.

## Why does Codex refuse a command?
The sandbox or approval policy may block it. Approve the command or adjust the task so it can run without elevated permissions.

## Can Codex access the network?
That depends on your org policy. If network access is restricted, Codex will ask for approval.

## How do I share a Codex session?
Copy the session transcript from your terminal or chat UI, or summarize the task and results.

## What if Codex makes a mistake?
Tell it to roll back, adjust the approach, or limit scope. You can also reset files using Git if needed.
