BASE: f9d26703e85f9f11ae74bf1fa7ad21d6e7be8382
TASK: Replace TaskTrack-only tunnel ownership with the machine-level multi-service MCP tunnel architecture.
BUILD: 0.3.2-rc1
BRANCH: tt-tunnel-machine-runtime
TOUCHED: McpTunnelRuntime/; tests/McpTunnelRuntimeTests/; TaskTrack/TunnelApp/; TaskTrack/TunnelCore/TaskTrackTunnelCore.cpp; README.md; docs/TUNNEL_ARCHITECTURE.md; docs/TUNNEL_TEST.md; verify.ps1.
STATUS: Machine profile + services[] + neutral runtime supervisor + secure credential flow implemented; source review complete; Windows/platform validation pending.
ARCHITECTURE: One machine profile owns one official OpenAI tunnel runtime; exactly one enabled main channel plus optional separate named MCP services. TaskTrack domain semantics remain separate.
SECURITY: New Windows profiles default to Windows Credential Manager; profile JSON stores only a credential reference. Environment mode remains compatibility-only. Secret is passed only in the tunnel child environment, never argv/profile/diagnostics.
MIGRATION: Schema-1 TaskTrack tunnel profiles migrate to schema 2 with TaskTrack as main and preserve environment credential mode.
VALIDATION: Added deterministic McpTunnelRuntimeTests to verify.ps1; live tunnel/browser acceptance still requires Curt's tunnel ID/key/runtime.
DEPENDENCY: Validate first against current upp_Ui main; do not revive the previous disposable UiGraph exclusion workaround unless current upstream is proven broken.
NEXT: Windows compile/full verify + Overview/Setup/Services visual/credential validation; then live TaskTrack and second-channel ChatGPT acceptance before merge to main/stable 0.3.2.
