# TaskTrack Status

## Current recovery state — 2026-08-25

TaskTrack development is directly on `main`; remote GitHub is authoritative and no feature/review/checkpoint branch is used.

Current objective: finish the live TaskTrack human↔agent lifecycle and close the 0.2.x project line after Windows/Codex acceptance.

### Version identity

- Accepted release line: `0.2.0`, schema 2.
- Current validation build: `0.2.1-rc5`.
- `TaskTrackGui.exe --version` shows both the release and validation-build identities.
- `TaskTrackMcp.exe --version` reports the validation build explicitly.
- MCP `version`, server metadata, task/status and assistance responses expose the build identity so live transcripts can prove which binary candidate handled the interaction.
- MCP selftest prints its build identity before running checks.
- Only after final visual/runtime acceptance passes is the release version promoted to `0.2.1`.

This distinction exists specifically to prevent stale binaries or a different output copy from being mistaken for the current validation candidate.

## Accepted foundation

- 18 canonical semantic public question types.
- `items[].answer.data` is authoritative human answer evidence; `answer.value` is display/log only.
- Recommendations/defaults/agent replies remain advisory until explicit human action.
- Pass/Fail uses canonical `confirm`, not a nineteenth public type.
- Distribution remains two executables: `TaskTrackMcp.exe` plus sibling `TaskTrackGui.exe`.
- Core remains GUI-independent; MCP remains a thin transport layer.
- `.tasktrack.json` and `.agent.json` are durability/recovery storage, never the normal answer transport.
- Current Ui ownership contract is `SetModel(...)` / `UseInternalModel()` / `Model()` / `ClearModel()`; TaskTrack no longer uses retired model APIs.

## Live interaction lifecycle

Human answer completion and human→agent assistance/delegation are separate state concepts.

Only main task states `completed` and `closed` are terminal.

The assistance sidecar persists the live interaction phase:

```text
awaiting_human -> awaiting_agent -> awaiting_human
```

- Suggest / Clarify / Use judgement queue a durable request and enter `awaiting_agent`.
- The GUI stays open while the request is pending.
- Suggest/Clarify responses return the interaction to `awaiting_human`; they do not create human evidence.
- Accepting a returned recommendation is explicit human finalization and may auto-complete without a second Submit.
- Ordinary text editing does not auto-complete while the human types; Submit remains its explicit final action.
- Terminal task state outranks sidecar state: stale pending requests cannot resurrect `agent_action_required` after completion/close.

Use judgement is explicit human delegation, not a human answer. After the agent acknowledges `continue_with_judgement`, TaskTrack may close the human-facing task when every still-required unanswered item is delegated. The main state is `closed`; the sidecar preserves delegation authority; delegated items retain empty `answer.data`. MCP exposes `delegated_to_agent=true`, `closure_reason=agent_judgement`, and `delegated_item_ids` so the agent can continue under its own judgement without claiming the judgement came from the human.

A legitimate race is supported: an assistance response already in flight may arrive just after the human independently completes/closes. TaskTrack may settle that sidecar request, but it cannot change `answer.data`, reopen the task, or make a terminal task non-terminal.

Normative detail: `docs/INTERACTION_LIFECYCLE.md` and `docs/MCP.md`.

## Host behaviour

Normal launched `create_task` owns the live human interaction until terminal human state.

When a modern host advertises the required sampling/MRTR capability, Suggest/Clarify/Use judgement may use `input_required` and retry the same interaction with `requestState` + `inputResponses`.

When the host does not expose sampling, TaskTrack uses the structured compatibility continuation and the agent resolves pending requests then waits on `get_task(..., wait_ms=300000)` without requiring a human wake-up message. A compatibility fallback that completes automatically with no additional human chat is an accepted host path; lack of native MRTR alone is not a TaskTrack failure.

A terminal MCP result must continue the originating agent turn. Terminal statuses expose `agent_must_continue=true`, `agent_response_required=true`, and `human_followup_required=false`, with a concrete next action for completed, ordinary closed, and delegated closed outcomes.

## Verified rc2 acceptance

At repository SHA `ee277b19606db4750fd7f3299b0dd371b4a62066` with `upp_Ui` SHA `537ad1d7e102d43f5ed8e7f80492d3075aa6583f`:

- `verify.ps1`: PASS.
- `TaskTrackTests`: 142 passed, 0 failed.
- `TaskTrackMcp.exe --version`: validation build `0.2.1-rc2`.
- MCP selftest identified itself as `0.2.1-rc2` and finished `tasktrack-mcp-selftest: ok`.
- Verified MCP SHA-256: `B7C8C6CCE2DAD87C9EE2B2D68FCB25849796F26743E58C99E4472C3CCCE38D89`.

