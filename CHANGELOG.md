# Changelog

## Unreleased

The 0.2.1 line has completed its functional Windows and live-host acceptance. A final source/repository hygiene build is pending before the release stamp is applied.

- Live `create_task` owns the human interaction through completion, cancellation or delegation and returns a terminal structured result without requiring a wake-up chat message.
- Added durable Suggest, Clarify and Use judgement round-trips. Agent proposals remain advisory; delegation never fabricates human `answer.data`.
- Use judgement closes the human-facing task automatically once all remaining required decisions have been delegated and acknowledged.
- Terminal results explicitly tell the host to continue visibly after completion, cancellation or delegation.
- Agent-launched window sizing measures the controls actually assembled. Preferred width, card packing and live layout share one geometry path; overflow scrolls instead of relying on question-type height estimates.
- Added the Pass/Fail presentation for canonical `confirm`, including an optional durable verdict note.
- Split distribution cleanly into `TaskTrackGui.exe` and `TaskTrackMcp.exe`, with self-describing CLI/version surfaces and an MCP self-test.
- Added the optional Agent Skill for hosts that need guidance around assistance fallback and lifecycle ownership.
- Simplified the public repository: concise build/host documentation, architecture/reference docs only, machine-local U++ assembly files removed, generated build folders ignored, and package dependencies reduced to their direct boundaries.
- Deterministic Windows baseline before this final hygiene pass: 142 tests passing in Release and Debug/BLITZ, with MCP self-test passing.

## 0.2.0 — 2026-08-07

- Expanded the public vocabulary to 18 semantic human-response types.
- Added schema 2 structured `answer.data` and schema 1 migration.
- Added the native semantic renderer and specialist position, direction, range and gradient controls.
- Reworked the application into a compact responsive card layout with conditional categories.
- Added durable recovery, structured exports, example coverage and deterministic regression tests.

## 0.1.0 — 2026-08-07

Initial durable TaskTrack implementation with U++ GUI, MCP bridge, persistence/recovery, pause/reminders, exports and tests.
