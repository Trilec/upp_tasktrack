# TaskTrack MCP contracts

TaskTrack exposes two local STDIO MCP services with deliberately different authority and lifecycle.

## Human-decision MCP

`TaskTrackMcp.exe` is the established human evidence server. Its tools remain:

- `version`
- `create_task`
- `get_task`
- `open_task`
- `list_tasks`
- `close_task`
- `respond_to_request`

`create_task` normally owns the live interaction until completed/closed or an assistance compatibility checkpoint. Canonical human evidence is `items[].answer.data`. Dashboard work does not alter this contract.

See `INTERACTION_LIFECYCLE.md` and `QUESTION_TYPES.md` for the human path.

## Dashboard MCP

`TaskTrackDashboardMcp.exe` is a separate non-blocking project-state service over DashboardCore.

### Tools

- `version` — dashboard component/schema/protocol information.
- `validate_dashboard` — validate a dashboard document without installing it.
- `upsert_dashboard` — create/update current state and create the next immutable accepted revision.
- `get_dashboard` — retrieve current state.
- `open_dashboard` — launch the read-only native viewer for an existing dashboard.
- `list_dashboards` — list recent current dashboard documents.
- `list_dashboard_revisions` — list accepted immutable revisions.
- `get_dashboard_revision` — retrieve a specific accepted historical revision.

### Upsert

Create:

```json
{
  "dashboard": { "dashboard_id": "my-project", "title": "Project status", "panels": [...] }
}
```

A new dashboard becomes revision 1. `base_revision` may be omitted or zero.

Update:

```json
{
  "dashboard_id": "my-project",
  "base_revision": 4,
  "dashboard": { "dashboard_id": "my-project", "title": "Project status", "panels": [...] }
}
```

Existing dashboards require an exact current `base_revision`.

- `REVISION_REQUIRED` — update omitted a base revision.
- `REVISION_CONFLICT` — base revision is stale; re-read/merge/retry.
- `WRITE_BUSY` — another process currently owns the short dashboard writer lock; retry the normal read/merge/upsert path.

The upsert path serializes writers with a cross-process lock; `WRITE_BUSY` is retryable and must not be bypassed.

### Presentation semantics

Agents send semantic panel/entry data only. Do not send U++ class names, coordinates, widths, colours or other GUI construction instructions. The native app chooses presentation.

### Open

`open_dashboard` returns after launching the viewer. Unlike a human TaskTrack question, a dashboard view is informational and does not hold the MCP request open waiting for user evidence.

### Authority

Dashboard output is useful project state, not human testimony. If an Attention item needs a human decision, create a normal TaskTrack human task and optionally reference its `task_id` from the dashboard entry.

## Dashboard CLI

```text
TaskTrackDashboardMcp.exe
TaskTrackDashboardMcp.exe --help
TaskTrackDashboardMcp.exe --version
TaskTrackDashboardMcp.exe --selftest
TaskTrackDashboardMcp.exe --oneshot <request.json>
```

Register the executable as a local STDIO MCP service with no arguments.
