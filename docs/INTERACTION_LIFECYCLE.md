# TaskTrack Interaction Lifecycle

TaskTrack separates **task completion** from **human-to-agent assistance**.

This distinction is normative. A human request for Suggest/Clarify/agent judgement must never close the TaskTrack task or be mistaken for completed human evidence.

## 1. Terminal task states

Only the main TaskTrack document decides whether the human task has ended.

Terminal states:

- `completed` — all required human evidence has been explicitly completed;
- `closed` — the human ended/cancelled the task without additional required evidence.

All other task states are non-terminal.

`items[].answer.data` remains the authoritative human evidence. Agent recommendations and assistance responses never become human evidence by themselves.

## 2. Assistance interaction states

The separate `.agent.json` assistance channel carries a non-terminal interaction phase:

```text
awaiting_human
    |
    | human presses Suggest / Clarify / Use judgement
    v
awaiting_agent
    |
    | agent response resolves all pending assistance requests
    v
awaiting_human
```

The sidecar persists `interaction_state` as either:

- `awaiting_human`
- `awaiting_agent`

Pending assistance requests are canonical. If any request is pending, the effective state is `awaiting_agent`; once none remain, it is `awaiting_human`. This also migrates legacy sidecars safely.

This phase is deliberately separate from the main task state. `awaiting_agent` is **not terminal** and must not close the GUI.

## 3. Normal direct human flow

```text
create_task
    -> GUI opens
    -> awaiting_human
    -> human answers
    -> Submit
    -> completed
    -> GUI closes
    -> create_task returns structured human evidence
```

For the normal live path the agent does not poll JSON or ask the human to announce completion.

## 4. Suggest / Clarify flow with MRTR-capable hosts

```text
human presses Suggest
    -> sidecar request pending
    -> interaction_state=awaiting_agent
    -> MCP input_required
    -> host fulfils agent/model request
    -> same create_task re-enters with inputResponses
    -> request becomes answered
    -> interaction_state=awaiting_human
    -> proposal appears in open GUI
    -> human Accepts
    -> if all required evidence is complete: completed + GUI closes
```

The in-call `input_required` result is an intermediate protocol round, not TaskTrack completion.

## 5. Compatibility continuation when host sampling is unavailable

Some hosts do not advertise the MCP sampling capability required for automatic agent assistance inside the same live tool call.

In that case TaskTrack must give the agent a continuation checkpoint so the model can act, but the human task remains open.

The structured result exposes:

- `interaction_state=awaiting_agent`
- `task_terminal=false`
- `agent_action_required=true`
- `agent_must_continue=true`
- `human_followup_required=false`
- `pending_requests=[...]`

The agent must immediately:

1. resolve every pending request with `respond_to_request`;
2. observe `interaction_state=awaiting_human` once no requests remain;
3. call `get_task(task_id, include_items=true, wait_ms=300000)`;
4. remain in that loop until `completed`, `closed`, or another `awaiting_agent` round occurs.

The compatibility tool round being returned to the model does **not** mean the TaskTrack task is complete. It exists only because a non-sampling host needs a model turn in order to fulfil the human's assistance request.

## 6. GUI feedback

The GUI should make the current assistance phase obvious:

- normal unresolved question — human decision controls available;
- request queued — `Request sent — waiting for agent`;
- returned proposal — `Suggested: ...` plus `Accept`;
- accepted final evidence — `Answer saved — returning to agent...`;
- terminal close without evidence — `Task closed — returning no human evidence...`.

The dialog remains open throughout `awaiting_agent`.

## 7. Accept and completion

Accepting an agent proposal is an explicit human act. The proposal is advisory until that action.

For a single required question, if Accept satisfies the final required item, Accept is also the finalization action: TaskTrack completes and closes automatically without requiring a second Submit click.

Ordinary text editing does not auto-complete on first edit because text controls persist incrementally; Submit remains the explicit final action for direct text entry.

## 8. Durable files

`.tasktrack.json` and `.agent.json` exist for durability, crash recovery, restart recovery, diagnostics, and offline inspection.

They are not the normal answer transport.

Normal delivery is:

```text
TaskTrack GUI -> TaskTrack MCP lifecycle -> agent
```

## 9. Agent-launched dialog sizing

Agent-launched windows are sized from the semantic question model rather than a fixed workspace size.

The estimator considers:

- number of items;
- item type;
- choice count;
- long instructions/titles;
- controls that naturally need vertical room such as notes, list selection, rank order, hierarchy, range and curve editors.

Short one-question tasks stay dialog-sized. Richer or multi-question tasks grow only as needed, then use scrolling rather than expanding beyond a sensible desktop working area. The result is clamped to the primary work area with a small margin.

## 10. Invariants

1. Suggest/Clarify/Use judgement never create human evidence.
2. `awaiting_agent` never closes the task or GUI by itself.
3. Agent response returns the assistance phase to `awaiting_human`.
4. Only explicit human completion or close/cancel reaches a terminal task state.
5. Agent recommendations remain advisory until explicit human acceptance.
6. JSON is durability, not normal transport.
7. The human must not need to send a chat message merely to wake the agent after completing TaskTrack.
