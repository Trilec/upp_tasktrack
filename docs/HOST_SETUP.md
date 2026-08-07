# TaskTrack MCP Host Setup

Build `TaskTrack/Mcp` so `TaskTrackMcp.exe` sits beside `TaskTrack.exe` in the repository `build` directory. Register `TaskTrackMcp.exe` as an argument-free **stdio** MCP server in Codex, OpenCode, Hermes, or another compatible host.

The host-specific configuration syntax changes independently of TaskTrack, so this repository keeps the integration rule simple: executable path + stdio, no shell wrapper required.

## Expected executable layout

```text
build/
  TaskTrack.exe
  TaskTrackMcp.exe
  TaskTrackTests.exe
  TaskTrackExample.exe
```

## Host behaviour

An agent normally calls `create_task` with `launch:true`. TaskTrack persists the task first and then starts `TaskTrack.exe --task <path>`.

The agent should retain the returned `task_id`; human work can outlive the current MCP process. Later `get_task` calls resolve the durable task from the per-task locator.

See `AGENT_GUIDE.md` for how to choose question types and `MCP.md` for the wire contract.
