#include <Ui/Ui.h>
#include <TaskTrack/Core/TaskTrackBuild.h>
#include <TaskTrack/TunnelCore/TaskTrackTunnelCore.h>

using namespace Upp;

namespace {

String CompactOutput(String text)
{
    text.Replace("\r", "");
    text.Replace("\n", " | ");
    while(text.Find("  ") >= 0)
        text.Replace("  ", " ");
    if(text.GetCount() > 280)
        text = text.Left(277) + "...";
    return text;
}

Value FindJsonKey(const Value& value, const String& key)
{
    if(value.Is<ValueMap>()) {
        ValueMap map = value;
        int q = map.Find(key);
        if(q >= 0)
            return map[q];
        for(int i = 0; i < map.GetCount(); ++i) {
            Value found = FindJsonKey(map.GetValue(i), key);
            if(!IsNull(found))
                return found;
        }
    }
    else if(value.Is<ValueArray>()) {
        ValueArray array = value;
        for(int i = 0; i < array.GetCount(); ++i) {
            Value found = FindJsonKey(array[i], key);
            if(!IsNull(found))
                return found;
        }
    }
    return Value();
}

class TaskTrackTunnelWindow : public TopWindow {
public:
    typedef TaskTrackTunnelWindow CLASSNAME;

    TaskTrackTunnelWindow(const String& tunnel_id, const String& client_path, const String& alias)
        : tunnel_id_(tunnel_id), client_path_(client_path), alias_(alias)
    {
        mcp_path_ = GetExeDirFile("TaskTrackMcp.exe");
        Title("TaskTrack Tunnel");
        Sizeable().Zoomable();
        SetRect(0, 0, DPI(760), DPI(430));
        SetMinSize(Size(DPI(640), DPI(380)));
        surface_.SetRadius(0);
        BuildUi();
        Wire();
        RefreshLocalState();
    }

private:
    String tunnel_id_;
    String client_path_;
    String alias_;
    String mcp_path_;
    String health_url_;
    String health_url_file_;
    String last_runtime_output_;
    LocalProcess runtime_;

    UiPanel surface_;
    UiBoxLayout root_{UiDirection::V};
    UiGroupPanel status_group_;
    UiBoxLayout status_box_{UiDirection::V};
    UiLabel version_label_;
    UiLabel tunnel_label_;
    UiLabel client_label_;
    UiLabel mcp_label_;
    UiLabel key_label_;
    UiLabel runtime_label_;
    UiLabel probe_label_;
    UiBoxLayout actions_{UiDirection::H};
    UiButton connect_button_;
    UiButton status_button_;
    UiButton stop_button_;
    UiButton probe_button_;
    UiButton open_ui_button_;
    UiButton close_button_;
    UiLabel note_label_;

    void BuildUi()
    {
        surface_.Add(root_.SizePos());
        Add(surface_.SizePos());

        root_.SetDirection(UiDirection::V).SetGap(DPI(8)).SetInset(DPI(10));

        status_group_.SetTitle("Secure MCP Tunnel")
                     .SetSubTitle("Official OpenAI tunnel-client supervising the local TaskTrack MCP");

        status_box_.SetDirection(UiDirection::V).SetGap(DPI(4)).SetInset(DPI(8));
        status_box_.Add(version_label_).Fit().MinCross(DPI(22));
        status_box_.Add(tunnel_label_).Fit().MinCross(DPI(22));
        status_box_.Add(client_label_).Fit().MinCross(DPI(22));
        status_box_.Add(mcp_label_).Fit().MinCross(DPI(22));
        status_box_.Add(key_label_).Fit().MinCross(DPI(22));
        status_box_.Add(runtime_label_).Fit().MinCross(DPI(28));
        status_box_.Add(probe_label_).Fit().MinCross(DPI(22));
        status_group_.SetContent(status_box_);

        actions_.SetDirection(UiDirection::H)
                .SetGap(DPI(6))
                .SetInset(0)
                .SetWrap(UiBoxWrap::Flow)
                .SetWrapAutoResize(true)
                .SetAlignItems(UiCrossAlign::Center);

        connect_button_.SetText("Connect").SetContentInset(DPI(4));
        status_button_.SetText("Status").SetContentInset(DPI(4));
        stop_button_.SetText("Stop").SetContentInset(DPI(4));
        probe_button_.SetText("Send probe").SetContentInset(DPI(4));
        open_ui_button_.SetText("Open tunnel UI").SetContentInset(DPI(4));
        close_button_.SetText("Close window").SetContentInset(DPI(4));

        actions_.Add(connect_button_).Fixed(DPI(88)).MinCross(DPI(30));
        actions_.Add(status_button_).Fixed(DPI(80)).MinCross(DPI(30));
        actions_.Add(stop_button_).Fixed(DPI(70)).MinCross(DPI(30));
        actions_.Add(probe_button_).Fixed(DPI(100)).MinCross(DPI(30));
        actions_.Add(open_ui_button_).Fixed(DPI(118)).MinCross(DPI(30));
        actions_.AddSpacer(1).Expand(1).MinMain(DPI(8));
        actions_.Add(close_button_).Fixed(DPI(106)).MinCross(DPI(30));

        note_label_.SetText(
            "The runtime API key is read only from CONTROL_PLANE_API_KEY and is never stored by "
            "TaskTrack. Send probe changes local read-only probe state; browser ChatGPT reads it "
            "through the tunnel_probe MCP tool.");

        root_.Add(status_group_).Fit().MinMain(DPI(230)).AlignSelf(UiCrossAlign::Stretch);
        root_.Add(actions_).Fit().MinMain(DPI(34)).AlignSelf(UiCrossAlign::Stretch);
        root_.Add(note_label_).Expand(1).MinMain(DPI(70)).AlignSelf(UiCrossAlign::Stretch);
    }

