# TaskTrack MCP Contract

TaskTrack MCP is a thin stdio bridge over the durable Core model. `TaskTrackMcp.exe` is an argument-free local stdio server; it runs alongside `TaskTrackGui.exe` (the native human GUI) and launches it with `--task <path>` when a task needs a human.

## CLI

```text
TaskTrackGui.exe                choose an existing task
TaskTrackGui.exe --task <path>  open a specific task
TaskTrackGui.exe --help         GUI usage dialog
TaskTrackGui.exe --version      version dialog

TaskTrackMcp.exe                run the stdio MCP server (no stdout banner)
TaskTrackMcp.exe --help         MCP usage
TaskTrackMcp.exe --version      version/schema/protocol summary
TaskTrackMcp.exe --selftest     deterministic MCP self-test
```

## Tools

- `version` — version/schema/protocol information.
- `create_task` — durably create structured human input; optionally launch GUI.
- `get_task` — retrieve state, structured answers, and pending human→agent requests.
- `open_task` — launch GUI for an existing task.
- `list_tasks` — recent tasks from a store.
- `close_task` — explicitly close unfinished work.
- `respond_to_request` — resolve one pending human→agent assistance request.

## Question vocabulary

`create_task.items[].type` accepts the 18 canonical types documented in `QUESTION_TYPES.md`:

`confirm`, `single_choice`, `multi_choice`, `select`, `list_select`, `text`, `notes`, `number`, `amount`, `range`, `rating`, `color`, `gradient`, `position`, `direction`, `rank_order`, `hierarchy_select`, `curve`.

For ordinary human verification prefer `confirm` with `choices = ["Pass", "Fail"]` (boolean `answer.data`, optional `answer.note`). The public MCP schema deliberately does **not** advertise V0.1 aliases (e.g. `pass_fail`) even though the persistence loader accepts them for recovery/compatibility.

## Durable asynchronous-by-design workflow

TaskTrack does not require the original `create_task` invocation to stay alive while a human works. The task file exists before the id/handle is returned. The caller stores `task_id` and polls later.

This is the preferred behaviour even if a host supports very long tool-call timeouts.

## Protocol eras

The server supports:

- modern stateless `2026-07-28` request metadata and discovery;
- the `io.modelcontextprotocol/tasks` extension when the modern client declares it;
- conservative legacy `initialize` / `notifications/initialized` behaviour for hosts that still use older MCP revisions.

Modern task-capable clients receive a task handle from `create_task` and can call `tasks/get`. Other clients receive an ordinary tool result containing the same stable TaskTrack task id.

## Polling

`get_task` and `tasks/get` may update a separate poll marker when `nudge_on_agent_poll` is enabled. This marker is not human evidence and does not change task state. The GUI may use it to ask an inactive human whether they are still working.

## Human → agent assistance

`get_task` exposes `agent_action_required` plus compact `pending_requests[]` (`id`, `item_id`, `action`, and `mode` where applicable). Resolve them with `respond_to_request`.

Canonical actions:

- `propose_answer` → `recommended` required.
- `clarify` (`mode=simplify`) → `clarification` required; `recommended` optional.
- `continue_with_judgement` → no response payload (the human delegates the blocked judgement back to the agent).

Request lifecycle:

```text
pending -> answered -> (cancelled)
```

An answered request is not a resolved human question. Agent responses are advisory and never write `TaskTrackAnswer`; only explicit human action creates `answer.data`.

## Result authority

On completion, use `items[].answer.data` as structured evidence. `answer.value` is a compact display/log representation; `answer.note` is optional supporting human evidence; Markdown is a derived export.
