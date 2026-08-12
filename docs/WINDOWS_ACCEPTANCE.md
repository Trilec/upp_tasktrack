# Windows + Host Acceptance

This is the authoritative final acceptance scope for the TaskTrack baseline after the TT-010 productization. It has **not** been claimed as passed; it is the definition of the acceptance that remains.

The authoritative source is remote `main`. The validator pulls `main`, confirms the exact TaskTrack commit supplied by the supervisor, and records current `upp_Ui`, `upp_animation`, and `upp_statemachine` SHAs before building. Do not reset newer dependencies merely to match an older reference unless a proven compatibility issue requires a separate corrective task.

Do not commit or push source repairs during the acceptance run. Return the first useful failure and diagnosis.

## Release

From `E:\apps\github\upp_tasktrack`:

```powershell
powershell -ExecutionPolicy Bypass -File .\verify.ps1 -UppRoot E:\upp-18468
```

Required:

- TaskTrack/App PASS
- TaskTrack/Mcp PASS
- tests/TaskTrackTests PASS
- examples/TaskTrackExample PASS
- TaskTrackTests runtime PASS
- TaskTrackMcp `--selftest` PASS
- final line `verify.ps1: ok`

## Debug/BLITZ

Build and run the same four packages with CLANG x64 Debug/BLITZ. Any U++ assertion, access violation, heap diagnostic, or abnormal shutdown is a FAIL. Close a running `build\TaskTrack.exe` before rebuilding it; Windows executable locking is not a source failure.

## Tests / selftest

`TaskTrackTests.exe` must pass (the accepted deterministic baseline is at least the pre-TT-010 97; a higher all-passing total is expected after TT-010). `TaskTrackMcp.exe --selftest` must report `tasktrack-mcp-selftest: ok`.

## 18 controls

Generate the demo with `TaskTrackExample.exe` and open it with `TaskTrack.exe --task "<path>"`. The document must be schema V2 with exactly 18 questions, one of each canonical type: confirm, single_choice, multi_choice, select, list_select, text, notes, number, amount, range, rating, color, gradient, position, direction, rank_order, hierarchy_select, curve. Every control must render and interact, with structured `answer.data` preserved (boolean, arrays, numbers, `{low,high}`, ordered arrays, node ids, curves).

## Pass/Fail + optional note

- The demo `confirm` presents Pass (green) / Fail (red) with accessible text labels.
- Selecting Pass persists `answer.data = true`; Fail persists `answer.data = false`; `answer.value` is the compact label.
- The optional Note editor persists through autosave and reopen.
- Note survives changing Pass↔Fail and survives accepting a recommendation.
- Note alone does not answer the required question.
- Ordinary Yes/No confirm remains ordinary (covered deterministically in tests).

## Four states

Grey = recommendation available; orange = required/no recommendation; green = human-resolved; red = required still blocking after attempted continuation. No fifth blue workflow state.

## Three assistance actions

`propose_answer` (recommended required), `clarify`/`simplify` (clarification required), and `continue_with_judgement` (no payload). Request lifecycle `pending → answered → (cancelled)`; an answered request is not a resolved human question. Agent replies never create `answer.data`.

## Restart durability

A pending assistance request and its response survive MCP/TaskTrack/client restarts using the stable `task_id`. A separate `.agent.json` sidecar holds assistance; the main task JSON remains canonical human evidence.

## Host acceptance

1. Install/register the published MCP binary into **OpenCode first**; confirm connection; run an **unnamed** first prompt that naturally reaches a human-dependent decision and observe whether the model discovers TaskTrack from its tool descriptions.
2. Exercise a real pending assistance request through `get_task` / `respond_to_request`.
3. Install/register into **Codex second**; repeat discovery and dogfood.
4. Confirm the executable used is `E:\apps\github\upp_tasktrack\build\TaskTrackMcp.exe` as an argument-free stdio server; record the actual installed CLI command names rather than assuming syntax.

## Curt visual handoff

Leave an appropriate TaskTrack window open showing useful states (grey proposal, orange required/no-proposal, green resolved, Pass/Fail + note). Curt judges proportions, font hierarchy, card weight, spacing, and the Pass/Fail colour treatment.

## Report

Return: exact TaskTrack SHA and branch; dependency SHAs; Release and Debug/BLITZ results for all four packages; full TaskTrackTests summary; MCP selftest result; generated demo path; 18/18 control result; Pass/Fail + note result; four-state result; three-action assistance result; restart durability result; OpenCode-first and Codex-second host install + unnamed-discovery results; whether the GUI is ready for Curt; first exact failure + diagnosis; final `git status --short`.
