# TaskTrack V0.1 Windows Acceptance

This document defines the first authoritative Windows bring-up. It is intentionally stricter than source review: the release is not considered Windows-accepted until these steps are run on the user's machine.

## Preconditions

- Validate the exact TaskTrack `main` commit supplied in the handoff.
- Working tree must be clean before testing.
- Do not modify TaskTrack, Ui, Animation or U++ sources merely to make the build pass.
- Record current dependency heads before compilation.
- If a dependency has moved from the source-review snapshot in `PLAN.md`, report the exact newer SHA; do not silently reset it unless directed.

## Build paths

Expected default paths:

```text
E:\apps\github\upp_tasktrack
E:\apps\github\upp_Ui
E:\apps\github\upp_statemachine
E:\apps\github\upp_animation
E:\upp-18468
```

## Automated build

From PowerShell:

```powershell
cd E:\apps\github\upp_tasktrack
powershell -ExecutionPolicy Bypass -File .\verify.ps1 -UppRoot E:\upp-18468
```

Expected programs in `build`:

```text
TaskTrack.exe
TaskTrackMcp.exe
TaskTrackTests.exe
TaskTrackExample.exe
```

The script must finish with:

```text
verify.ps1: ok
```

## Debug compile

After the release/smoke wrapper succeeds, also compile the four packages in U++ Debug with CLANG x64, either from TheIDE using `GitHubOut.var` or equivalent `umk` commands. This catches assertions and debug-only API misuse that a release build can hide.

Required packages:

- `TaskTrack/Core` as dependency
- `TaskTrack/App`
- `TaskTrack/Mcp`
- `tests/TaskTrackTests`
- `examples/TaskTrackExample`

Run `TaskTrackTests.exe` and `TaskTrackMcp.exe --selftest` from the debug outputs as well.

## GUI launch

Generate a real task:

```powershell
.\build\TaskTrackExample.exe
```

Copy the printed `.tasktrack.json` path, then:

```powershell
.\build\TaskTrack.exe --task "<printed path>"
```

Keep the GUI open for Curt's visual acceptance.

## Visual/interaction matrix

### Shell

- TaskTrack heading is compact and clear.
- Objective title/subtitle are readable without oversized text.
- Ordinary UI text is no larger than the intended compact 9pt family; item headings are about 10pt and main headings 11pt bold.
- Header buttons/dropdowns remain tight rather than oversized.
- No clipping or overlap at the default window size.

### Categories

- `All` plus generated categories appear in the layered category area.
- Category controls wrap cleanly when the window narrows.
- Selecting a category filters only the visible cards; answers in other categories remain intact.

### Responsive task cards

- At a wide window, multiple cards can share a row.
- As the window narrows, cards wrap without content overlap.
- Vertical scrolling works for a long task.

### Field types

Confirm all ten example controls function and persist:

- checkbox confirmation;
- pass/fail dropdown;
- choice dropdown;
- short text;
- multiline text;
- number observation;
- colour witness with visible expected swatch;
- file/output confirmation;
- interaction result;
- visual comparison result.

### Save/recovery

- Editing a field, waiting briefly, closing and reopening preserves the answer.
- Explicit Save preserves answers immediately.
- Save split menu exports Markdown.
- Save split menu exports JSON.
- Save Copy writes a separate task file without changing the authoritative task path.

### Required checks

- `Send Result` while a required check is unanswered reports the missing item IDs.
- After all required checks are answered, `Send Result` changes the task to completed.
- Completed cards are read-only on reopen.
- Export remains available for a completed task.

### Pause

- Pause changes state to `paused` and persists.
- Closing/reopening keeps the task paused.
- Resume returns to active work with answers intact.
- Entering new human evidence while paused resumes work rather than dropping the edit.

### Reminder

For practical acceptance, select the `1 minute` reminder.

- Leave an active task untouched for at least one minute.
- Reminder appears without closing the task.
- `Continue` keeps work active.
- `Pause` pauses it.
- Enable `Paused reminders`, wait again, and verify the reminder can appear while paused.
- `Close task` is deliberate and results in `closed`.
- Confirm there is no automatic close merely because time passes.

### Agent nudge

- Enable `Agent nudge`.
- Leave the task inactive for at least one minute.
- From a second console invoke `get_task` through the MCP server or use the protocol harness.
- Verify a fresh poll can trigger the human reminder.
- Verify the poll does not change any check answer.

### MCP persistence

- Run `TaskTrackMcp.exe --selftest` successfully.
- Create a task with `launch=false` through MCP.
- Verify the returned task ID resolves after the MCP process exits/restarts.
- Complete that task in the GUI.
- Verify later retrieval returns the completed evidence.

## Stop conditions

Stop and report immediately if:

- any package fails to compile;
- a U++ debug assertion occurs;
- save/reload loses an answer;
- pause is not durable;
- a task auto-closes because of elapsed time;
- `create_task` returns a task ID before the task file exists;
- agent polling mutates evidence;
- completed tasks remain editable;
- GUI controls overlap or become unusable at normal desktop sizes.

Do not weaken tests or change fixtures/contracts to convert a failure into a pass.

## Evidence to return

Return one report containing:

- validated TaskTrack commit SHA;
- Ui, Animation, StateMachine and U++ build/version context;
- exact commands used;
- Debug compile PASS/FAIL per package;
- release wrapper output summary;
- `TaskTrackTests` result;
- `TaskTrackMcp --selftest` result;
- generated task path;
- any assertion/error text verbatim;
- which GUI checks Curt still needs to judge visually;
- final `git status --short`.

Do not commit or push any repair unless explicitly asked after the failure report.
