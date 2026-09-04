# TaskTrack Secure MCP Tunnel smoke test

This is an experimental `0.3.2-rc1` integration. TaskTrack does not implement
OpenAI's tunnel wire protocol itself. `TaskTrackTunnelGui.exe` supervises the
official runtime-only Windows artifact downloaded from OpenAI Platform, which
forwards the existing local stdio `TaskTrackMcp.exe`.

## Local runtime

Keep these files together in a writable runtime/build directory:

- `TaskTrackMcp.exe`
- `TaskTrackGui.exe`
- `TaskTrackDashboardGui.exe`
- `TaskTrackTunnelGui.exe`
- official OpenAI tunnel runtime executable, named locally `tunnel-client.exe`

The Platform download can be the narrow `tunnel-client-runtime` artifact.
That binary intentionally exposes only `run`, `--help`, and `--version`.
TaskTrack therefore starts `run` directly and does not depend on the full
client's `runtimes ...` management commands.

Create the tunnel in OpenAI Platform with the ChatGPT workspace that will use
the connector. Record the returned `tunnel_...` id.

Set a Platform API key that is authorized to use that tunnel in the environment
that launches TaskTrack Tunnel:

```powershell
$env:CONTROL_PLANE_API_KEY="sk-..."
.\TaskTrackTunnelGui.exe --tunnel-id tunnel_...
```

The runtime itself also accepts `OPENAI_API_KEY` when
`CONTROL_PLANE_API_KEY` is absent, but TaskTrack uses the explicit
`CONTROL_PLANE_API_KEY` contract so the active credential source is obvious.
The key is inherited by the official runtime; TaskTrack does not persist or
display it.

`Connect` launches the official runtime approximately as:

```text
tunnel-client-runtime run
  --control-plane.tunnel-id <id>
  --control-plane.api-key env:CONTROL_PLANE_API_KEY
  --mcp.command <co-located TaskTrackMcp.exe>
  --health.listen-addr 127.0.0.1:0
  --health.url-file <local health-url file>
```

The runtime remains active while `TaskTrackTunnelGui.exe` is open.
`Status` reads the runtime's `/healthz` and `/readyz` endpoints directly.
`Stop` terminates the locally supervised runtime. `Open health` opens
`/readyz` in the browser. The narrow runtime artifact has no full admin UI.

## Browser connector

In ChatGPT plugin settings choose Connection = Tunnel, select the same tunnel,
and choose No Auth for this local stdio MCP path. Keep the local runtime
running. The logical tunnel can appear in ChatGPT before the local runtime is
ready; plugin creation/tool discovery should be tested only after Status shows
`ready=true`.

For the first acceptance use read operations:

1. `version` -> expect `0.3.2-rc1`, task schema 2, dashboard schema 1.
2. `tunnel_probe` -> confirms the browser reached this local TaskTrack runtime.
3. `list_dashboards` / `get_dashboard` -> confirms normal dashboard reads.

`Send probe` in `TaskTrackTunnelGui.exe` increments a small local diagnostic
record. A later `tunnel_probe` call must return the new sequence/message. The
probe does not contain the computer name and is not task, dashboard, repository
or human evidence.

## Boundary

The Secure MCP Tunnel is remote ingress to the existing MCP endpoint. It does
not replace local application IPC. A future TaskTrack/local application bus can
sit behind the MCP:

```text
ChatGPT -> Secure MCP Tunnel -> TaskTrackMcp -> local U++ application bus
```

Cloudflare, remote HTTP MCP, OAuth and a native U++ tunnel implementation are
out of scope for this smoke test.
