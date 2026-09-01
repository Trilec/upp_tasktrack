---
name: tasktrack-dashboard
description: Maintain and present durable visual project state through the dashboard tools exposed by the single TaskTrack MCP service. Use it for project status, milestones, objectives, next steps, attention, verification, changes or revision history. Dashboard state is agent-authored and never human TaskTrack answer evidence.
license: GPL-3.0-only
metadata:
  version: "0.3.0-rc1"
---

# TaskTrack Dashboard

TaskTrack Dashboard is the durable visual presentation path for project state. It is reached through the same registered `TaskTrackMcp.exe` as human TaskTrack decisions; `open_dashboard` launches the sibling `TaskTrackDashboardGui.exe`.

## When to use it

Use dashboard tools when a visual structured status is more useful than another prose list, especially for current state, milestones, objectives/workstreams, next steps, attention/blockers/risks, verification/audits/tests, important changes and revision history.

Do not use dashboard state as a substitute for human TaskTrack evidence. If a person must decide, approve or visually verify something, use the normal human TaskTrack workflow and optionally link its task id from an Attention entry.

## Panel selection

Choose only panels useful for the question:

- `project_state`: concise where-are-we snapshot.
- `timeline`: ordered phases/milestones/checkpoints.
- `progress_list`: objectives/workstreams with individual progress.
- `action_list`: next steps/future actions.
- `attention`: unresolved matters needing awareness/intervention.
- `verification`: tests/audits/acceptance evidence.
- `changes`: what changed since an earlier state/revision.
- `records`: structured information that fits none of the above.

A quick status normally needs only a few panels. Do not mechanically send all eight.

## Keep current state bounded

Current dashboard JSON should describe useful current state, not every event in project history. Preserve stable IDs, archive/compress completed detail, and rely on immutable dashboard revisions for previous accepted states.

## Update workflow

1. Read current state with `get_dashboard` when it exists.
2. Reconcile against current repository/build/test/audit truth.
3. Preserve stable panel and entry ids where meaning is unchanged.
4. Update semantic status/progress/evidence and remove obsolete current clutter.
5. Call `upsert_dashboard` using exact current `base_revision`. On `REVISION_CONFLICT`, re-read/merge/retry. On `WRITE_BUSY`, retry the normal path; never bypass the writer lock.
6. Use `open_dashboard` when the person needs the native visual presentation.

Prefer evidence-backed progress. Use explicit `overall_progress` only when there is a defensible management estimate distinct from derived progress. Keep `confidence` separate.
