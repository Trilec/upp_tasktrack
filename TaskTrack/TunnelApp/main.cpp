#include <Ui/Ui.h>
#include <Core/Inet.h>
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
        health_url_file_ = GetExeDirFile("tasktrack-tunnel-health-url.txt");
        runtime_log_file_ = GetExeDirFile("tasktrack-tunnel-runtime.log");
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
    String runtime_log_file_;
    String last_runtime_output_;
    LocalProcess runtime_process_;
    bool runtime_started_ = false;

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
    UiButton open_health_button_;
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
        open_health_button_.SetText("Open health").SetContentInset(DPI(4));
        close_button_.SetText("Close window").SetContentInset(DPI(4));

        actions_.Add(connect_button_).Fixed(DPI(88)).MinCross(DPI(30));
        actions_.Add(status_button_).Fixed(DPI(80)).MinCross(DPI(30));
        actions_.Add(stop_button_).Fixed(DPI(70)).MinCross(DPI(30));
        actions_.Add(probe_button_).Fixed(DPI(100)).MinCross(DPI(30));
        actions_.Add(open_health_button_).Fixed(DPI(118)).MinCross(DPI(30));
        actions_.AddSpacer(1).Expand(1).MinMain(DPI(8));
        actions_.Add(close_button_).Fixed(DPI(106)).MinCross(DPI(30));

        note_label_.SetText(
            "The Platform API key is read only from CONTROL_PLANE_API_KEY and is never stored by "
            "TaskTrack. The official runtime stays active while this window is open. Send probe "
            "changes local diagnostic state; browser ChatGPT reads it through tunnel_probe.");

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
        open_health_button_.WhenAction = [=] { OpenHealth(); };
        close_button_.WhenAction = [=] { Close(); };
    }

    bool LoadHealthUrl()
    {
        if(!FileExists(health_url_file_))
            return false;
        String url = LoadFile(health_url_file_);
        if(IsNull(url))
            return false;
        url = TrimBoth(url);
        if(url.IsEmpty())
            return false;
        while(url.EndsWith("/"))
            url = url.Left(url.GetCount() - 1);
        health_url_ = url;
        return true;
    }

    String RuntimeMcpCommand() const
    {
        String command = mcp_path_;
        command.Replace("\\", "/");
        if(command.Find(' ') >= 0 || command.Find('\t') >= 0)
            command = "\"" + command + "\"";
        return command;
    }

    void DrainRuntimeOutput()
    {
        if(!runtime_started_)
            return;
        for(int i = 0; i < 8; ++i) {
            String out, err;
            runtime_process_.Read2(out, err);
            if(out.IsEmpty() && err.IsEmpty())
                break;
            last_runtime_output_ << out << err;
            if(last_runtime_output_.GetCount() > 4000)
                last_runtime_output_ = last_runtime_output_.Right(4000);
        }
    }

    String RuntimeDiagnostics()
    {
        String out = last_runtime_output_;
        String log = LoadFile(runtime_log_file_);
        if(!IsNull(log) && !log.IsEmpty()) {
            if(log.GetCount() > 2400)
                log = log.Right(2400);
            if(!out.IsEmpty())
                out << "\n";
            out << log;
        }
        return out;
    }

    bool ProbeHealth(const String& suffix, int& status, String& error)
    {
        status = 0;
        error.Clear();
        if(health_url_.IsEmpty() && !LoadHealthUrl()) {
            error = "health URL is not available yet";
            return false;
        }
        String endpoint = health_url_ + suffix;
        HttpRequest request(~endpoint);
        request.Timeout(2000);
        request.Execute();
        status = request.GetStatusCode();
        if(request.IsSuccess())
            return true;
        error = request.GetErrorDesc();
        if(error.IsEmpty())
            error = Format("HTTP %d %s", status, request.GetReasonPhrase());
        return false;
    }

    void RefreshLocalState()
    {
        version_label_.SetText("TaskTrack build: " + TaskTrackBuildVersion());
        tunnel_label_.SetText("Tunnel: " + (tunnel_id_.IsEmpty() ? String("NOT SET") : tunnel_id_)
                              + "   Alias: " + alias_);
        client_label_.SetText(String("Tunnel runtime: ")
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

        if(!runtime_started_)
            runtime_label_.SetText("Runtime: not started");
    }

    void ConnectRuntime()
    {
        RefreshLocalState();
        if(tunnel_id_.IsEmpty()) {
            Exclamation("Tunnel ID is not set. Start with --tunnel-id <tunnel_id> or set TASKTRACK_TUNNEL_ID.");
            return;
        }
        if(!FileExists(client_path_)) {
            Exclamation("The official tunnel runtime executable was not found.");
            return;
        }
        if(!FileExists(mcp_path_)) {
            Exclamation("TaskTrackMcp.exe must be beside TaskTrackTunnelGui.exe.");
            return;
        }
        if(GetEnv("CONTROL_PLANE_API_KEY").IsEmpty()) {
            Exclamation("CONTROL_PLANE_API_KEY is not set. Start this app from an environment containing a Platform API key that is allowed to use this tunnel.");
            return;
        }
        if(runtime_started_ && runtime_process_.IsRunning()) {
            RefreshRuntimeStatus(true);
            return;
        }

        runtime_process_.Kill();
        runtime_started_ = false;
        health_url_.Clear();
        last_runtime_output_.Clear();
        DeleteFile(health_url_file_);
        DeleteFile(runtime_log_file_);

        Vector<String> args;
        args.Add("run");
        args.Add("--control-plane.tunnel-id");
        args.Add(tunnel_id_);
        args.Add("--control-plane.api-key");
        args.Add("env:CONTROL_PLANE_API_KEY");
        args.Add("--mcp.command");
        args.Add(RuntimeMcpCommand());
        args.Add("--health.listen-addr");
        args.Add("127.0.0.1:0");
        args.Add("--health.url-file");
        args.Add(health_url_file_);
        args.Add("--log.file");
        args.Add(runtime_log_file_);

        if(!runtime_process_.Start(~client_path_, args)) {
            runtime_label_.SetText("Runtime: START FAILED");
            Exclamation("Unable to start the official tunnel runtime.");
            return;
        }

        runtime_started_ = true;
        runtime_label_.SetText("Runtime: starting");
        for(int i = 0; i < 40; ++i) {
            DrainRuntimeOutput();
            if(LoadHealthUrl() || !runtime_process_.IsRunning())
                break;
            Sleep(100);
        }
        RefreshRuntimeStatus(false);
    }

    void RefreshRuntimeStatus(bool show_dialog)
    {
        DrainRuntimeOutput();

        bool running = runtime_started_ && runtime_process_.IsRunning();
        if(!running) {
            String detail;
            if(runtime_started_) {
                String output;
                int code = runtime_process_.Finish(output);
                last_runtime_output_ << output;
                detail = Format("process exited with code %d", code);
                runtime_process_.Kill();
                runtime_started_ = false;
            }
            else
                detail = "not started";

            String diagnostics = RuntimeDiagnostics();
            runtime_label_.SetText("Runtime: " + detail);
            if(show_dialog) {
                String message = "Tunnel runtime: " + detail;
                if(!diagnostics.IsEmpty())
                    message << "\n\n" << diagnostics;
                PromptOK(message);
            }
            return;
        }

        LoadHealthUrl();
        int health_status = 0, ready_status = 0;
        String health_error, ready_error;
        bool healthy = ProbeHealth("/healthz", health_status, health_error);
        bool ready = ProbeHealth("/readyz", ready_status, ready_error);

        String summary = Format("process=true  healthy=%s  ready=%s",
                                healthy ? "true" : "false",
                                ready ? "true" : "false");
        runtime_label_.SetText("Runtime: " + summary);

        if(show_dialog) {
            String message = summary;
            if(!health_url_.IsEmpty())
                message << "\nHealth: " << health_url_;
            if(!healthy && !health_error.IsEmpty())
                message << "\nhealthz: " << health_error;
            if(!ready && !ready_error.IsEmpty())
                message << "\nreadyz: " << ready_error;
            PromptOK(message);
        }
    }

    void StopRuntime()
    {
        if(runtime_started_) {
            runtime_process_.Kill();
            runtime_started_ = false;
        }
        health_url_.Clear();
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
        probe.source = "TaskTrackTunnelGui";
        probe.tunnel_id = tunnel_id_;
        probe.message = Format("TaskTrack local probe #%d", probe.sequence);

        if(!TaskTrackTunnelSaveProbe(probe, error)) {
            Exclamation("Unable to save local tunnel probe.\n\n" + error);
            return;
        }

        probe_label_.SetText(Format("Local probe: #%d   %s", probe.sequence, probe.message));
        PromptOK(Format("Probe #%d is ready.\n\nAsk browser ChatGPT to call tunnel_probe.", probe.sequence));
    }

    void OpenHealth()
    {
        if(health_url_.IsEmpty())
            RefreshRuntimeStatus(false);
        if(health_url_.IsEmpty()) {
            Exclamation("The tunnel runtime has not reported its health URL yet. Use Status after Connect.");
            return;
        }
        LaunchWebBrowser(health_url_ + "/readyz");
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
        "  CONTROL_PLANE_API_KEY   Platform API key authorized to use the selected tunnel.\n"
        "  TASKTRACK_TUNNEL_ID     Optional tunnel id when --tunnel-id is omitted.\n\n"
        "The API key is inherited by the official runtime and is never stored by TaskTrack.";
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
