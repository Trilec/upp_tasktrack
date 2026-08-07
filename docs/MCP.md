# TaskTrack MCP

TaskTrackMcp is an argument-free stdio server when run normally.

## Core tools

### `version`

Returns TaskTrack and protocol version information.

### `create_task`

Creates and durably saves a verification task before returning.

Important arguments:

- `project`
- `title` — required
- `subtitle`
- `actor`
- `store_root`
- `reminder_minutes`
- `remind_while_paused`
- `nudge_on_agent_poll`
- `history_limit`
- `launch` — defaults to true
- `items` — required non-empty array

The complete example is `examples/mcp_create_task.json`.

### `get_task`

Arguments:

- `task_id` — required
- `store_root` — optional
- `include_items` — optional, defaults true

The call also writes an independent poll marker. If the GUI has `Agent nudge` enabled and the operator has been inactive, this may cause a human reminder. It does not alter answers or task state by itself.

### `open_task`

Resolves the durable task and launches `TaskTrack.exe` located beside `TaskTrackMcp.exe`.

### `list_tasks`

Lists recent durable task summaries for a storage root.

### `close_task`

Explicitly closes an unfinished task. This is deliberate cancellation, not timeout cleanup. Completed tasks cannot be retroactively closed through this tool.

## Current protocol era

TaskTrack recognizes `2026-07-28` through request `_meta` and advertises the `io.modelcontextprotocol/tasks` extension during `server/discover`.

The current protocol requires per-request protocol/capability metadata. TaskTrack rejects a supplied unsupported current-style protocol revision instead of silently falling back to legacy semantics.

When a modern client declares `io.modelcontextprotocol/tasks`, `create_task` may return a formal task handle:

```json
{
  "resultType": "task",
  "taskId": "task-...",
  "status": "working",
  "pollIntervalMs": 30000
}
```

TaskTrack deliberately supplies no application expiry/TTL for the human task. A person may resume the task a day later.

### `tasks/get`

Maps TaskTrack states onto the MCP task lifecycle:

- awaiting/in progress/paused -> `working`
- completed -> `completed`, with the original tool-style result attached
- closed -> `cancelled`

### `tasks/cancel`

Explicitly closes a non-completed TaskTrack task.

### `tasks/update`

TaskTrack V0.1 has no protocol-side input requests to answer. Human input belongs to the durable GUI task, so the method returns an explanatory error.

## Legacy host compatibility

TaskTrack also supports older practical hosts using:

- `initialize`
- `notifications/initialized`
- `ping`
- `tools/list`
- `tools/call`

Legacy replies intentionally do not leak current-only `resultType` or cache fields.

This compatibility layer exists so TaskTrack can be useful with Codex, OpenCode, Hermes and similar hosts while they transition protocol eras.

## Development utilities

```powershell
TaskTrackMcp.exe --selftest
TaskTrackMcp.exe --oneshot request.json
```

`--selftest` verifies modern discovery, legacy envelope separation, tool discovery, durable creation-before-return, task polling, completion result retrieval and unsupported-protocol rejection.