    void Wire()
    {
        connect_button_.WhenAction = [=] { ConnectRuntime(); };
        status_button_.WhenAction = [=] { RefreshRuntimeStatus(true); };
        stop_button_.WhenAction = [=] { StopRuntime(); };
        probe_button_.WhenAction = [=] { SendProbe(); };
        open_ui_button_.WhenAction = [=] { OpenTunnelUi(); };
        close_button_.WhenAction = [=] { Close(); };
    }

    bool RunClient(const Vector<String>& args, String& output, int& exit_code)
    {
        output.Clear();
        exit_code = -1;
        if(!FileExists(client_path_)) {
            output = "tunnel-client executable not found: " + client_path_;
            return false;
        }

        LocalProcess process;
        if(!process.Start(~client_path_, args)) {
            output = "Unable to start tunnel-client.";
            return false;
        }

        exit_code = process.Finish(output);
        return true;
    }

    void RefreshLocalState()
    {
        version_label_.SetText("TaskTrack build: " + TaskTrackBuildVersion());
        tunnel_label_.SetText("Tunnel: " + (tunnel_id_.IsEmpty() ? String("NOT SET") : tunnel_id_)
                              + "   Alias: " + alias_);
        client_label_.SetText(String("Tunnel client: ")
                              + (FileExists(client_path_) ? "FOUND   " : "MISSING   ")
                              + client_path_);
        mcp_label_.SetText(String("TaskTrack MCP: ")
                           + (FileExists(mcp_path_) ? "FOUND   " : "MISSING   ")
                           + mcp_path_);
        key_label_.SetText(String("Runtime key: ")
                           + (GetEnv("CONTROL_PLANE_API_KEY").IsEmpty()
                                  ? "NOT SET (set CONTROL_PLANE_API_KEY)"
                                  : "SET in environment"));

        TaskTrackTunnelProbe probe;
        String error;
        if(TaskTrackTunnelLoadProbe(probe, error))
            probe_label_.SetText(Format("Local probe: #%d   %s", probe.sequence, probe.message));
        else
            probe_label_.SetText("Local probe: none yet");

        if(last_runtime_output_.IsEmpty())
            runtime_label_.SetText("Runtime: not checked");
    }

    void ConnectRuntime()
    {
        RefreshLocalState();
        if(tunnel_id_.IsEmpty()) {
            Exclamation("Tunnel ID is not set. Start with --tunnel-id <tunnel_id> or set TASKTRACK_TUNNEL_ID.");
            return;
        }
        if(!FileExists(client_path_)) {
            Exclamation("tunnel-client.exe was not found.");
            return;
        }
        if(!FileExists(mcp_path_)) {
            Exclamation("TaskTrackMcp.exe must be beside TaskTrackTunnelGui.exe.");
            return;
        }
        if(GetEnv("CONTROL_PLANE_API_KEY").IsEmpty()) {
            Exclamation("CONTROL_PLANE_API_KEY is not set. Create a restricted runtime API key with Tunnels Read + Use, then start this app from that environment.");
            return;
        }

        if(runtime_.IsRunning()) {
            runtime_label_.SetText("Runtime: already running");
            RefreshRuntimeStatus(false);
            return;
        }

        health_url_file_ = GetTempFileName("tasktrack-tunnel-health-");
        SaveFile(health_url_file_, "");
        Vector<String> args;
        args.Add("run");
        args.Add("--control-plane.api-key");
        args.Add("env:CONTROL_PLANE_API_KEY");
        args.Add("--control-plane.tunnel-id");
        args.Add(tunnel_id_);
        args.Add("--mcp.command");
        args.Add(mcp_path_);
        args.Add("--health.url-file");
        args.Add(health_url_file_);
        args.Add("--log.file");
        args.Add("file:" + GetExeDirFile("TaskTrackTunnelRuntime.log"));

        if(!runtime_.Start(~client_path_, args)) {
            runtime_label_.SetText("Runtime: START FAILED");
            Exclamation("Unable to start tunnel-client run.");
            return;
        }
        runtime_label_.SetText("Runtime: starting");
        RefreshRuntimeStatus(false);
    }

