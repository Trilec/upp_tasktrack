# TaskTrack Status

## Current recovery state — 2026-08-23

TaskTrack development is directly on `main`; remote GitHub is authoritative and no feature/review/checkpoint branch is used.

Current objective: finish the live TaskTrack human↔agent lifecycle and close the 0.2.x project line after Windows/Codex acceptance.

### Version identity

- Accepted release line: `0.2.0`, schema 2.
- Current validation build: `0.2.1-rc1`.
- `TaskTrackGui.exe --version` shows both the release and validation-build identities.
- Normal MCP task/status and assistance responses expose `build_version` so live transcripts can prove which binary candidate handled the interaction.
- Only after live acceptance passes is the release version promoted to `0.2.1`.

This distinction exists specifically to prevent stale binaries from being mistaken for the current validation candidate.

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

Real Codex dogfood established that human task completion and human→agent assistance must be separate state concepts.

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

A legitimate race is also supported: an assistance response already in flight may arrive just after the human independently completes/closes. TaskTrack may settle that advisory sidecar request, but it cannot change `answer.data`, reopen the task, or make a terminal task non-terminal.

Normative detail: `docs/INTERACTION_LIFECYCLE.md` and `docs/MCP.md`.

## Host behaviour

Normal launched `create_task` owns the live human interaction until terminal human state.

When a modern host advertises the required sampling/MRTR capability, Suggest/Clarify may use `input_required` and retry the same interaction with `requestState` + `inputResponses`.

Tested Codex configurations have not advertised sampling. In that case TaskTrack returns a non-terminal compatibility checkpoint:

```text
interaction_state: awaiting_agent
task_terminal: false
agent_action_required: true
human_followup_required: false
```

The agent must immediately resolve `pending_requests` using `respond_to_request`, then block in `get_task(..., wait_ms=300000)` until the human completes/closes or requests further assistance. The human must not need to type “done”, “check TaskTrack”, or any other wake-up message.

## Agent dialog sizing

Agent-launched windows now estimate useful size from the actual semantic question model instead of a fixed 1/4/many preset. Question count, semantic type, choice count, instruction length and naturally taller controls contribute to the estimate. The dialog is clamped to the current desktop work area and prefers scrolling over an unnecessarily large window.

## Latest validation history

At `ce051094a0d08f25df40688c6d11834d2764f097` Windows builds and TaskTrackTests passed, but MCP selftest stopped with:

```text
live create_task did not return terminal human evidence directly
live create_task retry did not apply sampling response to advisory channel
```

Supervisor diagnosis: the deterministic selftest deliberately completed the human task while a previously-issued sampling response was still in flight. The newly-added terminal guard rejected that late advisory response. The correct race semantics are to settle the already-issued sidecar response while preserving terminal human authority. The MCP assistance boundary has been corrected accordingly; the next validation must rerun the selftest before any live GUI test.

## Validation state

**IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING**

Next gate:

1. Refresh exact current `main` and record SHA.
2. Build Release `TaskTrackGui`, `TaskTrackMcp`, `TaskTrackTests`.
3. Run `TaskTrackTests` and `TaskTrackMcp.exe --selftest`; both must pass.
4. Confirm GUI reports validation build `0.2.1-rc1` and a normal MCP task/status response reports `build_version: 0.2.1-rc1`.
5. Quick Debug/BLITZ compile for GUI + MCP.
6. Restart Codex with fresh MCP binary.
7. Direct Submit test: no Suggest, answer + Submit, GUI closes, original interaction returns human evidence without JSON/manual poll/human wake-up.
8. Suggest test: GUI stays open in `awaiting_agent`; agent compatibility/MRTR response returns proposal; phase returns `awaiting_human`; Accept auto-completes; agent receives terminal result without a human wake-up message.
9. Quick multi-question sizing check.

After PASS: promote release version to `0.2.1`, record final Windows/Codex acceptance, publish final release checkpoint, and freeze further feature expansion unless a real workflow need appears.
