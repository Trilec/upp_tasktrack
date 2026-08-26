# TaskTrack MCP contract

`TaskTrackMcp.exe` is a local STDIO MCP server over `TaskTrack/Core`. It runs beside `TaskTrackGui.exe` and launches the GUI with `--task <path>` when a task needs a person.

## Tools

- `version` — TaskTrack version, build and protocol information.
- `create_task` — create durable structured human input and normally own the live interaction until terminal state.
- `get_task` — retrieve/recover task state and structured answers; also used by compatibility continuation.
- `open_task` — reopen the GUI for an existing task.
- `list_tasks` — list recent durable tasks.
- `close_task` — explicitly close unfinished work.
- `respond_to_request` — answer a pending human-to-agent assistance request.

## Question vocabulary

`create_task.items[].type` accepts the 18 canonical types documented in `QUESTION_TYPES.md`:

`confirm`, `single_choice`, `multi_choice`, `select`, `list_select`, `text`, `notes`, `number`, `amount`, `range`, `rating`, `color`, `gradient`, `position`, `direction`, `rank_order`, `hierarchy_select`, `curve`.

Agents specify decision meaning, not U++ widget names.

For ordinary verification, `confirm` with `choices=["Pass","Fail"]` is the compact fast path. Its `answer.data` is boolean; an optional verdict note is supporting evidence.

## Normal live flow

With `launch=true` (the default):

```text
create_task
  -> validate and persist task
  -> launch TaskTrackGui.exe
  -> human works
  -> completed / closed
  -> structured terminal result returns
```

The normal path does not require manual JSON inspection or a second human chat message.

`launch=false` is the explicit detached/recovery mode.

## Human evidence

On normal completion:

- `items[].answer.data` is the canonical structured human answer;
- `answer.value` is a compact display/log representation;
- `answer.note` is optional supporting human evidence.

Recommendations and agent responses are advisory until explicitly accepted by the human.

## Assistance lifecycle

The human can request three kinds of agent assistance:

- `propose_answer` — return a responsible recommendation;
- `clarify` — return a simpler explanation, optionally with a recommendation;
- `continue_with_judgement` — acknowledge that the human delegates this decision to the agent.

A pending request moves the assistance phase to `awaiting_agent`. This is not task completion.

### Suggest and Clarify

After the agent responds, the task returns to `awaiting_human`. A suggestion is visible but does not create `answer.data`. The human must accept it explicitly.

### Use judgement

Use judgement is explicit delegation, not acceptance of an agent answer. If every still-required unanswered item is covered by an acknowledged `continue_with_judgement`, the human-facing task closes automatically.

The terminal result includes:

```text
state: closed
delegated_to_agent: true
closure_reason: agent_judgement
delegated_item_ids: [...]
human_followup_required: false
```

Delegated items retain empty human `answer.data`.

## Hosts with and without in-call assistance

When the host exposes the required modern sampling/multi-round capability, TaskTrack can service assistance inside the live interaction.

When it does not, TaskTrack returns a structured compatibility checkpoint containing pending requests. The agent should:

1. resolve each pending request with `respond_to_request`;
2. call `get_task(task_id, include_items=true, wait_ms=300000)`;
3. continue that loop until the task is `completed`, `closed`, or another assistance request appears.

This compatibility checkpoint is non-terminal. The human should not have to type “done” or “check TaskTrack” to resume the agent.

## Terminal continuation

A terminal result tells the host that the originating agent turn should continue visibly:

- `completed` — use and acknowledge the returned human evidence;
- ordinary `closed` — acknowledge closure and do not invent missing answers;
- delegated `closed` — acknowledge delegation and continue using agent judgement for `delegated_item_ids`.

## Persistence

`.tasktrack.json` and `.agent.json` are durability and recovery state. They are not the routine answer transport.

## CLI

```text
TaskTrackMcp.exe                run stdio server
TaskTrackMcp.exe --help         usage
TaskTrackMcp.exe --version      version / schema / protocol
TaskTrackMcp.exe --selftest     deterministic MCP self-test
TaskTrackMcp.exe --oneshot <request.json>
                                diagnostic one-request mode
```

For host registration, run the executable with no arguments.
