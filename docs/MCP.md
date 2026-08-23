# TaskTrack MCP Contract

TaskTrack MCP is a thin stdio bridge over the durable Core model. `TaskTrackMcp.exe` is an argument-free local stdio server; it runs alongside `TaskTrackGui.exe` (the native human GUI) and launches it with `--task <path>` when a task needs a human.

The normative interaction state machine is documented in `INTERACTION_LIFECYCLE.md`.

## CLI

```text
TaskTrackGui.exe                choose an existing task
TaskTrackGui.exe --task <path>  open a specific task
TaskTrackGui.exe --help         GUI usage dialog
TaskTrackGui.exe --version      release + validation-build identity

TaskTrackMcp.exe                run the stdio MCP server (no stdout banner)
TaskTrackMcp.exe --help         MCP usage
TaskTrackMcp.exe --version      release/schema/protocol summary
TaskTrackMcp.exe --selftest     deterministic MCP self-test
TaskTrackMcp.exe --oneshot <request.json>   process one MCP JSON-RPC request file and exit
                                         (diagnostic/test utility)
```

## Release version vs validation build

TaskTrack keeps two identities during an acceptance cycle:

- `TaskTrackVersion()` is the accepted release line;
- `TaskTrackBuildVersion()` is the exact supervisor validation candidate.

The validation build changes whenever a new executable candidate is handed to Windows/Codex acceptance. `TaskTrackGui.exe --version` shows both values. Normal MCP task/status and assistance responses include `build_version`, so a live host transcript can prove which candidate binary actually answered the request.

A candidate such as `0.2.1-rc1` does not become release `0.2.1` until the acceptance gate passes. This prevents an unvalidated build from masquerading as a finished release while still making stale-binary mistakes obvious.

## Tools

- `version` — release/schema/protocol information.
- `create_task` — durably create structured human input and normally keep ownership of the live human interaction.
- `get_task` — recovery/compatibility retrieval of state, structured answers, and pending human→agent requests.
- `open_task` — launch GUI for an existing task.
- `list_tasks` — recent tasks from a store.
- `close_task` — explicitly close unfinished work.
- `respond_to_request` — resolve one pending human→agent assistance request.

## Question vocabulary

`create_task.items[].type` accepts the 18 canonical types documented in `QUESTION_TYPES.md`:

`confirm`, `single_choice`, `multi_choice`, `select`, `list_select`, `text`, `notes`, `number`, `amount`, `range`, `rating`, `color`, `gradient`, `position`, `direction`, `rank_order`, `hierarchy_select`, `curve`.

For ordinary human verification prefer `confirm` with `choices = ["Pass", "Fail"]` (boolean `answer.data`, optional `answer.note`). The public MCP schema deliberately does **not** advertise V0.1 aliases (e.g. `pass_fail`) even though the persistence loader accepts them for recovery/compatibility.

## Normal live workflow

Normal `create_task` with GUI launch enabled is a live human interaction:

```text
create_task
  -> GUI launches
  -> human works
  -> completed / closed
  -> structured result returns to the agent
```

The human does not announce completion in chat and the agent does not inspect JSON files for the normal result.

`launch=false` is the explicit detached/recovery path.

## Task state vs assistance state

Task completion and human-to-agent assistance are separate concerns.

Only the main task states `completed` and `closed` are terminal.

The assistance channel persists a separate, non-terminal interaction phase:

```text
awaiting_human -> awaiting_agent -> awaiting_human
```

When the human presses Suggest, Clarify, or Use judgement, a durable assistance request is queued and the effective interaction state becomes `awaiting_agent`. The GUI remains open. When all pending agent requests are answered, the interaction returns to `awaiting_human`.

MCP status exposes `interaction_state` and `task_terminal`. An assistance checkpoint must therefore be interpreted as:

```text
interaction_state: awaiting_agent
task_terminal: false
```

not as task completion.

A response that was already in flight may arrive just after the human independently completes or closes the task. TaskTrack is allowed to settle that advisory sidecar request so it does not remain pending forever. The terminal main task still wins: the late response never changes `answer.data`, never reopens the GUI, and never resurrects `agent_action_required`.

## Protocol eras

The server supports:

- modern stateless `2026-07-28` request metadata and discovery;
- the `io.modelcontextprotocol/tasks` extension when the modern client declares it;
- conservative legacy `initialize` / `notifications/initialized` behaviour for hosts that still use older MCP revisions.

For a modern host that advertises the required sampling/MRTR capability, Suggest/Clarify can be serviced as an in-call `input_required` round and the same `create_task` interaction resumes afterwards.

## Compatibility continuation when sampling is unavailable

Some hosts, including tested Codex configurations, may support TaskTrack MCP while not advertising sampling. The model must then be given a turn in order to fulfil a human→agent request.

TaskTrack returns an explicit compatibility continuation containing the pending requests. This protocol round is not the TaskTrack task ending. The structured status states that the task is non-terminal and the agent must continue.

The agent must:

1. call `respond_to_request` for every pending request;
2. observe the interaction returning to `awaiting_human`;
3. call `get_task(task_id, include_items=true, wait_ms=300000)`;
4. remain in the workflow until `completed`, `closed`, or another `awaiting_agent` round occurs.

The human must not need to send “done”, “check TaskTrack”, or similar merely to wake the agent.

## Polling

`get_task` is primarily a recovery/compatibility operation. A bounded `wait_ms` lets the agent remain blocked while the human is working after an assistance round.

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

## Dialog sizing

The native GUI remains a separate executable, but agent-launched windows are sized from the semantic question model rather than a fixed workspace size. Item count, type, choice count and richer controls contribute to an estimated useful dialog size. The result is clamped to the desktop work area, with scrolling preferred over an unnecessarily large window.

## Durable storage

`.tasktrack.json` and `.agent.json` are durability/recovery state, not normal answer transport.

## Result authority

On completion, use `items[].answer.data` as structured evidence. `answer.value` is a compact display/log representation; `answer.note` is optional supporting human evidence; Markdown is a derived export.
