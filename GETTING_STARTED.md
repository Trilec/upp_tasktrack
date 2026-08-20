# TaskTrack — Getting Started

## Windows/U++ baseline

The checked-in assembly examples assume:

```text
repository:        E:\apps\github\upp_tasktrack
U++:               E:\upp-18468
Ui:                E:\apps\github\upp_Ui
Animation:         E:\apps\github\upp_animation
StateMachine:      E:\apps\github\upp_statemachine
```

`GitHubOut.var` / `example.var` contain the corresponding assembly/output paths.

## Build and deterministic verification

```powershell
cd E:\apps\github\upp_tasktrack
powershell -ExecutionPolicy Bypass -File .\verify.ps1 -UppRoot E:\upp-18468
```

This builds directly into `build`:

- `TaskTrackGui.exe` — native human GUI
- `TaskTrackMcp.exe` — stdio MCP server
- `TaskTrackTests.exe`
- `TaskTrackExample.exe`

and runs `TaskTrackTests.exe` plus `TaskTrackMcp.exe --selftest`.

Do not run the wrapper while `TaskTrackGui.exe` from the same build path is still open on Windows; the running executable may prevent `ld.lld` from replacing it.

## CLI

```text
TaskTrackGui.exe                choose an existing task
TaskTrackGui.exe --task <path>  open a specific task
TaskTrackGui.exe --help         show GUI usage
TaskTrackGui.exe --version      show version

TaskTrackMcp.exe                run the stdio MCP server
TaskTrackMcp.exe --help         show MCP usage
TaskTrackMcp.exe --version      show version/schema/protocol
TaskTrackMcp.exe --selftest     run deterministic MCP self-test
TaskTrackMcp.exe --oneshot <request.json>   process one MCP JSON-RPC request file and exit
                                         (diagnostic/test utility; not needed for host registration)
```

Both executables should normally be placed in the same directory. `TaskTrackMcp.exe` launches `TaskTrackGui.exe --task <path>` beside itself.

## Generate the 18-type demo

```powershell
.\build\TaskTrackExample.exe
```

The example prints a durable `.tasktrack.json` path. Open it with:

```powershell
.\build\TaskTrackGui.exe --task "<printed path>"
```

The task contains exactly one canonical V0.2 question of each type and is the preferred first visual/interaction acceptance surface.

## MCP live workflow

`build\TaskTrackMcp.exe` is an argument-free local stdio MCP server.

Normal launched work is live:

1. agent calls `create_task`;
2. TaskTrack persists the task and launches the GUI;
3. the initiating interaction stays owned while the person answers;
4. final human evidence returns through MCP;
5. the agent continues without requiring the person to send another chat message.

`launch=false` is the explicit detached/recovery mode.

### Human → agent assistance

If the person presses Suggest/Clarify while a host exposes the required modern MCP capability, TaskTrack uses the in-call multi-round-trip path.

Some hosts do not expose that capability. TaskTrack then returns a structured compatibility result with pending assistance requests. The agent must immediately:

1. resolve every pending request with `respond_to_request`;
2. call `get_task(task_id, include_items=true, wait_ms=300000)`;
3. remain in that loop until the human completes/closes the task or asks for more assistance.

The person should never need to type “done”, “I submitted”, “check TaskTrack”, or similar merely to wake the agent.

Task JSON and `.agent.json` are durability/recovery state only, not the routine answer transport.

Read `docs/AGENT_GUIDE.md` before authoring real requests.

## Host registration

Register only the MCP executable:

```text
E:\apps\github\upp_tasktrack\build\TaskTrackMcp.exe
```

`TaskTrackGui.exe` must sit beside it. No wrapper or GUI registration is required.

After rebuilding the MCP binary, fully restart the host so it loads the fresh executable and fresh tool descriptions.

## Codex workflow plugin (recommended)

TaskTrack also ships a small workflow plugin/skill under:

```text
.codex-plugin/plugin.json
skills/tasktrack/SKILL.md
```

The skill does not replace the MCP server. It teaches Codex the TaskTrack invocation boundary and, importantly, tells it to drive the compatibility fallback automatically when the host does not expose MCP sampling/multi-round-trip support.

Using the current Codex Git-marketplace pattern, installation is expected to be equivalent to:

```text
codex plugin marketplace add Trilec/upp_tasktrack
codex plugin add tasktrack@tasktrack
```

If the installed Codex version exposes different plugin commands, inspect `codex plugin --help` and use the equivalent marketplace/local-install commands rather than guessing.

Restart Codex after installing/updating the plugin and use a fresh session for acceptance.

## OpenCode

The MCP server remains independently usable without the Codex plugin. Before documenting exact OpenCode commands, inspect the installed version and `opencode mcp --help`; register `TaskTrackMcp.exe` as the local stdio server with no arguments and validate the same human lifecycle.
