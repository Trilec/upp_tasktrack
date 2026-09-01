BASE: bbd9f566d7222fac336fcd458eb63a873a997823 (TaskTrack main before dashboard work)
TASK: Complete native AI-maintained project dashboard, TaskTrack-local timeline rail, persistence/MCP, tests/docs.
TOUCHED: DashboardCore, DashboardWidgets, DashboardApp, DashboardMcp, dashboard tests/examples/docs, README, verify, ignore rules.
STATUS: Implementation complete; published checkpoint requires native Windows/U++ visual/build validation.
PUBLISHED: This recovery record is included in the dashboard implementation publish; use Git history for the exact commit SHA.
VALIDATION: Full static source/diff audit performed against TaskTrack base and upp_Ui f1d20a7abbcb4edec614c051bd761cf94eb170bf.
VALIDATION: Deterministic DashboardCore and DashboardMcp tests are included in verify.ps1; native execution requires the Windows U++ assembly.
BOUNDARY: Existing TaskTrack human-question Core/App/Mcp lifecycle is unchanged. Dashboard state never becomes human answer.data.
NEXT: Run verify.ps1, open examples/TaskTrackDashboardExample/project-dashboard.json, inspect light/dark + narrow/wide layouts, then fix only observed bounded defects.
