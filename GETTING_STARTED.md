# TaskTrack — Getting Started

## Windows/U++ baseline

The checked-in assembly examples assume:

```text
repository:        E:\apps\github\upp_tasktrack
U++:               E:\upp-18468
Ui:                E:\apps\github\upp_Ui
Animation:         E:\apps\github\upp_animation
StateMachine:      E:\apps\github\upp_statemachine
```

`GitHubOut.var` / `example.var` contain the corresponding assembly/output paths.

## Build and deterministic verification

```powershell
cd E:\apps\github\upp_tasktrack
powershell -ExecutionPolicy Bypass -File .\verify.ps1 -UppRoot E:\upp-18468
```

This builds directly into `build`:

- `TaskTrackGui.exe` — native human GUI
- `TaskTrackMcp.exe` — stdio MCP server
- `TaskTrackTests.exe`
- `TaskTrackExample.exe`

and runs `TaskTrackTests.exe` plus `TaskTrackMcp.exe --selftest`.

Do not run the wrapper while `TaskTrackGui.exe` from the same build path is still open on Windows; the running executable may prevent `ld.lld` from replacing it.

## CLI

```text
TaskTrackGui.exe                choose an existing task
TaskTrackGui.exe --task <path>  open a specific task
TaskTrackGui.exe --help         show GUI usage
TaskTrackGui.exe --version      show version

TaskTrackMcp.exe                run the stdio MCP server
TaskTrackMcp.exe --help         show MCP usage
TaskTrackMcp.exe --version      show version/schema/protocol
TaskTrackMcp.exe --selftest     run deterministic MCP self-test
```

Both executables should normally be placed in the same directory. `TaskTrackMcp.exe` launches `TaskTrackGui.exe --task <path>` beside itself.

## Generate the 18-type demo

```powershell
.\build\TaskTrackExample.exe
```

The example prints a durable `.tasktrack.json` path. Open it with:

```powershell
.\build\TaskTrackGui.exe --task "<printed path>"
```

The task contains exactly one canonical V0.2 question of each type and is the preferred first visual/interaction acceptance surface.

## MCP

`build\TaskTrackMcp.exe` is an **argument-free local stdio MCP server**. The main workflow is:

1. agent calls `create_task`;
2. TaskTrack persists it and optionally launches the GUI;
3. agent retains `task_id` rather than waiting for the person;
4. agent calls `get_task` later;
5. `respond_to_request` resolves any pending human→agent assistance (`propose_answer`, `clarify`, `continue_with_judgement`);
6. completed results expose structured `answer.data` (and optional `answer.note`).

Read `docs/AGENT_GUIDE.md` before authoring real requests.

## Host registration

The executable is a stdio MCP server with no required command-line arguments:

```text
E:\apps\github\upp_tasktrack\build\TaskTrackMcp.exe
```

Host configuration is **not** modified by the repository. Final host acceptance will:

1. install/register the published binary into **OpenCode** first;
2. verify `opencode mcp list` shows the TaskTrack server connected;
3. then install/register it into **Codex** second.

Before documenting exact per-host commands, the validator inspects the actually installed CLI (`opencode --version` / `opencode mcp --help`, `codex --version` / `codex mcp --help`) and records the real command names rather than assuming syntax. No wrapper is added unless a host genuinely requires one.
