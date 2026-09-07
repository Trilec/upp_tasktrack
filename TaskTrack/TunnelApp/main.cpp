#include "TaskTrackTunnelManager.h"

using namespace Upp;

namespace {

String TunnelHelpText()
{
    return
        "MCP Tunnel Manager\n"
        "Native machine-level supervisor for the official OpenAI Secure MCP Tunnel runtime.\n\n"
        "Usage:\n"
        "  TaskTrackTunnelGui.exe\n"
        "  TaskTrackTunnelGui.exe --tunnel-id <tunnel_id>\n"
        "  TaskTrackTunnelGui.exe --client <path-to-tunnel-client.exe>\n"
        "  TaskTrackTunnelGui.exe --alias <local-alias>\n\n"
        "Machine profiles can expose multiple named local MCP services through one tunnel runtime.\n"
        "For validation, use a session-only key or CONTROL_PLANE_API_KEY.\n"
        "Durable cross-platform encrypted storage is intentionally deferred until the tunnel flow is accepted.\n"
        "Secrets are never written to the TaskTrack profile or displayed in diagnostics.";
}

}

GUI_APP_MAIN
{
    TaskTrackTunnelManagerOptions options;
    options.tunnel_id = GetEnv("TASKTRACK_TUNNEL_ID");

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
