# TaskTrack — Getting Started

## Prerequisites

TaskTrack is a U++ project. The checked-in Windows assembly file assumes:

- repository: `E:\apps\github\upp_tasktrack`
- U++: `E:\upp-18468`
- Ui: `E:\apps\github\upp_Ui`
- StateMachine: `E:\apps\github\upp_statemachine`
- Animation: `E:\apps\github\upp_animation`

`GitHubOut.var` contains the equivalent TheIDE assembly/output configuration. Change those paths if your workstation differs.

## Windows build

The normal verification path is:

```powershell
cd E:\apps\github\upp_tasktrack
powershell -ExecutionPolicy Bypass -File .\verify.ps1 -UppRoot E:\upp-18468
```

The script builds these programs into `build`:

- `TaskTrack.exe`
- `TaskTrackMcp.exe`
- `TaskTrackTests.exe`
- `TaskTrackExample.exe`

It then runs the core tests and MCP self-test. The GUI still requires a live Windows visual acceptance pass.

## Create an example task

```powershell
.\build\TaskTrackExample.exe
```

The program prints the durable `.tasktrack.json` path it created.

Open it with:

```powershell
.\build\TaskTrack.exe --task "<printed-path>"
```

TaskTrack autosaves edited answers after a short debounce. Explicit Save, pause/resume, settings changes, completion, and Exit save synchronously. The footer split button can also export Markdown or JSON evidence.

## MCP server

Register `build\TaskTrackMcp.exe` as an argument-free stdio MCP server. The canonical tools are:

- `version`
- `create_task`
- `get_task`
- `open_task`
- `list_tasks`
- `close_task`

`create_task` creates and persists the request immediately, returns a stable `task_id`, and can launch the GUI. Agents retrieve or poll the durable task rather than holding the original invocation open.

See `docs/MCP.md` for request examples and current/legacy protocol behavior.
