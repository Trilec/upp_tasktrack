# TaskTrack Status

## Current recovery state — 2026-08-23

TaskTrack development is directly on `main`; remote GitHub is authoritative and no feature/review/checkpoint branch is used.

Current objective: finish the live TaskTrack human↔agent lifecycle and close the 0.2.x project line after Windows/Codex acceptance.

### Version identity

- Accepted release line: `0.2.0`, schema 2.
- Current validation build: `0.2.1-rc2`.
- `TaskTrackGui.exe --version` shows both the release and validation-build identities.
- `TaskTrackMcp.exe --version` reports the validation build explicitly.
- MCP `version`, server metadata, task/status and assistance responses expose the build identity so live transcripts can prove which binary candidate handled the interaction.
- MCP selftest prints its build identity before running checks.
- Only after live acceptance passes is the release version promoted to `0.2.1`.

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

Agent-launched windows estimate useful size from the actual semantic question model instead of a fixed 1/4/many preset. Question count, semantic type, choice count, instruction length and naturally taller controls contribute to the estimate. The dialog is clamped to the current desktop work area and prefers scrolling over an unnecessarily large window.

## Latest validation history

At `ce051094a0d08f25df40688c6d11834d2764f097`, and again from the binary reported while the repository was at `41f45d711561cc4a54419b4cf2fc136a691d4d60`, MCP selftest reported:

```text
live create_task did not return terminal human evidence directly
live create_task retry did not apply sampling response to advisory channel
```

The deterministic MRTR selftest was still ordering the phases incorrectly: it marked the human task `completed` before applying the pending sampling response. The normal lifecycle is `input_required -> apply agent response -> awaiting_human -> human completion -> terminal create_task result`.

`0.2.1-rc2` corrects the deterministic test to validate those phases in lifecycle order, while retaining separate support for a genuinely late in-flight advisory response after terminal human completion. It also makes the validation build visible through CLI, MCP metadata/results and selftest output so the exact executable under test can be confirmed before interpreting failures.

A later Windows report at repository SHA `f364c70def2e0a0394466669a0e14193a9309ee8` still executed a binary whose `--version` output did not contain `validation build 0.2.1-rc2` and whose selftest emitted the pre-rc2 failures. The source at that SHA does contain both the build-version output and corrected selftest. Therefore that result is classified as **stale/different binary evidence, not an rc2 source validation**.

`verify.ps1` is now fail-safe for this case: it removes each old target before building, fails if Windows has the target locked, prints the exact MCP path/timestamp/size/SHA-256, executes `--version`, and requires the validation build read from `TaskTrack/Core/TaskTrackBuild.h` before any deterministic selftest is accepted.

## Validation state

**IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING**

Next gate:

1. Refresh exact current `main` and record SHA.
2. Run the repository `verify.ps1` rather than relying on an unrelated/stale output copy.
3. Require the freshly-linked `build/TaskTrackMcp.exe --version` to contain `validation build 0.2.1-rc2`.
4. Require `TaskTrackTests` to pass and the same executable's selftest to print `tasktrack-mcp-selftest build 0.2.1-rc2` and finish `tasktrack-mcp-selftest: ok`.
5. Quick Debug/BLITZ compile for GUI + MCP.
6. Restart Codex only after that gate passes.
7. Direct Submit test: no Suggest, answer + Submit, GUI closes, original interaction returns human evidence without JSON/manual poll/human wake-up.
8. Suggest test: GUI stays open in `awaiting_agent`; agent compatibility/MRTR response returns proposal; phase returns `awaiting_human`; Accept auto-completes; agent receives terminal result without a human wake-up message.
9. Quick multi-question sizing check.

After PASS: promote release version to `0.2.1`, record final Windows/Codex acceptance, publish final release checkpoint, and freeze further feature expansion unless a real workflow need appears.
