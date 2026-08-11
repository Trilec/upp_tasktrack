# TaskTrack Status

## Recovery log — 2026-08-11

BASE: `d5d0b0e1f6bdf4bcf7629848286c18ab1bdf73bf` on `main`

TASK: TT-006-R1 category rebuild stability and post-crash acceptance

STATUS:

- TT-006 compiled in Release and Debug/BLITZ after the authoritative TT-006 implementation checkpoint. TaskTrackTests passed 73/73 and TaskTrackMcp `--selftest` passed.
- The follow-up runtime investigation found two genuine lifetime/timer faults. They are already published in `d5d0b0e1f6bdf4bcf7629848286c18ab1bdf73bf`: TaskTrack timer IDs now use valid small Ctrl timer offsets, and `TaskTrackQuestionCtrl` suppresses recommendation/layout re-entry while being destroyed. The recommendation header attachment also marks itself attached before layout-affecting setters to prevent recursive attachment.
- Gary's report that question-card controls did not accept mouse clicks was reproduced in the supplied screenshots only while the task state was `closed`. This is the intended terminal read-only contract: `RebuildItems()` disables question cards for `Completed` and `Closed`, while Categories, Save/export and Exit remain available. Do not change that behavior unless product semantics are deliberately revised.
- The remaining visible category regression has a concrete rebuild cause. `RebuildCategories()` temporarily clears every category button before repopulating the flow. During that empty interval the parent can synchronously remeasure `TaskTrackCategoryPanel`, observe a zero-row body, and retain the smaller height after the buttons return. That explains why first-open layout is correct but the strip tightens/clips after the first category selection.
- TT-006-R1 fixes that boundary in `TaskTrackCategoryPanel`: it retains the last settled non-empty category-body height while the flow is transiently empty, so rebuild cannot collapse the GroupPanel. Normal non-empty measurement still follows the current width and therefore continues to support wrapping.
- The existing recommendation semantics remain unchanged: `recommended` is advisory until explicitly accepted; `default` is neutral presentation state; bulk acceptance affects only unanswered recommendations and never overwrites human evidence.
- Durable task recovery is already part of the architecture. A task JSON file is saved before creation returns, answers continue to persist independently of the live MCP call, and a later agent can recover by `task_id` with `get_task`; `list_tasks` can locate recent persisted tasks if a restarted agent lost the id. Active tasks are not pruned by history cleanup.

AGENT TOOL-DISCOVERY REVIEW:

- Current MCP guidance is directionally correct but should be strengthened in a dedicated follow-up rather than mixed into this regression patch.
- The key routing lesson from `upp_patchtrack` is that procedural wording strongly changes tool selection: its preview/apply descriptions begin by telling the model to hash every target first, which makes the simpler hash action unusually salient. TaskTrack should use the same effect intentionally for its primary purpose: the server/tool description must say plainly WHEN to invoke TaskTrack, WHEN NOT to, and what minimum structured information to provide.
- The next agent-contract pass should keep one canonical human-input tool rather than duplicate aliases, make its description lead with the human-only decision boundary, explain recommendation/default behavior, and make reconnect recovery (`task_id`, `get_task`, `list_tasks`) explicit in MCP discovery/initialize text.
- Optional image/thumbnail evidence and an explicitly human-authorised timeout/auto-accept policy remain product extensions, not part of TT-006-R1. Any timeout must be opt-in human policy; TaskTrack's default remains indefinite durable waiting.

DEPENDENCY CONTEXT:

- Required `upp_Ui/main` checkpoint: `3ea8bed64aa5a3ef6d98caf108890296e6245eb5` or a descendant.
- TaskTrack validates against normal current `upp_Ui/main`.

VALIDATION:

- TT-006 Release/Debug build and deterministic tests before R1: PASS, 73/73.
- TT-006 crash/timer corrections at `d5d0b0e...`: source reviewed; authoritative post-publication Windows acceptance still required together with R1.
- TT-006-R1 category rebuild correction: source reviewed; Windows runtime/visual acceptance PENDING Gary.
- Question-card interaction must be tested on a fresh task whose header state is `in_progress`, not on a reopened `closed` task.

NEXT:

1. Gary pulls current TaskTrack `main` and current `upp_Ui/main`, runs the normal Release and Debug/BLITZ acceptance matrix, and stops at the first useful failure without source edits.
2. Generate a brand-new TaskTrackExample task and confirm the header becomes `in_progress` before testing card mouse input.
3. Click several question controls first; then select every category repeatedly and confirm question controls remain interactive and the Categories panel never changes height/clips merely because its buttons were rebuilt.
4. If mechanical acceptance passes, install the built TaskTrack MCP into the current OpenCode setup and use TaskTrack itself for a small dogfood round-trip: OpenCode asks Curt a few visual/interactive acceptance questions, Curt answers them in TaskTrack, and OpenCode retrieves the persisted answers after the GUI interaction.
5. Leave a fresh recommendation-rich demo open for Curt's visual review.
