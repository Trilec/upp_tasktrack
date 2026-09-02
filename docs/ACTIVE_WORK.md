BASE: f3fed550b144666afa557b96205e5e7e16374a5f (remote main before 0.3.0 release)
RELEASE: Unified TaskTrack + Dashboard 0.3.0

OBJECTIVE: Promote the accepted unified implementation from 0.3.0-rc1 to 0.3.0.
STATUS: Release identity and repository terminology updated; verification passed.
BOUNDARY: One registered TaskTrackMcp.exe launches TaskTrackGui.exe or
TaskTrackDashboardGui.exe. Dashboard state remains separate from human
TaskTrack answer.data and the accepted human interaction lifecycle.
VALIDATION: verify.ps1 must pass six builds, TaskTrack tests, unified MCP
selftest, Dashboard tests, version 0.3.0, task schema 2 and dashboard schema 1.
PUBLISHED: Release commit is on origin/main; Git main HEAD is authoritative.
NEXT: None for this release.
