# TaskTrack MCP contract

TaskTrack exposes **one local STDIO MCP service**: `TaskTrackMcp.exe`.

It carries two deliberately separate authority domains: durable human evidence and AI-maintained project-dashboard state. Sharing one MCP process does not merge those data models.

## Registration

Register only:

```text
name: tasktrack
command: C:\path\to\TaskTrackMcp.exe
```

Keep `TaskTrackGui.exe` and `TaskTrackDashboardGui.exe` beside the MCP executable.

## Version identity

For the current bring-up candidate:

```text
TaskTrackMcp.exe --version
```

reports the unified build identity plus both schemas:

```text
TaskTrack MCP
version 0.3.1
task core version 0.2.1
task schema version 2
dashboard schema version 1
MCP protocol 2026-07-28
```

The MCP `version` tool exposes the same build identity and schema fields.

## Human-decision tools

- `version`
- `create_task`
- `get_task`
- `open_task`
- `list_tasks`
- `close_task`
- `respond_to_request`

`create_task` normally owns the live interaction until completed/closed or an assistance compatibility checkpoint. Canonical human evidence is `items[].answer.data`.

`create_task` and `open_task` launch `TaskTrackGui.exe`.

See `INTERACTION_LIFECYCLE.md` and `QUESTION_TYPES.md`.

## Dashboard tools

- `validate_dashboard` — validate semantic dashboard state without persisting it.
- `upsert_dashboard` — create/update current state and append the next immutable accepted revision.
- `get_dashboard` — retrieve current state and progress/attention summaries.
- `open_dashboard` — launch `TaskTrackDashboardGui.exe` for an existing dashboard.
- `list_dashboards` — list current dashboards.
- `list_dashboard_revisions` — list accepted immutable revisions.
- `get_dashboard_revision` — retrieve one historical revision.

Dashboard calls return normally; opening the viewer does not hold the MCP request open waiting for human evidence.

### Upsert

Creation starts at revision 1. A new dashboard may omit `base_revision` or use zero.

Existing dashboards require exact current `base_revision`:

- `REVISION_REQUIRED` — base revision omitted.
- `REVISION_CONFLICT` — stale base; re-read, merge and retry.
- `WRITE_BUSY` — another process owns the short writer lock; retry the normal read/merge/upsert path.

Never bypass the writer lock or force stale state over newer project truth.

## Presentation boundary

Agents send semantic panel and entry data only. Do not send U++ class names, coordinates, widths or other GUI construction instructions. The native dashboard chooses presentation.

Dashboard output is project state, not human testimony. If an Attention item requires a human decision, create a normal TaskTrack human task and optionally reference its `task_id` from the dashboard entry.
