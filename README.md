# TaskTrack

TaskTrack is a compact U++ human-in-the-loop decision and verification console for AI-assisted workflows.

An agent uses TaskTrack when automation reaches a fact or choice that genuinely requires a person: visual judgement, interaction behaviour, wording, placement, colour, priorities, hierarchy, numeric preference, approval, classification, selection, or another structured decision. Normal launched work is a live interaction: TaskTrack persists first for durability, opens the native GUI, and returns terminal human evidence through MCP so the agent can continue without asking the person to send a second chat message merely to wake it.

## Agent semantics, not GUI widgets

TaskTrack exposes 18 semantic question types:

`confirm`, `single_choice`, `multi_choice`, `select`, `list_select`, `text`, `notes`, `number`, `amount`, `range`, `rating`, `color`, `gradient`, `position`, `direction`, `rank_order`, `hierarchy_select`, `curve`.

Agents never select `UiRadioButton`, `UiSliderEdit`, or another U++ class. They describe the human decision; TaskTrack chooses a compact renderer.

## Verification fast path

For ordinary human verification the preferred form is a `confirm` item with the semantic pair:

```json
"type": "confirm",
"choices": ["Pass", "Fail"]
```

TaskTrack renders Pass/Fail specially: Pass is green, Fail is red, with accessible text labels and a compact optional verdict Note editor. `answer.data` is the boolean verdict authority (`true` for Pass, `false` for Fail); `answer.note` is optional supporting human evidence. This is presentation sugar over the canonical `confirm` type, not a new semantic type. Richer types remain available when the requested evidence genuinely needs them.

## Compact native UI

Each question is a restrained `UiGroupPanel` using its title and subtitle support, with only the required response control in the content area. Questions use responsive TaskTrack flow layout so small requests stay compact while larger tasks expand only as needed.

A multi-category request gets a small wrapped category strip. A single-category request does not waste space on it. Generic per-card note editors are intentionally absent; free text is requested explicitly with `notes`. The only exception is the narrow optional verdict note attached to a Pass/Fail verification.

Agent-launched one-question tasks use a compact dialog presentation and remove secondary console controls that are not useful for a short decision.

## Package layout

- `TaskTrack/Core` — schema, validation, persistence, recovery, lookup, results/export.
- `TaskTrack/Widgets` — semantic question renderer and small specialist selectors.
- `TaskTrack/App` — compact native GUI (`TaskTrackGui.exe`).
- `TaskTrack/Mcp` — stdio MCP server (`TaskTrackMcp.exe`).
- `examples/TaskTrackExample` — exactly one example of each canonical question type.
- `tests/TaskTrackTests` — deterministic model/persistence/migration regression coverage.
- `skills/tasktrack` — optional agent workflow skill for reliable TaskTrack lifecycle handling.
- `docs` — architecture, agent guide, semantic types, schema, MCP and Windows acceptance.

The two executables form a pair and should normally live in the same directory. `TaskTrackGui.exe --task <path>` opens a task; `TaskTrackMcp.exe` is the agent-facing stdio server and launches `TaskTrackGui.exe` beside itself. The MCP executable remains protocol-clean; the GUI remains a native U++ application.

## Durability

Task JSON is canonical human evidence. Saves are validated, written through a verified temporary file, and retain a `.bak` recovery copy. A tiny locator file exists per task id rather than one shared registry document.

Task states are:

`awaiting_human`, `in_progress`, `paused`, `completed`, `closed`.

Pause is indefinite. Optional inactivity reminders may ask the operator what to do; TaskTrack never closes a task just because time passed.

The `.tasktrack.json` file and separate `.agent.json` assistance sidecar are durability/recovery storage. They are not the normal answer transport to an agent.

## Human / agent assistance

TaskTrack uses four local workflow states: grey = a recommendation is available, orange = required item with no recommendation, green = human-resolved, red = required item still blocking after an attempted continuation.

Human→agent assistance uses the compact actions:

`propose_answer`, `clarify`, `continue_with_judgement`.

Requests live in the separate durable `<task>.agent.json` sidecar with lifecycle `pending → answered → (cancelled)`. An answered assistance request is not a resolved human question; only explicit human action creates `answer.data`.

On hosts that expose the required modern MCP capability, Suggest/Clarify can use the in-call multi-round-trip path. On hosts that do not, TaskTrack returns a structured compatibility action; the agent is expected to resolve the pending assistance request itself and immediately wait on `get_task` until the person completes. The optional TaskTrack Codex skill packages that fallback workflow so the human does not need to restart the agent manually.

## Codex plugin / skill

TaskTrack includes:

- `.codex-plugin/plugin.json`
- `.agents/plugins/marketplace.json`
- `skills/tasktrack/SKILL.md`

This plugin is workflow guidance, not a second transport. The registered `TaskTrackMcp.exe` remains the actual MCP connection. The skill teaches Codex when to invoke TaskTrack and how to keep the interaction moving automatically when the host lacks sampling/MRTR support.

See `GETTING_STARTED.md` for installation and validation.

## Compatibility

TaskTrack writes schema version 2 and structured `answer.data`. V0.1 task files remain readable and are normalized to the current semantic vocabulary on load; legacy `pass_fail` remains a loader compatibility alias only.

## Build

See `GETTING_STARTED.md`. `verify.ps1` remains the Windows convenience wrapper for the four U++ Release builds, deterministic Core tests, and MCP self-test.

For agents, start with `skills/tasktrack/SKILL.md` and `docs/AGENT_GUIDE.md`. For the precise 18-type contract, see `docs/QUESTION_TYPES.md`.
