# Changelog

## 0.3.2-rc5 — 2026-09-09

- Windows `open_dashboard` now launches the native Dashboard GUI through a visible `CreateProcessW` path instead of U++ `LocalProcess`, whose Windows startup contract uses `SW_HIDE`.
- Dashboard launch acknowledgement is posted from the GUI event loop after the requested dashboard is loaded and the window has been opened/shown.
- Project State now uses `UiChartRing` for weighted measurable state while the right-hand state list keeps explicit percentages and shows richer detail; the top-level dashboard retains its single overall `UiProgressRing`.
- Tunnel profile, probe and activity state moved out of the staged bundle into the shared per-user `TaskTrack/tunnel` application-data folder, with legacy migration and stage-time preservation.
- Tunnel-state tests now use an isolated state-root override and verify that override contract.

## 0.3.2-rc4 — 2026-09-08

- `open_dashboard` now requires a GUI launch acknowledgement carrying the exact normalized dashboard path before reporting `launched: true`.
- `TaskTrackDashboardGui.exe --dashboard <path>` opens and foregrounds the requested dashboard; no-argument launch now opens a native empty shell with a visible Load dashboard action and recent-dashboard summary instead of immediately showing a file picker.
- Tunnel activity retains the last 10 communications and uses a denser 20px row height with smaller text.
- Dashboard default storage moved out of the staged executable bundle into the shared per-user application-data folder; legacy executable-relative dashboard data is migrated/preserved during the RC4 transition.
- Acceptance now distinguishes direct stdio host calls from ChatGPT calls that traverse the active OpenAI tunnel.

## 0.3.2-rc3 — 2026-09-08

- Canonicalizes pasted Windows MCP executable paths before profile persistence and tunnel launch so the upstream stdio command parser cannot consume backslashes as escapes.
- Adds local preflight for every enabled path-like MCP service command before starting the shared tunnel runtime.
- Tunnel Manager footer now exposes profile auto-save state (Saved to disk / failure) and diagnostics include the profile-store path and save status.
- Keeps TaskTrack/PatchTrack as separate named services; no tunnel-client fork or aggregator added.
- Extends live acceptance to create/read/open a real TaskTrack dashboard after tunnel connectivity is revalidated.

## 0.3.2-rc2 — 2026-09-07

- Added exact MCP runtime identity: `version`, `tunnel_probe` and `--version` report the running `TaskTrackMcp.exe` SHA-256.
- When running from a staged bundle, MCP identity also reports whether the executable matches `manifest.json`, the staged build label and the verified source commit.
- Tunnel Manager compares its configured TaskTrack MCP against the staged bundle manifest and shows Current / Different / Unverified / Missing status.
- A different or unverified TaskTrack MCP warns before tunnel connection but may still be run deliberately.
- `verify.ps1` checks the MCP self-reported executable hash against the actual built file.
- Runtime-bundle provenance remains enforced by `verification-manifest.json` and `stage-bin.ps1`.
- Includes the machine-tunnel security hardening and verified runtime staging work completed after 0.3.2-rc1.

## 0.3.1 — 2026-09-03

- Clarified the agent-facing human TaskTrack, dashboard and cross-agent handoff workflows.
- Added operational MCP tool and schema descriptions without changing protocol or data semantics.

## 0.3.0 — 2026-09-02

- Promoted the accepted unified TaskTrack and Dashboard release from 0.3.0-rc1.
- One registered `TaskTrackMcp.exe` launches `TaskTrackGui.exe` and `TaskTrackDashboardGui.exe`.
- Task schema remains 2 and dashboard schema remains 1.

## 0.3.0-rc1 — 2026-09-01

- Unified TaskTrack behind one registered `TaskTrackMcp.exe`; the MCP now exposes both human-decision and dashboard tool families.
- `create_task` / `open_task` continue to launch `TaskTrackGui.exe`; `open_dashboard` launches sibling `TaskTrackDashboardGui.exe`.
- Removed the standalone `TaskTrackDashboardMcp.exe` product and its separate host registration.
- Added dashboard dispatch and dashboard MCP self-test coverage to the existing TaskTrack MCP without changing the accepted human interaction lifecycle.
- Unified build identity is `0.3.0-rc1`; the version surface reports task schema 2 and dashboard schema 1 for restart verification.
- Dashboard Core/Widgets/App remain separate internal subsystems and dashboard state remains distinct from authoritative human `items[].answer.data`.

## Dashboard companion — 2026-09-01

- Added a complete semantic project-dashboard subsystem without changing the accepted human-question lifecycle.
- Added `TaskTrack/DashboardCore` with strict schema validation, bounded current-state documents, weighted derived progress, attention summaries, atomic current-file recovery, optimistic `base_revision` updates and immutable numbered revision history.
- Added `TaskTrack/DashboardWidgets`, including a TaskTrack-local `TaskTrackTimelineRail` with theme/custom-style, data, selection, mouse, keyboard and focus behaviour.
- Uses the existing `UiProgressRing` directly for overall/project-state circular progress.
- Added `TaskTrackDashboardGui.exe`, a read-only native cockpit with category filtering, compact/full panel inspection, current-state auto-refresh and historical revision browsing.
- Added the dashboard MCP tool family with validate/upsert/read/open/list/history operations; the final 0.3.0-rc1 boundary is the single `TaskTrackMcp.exe`.
- Added deterministic DashboardCore regression tests and dashboard MCP tool-family self-test coverage.
- Hardened dashboard writes with a short cross-process writer lock and safe recovery of an incomplete beyond-current next revision after a crashed writer.
- Added dashboard examples, Agent Skill guidance and full dashboard contract documentation.
- Extended `verify.ps1` to build and validate both existing TaskTrack executables and the new dashboard companion targets.
- Existing human-decision semantics, `TaskTrackGui.exe`, human task schema and authoritative `items[].answer.data` path remain unchanged by the dashboard model addition.

## 0.2.1 — 2026-08-27

The final Windows release gate passed:

- Release builds: PASS
- Debug/BLITZ builds: PASS
- TaskTrackTests: 142 passed, 0 failed
- MCP selftest: PASS

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
