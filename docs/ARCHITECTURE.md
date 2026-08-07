# TaskTrack Architecture

## Principle

TaskTrack models a human as a structured decision/observation source, not as a generic chat box. The caller sends a compact inspection/decision protocol; the human returns typed evidence.

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
- polling reminder marker.

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
- export actions.

The question grid is `UiBoxLayout` horizontal flow with wrapping and fixed-column sizing. Wide windows naturally show more columns; narrower windows fall to two/one without an alternate UI implementation.

### `TaskTrack/Mcp`

Thin stdio transport. It advertises the 18 semantic types and returns durable task ids/results. It contains no GUI logic and does not interpret human evidence beyond transport/status.

## Long-running lifecycle

TaskTrack intentionally does not keep an MCP tool call waiting for a person.

1. validate request;
2. generate/validate stable task id;
3. persist the complete task;
4. register its independent locator;
5. optionally launch the GUI;
6. return task id/handle;
7. later retrieval resolves the durable task from disk.

The human can therefore take seconds or days without coupling work lifetime to an MCP process or host timeout.

## Pause/reminders

Pause is explicit durable state. It never implies expiry.

Periodic reminder and agent-poll reminder are UI prompts only. Neither is permitted to complete or close a task automatically. `Close task` is an explicit user choice.

## Compactness

TaskTrack avoids one generic note editor on every card. Free-form text is a semantic `notes` question when it is actually useful. This preserves density and teaches agents to ask typed questions rather than falling back to prose.
