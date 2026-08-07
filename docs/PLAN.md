# TaskTrack V0.1 Implementation Plan

## Goal

TaskTrack is a standalone U++ human-in-the-loop verification console. It exists for the points where an agent has completed automated work but needs a person to verify something that code cannot reliably prove: GUI appearance, drag behaviour, grouping, save/reload behaviour, exported output, colour, layout, or other observable facts.

The product is intentionally separate from PatchTrack. PatchTrack is only a reference for durable core logic, small frontends, protocol boundaries, persistence discipline, testing, and publication workflow.

## V0.1 architecture

TaskTrack is split into three production packages:

- `TaskTrack/Core` — authoritative task model, validation, persistence, recovery, history, export, and task lookup.
- `TaskTrack/App` — U++ GUI using the `Ui` control package.
- `TaskTrack/Mcp` — thin stdio MCP frontend over `TaskTrack/Core`.

Support packages:

- `examples/TaskTrackExample` — creates a real task containing every V0.1 input type.
- `tests/TaskTrackTests` — deterministic non-GUI tests for model, persistence, recovery, list/lookup and evidence export.

## Human workflow

1. An agent reaches a verification point that automation cannot prove.
2. The agent calls `create_task` with an objective and structured check items.
3. TaskTrack persists the complete task before returning its `task_id`.
4. The GUI may launch immediately, or the task can be opened later.
5. Human answers autosave while the task remains active.
6. The human may pause indefinitely without losing state.
7. Optional inactivity reminders ask whether the user is still working; TaskTrack never auto-closes.
8. Optional agent polling can generate a gentle reminder signal without altering task evidence.
9. `Send Result` is allowed only when every required item is answered.
10. The agent retrieves the durable result by `task_id` and continues.

## V0.1 verification field types

- `check`
- `pass_fail`
- `choice`
- `text`
- `multiline`
- `number`
- `color`
- `file`
- `interaction`
- `visual_compare`

This set is deliberately small. The wire/storage model is data-driven so later field types do not require redesigning the transport.

## GUI principles

- compact text: ordinary controls at 9pt, card/check headings at 10pt, main/objective headings at 11pt bold;
- layered panels rather than oversized form chrome;
- wrapping category controls similar in spirit to the current SymbolPicker shell;
- responsive wrapped check cards rather than one very long vertical checklist;
- categories are optional: one-category tasks remain simple, larger tasks can be split cleanly;
- the footer stays small and predictable: progress, Save split button, Send Result;
- terminal tasks remain viewable/exportable but are read-only.

## Persistence and recovery

Task JSON is the application authority. Saves use a verified temporary file and retain a `.bak` recovery copy. A tiny locator file exists per task ID rather than one shared registry document, avoiding a multi-agent write hotspot.

A short debounced autosave is used for typing. Explicit Save, pause/resume, settings changes, Send Result and Exit save synchronously.

Completed/closed history is bounded by `history_limit`; active and paused tasks are never pruned merely to satisfy that limit.

## Long human waits

TaskTrack does not make the original tool call wait for a person. A human can take minutes, hours, or a day. The durable `task_id` remains the continuation key.

States:

- `awaiting_human`
- `in_progress`
- `paused`
- `completed`
- `closed`

Pause is explicit and indefinite. There is no automatic abandonment transition.

## MCP strategy

The stdio bridge supports two eras:

- current `2026-07-28` stateless MCP requests, including the `io.modelcontextprotocol/tasks` extension where the client declares it;
- conservative older `initialize` / `notifications/initialized` hosts for practical Codex/OpenCode/Hermes compatibility.

Modern Tasks-extension clients can receive a formal task handle and poll `tasks/get`. Other clients receive an ordinary immediate TaskTrack result containing the durable `task_id` and then use `get_task`.

## Build/output layout

Repository root:

- `/TaskTrack`
- `/docs`
- `/examples`
- `/tests`
- `/build` (local generated output, Git-ignored)
- `GitHubOut.var`
- `example.var`
- `GETTING_STARTED.md`
- `verify.ps1`

The Windows validation script places the primary runnable programs directly under `/build`, matching the PatchTrack-style local distribution layout.

## Validation ownership

Source-side implementation and review are performed against the GitHub `main` branch as authority.

Windows remains the authoritative platform boundary for this first release:

- Gary: compile, automated execution, runtime/protocol evidence, and live-data operation.
- Curt: visual/interaction acceptance of the GUI.
- Source implementer: repair any defects reported against the exact validated commit and republish to `main`.

No feature-branch workflow is required during initial bring-up. The priority is a stable compiling `main` baseline.

## Source-review dependency snapshot

The V0.1 source review was performed against the active U++ repositories, including:

- `upp_Ui` main: `128b89100c62edea4436b705c3300a64ed4cadde`
- `upp_animation` main: `4a01b6f4e2a9f122ea1a93457b62a4054d01f970`
- `upp_statemachine` main: `93e20f109181e16667176214484cb4f585a3d2b8`

TaskTrack does not directly use StateMachine in V0.1, but the path is retained in the user's standard U++ assembly for future workflow integration.

## Deferred work

Not required for the first compiling/usable baseline:

- screenshots/images embedded directly into a task answer;
- richer evidence attachments;
- multiple simultaneous human assignees;
- remote/network task broker;
- AgentFlow integration;
- notification tray integration;
- full current-MCP task listing/result pagination beyond the initial practical bridge;
- configurable visual themes beyond inherited Ui theming.
