# TaskTrack Machine MCP Tunnel

TaskTrack `0.3.2-rc5` uses the official OpenAI Secure MCP Tunnel runtime.
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
does **not** copy the tunnel ID or credential reference. The duplicate starts in
session mode and requires its own key, even when the original uses environment mode.

Existing schema-1 TaskTrack tunnel profiles migrate automatically to schema 2
with the previous TaskTrack MCP path represented as the `main` service.

## Credential handling

For this RC the manager deliberately uses portable validation sources only.

### Session key

Choose **Session key (memory only)** and click **Set key**.

The key is masked in the UI, held only in manager memory, and disappears when
the manager closes. The profile does not persist it. Set the tunnel ID and runtime
path first; changing either, or the machine ID, invalidates the session key.

This is the preferred manual validation path because it proves the tunnel flow
without committing the product to an OS-specific secret store.

### Environment variable

Automation and existing setups may choose **Environment variable** and set:

```powershell
$env:CONTROL_PLANE_API_KEY="sk-..."
```

The manager reads it at launch. The secret is not copied into profile JSON or
diagnostics.

### Durable storage direction

After the tunnel and multi-service flow are accepted, the intended portable
storage is a U++ encrypted vault using `Core/SSL` AES-256-GCM with PBKDF2.
The UX target is:

```text
Stored credential   •••• / short fingerprint
[Replace] [Clear]
```

The full key is never displayed.

OAuth remains relevant for MCP/connector authentication, but the current OpenAI
tunnel runtime still requires its own control-plane runtime API key.

## Runtime launch

For a two-service profile the manager launches the official runtime
conceptually as:

```text
tunnel-client.exe run
  --control-plane.api-key file:<private-pipe-reference>
  --control-plane.tunnel-id <tunnel>
  --control-plane.base-url https://api.openai.com
  --log.http-raw-unsafe=false
  --mcp.command "channel=main,command=<TaskTrackMcp.exe>"
  --mcp.command "channel=patchtrack,command=<PatchTrackMcp.exe>"
  --health.listen-addr 127.0.0.1:0
  --health.url-file <temp-file>
  --log.file <temp-file>
```

No API key is placed on argv.

Windows uses a current-user-only named pipe with expected-reader PID verification
and bounded handoff. POSIX uses `/dev/stdin` backed by the anonymous child stdin
pipe (platform acceptance pending). Neither path writes a plaintext key file.
Windows assigns the runtime to a kill-on-close Job Object before releasing the
key. Stop ends the entire tree, including service-launched GUIs. One runtime is
allowed per Windows user across sessions. This does not enforce machine-wide
ownership across different users; POSIX ownership/containment is pending.

Only OS/session plumbing is inherited. Ambient vendor profiles, arbitrary secret
variables and proxy settings are excluded; custom proxies/CAs require a future
explicit configuration interface.

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
- configured `main` MCP service and whether the configured TaskTrack MCP matches the verified staged bundle;

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

Changes auto-save to the profile JSON. The footer shows the current auto-save
state and diagnostics include the exact profile-store path. RC5 stores the
profile, local probe and remote-activity JSON under the shared per-user
`TaskTrack/tunnel` application-data folder rather than inside the staged
runtime bundle; legacy executable-relative files are migrated/preserved.
On Windows, pasted simple executable paths are canonicalized to forward slashes
before persistence and launch because the upstream tunnel runtime treats
backslash as an escape in stdio command strings.

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
- child runtime environment excludes OpenAI control/admin key variables;
- profile-bound session keys and invalidation;
- Windows pipe EOF, expected reader and timeout cleanup;
- Windows repeated start/stop, duplicate owner and runtime/manager crash containment.

If the bundled vendor runtime is present, `verify.ps1` also runs the unchanged
binary key-pipe compatibility test. It uses a dummy key, loopback control-plane
endpoint and deliberately missing MCP executable; it proves key resolution, not
live authentication or channel acceptance.

## Live acceptance

