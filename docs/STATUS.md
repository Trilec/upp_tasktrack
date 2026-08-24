# TaskTrack Status

## Current recovery state — 2026-08-24

TaskTrack development is directly on `main`; remote GitHub is authoritative and no feature/review/checkpoint branch is used.

Current objective: finish the live TaskTrack human↔agent lifecycle and close the 0.2.x project line after Windows/Codex acceptance.

### Version identity

- Accepted release line: `0.2.0`, schema 2.
- Current validation build: `0.2.1-rc3`.
- `TaskTrackGui.exe --version` shows both the release and validation-build identities.
- `TaskTrackMcp.exe --version` reports the validation build explicitly.
- MCP `version`, server metadata, task/status and assistance responses expose the build identity so live transcripts can prove which binary candidate handled the interaction.
- MCP selftest prints its build identity before running checks.
- Only after final visual/runtime acceptance passes is the release version promoted to `0.2.1`.

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

## Verified rc2 acceptance

At repository SHA `ee277b19606db4750fd7f3299b0dd371b4a62066` with `upp_Ui` SHA `537ad1d7e102d43f5ed8e7f80492d3075aa6583f`:

- `verify.ps1`: PASS.
- `TaskTrackTests`: 142 passed, 0 failed.
- `TaskTrackMcp.exe --version`: validation build `0.2.1-rc2`.
- MCP selftest identified itself as `0.2.1-rc2` and finished `tasktrack-mcp-selftest: ok`.
- Verified MCP SHA-256: `B7C8C6CCE2DAD87C9EE2B2D68FCB25849796F26743E58C99E4472C3CCCE38D89`.

Codex live-flow acceptance on that verified rc2 binary:

1. **Direct Submit — PASS**: one required text question returned `RC-direct-rc2` through the original `create_task` as `direct_create_task_result`, with no polling, JSON inspection, compatibility continuation, or second human chat message.
2. **Suggest / Accept — PASS**: Suggest produced `RC2-final`, human Accept completed the task with answer status `accepted`, and Codex resumed automatically with no additional human chat message.
3. **Close / cancel — PASS**: closing an unanswered task produced terminal `closed`, no human evidence, Codex resumed automatically, and no later reminder was observed.
4. **Three structured answers — PASS**: text `short`, choice `Beta`, and notes `valadate` all returned through the original TaskTrack result without `get_task` or JSON inspection.

These passes establish the primary TaskTrack lifecycle and no-evidence close semantics.

## rc3 visual polish

The final rc2 three-question visual check exposed presentation issues rather than transport defects:

- the workflow status (`Needs decision`) competed with the question title/subtitle header;
- the adaptive dialog height still summed questions too much and left a large unused lower area despite the two-column flow;
- the compact header `×` beside Pause/reminder controls was visually ambiguous.

`0.2.1-rc3` addresses only that presentation slice:

- workflow status and Suggest/Clarify/Use judgement actions now have a dedicated full-width row below the question response rather than occupying group-panel title chrome; the status label expands into reserved space and waiting labels are shorter;
- dialog height estimation now models the actual two-column question packing (maximum height per visual row plus the real gutter) instead of summing every question vertically; tall semantic-control estimates and chrome caps were tightened, while scrolling remains the fallback for larger tasks;
- the agent-mode header `×` is removed and the existing exit action is reparented into the footer as the explicit `Cancel task` button beside `Submit`; the native titlebar close control retains the same close semantics.

Production diff from accepted rc2 presentation checkpoint `8c5891423b1d2176c2b84a3295c8c84618311f71` to rc3 candidate touches only:

- `TaskTrack/Widgets/TaskTrackQuestionState.cpp`
- `TaskTrack/App/TaskTrackAgentLaunch.cpp`
- `TaskTrack/Core/TaskTrackBuild.h`

No human-evidence, MCP transport, or persistence semantics were intentionally changed.

## Validation state

**RC3 VISUAL POLISH — WINDOWS VALIDATION PENDING**

Next gate:

1. Refresh exact current `main` and `upp_Ui`; require clean worktrees.
2. Run repository `verify.ps1`; require `0.2.1-rc3`, 142/142 TaskTrack tests and MCP selftest PASS.
3. Restart Codex with the newly-built pair only after deterministic verification passes.
4. Repeat the three-question text / four-choice / notes visual check. Require no title/subtitle/status collision, materially less unused vertical space, no clipping, and footer `Cancel task` + `Submit` with no compact header `×`.
5. Smoke `Cancel task` on an unanswered task and normal Submit on the mixed task to confirm the already-accepted terminal/direct-return behaviour remains intact.

After PASS: promote release version to `0.2.1`, record final Windows/Codex acceptance, publish the final release checkpoint, and freeze further feature expansion unless a real workflow need appears.
