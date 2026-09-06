BASE: 7a129e887ade286545785fb54767c09ecbeefa17
TASK: Implement the accepted TaskTrack Tunnel Manager visual design in native U++.
BUILD: 0.3.2-rc1
PROOF: Browser ChatGPT already reached the local runtime, returned version 0.3.2-rc1 and tunnel_probe #2, and executed list_dashboards.
STATUS: Full Overview/Setup Tunnel Manager implementation published for Windows validation.
OVERVIEW: polished title card, light/dark theme, Ready/Connecting/Stopped/Error states, profile/tunnel summary, four-cell status strip, recent remote request/result table.
SETUP: named profiles, dropdown/new/duplicate/delete, tunnel ID, runtime/MCP paths, credential-source availability, auto-connect and remember-profile.
ACTIVITY: last six remote communications plus in/out counters; only TASKTRACK_TUNNEL_REMOTE MCP traffic is recorded.
SECURITY: tunnel IDs/profile paths are local configuration; CONTROL_PLANE_API_KEY remains environment-only and is never stored/displayed.
BOUNDARY: no task/dashboard schema, persistence, human lifecycle or tunnel wire-protocol change.
NEXT: Gary compiles/visually validates current HEAD and only fixes concrete native defects; then reconnect and promote 0.3.2 after browser dashboard acceptance.
