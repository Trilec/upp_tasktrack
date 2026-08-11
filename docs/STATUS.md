# TaskTrack Status

## Recovery log — 2026-08-11

BASE: `5edbf21d120619ca0a69b736922027e4b91c5078` on `main`

TASK: TT-006 actionable agent recommendations and final workspace fit

STATUS:

- TT-005-W1 is accepted for build/runtime: authoritative BLITZ Release wrapper PASS, TaskTrackTests 61/61, MCP selftest PASS, compact header PASS, native `UiRangeSliderEdit` persistence PASS, responsive equal-column grid PASS, clean trees.
- Curt's remaining TT-005 visual findings are contained: the category row still clips slightly at the bottom, the native range card needs ordinary numeric display/inset without endpoint-marker crowding, and recommendations need to be visible/actionable through `UiGroupPanel` header content instead of a separate `Agent suggests:` body line.
- TT-006 makes `recommended` the explicit agent fast path while keeping human evidence authoritative. Recommended values are advisory until the human presses the per-question `Accept` action or the workspace `Accept suggestions` action.
- Shared Core acceptance supports structured recommendation evidence across the existing 18 semantic types, validates the value before accepting it, never falls back to `default_value`, and does not overwrite an already-answered human item by default.
- Existing task-load validation still protects the canonical discrete/color/gradient recommendation cases; the shared acceptance path additionally refuses malformed, out-of-range, unknown, duplicated, or structurally invalid compound recommendations before they can become human evidence.
- Recommended confirm/single-choice/multi-choice/rating/colour options receive Accent treatment without being selected or counted as answered. Every recommended question also uses the `UiGroupPanel` right-side header-content slot for `Suggested: ...` plus `Accept`.
- The old V0.2 `Agent suggests:` body row is collapsed when header-content recommendation UI is present, reducing card height.
- The footer now provides `Accept suggestions`, which fills only unanswered recommended items and does not submit the task. Reminder/exit acceptance uses the same recommendation-only, non-overwriting path; neutral defaults are never accepted as human evidence.
- The 18-type demo deliberately supplies defensible recommendations across most structured types so the recommendation UI and fast path can be visually and mechanically accepted. Open-ended text/notes remain without invented recommendations for contrast.
- The active range adapter keeps current `UiRangeSliderEdit`, uses a compact 5px inner inset and smaller numeric fields, suppresses crowded endpoint markers, and uses sane significant precision so ordinary values such as 320/900 do not display as `3e2`/`9e2`.
- Category measurement now uses a 30px button-row authority plus explicit lower breathing room instead of the old 26px row estimate that under-counted the themed button height.
- MCP discovery, `create_task` description/schema, and `docs/AGENT_GUIDE.md` teach the same construction algorithm: exhaust machine evidence first, ask the minimum human-dependent decisions, group related decisions, choose semantic types, and provide `recommended` whenever the agent has a defensible preferred answer. `default` remains neutral presentation state.

DEPENDENCY CONTEXT:

- Required `upp_Ui/main` checkpoint: `3ea8bed64aa5a3ef6d98caf108890296e6245eb5` or a descendant.
- That Ui checkpoint reserves an attached `UiGroupPanel` header-content root before clipping long title/subtitle content, so right-side actions remain visible; it also makes `ClearHeaderContent()` clear the owned pointer safely.
- TaskTrack validation should use normal current `upp_Ui/main`; no temporary Ui validation branch is required.

TOUCHED TASKTRACK PATHS:

- `TaskTrack/Core/TaskTrackCore.h`
- `TaskTrack/Widgets/TaskTrackWidgets.h`
- `TaskTrack/App/TaskTrackApp.h`
- `TaskTrack/App/TaskTrackApp.cpp`
- `TaskTrack/App/TaskTrackResponsiveFlow.h`
- `TaskTrack/Mcp/main.cpp`
- `examples/TaskTrackExample/main.cpp`
- `tests/TaskTrackTests/main.cpp`
- `docs/AGENT_GUIDE.md`
- `docs/STATUS.md`
- `CHANGELOG.md`

PUBLISHED:

- TT-006 implementation commit: `aa0a45ae0ea3e58b68e9712460f46e903277d9f6` on TaskTrack `main`.
- The implementation commit is one squashed descendant of the accepted TT-005 base; the accepted `TaskTrack/Core/TaskTrackCore.cpp` blob was deliberately preserved byte-for-byte to avoid carrying a broad staging rewrite.
- `upp_Ui/main` dependency correction: `3ea8bed64aa5a3ef6d98caf108890296e6245eb5`.

VALIDATION:

- TT-005-W1 automated/runtime baseline: PASS.
- TT-006 source/API/diff review: PASS for intended scope, current Ui theme resolver APIs, `UiGroupPanel` header-content contract, current `UiRangeSliderEdit`, package membership and unchanged task schema.
- TT-006 Windows compile/runtime: PENDING Gary.
- TT-006 recommendation/header-content/category/range visual acceptance: PENDING Curt after Gary launches the fresh demo.

NEXT:

1. Gary validates current TaskTrack `main` and confirms `aa0a45ae0ea3e58b68e9712460f46e903277d9f6` is an ancestor of HEAD.
2. Validate against current `upp_Ui/main` with `3ea8bed64aa5a3ef6d98caf108890296e6245eb5` as an ancestor, stopping at the first useful compiler/runtime failure without source edits.
3. Mechanically verify recommendation highlighting/acceptance, non-overwrite/default separation, exact category bottom clearance and range display/persistence.
4. Leave a fresh recommendation-rich 18-question demo open for Curt's visual acceptance.
