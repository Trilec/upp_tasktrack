---
name: tasktrack
description: Use TaskTrack for durable human decisions, approvals, selections, classifications, preferences, visual or interaction verification, Pass/Fail checks, wording, placement, colour, prioritisation, hierarchy, numeric bounds, and other human evidence the agent cannot establish from available repository, test, tool, or machine evidence. Also use it when the user explicitly asks for TaskTrack. Keep the TaskTrack interaction live until terminal human state. Never require the human to send a follow-up chat message merely to wake the agent. Never read TaskTrack JSON as the normal answer transport.
license: Apache-2.0
metadata:
  version: "0.2"
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
- `closed` — task ended without additional required human evidence.

Human-to-agent assistance is explicitly non-terminal:

`awaiting_human -> awaiting_agent -> awaiting_human`

`interaction_state=awaiting_agent` or `task_terminal=false` means the TaskTrack task remains open. A compatibility tool result is only a continuation checkpoint; it is never permission to abandon the TaskTrack workflow.

## Primary live lifecycle

For normal use call `create_task` with GUI launch enabled and keep ownership of the interaction until it reaches a task-terminal state.

After calling `create_task`:

1. If the call remains active, wait. Do not ask the human to tell you when they have finished.
2. If it returns completed human evidence, consume `items[].answer.data` and continue immediately.
3. If it returns closed/cancelled with no answer, treat that as no human evidence. Do not invent an answer.
4. If it returns `input_required`, let the MCP multi-round-trip complete and continue the same TaskTrack interaction.
5. If it returns `interaction_state=awaiting_agent`, `task_terminal=false`, `compatibility_fallback`, `interaction_blocked=true`, or `agent_action_required=true`, **the task is still open**. Do not stop the turn and do not ask the human to wake you. Run the compatibility continuation below immediately.

A terminal TaskTrack result is not permission to end the assistant turn silently. After `completed`, always send a concise user-visible acknowledgement that TaskTrack completed and summarize or act on the returned human evidence. After `closed`, always send a concise user-visible acknowledgement that the task was cancelled/closed and that no additional human evidence was supplied. Never leave the user with only a TaskTrack tool card as the final visible output.

JSON files are durability/recovery storage only. Never inspect `.tasktrack.json` or `.agent.json` to obtain the normal human answer.

## Compatibility continuation — mandatory when sampling/MRTR is unavailable

When `create_task` exposes pending human->agent requests, process them immediately. This is a non-terminal assistance round, not task completion.

For every entry in `pending_requests`:

- `propose_answer`: derive one responsible canonical recommendation from the current task context and call `respond_to_request` with that request id and `recommended`.
- `clarify`: produce a concise plain-language restatement and call `respond_to_request` with that request id and `clarification`. Include `recommended` only when a responsible proposal is also possible.
- `continue_with_judgement`: call `respond_to_request` with the request id and no answer payload, then continue the delegated agent work as authorized. Never synthesize human `answer.data`.

After resolving all pending requests, TaskTrack should report `interaction_state=awaiting_human`. Immediately call:

`get_task(task_id, include_items=true, wait_ms=300000)`

Then:

- if completed: consume `items[].answer.data`, continue the originating work, and acknowledge completion visibly;
- if closed: continue with no human evidence and acknowledge the cancellation visibly;
- if new `interaction_state=awaiting_agent` / `agent_action_required=true`: resolve the new pending requests and call `get_task(..., wait_ms=300000)` again;
- if still `awaiting_human`: remain in the wait loop;
- if the wait times out while the durable task remains active: call `get_task(..., wait_ms=300000)` again when continued waiting is still appropriate.

The human must never need to send “done”, “I submitted”, “check TaskTrack”, or similar merely to resume the agent.

## Recommendations are advisory

`recommended`, proposals returned by `propose_answer`, neutral defaults, and agent clarification are not human evidence.

Only explicit human action creates authoritative `items[].answer.data`.

When the human accepts a returned proposal, consume the resulting human answer normally after TaskTrack completes.

## Result authority

Use:

- `items[].answer.data` — authoritative human evidence;
- `answer.value` — display/log representation only;
- `answer.note` — optional supporting human evidence, never a substitute for `answer.data`.

Never silently substitute an agent recommendation, neutral default, task title, sidecar value, or inferred response for missing human evidence.
