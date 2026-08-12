# Changelog

## Unreleased — 2026-08-12

- Refreshed TaskTrack against current `upp_Ui/main` and removed the retired `UiCompositeColor` dependency from the question renderer; the custom-colour field delegates to first-class `UiColorMatrix`.
- Hardened the agent guide around the real operating boundary: ask only for genuinely human-dependent facts, ask the minimum needed to continue, choose types by semantic meaning, avoid GUI/layout instructions, retain `task_id`, and consume structured `answer.data`.
- Replaced the question workspace's masonry-like natural-width flow with deterministic equal-width responsive columns and stable row alignment.
- Hardened category reflow, compacted task context into the top header, and moved semantic range rendering to current `UiRangeSliderEdit` while preserving `{low, high}` evidence.
- Made recommendations actionable through `UiGroupPanel` header content and one shared Core acceptance path that applies only unanswered recommendations, never promotes neutral defaults, and never overwrites human evidence.
- Strengthened MCP discovery and `create_task` guidance so agents exhaust machine evidence first, ask the minimum human-dependent decisions, and normally supply a defensible `recommended` answer.
- Added TaskTrack-local resolved/review colour treatment and routing to unresolved required categories after bulk acceptance or blocked submit.
- Standardized every question body on a shared 8px inset; removed the range field's redundant local inset.
- TT-008-W1 passed Release/BLITZ, 73/73 tests, MCP selftest, native range presentation, responsive layout and the consistent 8px body-inset acceptance.
- TT-009 defines the four workflow states precisely: grey = normal agent proposal, orange = required/no proposal, green = human-resolved, red = required item still blocking after attempted continuation. These are TaskTrack-local states, not global `UiRole` remappings.
- Added a durable `<task>.agent.json` assistance sidecar so human→agent requests survive disconnects without sharing the authoritative human-answer write path.
- Added compact human→agent actions: `propose_answer` requires `recommended`; `clarify` with `mode=simplify` requires `clarification` and may also return `recommended`.
- Added MCP `respond_to_request`; `get_task` now exposes `agent_action_required` and compact `pending_requests`. Agent responses are validated against the referenced semantic item and remain advisory until explicit human action.
- Added per-question `Suggest` and `?` assistance actions. A returned proposal changes the required/no-proposal question back to the normal suggested state; only human acceptance creates `answer.data`. Clarification preserves the original question and adds the latest plain-language explanation.
- Kept task creation, human answers, polling and assistance durable: the GUI can remain open while the agent responds, and pending assistance survives MCP/client restarts.

## 0.2.0 — 2026-08-07

- Integrated the TT-001-W1 Windows findings: U++ task-id formatting now uses the escaped literal `T` and `int64` formatter arguments, and JSON numeric Values are accepted correctly during save/load validation.
- Expanded the public question vocabulary from the V0.1 verification-specific set to 18 semantic human-response types.
- Added schema V2 structured `answer.data` and V0.1 task-type migration.
- Added `TaskTrack/Widgets` with a compact `UiGroupPanel` question renderer and four specialist controls for position, direction, range, and gradient selection.
- Reworked the application into a restrained wrapped card grid with conditional wrapped categories and less redundant chrome.
- Removed generic per-card note rows; `notes` is now the explicit free-text escape hatch.
- Expanded MCP schema/descriptions so agents choose semantic response meanings rather than U++ controls.
- Replaced the example with exactly one question of each canonical type.
- Added agent/question-type documentation and regression coverage for the Windows defects, V1 migration, structured results, validation and recovery.

## 0.1.0 — 2026-08-07

Initial durable TaskTrack implementation with U++ GUI, MCP bridge, persistence/recovery, pause/reminders, exports, tests, and Windows acceptance materials.
