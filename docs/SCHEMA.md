# TaskTrack Persisted Schema — V1

The persisted document is UTF-8 JSON. `schema_version` is currently `1`.

## Document

```json
{
  "schema_version": 1,
  "tasktrack_version": "0.1.0",
  "task_id": "task-...",
  "project": "Ui",
  "title": "Verify responsive button sizing",
  "subtitle": "Human visual acceptance",
  "actor": "agent",
  "created_at": "2026-08-07T06:00:00Z",
  "updated_at": "2026-08-07T06:10:00Z",
  "last_human_activity_at": "2026-08-07T06:10:00Z",
  "last_human_activity_epoch": 1786083000,
  "state": "in_progress",
  "reminder_minutes": 60,
  "remind_while_paused": false,
  "nudge_on_agent_poll": true,
  "reminder_count": 0,
  "history_limit": 20,
  "items": []
}
```

## States

- `awaiting_human`
- `in_progress`
- `paused`
- `completed`
- `closed`

No state is inferred from elapsed time.

## Item

```json
{
  "id": "mobile",
  "category": "Responsive",
  "type": "visual_compare",
  "title": "Mobile proportions",
  "instruction": "Confirm controls reduce proportionally.",
  "required": true,
  "choices": [],
  "expected_color": "",
  "expected_value": "",
  "answer": {
    "answered": false,
    "status": "",
    "value": "",
    "note": "",
    "answered_at": ""
  }
}
```

## Types

`check`
: boolean confirmation. Answer status is `confirmed` and value is `true` when checked.

`pass_fail`
: one of Pass, Fail, Blocked, Not applicable.

`choice`
: one agent-supplied choice. At least one choice is required by validation.

`text`
: one-line observation.

`multiline`
: longer human evidence.

`number`
: V0.1 captures the numeric observation as text so the exact human evidence is preserved; numeric constraints can be added later without changing the outer document.

`color`
: expected colour swatch plus Match, Different or Unsure.

`file`
: Found, Missing, Wrong output or Unsure.

`interaction`
: Pass, Fail, Partial or Blocked.

`visual_compare`
: Match, Different or Unsure.

## Validation

The loader rejects malformed field types rather than coercing them. Examples:

- `items` must be an array;
- `choices` must be an array of strings;
- `required` must be boolean;
- duplicate item IDs are rejected;
- unsupported item types are rejected;
- colour expectations must be `#RRGGBB` or `#RRGGBBAA` when supplied;
- empty task titles and empty item titles are rejected.

## Files

Primary:

`<task-id>.tasktrack.json`

Recovery:

`<task-id>.tasktrack.json.bak`

Temporary save:

`<task-id>.tasktrack.json.tmp`

Agent-poll signal:

`<task-id>.tasktrack.json.poll`

Per-task locator:

`<default-store>/registry/<task-id>.path`
