BASE: 931aa156f228d518a7ab39df90b9207a4f4a981b
TASK: Harden and operationalize the machine-level multi-service MCP tunnel manager after architecture red-team review.
BUILD: 0.3.2-rc1
SECURITY CHECKPOINT: 8ee534bbda579e83c09d9f56f2a4c6404e717765 passed Windows security verification.
STATUS: Security hardening is published on main. A generated runtime-distribution boundary is now defined by stage-bin.ps1 + bin/README.md; build output remains separate from deployable binaries.
ARCHITECTURE: Keep the official OpenAI tunnel runtime external and unchanged. McpTunnelRuntime owns neutral supervision/configuration; TaskTrack/PatchTrack domains remain separate. Native channels are routing boundaries, not failure-isolation boundaries.
SECURITY: Session keys are recipient-bound and memory-held; Windows vendor handoff uses a protected named pipe with no plaintext key-file fallback. Child environment is allowlisted; control-plane endpoint pinned and raw HTTP logging disabled. Durable cross-platform encrypted vault remains deferred.
PACKAGING: bin/windows-x64/ is generated from verified build output and contains only TaskTrackMcp.exe, TaskTrackGui.exe, TaskTrackDashboardGui.exe, TaskTrackTunnelGui.exe, pinned tunnel-client.exe, notices/SBOM, README and SHA-256 manifest. Vendor artifact is pinned in tunnel-client/runtime-manifest.json.
VALIDATION: Windows security pass: 92 runtime checks, 142 TaskTrack Core checks, unified MCP selftest, 27 Dashboard checks, git diff --check, unchanged-vendor dummy-key pipe compatibility. stage-bin.ps1 still needs Windows execution against the verified build output.
OPEN: Manual GUI/live tunnel/ChatGPT multi-channel acceptance; POSIX handoff/process-tree validation; durable vault; vendor stdio child failure still stops whole runtime; strict upstream channel canonicalization/profile persistence audit follow-up; asynchronous health UI deferred.
NEXT: Gary: pull current main, run full verify in a clean output directory, run stage-bin.ps1, inspect bin/windows-x64 manifest/layout, then perform manual GUI and live tunnel acceptance from the staged bundle.
