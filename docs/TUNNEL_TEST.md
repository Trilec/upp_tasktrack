# TaskTrack Machine MCP Tunnel

TaskTrack `0.3.2-rc1` uses the official OpenAI Secure MCP Tunnel runtime.
TaskTrack does not implement or fork the tunnel wire protocol.

The native `TaskTrackTunnelGui.exe` application now manages a **machine tunnel
profile** rather than a TaskTrack-only tunnel. One running profile owns one
OpenAI tunnel runtime and may expose multiple separate local STDIO MCP services
through named logical channels.

See [Machine tunnel architecture](TUNNEL_ARCHITECTURE.md).

## Runtime files

For the current TaskTrack service keep these available:

- `TaskTrackMcp.exe`
- `TaskTrackGui.exe`
- `TaskTrackDashboardGui.exe`
- `TaskTrackTunnelGui.exe`
- official OpenAI tunnel runtime executable, normally named
  `tunnel-client.exe`

Additional services may point at MCP executables elsewhere on the same machine.

The supported narrow OpenAI runtime artifact exposes `run`, `--help` and
`--version`. The manager launches `run` directly and does not depend on the
full client's `runtimes ...` command tree.

## Machine profile

A profile contains:

- profile name;
- explicit `machine_id`;
- tunnel ID;
- tunnel runtime executable path;
- credential source/reference;
- auto-connect;
- remember-profile;
- `services[]`.

Each service contains:

- service ID;
- display name;
- logical channel;
- MCP command;
- enabled state.

Exactly one enabled service must use channel `main`. Enabled IDs and channels
must be unique. The OpenAI-reserved `harpoon` channel cannot be used for a
customer service.

A new profile starts with:

```text
TaskTrack
    channel = main
    command = TaskTrackMcp.exe
    enabled = true
```

Additional services are created disabled so merely adding one cannot change a
working runtime.

Duplicating a machine profile copies local runtime/service configuration but
does **not** copy the tunnel ID or credential secret. The duplicate receives a
new credential reference.

Existing schema-1 TaskTrack tunnel profiles migrate automatically to schema 2
with the previous TaskTrack MCP path represented as the `main` service.

## Credential handling

### Recommended: Windows Credential Manager

New profiles default to Windows Credential Manager.

Use **Set key** once in Setup. The secret is entered through a masked
`UiPasswordEdit` and stored as a Windows generic credential under a
profile-specific reference. TaskTrack profile JSON contains only the reference.

At runtime TaskTrack reads the secret into memory and passes
`CONTROL_PLANE_API_KEY` only in the child environment supplied to
`tunnel-client.exe`. TaskTrack does not set the variable in its own process
environment.

The key is never displayed by the manager and is excluded from copied
diagnostics.

Deleting a profile removes its dedicated Windows Credential Manager key.

### Compatibility: environment variable

Existing installs and automation may select **Environment variable** and set:

```powershell
$env:CONTROL_PLANE_API_KEY="sk-..."
```

This preserves the original TaskTrack tunnel behavior but is not the default
for new Windows profiles.

## Runtime launch

For a two-service profile the manager launches the official runtime
conceptually as:

```text
tunnel-client.exe run
  --control-plane.api-key env:CONTROL_PLANE_API_KEY
  --control-plane.tunnel-id <tunnel>
  --mcp.command "channel=main,command=<TaskTrackMcp.exe>"
  --mcp.command "channel=patchtrack,command=<patchtrack_mcp.exe>"
  --health.listen-addr 127.0.0.1:0
  --health.url-file <temp-file>
  --log.file <temp-file>
```

No API key is placed on argv.

The generic child environment also carries:

```text
MCP_TUNNEL_REMOTE=1
MCP_TUNNEL_MACHINE_ID=<machine-id>
MCP_TUNNEL_PROFILE_ID=<profile-id>
```

TaskTrack recognizes `MCP_TUNNEL_REMOTE=1` for remote-activity recording and
temporarily also accepts the retired `TASKTRACK_TUNNEL_REMOTE=1` marker for
migration compatibility.

## Manager pages

### Overview

Overview shows:

- machine/tunnel state;
- configured `main` MCP service;
- OpenAI runtime health;
- TaskTrack-specific remote activity;
- enabled service count;
- recent TaskTrack request/result pairs.

The state model is:

- Ready;
- Connecting;
- Stopped;
- Error.

**Send probe** writes the existing TaskTrack-local diagnostic probe consumed by
the read-only `tunnel_probe` MCP tool.

### Setup

Setup manages:

- machine profiles;
- machine ID;
- tunnel ID;
- secure credential source;
- runtime executable;
- auto-connect;
- remember-profile.

### Services

Services manages explicit channel bindings:

- display name;
- service ID;
- channel;
- command;
- enabled state.

Stop the tunnel before changing profiles or services.

## Local deterministic validation

From the repository root:

```powershell
powershell -ExecutionPolicy Bypass -File .\verify.ps1 -UppRoot <U++ root>
git diff --check
```

The verification wrapper now includes `McpTunnelRuntimeTests.exe` in addition
to the existing TaskTrack targets.

The runtime-model tests cover:

- schema-2 profile round-trip;
- schema-1 migration;
- multi-service argument generation;
- exactly-one-main validation;
- unique service IDs/channels;
- reserved channel rejection;
- 32-channel limit;
- duplicate-profile tunnel/credential separation;
- child-only tunnel environment construction.

## Live acceptance

With Curt's real tunnel ID, runtime key and official runtime:

1. store/select the credential source;
2. confirm TaskTrack is the enabled `main` service;
3. Connect;
4. confirm process running;
5. confirm `/healthz` succeeds;
6. confirm `/readyz` succeeds;
7. from browser ChatGPT call `version`;
8. click **Send probe**, then call `tunnel_probe`;
9. call `list_dashboards`;
10. verify TaskTrack remote activity increments;
11. Stop and reconnect.

Then add a harmless second MCP service on a distinct channel and perform the
multi-channel acceptance:

1. enable the second service;
2. keep exactly one `main` service;
3. reconnect;
4. verify the runtime advertises/accepts both bindings;
5. verify ChatGPT can address the additional channel distinctly;
6. verify TaskTrack calls continue to reach TaskTrack;
7. stop one child service during the test and verify the failure remains
   service-specific.

The official runtime supports the channel bindings used by the manager. The
final ChatGPT product-side addressing of an additional named channel remains a
live acceptance item until tested with the real account.

## Boundary

The machine tunnel layer owns transport/runtime only.

TaskTrack remains authoritative for human decisions and project dashboards.
PatchTrack remains authoritative for transactional file mutation when it is
added as another service. No domain persistence or tool family is merged merely
because the services share one machine tunnel.
