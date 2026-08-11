# TaskTrack Status

## Recovery log — 2026-08-11

BASE: `18c535e71226255a4f947f2d00e95f832be07455` on `main`

TASK: TT-004 responsive human-input workspace correction after Curt visual review

IMPLEMENTATION PATHS SINCE BASE:

- `TaskTrack/App/TaskTrackResponsiveFlow.h`
- `TaskTrack/App/TaskTrackApp.h`
- `TaskTrack/App/App.upp`
- `TaskTrack/Widgets/TaskTrackWidgets.h`

CHECKPOINT DOCUMENTATION:

- `docs/STATUS.md`
- `CHANGELOG.md`

STATUS:

- TT-003-W1 is accepted PASS on Windows: Release wrapper PASS, TaskTrackTests 61/61, MCP selftest PASS, 18/18 semantic controls exercised, current colour picker path PASS, persistence PASS, MCP create_task smoke PASS, clean tree.
- Curt's visual review found two layout defects in the otherwise working V0.2 workspace: category height could be over-reserved on first open and then fail to follow wrapped content on resize, while natural 310–350px question widths produced a masonry/brickwork appearance.
- TT-004 adds reusable responsive presentation policies rather than per-question exceptions.
- Question rows now resolve to one deterministic width, at most three columns, with a minimum 10px gutter; every card in a row stretches to the tallest row card and a partial final row keeps the same column width.
- The responsive question policy explicitly clears the old V0.2 350px fixed-column cap so two-column layouts can use the available width rather than leaving dead space.
- Category buttons remain compact and equal within the current row, use 6px-or-greater internal gaps, and wrap deterministically between 120px and 180px button widths.
- The category shell remains a `UiGroupPanel`, but TaskTrack now supplies a width-aware minimum-height specialization. This avoids the generic wrapped-child minimum from reserving a one-button-per-row height before the real window width is known, while also allowing the shell to grow when category buttons genuinely wrap.
- The responsive category policy explicitly clears the old 148px fixed-column cap once it takes ownership of resolved button widths.
- Question title typography is raised from the old compact 10pt treatment to 12pt bold with a 9pt subtitle and slightly more title/subtitle separation.
- Existing task schema, all 18 semantic question types, MCP contract, persistence, answers, recommendations, reminders and lifecycle behaviour are unchanged by TT-004.
- Source package membership includes the new responsive-flow header. Temporary staging files were removed; no `.tmp` or `.new` source remains in the App package.

DEPENDENCY CONTEXT:

- Last remote `upp_Ui/main` inspected through the connected repository: `382c913e19c3ac06e3daa412361f52305c5ea75e`.
- Gary's successful TT-003-W1 Windows environment used a newer local `upp_Ui` SHA `76c2ca1fdb7ca491eb2a3ba3a6c6a7adbc99919c`; validator should continue using the current local dependency rather than downgrading it.

PUBLISHED:

- TT-004 implementation is on current TaskTrack `main`; use the published HEAD supplied with the validator task and confirm `18c535e71226255a4f947f2d00e95f832be07455` remains its ancestor.

VALIDATION:

- TT-003-W1 baseline: PASS.
- TT-004 source/API/diff review: PASS for intended scope and current `UiBoxLayout`/`UiGroupPanel` measurement APIs.
- TT-004 Windows compile/runtime: PENDING.
- TT-004 responsive visual acceptance: PENDING Curt review after Gary launches the fresh demo.

NEXT:

1. Gary pulls current `main`, builds/tests without source edits, and stops at the first useful failure if the responsive presentation header exposes any current-Ui API mismatch.
2. If green, Gary opens a fresh 18-question demo and exercises first-open plus repeated wide/medium/narrow resizing, category wrapping, category selection and scrolling.
3. Leave the GUI open for Curt to accept/reject card geometry, typography and category proportions visually.
4. After visual acceptance, return to deferred agent-facing semantic/recommendation expansion only if still needed; do not mix it into this layout acceptance.
