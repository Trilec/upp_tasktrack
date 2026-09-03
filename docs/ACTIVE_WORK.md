BASE: e3a24cf363cb844c689dcad6a9253621e945cd32 (remote main with OpenAI tunnel-client docs)
TASK: Prove browser ChatGPT can reach the existing local TaskTrack stdio MCP through OpenAI Secure MCP Tunnel.
BUILD: 0.3.2-rc1
STATUS: Architectural implementation published for native Windows/U++ validation.
TOUCHED: TunnelCore, TunnelApp, unified MCP read-only tunnel_probe, verify.ps1, tunnel test guide.
BOUNDARY: Official OpenAI tunnel-client owns the tunnel transport; TaskTrack does not store the runtime API key and does not disguise writes as reads.
PROBE: TaskTrackTunnelGui writes a tiny co-located diagnostic state; tunnel_probe only reads it and cannot modify task/dashboard/repository state.
IPC: Tunnel is remote ingress only; any future local U++ application bus remains behind TaskTrackMcp.
VALIDATION: Static source/package review only in supervisor environment; native CLANGx64 build and runtime tunnel test are pending.
NEXT: Gary builds seven targets and runs verify.ps1; Curt creates tunnel/runtime key, launches TaskTrackTunnelGui, then browser ChatGPT performs version/tunnel_probe/dashboard read smoke test.
