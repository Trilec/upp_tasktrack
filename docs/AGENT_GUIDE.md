# TaskTrack Agent Guide

## Purpose

TaskTrack is for facts that genuinely require a person: judgement, preference, visual comparison, interactive behaviour, wording, placement, colour, prioritisation, hierarchy, numeric bounds, or another decision repository evidence/tests/tools cannot establish reliably.

Do not use TaskTrack merely because asking a person is easier. Use machine evidence first.

## Assembly algorithm

When work reaches a human-dependent boundary:

1. Give the task one short objective.
2. Ask only the decisions needed to continue.
3. Keep related decisions in one task.
4. Use one independent decision per item.
5. Choose the semantic type by answer meaning, never by GUI widget.
6. Make `title` the question; use `instruction` only for useful criteria/context.
7. Use few categories.
8. Set `required=true` only when the decision really blocks the normal continuation path.
9. **Provide `recommended` unless no responsible proposal is possible.** A missing recommendation on a required item means the human must actually decide it.
10. Keep `recommended` and `default` separate. Recommendation = agent proposal. Default = neutral control starting value. Neither is human evidence until the human acts.
11. Create durably, retain `task_id`, then poll `get_task` / `tasks/get` later.
12. Consume `items[].answer.data` as authoritative human evidence.

## Semantic types

- yes/no proposition → `confirm`
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

## Recommendations are the normal fast path

For every remaining human decision ask:

> Given the evidence I already have, can I responsibly propose the answer?

If yes, set `recommended` to the canonical semantic value. If no, omit it. Do not invent a recommendation merely to avoid human input.

Examples:

- `confirm` → `Yes` / `No` (or custom display label)
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

- **Grey — suggested/normal:** an agent recommendation is available. This is the expected baseline for most questions.
- **Orange — required pending:** required item with no recommendation. The human must decide or request agent assistance.
- **Green — resolved:** the human answered manually or explicitly accepted the proposal.
- **Red — escalated required:** a required item still remains after the human tried to accept/submit/continue.

Normal path: grey → green.

Exceptional path: orange → red if still blocking → green.

## Human → agent assistance

The TaskTrack GUI may ask the agent for help while the window remains open. Requests are durable and survive client/MCP restarts.

The wire vocabulary is deliberately compact:

```text
propose_answer
clarify  mode=simplify
```

`get_task` returns:

```text
agent_action_required: true|false
pending_requests: [...] 
```

If `agent_action_required=true`, process pending requests before waiting again.

### `propose_answer`

Required response:

```text
recommended
```

Return a valid canonical recommendation for the referenced item. Do not write or imply a human answer.

### `clarify`

For `mode=simplify`, required response:

```text
clarification
```

Keep it concise, plain-language, and directly useful to the person. You may also return:

```text
recommended
```

when the clarification makes a responsible proposal possible.

Resolve both request types with `respond_to_request`.

Agent responses are advisory. They never become `answer.data` until the human explicitly acts.

## Durable/reconnect behaviour

Task creation is persisted before `create_task` returns. Human→agent assistance is persisted in a separate `<task>.agent.json` sidecar so agent replies cannot race GUI answer autosave or masquerade as human evidence.

A typical flow is:

1. `create_task`
2. retain `task_id`
3. poll `get_task`
4. if `agent_action_required`, process each `pending_requests` entry and call `respond_to_request`
5. poll later for human completion
6. consume `items[].answer.data`

If the client disconnects, reconnect with the same `task_id`. If task context is lost, use `list_tasks` to rediscover active work. TaskTrack does not expire or auto-close active human work.

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

## Result handling

Use `items[].answer.data` as the authoritative human response. `answer.value` is compact display/log text. Never silently substitute an agent recommendation or neutral default for human evidence.
