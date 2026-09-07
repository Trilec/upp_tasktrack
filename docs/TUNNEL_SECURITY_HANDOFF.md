# Tunnel security implementation handoff

Scope: TaskTrack changes only; official OpenAI runtime stays unchanged. Published on main at `8ee534bbda579e83c09d9f56f2a4c6404e717765`. The original audit remains a historical review of checkpoint `931aa156`.

## Product intent and stopping point

PatchTrack is a dedicated code patcher. TaskTrack provides thoughtful native human
question/answer workflows and graphical, agent-maintained project dashboards.
The tunnel lets chatbots reach these local MCP services when they cannot access
the coding agent's own MCP connections. Each of the two machines has its own
tunnel/key. Keep the service boundaries and direct-local access intact; no
aggregator or transport rewrite is needed to complete this goal.

This was a bounded, tested hardening pass. The completed pieces below are now published on main. Do not modify the third-party runtime merely to extend this pass.
Embedded U++ transport and durable portable key storage are future decisions.
The vault should protect retained local credentials without requiring an OS
keychain; it cannot protect an unlocked process from equivalent local access.

## Piece 1 — profile-bound session credentials

Implemented: opaque credential references, independent session entries bound to profile/machine/tunnel/runtime path, invalidation on recipient changes, replacement/clear/delete cleanup, and duplicates unbound in session mode. Owned secret storage is wiped on removal; UI and launch String temporaries still exist. Also fixed the default CLI runtime path overwriting a saved selection at startup.

Validation: Windows runtime tests passed (45 checks at this checkpoint); TunnelApp compiled successfully after correcting existing Windows ERROR macro collision, Vector::Find test use, and password edit method chaining. The state enum is now FAULT.

## Piece 2 — controlled runtime environment

Implemented: OS/session environment allowlist, explicit OpenAI control-plane URL and raw HTTP logging disabled. Arbitrary credentials, vendor profile/routing overrides, proxy variables and interpreter/loader overrides are excluded. NUL injection through profile identity is rejected. Corporate proxies/custom CAs and service-specific secret environments now need an explicit future configuration interface; ambient overrides are intentionally unsupported.

Validation: 51 runtime tests passed, including distinctive seeded secrets/configuration; Windows TunnelApp compiled and linked.

## Piece 3 — memory-only handoff

Implemented: deleted plaintext key-file creation/cleanup. Windows uses a protected current-user-only, local-only named pipe, expected-reader PID verification, bounded overlapped connect/write, and EOF by closing the server handle. Timeout, wrong reader and failed launch close handles. No disk fallback. POSIX uses the anonymous child stdin pipe through `/dev/stdin`, with charset conversion disabled; Linux/macOS acceptance is still required.

Validation: 74 checks passed at this checkpoint, including actual bundled unchanged vendor binary, EOF and wrong-reader/timeout cases. Vendor compatibility test uses a dummy key, loopback control plane and intentionally missing MCP executable. No real credential or external service required.

Tested vendor artifact SHA-256: `09eac072d392b8d27b7aea8cbc146ab3738934961278b6b69cb23513607a08d7`. This records tested bytes, not an upstream signature/provenance verification. Runtime path binding does not detect replacement of executable bytes at that path.

## Piece 4 — Windows ownership and crash containment

Implemented: one owner per Windows user across sessions, independent of editable profile IDs. A kill-on-close Job Object is assigned before sending the key; the trusted vendor resolves configuration before spawning MCP children. Failed containment releases no secret. Stop terminates the tree and waits for bounded confirmation before releasing ownership; manager crash closes the job. Service-launched GUI descendants are part of that tree. Runtime crash is detected without an unbounded output-drain Finish call. A running fault can now be stopped from the UI. Health requests use whole-request deadlines, no retries/redirects and bounded bodies.

Validation: 91 checks passed with vendor compatibility at the containment checkpoint, including repeated start/stop, duplicate owner, root crash, manager force-kill, descendant termination and owner recovery. All eight executables subsequently compiled; full verify.ps1 passed (142 Core checks, unified MCP selftest including 10 dashboard checks, 27 Dashboard checks). A final test-only refinement waits explicitly for manager termination before reacquiring ownership; its targeted result is recorded below.

Final targeted rebuild/result: **92 runtime checks passed, zero failures**, including
the unchanged vendor compatibility test. `git diff --check` passed. No Linux/macOS,
manual GUI or live connector acceptance is implied by these results.

This is forced Windows tree shutdown, not a vendor graceful-shutdown protocol. Windows per-user ownership is not a cross-user machine singleton. POSIX tree supervision/ownership and graceful bounded shutdown remain outstanding. No automatic restart or mutation replay was added.

## Remaining work

- Manual GUI acceptance and real-account main/second-channel routing; no live credential or ChatGPT connector test was performed.
- POSIX memory-only handoff acceptance and process-tree ownership/containment; no POSIX test results are claimed.
- Graceful shutdown protocol (if vendor-supported) before the bounded forced Windows tree termination.
- Vendor limitation: stdio child failure requests whole-runtime shutdown. Do not claim service failure isolation. No vendor fork or runtime patch is planned.
- Durable encrypted vault and asynchronous UI health polling are outside this focused pass.

## Original audit issues 1–5

| Issue | Handoff status |
| --- | --- |
| 1. Child-failure isolation | Vendor limitation documented; unchanged stdio runtime still has shared fate. |
| 2. Plaintext key file | Removed. Windows pipe tested against the vendor; POSIX acceptance pending. |
| 3. Ambient environment | Restricted; custom proxy/CA/service-secret configuration requires explicit future support. |
| 4. Profile credential binding | Implemented and tested for session credentials; durable vault deferred. |
| 5. Process ownership/lifetime | Windows ownership and forced tree cleanup implemented/tested. POSIX and graceful shutdown remain open. |

## Reproduce / restart

The normal build directory contained an active, locked TaskTrackMcp.exe. No user
process was terminated. The user subsequently authorized stopping that testing
process if needed; the separate build had already avoided the conflict.

```powershell
.\verify.ps1 -UppRoot E:\upp-18468 -OutputDir E:\apps\github\upp_tasktrack\build-security-verification
```

Fresh binaries are in `build-security-verification/`, including TaskTrackMcp.exe,
TaskTrackTunnelGui.exe and its companion GUIs. The vendor binary remains at
`tunnel-client/tunnel-client-runtime.exe`; select it explicitly in Setup or pass
its absolute path with `--client`. Do not mistake the active old MCP process for
the newly verified build. Restart/switch binaries when ready. Source is already published on main; the separate verification binaries remain local build artifacts.
