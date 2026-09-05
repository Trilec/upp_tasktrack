#include <Core/Core.h>
#include <Ui/Ui.h>
#include <TaskTrack/Core/TaskTrackBuild.h>
#include <TaskTrack/TunnelCore/TaskTrackTunnelCore.h>

using namespace Upp;

namespace {

static const int TIMER_REFRESH = 1;

String RememberedTunnelIdPath()
{
    return ConfigFile("tasktrack-tunnel-id.txt");
}

String LoadRememberedTunnelId()
{
    String value = LoadFile(RememberedTunnelIdPath());
    return IsNull(value) ? String() : TrimBoth(value);
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
        SetRect(0, 0, DPI(780), DPI(470));
        SetMinSize(Size(DPI(660), DPI(410)));
        surface_.SetRadius(0);
        BuildUi();
        Wire();
        tunnel_id_edit_.SetTextUtf8(tunnel_id_);
        RefreshLocalState();
        SetTimeCallback(-1000, [=] { Tick(); }, TIMER_REFRESH);
    }

    ~TaskTrackTunnelWindow()
    {
        KillTimeCallback(TIMER_REFRESH);
        if(runtime_started_)
            runtime_process_.Kill();
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
    String status_snapshot_;
    LocalProcess runtime_process_;
    bool runtime_started_ = false;
    bool runtime_healthy_ = false;
    bool runtime_ready_ = false;

    UiPanel surface_;
    UiBoxLayout root_{UiDirection::V};
    UiGroupPanel status_group_;
    UiBoxLayout status_box_{UiDirection::V};
    UiBoxLayout tunnel_row_{UiDirection::H};
    UiLabel tunnel_id_label_;
    UiLineEdit tunnel_id_edit_;
    UiBoxLayout indicator_row_{UiDirection::H};
    UiLabel runtime_indicator_;
    UiLabel activity_indicator_;
    UiLabel status_text_;
    UiBoxLayout actions_{UiDirection::H};
    UiButton connect_button_;
    UiButton status_button_;
    UiButton stop_button_;
    UiButton probe_button_;
    UiButton copy_button_;
    UiButton open_health_button_;
    UiButton close_button_;
    UiLabel note_label_;

    void SetIndicator(UiLabel& label, Color color, const String& text)
    {
        label.ClearSpans()
             .EnableRich(true)
             .AddBulletSpan(color, DPI(8))
             .AddTextSpan("  " + text);
    }

    void BuildUi()
    {
        surface_.Add(root_.SizePos());
        Add(surface_.SizePos());

        root_.SetDirection(UiDirection::V).SetGap(DPI(8)).SetInset(DPI(10));

        status_group_.SetTitle("Secure MCP Tunnel")
                     .SetSubTitle("Official OpenAI tunnel runtime supervising the local TaskTrack MCP");

        status_box_.SetDirection(UiDirection::V).SetGap(DPI(6)).SetInset(DPI(8));

        tunnel_row_.SetDirection(UiDirection::H)
                   .SetGap(DPI(8))
                   .SetInset(0)
                   .SetAlignItems(UiCrossAlign::Center);
        tunnel_id_label_.SetText("Tunnel ID");
        tunnel_id_edit_.SetPlaceholder("tunnel_...");
        tunnel_row_.Add(tunnel_id_label_).Fixed(DPI(72)).MinCross(DPI(28));
        tunnel_row_.Add(tunnel_id_edit_).Expand(1).MinCross(DPI(30));

        indicator_row_.SetDirection(UiDirection::H)
                      .SetGap(DPI(18))
                      .SetInset(0)
                      .SetAlignItems(UiCrossAlign::Center);
        indicator_row_.Add(runtime_indicator_).Fit().MinCross(DPI(26));
        indicator_row_.Add(activity_indicator_).Fit().MinCross(DPI(26));
        indicator_row_.AddSpacer(1).Expand(1).MinMain(DPI(8));

        status_text_.SetSelectable(true);
        status_text_.SetAlign(UiAlign::LEFT, UiAlign::TOP);

        status_box_.Add(tunnel_row_).Fit().MinCross(DPI(32));
        status_box_.Add(indicator_row_).Fit().MinCross(DPI(28));
        status_box_.Add(status_text_).Expand(1).MinCross(DPI(150));
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
        copy_button_.SetText("Copy status").SetContentInset(DPI(4));
        open_health_button_.SetText("Open health").SetContentInset(DPI(4));
        close_button_.SetText("Close window").SetContentInset(DPI(4));

        actions_.Add(connect_button_).Fixed(DPI(88)).MinCross(DPI(30));
        actions_.Add(status_button_).Fixed(DPI(80)).MinCross(DPI(30));
        actions_.Add(stop_button_).Fixed(DPI(70)).MinCross(DPI(30));
        actions_.Add(probe_button_).Fixed(DPI(100)).MinCross(DPI(30));
        actions_.Add(copy_button_).Fixed(DPI(108)).MinCross(DPI(30));
        actions_.Add(open_health_button_).Fixed(DPI(108)).MinCross(DPI(30));
        actions_.AddSpacer(1).Expand(1).MinMain(DPI(8));
        actions_.Add(close_button_).Fixed(DPI(106)).MinCross(DPI(30));

        note_label_.SetText(
            "Tunnel ID is ordinary local configuration and is remembered after Connect. "
            "The Platform API key is read only from CONTROL_PLANE_API_KEY and is never stored. "
            "The green indicator means the runtime is ready; orange activity shows remote MCP traffic.");

        root_.Add(status_group_).Expand(1).MinMain(DPI(285)).AlignSelf(UiCrossAlign::Stretch);
        root_.Add(actions_).Fit().MinMain(DPI(34)).AlignSelf(UiCrossAlign::Stretch);
        root_.Add(note_label_).Fit().MinMain(DPI(54)).AlignSelf(UiCrossAlign::Stretch);
    }

    void Wire()
    {
        tunnel_id_edit_.WhenChange = [=] {
            tunnel_id_ = TrimBoth(tunnel_id_edit_.GetTextUtf8());
            RefreshLocalState();
        };
        tunnel_id_edit_.WhenAction = [=] { ConnectRuntime(); };
        connect_button_.WhenAction = [=] { ConnectRuntime(); };
        status_button_.WhenAction = [=] { RefreshRuntimeStatus(true); };
        stop_button_.WhenAction = [=] { StopRuntime(); };
        probe_button_.WhenAction = [=] { SendProbe(); };
        copy_button_.WhenAction = [=] { CopyStatus(); };
        open_health_button_.WhenAction = [=] { OpenHealth(); };
        close_button_.WhenAction = [=] { Close(); };
    }

    String RuntimeMcpCommand() const
    {
        String command = mcp_path_;
        command.Replace("\\", "/");
        if(command.Find(' ') >= 0 || command.Find('\t') >= 0)
            command = "\"" + command + "\"";
        return command;
    }

    bool LoadHealthUrl()
    {
        if(health_url_file_.IsEmpty() || !FileExists(health_url_file_))
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
        String log = runtime_log_file_.IsEmpty() ? String() : LoadFile(runtime_log_file_);
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

        HttpRequest request(~(health_url_ + suffix));
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

    String RuntimeSummary() const
    {
        if(!runtime_started_)
            return "not started";
        if(runtime_ready_)
            return "process=true  healthy=true  ready=true";
        if(runtime_healthy_)
            return "process=true  healthy=true  ready=false";
        return "process=true  healthy=false  ready=false";
    }

    void RefreshIndicatorsAndStatus()
    {
        Color ready_color = runtime_ready_ ? Color(37, 166, 91)
                          : runtime_started_ ? Color(224, 153, 44)
                                             : SColorDisabled();
        String ready_text = runtime_ready_ ? "Ready"
                          : runtime_started_ ? "Running / waiting"
                                             : "Stopped";
        SetIndicator(runtime_indicator_, ready_color, ready_text);

        TaskTrackTunnelActivity activity;
        String activity_error;
        bool has_activity = TaskTrackTunnelLoadActivity(activity, activity_error);
        int64 received = has_activity ? activity.received : 0;
        int64 sent = has_activity ? activity.sent : 0;
        Color activity_color = (received || sent) ? Color(230, 145, 35) : SColorDisabled();
        SetIndicator(activity_indicator_, activity_color,
                     Format("Remote activity: %lld in / %lld out",
                            (long long)received, (long long)sent));

        TaskTrackTunnelProbe probe;
        String probe_error;
        bool has_probe = TaskTrackTunnelLoadProbe(probe, probe_error);

        String text;
        text << "TaskTrack build: " << TaskTrackBuildVersion() << "\n"
             << "Tunnel: " << (tunnel_id_.IsEmpty() ? String("NOT SET") : tunnel_id_)
             << "   Alias: " << alias_ << "\n"
             << "Tunnel runtime: " << (FileExists(client_path_) ? "FOUND   " : "MISSING   ")
             << client_path_ << "\n"
             << "TaskTrack MCP: " << (FileExists(mcp_path_) ? "FOUND   " : "MISSING   ")
             << mcp_path_ << "\n"
             << "Runtime key: "
             << (GetEnv("CONTROL_PLANE_API_KEY").IsEmpty()
                    ? "NOT SET (set CONTROL_PLANE_API_KEY)"
                    : "SET in environment") << "\n"
             << "Runtime: " << RuntimeSummary() << "\n"
             << Format("Remote activity: %lld received / %lld sent",
                       (long long)received, (long long)sent);

        if(has_activity && (!activity.last_method.IsEmpty() || !activity.updated_at.IsEmpty())) {
            text << "   Last: " << activity.last_method;
            if(!activity.last_tool.IsEmpty())
                text << " / " << activity.last_tool;
            if(!activity.updated_at.IsEmpty())
                text << "   " << activity.updated_at;
        }
        text << "\n";

        if(has_probe)
            text << Format("Local probe: #%d   %s", probe.sequence, probe.message);
        else
            text << "Local probe: none yet";

        status_snapshot_ = text;
        if(status_text_.GetText() != status_snapshot_)
            status_text_.SetText(status_snapshot_);
    }

    void RefreshLocalState()
    {
        RefreshIndicatorsAndStatus();
    }

    void ConnectRuntime()
    {
        tunnel_id_ = TrimBoth(tunnel_id_edit_.GetTextUtf8());
        RefreshLocalState();

        if(tunnel_id_.IsEmpty()) {
            Exclamation("Tunnel ID is not set.");
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

        if(!SaveFile(RememberedTunnelIdPath(), tunnel_id_))
            Exclamation("The tunnel connected configuration could not be remembered locally.");

        runtime_process_.Kill();
        runtime_started_ = false;
        runtime_healthy_ = false;
        runtime_ready_ = false;
        health_url_.Clear();
        last_runtime_output_.Clear();

        health_url_file_ = GetTempFileName("tasktrack-tunnel-health-");
        SaveFile(health_url_file_, "");
        runtime_log_file_ = GetTempFileName("tasktrack-tunnel-runtime-");
        DeleteFile(runtime_log_file_);

        String activity_error;
        TaskTrackTunnelResetActivity(activity_error);

        String old_remote = GetEnv("TASKTRACK_TUNNEL_REMOTE");
        SetEnv("TASKTRACK_TUNNEL_REMOTE", "1");

        Vector<String> args;
        args.Add("run");
        args.Add("--control-plane.api-key");
        args.Add("env:CONTROL_PLANE_API_KEY");
        args.Add("--control-plane.tunnel-id");
        args.Add(tunnel_id_);
        args.Add("--mcp.command");
        args.Add(RuntimeMcpCommand());
        args.Add("--health.listen-addr");
        args.Add("127.0.0.1:0");
        args.Add("--health.url-file");
        args.Add(health_url_file_);
        args.Add("--log.file");
        args.Add(runtime_log_file_);

        bool started = runtime_process_.Start(~client_path_, args);
        SetEnv("TASKTRACK_TUNNEL_REMOTE", old_remote);

        if(!started) {
            RefreshIndicatorsAndStatus();
            Exclamation("Unable to start the official tunnel runtime.");
            return;
        }

        runtime_started_ = true;
        RefreshIndicatorsAndStatus();

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
            String detail = "not started";
            if(runtime_started_) {
                String output;
                int code = runtime_process_.Finish(output);
                last_runtime_output_ << output;
                detail = Format("process exited with code %d", code);
                runtime_process_.Kill();
            }

            runtime_started_ = false;
            runtime_healthy_ = false;
            runtime_ready_ = false;
            RefreshIndicatorsAndStatus();

            if(show_dialog) {
                String message = "Tunnel runtime: " + detail;
                String diagnostics = RuntimeDiagnostics();
                if(!diagnostics.IsEmpty())
                    message << "\n\n" << diagnostics;
                PromptOK(message);
            }
            return;
        }

        LoadHealthUrl();
        int health_status = 0, ready_status = 0;
        String health_error, ready_error;
        runtime_healthy_ = ProbeHealth("/healthz", health_status, health_error);
        runtime_ready_ = ProbeHealth("/readyz", ready_status, ready_error);
        RefreshIndicatorsAndStatus();

        if(show_dialog) {
            String message = RuntimeSummary();
            if(!health_url_.IsEmpty())
                message << "\nHealth: " << health_url_;
            if(!runtime_healthy_ && !health_error.IsEmpty())
                message << "\nhealthz: " << health_error;
            if(!runtime_ready_ && !ready_error.IsEmpty())
                message << "\nreadyz: " << ready_error;
            PromptOK(message);
        }
    }

    void StopRuntime()
    {
        if(runtime_started_)
            runtime_process_.Kill();
        runtime_started_ = false;
        runtime_healthy_ = false;
        runtime_ready_ = false;
        health_url_.Clear();
        RefreshIndicatorsAndStatus();
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

        RefreshIndicatorsAndStatus();
        PromptOK(Format("Probe #%d is ready.\n\nAsk browser ChatGPT to call tunnel_probe.", probe.sequence));
    }

    void CopyStatus()
    {
        RefreshIndicatorsAndStatus();
        WriteClipboardText(status_snapshot_);
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

    void Tick()
    {
        if(runtime_started_)
            RefreshRuntimeStatus(false);
        else
            RefreshIndicatorsAndStatus();
    }
};

String TunnelHelpText()
{
    return
        "TaskTrack Tunnel GUI\n"
        "Supervises the official OpenAI tunnel runtime for the local TaskTrack MCP.\n\n"
        "Usage:\n"
        "  TaskTrackTunnelGui.exe --tunnel-id <tunnel_id>\n"
        "  TaskTrackTunnelGui.exe --tunnel-id <tunnel_id> --client <path-to-tunnel-client.exe>\n"
        "  TaskTrackTunnelGui.exe --tunnel-id <tunnel_id> --alias <local-alias>\n\n"
        "Tunnel ID precedence:\n"
        "  command line -> TASKTRACK_TUNNEL_ID -> remembered local value -> editable GUI field.\n\n"
        "Environment:\n"
        "  CONTROL_PLANE_API_KEY   Platform API key authorized to use the selected tunnel.\n"
        "  TASKTRACK_TUNNEL_ID     Optional tunnel id.\n\n"
        "The tunnel ID may be remembered locally. The API key is inherited by the official runtime and is never stored by TaskTrack.";
}

}

GUI_APP_MAIN
{
    String tunnel_id = GetEnv("TASKTRACK_TUNNEL_ID");
    if(tunnel_id.IsEmpty())
        tunnel_id = LoadRememberedTunnelId();

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
