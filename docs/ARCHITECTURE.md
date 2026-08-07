# TaskTrack Architecture

## Dependency direction

```text
TaskTrack/App  ----> TaskTrack/Core <---- TaskTrack/Mcp
      |                                      |
      v                                      v
     Ui                                    Core
      |
      v
  Animation

examples/TaskTrackExample ----> TaskTrack/Core
tests/TaskTrackTests -------> TaskTrack/Core
```

`TaskTrack/Core` has no GUI or MCP dependency. The GUI never owns task truth, and the MCP bridge never owns task truth. Both operate on the same durable document model.

## Core model

`TaskTrackDocument` contains:

- stable `task_id`;
- project/objective metadata;
- lifecycle state;
- reminder settings;
- bounded-history preference;
- ordered verification items.

Each `TaskTrackItem` contains:

- stable item ID;
- category;
- field type;
- title/instruction;
- required flag;
- optional choices;
- optional expected colour/value;
- one structured human answer.

## Persistence authority

The `.tasktrack.json` file is authoritative. It is not a cache of GUI state.

Save sequence:

1. serialize current document;
2. parse/validate the serialized form;
3. write `<task>.tmp`;
4. read back and byte-verify the temporary file;
5. copy the previous primary to `<task>.bak` when present;
6. replace the primary;
7. restore the backup if installation of the new primary fails.

Load sequence tries the primary first and then the backup. Recovery is surfaced to the caller instead of silently pretending the primary was healthy.

## Per-task locator

Custom storage roots are supported. To let a later agent resolve only a `task_id`, TaskTrack writes a small locator file per task under the default registry folder.

The registry is intentionally not one shared JSON index. Independent locator files avoid unrelated tasks contending on a single write target when several agents create work at once.

## Human activity

Human activity timestamps are persisted in the task. Text entry updates the in-memory answer immediately and schedules a short autosave. Explicit actions save immediately.

Agent polling uses a separate `.poll` marker. Polling therefore cannot mutate the evidence document merely by asking for status.

## Reminder model

Reminder settings are task-local:

- `reminder_minutes`: 0 disables timer reminders;
- `remind_while_paused`: allows reminders even while intentionally paused;
- `nudge_on_agent_poll`: lets a fresh agent poll prompt a reminder after inactivity.

A reminder presents three human decisions:

- continue/resume;
- pause/keep paused;
- close task.

There is no automatic close path.

## GUI shell

The application uses Ui controls/layouts only for its main interface:

- `UiTitleCard`
- `UiPanel`
- `UiBoxLayout`
- `UiScrollPanel`
- `UiButton`
- `UiSplitButton`
- `UiCheckBox`
- `UiDropdown`
- `UiLineEdit`
- `UiMultiEdit`
- `UiCompositeColor`
- `UiLabel`

The category and check areas use wrapped `UiBoxLayout` flows. This gives a compact two-or-more-column desktop view when space permits and degrades naturally toward fewer columns as the window narrows.

## Terminal state

A `completed` or `closed` task is read-only in the GUI. Export remains available because persisted human evidence is still useful after the workflow gate has been crossed.

## MCP boundary

MCP does not keep a human wait in-flight. `create_task` first persists the task and only then returns a stable identity. This ordering is essential: once the agent sees a task ID, that task must already survive process/network failure.

Modern clients declaring the Tasks extension can use MCP's formal task lifecycle. Legacy clients use the same core through normal TaskTrack tools.
