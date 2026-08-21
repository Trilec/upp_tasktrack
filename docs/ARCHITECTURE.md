# TaskTrack Architecture

## Principle

TaskTrack models a human as a structured decision/observation source, not as a generic chat box. The caller sends a compact inspection/decision protocol; the human returns typed evidence.

Task completion and agent assistance are deliberately separate state machines. See `INTERACTION_LIFECYCLE.md` for the normative lifecycle.

## Package boundary

### `TaskTrack/Core`

GUI- and MCP-independent authority for:

- semantic task/question model;
- V1→V2 compatibility;
- validation;
- JSON serialization;
- atomic persistence and `.bak` recovery;
- stable task-id lookup;
- bounded terminal history;
- Markdown export;
- polling reminder marker;
- durable human→agent assistance sidecar.

The main task JSON remains the human-evidence authority. The separate `.agent.json` channel stores advisory assistance traffic and a non-terminal interaction phase (`awaiting_human` / `awaiting_agent`).

### `TaskTrack/Widgets`

Semantic renderer over the existing U++ `Ui` package.

`TaskTrackQuestionCtrl` is a restrained `UiGroupPanel`: question title in the panel title, concrete instruction in the subtitle, and exactly one response composition in its content area.

The renderer owns presentation heuristics. Agents never name U++ controls.

Existing Ui controls cover most semantics. Four small TaskTrack-specific controls cover genuinely spatial/visual interactions not represented directly by one Ui control:

- 3×3 position selector;
- 8-way direction selector;
- dual-thumb range selector;
- gradient option selector.

### `TaskTrack/App`

Owns only application composition and lifecycle:

- compact TaskTrack/project header;
- objective + progress;
- conditional wrapped category strip;
- responsive wrapped question-card area;
- small Save / Submit footer;
- pause/resume;
- debounced autosave;
- reminder prompt;
- export actions;
- agent-launched foreground/close behaviour;
- semantic-model-based dialog sizing.

Agent-launched windows estimate useful size from item count, semantic type, choice count and naturally taller editors. They grow only as much as the requested interaction warrants, then prefer scrolling over an oversized workspace. The result is clamped to the primary desktop work area.

### `TaskTrack/Mcp`

Thin stdio transport. It advertises the 18 semantic types and returns durable task ids/results. It contains no GUI logic and does not reinterpret agent recommendations as human evidence.

The MCP process and GUI remain separate executables. Process separation is not the interaction boundary: the durable task/assistance model is the shared authority between them.

## Live interaction lifecycle

Normal launched `create_task` owns the human interaction until the main task reaches a terminal state:

1. validate request;
2. generate/validate stable task id;
3. persist the complete task;
4. register its independent locator;
5. launch the GUI;
6. wait for human completion/close;
7. return structured human evidence directly when the task terminates.

`launch=false` is the explicit detached/recovery path.

Only `completed` and `closed` are task-terminal.

Human assistance is non-terminal:

```text
awaiting_human -> awaiting_agent -> awaiting_human
```

Suggest/Clarify/Use judgement create pending sidecar requests and move the effective interaction to `awaiting_agent`. Agent response moves it back to `awaiting_human`; the GUI remains open throughout.

For MRTR/sampling-capable hosts this may happen inside the same modern MCP interaction. For hosts without sampling, TaskTrack returns a compatibility continuation so the model can fulfil the request, but the structured status explicitly says `task_terminal=false`. The agent must resolve the request and resume waiting for the human; that checkpoint is not task completion.

## Durability

`.tasktrack.json` and `.agent.json` make the interaction restartable and auditable, but they are not the normal answer transport. Normal delivery is GUI → MCP lifecycle → agent.

## Pause/reminders

Pause is explicit durable state. It never implies expiry.

Periodic reminder and agent-poll reminder are UI prompts only. Neither is permitted to complete or close a task automatically. `Close task` is an explicit user choice.

## Compactness

TaskTrack avoids one generic note editor on every card. Free-form text is a semantic `notes` question when it is actually useful. This preserves density and teaches agents to ask typed questions rather than falling back to prose.
