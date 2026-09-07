# Machine MCP Tunnel Architecture

## Product purpose

PatchTrack began as a dedicated code-patching tool to make agent edits more
reliable. TaskTrack provides native human-question workflows and agent-maintained
project dashboards, so people can consider questions carefully and see current
project status graphically. These remain separate products with separate contracts.

The tunnel extends access to those local services from chatbots that cannot use
the coding agent's local MCP connections. It is an additional access path, not a
replacement for direct local MCP use. Two physical machines have separate tunnel
identities and runtime credentials. Profiles organize each machine's connection;
they are not a reason to combine the services or share keys between machines.

The present implementation deliberately supervises the official runtime executable
unchanged. A future U++-embedded transport is an optional footprint/packaging
decision requiring its own protocol compatibility and maintenance assessment; it
is not part of the current security work. Portable encrypted storage should give
operators a practical way to retain their local keys, not introduce an enterprise
identity system. Session-only keys remain usable while the vault is deferred.

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

Credential storage is intentionally separated from the machine/tunnel model.

For RC validation the manager supports:

- `session`: the runtime key is pasted once and held in memory only for the
  lifetime of the manager process;
- `environment`: `CONTROL_PLANE_API_KEY` is read at launch for automation
  and compatibility.

The machine profile never persists the secret.

The intended durable cross-platform design is a U++ encrypted vault built on
`Core/SSL` AES-256-GCM. Current U++ provides authenticated AES-256-GCM,
PBKDF2-HMAC-SHA256 key derivation, random salt/IV generation, `SecureBuffer`
and `SecureZero`.

A future vault should:

- store only authenticated ciphertext plus non-secret metadata;
- unlock with a user-controlled secret or another explicit portable unlock
  provider;
- expose only stored/not-stored plus a short non-secret fingerprint/hint;
- allow replace and clear without ever displaying the full key;
- keep the vault backend independent of Windows/macOS/Linux credential stores;
- allow optional platform keychain or SSH-agent providers later without making
  them architectural requirements.

The official OpenAI tunnel runtime currently still requires a control-plane
runtime API key. Its OAuth support applies to MCP/connector authentication and
does not currently remove this runtime-key requirement.

Session entries are bound to profile, machine, tunnel and runtime path. Changing
that recipient invalidates the entry; duplicates are unbound in session mode.
Only an opaque credential reference is persisted, never the key.

The official runtime stays unchanged. On Windows the existing `file:` resolver
reads a private, local-only named pipe. The manager verifies the reader PID,
bounds connect/write waits and closes the pipe to supply EOF. No plaintext key
file is created. POSIX uses the inherited stdin pipe through `file:/dev/stdin`
and requires platform acceptance testing. There is no disk-file fallback.

The child environment contains only selected OS/session variables and the
generic remote markers. Ambient credentials, vendor configuration selectors,
proxy and loader overrides are excluded. The OpenAI control-plane URL is explicit
and raw HTTP logging is disabled. Custom enterprise configuration needs a future
explicit interface; environment overrides are not supported.

Windows ownership is one runtime per user across sessions, enforced by an OS
object independent of editable machine/profile IDs. Before releasing the key,
the manager puts the runtime in a kill-on-close Job Object. Stop terminates the
whole tree, including service-launched GUI descendants; a manager crash also
closes the job. POSIX process-tree containment is still outstanding.

Native channels separate routing, not failure domains: the unchanged vendor
runtime shuts down on a stdio child failure. The manager reports that exit and
cleans up its tree; it does not automatically restart or replay interrupted calls.

No secret value is displayed or written to machine profile JSON or diagnostics.

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
