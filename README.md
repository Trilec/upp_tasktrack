# TaskTrack

TaskTrack is a compact U++ human-in-the-loop decision and verification console for AI-assisted workflows.

An agent uses TaskTrack when automation has reached a fact or choice that genuinely requires a person: visual judgement, interaction behaviour, wording, placement, colour, priorities, hierarchy, numeric preference, or another structured decision. TaskTrack saves the request before returning its stable task id, so the human may answer immediately, pause for hours, or come back the next day without tying the work to one tool call.

## Agent semantics, not GUI widgets

TaskTrack V0.2 exposes 18 semantic question types:

`confirm`, `single_choice`, `multi_choice`, `select`, `list_select`, `text`, `notes`, `number`, `amount`, `range`, `rating`, `color`, `gradient`, `position`, `direction`, `rank_order`, `hierarchy_select`, `curve`.

Agents never select `UiRadioButton`, `UiSliderEdit`, or another U++ class. They describe the human decision; TaskTrack chooses a compact renderer.

## Compact native UI

Each question is a restrained `UiGroupPanel` using its title and subtitle support, with only the required response control in the content area. Questions live in a horizontal wrapping `UiBoxLayout` fixed-column grid, naturally moving from roughly three columns to two to one as space narrows.

A multi-category request gets a small wrapped category strip. A single-category request does not waste space on it. The old extra “Verification” heading is gone, and generic per-card note editors are intentionally absent; free text is requested explicitly with `notes`.

## Package layout

- `TaskTrack/Core` — schema, validation, persistence, recovery, lookup, results/export.
- `TaskTrack/Widgets` — semantic question renderer and four small specialist selectors.
- `TaskTrack/App` — compact native shell, categories, autosave, pause/reminders, export.
- `TaskTrack/Mcp` — stdio MCP transport.
- `examples/TaskTrackExample` — exactly one example of each canonical question type.
- `tests/TaskTrackTests` — deterministic model/persistence/migration regression coverage.
- `docs` — architecture, agent guide, semantic types, schema, MCP and Windows acceptance.

## Durability

Task JSON is canonical. Saves are validated, written through a verified temporary file, and retain a `.bak` recovery copy. A tiny locator file exists per task id rather than one shared registry document.

Task states are:

`awaiting_human`, `in_progress`, `paused`, `completed`, `closed`.

Pause is indefinite. Optional inactivity reminders and agent-poll nudges may ask the operator what to do; TaskTrack never closes a task just because time passed.

## Compatibility

V0.2 writes schema version 2 and structured `answer.data`. V0.1 task files remain readable and are normalized to the new semantic vocabulary on load.

## Build

See `GETTING_STARTED.md`. `verify.ps1` remains the Windows convenience wrapper for the four U++ Release builds, deterministic Core tests, and MCP self-test.

For agents, start with `docs/AGENT_GUIDE.md`. For the precise 18-type contract, see `docs/QUESTION_TYPES.md`.
