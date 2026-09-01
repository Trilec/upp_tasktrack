#ifndef _TaskTrack_Mcp_TaskTrackDashboardBridge_h_
#define _TaskTrack_Mcp_TaskTrackDashboardBridge_h_

/*
    Dashboard tool family for the unified TaskTrack MCP server.

    The dashboard model remains separate from authoritative human answer.data,
    but both tool families are exposed by the single TaskTrackMcp executable.
*/

#include <Core/Core.h>

namespace Upp {

struct TaskTrackDashboardToolResult {
    Value structured;
    bool is_error = false;
};

void TaskTrackAppendDashboardToolSpecs(ValueArray& tools);
bool TaskTrackIsDashboardTool(const String& name);
TaskTrackDashboardToolResult TaskTrackExecuteDashboardTool(const String& name,
                                                         const Value& args);

// Runs the dashboard tool-family persistence/dispatch checks and appends any
// failures to the caller's list. Returns the number of checks that passed.
int TaskTrackDashboardBridgeSelfTest(Vector<String>& failures);

} // namespace Upp

#endif
