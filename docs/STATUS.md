# TaskTrack Status

## Recovery log — 2026-08-19

BASE: `9e5ef843f3cecf1df8cfefe450e647b0f1f1d4c2` on `main`

CURRENT SOURCE CANDIDATE: pending Windows validation on current `main`.

TaskTrack development remains directly on `main`; no feature/review/checkpoint branches are used.

## Accepted foundation

- **TT-009 PASS** — four-state workflow and durable human→agent assistance.
- **TT-009-R1 PASS** — assistance lifecycle unified as `pending → answered → (cancelled)` with `continue_with_judgement`; deterministic baseline 97 passed.
- **TT-010 source accepted** — Pass/Fail fast path over canonical `confirm`, boolean `answer.data`, optional verdict note; deterministic baseline 123 passed.
- **TT-010-R1 source accepted** — two-executable distribution: `TaskTrackGui.exe` + argument-free stdio `TaskTrackMcp.exe`; MCP launches the sibling GUI with `--task <path>`.
- **TT-010-R2 machine validation** — Release builds, 142/142 deterministic checks, MCP selftest, CLI/distribution checks passed at `174edf6c51a1222c3898881f183c55b4789e9ec6`.

## Live-host correction

Real Codex MCP dogfood exposed UX/lifecycle issues that machine validation could not establish: the GUI could launch behind the host, single-question tasks were oversized, reminders could appear too early, completion required too many explicit steps, and the agent-side assistance round-trip was not sufficiently live.

`9e5ef843f3cecf1df8cfefe450e647b0f1f1d4c2` corrected that boundary:

- MCP-launched GUI has a dedicated focused-dialog lifecycle;
- foreground/restore is attempted explicitly on Windows;
- reminder timing starts from actual GUI presentation;
- Submit persists `Completed` then closes;
- closing after all required evidence is present also completes and closes;
- closing with required evidence missing records `Closed` without fabricating an answer;
- agent-facing `get_task` supports bounded waiting for completion, closure, or a pending human→agent assistance request;
- TaskTrack JSON remains internal durable storage; agents receive evidence through MCP, not by reading JSON files.

## 2026-08-19 Ui convergence and compact-dialog pass

TaskTrack was rechecked against current `upp_Ui/main` after the Ui model/view overhaul and button interaction hardening.

Current dependency inspected:

- `upp_Ui/main`: `1c239c68c504919e60859955db4faf9ea537d181`

The current Ui ownership contract is `SetModel(...)` / `UseInternalModel()` / `Model()` / `ClearModel()`. TaskTrack still contained retired `GetInternalModel()` / `GetModel()` calls in List/Tree/Rank rendering. These were migrated on `main`; no compatibility shim or Ui rollback was introduced.

The agent-launched TaskTrack shell was also tightened for real use:

- one simple decision targets a compact `660×350` dialog with a reduced task-area minimum;
- one visually/structurally richer decision receives more room (`720×440`);
- 2–4 items use an intermediate `880×560` shell;
- larger tasks retain a workspace-sized fallback;
- redundant top state/progress remains hidden; the footer is the progress authority;
- Save/export remains hidden in agent mode because edits autosave;
- Paused-reminder and Agent-nudge controls are hidden in agent mode;
- a one-item dialog also hides Pause/Reminder configuration, leaving the decision, progress, Submit and compact close affordance;
- delayed foreground callback uses a weak `Ptr` guard and terminal paths cancel agent reminder/foreground callbacks.

Source checkpoints:

- `cc423f11a9b4f572d090a579d73e0820c94cd5fe` — compact MCP dialog/lifecycle refinement.
- `8e8193f1e38aad39478fec75a3d2e2a35f4b7c15` — current Ui model ownership API migration.

## Human evidence contract

TaskTrack uses four local workflow states:

- grey = normal agent proposal / suggested baseline;
- orange = required item with no responsible agent proposal;
- green = human-resolved, manually or by explicit proposal acceptance;
- red = required item still unresolved after attempted continuation/submit.

Recommendations are advisory. Defaults, recommendations, and agent replies never silently become human evidence. `items[].answer.data` remains authoritative.

A durable `<task>.agent.json` sidecar carries human→agent assistance separately from authoritative human-answer storage:

- `propose_answer` → `recommended` required;
- `clarify`, `mode=simplify` → `clarification` required; `recommended` optional;
- `continue_with_judgement` → no response payload; delegates judgement to the agent and never writes human evidence.

Agent-facing guidance is intentionally terse: use TaskTrack for durable human evidence the agent cannot establish (or when TaskTrack is explicitly requested), not ordinary conversation; retain `task_id`; obtain results through MCP; `closed` with no answer means no human evidence.

## Validation state

Source/static review against current Ui: **IMPLEMENTATION COMPLETE — PLATFORM VALIDATION PENDING**.

Still required before release closure:

1. Build TaskTrack against current `upp_Ui/main` on Windows (Release plus a quick Debug/BLITZ compile check).
2. Run TaskTrack deterministic tests and MCP selftest.
3. Real Codex MCP smoke: foreground, compact one-item shell, Submit/auto-close, close-with-answer, close-without-answer, and result returned through MCP.
4. Real assistance smoke: Suggest (and preferably Clarify) must reach the agent, receive `respond_to_request`, leave Waiting, and remain advisory until explicit human acceptance.
5. Confirm no immediate reminder and no residual TaskTrack GUI/reminder after terminal close.
6. Quick 3-item task visual check to confirm adaptive sizing remains usable.

After that PASS, update release/version/acceptance documentation and close the project.
