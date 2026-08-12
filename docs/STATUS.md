# TaskTrack Status

## Recovery log — 2026-08-12

BASE: `27ba66082b0eb078090695ba239c7f986b0f2d1e` on `main`

## Accepted state

- **TT-009 PASS** — source/API/package/diff review + deterministic tests accepted; four-state workflow and durable human→agent assistance implemented on `main`.
- **TT-009-R1 PASS** — unified the assistance protocol: request lifecycle `pending → answered → (cancelled)`; added `continue_with_judgement`; legacy sidecar `resolved` migrates to `answered`; new sidecars never emit request status `resolved`. Published at `149d477`; deterministic baseline 97 passed.
- **TT-010 implemented (source/product/docs)** — Pass/Fail verification fast path over canonical `confirm` (`choices = ["Pass","Fail"]`), boolean `answer.data`, optional verdict `answer.note`, agent-facing MCP/documentation finalized. Deterministic baseline after TT-010: 123 passed, 0 failed; MCP selftest ok.
- **Runtime/platform/host acceptance is still PENDING.** No authoritative Windows Release + Debug/BLITZ, live-GUI, OpenCode, or Codex acceptance has been claimed yet.

## TT-009/TT-009-R1 reference detail

Development remains directly on `main`; no feature branches are used.

TaskTrack uses four local workflow states rather than remapping global Ui roles:

- grey = normal agent proposal / suggested baseline;
- orange = required item with no responsible agent proposal;
- green = human-resolved, manually or by explicit proposal acceptance;
- red = required item still unresolved after attempted continuation/submit.

A durable `<task>.agent.json` sidecar carries human→agent assistance separately from authoritative human-answer JSON. Compact request actions are exact:

- `propose_answer` → `recommended` required;
- `clarify`, `mode=simplify` → `clarification` required; `recommended` optional;
- `continue_with_judgement` → no response payload required (human delegates judgement back to the agent).

Request lifecycle is `pending → answered → (cancelled)`. An answered agent request is not a resolved human question; only explicit human acceptance/manual answering creates `answer.data`. `get_task` exposes `agent_action_required` + `pending_requests`; `respond_to_request` resolves the compact request and validates any recommendation against the referenced semantic item. Agent replies never write `TaskTrackAnswer`.

## TT-010 reference detail

- Ordinary verification is `type=confirm` with `choices=["Pass","Fail"]`: Pass → `answer.data=true`, Fail → `answer.data=false`; `answer.value` is the compact display label; Pass renders green and Fail red with accessible text labels.
- A restrained optional verdict **Note** editor is available on Pass/Fail; `answer.note` persists, round-trips, returns in status/result evidence, does not answer the question by itself, and survives verdict change and recommendation acceptance (the shared `TaskTrackApplyRecommendation()` authority preserves it).
- No new semantic type; `pass_fail` remains loader compatibility only; schema V2 unchanged.

PUBLISHED CHECKPOINTS (TT-009):

- `4a78a171602719816dba4a504717a423cf386fd8` — durable agent request sidecar.
- `38af2e832ef70cf5c4362e88d9633ed4cbb83efa` — compact MCP request/response contract.
- `c55cf1de6d8ff2fcae417b4d5177c40c48bcac93` — four-state question assistance UI.
- `3598a1aedcded91783134157a4f17e6d13c90fba` — keep proposals actionable after red/escalated review.
- `149d4770f66b3915ee5256eb59257d22172ff1da` — TT-009-R1 protocol unification.

CURRENT DEPENDENCY CONTEXT:

- Use current `upp_Ui/main`; no Ui source changes are required.
- Core remains GUI-independent; MCP is a thin semantic bridge; Widgets own TaskTrack-local presentation.

VALIDATION:

- TT-009 source/deterministic review: PASS.
- TT-009-R1 deterministic review: PASS (97 passed; selftest ok).
- TT-010 deterministic review: PASS (123 passed; selftest ok).
- Authoritative Windows Release + Debug/BLITZ, live GUI, OpenCode-first and Codex-second host install + unnamed dogfood, and Curt visual acceptance: **PENDING**.

NEXT:

1. Freeze TT-010 (`docs/PLAN.md` reflects the current phase) and run authoritative Windows Release + Debug/BLITZ validation.
2. Install the published MCP binary into OpenCode first; run unnamed-agent discovery + real TaskTrack dogfood.
3. Install into Codex second; run unnamed-agent discovery + dogfood.
4. Leave the GUI open for Curt visual acceptance of the compact shell, Pass/Fail + note, four states, and assistance controls.
