# TaskTrack Status

## Current recovery state — 2026-08-24

TaskTrack development is directly on `main`; remote GitHub is authoritative and no feature/review/checkpoint branch is used.

Current objective: finish the live TaskTrack human↔agent lifecycle and close the 0.2.x project line after Windows/Codex acceptance.

### Version identity

- Accepted release line: `0.2.0`, schema 2.
- Current validation build: `0.2.1-rc2`.
- `TaskTrackGui.exe --version` shows both the release and validation-build identities.
- `TaskTrackMcp.exe --version` reports the validation build explicitly.
- MCP `version`, server metadata, task/status and assistance responses expose the build identity so live transcripts can prove which binary candidate handled the interaction.
- MCP selftest prints its build identity before running checks.
- Only after final live acceptance passes is the release version promoted to `0.2.1`.

This distinction exists specifically to prevent stale binaries or a different output copy from being mistaken for the current validation candidate.

## Accepted foundation

- 18 canonical semantic public question types.
- `items[].answer.data` is authoritative human evidence; `answer.value` is display/log only.
- Recommendations/defaults/agent replies remain advisory until explicit human action.
- Pass/Fail uses canonical `confirm`, not a nineteenth public type.
- Distribution remains two executables: `TaskTrackMcp.exe` plus sibling `TaskTrackGui.exe`.
- Core remains GUI-independent; MCP remains a thin transport layer.
- `.tasktrack.json` and `.agent.json` are durability/recovery storage, never the normal answer transport.
- Current Ui ownership contract is `SetModel(...)` / `UseInternalModel()` / `Model()` / `ClearModel()`; TaskTrack no longer uses retired model APIs.

## Live interaction lifecycle

Human task completion and human→agent assistance are separate state concepts.

Only main task states `completed` and `closed` are terminal.

The assistance sidecar persists the non-terminal interaction phase:

```text
awaiting_human -> awaiting_agent -> awaiting_human
```

- Suggest / Clarify / Use judgement queue a durable request and enter `awaiting_agent`.
- The GUI stays open while waiting for the agent.
- Resolving all pending assistance requests returns the interaction to `awaiting_human`.
- MCP status exposes `interaction_state` and `task_terminal` so an assistance checkpoint cannot be mistaken for task completion.
- Accepting the only required returned recommendation is an explicit human finalization action and auto-completes/closes without a second Submit.
- Ordinary text editing does not auto-complete while the human types; Submit remains its explicit final action.
- Terminal task state outranks sidecar state: stale pending requests cannot resurrect `agent_action_required` after completion/close.

A legitimate race is supported: an assistance response already in flight may arrive just after the human independently completes/closes. TaskTrack may settle that advisory sidecar request, but it cannot change `answer.data`, reopen the task, or make a terminal task non-terminal.

Normative detail: `docs/INTERACTION_LIFECYCLE.md` and `docs/MCP.md`.

## Host behaviour

Normal launched `create_task` owns the live human interaction until terminal human state.

When a modern host advertises the required sampling/MRTR capability, Suggest/Clarify may use `input_required` and retry the same interaction with `requestState` + `inputResponses`.

When the host does not expose that capability, TaskTrack uses the structured non-terminal compatibility continuation and the agent resolves pending requests then waits on `get_task(..., wait_ms=300000)` without requiring a human wake-up message.

## Agent dialog sizing

Agent-launched windows estimate useful size from the actual semantic question model instead of a fixed 1/4/many preset. Question count, semantic type, choice count, instruction length and naturally taller controls contribute to the estimate. The dialog is clamped to the current desktop work area and prefers scrolling over an unnecessarily large window.

## Latest validation history

Earlier builds reported two MCP self-test failures:

```text
live create_task did not return terminal human evidence directly
live create_task retry did not apply sampling response to advisory channel
```

`0.2.1-rc2` corrected the deterministic MRTR test order and added explicit binary identity. A later repeated failure was traced to a stale/different executable: the repository source contained rc2 identity and corrected tests while the executed binary did not. `verify.ps1` was hardened to remove old targets, fail on locked outputs, print the exact MCP identity, require the validation build, then run deterministic tests.

### Verified Windows binary gate

At repository SHA `ee277b19606db4750fd7f3299b0dd371b4a62066` with `upp_Ui` SHA `537ad1d7e102d43f5ed8e7f80492d3075aa6583f`:

- `verify.ps1`: PASS.
- `TaskTrackTests`: 142 passed, 0 failed.
- `TaskTrackMcp.exe --version`: validation build `0.2.1-rc2`.
- MCP selftest identified itself as `0.2.1-rc2` and finished `tasktrack-mcp-selftest: ok`.
- Verified MCP SHA-256: `B7C8C6CCE2DAD87C9EE2B2D68FCB25849796F26743E58C99E4472C3CCCE38D89`.

### Verified Codex live flow

Using that verified rc2 binary:

1. **Direct Submit — PASS**
   - one required text question;
   - no Suggest/Clarify/Use judgement;
   - human submitted `RC-direct-rc2`;
   - original `create_task` returned `delivery: direct_create_task_result` with `RC-direct-rc2`;
   - no polling, JSON inspection, compatibility continuation, or second human chat message was required.

2. **Suggest / Accept — PASS**
   - human requested Suggest from the open TaskTrack GUI;
   - agent assistance round-trip completed automatically;
   - proposal `RC2-final` returned to the still-live task;
   - human explicitly accepted it;
   - task state became `completed`, answer status `accepted`, returned answer `RC2-final`;
   - no additional human chat message was required.

These two passes establish the primary live TaskTrack lifecycle: direct human completion and non-terminal human→agent assistance both return control to Codex automatically.

## Validation state

**LIVE FLOW ACCEPTED — FINAL SANITY PENDING**

Remaining checks before promoting `0.2.1-rc2` to release `0.2.1`:

1. Close an unanswered one-question task with the window close control. Require terminal `closed`, no human evidence, Codex resumes automatically, and no later reminder appears.
2. Create a small mixed 3-question task and confirm adaptive dialog sizing is sensible: no clipped controls, no excessive empty space, and scrolling is preferred when content exceeds the work area.

After PASS: promote release version to `0.2.1`, record final Windows/Codex acceptance, publish the final release checkpoint, and freeze further feature expansion unless a real workflow need appears.
