# TaskTrack MCP Host Setup

TaskTrackMcp uses stdio. The host should launch the executable with no arguments and communicate through stdin/stdout.

Example executable path:

```text
E:\apps\github\upp_tasktrack\build\TaskTrackMcp.exe
```

The exact host configuration syntax differs between Codex, OpenCode, Hermes and other MCP clients, but the registration has the same essentials:

- transport: stdio
- command: absolute path to `TaskTrackMcp.exe`
- arguments: none
- working directory: repository root or `build` is acceptable

TaskTrackMcp expects `TaskTrack.exe` beside it when `launch=true` or `open_task` is used. The checked-in build script therefore places both executables in the same `/build` directory.

## Recommended agent behaviour

1. Use ordinary tests first.
2. Create a TaskTrack task only for facts automation cannot establish confidently.
3. Keep each check concrete and observable.
4. Use categories when they materially organize a larger request; do not create tabs/categories merely for decoration.
5. Make a check `required` only when the agent truly cannot continue safely without the human answer.
6. Poll by `task_id`; never keep the original tool invocation open waiting for a human.
7. Treat `paused` as intentional human state, not failure.
8. Continue only after `completed`, unless the workflow has explicit logic for a human-closed/cancelled task.

## Good check wording

Prefer:

- `Drag the divider left and right. Does the preview update continuously?`
- `Export Markdown. Does <task-id>.md exist and contain the completed checks?`
- `Compare the button against #0078D4. Match / Different / Unsure.`

Avoid vague prompts such as:

- `Does it look good?`
- `Check everything.`

TaskTrack should reduce human ambiguity, not simply move an unstructured chat question into another window.
