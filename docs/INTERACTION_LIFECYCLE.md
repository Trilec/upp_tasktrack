# TaskTrack Interaction Lifecycle

TaskTrack separates **human answer evidence** from **human-to-agent assistance/delegation**.

This distinction is normative. Suggest/Clarify and a pending Use judgement request are non-terminal assistance. An acknowledged Use judgement request is different: it is explicit human authorization for the agent to decide, and may end the human-facing task without creating human `answer.data`.

## 1. Terminal task states

Only the main TaskTrack document decides whether the human-facing task has ended.

Terminal states:

- `completed` — required human answer evidence has been explicitly completed;
- `closed` — the human-facing task ended without requiring more human input. This can be ordinary cancellation/closure, or an acknowledged delegation to agent judgement.

All other task states are non-terminal.

`items[].answer.data` remains the authoritative human **answer** evidence. Agent recommendations and assistance responses never become human answers by themselves. An explicit Use judgement action is preserved separately as durable delegation authority in `.agent.json`.

## 2. Assistance interaction states

The separate `.agent.json` assistance channel carries the live interaction phase:

```text
awaiting_human
    |
    | human presses Suggest / Clarify / Use judgement
    v
awaiting_agent
    |
    | agent response acknowledges/resolves pending request
    v
awaiting_human
```

The sidecar persists `interaction_state` as either:

- `awaiting_human`
- `awaiting_agent`

Pending assistance requests are canonical. If any request is pending, the effective state is `awaiting_agent`; once none remain, it is `awaiting_human`. This also migrates legacy sidecars safely.

`awaiting_agent` is **not terminal** and must not close the GUI merely because the agent has not responded yet.

For `propose_answer` and `clarify`, an answered request simply returns control to the human. For `continue_with_judgement`, an answered request means the agent has received the human's delegation. If every still-required unanswered item is now covered by an acknowledged delegation, TaskTrack closes the human-facing task automatically.

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

Clarify follows the same non-terminal round-trip but returns clarification rather than a proposal.

The in-call `input_required` result is an intermediate protocol round, not TaskTrack completion.

## 5. Use judgement delegation

Use judgement means: **the human explicitly authorizes the agent to decide this item rather than asking the human for an answer**.

```text
human presses Use judgement
    -> continue_with_judgement pending
    -> interaction_state=awaiting_agent
    -> agent acknowledges delegation
    -> request becomes answered
    -> no human answer.data is created
    -> if other required human work remains: awaiting_human, GUI stays open
    -> if no other required human work remains: main task -> closed, GUI closes
    -> terminal result exposes delegated_to_agent=true,
       closure_reason=agent_judgement, delegated_item_ids=[...]
    -> agent continues using its own judgement
```

Delegation is durable human authorization, but it is not a human answer. The agent must never present its resulting judgement as though it came from the human.

## 6. Compatibility continuation when host sampling is unavailable

Some hosts do not advertise the MCP sampling capability required for automatic agent assistance inside the same live tool call.

In that case TaskTrack gives the agent a continuation checkpoint so the model can act, while the human task remains open during the pending request.

The structured result exposes:

- `interaction_state=awaiting_agent`
- `task_terminal=false`
- `agent_action_required=true`
- `agent_must_continue=true`
- `human_followup_required=false`
- `pending_requests=[...]`

The agent must immediately:

1. resolve every pending request with `respond_to_request`;
2. call `get_task(task_id, include_items=true, wait_ms=300000)`;
3. remain in that loop until `completed`, `closed`, or another `awaiting_agent` round occurs.

For Suggest/Clarify, resolving the request normally returns the GUI to `awaiting_human`. For Use judgement, resolving the request may cause the GUI to close automatically if no further required human work remains.

The compatibility tool round being returned to the model does **not** mean the TaskTrack task is complete. It exists only because a non-sampling host needs a model turn in order to fulfil the human's assistance request.

A tested compatibility fallback that runs automatically without another human chat message is an accepted host path, not a TaskTrack failure merely because native MRTR/sampling was unavailable.

## 7. GUI feedback

The GUI should make the current phase obvious:

- normal unresolved question — human decision controls available;
- request queued — waiting for agent/clarification/judgement;
- returned proposal — `Suggested: ...` plus `Accept`;
- accepted final evidence — `Answer saved — returning to agent...`;
- acknowledged delegation completing the human task — `Judgement delegated — returning to agent...`;
- ordinary terminal close — `Task closed — returning no human evidence...`.

The dialog remains open throughout a pending `awaiting_agent` phase. It may close after acknowledged delegation when there is no remaining required human work.

## 8. Accept and completion

Accepting an agent proposal is an explicit human act. The proposal is advisory until that action.

For a single required question, if Accept satisfies the final required item, Accept is also the finalization action: TaskTrack completes and closes automatically without requiring a second Submit click.

Ordinary text editing does not auto-complete on first edit because text controls persist incrementally; Submit remains the explicit final action for direct text entry.

Use judgement is not Accept. It does not copy an agent answer into `answer.data`; it records delegation authority and lets the agent continue under its own judgement.

## 9. Durable files

`.tasktrack.json` and `.agent.json` exist for durability, crash recovery, restart recovery, diagnostics, and offline inspection.

They are not the normal answer transport.

Normal delivery is:

```text
TaskTrack GUI -> TaskTrack MCP lifecycle -> agent
```

## 10. Agent-launched dialog sizing

Agent-launched windows are sized from the semantic question model rather than a fixed workspace size.

The estimator considers:

- number of items;
- item type;
- choice count;
- long instructions/titles;
- controls that naturally need vertical room such as notes, list selection, rank order, hierarchy, range and curve editors.

Short one-question tasks stay dialog-sized but reserve enough height for the group-panel border and workflow row. Richer or multi-question tasks grow only as needed, then use scrolling rather than expanding beyond a sensible desktop working area. The result is clamped to the primary work area with a small margin.

## 11. Terminal result acknowledgement

A terminal TaskTrack tool result must not leave only a tool card visible in the host UI.

- `completed`: agent continues and visibly acknowledges/summarizes the returned human evidence;
- ordinary `closed`: agent visibly acknowledges that the human task ended and does not invent missing evidence;
- delegated `closed`: agent visibly acknowledges the delegation and continues using its own judgement for `delegated_item_ids`, without fabricating human answers.

No terminal path requires the human to send another chat message merely to wake the agent.

## 12. Invariants

1. Suggest/Clarify never create human evidence and never close the task merely because their request was answered.
2. A pending `awaiting_agent` request never closes the task or GUI by itself.
3. Agent response normally returns the assistance phase to `awaiting_human`.
4. Explicit acknowledged Use judgement may end the human-facing task when every remaining required item is delegated.
5. Delegation never writes `items[].answer.data` and must never be represented as a human answer.
6. Agent recommendations remain advisory until explicit human acceptance.
7. JSON is durability, not normal transport.
8. The human must not need to send a chat message merely to wake the agent after completing, cancelling, or delegating TaskTrack.
