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

- `TaskTrack.exe`
- `TaskTrackMcp.exe`
- `TaskTrackTests.exe`
- `TaskTrackExample.exe`

and runs `TaskTrackTests.exe` plus `TaskTrackMcp.exe --selftest`.

Do not run the wrapper while `TaskTrack.exe` from the same build path is still open on Windows; the running executable may prevent `ld.lld` from replacing it.

## Generate the 18-type demo

```powershell
.\build\TaskTrackExample.exe
```

The example prints a durable `.tasktrack.json` path. Open it with:

```powershell
.\build\TaskTrack.exe --task "<printed path>"
```

The task contains exactly one canonical V0.2 question of each type and is the preferred first visual/interaction acceptance surface.

## MCP

Register `build\TaskTrackMcp.exe` as an argument-free stdio MCP server. The main workflow is:

1. agent calls `create_task`;
2. TaskTrack persists it and optionally launches the GUI;
3. agent retains `task_id` rather than waiting for the person;
4. agent calls `get_task` later;
5. completed results expose structured `answer.data`.

Read `docs/AGENT_GUIDE.md` before authoring real requests.
