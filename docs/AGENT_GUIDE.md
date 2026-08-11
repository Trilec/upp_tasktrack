# TaskTrack Agent Guide

## What TaskTrack is for

TaskTrack is the hand-off point for facts that genuinely require a person: judgement, preference, visual comparison, interactive behaviour, wording, placement, colour, prioritisation, hierarchy, numeric bounds, or another decision normal code/tests/tools cannot establish reliably.

Do **not** use TaskTrack merely because asking a person is easier. First inspect the repository, run the relevant test/tool, and use machine-readable evidence where it exists.

## The agent assembly algorithm

When work reaches a genuinely human-dependent boundary:

1. **State what is blocked.** Give the task one short objective in `title`; use `subtitle` only for one-line context.
2. **Ask the minimum needed to continue.** Do not turn a small decision into a survey.
3. **Keep related decisions together.** Prefer one coherent TaskTrack task over a series of modal-style interruptions.
4. **Use one independent human decision per item.** Split unrelated decisions; do not split one decision into several near-duplicates.
5. **Choose the semantic type by answer meaning, not by the control you imagine.** The agent never names U++ classes or specifies layout geometry.
6. **Write the item `title` as the actual question.** Use `instruction` for one concrete criterion/action rather than repeating the title.
7. **Use few categories.** Add a category only when it materially improves scanning; do not create one category per item.
8. **Set `required=true` only when the work really cannot continue without that answer.** Optional qualification should remain optional.
9. **Use `recommended` only when you have a defensible preference.** It is advisory display text, never human evidence and never a pre-answer.
10. **Use `default` only as a neutral initial value.** A default is not a recommendation and does not make an item answered.
11. **Create durably, then continue asynchronously.** Keep the returned `task_id`; call `get_task` / `tasks/get` later rather than holding the original tool call open.
12. **Consume `answer.data` as authority.** `answer.value` is compact display/log text; Markdown is a derived export.

## Choosing the semantic type

- yes/no proposition → `confirm`
- exactly one named alternative → `single_choice`
- several independent alternatives → `multi_choice`
- compact populated lookup → `select`
- larger visible lookup → `list_select`
- short exact wording → `text`
- explanation, exception, constraint, or qualification → `notes`
- exact numeric engineering value → `number`
- subjective magnitude on a numeric scale → `amount`
- acceptable lower/upper bounds → `range`
- small ordinal score → `rating`
- one colour → `color`
- visual gradient treatment → `gradient`
- 3×3 spatial placement/alignment → `position`
- 8-way opening/travel/orientation → `direction`
- priority or sequence → `rank_order`
- component/location in a hierarchy → `hierarchy_select`
- easing/falloff/response shape → `curve`

If a precise structured type exists, prefer it over `text` or `notes`.

## Good question construction

Good:

```json
{
  "id": "panel-direction",
  "category": "Behaviour",
  "type": "direction",
  "title": "Where should this panel open?",
  "instruction": "Choose the direction that best preserves access to the canvas.",
  "required": true,
  "recommended": "east"
}
```

Avoid:

```json
{
  "type": "direction",
  "title": "Direction",
  "instruction": "Pick a direction.",
  "widget": "UiMatrixSelector",
  "width": 280
}
```

The second request makes the agent a GUI author. TaskTrack deliberately owns presentation.

## Categories

Use categories only when they improve navigation. Prefer a few short nouns such as:

`Decision`, `Layout`, `Behaviour`, `Visual`, `Content`, `Validation`.

A single-category task does not need category navigation. Do not create one category per question.

## Recommendations and defaults are different

`recommended` means the agent has a preference but the human remains the authority. It is shown separately and does not mark the item answered.

```json
{
  "type": "single_choice",
  "title": "Which implementation direction?",
  "choices": ["Minimal", "Balanced", "Advanced"],
  "recommended": "Balanced"
}
```

`default` is only a neutral initial control value, useful for semantics such as `amount`, `range`, `position`, `direction`, and `curve`. A default is not evidence and is not a substitute for an answer.

## Use structured questions instead of prose

Prefer:

```json
{
  "type": "multi_choice",
  "title": "What should this correction include?",
  "choices": ["Layout", "Styling", "Behaviour", "Tests"]
}
```

over:

```json
{
  "type": "notes",
  "title": "Tell me what should change"
}
```

when the alternatives are already known. Free text is for information that cannot be represented cleanly by a structured type.

## Do not over-ask

Before creating a task, remove any item whose answer can be obtained by:

- reading source/configuration;
- running a deterministic test;
- inspecting machine-readable output;
- calling an available tool;
- applying an already-established project rule.

If only one human decision remains, create one item. If several related decisions remain, group them in one task with a small number of categories.

## Compact request example

```json
{
  "project": "UiControls",
  "title": "Resolve panel behaviour before implementation continues",
  "subtitle": "Three decisions are still genuinely subjective",
  "reminder_minutes": 60,
  "nudge_on_agent_poll": true,
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
      "id": "scope",
      "category": "Behaviour",
      "type": "multi_choice",
      "title": "What should this correction include?",
      "instruction": "Select every independent area that should change.",
      "choices": ["Layout", "Styling", "Behaviour", "Tests"],
      "required": true
    },
    {
      "id": "accent",
      "category": "Visual",
      "type": "color",
      "title": "Which accent colour should be used?",
      "colors": ["#2F6FED", "#7C4DFF", "#00A878"],
      "recommended": "#2F6FED",
      "required": true
    }
  ]
}
```

## Long human waits

TaskTrack persists the request before `create_task` returns. A person may answer immediately, pause for hours, or return the next day. `paused` is active durable work, not failure or abandonment.

Never infer that an old task should be cancelled. TaskTrack closes work only through an explicit human/agent close action.

A typical agent flow is:

1. call `create_task`;
2. retain `task_id`;
3. continue other safe work or yield;
4. later call `get_task` / `tasks/get`;
5. if still unfinished, do not fabricate an answer;
6. when complete, consume the structured results and continue the blocked work.

## Result handling

Use:

```text
items[].answer.data
```

as the authoritative response. `answer.value` is compact human-readable text for logs/export. Do not scrape Markdown when structured MCP/JSON result data is available.

For required items, treat TaskTrack completion as the durable signal that the human supplied the blocking evidence; do not silently substitute the agent's earlier recommendation or a neutral default.