    void RefreshRuntimeStatus(bool show_dialog)
    {
        if(!runtime_.IsRunning()) {
            runtime_label_.SetText("Runtime: stopped (exit=" + AsString(runtime_.GetExitCode()) + ")");
            return;
        }
        if(health_url_.IsEmpty() && FileExists(health_url_file_))
            health_url_ = TrimBoth(LoadFile(health_url_file_));
        String summary = "process=true healthy=false ready=false";
        if(!health_url_.IsEmpty()) {
            HttpRequest health(health_url_ + "/healthz");
            health.RequestTimeout(1000);
            String body = health.Execute();
            bool healthy = health.IsSuccess();
            HttpRequest ready(health_url_ + "/readyz");
            ready.RequestTimeout(1000);
            ready.Execute();
            summary = "process=true healthy=" + AsString(healthy) + " ready=" + AsString(ready.IsSuccess());
        }
        runtime_label_.SetText("Runtime: " + summary + "  health=" + health_url_);
        if(show_dialog)
            PromptOK("Tunnel runtime status\n\n" + summary);
    }

    void StopRuntime()
    {
        if(runtime_.IsRunning())
            runtime_.Kill();
        runtime_label_.SetText("Runtime: stopped");
    }

    void SendProbe()
    {
        TaskTrackTunnelProbe probe;
        String error;
        if(!TaskTrackTunnelLoadProbe(probe, error))
            probe = TaskTrackTunnelProbe();

        probe.sequence++;
        probe.updated_at = AsString(GetSysTime());
        probe.source = GetComputerName();
        probe.tunnel_id = tunnel_id_;
        probe.message = Format("TaskTrack local probe #%d from %s", probe.sequence, probe.source);

        if(!TaskTrackTunnelSaveProbe(probe, error)) {
            Exclamation("Unable to save local tunnel probe.\n\n" + error);
            return;
        }

        probe_label_.SetText(Format("Local probe: #%d   %s", probe.sequence, probe.message));
        PromptOK(Format("Probe #%d is ready.\n\nAsk browser ChatGPT to call tunnel_probe.", probe.sequence));
    }

    void OpenTunnelUi()
    {
        if(health_url_.IsEmpty())
            RefreshRuntimeStatus(false);
        if(health_url_.IsEmpty()) {
            Exclamation("The managed runtime did not report a health URL yet. Use Status and confirm the runtime is ready.");
            return;
        }

        String url = health_url_;
        while(url.EndsWith("/"))
            url = url.Left(url.GetCount() - 1);
        LaunchWebBrowser(url + "/ui");
    }
};

String TunnelHelpText()
{
    return
        "TaskTrack Tunnel GUI\n"
        "Supervises the official OpenAI tunnel-client for the local TaskTrack MCP.\n\n"
        "Usage:\n"
        "  TaskTrackTunnelGui.exe --tunnel-id <tunnel_id>\n"
        "  TaskTrackTunnelGui.exe --tunnel-id <tunnel_id> --client <path-to-tunnel-client.exe>\n"
        "  TaskTrackTunnelGui.exe --tunnel-id <tunnel_id> --alias <local-alias>\n\n"
        "Environment:\n"
        "  CONTROL_PLANE_API_KEY   Restricted runtime key with Tunnels Read + Use.\n"
        "  TASKTRACK_TUNNEL_ID     Optional tunnel id when --tunnel-id is omitted.\n\n"
        "The API key is inherited by tunnel-client and is never stored by TaskTrack.";
}

}

GUI_APP_MAIN
{
    String tunnel_id = GetEnv("TASKTRACK_TUNNEL_ID");
    String client_path = GetExeDirFile("tunnel-client.exe");
    String alias = "tasktrack-browser";

    const Vector<String>& cmd = CommandLine();
    for(int i = 0; i < cmd.GetCount(); ++i) {
        if(cmd[i] == "--help") {
            PromptOK(TunnelHelpText());
            return;
        }
        if(cmd[i] == "--version") {
            PromptOK("TaskTrack Tunnel GUI\nTaskTrack build " + TaskTrackBuildVersion());
            return;
        }
        if(cmd[i] == "--tunnel-id" && i + 1 < cmd.GetCount()) {
            tunnel_id = cmd[++i];
            continue;
        }
        if(cmd[i] == "--client" && i + 1 < cmd.GetCount()) {
            client_path = cmd[++i];
            continue;
        }
        if(cmd[i] == "--alias" && i + 1 < cmd.GetCount()) {
            alias = cmd[++i];
            continue;
        }

        PromptOK("Unknown or incomplete arguments.\n\n" + TunnelHelpText());
        return;
    }

    TaskTrackTunnelWindow window(tunnel_id, client_path, alias);
    window.Run();
}
