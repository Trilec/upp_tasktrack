# TaskTrack Status

## Recovery log — 2026-08-12

BASE: `43cca36865e1b143ccf7095d7ec511ab255dbcfe` on `main`

TASK: TT-007 semantic human-decision visual states

STATUS:

- TT-006-R1-W1 is accepted: Release and Debug/BLITZ builds PASS, TaskTrackTests 73/73, MCP selftest PASS, fresh `in_progress` question controls interactive, category switching/resizing stable, closed-task read-only behavior correct, 60-second soak clean, durable get/list recovery PASS, and TaskTrack MCP installed/connected in OpenCode.
- TT-007 keeps the existing persistence/schema/MCP semantics and adds a TaskTrack-local visual state layer rather than changing global `UiRole::Standard` semantics.
- Visual language is deterministic: suggested/unanswered = Accent/blue; answered by the human (manual or accepted suggestion) = light green face with a 2px green frame; required unanswered after a bulk-accept/submit review = Alert/red; optional unresolved = neutral.
- Answered cards expose a compact green `Answered` / `Done` status in the GroupPanel header-content lane. Recommended unanswered cards retain `Suggested: ...` + `Accept`. Required review cards show `Needs input` in Alert styling.
- `Accept suggestions` still applies only valid unanswered recommendations and never overwrites human evidence or promotes neutral defaults.
- After bulk acceptance, TaskTrack computes remaining required questions. If any remain, it enters review mode, automatically opens the first affected category, marks affected categories Alert/red with a remaining count, marks the unresolved required cards red, and changes the footer action to `Review N required`.
- Optional recommendation-free questions remain neutral and do not block completion.
- Once all required review items are answered, review mode clears automatically. If no suggestions remain, the footer shows a green `Suggestions applied` state when the task contained recommendations.
- A failed Submit/Accept-and-finish path activates the same red review route instead of only presenting opaque item IDs.
- TT-006-R1 category-height preservation remains unchanged underneath this work.

DEPENDENCY CONTEXT:

- Use current `upp_Ui/main`; required accepted ancestor remains `3ea8bed64aa5a3ef6d98caf108890296e6245eb5` or a descendant.
- No Ui dependency changes are required for TT-007. The success/green state is intentionally local to TaskTrack; Ui currently exposes Standard/Subtle/Accent/Alert but no global Success role.

TOUCHED TASKTRACK PATHS:

- `TaskTrack/Widgets/TaskTrackWidgets.h`
- `TaskTrack/App/TaskTrackApp.h`
- `TaskTrack/App/TaskTrackApp.cpp`
- `docs/STATUS.md`
- `CHANGELOG.md`

PUBLISHED:

- TT-007 implementation checkpoint: `3b134306389919e0d4e01c3ae12f05750d6ee3e1` on `main`.
- Publication was verified by re-reading the remote `main` ref and comparing against the accepted TT-006-R1 base. Net production scope is limited to the five paths listed above; no staging/temp files remain in the resulting tree.

VALIDATION:

- TT-006-R1-W1 baseline: PASS.
- TT-007 source/API/diff review: PASS for intended state precedence, current Ui style/palette APIs, recommendation/default evidence separation, category routing and unchanged schema/dependency direction.
- TT-007 Windows Release/Debug/runtime/visual acceptance: PENDING Gary.

NEXT:

1. Gary pulls current TaskTrack `main`, confirms `3b134306389919e0d4e01c3ae12f05750d6ee3e1` is an ancestor, and validates against current `upp_Ui/main`.
2. Run Release + Debug/BLITZ + 73-test/MCP selftest regression.
3. On a fresh task, confirm blue suggestions are not answers; accepting one turns the card green; `Accept suggestions` fills available recommendations, turns accepted cards green, routes to the first unresolved required category, and marks only those required gaps red.
4. Confirm optional unresolved questions stay neutral, resolving required gaps clears red review state, and Submit remains explicit.
5. Leave a review-state demo open for Curt visual acceptance.
