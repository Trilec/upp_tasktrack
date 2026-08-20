# TaskTrack Agent Guide

## Purpose

TaskTrack is for durable human evidence that repository inspection, tests, tools, or objective reasoning cannot establish reliably: judgement, approval, preference, visual comparison, interactive behaviour, wording, placement, colour, prioritisation, hierarchy, numeric bounds, classification, selection, or another genuinely human-dependent decision.

Also use TaskTrack when the user explicitly asks for TaskTrack recording.

Do not use TaskTrack merely because asking a person is easier. Use machine evidence first. Do not use it for ordinary conversation, explanations, summaries, or information requests.

## Simple verification is the fast path

For ordinary human verification, prefer the most direct form:

```json
{
  "type": "confirm",
  "choices": ["Pass", "Fail"],
  "title": "Does the result match the approved reference?"
}
```

TaskTrack shows Pass (green) / Fail (red) and a compact optional verdict note. `answer.data` is the boolean verdict authority; `answer.note` is optional supporting human evidence and never answers the question by itself. This remains the canonical `confirm` type, not a separate semantic type.

Use a richer semantic type (`text`, `notes`, `single_choice`, `color`, `range`, …) only when the requested evidence genuinely requires it.

## Assembly algorithm

When work reaches a human-dependent boundary:

1. Give the task one short objective.
2. Ask only the decisions needed to continue.
3. Keep related decisions in one task.
4. Use one independent decision per item.
5. Choose the semantic type by answer meaning, never by GUI widget.
6. Make `title` the question; use `instruction` only for useful criteria/context.
7. Use few categories.
8. Set `required=true` only when the decision blocks the normal continuation path.
9. Provide `recommended` when a responsible canonical proposal is possible. Omit it when no responsible proposal exists.
10. Keep `recommended` and `default` separate. Recommendation = agent proposal. Default = neutral control starting value. Neither is human evidence until the human acts.
11. For normal launched work, call `create_task` and keep ownership of the interaction until TaskTrack returns terminal human state.
12. Consume `items[].answer.data` as authoritative human evidence.

## Semantic types

- yes/no proposition → `confirm` (use `choices=["Pass","Fail"]` for ordinary verification)
- exactly one named alternative → `single_choice`
- several independent alternatives → `multi_choice`
- compact populated lookup → `select`
- larger visible lookup → `list_select`
- short exact wording → `text`
- explanation/exception/qualification → `notes`
- exact numeric value → `number`
- subjective magnitude → `amount`
- lower/upper bounds → `range`
- ordinal score → `rating`
- one colour → `color`
- gradient treatment → `gradient`
- 3×3 placement → `position`
- 8-way direction → `direction`
- priority/sequence → `rank_order`
- hierarchy node → `hierarchy_select`
- easing/falloff shape → `curve`

Prefer a structured type over `text` or `notes` when one fits.

## Recommendations are advisory

For every remaining human decision ask:

> Given the evidence already available, can I responsibly propose the answer?

If yes, set `recommended` to the canonical semantic value. If no, omit it. Do not invent a recommendation merely to avoid human input.

Examples:

- `confirm` → `Yes` / `No` or the exact custom label
- `single_choice`, `select` → exact choice
- `multi_choice` → `Layout, Tests` or JSON array text
- `list_select` → exact choice; array form when multiple is allowed
- `text`, `notes` → proposed text
- `number`, `amount`, `rating` → numeric text
- `range` → `320,900`, `[320,900]`, or `{ "low":320, "high":900 }`
- `color` → `#RRGGBB`
- `gradient` → gradient id
- `position` → e.g. `center`
- `direction` → e.g. `east`
- `rank_order` → full ordered JSON array
- `hierarchy_select` → node id(s)
- `curve` → `[x1,y1,x2,y2]`

A recommendation remains advisory until the human accepts it or supplies another answer.

## Four human workflow states

TaskTrack owns these presentation states locally; they are not global Ui-role definitions.

- Grey — suggested/normal: an agent recommendation is available.
- Orange — required pending: required item with no recommendation.
- Green — resolved: the human answered manually or explicitly accepted the proposal.
- Red — escalated required: a required item still remains after an attempted continuation.

Normal path: grey → green.

Exceptional path: orange → red if still blocking → green.

