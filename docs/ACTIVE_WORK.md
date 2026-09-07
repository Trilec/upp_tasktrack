BASE: 931aa156f228d518a7ab39df90b9207a4f4a981b
TASK: Security-harden the machine-level multi-service MCP tunnel manager after architecture red-team review.
BUILD: 0.3.2-rc1
PUBLISHED: 8ee534bbda579e83c09d9f56f2a4c6404e717765 on main.
STATUS: Windows security hardening implemented and verified: profile-bound session credentials, controlled environment, memory-only vendor key handoff, per-user ownership and kill-on-close descendant containment. See docs/TUNNEL_SECURITY_HANDOFF.md.
ARCHITECTURE: One machine profile owns one official OpenAI tunnel runtime; exactly one enabled main channel plus optional separate named MCP services. TaskTrack/PatchTrack domain semantics remain separate. Native channels are routing boundaries, not failure-isolation boundaries.
SECURITY: Session keys are recipient-bound and memory-held; unchanged vendor file: resolver consumes a protected Windows named pipe. No plaintext key file fallback. Child environment is allowlisted; control-plane endpoint pinned and raw HTTP logging disabled. Durable cross-platform encrypted vault remains deferred.
VALIDATION: Windows full verify passed in separate output dir; 92 runtime checks, 142 TaskTrack Core checks, unified MCP selftest, 27 Dashboard checks, git diff --check, and unchanged-vendor dummy-key pipe compatibility all passed.
OPEN: POSIX handoff/process-tree validation; live GUI/tunnel/ChatGPT/multi-channel acceptance; durable vault; vendor stdio child failure still stops the whole runtime; asynchronous health UI remains deferred.
AUDIT FOLLOW-UP: Strict upstream channel canonicalization and profile schema/persistence hardening from TUNNEL_ARCHITECTURE_AUDIT.md remain to be dispositioned before stable release.
NEXT: Manual/live acceptance on current main, then close remaining audit follow-ups and portable vault/POSIX work before stable 0.3.2 as appropriate.
