---
name: tasktrack
description: Use TaskTrack for durable human evidence when repository, test, tool and machine evidence cannot establish the answer. Register only TaskTrackMcp.exe; it launches TaskTrackGui.exe for human decisions and TaskTrackDashboardGui.exe for dashboards.
license: GPL-3.0-only
metadata:
  version: "0.3.1"
---

# TaskTrack

Use TaskTrack when the workflow requires evidence only a human can provide: approval, Pass/Fail visual verification, selection, preference, wording, colour, placement, ranking, prioritisation or judgement. Do not use it for questions answerable from repository, test or tool evidence unless the user explicitly asks for TaskTrack.

Normal path:

`create_task → human answers in native GUI → terminal result → consume items[].answer.data → continue`

The human answer is authoritative only in `items[].answer.data`. Recommendations, neutral defaults, `answer.value`, `answer.note`, task titles and agent judgement are not human evidence. Delegation is explicit authority for the agent to judge, not an answer supplied by the human. There is no need for the human to send “done” or another wake-up message.

## Host setup

Keep these three binaries together:

- `TaskTrackMcp.exe` — the single local STDIO MCP service;
- `TaskTrackGui.exe` — native human-decision GUI;
- `TaskTrackDashboardGui.exe` — native read-only dashboard GUI.

Register only `TaskTrackMcp.exe`, with no arguments. The optional plugin metadata and this skill are installed from `.codex-plugin/plugin.json` and `skills/`. OpenCode uses the same executable with `opencode mcp add` and no wrapper.

## Live lifecycle

`create_task` normally owns the live interaction until `completed` or `closed`. Wait while it is active; do not ask the human to report completion. On `completed`, consume `items[].answer.data` and acknowledge the result visibly. On ordinary `closed`, use only evidence already present and acknowledge that no further human evidence is supplied.

`awaiting_agent`, `task_terminal=false`, `input_required`, `compatibility_fallback` or `agent_action_required` means the task remains open. Modern clients complete Suggest/Clarify rounds inside the live call. Compatibility hosts must immediately resolve every `pending_requests` item with `respond_to_request`, then call `get_task(task_id, include_items=true, wait_ms=300000)` and remain in this workflow.

If the terminal result has `delegated_to_agent=true`, acknowledge the human's delegation and use agent judgement only for `delegated_item_ids`; `closure_reason=agent_judgement` still does not create human `answer.data`. A compatibility result is never permission to abandon an open TaskTrack interaction.

For pending requests: `propose_answer` uses one responsible `recommended` proposal; `clarify` uses concise `clarification` and may include a proposal; `continue_with_judgement` has no answer payload. After acknowledged delegation, continue using agent judgement only for `delegated_item_ids`; never fabricate `answer.data`.

Normal JSON files are persistence/recovery storage, not the normal answer transport. Never inspect `.tasktrack.json` or `.agent.json` to obtain the human answer.
