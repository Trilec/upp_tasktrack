# TaskTrack Status

## Recovery log — 2026-08-11

BASE: `752a676eb3d6c75a0a79b3eaca5f6fc6a4cec41b` on `main`

TASK: current-Ui compatibility and agent-authoring checkpoint before Windows validation

TOUCHED:

- `TaskTrack/Widgets/TaskTrackWidgets.h`
- `docs/AGENT_GUIDE.md`
- `docs/STATUS.md`
- `CHANGELOG.md`

STATUS:

- Current `upp_Ui/main` was refreshed at `382c913e19c3ac06e3daa412361f52305c5ea75e` before this checkpoint.
- `UiCompositeColor` no longer exists in current `upp_Ui`; TaskTrack now delegates its custom-colour field to first-class `UiColorMatrix` without restoring the retired Ui control.
- Existing 18 semantic question types, schema V2 persistence, MCP durability, pause/reminder behaviour, and structured result contract are intentionally unchanged in this checkpoint.
- `docs/AGENT_GUIDE.md` now gives agents an explicit construction algorithm: when TaskTrack is appropriate, how many questions to ask, how to choose semantic types, how to use categories/required/recommended/default, and how to continue asynchronously from `task_id`.
- No V0.3/23-type schema expansion is included here. That work is deliberately deferred until the current V0.2 surface compiles and runs cleanly against the latest Ui tree.

PUBLISHED: this recovery log belongs to the checkpoint commit that contains it; use that commit SHA/current `main` as the validator baseline.

VALIDATION:

- Source/API review: PASS for the touched compatibility boundary against `upp_Ui` `382c913e...`.
- Windows U++ Release/Debug compilation and runtime validation: PENDING.
- GUI visual acceptance: PENDING.
- Real MCP host workflow acceptance: PENDING after the build is proven.

NEXT:

1. Gary performs the Windows compile/test/live-GUI acceptance against this checkpoint.
2. Fix only concrete compile/runtime regressions first, directly against the current remote tip.
3. Once the V0.2 baseline is green, continue agent-facing enhancements from evidence rather than expanding the schema speculatively.
