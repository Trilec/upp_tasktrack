# TaskTrack Status

## Recovery log — 2026-08-12

BASE: `0c8255149b8048da1a30be184dff836683dab860` on `main`

TASK: TT-009 durable human→agent assistance and four-state workflow

STATUS:

- TT-008-W1 is accepted: Release/BLITZ PASS, TaskTrackTests 73/73, MCP selftest PASS, 8px question-body inset PASS, native range presentation PASS, responsive layout PASS, no assertions/crashes.
- Development remains directly on `main`; no TT-009 feature branch was created.
- TaskTrack now uses four local workflow states rather than remapping global Ui roles:
  - grey = normal agent proposal / suggested baseline;
  - orange = required item with no responsible agent proposal;
  - green = human-resolved, manually or by explicit proposal acceptance;
  - red = required item still unresolved after attempted continuation/submit.
- `recommended` is the expected agent fast path. MCP creation guidance says to provide it unless no responsible proposal is possible. A required item without one intentionally creates real human work.
- A durable `<task>.agent.json` sidecar carries human→agent assistance separately from authoritative human-answer JSON.
- Compact request actions are exact:
  - `propose_answer` → `recommended` required;
  - `clarify`, `mode=simplify` → `clarification` required; `recommended` optional.
- MCP `get_task` exposes `agent_action_required` + `pending_requests`; `respond_to_request` resolves the compact request and validates any recommendation against the referenced semantic item. Agent replies never write `TaskTrackAnswer`.
- Required/no-proposal cards expose `Suggest`; unanswered cards expose `?` for simplification. The relevant open card polls the durable sidecar while assistance is pending. A returned proposal becomes an advisory grey proposal and still requires explicit human acceptance before green/resolved evidence exists.
- Clarification keeps the original question/instruction and adds the latest plain-language explanation; this remains a decision workspace, not a free-form chat surface.
- Pending requests and responses survive MCP/client restarts. `task_id` remains the durable handle.

PUBLISHED IMPLEMENTATION CHECKPOINTS:

- `4a78a171602719816dba4a504717a423cf386fd8` — durable agent request sidecar.
- `38af2e832ef70cf5c4362e88d9633ed4cbb83efa` — compact MCP request/response contract.
- `c55cf1de6d8ff2fcae417b4d5177c40c48bcac93` — four-state question assistance UI.
- `a95d1b97bd12713d165c7c842692d1fe33a07824` — documentation/changelog boundary before final handoff status.

CURRENT DEPENDENCY CONTEXT:

- Use current `upp_Ui/main`; TT-009 source review baseline was `5b398818a11db06e9a3a9511efaa7e6f190b7793` or a descendant.
- No Ui source changes are required for TT-009.

TOUCHED SOURCE/PACKAGE PATHS:

- `TaskTrack/Core/TaskTrackAgent.h`
- `TaskTrack/Core/Core.upp`
- `TaskTrack/Mcp/TaskTrackAgentMcp.h`
- `TaskTrack/Mcp/main.cpp`
- `TaskTrack/Mcp/Mcp.upp`
- `TaskTrack/Widgets/TaskTrackWidgets.h`
- `TaskTrack/Widgets/TaskTrackQuestionState.cpp`
- `TaskTrack/Widgets/Widgets.upp`

DOCUMENTATION:

- `docs/AGENT_GUIDE.md`
- `docs/STATUS.md`
- `CHANGELOG.md`

SOURCE REVIEW:

- TT-008 accepted Core/App/renderer behavior remains the base.
- Package membership/dependency direction checked: Core agent sidecar depends only on Core; MCP bridge depends on TaskTrack/Core; Widgets uses TaskTrack/Core and current Ui; App public API/schema is unchanged.
- Main human answer JSON and agent-assistance sidecar have separate writers; MCP response validation never commits human evidence.
- Existing 18 semantic item builders remain in `TaskTrackWidgets.cpp`; TT-009 moved workflow-state logic out of the header into one normal `.cpp` rather than fragmenting source.
- Current MCP 2026-07-28 Tasks path remains compatible with the existing task handle; the portable model-controlled fallback is the explicit `get_task` / `respond_to_request` tool pair.
- No TaskTrack schema-version bump is required because agent assistance is a separate sidecar and existing `answer.data` remains unchanged.

VALIDATION:

- TT-008-W1 baseline: PASS.
- TT-009 source/API/package/diff review: PASS.
- TT-009 Windows compile/runtime/MCP/OpenCode dogfood: PENDING Gary.

NEXT:

1. Gary runs TT-009-W1 against current TaskTrack `main` and current `upp_Ui/main`.
2. Prove Release + Debug/BLITZ, 73-test regression baseline or legitimate higher count, and MCP selftest.
3. Prove grey/orange/green/red state transitions.
4. Prove `Suggest` → durable `propose_answer` → MCP reply → still-open GUI proposal → explicit human Accept → green evidence.
5. Prove `?` → durable `clarify/simplify` → plain-language reply, optional proposal, original question retained.
6. Restart MCP between request/reply and prove durable recovery.
7. Run an unnamed `opencode run` discovery test so we learn whether the agent chooses TaskTrack from its tool contract without being named.
8. Leave the GUI open for Curt.
