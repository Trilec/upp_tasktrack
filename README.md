# TaskTrack

TaskTrack is a compact U++ human-in-the-loop verification tool for AI-assisted development.

An agent can create a durable verification task when automated tests cannot prove a visual or interactive fact. TaskTrack presents the task as a focused desktop checklist, saves human answers as they are made, supports pause/resume and inactivity reminders, and exposes the durable result back through MCP.

Typical checks include:

- whether a GUI is actually visible and usable;
- pass/fail interaction tests such as drag, resize, grouping, undo, or save/reload;
- visual comparisons and colour confirmation;
- file/export/output confirmation;
- free-text or numeric observations;
- multiple-choice observations where the agent needs one concrete answer.

TaskTrack does not edit source code and is not part of PatchTrack. It is a separate verification product that can be used alongside any agent or development workflow.

## Repository layout

- `TaskTrack/Core` — durable task model, validation, JSON, recovery, history, export.
- `TaskTrack/App` — U++ GUI built with the `Ui` control package.
- `TaskTrack/Mcp` — stdio MCP bridge.
- `examples` — sample task generation and MCP request data.
- `tests` — deterministic core/protocol tests.
- `docs` — architecture, schema, MCP integration, and acceptance notes.
- `build` — local output directory; ignored by Git.

The GUI intentionally follows the compact shell patterns already proven in the U++ Ui applications: restrained headings, layered panels, wrapping category controls, and a responsive task area. The application uses data-driven check cards rather than building a different screen for every request.

## V0.1 check types

| Type | Human response |
| --- | --- |
| `check` | confirmed/not confirmed |
| `pass_fail` | Pass / Fail / Blocked / N/A |
| `choice` | agent-supplied choice |
| `text` | short observation |
| `multiline` | longer notes/evidence |
| `number` | numeric observation |
| `color` | Match / Different / Unsure, with expected swatch |
| `file` | Found / Missing / Wrong output / Unsure |
| `interaction` | Pass / Fail / Partial / Blocked |
| `visual_compare` | Match / Different / Unsure |

Every check may be required or optional and may carry an instruction, expected value, and note.

## Durable lifecycle

A newly created task begins as `awaiting_human`. Opening it moves it to `in_progress`. The operator can pause it indefinitely. Pause is explicit state, not a timeout. A task can finish as `completed`, or be deliberately closed without completion as `closed`.

Task files are written through a verified temporary file and retain a `.bak` recovery copy. The MCP bridge works through stable `task_id` values rather than keeping a tool call open while a person works. This makes a one-minute review and an overnight review the same application-level workflow.

Optional reminders can ask the operator whether they are still working. TaskTrack never auto-closes a task. Optional agent-poll nudging lets a status poll become a gentle reminder signal without changing the task result.

## MCP

The stdio server supports the current `2026-07-28` stateless request shape, including `server/discover`, per-request protocol metadata, cacheable list results, and the `io.modelcontextprotocol/tasks` extension when a modern client declares it. It also retains the older `initialize` / `notifications/initialized` flow for Codex, OpenCode, Hermes, and other hosts that have not migrated yet.

For clients without the Tasks extension, `create_task` returns a normal durable TaskTrack result immediately. For modern clients that declare the extension, the same tool can return a proper task handle and be polled through `tasks/get`. TaskTrack never relies on a long-lived MCP connection for the human wait.

## Quick start

See [GETTING_STARTED.md](GETTING_STARTED.md).

The implementation plan is in [docs/PLAN.md](docs/PLAN.md). The persisted format is in [docs/SCHEMA.md](docs/SCHEMA.md). The MCP contract is in [docs/MCP.md](docs/MCP.md), with host registration examples in [docs/HOST_SETUP.md](docs/HOST_SETUP.md).
