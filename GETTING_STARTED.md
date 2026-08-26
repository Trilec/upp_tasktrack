# Getting started with TaskTrack

## 1. Requirements

TaskTrack is a U++ project. You need:

- U++ / TheIDE with `umk.exe`;
- `upp_tasktrack`;
- `upp_Ui`;
- `upp_animation`.

The easiest layout is to keep the three repositories beside each other:

```text
C:\dev\upp_tasktrack
C:\dev\upp_Ui
C:\dev\upp_animation
```

TaskTrack does not require `upp_statemachine`.

## 2. Build and verify

From the TaskTrack repository:

```powershell
powershell -ExecutionPolicy Bypass -File .\verify.ps1 -UppRoot C:\upp
```

If `upp_Ui` and `upp_animation` are not siblings, pass them explicitly:

```powershell
powershell -ExecutionPolicy Bypass -File .\verify.ps1 `
  -UppRoot C:\upp `
  -UiRoot C:\src\upp_Ui `
  -AnimationRoot C:\src\upp_animation
```

`-UppRoot` can be omitted when `UPP_ROOT` is set or `umk.exe` is already on `PATH`.

The wrapper produces:

```text
build\TaskTrackGui.exe
build\TaskTrackMcp.exe
build\TaskTrackTests.exe
build\TaskTrackExample.exe
```

It then runs `TaskTrackTests.exe` and `TaskTrackMcp.exe --selftest`.

On Windows, close a running `TaskTrackGui.exe` or `TaskTrackMcp.exe` before rebuilding the same output path; Windows may otherwise keep the executable locked.

### TheIDE

Create an assembly containing:

```text
<tasktrack repo>
<upp_Ui repo>
<upp_animation repo>
<U++ root>\uppsrc
```

Build `TaskTrack/App` for the GUI and `TaskTrack/Mcp` for the MCP server. `examples/TaskTrackExample` and `tests/TaskTrackTests` are useful first validation packages.

## 3. Command line

```text
TaskTrackGui.exe                choose an existing task
TaskTrackGui.exe --task <path>  open a specific task
TaskTrackGui.exe --help         show GUI usage
TaskTrackGui.exe --version      show version information

TaskTrackMcp.exe                run the stdio MCP server
TaskTrackMcp.exe --help         show MCP usage
TaskTrackMcp.exe --version      show version/schema/protocol
TaskTrackMcp.exe --selftest     run the MCP self-test
TaskTrackMcp.exe --oneshot <request.json>
                                process one request file and exit (diagnostic)
```

`TaskTrackGui.exe` and `TaskTrackMcp.exe` should normally live in the same directory.

## 4. Register the MCP server

Register only:

```text
C:\path\to\TaskTrackMcp.exe
```

as a **local STDIO** MCP server with no arguments. The server finds the GUI beside itself and launches it with `--task <path>` when human input is required.

After replacing the MCP executable, restart the host before testing so the old process is not still connected.

### Codex

Add a custom local MCP named `tasktrack` using STDIO and the full path to `TaskTrackMcp.exe`. No arguments or wrapper command are required.

The optional Agent Skill lives at `skills/tasktrack/SKILL.md`. It is workflow guidance, not transport: MCP remains the actual connection.

### OpenCode

Current OpenCode releases support local stdio MCP servers in `opencode.jsonc`:

```jsonc
{
  "$schema": "https://opencode.ai/config.json",
  "mcp": {
    "servers": {
      "tasktrack": {
        "type": "local",
        "command": ["C:\\path\\to\\TaskTrackMcp.exe"]
      }
    }
  }
}
```

This is enough for TaskTrack itself. OpenCode does not need the Codex plugin metadata.

## 5. First smoke test

Generate the example task:

```powershell
.\build\TaskTrackExample.exe
```

It prints the path to a `.tasktrack.json` file containing one example of each canonical question type. Open it with:

```powershell
.\build\TaskTrackGui.exe --task "<printed path>"
```

For an MCP smoke test, first run:

```powershell
.\build\TaskTrackMcp.exe --version
.\build\TaskTrackMcp.exe --selftest
```

Then start a fresh host session and ask it to create one simple TaskTrack decision.

## 6. Normal live behaviour

With `launch=true` (the default):

1. `create_task` validates and persists the complete task;
2. the GUI opens;
3. the initiating tool interaction remains owned while the person works;
4. completion, cancellation or delegation returns a terminal structured result;
5. the agent continues without asking the person to send another message.

Task JSON is durability and recovery storage. It is not the normal answer transport.

### Suggest and Clarify

These are advisory. If the host can service the request inside the live MCP call, TaskTrack uses that path. Otherwise it exposes a compatibility request that the agent resolves with `respond_to_request` before continuing to wait on `get_task`.

### Use judgement

This is explicit delegation. The agent acknowledges the delegation; if no other required human input remains, TaskTrack closes automatically. The terminal result reports `delegated_to_agent=true`, while human `answer.data` remains empty for the delegated item.

## Troubleshooting

**The host says the MCP transport closed after a rebuild**  
Restart the host and open a fresh session. An existing session may still own the old stdio process.

**The linker cannot replace TaskTrackGui.exe or TaskTrackMcp.exe**  
Close the running executable and rebuild.

**The GUI is missing**  
Keep `TaskTrackGui.exe` beside `TaskTrackMcp.exe`.

**The agent stops after Suggest / Clarify / Use judgement**  
Install/use the TaskTrack skill where supported, or make sure the host follows the compatibility continuation documented in `docs/MCP.md`.
