# TaskTrack Secure MCP Tunnel

TaskTrack `0.3.2-rc1` uses the official OpenAI Secure MCP Tunnel runtime.
TaskTrack does not implement or fork the tunnel wire protocol. The native
`TaskTrackTunnelGui.exe` manager supervises the official runtime-only Windows
artifact and forwards the existing local stdio `TaskTrackMcp.exe`.

## Runtime files

Keep these together in a writable runtime/build directory:

- `TaskTrackMcp.exe`
- `TaskTrackGui.exe`
- `TaskTrackDashboardGui.exe`
- `TaskTrackTunnelGui.exe`
- official OpenAI tunnel runtime executable, named locally `tunnel-client.exe`

The Platform download may be the narrow `tunnel-client-runtime` artifact.
That binary exposes `run`, `--help` and `--version`. TaskTrack starts
`run` directly; it does not require the full client's management command tree.

The runtime API key remains outside TaskTrack:

```powershell
$env:CONTROL_PLANE_API_KEY="sk-..."
.\TaskTrackTunnelGui.exe
```

TaskTrack never persists or displays the secret value.

## Tunnel Manager

The native manager is intentionally a small desktop application rather than a
diagnostic text window.

### Overview

Overview answers five questions immediately:

1. Is remote TaskTrack available?
2. Which named profile/tunnel is active?
3. Is the TaskTrack MCP executable present?
4. Is remote traffic flowing?
5. What were the most recent MCP request/result pairs?

The Ready state is green. Connecting is amber. Faults are red. Stopped is
neutral. The Recent activity table retains the last six remote communications
with direction, MCP method/tool and result size/status.

Only MCP processes launched by the tunnel runtime increment the remote activity
log. Ordinary local Codex/TaskTrack MCP traffic is excluded.

### Setup

Setup manages named non-secret tunnel profiles. A profile contains:

- friendly profile name;
- tunnel ID;
- tunnel runtime executable path;
- TaskTrack MCP executable path;
- auto-connect preference;
- remember-profile preference.

The credential source is fixed to `CONTROL_PLANE_API_KEY` and the UI shows only
whether it is available.

Use one logical tunnel/profile per local machine. Two local machines must not
compete for the same stdio tunnel queue. A typical two-machine setup is:

```text
Curt PC      -> TaskTrack profile/tunnel A -> ChatGPT plugin A
Colleague PC -> TaskTrack profile/tunnel B -> ChatGPT plugin B
```

Duplicate profile intentionally copies local runtime settings but leaves the
new tunnel ID blank so a second deployment cannot accidentally reuse the same
logical tunnel.

## Browser connector

In ChatGPT plugin settings choose:

- Connection = Tunnel
- the same tunnel ID as the local profile
- Authentication = No Auth for the local stdio MCP path

The logical tunnel may appear in ChatGPT before a machine-side runtime is
ready. Create/test the connector only after the Overview reports Ready.

Initial read acceptance:

1. `version` -> current TaskTrack build/schema identity;
2. `tunnel_probe` -> proves the browser reached this exact local runtime;
3. `list_dashboards` / `get_dashboard` -> normal dashboard reads.

`Send probe` writes only a small local diagnostic record. It is not task,
dashboard, repository or human evidence.

## Boundary

The Secure MCP Tunnel is remote ingress to the existing MCP endpoint. It does
not replace a future local U++ application bus:

```text
ChatGPT -> Secure MCP Tunnel -> TaskTrackMcp -> local U++ application bus
```

Cloudflare, remote HTTP MCP, OAuth and a native U++ tunnel implementation remain
out of scope for this release.