## Primary live lifecycle

Normal `create_task` with GUI launch enabled is a live human interaction.

The intended path is:

1. agent calls `create_task`;
2. TaskTrack persists the task and launches the GUI;
3. the initiating MCP interaction remains owned until the human completes/cancels or an assistance round-trip is required;
4. final human evidence returns through MCP;
5. the agent continues without requiring a follow-up human chat message.

Do not ask the human to say “done”, “I submitted”, “check TaskTrack”, or similar merely to wake the agent.

When `create_task` returns completed human evidence, consume it immediately. When it returns closed/cancelled with no answer, treat that as no human evidence and do not fabricate an answer.

`launch=false` is the explicit detached/recovery path and may legitimately require later `get_task` calls.

## Human → agent assistance

The TaskTrack GUI may ask the agent for help while the window remains open. Requests are durable and survive client/MCP restarts.

The wire vocabulary is deliberately compact:

```text
propose_answer
clarify  mode=simplify
continue_with_judgement
```

Request lifecycle:

```text
pending -> answered -> (cancelled)
```

An answered assistance request is not a resolved human question. Only explicit human action creates `answer.data`.

### Modern multi-round-trip path

When the host exposes the required MCP capability, TaskTrack may return `resultType=input_required` from the active `create_task`. The client fulfils the requested input and retries the same TaskTrack call using `requestState` / `inputResponses`.

No manual polling is required on this path.

### Compatibility path — agent MUST continue automatically

Some hosts do not expose the required sampling/multi-round-trip capability. TaskTrack then returns a compatibility result containing:

```text
delivery: compatibility_fallback
interaction_blocked: true
agent_action_required: true
pending_requests: [...]
```

This is not a signal to stop and wait for another human chat message.

The agent must immediately process every pending request:

#### `propose_answer`

Derive one responsible canonical recommendation from the current context and call `respond_to_request` with:

```text
request_id
recommended
```

The returned recommendation is advisory only.

#### `clarify`

For `mode=simplify`, call `respond_to_request` with:

```text
request_id
clarification
```

Keep it concise and directly useful to the person. Include `recommended` only when a responsible proposal is also possible.

#### `continue_with_judgement`

The human explicitly delegates the blocked judgement back to the agent. Call `respond_to_request` with the request id and no semantic answer payload. Never synthesize `answer.data` or mark the question human-resolved.

After resolving all current pending requests, immediately call:

```text
get_task(task_id, include_items=true, wait_ms=300000)
```

Then loop:

- completed → consume `items[].answer.data` and continue;
- closed → continue with no human evidence;
- `agent_action_required=true` → resolve new pending requests and wait again;
- wait timeout while task remains active → continue waiting when still appropriate.

The human must not be responsible for restarting this loop.

## Durable/reconnect behaviour

Task creation is persisted before live waiting. Human→agent assistance is persisted in a separate `<task>.agent.json` sidecar so agent replies cannot race GUI answer autosave or masquerade as human evidence.

These files are durability/recovery state, not the normal answer transport.

Never read `.tasktrack.json` or `.agent.json` to obtain the routine human result. Use MCP results and `get_task` only for the defined live/recovery lifecycle.

If the client disconnects, reconnect with the same `task_id`. If task context is lost, use `list_tasks` to rediscover active work.

## Compact example

```json
{
  "project": "UiControls",
  "title": "Resolve panel behaviour",
  "items": [
    {
      "id": "direction",
      "category": "Behaviour",
      "type": "direction",
      "title": "Where should the panel open?",
      "instruction": "Choose the direction that preserves the most canvas space.",
      "required": true,
      "recommended": "east"
    },
    {
      "id": "label",
      "category": "Content",
      "type": "text",
      "title": "What should the primary label say?",
      "required": true
    }
  ]
}
```

`direction` starts in the normal grey proposal state. `label` starts orange because no responsible proposal was supplied; the human may answer it, request `propose_answer`, or ask for `clarify`/`simplify`.

## Result authority

Use `items[].answer.data` as the authoritative human response. `answer.value` is compact display/log text. `answer.note` is optional supporting human evidence and is never a substitute for `answer.data`.

Never silently substitute an agent recommendation, neutral default, task title, sidecar value, or inferred response for missing human evidence.
