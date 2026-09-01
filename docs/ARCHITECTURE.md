# TaskTrack architecture

TaskTrack has two intentionally separate semantic authority domains behind one local MCP service:

- **Human decisions** — a person supplies durable structured evidence.
- **Project dashboards** — an agent supplies durable structured project state for native visual presentation.

`TaskTrackDocument.items[].answer.data` remains human-answer authority. Dashboard state may reference a human task but is always agent-authored management/presentation state.

## Single MCP boundary

Agent hosts register only `TaskTrackMcp.exe`.

```text
Agent host
    |
    +-- TaskTrackMcp.exe
            |
            +-- create_task / open_task --> TaskTrackGui.exe
            |
            +-- dashboard tools ---------> TaskTrackDashboardGui.exe
```

The MCP executable exposes both tool families. Human-decision calls retain their existing live interaction lifecycle. Dashboard calls are non-blocking project-state operations and do not enter the human-evidence wait path.

## Human-decision packages

### `TaskTrack/Core`
Owns the human task/answer model, 18 semantic question types, validation/migration, persistence/recovery, locator/result helpers and the separate agent-assistance sidecar.

### `TaskTrack/Widgets`
Maps semantic human questions onto native `upp_Ui` controls. MCP never names GUI classes.

### `TaskTrack/App`
Owns the native question window, measured responsive layout, autosave/reminders and live human interaction.

### `TaskTrack/Mcp`
Owns the single public STDIO MCP boundary. The accepted human `create_task` lifecycle remains intact and the dashboard tool bridge delegates project-state operations to DashboardCore.

## Dashboard packages

### `TaskTrack/DashboardCore`
GUI- and MCP-independent authority for dashboard state:

- schema 1 semantic panel model;
- strict validation and safety bounds;
- globally stable/unique entry ids;
- weighted derived progress and attention summaries;
- validated JSON conversion;
- atomic current-file writes with `.bak` recovery;
- optimistic `base_revision` checking;
- cross-process writer lock around re-read/check/install;
- immutable numbered revision snapshots with incomplete-next-revision crash recovery;
- current/history listing and revision loading.

The current dashboard is not an append-only project log. Revisions hold prior accepted state.

### `TaskTrack/DashboardWidgets`
Maps the eight dashboard semantics onto native `upp_Ui` controls. It includes the TaskTrack-local `TaskTrackTimelineRail` while that control is being proven in real dashboard usage.

Project State consumes public `UiProgressRing`; progress/objective rows consume existing Ui controls. Agents provide semantics, not pixel geometry.

### `TaskTrack/DashboardApp`
Read-only native dashboard viewer with project summary, progress ring, attention count, categories, stacked independent panels, detail expansion, current auto-refresh and historical revision browsing.

## Distributable runtime

```text
TaskTrackMcp.exe
TaskTrackGui.exe
TaskTrackDashboardGui.exe
```

The MCP and both GUIs are kept together. Only `TaskTrackMcp.exe` is registered with the agent host.

## Persistence invariants

1. Dashboard JSON is never human evidence.
2. Existing dashboard updates require exact `base_revision`.
3. Writer locking closes the check-then-write race across simultaneous agent processes.
4. Accepted numbered revisions are immutable.
5. An incomplete beyond-current next revision from a crashed writer may be replaced under the writer lock; accepted revisions at or below current may not.
6. Current documents are bounded; history belongs in revisions.
7. Native layout is selected by TaskTrack, never by MCP pixel geometry.
8. The existing human-question lifecycle and answer authority remain unchanged.
