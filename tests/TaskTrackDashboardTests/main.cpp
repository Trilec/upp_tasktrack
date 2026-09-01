#include <Core/Core.h>
#include <TaskTrack/DashboardCore/TaskTrackDashboardCore.h>
using namespace Upp;
namespace {
#include "TaskTrackDashboardTestHelpers.inc"
#include "TaskTrackDashboardTestCases.inc"
}
CONSOLE_APP_MAIN
{
#include "TaskTrackDashboardTestRunA.inc"
#include "TaskTrackDashboardTestRunB.inc"
}
