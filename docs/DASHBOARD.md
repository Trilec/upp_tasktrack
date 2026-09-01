# TaskTrack Dashboard contract

TaskTrack Dashboard is the presentation half of TaskTrack. The original TaskTrack interaction asks a person for durable evidence. Dashboard presents durable AI-maintained project state. They may link to each other, but they do not share authority.

## Design goal

An agent should be able to replace a long prose status dump with a predictable native visual presentation without designing pixels or naming U++ controls.

The agent supplies semantic project information. TaskTrack chooses native controls and layout.

A dashboard is therefore a **container of independent semantic panels**, not one fixed dashboard layout. A quick status might contain Project State, Timeline and Next Steps. A long-running project may expose several categories and larger Objective/Attention/Verification panels. The GUI stacks the selected panels vertically and allows each one to expand.

## Semantic panel vocabulary

Schema 1 has eight panel types:

- `project_state` — compact “where are we?” state, typically Now / Next checkpoint / Main concern plus overall progress.
- `timeline` — ordered milestones/checkpoints rendered through the TaskTrack-local timeline rail.
- `progress_list` — objectives/workstreams/features with individual progress and evidence.
- `action_list` — ordered next steps or recommended actions.
- `attention` — unresolved risks, blockers, approvals, decisions or other matters requiring awareness/intervention.
- `verification` — tests, audits, acceptance evidence; problems should be supplied first when relevant.
- `changes` — important changes since an earlier state/revision.
- `records` — structured project information that does not fit a more specific panel.

Do not create new panel types merely to change a title. For example Objectives and Workstreams can both use `progress_list`.

## Categories

`category` is navigation, not layout. Typical categories are Overview, Plan, Attention, Quality and History, but project-specific categories are valid. The GUI provides `All` plus the categories present in the document.

## Density and preview limits

Each panel has:

- `density`: `summary`, `standard` or `full`;
- `preview_limit`: 1..100.

Density is a rendering hint. `preview_limit` is the bounded first view. The user can expand a panel to inspect all current entries. The current document itself is bounded to 64 panels and 1000 entries per panel.

Large historical truth should not accumulate forever in current JSON. Keep current state current and use immutable dashboard revisions for history.

## Entry fields

Every entry requires globally unique `id` and a human-readable `title`. Useful optional fields include:

- `subtitle`
- `status`
- `detail`
- `category`
- `progress` (0..100)
- `weight` (>0)
- `attention`
- `archived`
- `timestamp`
- `task_id`
- `evidence`
- `data` for additional structured information

Entry IDs are global within one dashboard document, not merely unique inside their panel. Stable IDs allow deterministic comparison across revisions.

## Status language

The native timeline uses semantic state rather than arbitrary agent colours. Recommended status values are:

- `complete` — finished/accepted (green)
- `active` — current work (accent/blue by default)
- `next` — immediate upcoming checkpoint
- `pending` — later/not started
- `blocked`, `failed`, `error`, `warning`, `attention`, `risk`, `critical` — attention/problem states

Text/status remains visible; colour is not the only carrier.

## Progress

`overall_progress` is optional. When it is present it is an explicit management estimate and is the value shown as the effective overall project percentage.

When it is absent, Core derives progress from non-archived entries in panels with `contributes_to_progress=true`:

`sum(entry.progress * entry.weight) / sum(entry.weight)`

Entries without progress are ignored. Timeline and Progress List panels contribute by default unless explicitly disabled. This makes the ring, timeline and objective numbers originate from one validated model rather than unrelated prose estimates.

`confidence` (0..100) is separate from progress.

## Attention

`attention=true` is explicit. Problem-like status values such as `blocked`, `failed`, `warning`, `risk` and `critical` also count as attention. The dashboard header reports the unresolved attention count.

An attention entry may carry a normal TaskTrack `task_id` so the visual status can point to a separate human-decision workflow. The dashboard itself remains read-only and does not manufacture human evidence.

## Revisions and update authority

Creation starts at revision 1.

Every update to an existing dashboard **must** supply `base_revision` equal to the current revision. If it is missing, Core returns `REVISION_REQUIRED`. If it is stale, Core returns `REVISION_CONFLICT`.

On conflict:

1. read the current dashboard;
2. merge the new repository/test/audit truth with that current state;
3. retry using the new revision.

Never force an old dashboard over newer truth.

This prevents two agents/sessions from silently overwriting each other. The write path also holds a short cross-process lock while it re-reads current state and installs the next revision, closing the simultaneous check-then-write race. A busy writer returns `WRITE_BUSY`; re-read/retry rather than bypassing the lock.

## Storage

Current:

`<store>/<dashboard_id>.tasktrack-dashboard.json`

Accepted revisions:

`<store>/revisions/<dashboard_id>/00000001.json`
`<store>/revisions/<dashboard_id>/00000002.json`
`...`

Current-file replacement is validated/atomic and retains `.bak` recovery. Immutable accepted revision snapshots are never overwritten. If a process dies after preparing N+1 but before installing current N+1, the next locked writer recognizes that beyond-current snapshot as incomplete and replaces it safely.

## Native presentation

`TaskTrackDashboardGui.exe` is read-only. It provides:

- project title/phase/current revision;
- an overall `UiProgressRing`;
- attention count;
- category filtering;
- a vertical stack of independent `UiGroupPanel` semantic renderers;
- compact/full panel expansion;
- current auto-refresh;
- historical revision selection.

The timeline renderer uses `TaskTrackTimelineRail`, which deliberately lives in TaskTrack while its usage is proven. It follows the Ui theme by default and supports custom style, data binding, selection, mouse, keyboard and focus behaviour like other Ui controls.

## Agent selection guide

Use `project_state` for a concise snapshot.

Use `timeline` only when ordered phases/milestones improve understanding.

Use `progress_list` when several objectives/workstreams have their own completion state.

Use `action_list` for future work.

Use `attention` for unresolved matters requiring awareness, decision or intervention.

Use `verification` for test/audit/acceptance state.

Use `changes` when the user asks what changed or a revision delta matters.

Use `records` only when the information does not fit a more specific semantic panel.

For a quick status, prefer a small number of useful panels rather than mechanically sending every panel type.
