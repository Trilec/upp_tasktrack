#include "TaskTrackDashboardApp.h"

using namespace Upp;

static String DashboardHelpText()
{
    return "TaskTrack Dashboard GUI\n"
           "Native read-only project-state cockpit.\n\n"
           "Usage:\n"
           "  TaskTrackDashboardGui.exe\n"
           "      Open the dashboard shell; use Load dashboard to choose a file.\n"
           "  TaskTrackDashboardGui.exe --dashboard <path>\n"
           "      Open the specified dashboard.\n"
           "  TaskTrackDashboardGui.exe --dashboard <path> --launch-ack <path>\n"
           "      Internal MCP launch handshake.\n"
           "  TaskTrackDashboardGui.exe --help\n"
           "  TaskTrackDashboardGui.exe --version\n";
}

static void WriteLaunchAck(const String& ack_path, bool ok,
                           const String& dashboard_path, const String& detail)
{
    if(ack_path.IsEmpty())
        return;
    String out;
    out << (ok ? "ok" : "error") << "\n";
    if(ok)
        out << NormalizePath(dashboard_path) << "\n";
    else
        out << detail << "\n";
    SaveFile(ack_path, out);
}

GUI_APP_MAIN
{
    const Vector<String>& cmd = CommandLine();

    if(cmd.GetCount() == 1 && cmd[0] == "--help") {
        PromptOK(DashboardHelpText());
        return;
    }
    if(cmd.GetCount() == 1 && cmd[0] == "--version") {
        PromptOK("TaskTrack Dashboard GUI\nDashboard version " + TaskTrackDashboardVersion() +
                 "\nSchema version " + AsString(TASKTRACK_DASHBOARD_SCHEMA_VERSION));
        return;
    }

    String dashboard_path;
    String launch_ack;
    for(int i = 0; i < cmd.GetCount();) {
        if(cmd[i] == "--dashboard" && i + 1 < cmd.GetCount()) {
            dashboard_path = cmd[i + 1];
            i += 2;
            continue;
        }
        if(cmd[i] == "--launch-ack" && i + 1 < cmd.GetCount()) {
            launch_ack = cmd[i + 1];
            i += 2;
            continue;
        }
        PromptOK("Unknown or incomplete arguments.\n\n" + DashboardHelpText());
        return;
    }

    TaskTrackDashboardWindow window;
    if(dashboard_path.IsEmpty()) {
        window.ShowEmptyState();
        window.Open();
        window.SetForeground();
        window.Run();
        return;
    }

    String error;
    if(!window.LoadDashboard(dashboard_path, error)) {
        WriteLaunchAck(launch_ack, false, dashboard_path, error);
        Exclamation("Unable to open TaskTrack dashboard.\n" + error);
        return;
    }

    window.Open();
    window.SetForeground();
    WriteLaunchAck(launch_ack, true, dashboard_path, String());
    window.Run();
}
