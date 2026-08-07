# TaskTrack Semantic Question Types

TaskTrack exposes human **decision semantics**, not GUI widget names. An agent describes what it needs from the human; the renderer decides which U++ control or composition best communicates that request.

## Canonical V0.2 vocabulary

| Type | Use when the human needs to… | Main request fields | Structured `answer.data` |
| --- | --- | --- | --- |
| `confirm` | confirm or refuse one proposition | optional two `choices` labels | boolean |
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

## Renderer policy

The renderer is deliberately free to adapt presentation to option count and available width. For example, a small `single_choice` set can use compact pill/radio controls while a larger set can collapse to a dropdown. This is not a schema change: the human decision is still “choose exactly one”.

The current U++ implementation uses existing Ui controls wherever possible and only introduces four small specialist controls:

- `TaskTrackPosition9`
- `TaskTrackDirection8`
- `TaskTrackRangeSelector`
- `TaskTrackGradientSelector`

Those class names are implementation details and are not valid MCP question types.

## Recommended is not an answer

`recommended` is a visible agent suggestion. It must never mark a question answered or silently select an option. The human remains the authority for the response.

## Required means blocking

Set `required=true` only when the calling workflow genuinely cannot continue without the answer. TaskTrack refuses final submission while any required item remains unanswered.

## Free text is the escape hatch

Prefer a structured type whenever the answer can be represented precisely. Use `notes` when the useful response is inherently explanatory or exceptional. TaskTrack intentionally does not place a generic notes field under every question because that would consume space and encourage unstructured answers.

## Compatibility aliases

Schema V2 writes only the canonical names above. The loader still accepts V0.1 task types and normalizes them in memory:

- `check` → `confirm`
- `choice`, `pass_fail`, `file`, `interaction`, `visual_compare` → `single_choice`
- `multiline` → `notes`
- `colour` → `color`
- `rank` → `rank_order`

Legacy verification types that carried fixed verdict sets receive equivalent choices during migration.
