# TaskTrack Agent Guide

## What TaskTrack is for

Use TaskTrack when the next useful fact requires a human decision, observation, visual judgement, or interactive action. Do not use it for facts that normal code/tests/tools can establish reliably.

Good examples:

- “Which of these three layout directions should I implement?”
- “Which components should be preserved?”
- “Choose the intended accent colour.”
- “Where should this panel open?”
- “Rank these implementation priorities.”
- “Which node in this hierarchy is the intended target?”
- “Shape the easing curve you want.”

## Agent decision rule

Ask one question per independent human decision. Choose the type by **meaning**, not by the control you imagine on screen.

- proposition with two outcomes → `confirm`
- exactly one named alternative → `single_choice`
- several independent alternatives → `multi_choice`
- compact populated lookup → `select`
- larger visible lookup → `list_select`
- short exact wording → `text`
- explanation/exception → `notes`
- exact numeric engineering value → `number`
- subjective magnitude on a numeric scale → `amount`
- acceptable low/high bounds → `range`
- small ordinal score → `rating`
- visual colour → `color`
- visual gradient treatment → `gradient`
- spatial alignment/placement → `position`
- travel/opening/orientation direction → `direction`
- priority/sequence → `rank_order`
- component/location in a tree → `hierarchy_select`
- easing/falloff/response shape → `curve`

Do not send `widget`, `UiRadioButton`, `UiSliderEdit`, or other toolkit names.

## Writing a good question

`title` should be the actual decision, normally one short line. `instruction` should add a concrete criterion or action rather than restating the title.

Good:

```json
{
  "type": "direction",
  "title": "Where should this panel open?",
  "instruction": "Choose the preferred opening direction."
}
```

Avoid:

```json
{
  "type": "direction",
  "title": "Direction",
  "instruction": "Pick a direction."
}
```

## Categories

Use a small number of category labels only when they improve scanning. The GUI automatically shows a category strip for multi-category tasks and hides it for single-category work.

Useful categories are short nouns such as `Decision`, `Input`, `Visual`, `Structure`, `Responsive`, or `Persistence`.

Do not create a category for every question.

## Recommendations

Use `recommended` when the agent has a preference but wants the human to decide. It is displayed separately as “Agent suggests: …” and does not pre-answer the question.

## Defaults

`default` is an initial value for controls that require a starting state, especially `amount`, `range`, `position`, `direction`, and `curve`. A default is not evidence and does not make the question answered.

## Long human waits

`create_task` returns only after the task has been saved durably. Do not hold the calling workflow open waiting for the human. Keep the returned `task_id`, then call `get_task`/`tasks/get` later.

A human may pause the task for minutes, hours, or a day. `paused` is active work, not failure or abandonment.

Never infer that an old task should be cancelled. TaskTrack closes work only through an explicit human/agent close action.

## Compact request example

```json
{
  "project": "UiDesigner",
  "title": "Choose the panel behaviour",
  "subtitle": "Three decisions needed before implementation continues",
  "reminder_minutes": 60,
  "nudge_on_agent_poll": true,
  "items": [
    {
      "id": "direction",
      "category": "Behaviour",
      "type": "direction",
      "title": "Where should this panel open?",
      "instruction": "Choose the preferred opening direction.",
      "required": true,
      "default": "east"
    },
    {
      "id": "scope",
      "category": "Behaviour",
      "type": "multi_choice",
      "title": "What should change?",
      "instruction": "Select every area the implementation should include.",
      "choices": ["Layout", "Style", "Behaviour", "Tests"],
      "required": true
    },
    {
      "id": "accent",
      "category": "Visual",
      "type": "color",
      "title": "Choose the accent colour",
      "colors": ["#2F6FED", "#7C4DFF", "#00A878", "#E26D2F"],
      "recommended": "#2F6FED",
      "required": true
    }
  ]
}
```

## Result handling

Treat `answer.data` as authoritative structured evidence. `answer.value` exists as a compact human-readable representation for logs/export.

Do not scrape the Markdown export when structured MCP/JSON result data is available.
