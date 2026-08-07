# TaskTrack Development Plan

## Current baseline — V0.2 semantic question workflow

The immediate goal is a stable Windows baseline where agents can request one of 18 semantic human response types and TaskTrack renders them compactly with current U++ Ui controls.

### V0.2 deliverables

- [x] Preserve V0.1 task/MCP durability model.
- [x] Integrate TT-001-W1 Windows fixes for U++ task-id formatting and JSON numeric Values.
- [x] Replace the old verification-specific 10-type vocabulary with 18 semantic question types.
- [x] Add schema V2 structured `answer.data` while retaining display text and V1 migration.
- [x] Add `TaskTrack/Widgets` renderer package.
- [x] Render each question as a restrained `UiGroupPanel` title/subtitle/content card.
- [x] Use wrapped `UiBoxLayout` fixed-column composition for responsive 3/2/1-column behaviour.
- [x] Add specialist Position9, Direction8, Range and Gradient controls only where existing Ui controls do not provide the semantic interaction directly.
- [x] Remove generic notes rows from every card; keep `notes` as explicit semantic escape hatch.
- [x] Update MCP tool schema and descriptions so agents choose semantic types, never U++ class names.
- [x] Add a demo with exactly one question of each canonical type.
- [x] Add agent/schema/question documentation and deterministic migration/regression tests.
- [ ] Complete authoritative Windows Release + Debug matrix for the published V0.2 commit.
- [ ] Complete Curt visual acceptance of compact shell/card proportions and all 18 controls.
- [ ] Complete live GUI autosave and reminder acceptance that was not manually exercised in TT-001-W1.

## After V0.2 acceptance

Keep the next work driven by real agent/human use rather than adding form-builder features pre-emptively. Likely candidates:

- richer visual option/reference images if actual workflows need them;
- optional evidence attachment/capture;
- recent-task/recovery inbox if concurrent agent use demonstrates the need;
- tighter TaskTrack/AgentFlow integration through the same Core schema rather than a direct dependency;
- accessibility/keyboard passes for specialist selectors;
- formal MCP Tasks-extension conformance coverage as host adoption matures.

## Non-goals

- arbitrary agent-authored GUI JSON;
- exposing U++ control names over MCP;
- embedding TaskTrack into PatchTrack;
- requiring StateMachine or AgentFlow dependencies for the V0.2 core;
- automatic task expiry/closure;
- uncontrolled accumulation of completed history.
