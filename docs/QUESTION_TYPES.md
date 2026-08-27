# TaskTrack semantic question types

TaskTrack exposes human **decision semantics**, not GUI widget names. An agent describes what it needs from the human; the renderer chooses a compact native control or composition.

## Canonical vocabulary

| Type | Use when the human needs to… | Main request fields | Structured `answer.data` |
| --- | --- | --- | --- |
| `confirm` | confirm or refuse one proposition / binary verification | optional two `choices` labels (`["Pass","Fail"]` for ordinary verification) | boolean |
| `single_choice` | choose exactly one named alternative | `choices`, optional `recommended` | string |
| `multi_choice` | choose several independent alternatives | `choices` | array of strings |
| `select` | pick one item from a populated compact set | `choices` | string |
| `list_select` | select from a larger visible list | `choices`, `allow_multiple` | string or array |
| `text` | provide short exact text | — | string |
| `notes` | explain an exception, constraint, qualification or instruction | — | string |
| `number` | enter an exact numeric value | `min`, `max`, `step`, `unit` | number |
| `amount` | choose a magnitude/feel on a scale | `min`, `max`, `step`, `unit`, optional `default` | number |
| `range` | choose low/high bounds | `min`, `max`, `step`, `unit`, optional `{low,high}` `default` | `{low,high}` |
| `rating` | provide a small ordinal score | `min`, `max` (normally 1–5) | number |
| `color` | choose a colour visually | optional `colors`, `recommended` | `#RRGGBB` string |
| `gradient` | choose between visual gradient treatments | `gradients`, optional `recommended` | gradient option id |
| `position` | choose a 3×3 placement/alignment | optional `default` | position token |
| `direction` | choose an 8-way direction | optional `default` | direction token |
| `rank_order` | put named items into priority/order | `choices` | ordered array of strings |
| `hierarchy_select` | choose one or more nodes in a hierarchy | `hierarchy`, `allow_multiple` | node id or array |
| `curve` | choose/edit easing, transition or falloff behaviour | optional four-number `default` | `[x1,y1,x2,y2]` |

## Pass/Fail verification

For ordinary human verification, prefer a `confirm` item with exactly `choices=["Pass","Fail"]`.

TaskTrack gives this pair a dedicated presentation:

- Pass is green and Fail is red, with text labels as well as colour;
- a compact optional verdict note is available;
- `answer.data` is boolean (`true` for Pass, `false` for Fail);
- `answer.value` is the display label;
- `answer.note` is supporting evidence and never answers the question by itself.

This remains the normal `confirm` type; it is not a nineteenth semantic type.

## Renderer policy

Presentation may adapt to the option count and available width without changing the question semantics. A short choice set can use visible controls while a larger set can use a dropdown or list. Range, colour and other visual questions use the existing `upp_Ui` controls where they fit and small TaskTrack adapters where a semantic composition is useful.

Those renderer classes are implementation details and are not valid MCP question types.

## Recommendations are advisory

`recommended` is an agent suggestion. It must never mark a question answered or silently select an option. The human creates evidence only through an explicit answer or by accepting the suggestion.

## Required means blocking

Set `required=true` only when the workflow genuinely needs human input before it can continue. A required item blocks normal Submit until it is answered or explicitly delegated through Use judgement.

## Free text is the escape hatch

Prefer a structured type whenever the answer can be represented precisely. Use `notes` when the useful response is inherently explanatory or exceptional. TaskTrack intentionally does not put a generic notes editor under every question.

## Compatibility aliases

Schema 2 writes only the canonical names above. The loader still accepts schema 1 names and normalizes them in memory:

- `check` → `confirm`
- `choice`, `pass_fail`, `file`, `interaction`, `visual_compare` → `single_choice`
- `multiline` → `notes`
- `colour` → `color`
- `rank` → `rank_order`

Older verification types that carried fixed verdict sets receive equivalent choices during migration.
