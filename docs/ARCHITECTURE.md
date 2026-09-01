# TaskTrack architecture

TaskTrack now has two intentionally separate semantic interfaces for agent workflows:

- **Human decisions** — a person supplies durable structured evidence.
- **Project dashboards** — an agent supplies durable structured project state for native visual presentation.

The authority domains never merge. `TaskTrackDocument.items[].answer.data` remains human-answer authority. Dashboard state may reference a human task but is always agent-authored management/presentation state.

## Existing human-decision packages

### `TaskTrack/Core`
Owns the human task/answer model, 18 semantic question types, validation/migration, persistence/recovery, locator/result helpers and the separate agent-assistance sidecar.

### `TaskTrack/Widgets`
Maps semantic human questions onto native `upp_Ui` controls. MCP never names GUI classes.

### `TaskTrack/App`
Owns the native question window, measured responsive layout, autosave/reminders and live human interaction.

### `TaskTrack/Mcp`
Owns the existing live `create_task` lifecycle and assistance protocol.

These packages were deliberately left unchanged by the dashboard addition.

## Dashboard companion packages

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

The current dashboard is not intended as an append-only project log. Revisions hold prior accepted state.

### `TaskTrack/DashboardWidgets`
Maps the eight dashboard semantics onto native `upp_Ui` controls. It includes the TaskTrack-local `TaskTrackTimelineRail` while that control is being proven in real dashboard usage.

The rail has a normal Ui-style public contract: theme-driven default style, custom-style override, data binding, selection, mouse, keyboard and focus behaviour.

Project State consumes the public `UiProgressRing`; progress/objective rows consume existing `UiProgressBar`. Other panels are compositions of standard Ui labels/layouts/group panels.

### `TaskTrack/DashboardApp`
Read-only native dashboard viewer:

- summary header/ring/attention count;
- category filter;
- revision selector;
- vertically stacked independent panels;
- panel detail expansion;
- current revision auto-refresh;
- historical revision browsing.

There is no fixed dashboard geometry supplied by the agent. The agent supplies semantic chunks; the App owns native composition.

### `TaskTrack/DashboardMcp`
Small independent local STDIO MCP server over DashboardCore. Dashboard tools return immediately; they do not enter the blocking human-input lifecycle.

Separating DashboardMcp from the accepted human-decision MCP avoids risking the live TaskTrack interaction while the dashboard contract is new.

## Executables

Human decision:

```text
TaskTrackMcp.exe
TaskTrackGui.exe
```

Dashboard:

```text
TaskTrackDashboardMcp.exe
TaskTrackDashboardGui.exe
```

## Persistence invariants

1. Dashboard JSON is never human evidence.
2. Existing dashboard updates require exact `base_revision`.
3. Writer locking closes the check-then-write race across simultaneous MCP processes.
4. Accepted numbered revisions are immutable.
5. An incomplete beyond-current next revision from a crashed writer may be replaced under the writer lock; accepted revisions at or below current may not.
6. Current documents are bounded; history belongs in revisions.
7. Native layout is selected by TaskTrack, never by MCP pixel geometry.
8. The existing human-question lifecycle and answer authority remain unchanged.
