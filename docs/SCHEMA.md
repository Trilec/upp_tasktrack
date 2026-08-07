# TaskTrack Persistence Schema

## Authority

TaskTrack JSON is the canonical durable state. Markdown is a derived human-readable export. The current writer emits `schema_version: 2`.

## Top-level document

Important fields:

- `schema_version` — currently `2`
- `task_id` — stable `task-...` continuation key
- `project`, `title`, `subtitle`, `actor`
- `state` — `awaiting_human`, `in_progress`, `paused`, `completed`, `closed`
- `created_at`, `updated_at`, `last_human_activity_at`
- `last_human_activity_epoch`
- `reminder_minutes`
- `remind_while_paused`
- `nudge_on_agent_poll`
- `reminder_count`
- `history_limit`
- `items`

## Item

Every item contains:

- `id`
- `category`
- `type` — one of the 18 canonical semantic types in `QUESTION_TYPES.md`
- `title`
- `instruction`
- `required`
- `answer`

Type-specific optional metadata:

- `choices`
- `allow_multiple`
- `recommended`
- `min`, `max`, `step`, `unit`
- `colors`
- `gradients`: `{id,label,from,to}` objects
- `hierarchy`: `{id,parent_id,label}` objects
- `default`

V0.1 compatibility fields `expected_color` and `expected_value` remain readable but are not the preferred V2 vocabulary.

## Answer

```json
{
  "answered": true,
  "status": "selected",
  "value": "Balanced",
  "data": "Balanced",
  "note": "",
  "answered_at": "2026-08-07T12:34:56Z"
}
```

`data` is canonical structured evidence and may be a boolean, number, string, array or object according to question type. `value` is display/log text. V1 answers without `data` are migrated by using their existing `value`.

## Save/recovery contract

A task save:

1. serializes and re-parses the complete document through the authoritative schema validator;
2. writes a temporary file;
3. verifies the temporary bytes;
4. preserves the previous primary as `.bak`;
5. installs the verified temporary file as the primary.

On load, a malformed primary may recover from a valid `.bak`; recovery is surfaced to the caller.

## Lookup and concurrency

Each task has an independent locator file `<task_id>.path`. Task creation does not mutate one shared registry JSON document, avoiding a cross-agent write hotspot.

## Migration

The V2 reader accepts schema versions 1 and 2. V1 question aliases normalize to canonical V2 semantics in memory and are written as V2 on the next save. See `QUESTION_TYPES.md` for the alias table.
