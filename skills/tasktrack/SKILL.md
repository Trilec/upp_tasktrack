---
name: tasktrack
description: Use TaskTrack for durable human decisions, approvals, selections, classifications, preferences, visual or interaction verification, Pass/Fail checks, wording, placement, colour, prioritisation, hierarchy, numeric bounds, and other human evidence the agent cannot establish from available repository, test, tool, or machine evidence. Also use it when the user explicitly asks for TaskTrack. Keep the TaskTrack interaction live until terminal human state. Never require the human to send a follow-up chat message merely to wake the agent. Never read TaskTrack JSON as the normal answer transport.
license: Apache-2.0
metadata:
  version: "0.2.1"
---

# TaskTrack

TaskTrack is a durable human-evidence tool, not ordinary chat.

## Invoke only when appropriate

Invoke TaskTrack when the workflow needs a human decision, approval, selection, classification, preference, visual/interaction verification, Pass/Fail verdict, wording, placement, colour, prioritisation, hierarchy choice, numeric bound, or another answer that available machine evidence cannot reliably establish.

Do not invoke TaskTrack for ordinary conversation, explanations, summaries, general opinions, or information requests unless the user explicitly asks for TaskTrack recording.

Ask only the minimum human evidence required to continue. Prefer structured semantic types. For ordinary verification prefer `confirm` with `choices=["Pass","Fail"]`.

## Lifecycle authority

Task completion and assistance are different state machines.

Task-terminal states are only:

- `completed` — authoritative human evidence is ready;
- `closed` — the human-facing task ended without requiring more human input. `closed` may be an ordinary cancellation, or an explicit `Use judgement` delegation recorded in the assistance sidecar.

Human-to-agent assistance is explicitly non-terminal while a request is pending:

`awaiting_human -> awaiting_agent -> awaiting_human`

`interaction_state=awaiting_agent` or `task_terminal=false` means the TaskTrack task remains open. A compatibility tool result is only a continuation checkpoint; it is never permission to abandon the TaskTrack workflow.

`Use judgement` is different from a human answer. After the agent acknowledges `continue_with_judgement`, TaskTrack may close automatically when every still-required unanswered item has been delegated. The terminal result then exposes `delegated_to_agent=true`, `closure_reason=agent_judgement`, and delegated item ids. Never create or infer `answer.data` for those items.

## Primary live lifecycle

For normal use call `create_task` with GUI launch enabled and keep ownership of the interaction until it reaches a task-terminal state.

After calling `create_task`:

1. If the call remains active, wait. Do not ask the human to tell you when they have finished.
2. If it returns completed human evidence, consume `items[].answer.data` and continue immediately.
3. If it returns `closed` with `delegated_to_agent=true`, continue immediately using your own judgement for `delegated_item_ids`. Do not fabricate human answers for those items.
4. If it returns ordinary closed/cancelled state, use any already-recorded human evidence that is present, but do not invent missing answers.
5. If it returns `input_required`, let the MCP multi-round-trip complete and continue the same TaskTrack interaction.
6. If it returns `interaction_state=awaiting_agent`, `task_terminal=false`, `compatibility_fallback`, `interaction_blocked=true`, or `agent_action_required=true`, **the task is still open**. Do not stop the turn and do not ask the human to wake you. Run the compatibility continuation below immediately.

A terminal TaskTrack result is not permission to end the assistant turn silently. After `completed`, always send a concise user-visible acknowledgement that TaskTrack completed and summarize or act on the returned human evidence. After delegated closure, acknowledge that the human delegated the decision and continue with your judgement. After ordinary `closed`, acknowledge that the human task ended and that no further human evidence will be supplied. Never leave the user with only a TaskTrack tool card as the final visible output.

JSON files are durability/recovery storage only. Never inspect `.tasktrack.json` or `.agent.json` to obtain the normal human answer.

## Compatibility continuation — mandatory when sampling/MRTR is unavailable

When `create_task` exposes pending human->agent requests, process them immediately. This is a non-terminal assistance round, not task completion.

For every entry in `pending_requests`:

- `propose_answer`: derive one responsible canonical recommendation from the current task context and call `respond_to_request` with that request id and `recommended`.
- `clarify`: produce a concise plain-language restatement and call `respond_to_request` with that request id and `clarification`. Include `recommended` only when a responsible proposal is also possible.
- `continue_with_judgement`: call `respond_to_request` with the request id and no answer payload. Do not synthesize human `answer.data`. Once the delegation is acknowledged, TaskTrack may close the human dialog automatically and return a delegated terminal result.

After resolving pending requests, immediately call:

`get_task(task_id, include_items=true, wait_ms=300000)`

Then:

- if completed: consume `items[].answer.data`, continue the originating work, and acknowledge completion visibly;
- if closed with `delegated_to_agent=true`: continue using your own judgement for the delegated items and acknowledge the delegation visibly;
- if ordinary closed: continue with any evidence already present, do not invent missing evidence, and acknowledge closure visibly;
- if new `interaction_state=awaiting_agent` / `agent_action_required=true`: resolve the new pending requests and call `get_task(..., wait_ms=300000)` again;
- if still `awaiting_human`: remain in the wait loop;
- if the wait times out while the durable task remains active: call `get_task(..., wait_ms=300000)` again when continued waiting is still appropriate.

The human must never need to send “done”, “I submitted”, “check TaskTrack”, or similar merely to resume the agent.

## Recommendations are advisory

`recommended`, proposals returned by `propose_answer`, neutral defaults, and agent clarification are not human evidence.

Only explicit human answer/acceptance creates authoritative `items[].answer.data`. Explicit `Use judgement` creates durable delegation authority in the assistance sidecar, not human answer evidence.

When the human accepts a returned proposal, consume the resulting human answer normally after TaskTrack completes.

## Result authority

Use:

- `items[].answer.data` — authoritative human answer evidence;
- `answer.value` — display/log representation only;
- `answer.note` — optional supporting human evidence, never a substitute for `answer.data`;
- `delegated_to_agent=true` + `delegated_item_ids` — explicit authority to use agent judgement for those items, without claiming the resulting judgement came from the human.

Never silently substitute an agent recommendation, neutral default, task title, sidecar value, or inferred response for missing human evidence.
