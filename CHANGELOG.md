# Changelog

## 0.2.0 — 2026-08-07

- Integrated the TT-001-W1 Windows findings: U++ task-id formatting now uses the escaped literal `T` and `int64` formatter arguments, and JSON numeric Values are accepted correctly during save/load validation.
- Expanded the public question vocabulary from the V0.1 verification-specific set to 18 semantic human-response types.
- Added schema V2 structured `answer.data` and V0.1 task-type migration.
- Added `TaskTrack/Widgets` with a compact `UiGroupPanel` question renderer and four specialist controls for position, direction, range, and gradient selection.
- Reworked the application into a restrained wrapped card grid with conditional wrapped categories and less redundant chrome.
- Removed generic per-card note rows; `notes` is now the explicit free-text escape hatch.
- Expanded MCP schema/descriptions so agents choose semantic response meanings rather than U++ controls.
- Replaced the example with exactly one question of each canonical type.
- Added agent/question-type documentation and regression coverage for the three Windows defects, V1 migration, structured results, validation and recovery.

## 0.1.0 — 2026-08-07

Initial durable TaskTrack implementation with U++ GUI, MCP bridge, persistence/recovery, pause/reminders, exports, tests, and Windows acceptance materials.
