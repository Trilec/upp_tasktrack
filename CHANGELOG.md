# Changelog

## Unreleased — 2026-08-11

- Refreshed TaskTrack against current `upp_Ui/main` and removed the retired `UiCompositeColor` dependency from the question renderer; the custom-colour field now delegates to first-class `UiColorMatrix`.
- Hardened the agent guide around the real operating boundary: ask only for genuinely human-dependent facts, ask the minimum needed to continue, choose types by semantic meaning, avoid GUI/layout instructions, distinguish recommendations from neutral defaults, retain `task_id`, and consume structured `answer.data`.
- Added `docs/STATUS.md` as a compact recovery/validation checkpoint before authoritative Windows compilation.
- Kept the existing V0.2 18-type schema and durable MCP contract unchanged so Windows validation can establish a clean baseline before any further semantic expansion.
- TT-003-W1 passed on Windows with 61/61 tests, MCP selftest, live create_task smoke, colour persistence and all 18 semantic controls.
- Replaced the question workspace's masonry-like natural-width flow with a deterministic responsive policy: at most three equal-width columns, minimum 10px gutters, row-equal card heights and stable partial final rows.
- Hardened category reflow so wrapped category height propagates to the main shell instead of leaving first-open gaps or overlapping the question area after resize.
- Increased question title hierarchy to 12pt bold with 9pt supporting text while preserving the existing semantic renderer and task contract.
- TT-004-W1-R2 passed the authoritative BLITZ Release wrapper and visual/mechanical responsive checks after the UiDoc BLITZ-safety fixes; equal-column wide/medium layouts and repeated category reflow are accepted.
- Folded task context and answered progress into the top wrapping header and removed the separate objective row, reclaiming vertical workspace without changing task semantics.
- Replaced the active hand-painted TaskTrack range field with the current `UiRangeSliderEdit` composition, retaining the same `{low, high}` answer evidence while gaining themed range rendering and direct lower/upper numeric editing.
- TT-005-W1 passed the complete build/test/MCP matrix and native range persistence; Curt retained only small category/range presentation issues plus the need for a visible recommendation fast path.
- Made agent recommendations actionable: each recommended question now uses `UiGroupPanel` header content for a compact `Suggested: ...` + `Accept` action, while recommended discrete controls receive Accent treatment without being selected or counted as human evidence.
- Added a workspace `Accept suggestions` action and routed reminder/exit acceptance through one Core recommendation path that applies only unanswered recommendations, never falls back to neutral defaults, and does not overwrite an existing human answer.
- Hardened shared recommendation acceptance across all 18 current question types so malformed, out-of-range, unknown or structurally invalid values are refused before they can become human evidence; deterministic tests cover non-pre-answering, non-overwrite, default/evidence separation, structured multi-choice and structured range acceptance.
- Strengthened MCP discovery, `create_task` schema/tool descriptions, and the agent guide so agents exhaust machine evidence first, ask the minimum human-dependent decisions, and normally supply a defensible preferred answer rather than presenting blank choices.
- Expanded the 18-type demo with representative recommendations across structured controls so the header-content/Accent/acceptance workflow is directly visible; open-ended text and notes remain recommendation-free when no responsible preference exists.
- Tightened the native range presentation with a compact internal inset, smaller lower/upper fields, suppressed crowded endpoint markers, and sane numeric precision so ordinary values do not collapse into scientific notation.
- Corrected the remaining category fit estimate by using the themed button's 30px row authority plus explicit lower breathing room.
- Added TaskTrack-local human-decision state colours without changing global Ui roles: blue/Accent means an unanswered agent suggestion, light green with a 2px green frame means human-resolved, red/Alert means a required human decision still missing after review, and optional unresolved questions remain neutral.
- After `Accept suggestions` or a blocked submit/finish attempt, TaskTrack now routes directly to the first category with unresolved required input, marks affected category buttons/cards red with remaining counts, and changes the footer action to `Review N required`; resolved cards and the exhausted suggestion action turn green.

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