Codex live-flow acceptance on that verified rc2 binary established direct Submit, Suggest/Accept, close/cancel with no evidence, and three structured answers returning automatically through TaskTrack.

## rc3 / rc4 visual and terminal acknowledgement acceptance

`0.2.1-rc3` fixed title/subtitle/status collision, introduced explicit footer `Cancel task`, and materially tightened the multi-question dialog.

`0.2.1-rc4` then tightened window fitting further and made terminal results require a normal visible Codex acknowledgement.

Deterministic rc4 gate at TaskTrack SHA `c6f0f5234ad6f194f2f8c86c952b51fb20ca0da5`, `upp_Ui` SHA `78c8bfb55f1b351ed7d63c66f467eaf308dd7b44`:

- clean worktrees;
- `verify.ps1`: PASS;
- validation build `0.2.1-rc4`;
- TaskTrackTests: 142 passed, 0 failed;
- MCP selftest: `tasktrack-mcp-selftest: ok`;
- MCP SHA-256: `250912C646B48324C24FD1E457B8CEA887236F5BB1AE0087927A25A8BCA5343C`.

Live rc4 results:

1. **Three-question Submit — PASS**
   - window fit accepted; no reported overlap/clipping or excessive empty space;
   - all three structured answers returned;
   - Codex produced a visible acknowledgement naming release label `dfg`, channel `Beta`, note `dfg`;
   - no human follow-up, manual polling or JSON inspection.

2. **Cancel task — PASS**
   - compact one-question flow;
   - terminal `closed`, no human evidence;
   - Codex visibly acknowledged closed/cancelled with no answer;
   - no human follow-up or later reminder.

3. **Suggest / Accept regression — PASS**
   - proposal `RC4` returned;
   - waiting state cleared;
   - Accept auto-completed;
   - visible Codex acknowledgement returned the accepted `RC4` answer;
   - no human follow-up.
   - tested Codex host used the documented compatibility fallback rather than native MRTR; the fallback itself completed automatically and is therefore accepted.

4. **Use judgement — PARTIAL / remaining defect**
   - delegation reached the agent;
   - agent continued automatically;
   - no human evidence was fabricated;
   - visible Codex acknowledgement occurred;
   - but the TaskTrack GUI returned to `awaiting_human` and remained open, forcing the human to close it manually even though the human had explicitly delegated the decision.

The rc4 screenshot also showed the one-question dialog had been tightened slightly too far: the question panel needed a small additional vertical allowance to avoid bottom clipping.

## rc5 candidate

`0.2.1-rc5` is bounded to those final two findings:

- agent-launch GUI watches the durable assistance channel; once every still-required unanswered item has an **answered** `continue_with_judgement`, it closes the human-facing task automatically as `closed` without writing `answer.data`;
- terminal MCP status distinguishes this from ordinary cancellation with `delegated_to_agent=true`, `closure_reason=agent_judgement`, and `delegated_item_ids`, and instructs the agent to continue under its own judgement;
- one-question dialog width remains compact, but its minimum/task-area height receives a small allowance so group-panel and workflow chrome do not clip;
- the TaskTrack skill, MCP contract and interaction-lifecycle docs define acknowledged delegation as a terminal human-interaction outcome while preserving the human-answer evidence boundary.

No new public question type or main task state is introduced. No recommendation is promoted to human evidence. The schema remains 2.

## Validation state

**RC5 — WINDOWS/CODEX VALIDATION PENDING**

Next gate:

1. Refresh latest `main` and `upp_Ui`; require clean worktrees.
2. Run repository `verify.ps1`; require validation build `0.2.1-rc5`, TaskTrack tests and MCP selftest PASS.
3. Restart Codex/new session against the freshly-built MCP binary only after deterministic verification passes.
4. One required text question: visually confirm the card no longer clips while the dialog remains compact.
5. Press Use judgement and send no further human chat message. Require automatic agent acknowledgement, no human `answer.data`, automatic GUI close, terminal `closed` with `delegated_to_agent=true` / `closure_reason=agent_judgement`, and visible Codex continuation under agent judgement.
6. Quick Suggest/Accept smoke to ensure the accepted rc4 path remains intact.

After PASS: promote release version to `0.2.1`, record final Windows/Codex acceptance, publish the release checkpoint, and freeze further feature expansion unless a real workflow need appears.
