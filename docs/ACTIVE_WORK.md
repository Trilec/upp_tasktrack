BASE: 3f0eb25d5ca1073ba0aa6def3b36d5e816ca691c (remote main before 0.3.1)
RELEASE: TaskTrack 0.3.1 agent workflow guidance

OBJECTIVE: Make human TaskTrack, Dashboard and cross-agent handoff usage clear
to an unfamiliar capable developer/agent without changing application behavior.
STATUS: Skills, MCP descriptions/schemas, README and handoff guidance updated; verification passed.
BOUNDARY: One registered TaskTrackMcp.exe launches TaskTrackGui.exe or
TaskTrackDashboardGui.exe; human evidence remains items[].answer.data and
dashboard state remains agent-authored current project presentation.
VALIDATION: verify.ps1, MCP description/version selftest and complete diff review
passed; task schema 2, dashboard schema 1 and persistence remain unchanged.
PUBLISHED: Release commit is on origin/main; Git main HEAD is authoritative.
NEXT: None for this release.
