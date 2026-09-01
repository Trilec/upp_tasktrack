---
name: tasktrack-dashboard
description: Maintain and present durable visual project state through TaskTrack Dashboard. Use it when the user asks where a project stands, wants a project/status dashboard, milestones, objectives, next steps, attention items, verification/audit state, changes or revision history. Dashboard state is agent-authored and must never be treated as human TaskTrack answer evidence.
license: GPL-3.0-only
metadata:
  version: "0.1.0"
---

# TaskTrack Dashboard

TaskTrack Dashboard is the durable visual presentation path for project state.

## When to use it

Use the dashboard when a visual structured status is more useful than another prose list, especially for:

- current project state;
- milestones/checkpoints;
- objectives/workstreams and percentages;
- next steps;
- attention/blockers/risks;
- verification/audits/tests;
- important changes;
- durable revision history.

Do not use the dashboard as a substitute for human TaskTrack evidence. If a person must decide, approve or visually verify something, use the normal human TaskTrack workflow and optionally link its task id from an Attention entry.

## Panel selection

Choose only panels useful for the question.

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

Current dashboard JSON should describe useful current state, not every event in project history. Prefer stable IDs, archive/compress completed detail, and rely on immutable dashboard revisions for previous accepted states.

## Update workflow

1. Read current dashboard with `get_dashboard` when it already exists.
2. Reconcile against current repository/build/test/audit truth.
3. Preserve stable panel and entry ids where the meaning is unchanged.
4. Update semantic status/progress/evidence and remove obsolete current clutter where appropriate.
5. Call `upsert_dashboard` using the exact current `base_revision`. On `REVISION_CONFLICT`, re-read current state, merge and retry. On `WRITE_BUSY`, wait/retry the normal read/merge/upsert path. Never force a stale document or bypass the writer lock.
6. Use `open_dashboard` when the person needs the native visual presentation.

## Progress

Prefer evidence-backed entry progress. When suitable, let Core derive overall progress from weighted contributing Timeline/Progress List entries. Use explicit `overall_progress` only when there is a defensible management estimate distinct from the mathematical milestone average. Keep `confidence` separate.

## Attention

Use Attention for unresolved risk/blocker/approval/decision state. Write the title/detail in plain human language. Do not hide the actual problem behind test jargon.
