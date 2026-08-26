# TaskTrack architecture

TaskTrack treats a human as a structured decision source rather than a second chat channel. The caller describes the meaning of the decision; TaskTrack chooses the native control, persists the interaction, and returns typed evidence.

## Packages

### `TaskTrack/Core`

Core is GUI- and MCP-independent. It owns:

- the task and answer model;
- the 18 semantic question types;
- validation and schema migration;
- JSON serialization;
- verified atomic save and `.bak` recovery;
- stable task-id lookup;
- result/export helpers;
- the durable agent-assistance sidecar.

The main `.tasktrack.json` document is the authority for human answers. `<task>.agent.json` stores assistance and delegation traffic separately so an agent response cannot accidentally become human evidence.

### `TaskTrack/Widgets`

Widgets maps semantic question types onto U++ `Ui` controls. The MCP contract never names GUI classes.

Most questions use existing controls from `upp_Ui`. A few compact TaskTrack-specific controls cover interactions such as 3×3 position, 8-way direction, dual-ended range selection and gradient choice.

### `TaskTrack/App`

The native GUI owns presentation and human interaction:

- header and objective context;
- optional category strip;
- responsive question cards;
- autosave, pause and reminders;
- Submit / Cancel;
- Suggest, Clarify and Use judgement presentation;
- agent-launched foreground and close behaviour.

#### Measured dialog sizing

Agent-launched windows do not keep a table of guessed heights for different question types.

Questions are assembled first. `TaskTrackQuestionFlow` then chooses the card packing, lays each real card out at its assigned width, and measures its actual `UiGroupPanel` body and content. The same flow rules are used for preferred size and live layout. Header, footer, categories and scroll-panel gutter are measured as well.

While the result fits the desktop, the window uses the measured size. If it does not fit, the task viewport is shortened and the scroll panel owns the overflow.

This is the same algorithm whether there is one question or many.

### `TaskTrack/Mcp`

The MCP package is a thin local stdio server over Core. It:

- advertises the semantic question schema;
- creates and locates durable tasks;
- launches `TaskTrackGui.exe` beside itself;
- owns the normal live `create_task` interaction;
- returns structured terminal results;
- exposes recovery and assistance tools.

The MCP process contains no GUI rendering logic.

## Two executables

TaskTrack intentionally ships as a pair:

```text
TaskTrackMcp.exe   agent-facing stdio process
TaskTrackGui.exe   native human interface
```

Keeping them separate prevents GUI concerns from contaminating the stdio protocol, while the durable Core model provides the shared authority between processes.

## Task state and assistance state

These are separate concepts.

Main task states include `awaiting_human`, `in_progress`, `paused`, `completed` and `closed`. Only `completed` and `closed` are terminal.

Assistance has a smaller live phase:

```text
awaiting_human -> awaiting_agent -> awaiting_human
```

Suggest and Clarify return advice to the open human task. Use judgement is different: it records explicit human delegation. Once all remaining required items are delegated and the agent acknowledges them, the main task may close without creating human answers for those items.

See `INTERACTION_LIFECYCLE.md` for the full state flow.

## Persistence and recovery

A save is validated before installation, written through a temporary file, verified, and preserves the previous primary as `.bak`. Each task has an independent locator rather than one shared registry document.

Durable files make a task restartable and auditable, but the normal runtime path remains:

```text
GUI -> durable Core state -> MCP -> agent
```

Agents should not scrape JSON as the routine answer path.

## Important invariants

1. `items[].answer.data` is human answer authority.
2. Recommendations and agent assistance are not human answers until the human explicitly accepts a recommendation.
3. Use judgement records delegation, not an answer.
4. A pending assistance request is non-terminal.
5. Terminal task state wins over late assistance responses.
6. The human should never need to send an extra chat message merely to wake the agent after completing TaskTrack.
7. Layout is derived from assembled controls, not semantic size guesses.
