# TaskTrack Development Plan

## Current phase — agent-ready verification fast path (TT-010)

TaskTrack's source/product/documentation work is complete for its first genuinely usable agent-testing state. The 18 semantic types remain canonical; ordinary human verification is represented as a `confirm` item with `choices = ["Pass", "Fail"]` (boolean `answer.data`, optional `answer.note`). The human→agent assistance loop is unified: `propose_answer`, `clarify`, `continue_with_judgement`, request lifecycle `pending → answered → (cancelled)`.

## Completed milestones

- **V0.1** — durable task model, GUI, MCP, pause/reminders, exports, tests, acceptance materials.
- **TT-001-W1** — Windows fixes (U++ task-id formatting, JSON numeric Values).
- **V0.2** — 18 semantic question types, schema V2 structured `answer.data`, `TaskTrack/Widgets` renderer, responsive equal-width card grid, semantic MCP vocabulary.
- **TT-003/004/005/006** — deterministic equal-width columns, category clipping fixes, compact header, native `UiRangeSliderEdit`, timer-id and teardown crash fixes.
- **TT-007** — local workflow states (grey/orange/green/red), actionable recommendations.
- **TT-008-W1** — 8px question-body inset, native range, accepted.
- **TT-009** — durable two-way human→agent assistance (sidecar) + four-state workflow.
- **TT-009-R1** — unified assistance protocol (`pending/answered/cancelled`, added `continue_with_judgement`), legacy `resolved` migration.
- **TT-010** — Pass/Fail verification fast path, optional verdict note, agent-facing MCP/documentation finalized. Source + deterministic tests complete.

## Remaining blocking phase

The remaining work is authoritative platform/host acceptance, not new feature work:

1. Freeze the code and run authoritative **Windows Release + Debug/BLITZ** validation.
2. Install/register the MCP binary into **OpenCode first**, run unnamed-agent discovery + real TaskTrack dogfood.
3. Install/register into **Codex second**, run unnamed-agent discovery + dogfood.
4. **Curt visual acceptance** of the compact shell, Pass/Fail + note, four states, and assistance controls.

## Deferred / speculative (explicitly not in scope until real use demands them)

- visual option/reference images in questions;
- evidence attachment/capture;
- recent-task/recovery inbox;
- accept-all and agent timeout/"accept my proposal and continue" affordances;
- TaskTrack/AgentFlow integration through the same Core schema;
- accessibility/keyboard passes for specialist selectors;
- formal MCP Tasks-extension conformance coverage as host adoption matures.

## Non-goals

- arbitrary agent-authored GUI JSON;
- exposing U++ control names over MCP;
- embedding TaskTrack into PatchTrack;
- requiring StateMachine or AgentFlow dependencies in the core;
- automatic task expiry/closure;
- uncontrolled accumulation of completed history;
- adding a 19th semantic question type.
