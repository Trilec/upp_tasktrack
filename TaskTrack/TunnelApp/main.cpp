#include "TaskTrackTunnelManager.h"

using namespace Upp;

namespace {

String TunnelHelpText()
{
    return
        "TaskTrack Tunnel Manager\n"
        "Native supervisor for the official OpenAI Secure MCP Tunnel runtime.\n\n"
        "Usage:\n"
        "  TaskTrackTunnelGui.exe\n"
        "  TaskTrackTunnelGui.exe --tunnel-id <tunnel_id>\n"
        "  TaskTrackTunnelGui.exe --client <path-to-tunnel-client.exe>\n"
        "  TaskTrackTunnelGui.exe --alias <local-alias>\n\n"
        "The Tunnel Manager stores named non-secret profiles locally.\n"
        "CONTROL_PLANE_API_KEY is inherited from the environment and is never stored by TaskTrack.";
}

}

GUI_APP_MAIN
{
    TaskTrackTunnelManagerOptions options;
    options.tunnel_id = GetEnv("TASKTRACK_TUNNEL_ID");
    options.runtime_path = GetExeDirFile("tunnel-client.exe");

    const Vector<String>& cmd = CommandLine();
    for(int i = 0; i < cmd.GetCount(); ++i) {
        if(cmd[i] == "--help") {
            PromptOK(TunnelHelpText());
            return;
        }
        if(cmd[i] == "--version") {
            PromptOK("TaskTrack Tunnel Manager\nTaskTrack build " + TaskTrackBuildVersion());
            return;
        }
        if(cmd[i] == "--tunnel-id" && i + 1 < cmd.GetCount()) {
            options.tunnel_id = cmd[++i];
            continue;
        }
        if(cmd[i] == "--client" && i + 1 < cmd.GetCount()) {
            options.runtime_path = cmd[++i];
            continue;
        }
        if(cmd[i] == "--alias" && i + 1 < cmd.GetCount()) {
            options.alias = cmd[++i];
            continue;
        }

        PromptOK("Unknown or incomplete arguments.\n\n" + TunnelHelpText());
        return;
    }

    TaskTrackTunnelManager window(options);
    window.Run();
}
