BASE: a1bdbb35d56d58a37a79fac0388281b8ec90e743
TASK: Prove browser ChatGPT can reach local TaskTrack through OpenAI Secure MCP Tunnel.
BUILD: 0.3.2-rc1
STATUS: Runtime-only Platform artifact identified; supervisor now launches its direct run surface.
TOUCHED: TunnelApp runtime supervision, tunnel test guide; MCP probe/schema/persistence unchanged.
BOUNDARY: Official OpenAI runtime owns remote transport; TaskTrack owns only process supervision, health checks and read-only probe.
RUNTIME: Connect -> tunnel-client-runtime run; Status -> /healthz + /readyz; Stop -> local process termination.
SECURITY: API key stays in CONTROL_PLANE_API_KEY; probe no longer includes local computer name.
VALIDATION: Source/API review completed; native CLANGx64 rebuild and live browser acceptance pending.
NEXT: Gary rebuilds current HEAD; Curt runs Connect/Status to ready=true, sends probe, then creates No Auth ChatGPT plugin and tests tunnel_probe.
