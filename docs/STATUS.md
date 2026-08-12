# TaskTrack Status

## Recovery log — 2026-08-12

BASE: `0c8255149b8048da1a30be184dff836683dab860` on `main`

TASK: TT-009 durable human→agent assistance and four-state workflow

STATUS:

- TT-008-W1 is accepted: Release/BLITZ PASS, TaskTrackTests 73/73, MCP selftest PASS, 8px question-body inset PASS, native range presentation PASS, responsive layout PASS, no assertions/crashes.
- Development remains directly on `main`; no TT-009 feature branch was created.
- TaskTrack now uses four local workflow states rather than overloading global Ui roles:
  - grey = normal agent proposal / suggested baseline;
  - orange = required item with no responsible agent proposal;
  - green = human-resolved, manually or by explicit proposal acceptance;
  - red = required item still unresolved after attempted continuation/submit.
- Recommendations are now the expected agent fast path: create-task guidance says to provide `recommended` unless no responsible proposal is possible. A required item without `recommended` intentionally means the human must decide it.
- A durable `<task>.agent.json` sidecar carries human→agent assistance traffic separately from authoritative human answers. This prevents agent responses from racing GUI answer autosave or masquerading as `answer.data`.
- Compact request actions are:
  - `propose_answer` → agent must return `recommended`;
  - `clarify` with `mode=simplify` → agent must return `clarification`; `recommended` is optional.
- MCP now exposes `agent_action_required` and `pending_requests` from `get_task`, plus `respond_to_request` for agent replies. Replies are validated against the referenced semantic item and remain advisory.
- Question headers expose `Suggest` when a required item has no proposal and `?` for plain-language clarification. Pending requests survive MCP/client restarts. While the GUI remains open, the relevant question polls its durable sidecar; a returned recommendation becomes the new grey proposal and still requires explicit human acceptance before the card becomes green.
- Clarification preserves the original question/instruction and adds the latest plain-language explanation; TaskTrack does not become a free-form chat surface.
- Existing recommendation acceptance, defaults-non-evidence, category rebuild, range, persistence and responsive-grid behavior are intended to remain unchanged.

PUBLISHED CHECKPOINTS:

- `4a78a171602719816dba4a504717a423cf386fd8` — durable agent request sidecar.
- `38af2e832ef70cf5c4362e88d9633ed4cbb83efa` — compact MCP request/response contract.
- `c55cf1de6d8ff2fcae417b4d5177c40c48bcac93` — four-state question assistance UI.
- later documentation commits on `main` update the agent contract/status only.

CURRENT DEPENDENCY CONTEXT:

- Use current `upp_Ui/main`; TT-009 source review baseline was `5b398818a11db06e9a3a9511efaa7e6f190b7793` or a descendant.
- No Ui source change is required for TT-009.

CORE/MCP PATHS:

- `TaskTrack/Core/TaskTrackAgent.h`
- `TaskTrack/Core/Core.upp`
- `TaskTrack/Mcp/TaskTrackAgentMcp.h`
- `TaskTrack/Mcp/main.cpp`
- `TaskTrack/Mcp/Mcp.upp`

UI PATHS:

- `TaskTrack/Widgets/TaskTrackWidgets.h`
- `TaskTrack/Widgets/TaskTrackQuestionState.cpp`
- `TaskTrack/Widgets/Widgets.upp`

DOCUMENTATION:

- `docs/AGENT_GUIDE.md`
- `docs/STATUS.md`

VALIDATION:

- TT-008-W1 baseline: PASS.
- TT-009 source/API review: in final publication review.
- TT-009 Windows Release/Debug/runtime/MCP/OpenCode dogfood: PENDING Gary.

NEXT:

1. Verify final `main` diff from TT-008 and remote HEAD.
2. Gary runs TT-009-W1 against current `upp_Ui/main`.
3. Prove the four workflow colours and no regression in the accepted 18-question workspace.
4. Prove `Suggest` creates durable `propose_answer`, MCP surfaces it, `respond_to_request` supplies a valid proposal, the open GUI updates, and only human Accept creates `answer.data`.
5. Prove `?` creates `clarify/simplify`, the agent response survives reconnect and is shown without replacing the original question.
6. Dogfood through the already-installed OpenCode TaskTrack MCP and leave the GUI open for Curt.
