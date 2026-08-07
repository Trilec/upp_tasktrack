# TaskTrack MCP Contract

TaskTrack MCP is a thin stdio bridge over the durable Core model.

## Tools

- `version` — version/schema/protocol information.
- `create_task` — durably create structured human input; optionally launch GUI.
- `get_task` — retrieve state and structured answers; also writes the optional poll-reminder marker.
- `open_task` — launch GUI for an existing task.
- `list_tasks` — recent tasks from a store.
- `close_task` — explicitly close unfinished work.

## Question vocabulary

`create_task.items[].type` accepts the 18 canonical types documented in `QUESTION_TYPES.md`:

`confirm`, `single_choice`, `multi_choice`, `select`, `list_select`, `text`, `notes`, `number`, `amount`, `range`, `rating`, `color`, `gradient`, `position`, `direction`, `rank_order`, `hierarchy_select`, `curve`.

The public MCP schema deliberately does **not** advertise V0.1 aliases even though the persistence loader accepts them for recovery/compatibility.

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

## Result authority

On completion, use `items[].answer.data` as structured evidence. `answer.value` is a compact display/log representation and Markdown is a derived export.
