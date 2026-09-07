BASE: f9d26703e85f9f11ae74bf1fa7ad21d6e7be8382
TASK: Replace TaskTrack-only tunnel ownership with the machine-level multi-service MCP tunnel architecture.
BUILD: 0.3.2-rc1
PUBLISHED: 03f6ff13001fbd17c8270865654024e43cba2976 on main.
STATUS: Machine profile + services[] + neutral runtime supervisor + portable RC credential flow are now on main; Windows/platform and live tunnel validation remain pending.
ARCHITECTURE: One machine profile owns one official OpenAI tunnel runtime; exactly one enabled main channel plus optional separate named MCP services. TaskTrack domain semantics remain separate.
SECURITY: RC uses session-only memory key or CONTROL_PLANE_API_KEY; profile JSON never stores the secret. Current runtime handoff uses a short-lived file: reference and MCP children inherit no OpenAI control/admin key variables. Durable direction is a cross-platform U++ AES-256-GCM vault after security review.
MIGRATION: Schema-1 TaskTrack tunnel profiles migrate to schema 2 with TaskTrack as main and preserve environment credential mode.
VALIDATION: Added deterministic McpTunnelRuntimeTests to verify.ps1; live tunnel/browser acceptance still requires Curt's tunnel ID/key/runtime.
DEPENDENCY: Validate first against current upp_Ui main; do not revive the previous disposable UiGraph exclusion workaround unless current upstream is proven broken.
NEXT: Run the architecture/security red-team audit against main, then Windows compile/full verify + Overview/Setup/Services/session-key validation and live TaskTrack/second-channel acceptance.
