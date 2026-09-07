BASE: f9d26703e85f9f11ae74bf1fa7ad21d6e7be8382
TASK: Replace TaskTrack-only tunnel ownership with the machine-level multi-service MCP tunnel architecture.
BUILD: 0.3.2-rc1
PUBLISHED: 03f6ff13001fbd17c8270865654024e43cba2976 on main.
STATUS: Local uncommitted security follow-up: profile-bound session keys, restricted environment, memory-only runtime handoff, Windows single-owner/job containment. See docs/TUNNEL_SECURITY_HANDOFF.md for completed pieces and validation.
ARCHITECTURE: One machine profile owns one official OpenAI tunnel runtime; exactly one enabled main channel plus optional separate named MCP services. TaskTrack domain semantics remain separate.
SECURITY: RC uses profile-bound session keys or explicit CONTROL_PLANE_API_KEY automation mode; profile JSON never stores the secret. Unchanged vendor file: resolver reads a private Windows named pipe or POSIX stdin pipe, with no plaintext key-file fallback. Child environment is allowlisted. Durable vault remains deferred.
MIGRATION: Schema-1 TaskTrack tunnel profiles migrate to schema 2 with TaskTrack as main and preserve environment credential mode.
VALIDATION: Windows full verify.ps1 passed using build-security-verification (all eight executables; runtime tests plus unchanged vendor pipe proof, 142 Core checks, unified MCP selftest, 27 Dashboard checks). Live tunnel/browser acceptance and POSIX tests remain pending; see TUNNEL_SECURITY_HANDOFF.md.
DEPENDENCY: Validate first against current upp_Ui main; do not revive the previous disposable UiGraph exclusion workaround unless current upstream is proven broken.
NEXT: User restart/publishing and manual GUI/live-channel acceptance. This bounded pass is finished; handoff is TUNNEL_SECURITY_HANDOFF.md. POSIX containment remains outstanding. Vendor stdio child crashes affect the entire runtime; no vendor changes, automatic restart or replay were made.
