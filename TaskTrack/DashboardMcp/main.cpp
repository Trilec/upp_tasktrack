/* TaskTrack Dashboard stdio MCP companion. */
#include <Core/Core.h>
#include <TaskTrack/DashboardCore/TaskTrackDashboardCore.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
using namespace Upp;
namespace {
#include "TaskTrackDashboardMcpBase.inc"
#include "TaskTrackDashboardMcpSchema.inc"
#include "TaskTrackDashboardMcpTools.inc"
#include "TaskTrackDashboardMcpProtocol.inc"
#include "TaskTrackDashboardMcpSelfTest.inc"
}
CONSOLE_APP_MAIN{const Vector<String>& cmd=CommandLine();if(cmd.IsEmpty()){SetExitCode(RunServer());return;}if(cmd.GetCount()==1&&cmd[0]=="--version"){Cout()<<"TaskTrack Dashboard MCP\ndashboard version "<<TaskTrackDashboardVersion()<<"\nschema version "<<TASKTRACK_DASHBOARD_SCHEMA_VERSION<<"\nMCP protocol "<<CURRENT_PROTOCOL<<"\n";return;}if(cmd.GetCount()==1&&cmd[0]=="--selftest"){SetExitCode(RunSelfTest());return;}if(cmd.GetCount()==2&&cmd[0]=="--oneshot"){SetExitCode(RunOneShot(cmd[1]));return;}if(cmd.GetCount()==1&&cmd[0]=="--help"){Cout()<<HelpText();return;}Cerr()<<"Unknown TaskTrack Dashboard MCP arguments.\n"<<HelpText();SetExitCode(2);}
