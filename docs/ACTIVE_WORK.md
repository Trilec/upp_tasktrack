BASE: f21cfada55f9fb1ae8a15665673f5a088f7d8419 (published dashboard companion before MCP unification)
TASK: Unify human-decision and dashboard tool families behind the single registered TaskTrackMcp.exe.
TOUCHED: TaskTrack/Mcp, TaskTrackBuild identity, DashboardCore version marker, verify/docs/plugin/skill; retire standalone DashboardMcp package.
STATUS: Unified MCP correction implemented; publication checkpoint complete when this record reaches main.
VERSION: Unified build 0.3.0-rc1; task schema 2; dashboard schema 1.
BOUNDARY: Human create_task lifecycle remains accepted code; dashboard Core/Widgets/App remain separate and dashboard state never becomes human answer.data.
PUBLISHED: This record travels with the 0.3.0-rc1 unification commit; Git main HEAD is authoritative.
VALIDATION: Static source/package/diff checks performed; native Windows/U++ compile and refreshed-Codex runtime gate remain.
NEXT: Build all six targets, run human+dashboard tests and unified MCP selftest, install TaskTrackMcp.exe with both GUIs, restart Codex, verify 0.3.0-rc1.
