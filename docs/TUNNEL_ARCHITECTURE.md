# Machine MCP Tunnel Architecture

## Decision

TaskTrack uses a machine-level Secure MCP Tunnel architecture.

One machine profile owns one official OpenAI tunnel runtime and may bind multiple
separate local MCP services to logical channels.

```text
ChatGPT
   |
OpenAI Secure MCP Tunnel
   |
McpTunnelRuntime
   |-- main / tasktrack -> TaskTrackMcp.exe
   |-- patchtrack       -> patchtrack_mcp.exe
   `-- future services -> their own MCP servers
```

TaskTrack does not implement the OpenAI tunnel wire protocol.

## Ownership boundary

`McpTunnelRuntime` owns only machine/transport concerns:

- machine identity;
- tunnel profile identity;
- tunnel runtime executable;
- credential source/reference;
- logical service/channel bindings;
- runtime process start/stop;
- child-only runtime environment;
- health/readiness probing;
- runtime diagnostics.

TaskTrack continues to own:

- TaskTrack MCP identity and tools;
- human-decision semantics;
- dashboards and revisions;
- `tunnel_probe`;
- TaskTrack-specific remote activity.

Additional MCP services remain separate domain products. Adding a service binding
does not import its persistence, tools or business rules into TaskTrack.

## Machine profile schema

Profile schema 2 contains:

```text
id
name
machine_id
tunnel_id
runtime_path
credential_source
credential_ref
auto_connect
remember_profile
services[]
```

Each service contains:

```text
id
name
channel
command
enabled
```

Exactly one enabled service must use channel `main`. Enabled service IDs and
channels must be unique.

Schema-1 TaskTrack profiles migrate automatically to one enabled TaskTrack
service on `main`.

## Credentials

New Windows profiles default to Windows Credential Manager using a profile-local
generic credential reference such as:

```text
Trilec.McpTunnel/local-machine
```

Only the reference is persisted in the profile JSON.

At launch the manager reads the secret into process memory and constructs a
child-specific environment block for the official tunnel runtime. TaskTrack's
own process environment is not modified.

Environment-variable mode remains available for existing installs and automation:

```text
CONTROL_PLANE_API_KEY
```

No secret value is displayed or written to profile JSON or diagnostics.

## Service routing

The official runtime is launched with one channel-qualified `--mcp.command`
entry for every enabled service.

The manager deliberately does not add a domain-aware MCP gateway. Native logical
channels are the architecture. End-to-end ChatGPT use of additional channels
still requires live connector acceptance with the real tunnel account.

## Compatibility

`MCP_TUNNEL_REMOTE=1` is the generic remote-session marker inherited by child
MCP processes. TaskTrack also recognizes the older
`TASKTRACK_TUNNEL_REMOTE=1` marker during migration.

The TaskTrack tunnel activity file remains TaskTrack-specific. It is not a
machine-wide journal and does not claim to observe uninstrumented third-party
MCP services.

## Security rules

- one machine identity owns one running tunnel profile;
- secrets are not stored in profile JSON;
- no API key appears in runtime argv;
- additional services are explicit and disabled when first created;
- exactly one enabled `main` service is required before connect;
- domain mutation policy belongs to the child MCP service, not the tunnel layer.
