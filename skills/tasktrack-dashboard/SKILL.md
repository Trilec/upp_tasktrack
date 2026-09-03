---
name: tasktrack-dashboard
description: Use the Dashboard to maintain a bounded visual projection of useful current project truth through the single TaskTrack MCP service. It is agent-authored presentation/management state, never human TaskTrack evidence.
license: GPL-3.0-only
metadata:
  version: "0.3.1"
---

# TaskTrack Dashboard

Use the Dashboard for bounded, current-state, evidence-backed project presentation: phase, milestones, objectives, next actions, attention, verification, changes and useful records. It is revisioned management state, not human testimony, not a replacement for a human decision, not an unbounded project log and not the primary source of repository truth.

Source-of-truth workflow:

`durable project evidence → reconcile → get_dashboard → merge → upsert_dashboard → optional open_dashboard`

Evidence may come from repository HEAD, builds/tests, audits, plans, status documents or another agent's durable handoff. If a dashboard exists, read it before updating. Preserve stable panel and entry IDs when meanings are unchanged; replace stale current detail and rely on immutable revisions for history.

Use only panels that help answer the request: `project_state`, `timeline`, `progress_list`, `action_list`, `attention`, `verification`, `changes` and `records`. Categories are navigation/grouping, not layout. `density` is a presentation hint, not geometry. Keep current state compact and evidence-backed.

## Agent handoff

When another chatbot/agent cannot call TaskTrack, it may leave a bounded project-status handoff such as `docs/PROJECT_STATUS.md` or an established equivalent. Read it, inspect fresher repository/test evidence, resolve conflicts, read the current dashboard, merge current truth, preserve stable IDs and upsert with the exact current `base_revision`. Never blindly promote a stale handoff.
