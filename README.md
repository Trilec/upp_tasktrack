# TaskTrack

TaskTrack is a small native U++ application for durable human decisions inside agent workflows. An agent can assemble one task containing one or many questions, including different semantic categories, when the machine can do most of the work but a person still needs to judge something: a visual check, approval, wording choice, colour, ranking, numeric preference, interaction result, or another structured decision.

![TaskTrack native decision dialog](screenshot.jpg)

The agent talks to `TaskTrackMcp.exe`. When human input is needed, the MCP server persists the task and opens `TaskTrackGui.exe`. The answer comes back through the same workflow, so the person does not have to return to chat and type “done” just to wake the agent.

## What it does

- 18 semantic question types, from simple confirmation and text through colour, range, ranking, hierarchy and curve editing.
- One task can contain multiple questions across different semantic categories in a single measured dialog.
- Native U++ GUI rather than a browser form.
- Structured `answer.data` so results remain machine-readable.
- Durable task files with verified writes and recovery backups.
- Suggestions and clarification without turning agent advice into human evidence.
- **Use judgement** lets the human explicitly delegate a blocked decision back to the agent without fabricating a human answer.
- Compact dialogs are measured from the controls that were actually assembled; one question and many questions use the same layout algorithm.
- A Pass/Fail fast path uses the normal `confirm` type and can carry an optional verdict note.

## Build

TaskTrack depends on:

- U++ / TheIDE;
- [`upp_Ui`](https://github.com/Trilec/upp_Ui);
- [`upp_animation`](https://github.com/Trilec/upp_animation).

If the three repositories are siblings, the verification wrapper finds `upp_Ui` and `upp_animation` automatically:

```powershell
cd C:\dev\upp_tasktrack
powershell -ExecutionPolicy Bypass -File .\verify.ps1 -UppRoot C:\upp
```

You can also pass `-UiRoot` and `-AnimationRoot` explicitly. `UPP_ROOT` or an `umk.exe` already on `PATH` can be used instead of `-UppRoot`.

The wrapper builds:

```text
build\TaskTrackGui.exe
build\TaskTrackMcp.exe
build\TaskTrackTests.exe
build\TaskTrackExample.exe
```

and runs the deterministic Core tests plus the MCP self-test.

For a normal TheIDE assembly, include the TaskTrack repository, `upp_Ui`, `upp_animation`, and your U++ `uppsrc` directory. The main packages are `TaskTrack/App` and `TaskTrack/Mcp`.

The verification wrapper keeps generated build outputs under `build`; the release package is assembled there as well.

## Connect it to an agent host

Register **only** `TaskTrackMcp.exe` as a local STDIO MCP server, with no arguments. Keep `TaskTrackGui.exe` beside it; the MCP process launches the GUI when required.

Typical local registration is simply:

```text
name: tasktrack
transport: stdio
command: C:\path\to\TaskTrackMcp.exe
arguments: none
```

### OpenCode

OpenCode supports local STDIO MCP servers. The most version-safe setup is:

```text
opencode mcp add
```

Choose a local server, name it `tasktrack`, and use the full path to `TaskTrackMcp.exe` as the command with no arguments. Then verify the connection with:

```text
opencode mcp list
```

This avoids depending on a hand-written config shape, which differs between current OpenCode release lines. No TaskTrack-specific OpenCode plugin is required.

### Codex / Agent Skills

The MCP server works on its own. The repository also contains an optional workflow skill for hosts that support the Agent Skills format:

```text
.codex-plugin/plugin.json
skills/tasktrack/SKILL.md
```

The nested `skills/tasktrack/` directory is intentional: a skill is a folder bundle whose entry point is `SKILL.md`, and the plugin manifest points to the containing `skills/` directory. The skill teaches the host when TaskTrack is appropriate and how to keep Suggest / Clarify / Use judgement round-trips moving on hosts without native in-call sampling.

## How the interaction works

A normal task is deliberately simple:

```text
agent create_task
      ↓
TaskTrack GUI
      ↓
human answers / accepts / cancels / delegates
      ↓
structured terminal result
      ↓
agent continues
```

Human answers and agent assistance are separate. A returned suggestion is advisory until the human explicitly accepts it. `Use judgement` records delegation authority but leaves human `answer.data` empty.

## Package layout

```text
TaskTrack/Core       durable model, validation, persistence and recovery
TaskTrack/Widgets    semantic question rendering
TaskTrack/App        native GUI
TaskTrack/Mcp        local stdio MCP server
examples             18-type example task
tests                deterministic Core regression tests
skills/tasktrack     optional Agent Skill
```

The two distributable executables are `TaskTrackGui.exe` and `TaskTrackMcp.exe`. Keep them together in the same directory; no U++ installation is needed to run a packaged Windows release.

## Documentation

- [Architecture](docs/ARCHITECTURE.md) — package boundaries and design decisions.
- [MCP contract](docs/MCP.md) — tools, live interaction and fallback behaviour.
- [Interaction lifecycle](docs/INTERACTION_LIFECYCLE.md) — direct answers, assistance and delegation.
- [Question types](docs/QUESTION_TYPES.md) — the 18 semantic response types.
- [Persistence schema](docs/SCHEMA.md) — durable JSON and recovery rules.
- [Changelog](CHANGELOG.md) — release history.

## License

GNU General Public License v3.0 only. See [LICENSE](LICENSE).
