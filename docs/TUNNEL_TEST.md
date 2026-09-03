# TaskTrack Secure MCP Tunnel smoke test

This is an experimental `0.3.2-rc1` integration. TaskTrack does not implement
OpenAI's tunnel wire protocol itself. `TaskTrackTunnelGui.exe` supervises the
official `tunnel-client.exe`, which forwards the existing local stdio
`TaskTrackMcp.exe`.

## Local runtime

Keep these files together in a writable runtime/build directory:

- `TaskTrackMcp.exe`
- `TaskTrackGui.exe`
- `TaskTrackDashboardGui.exe`
- `TaskTrackTunnelGui.exe`
- official `tunnel-client.exe`

Create the tunnel in OpenAI Platform with the ChatGPT workspace that will use
the connector. Record the returned `tunnel_...` id.

Create a restricted Platform runtime API key with only Tunnels Read + Use and
set it in the environment that launches TaskTrack Tunnel:

```powershell
$env:CONTROL_PLANE_API_KEY="sk-..."
.\TaskTrackTunnelGui.exe --tunnel-id tunnel_...
```

The key is inherited by `tunnel-client`; TaskTrack does not persist or display
it.

`Connect` uses the official managed-runtime path:

```text
tunnel-client runtimes connect
  --alias tasktrack-browser
  --tunnel-id <id>
  --runtime-api-key env:CONTROL_PLANE_API_KEY
  --mcp-command <co-located TaskTrackMcp.exe>
```

`Status` calls `tunnel-client runtimes status ... --json`; `Stop` stops that
managed runtime. `Open tunnel UI` opens the health URL reported by the client
when available.

## Browser connector

In ChatGPT connector/plugin settings choose Connection = Tunnel and select the
same tunnel id. Keep the local runtime running.

For the first acceptance enable/use only read operations. Call:

1. `version` -> expect `0.3.2-rc1`, task schema 2, dashboard schema 1.
2. `tunnel_probe` -> confirms the browser reached this local TaskTrack runtime.
3. `list_dashboards` / `get_dashboard` -> confirms normal dashboard reads.

`Send probe` in `TaskTrackTunnelGui.exe` increments a small local diagnostic
record. A later `tunnel_probe` call must return the new sequence/message. The
probe is read-only from MCP and is not task, dashboard, repository or human
evidence.

## Boundary

The Secure MCP Tunnel is remote ingress to the existing MCP endpoint. It does
not replace local application IPC. A future TaskTrack/local application bus can
sit behind the MCP:

```text
ChatGPT -> Secure MCP Tunnel -> TaskTrackMcp -> local U++ application bus
```

Cloudflare, remote HTTP MCP, OAuth and a native U++ tunnel implementation are
out of scope for this smoke test.