With Curt's real tunnel ID, runtime key and official runtime:

1. store/select the credential source;
2. confirm TaskTrack is the enabled `main` service;
3. Connect;
4. confirm process running;
5. confirm `/healthz` succeeds;
6. confirm `/readyz` succeeds;
7. from browser ChatGPT call `version`;
8. require `build_version = 0.3.2-rc5`, `bundle_verified = true`, and compare `executable_sha256` + `bundle_source_commit` with `bin/windows-x64/manifest.json`;
9. if these do not match, stop acceptance and restart/reconfigure the MCP host rather than testing stale code;
10. click **Send probe**, then call `tunnel_probe` and confirm it reports the same executable/bundle identity;
11. call `list_dashboards`;
12. verify TaskTrack remote activity increments;
13. Stop and reconnect; after reconnect, repeat `version` and require the same identity.

After the single-channel reconnect passes, create a real dashboard through the
remote MCP rather than seeding production state in code:

1. call `upsert_dashboard` for a small acceptance dashboard with ID
   `dashboard-tunnel-acceptance`;
2. include at least Project State, Verification and Next Steps panels so the
   native renderer has meaningful content;
3. call `list_dashboards` and require that dashboard to appear;
4. call `get_dashboard` and verify the stored content/revision;
5. call `open_dashboard` and require `launched = true`; RC5 launches the Windows GUI through a visible process path and acknowledges from the GUI event loop after the exact normalized dashboard has loaded;
6. confirm the staged `TaskTrackDashboardGui.exe` is foregrounded with the
   requested dashboard already loaded (no file picker);
7. also launch `TaskTrackDashboardGui.exe` with no arguments and confirm the
   empty shell shows a visible Load dashboard action plus recent-dashboard summary;
8. leave the dashboard in the store as explicit acceptance data unless Curt
   chooses to delete it later.

Before PatchTrack acceptance, test the original human-decision workflow through
browser ChatGPT. Start with the simplest round-trip so the tunnel/client boundary
is isolated from agent-assistance callbacks:

1. call `create_task` with one required `confirm` item and `launch=true`;
2. require the native TaskTrack GUI to become visibly open;
3. answer the item in the GUI and Submit;
4. require the SAME ChatGPT tool call/turn to return the completed structured
   human answer without a manual chat wake-up;
5. confirm Tunnel Manager activity increments for the create_task request/result.

If that passes, test the harder assistance round-trip separately:

1. create another one-item task;
2. in the GUI press Suggest or Clarify;
3. observe whether ChatGPT receives/resolves the pending agent request through
   modern in-call `input_required` sampling;
4. if the host cannot service the callback in-call, require the compatibility
   path (`respond_to_request` + `get_task(... wait_ms=300000)`) to continue
   without fabricating human evidence;
5. verify Accept remains the human act that creates `answer.data`.

Classify these independently as HUMAN RETURN PASS/FAIL and ASSISTANCE CALLBACK
PASS/COMPATIBILITY/FAIL. Do not infer one from the other.

Then add PatchTrack as a second MCP service on a distinct channel and perform the
multi-channel acceptance:

1. enable the second service;
2. keep exactly one `main` service;
3. reconnect;
4. verify the runtime advertises/accepts both bindings;
5. verify ChatGPT can address the additional channel distinctly;
6. verify TaskTrack calls continue to reach TaskTrack;
7. stop one child service: the unchanged runtime currently exits as a whole.
   Verify the manager reports the exit and removes remaining Windows descendants.
   Do not claim service-specific failure isolation or replay uncertain mutations.

The official runtime supports the channel bindings used by the manager. The
final ChatGPT product-side addressing of an additional named channel remains a
live acceptance item until tested with the real account.

## Boundary

The machine tunnel layer owns transport/runtime only.

TaskTrack remains authoritative for human decisions and project dashboards.
PatchTrack remains authoritative for transactional file mutation when it is
added as another service. No domain persistence or tool family is merged merely
because the services share one machine tunnel.
