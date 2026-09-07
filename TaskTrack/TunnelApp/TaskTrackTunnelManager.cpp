#include "TaskTrackTunnelManager.h"
#include "TunnelAppIcon.h"

namespace Upp {

namespace {

static const int TIMER_REFRESH = 1;

String BoolText(bool value)
{
    return value ? "true" : "false";
}

String EllipsizeMiddle(const String& text, int keep = 12)
{
    if(text.GetCount() <= keep * 2 + 3)
        return text;
    return text.Left(keep) + "..." + text.Right(keep);
}

class ApiKeyDialog : public TopWindow {
public:
    typedef ApiKeyDialog CLASSNAME;

    ApiKeyDialog()
    {
        Title("Set session tunnel key");
        SetRect(0, 0, DPI(470), DPI(150));
        SetMinSize(Size(DPI(430), DPI(150)));
        Add(message_);
        Add(secret_);
        Add(save_);
        Add(cancel_);

        message_.SetText("The key is kept only in memory for this manager session and is never written to the machine profile.");
        secret_.SetPlaceholder("OpenAI runtime API key");
        secret_.EnableVisibilityIcon(true);
        save_.SetText("Use key");
        cancel_.SetText("Cancel");
        save_.WhenAction = [=] {
            if(TrimBoth(secret_.GetTextUtf8()).IsEmpty()) {
                Exclamation("Enter the runtime API key.");
                return;
            }
            Break(IDOK);
        };
        cancel_.WhenAction = [=] { Break(IDCANCEL); };
    }

    String GetSecret() const { return TrimBoth(secret_.GetTextUtf8()); }

    virtual void Layout() override
    {
        Size sz = GetSize();
        const int pad = DPI(14);
        message_.SetRect(pad, DPI(12), max(0, sz.cx - pad * 2), DPI(36));
        secret_.SetRect(pad, DPI(53), max(0, sz.cx - pad * 2), DPI(32));
        save_.SetRect(max(pad, sz.cx - DPI(190)), DPI(101), DPI(90), DPI(30));
        cancel_.SetRect(max(pad, sz.cx - DPI(92)), DPI(101), DPI(78), DPI(30));
    }

private:
    UiLabel message_;
    UiPasswordEdit secret_;
    UiButton save_, cancel_;
};

int EnabledServiceCount(const McpTunnelProfile& profile)
{
    int count = 0;
    for(const McpTunnelService& service : profile.services)
        if(service.enabled)
            count++;
    return count;
}

String CommandExecutablePath(const String& command)
{
    String value = TrimBoth(command);
    if(value.IsEmpty())
        return String();
    if(value[0] == '"') {
        int end = value.Find('"', 1);
        return end > 1 ? value.Mid(1, end - 1) : String();
    }

    int end = value.GetCount();
    int space = value.Find(' ');
    int tab = value.Find('\t');
    if(space >= 0)
        end = min(end, space);
    if(tab >= 0)
        end = min(end, tab);
    return value.Left(end);
}

}

TaskTrackTunnelManager::TaskTrackTunnelManager(const TaskTrackTunnelManagerOptions& options)
    : options_(options)
{
    Title("MCP Tunnel");
    Icon(TunnelAppIcon(), TunnelAppIcon());
    Sizeable().Zoomable();
    SetRect(0, 0, DPI(920), DPI(610));
    SetMinSize(Size(DPI(780), DPI(610)));

    LoadProfiles();
    EnsureDefaultProfile();

    if(McpTunnelProfile *profile = CurrentProfile()) {
        if(!options_.tunnel_id.IsEmpty())
            profile->tunnel_id = options_.tunnel_id;
        if(!options_.runtime_path.IsEmpty())
            profile->runtime_path = options_.runtime_path;
        if(profile->services.IsEmpty()) {
            McpTunnelService service;
            service.id = "tasktrack";
            service.name = "TaskTrack";
            service.channel = "main";
            service.command = McpTunnelCommandForExecutable(GetExeDirFile("TaskTrackMcp.exe"));
            profile->services.Add(pick(service));
        }
    }

    UiThemeContext context = UiTheme::GetContext();
    context.preset = UiThemePreset::Minimal;
    context.mode = dark_theme_ ? UiThemeMode::Dark : UiThemeMode::Light;
    UiTheme::Set(context);

    BuildUi();
    Wire();
    ApplyTheme();
    RebuildProfileDropdown();
    LoadProfileIntoUi();
    SelectPage(PAGE_OVERVIEW);
    RefreshProjection();

    SetTimeCallback(-1000, [=] { Tick(); }, TIMER_REFRESH);

    const McpTunnelProfile *profile = CurrentProfile();
    String credential_error;
    if(profile && profile->auto_connect && !profile->tunnel_id.IsEmpty()
       && CredentialAvailable(credential_error))
        PostCallback([=] { ConnectRuntime(); });
}

TaskTrackTunnelManager::~TaskTrackTunnelManager()
{
    KillTimeCallback(TIMER_REFRESH);
    SaveProfileFromUi();
    SaveProfiles();
    runtime_.Stop();
}

void TaskTrackTunnelManager::BuildUi()
{
    root_.SetCustomStyle(MakePanelStyle(SurfaceColor(), 0, SurfaceColor()));
    Add(root_.SizePos());

    root_.Add(header_);
    root_.Add(nav_);
    root_.Add(pages_);
    root_.Add(footer_);

    header_.SetTitle("MCP Tunnel")
           .SetSubTitle("Secure ChatGPT ↔ local MCP services")
           .SetMedia(TunnelAppIcon())
           .SetMediaSide(UiAlign::LEFT)
           .SetMediaAlign(UiAlign::CENTER, UiAlign::CENTER)
           .SetMediaReserve(DPI(44))
           .SetMediaMin(DPI(26))
           .SetMediaAutoFit(true)
           .ShowTitleLine(false)
           .SetContentCell(header_actions_);

    header_actions_.SetGap(DPI(5)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    header_actions_.AddSpacer(1).Expand(1);
    theme_button_.SetIcon(ICON_ACTION_DARK_MODE_48()).SetIconSize(DPI(16), DPI(16)).Tip("Toggle light/dark theme");
    help_button_.SetIcon(ICON_DESIGN_HELP_48()).SetIconSize(DPI(16), DPI(16)).Tip("MCP Tunnel help");
    exit_button_.SetIcon(ICON_DESIGN_MODE_OFF_ON_48()).SetIconSize(DPI(16), DPI(16)).Tip("Close");
    header_actions_.Add(help_button_).Fixed(DPI(32));
    header_actions_.Add(theme_button_).Fixed(DPI(32));
    header_actions_.Add(exit_button_).Fixed(DPI(32));

    nav_.Add(overview_button_);
    nav_.Add(setup_button_);
    nav_.Add(services_button_);
    nav_.Add(nav_note_);
    overview_button_.SetText("Overview").SetCheckable();
    setup_button_.SetText("Setup").SetCheckable();
    services_button_.SetText("Services").SetCheckable();
    nav_note_.SetText("Machine tunnel control").SetAlign(UiAlign::RIGHT, UiAlign::CENTER);

    pages_.Add(overview_page_, "overview");
    pages_.Add(setup_page_, "setup");
    pages_.Add(services_page_, "services");

    BuildOverview();
    BuildSetup();
    BuildServices();

    footer_.Add(footer_build_);
    footer_.Add(footer_mcp_);
    footer_.Add(footer_dashboard_);
    footer_.Add(footer_help_);
    footer_.Add(footer_copy_);
    footer_build_.SetText("TaskTrack " + TaskTrackBuildVersion());
    footer_mcp_.SetText("MCP schema 2");
    footer_dashboard_.SetText("Dashboard schema 1");
    footer_help_.SetText("Help");
    footer_copy_.SetText("Copy diagnostics");
}

void TaskTrackTunnelManager::BuildOverview()
{
    overview_page_.Add(hero_);
    overview_page_.Add(status_strip_);
    overview_page_.Add(activity_panel_);

    hero_.Add(beacon_);
    beacon_.Add(beacon_core_.SizePos());
    hero_.Add(state_eyebrow_);
    hero_.Add(state_title_);
    hero_.Add(state_subtitle_);
    hero_.Add(profile_caption_);
    hero_.Add(profile_value_);
    hero_.Add(tunnel_caption_);
    hero_.Add(tunnel_value_);
    hero_.Add(sync_caption_);
    hero_.Add(sync_value_);
    hero_.Add(primary_button_);
    hero_.Add(health_button_);

    state_eyebrow_.SetText("TUNNEL STATE");
    profile_caption_.SetText("Machine");
    tunnel_caption_.SetText("Tunnel");
    sync_caption_.SetText("Last sync");
    primary_button_.SetText("Connect");
    health_button_.SetText("Open health");

    for(int i = 0; i < 4; ++i) {
        status_strip_.Add(status_cell_[i]);
        status_cell_[i].Add(status_caption_[i]);
        status_cell_[i].Add(status_value_[i]);
    }
    status_caption_[0].SetText("MAIN MCP");
    status_caption_[1].SetText("OPENAI TUNNEL");
    status_caption_[2].SetText("TASKTRACK ACTIVITY");
    status_caption_[3].SetText("SERVICES");

    activity_panel_.Add(activity_title_);
    activity_panel_.Add(activity_live_);
    activity_panel_.Add(activity_count_);
    activity_panel_.Add(activity_table_);
    activity_panel_.Add(send_probe_button_);
    activity_panel_.Add(copy_diagnostics_button_);
    activity_panel_.Add(clear_activity_button_);
    activity_panel_.Add(activity_footer_note_);

    activity_title_.SetText("TaskTrack activity");
    activity_live_.EnableRich(true).ClearSpans().AddBulletSpan(OkColor(), DPI(6)).AddTextSpan("  live");
    activity_count_.SetText("Last 6 communications").SetAlign(UiAlign::RIGHT, UiAlign::CENTER);
    send_probe_button_.SetText("Send probe");
    copy_diagnostics_button_.SetText("Copy diagnostics");
    clear_activity_button_.SetText("Clear activity");
    activity_footer_note_.SetText("No TaskTrack remote traffic yet").SetAlign(UiAlign::RIGHT, UiAlign::CENTER);

    activity_table_.SetModel(activity_model_)
                   .ShowRowHeaders(false)
                   .ShowColumnHeaders(false)
                   .SetRowHeight(DPI(31))
                   .SetDefaultColumnWidth(DPI(120));
}

void TaskTrackTunnelManager::BuildSetup()
{
    setup_page_.Add(profile_bar_);
    setup_page_.Add(setup_form_);

    profile_bar_.Add(profile_select_caption_);
    profile_bar_.Add(profile_dropdown_);
    profile_bar_.Add(new_profile_button_);
    profile_bar_.Add(duplicate_profile_button_);
    profile_bar_.Add(delete_profile_button_);

    profile_select_caption_.SetText("Machine profile");
    new_profile_button_.SetText("+ New");
    duplicate_profile_button_.SetText("Duplicate");
    delete_profile_button_.SetText("Delete");

    Ctrl *controls[] = {
        &section_profile_, &section_runtime_, &section_launch_,
        &profile_name_label_, &profile_name_edit_,
        &machine_id_label_, &machine_id_edit_,
        &tunnel_id_label_, &tunnel_id_edit_,
        &credential_label_, &credential_source_dropdown_, &credential_status_,
        &credential_set_button_, &credential_clear_button_, &credential_note_,
        &runtime_path_label_, &runtime_path_edit_, &runtime_browse_button_,
        &auto_connect_label_, &auto_connect_toggle_, &auto_connect_title_, &auto_connect_note_,
        &remember_label_, &remember_toggle_, &remember_title_, &remember_note_
    };
    for(Ctrl *ctrl : controls)
        setup_form_.Add(*ctrl);

    section_profile_.SetText("MACHINE PROFILE");
    section_runtime_.SetText("RUNTIME");
    section_launch_.SetText("LAUNCH BEHAVIOUR");

    profile_name_label_.SetText("Profile name");
    machine_id_label_.SetText("Machine ID");
    tunnel_id_label_.SetText("Tunnel ID");
    credential_label_.SetText("Credential");

    credential_source_dropdown_.UseInternalModel();
    credential_source_dropdown_.Clear();
    credential_source_dropdown_.Add("Session key (memory only)", "session");
    credential_source_dropdown_.Add("Environment variable", "environment");
    credential_set_button_.SetText("Set key");
    credential_clear_button_.SetText("Clear");
    credential_note_.SetText("Testing only: use a session key or CONTROL_PLANE_API_KEY. Durable cross-platform authentication is intentionally not decided yet.");

    runtime_path_label_.SetText("Runtime executable");
    runtime_browse_button_.SetText("Browse");

    auto_connect_label_.SetText("Auto-connect");
    auto_connect_title_.SetText("Auto-connect on launch");
    auto_connect_note_.SetText("Start this machine tunnel and all enabled services when the manager opens.");

    remember_label_.SetText("Remember profile");
    remember_title_.SetText("Remember machine profile");
    remember_note_.SetText("Reopen with the last selected machine profile.");
}

void TaskTrackTunnelManager::BuildServices()
{
    services_page_.Add(service_bar_);
    services_page_.Add(service_form_);

    service_bar_.Add(service_select_caption_);
    service_bar_.Add(service_dropdown_);
    service_bar_.Add(new_service_button_);
    service_bar_.Add(duplicate_service_button_);
    service_bar_.Add(delete_service_button_);

    service_select_caption_.SetText("MCP service");
    new_service_button_.SetText("+ New");
    duplicate_service_button_.SetText("Duplicate");
    delete_service_button_.SetText("Delete");

    Ctrl *controls[] = {
        &section_service_,
        &service_name_label_, &service_name_edit_,
        &service_id_label_, &service_id_edit_,
        &service_channel_label_, &service_channel_edit_,
        &service_command_label_, &service_command_edit_, &service_browse_button_,
        &service_enabled_label_, &service_enabled_toggle_, &service_enabled_title_, &service_enabled_note_
    };
    for(Ctrl *ctrl : controls)
        service_form_.Add(*ctrl);

    section_service_.SetText("SERVICE BINDING");
    service_name_label_.SetText("Display name");
    service_id_label_.SetText("Service ID");
    service_channel_label_.SetText("MCP channel");
    service_command_label_.SetText("MCP command");
    service_browse_button_.SetText("Browse");
    service_enabled_label_.SetText("Enabled");
    service_enabled_title_.SetText("Expose this service");
    service_enabled_note_.SetText("Enabled services are launched under one machine tunnel. Exactly one enabled service must use channel 'main'.");
}

void TaskTrackTunnelManager::Wire()
{
    overview_button_.WhenAction = [=] { SelectPage(PAGE_OVERVIEW); };
    setup_button_.WhenAction = [=] { SelectPage(PAGE_SETUP); };
    services_button_.WhenAction = [=] { SelectPage(PAGE_SERVICES); };
    theme_button_.WhenAction = [=] { ToggleTheme(); };
    help_button_.WhenAction = [=] { ShowHelp(); };
    exit_button_.WhenAction = [=] { Close(); };
    footer_help_.WhenAction = [=] { ShowHelp(); };
    footer_copy_.WhenAction = [=] { CopyDiagnostics(); };

    primary_button_.WhenAction = [=] {
        if(runtime_.IsStarted())
            StopRuntime();
        else
            ConnectRuntime();
    };
    health_button_.WhenAction = [=] { OpenHealth(); };
    send_probe_button_.WhenAction = [=] { SendProbe(); };
    copy_diagnostics_button_.WhenAction = [=] { CopyDiagnostics(); };
    clear_activity_button_.WhenAction = [=] { ClearActivity(); };

    profile_dropdown_.WhenSelectData = [=](const Value& value) {
        if(loading_profile_)
            return;
        SaveServiceFromUi();
        SaveProfileFromUi();
        SelectProfileById(AsString(value));
    };
    new_profile_button_.WhenAction = [=] { NewProfile(); };
    duplicate_profile_button_.WhenAction = [=] { DuplicateProfile(); };
    delete_profile_button_.WhenAction = [=] { DeleteProfile(); };

    profile_name_edit_.WhenChange = [=] { if(!loading_profile_) SaveProfileFromUi(); };
    machine_id_edit_.WhenChange = [=] { if(!loading_profile_) SaveProfileFromUi(); };
    tunnel_id_edit_.WhenChange = [=] { if(!loading_profile_) SaveProfileFromUi(); };
    runtime_path_edit_.WhenChange = [=] { if(!loading_profile_) SaveProfileFromUi(); };
    auto_connect_toggle_.WhenAction = [=] { if(!loading_profile_) SaveProfileFromUi(); };
    remember_toggle_.WhenAction = [=] { if(!loading_profile_) SaveProfileFromUi(); };
    runtime_browse_button_.WhenAction = [=] { BrowseRuntime(); };
    credential_source_dropdown_.WhenSelectData = [=](const Value& value) {
        if(loading_profile_)
            return;
        McpTunnelProfile *profile = CurrentProfile();
        if(!profile)
            return;
        profile->credential_source = McpTunnelCredentialSourceFromId(AsString(value));
        SaveProfiles();
        RefreshCredentialProjection();
        RefreshProjection();
    };
    credential_set_button_.WhenAction = [=] { SetCredential(); };
    credential_clear_button_.WhenAction = [=] { ClearCredential(); };

    service_dropdown_.WhenSelectData = [=](const Value& value) {
        if(loading_service_)
            return;
        SaveServiceFromUi();
        SelectServiceById(AsString(value));
    };
    new_service_button_.WhenAction = [=] { NewService(); };
    duplicate_service_button_.WhenAction = [=] { DuplicateService(); };
    delete_service_button_.WhenAction = [=] { DeleteService(); };
    service_name_edit_.WhenChange = [=] { if(!loading_service_) SaveServiceFromUi(); };
    service_id_edit_.WhenChange = [=] { if(!loading_service_) SaveServiceFromUi(); };
    service_channel_edit_.WhenChange = [=] { if(!loading_service_) SaveServiceFromUi(); };
    service_command_edit_.WhenChange = [=] { if(!loading_service_) SaveServiceFromUi(); };
    service_enabled_toggle_.WhenAction = [=] { if(!loading_service_) SaveServiceFromUi(); };
    service_browse_button_.WhenAction = [=] { BrowseServiceCommand(); };
}

Color TaskTrackTunnelManager::SurfaceColor() const { return dark_theme_ ? Color(29,34,41) : Color(251,252,254); }
Color TaskTrackTunnelManager::SubtleColor() const { return dark_theme_ ? Color(37,44,53) : Color(242,244,248); }
Color TaskTrackTunnelManager::LineColor() const { return dark_theme_ ? Color(52,61,72) : Color(220,225,232); }
Color TaskTrackTunnelManager::TextColor() const { return dark_theme_ ? Color(237,241,245) : Color(41,47,56); }
Color TaskTrackTunnelManager::MutedColor() const { return dark_theme_ ? Color(165,173,184) : Color(114,121,135); }
Color TaskTrackTunnelManager::SoftColor() const { return dark_theme_ ? Color(121,130,142) : Color(150,156,168); }
Color TaskTrackTunnelManager::AccentColor() const { return dark_theme_ ? Color(145,160,186) : Color(101,119,146); }
Color TaskTrackTunnelManager::AccentStrongColor() const { return dark_theme_ ? Color(169,182,204) : Color(81,98,126); }
Color TaskTrackTunnelManager::OkColor() const { return dark_theme_ ? Color(85,189,145) : Color(61,167,125); }
Color TaskTrackTunnelManager::ActivityColor() const { return dark_theme_ ? Color(223,162,76) : Color(207,139,45); }
Color TaskTrackTunnelManager::DangerColor() const { return dark_theme_ ? Color(221,117,123) : Color(196,93,99); }
Color TaskTrackTunnelManager::StoppedColor() const { return dark_theme_ ? Color(139,146,157) : Color(138,144,153); }

UiPanel::Style TaskTrackTunnelManager::MakePanelStyle(Color face, int radius, Color frame) const
{
    UiPanel::Style style = UiTheme::ResolvePanel(UiPanelRole::Surface);
    if(IsNull(frame))
        frame = LineColor();
    for(int i = 0; i < 4; ++i) {
        style.palette.face[i] = UiFill::Solid(face);
        style.palette.frame[i] = frame;
    }
    style.metrics.face_enabled = true;
    style.metrics.frame_enabled = true;
    style.metrics.frame_width = DPI(1);
    style.metrics.radius = DPI(radius);
    style.transparent = false;
    return style;
}

UiLabel::Style TaskTrackTunnelManager::MakeLabelStyle(Color ink, int height, bool bold) const
{
    UiLabel::Style style = UiTheme::ResolveLabel(UiLabelRole::Body);
    for(int i = 0; i < 4; ++i)
        style.palette.ink[i] = ink;
    style.font = SansSerifZ(height);
    if(bold)
        style.font.Bold();
    style.transparent = true;
    return style;
}

UiButton::Style TaskTrackTunnelManager::MakeButtonStyle(UiButtonRole role) const
{
    UiButton::Style style = UiTheme::ResolveButton(role);
    style.metrics.radius = DPI(7);
    style.metrics.frame_width = DPI(1);
    return style;
}

void TaskTrackTunnelManager::ApplyTheme()
{
    UiThemeContext context = UiTheme::GetContext();
    context.preset = UiThemePreset::Minimal;
    context.mode = dark_theme_ ? UiThemeMode::Dark : UiThemeMode::Light;
    UiTheme::Set(context);

    root_.SetCustomStyle(MakePanelStyle(SurfaceColor(), 0, SurfaceColor()));
    overview_page_.SetCustomStyle(MakePanelStyle(SurfaceColor(), 0, SurfaceColor()));
    setup_page_.SetCustomStyle(MakePanelStyle(SurfaceColor(), 0, SurfaceColor()));
    services_page_.SetCustomStyle(MakePanelStyle(SurfaceColor(), 0, SurfaceColor()));
    nav_.SetCustomStyle(MakePanelStyle(SurfaceColor(), 0, LineColor()));
    footer_.SetCustomStyle(MakePanelStyle(SubtleColor(), 0, LineColor()));

    UiTitleCard::Style header_style = UiTheme::ResolveTitleCard(UiRole::Standard);
    header_style.title_line = false;
    header_style.card_line = true;
    header_style.card_line_side = UiAlign::BOTTOM;
    header_style.card_line_length = LARGE;
    header_style.card_line_thickness = DPI(1);
    header_style.card_line_color_enabled = true;
    header_style.card_line_color = LineColor();
    header_style.title_font = SansSerifZ(15).Bold();
    header_style.subtitle_font = SansSerifZ(11);
    header_style.title_color = TextColor();
    header_style.subtitle_color = MutedColor();
    header_style.palette.face[ST_NORMAL] = UiFill::Solid(SurfaceColor());
    header_style.metrics.radius = 0;
    header_style.metrics.content_margin = Rect(DPI(20), DPI(10), DPI(10), DPI(10));
    header_style.media_tint_mono = false;
    header_.SetCustomStyle(header_style);

    ConfigureNavButton(overview_button_);
    ConfigureNavButton(setup_button_);
    ConfigureNavButton(services_button_);
    nav_note_.SetCustomStyle(MakeLabelStyle(SoftColor(), 10));

    Color panel_face = dark_theme_ ? Color(34,40,48) : White();
    hero_.SetCustomStyle(MakePanelStyle(panel_face, 10));
    status_strip_.SetCustomStyle(MakePanelStyle(panel_face, 10));
    activity_panel_.SetCustomStyle(MakePanelStyle(panel_face, 10));
    profile_bar_.SetCustomStyle(MakePanelStyle(panel_face, 10));
    setup_form_.SetCustomStyle(MakePanelStyle(panel_face, 10));
    service_bar_.SetCustomStyle(MakePanelStyle(panel_face, 10));
    service_form_.SetCustomStyle(MakePanelStyle(panel_face, 10));

    state_eyebrow_.SetCustomStyle(MakeLabelStyle(SoftColor(), 9, true));
    state_title_.SetCustomStyle(MakeLabelStyle(TextColor(), 27, true));
    state_subtitle_.SetCustomStyle(MakeLabelStyle(MutedColor(), 12));
    profile_caption_.SetCustomStyle(MakeLabelStyle(MutedColor(), 10));
    tunnel_caption_.SetCustomStyle(MakeLabelStyle(MutedColor(), 10));
    sync_caption_.SetCustomStyle(MakeLabelStyle(MutedColor(), 10));
    profile_value_.SetCustomStyle(MakeLabelStyle(TextColor(), 11, true));
    tunnel_value_.SetCustomStyle(MakeLabelStyle(TextColor(), 10, true));
    sync_value_.SetCustomStyle(MakeLabelStyle(TextColor(), 11, true));

    primary_button_.SetCustomStyle(MakeButtonStyle(UiButtonRole::Accent));
    health_button_.SetCustomStyle(MakeButtonStyle(UiButtonRole::Subtle));

    for(int i = 0; i < 4; ++i) {
        status_cell_[i].SetCustomStyle(MakePanelStyle(panel_face, 0,
                                                      i == 3 ? panel_face : LineColor()));
        status_caption_[i].SetCustomStyle(MakeLabelStyle(SoftColor(), 9, true));
        status_value_[i].SetCustomStyle(MakeLabelStyle(TextColor(), 11, true));
    }

    activity_title_.SetCustomStyle(MakeLabelStyle(TextColor(), 12, true));
    activity_live_.SetCustomStyle(MakeLabelStyle(OkColor(), 10));
    activity_count_.SetCustomStyle(MakeLabelStyle(MutedColor(), 10));
    activity_footer_note_.SetCustomStyle(MakeLabelStyle(SoftColor(), 10));

    UiTable::Style table_style = UiTheme::ResolveTable();
    table_style.show_row_headers = false;
    table_style.show_column_headers = false;
    table_style.show_grid = false;
    table_style.alternate_rows = false;
    table_style.row_height = DPI(31);
    table_style.table_bg = panel_face;
    table_style.alternate_row_bg = panel_face;
    table_style.hover_bg = dark_theme_ ? Color(43,51,61) : Color(246,248,250);
    table_style.cell_ink = TextColor();
    table_style.muted_ink = MutedColor();
    table_style.grid_color = LineColor();
    activity_table_.SetCustomStyle(table_style);

    UiButton::Style small_button = MakeButtonStyle(UiButtonRole::Subtle);
    send_probe_button_.SetCustomStyle(small_button);
    copy_diagnostics_button_.SetCustomStyle(small_button);
    clear_activity_button_.SetCustomStyle(small_button);
    footer_help_.SetCustomStyle(small_button);
    footer_copy_.SetCustomStyle(small_button);
    new_profile_button_.SetCustomStyle(small_button);
    duplicate_profile_button_.SetCustomStyle(small_button);
    delete_profile_button_.SetCustomStyle(MakeButtonStyle(UiButtonRole::Danger));
    runtime_browse_button_.SetCustomStyle(small_button);
    credential_set_button_.SetCustomStyle(MakeButtonStyle(UiButtonRole::Accent));
    credential_clear_button_.SetCustomStyle(small_button);
    new_service_button_.SetCustomStyle(small_button);
    duplicate_service_button_.SetCustomStyle(small_button);
    delete_service_button_.SetCustomStyle(MakeButtonStyle(UiButtonRole::Danger));
    service_browse_button_.SetCustomStyle(small_button);

    section_profile_.SetCustomStyle(MakeLabelStyle(SoftColor(), 9, true));
    section_runtime_.SetCustomStyle(MakeLabelStyle(SoftColor(), 9, true));
    section_launch_.SetCustomStyle(MakeLabelStyle(SoftColor(), 9, true));
    section_service_.SetCustomStyle(MakeLabelStyle(SoftColor(), 9, true));

    UiLabel *form_labels[] = {
        &profile_select_caption_, &profile_name_label_, &machine_id_label_, &tunnel_id_label_,
        &credential_label_, &runtime_path_label_, &auto_connect_label_, &remember_label_,
        &service_select_caption_, &service_name_label_, &service_id_label_, &service_channel_label_,
        &service_command_label_, &service_enabled_label_
    };
    for(UiLabel *label : form_labels)
        label->SetCustomStyle(MakeLabelStyle(MutedColor(), 10));

    auto_connect_title_.SetCustomStyle(MakeLabelStyle(TextColor(), 10, true));
    remember_title_.SetCustomStyle(MakeLabelStyle(TextColor(), 10, true));
    service_enabled_title_.SetCustomStyle(MakeLabelStyle(TextColor(), 10, true));
    auto_connect_note_.SetCustomStyle(MakeLabelStyle(SoftColor(), 9));
    remember_note_.SetCustomStyle(MakeLabelStyle(SoftColor(), 9));
    service_enabled_note_.SetCustomStyle(MakeLabelStyle(SoftColor(), 9));
    credential_note_.SetCustomStyle(MakeLabelStyle(SoftColor(), 9));

    footer_build_.SetCustomStyle(MakeLabelStyle(SoftColor(), 9));
    footer_mcp_.SetCustomStyle(MakeLabelStyle(SoftColor(), 9));
    footer_dashboard_.SetCustomStyle(MakeLabelStyle(SoftColor(), 9));

    UiToolButton::Style utility_style = UiTheme::ResolveToolButton(UiToolButtonRole::Standard);
    utility_style.metrics.radius = DPI(7);
    theme_button_.SetCustomStyle(utility_style);
    help_button_.SetCustomStyle(utility_style);
    exit_button_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Alert));

    RefreshCredentialProjection();
    RefreshProjection();
    RefreshActivity();
    RefreshLayout();
}

void TaskTrackTunnelManager::ConfigureNavButton(UiToolButton& button)
{
    UiToolButton::Style style = UiTheme::ResolveToolButton(UiToolButtonRole::Standard);
    Color selected = dark_theme_ ? Color(43,51,61) : Color(242,244,248);
    style.palette.face[ST_HOT] = UiFill::Solid(selected);
    style.palette.face[ST_PRESSED] = UiFill::Solid(selected);
    style.palette.ink[ST_PRESSED] = TextColor();
    style.underline = false;
    style.underline_width = DPI(1);
    style.underline_offset = 0;
    button.SetCustomStyle(style);
    button.WhenPaintForeground = [this, &button](Draw& w, const Rect& outer,
                                                  const StyledPalette&, const StyledMetrics& metrics,
                                                  const StyledSkin&, StyledState state, bool) {
        Color line;
        if(button.IsChecked())
            line = Color(138, 58, 247);
        else if(state == ST_HOT)
            line = dark_theme_ ? Color(96, 104, 116) : Color(220, 225, 232);
        else
            return;

        int line_thickness = button.IsChecked() ? DPI(2) : DPI(1);
        Font font = metrics.use_text_font ? metrics.text_font : button.GetStyle().font;
        if(IsNull(font))
            font = StdFont();
        Size text_size = GetTextSize(button.GetText(), font);
        int x = max(0, (outer.GetWidth() - text_size.cx) / 2);
        int y = min(outer.bottom - DPI(1), (outer.GetHeight() + text_size.cy) / 2 + DPI(1));
        w.DrawRect(x, y, min(text_size.cx, outer.GetWidth() - x), line_thickness, line);
    };
}

void TaskTrackTunnelManager::ToggleTheme()
{
    dark_theme_ = !dark_theme_;
    Ctrl::SwapDarkLight();
    ApplyTheme();
    SaveProfiles();
}

void TaskTrackTunnelManager::SelectPage(int page)
{
    page = minmax(page, (int)PAGE_OVERVIEW, (int)PAGE_SERVICES);
    pages_.SetActivePage(page);
    overview_button_.SetChecked(page == PAGE_OVERVIEW);
    setup_button_.SetChecked(page == PAGE_SETUP);
    services_button_.SetChecked(page == PAGE_SERVICES);
}

String TaskTrackTunnelManager::ProfileStorePath() const
{
    return ConfigFile("tasktrack-tunnel-profiles.json");
}

void TaskTrackTunnelManager::LoadProfiles()
{
    profiles_.Clear();
    selected_profile_ = -1;
    selected_service_ = -1;

    String json = LoadFile(ProfileStorePath());
    if(IsNull(json) || json.IsEmpty())
        return;

    try {
        Value root = ParseJSON(json);
        if(!root.Is<ValueMap>())
            return;

        int schema_version = IsNull(root["schema_version"]) ? 1 : (int)root["schema_version"];
        dark_theme_ = !IsNull(root["dark_theme"]) && (bool)root["dark_theme"];
        String selected_id = AsString(root["selected_profile"]);
        Value list_value = root["profiles"];
        if(list_value.Is<ValueArray>()) {
            ValueArray list = list_value;
            for(int i = 0; i < list.GetCount(); ++i) {
                McpTunnelProfile profile = McpTunnelProfileFromValue(list[i], schema_version);
                if(profile.id.IsEmpty() || profile.name.IsEmpty())
                    continue;
                if(profile.runtime_path.IsEmpty())
                    profile.runtime_path = GetExeDirFile("tunnel-client.exe");
                if(profile.services.IsEmpty()) {
                    McpTunnelService service;
                    service.id = "tasktrack";
                    service.name = "TaskTrack";
                    service.channel = "main";
                    service.command = McpTunnelCommandForExecutable(GetExeDirFile("TaskTrackMcp.exe"));
                    profile.services.Add(pick(service));
                }
                profiles_.Add(pick(profile));
            }
        }

        for(int i = 0; i < profiles_.GetCount(); ++i)
            if(profiles_[i].id == selected_id)
                selected_profile_ = i;
    }
    catch(CParser::Error) {
        profiles_.Clear();
        selected_profile_ = -1;
        selected_service_ = -1;
    }
}

void TaskTrackTunnelManager::SaveProfiles()
{
    if(profiles_.IsEmpty())
        return;

    ValueMap root;
    root.Add("schema_version", 2);
    root.Add("dark_theme", dark_theme_);

    String selected_id;
    const McpTunnelProfile *profile = CurrentProfile();
    if(profile && profile->remember_profile)
        selected_id = profile->id;
    root.Add("selected_profile", selected_id);

    ValueArray list;
    for(const McpTunnelProfile& item : profiles_)
        list.Add(McpTunnelProfileToValue(item));
    root.Add("profiles", list);
    SaveFile(ProfileStorePath(), AsJSON(root, true));
}

void TaskTrackTunnelManager::EnsureDefaultProfile()
{
    if(profiles_.IsEmpty()) {
        McpTunnelProfile profile;
        profile.id = "local-machine";
        profile.name = "Local machine";
        profile.machine_id = McpTunnelDefaultMachineId();
        profile.runtime_path = options_.runtime_path.IsEmpty()
            ? GetExeDirFile("tunnel-client.exe") : options_.runtime_path;
        profile.tunnel_id = options_.tunnel_id;
        profile.credential_source = MCP_TUNNEL_CREDENTIAL_SESSION;

        McpTunnelService service;
        service.id = "tasktrack";
        service.name = "TaskTrack";
        service.channel = "main";
        service.command = McpTunnelCommandForExecutable(GetExeDirFile("TaskTrackMcp.exe"));
        profile.services.Add(pick(service));

        profiles_.Add(pick(profile));
        selected_profile_ = 0;
    }
    if(selected_profile_ < 0 || selected_profile_ >= profiles_.GetCount())
        selected_profile_ = 0;
    if(McpTunnelProfile *profile = CurrentProfile())
        selected_service_ = profile->services.IsEmpty() ? -1 : 0;
}

McpTunnelProfile* TaskTrackTunnelManager::CurrentProfile()
{
    return selected_profile_ >= 0 && selected_profile_ < profiles_.GetCount()
        ? &profiles_[selected_profile_] : nullptr;
}

const McpTunnelProfile* TaskTrackTunnelManager::CurrentProfile() const
{
    return selected_profile_ >= 0 && selected_profile_ < profiles_.GetCount()
        ? &profiles_[selected_profile_] : nullptr;
}

String TaskTrackTunnelManager::NewProfileId() const
{
    for(int n = 1;; ++n) {
        String id = Format("machine-%d", n);
        bool used = false;
        for(const McpTunnelProfile& profile : profiles_)
            if(profile.id == id) {
                used = true;
                break;
            }
        if(!used)
            return id;
    }
}

void TaskTrackTunnelManager::RebuildProfileDropdown()
{
    loading_profile_ = true;
    profile_dropdown_.UseInternalModel();
    UiListModel& model = profile_dropdown_.Model();
    model.Clear();
    for(const McpTunnelProfile& profile : profiles_)
        model.Add(profile.name, profile.id);
    if(const McpTunnelProfile *profile = CurrentProfile())
        profile_dropdown_.SelectByData(profile->id);
    loading_profile_ = false;
}

void TaskTrackTunnelManager::LoadProfileIntoUi()
{
    const McpTunnelProfile *profile = CurrentProfile();
    if(!profile)
        return;

    loading_profile_ = true;
    profile_name_edit_.SetTextUtf8(profile->name);
    machine_id_edit_.SetTextUtf8(profile->machine_id);
    tunnel_id_edit_.SetTextUtf8(profile->tunnel_id);
    runtime_path_edit_.SetTextUtf8(profile->runtime_path);
    credential_source_dropdown_.SelectByData(McpTunnelCredentialSourceId(profile->credential_source));
    auto_connect_toggle_.SetOn(profile->auto_connect);
    remember_toggle_.SetOn(profile->remember_profile);
    loading_profile_ = false;

    if(selected_service_ < 0 || selected_service_ >= profile->services.GetCount())
        selected_service_ = profile->services.IsEmpty() ? -1 : 0;
    RebuildServiceDropdown();
    LoadServiceIntoUi();
    RefreshCredentialProjection();
    RefreshProjection();
}

void TaskTrackTunnelManager::SaveProfileFromUi()
{
    if(loading_profile_)
        return;
    McpTunnelProfile *profile = CurrentProfile();
    if(!profile)
        return;

    profile->name = TrimBoth(profile_name_edit_.GetTextUtf8());
    if(profile->name.IsEmpty())
        profile->name = "Unnamed machine";
    profile->machine_id = TrimBoth(machine_id_edit_.GetTextUtf8());
    if(profile->machine_id.IsEmpty())
        profile->machine_id = McpTunnelDefaultMachineId();
    profile->tunnel_id = TrimBoth(tunnel_id_edit_.GetTextUtf8());
    profile->runtime_path = TrimBoth(runtime_path_edit_.GetTextUtf8());
    session_credentials_.InvalidateChangedBinding(*profile);
    profile->auto_connect = auto_connect_toggle_.IsOn();
    profile->remember_profile = remember_toggle_.IsOn();
    SaveProfiles();
    RebuildProfileDropdown();
    RefreshProjection();
}

void TaskTrackTunnelManager::SelectProfileById(const String& id)
{
    for(int i = 0; i < profiles_.GetCount(); ++i)
        if(profiles_[i].id == id) {
            selected_profile_ = i;
            selected_service_ = profiles_[i].services.IsEmpty() ? -1 : 0;
            LoadProfileIntoUi();
            SaveProfiles();
            return;
        }
}

void TaskTrackTunnelManager::NewProfile()
{
    if(runtime_.IsStarted()) {
        Exclamation("Stop the current tunnel before changing profiles.");
        return;
    }

    SaveServiceFromUi();
    SaveProfileFromUi();

    McpTunnelProfile profile;
    profile.id = NewProfileId();
    profile.name = "New machine profile";
    profile.machine_id = McpTunnelDefaultMachineId();
    profile.runtime_path = GetExeDirFile("tunnel-client.exe");
    profile.credential_source = MCP_TUNNEL_CREDENTIAL_SESSION;

    McpTunnelService service;
    service.id = "tasktrack";
    service.name = "TaskTrack";
    service.channel = "main";
    service.command = McpTunnelCommandForExecutable(GetExeDirFile("TaskTrackMcp.exe"));
    profile.services.Add(pick(service));

    profiles_.Add(pick(profile));
    selected_profile_ = profiles_.GetCount() - 1;
    selected_service_ = 0;
    RebuildProfileDropdown();
    LoadProfileIntoUi();
    SaveProfiles();
}

void TaskTrackTunnelManager::DuplicateProfile()
{
    if(runtime_.IsStarted()) {
        Exclamation("Stop the current tunnel before changing profiles.");
        return;
    }

    SaveServiceFromUi();
    SaveProfileFromUi();
    const McpTunnelProfile *source = CurrentProfile();
    if(!source)
        return;

    String id = NewProfileId();
    McpTunnelProfile profile = McpTunnelDuplicateProfile(*source, id, source->name + " copy");
    profiles_.Add(pick(profile));
    selected_profile_ = profiles_.GetCount() - 1;
    selected_service_ = profiles_[selected_profile_].services.IsEmpty() ? -1 : 0;
    RebuildProfileDropdown();
    LoadProfileIntoUi();
    SaveProfiles();
}

void TaskTrackTunnelManager::DeleteProfile()
{
    if(runtime_.IsStarted()) {
        Exclamation("Stop the current tunnel before changing profiles.");
        return;
    }
    if(profiles_.GetCount() <= 1) {
        Exclamation("At least one machine profile must remain.");
        return;
    }
    if(!PromptYesNo("Delete the selected machine profile?"))
        return;

    session_credentials_.Clear(profiles_[selected_profile_]);
    profiles_.Remove(selected_profile_);
    selected_profile_ = min(selected_profile_, profiles_.GetCount() - 1);
    selected_service_ = profiles_[selected_profile_].services.IsEmpty() ? -1 : 0;
    RebuildProfileDropdown();
    LoadProfileIntoUi();
    SaveProfiles();
}

McpTunnelService* TaskTrackTunnelManager::CurrentService()
{
    McpTunnelProfile *profile = CurrentProfile();
    return profile && selected_service_ >= 0 && selected_service_ < profile->services.GetCount()
        ? &profile->services[selected_service_] : nullptr;
}

const McpTunnelService* TaskTrackTunnelManager::CurrentService() const
{
    const McpTunnelProfile *profile = CurrentProfile();
    return profile && selected_service_ >= 0 && selected_service_ < profile->services.GetCount()
        ? &profile->services[selected_service_] : nullptr;
}

String TaskTrackTunnelManager::NewServiceId(const String& base) const
{
    const McpTunnelProfile *profile = CurrentProfile();
    if(!profile)
        return base;
    for(int n = 1;; ++n) {
        String id = n == 1 ? base : Format("%s-%d", base, n);
        bool used = false;
        for(const McpTunnelService& service : profile->services)
            if(service.id == id) {
                used = true;
                break;
            }
        if(!used)
            return id;
    }
}

String TaskTrackTunnelManager::NewServiceChannel(const String& base) const
{
    const McpTunnelProfile *profile = CurrentProfile();
    if(!profile)
        return base;
    for(int n = 1;; ++n) {
        String channel = n == 1 ? base : Format("%s-%d", base, n);
        bool used = false;
        for(const McpTunnelService& service : profile->services)
            if(service.channel == channel) {
                used = true;
                break;
            }
        if(!used)
            return channel;
    }
}

void TaskTrackTunnelManager::RebuildServiceDropdown()
{
    loading_service_ = true;
    service_dropdown_.UseInternalModel();
    UiListModel& model = service_dropdown_.Model();
    model.Clear();
    const McpTunnelProfile *profile = CurrentProfile();
    if(profile) {
        for(const McpTunnelService& service : profile->services) {
            String label = service.name;
            if(!service.enabled)
                label << " (disabled)";
            model.Add(label, service.id);
        }
    }
    if(const McpTunnelService *service = CurrentService())
        service_dropdown_.SelectByData(service->id);
    loading_service_ = false;
}

void TaskTrackTunnelManager::LoadServiceIntoUi()
{
    const McpTunnelService *service = CurrentService();
    loading_service_ = true;
    if(service) {
        service_name_edit_.SetTextUtf8(service->name);
        service_id_edit_.SetTextUtf8(service->id);
        service_channel_edit_.SetTextUtf8(service->channel);
        service_command_edit_.SetTextUtf8(service->command);
        service_enabled_toggle_.SetOn(service->enabled);
    }
    else {
        service_name_edit_.SetTextUtf8("");
        service_id_edit_.SetTextUtf8("");
        service_channel_edit_.SetTextUtf8("");
        service_command_edit_.SetTextUtf8("");
        service_enabled_toggle_.SetOn(false);
    }
    loading_service_ = false;
}

void TaskTrackTunnelManager::SaveServiceFromUi()
{
    if(loading_service_)
        return;
    McpTunnelService *service = CurrentService();
    if(!service)
        return;

    String original_id = service->id;
    service->name = TrimBoth(service_name_edit_.GetTextUtf8());
    if(service->name.IsEmpty())
        service->name = "Unnamed service";
    service->id = TrimBoth(service_id_edit_.GetTextUtf8());
    if(service->id.IsEmpty())
        service->id = original_id;
    service->channel = TrimBoth(service_channel_edit_.GetTextUtf8());
    service->command = TrimBoth(service_command_edit_.GetTextUtf8());
    service->enabled = service_enabled_toggle_.IsOn();

    SaveProfiles();
    RebuildServiceDropdown();
    RefreshProjection();
}

void TaskTrackTunnelManager::SelectServiceById(const String& id)
{
    McpTunnelProfile *profile = CurrentProfile();
    if(!profile)
        return;
    for(int i = 0; i < profile->services.GetCount(); ++i)
        if(profile->services[i].id == id) {
            selected_service_ = i;
            LoadServiceIntoUi();
            return;
        }
}

void TaskTrackTunnelManager::NewService()
{
    if(runtime_.IsStarted()) {
        Exclamation("Stop the current tunnel before changing services.");
        return;
    }
    McpTunnelProfile *profile = CurrentProfile();
    if(!profile)
        return;

    SaveServiceFromUi();
    McpTunnelService service;
    service.id = NewServiceId("service");
    service.name = "New MCP service";
    service.channel = NewServiceChannel("service");
    service.enabled = false;
    profile->services.Add(pick(service));
    selected_service_ = profile->services.GetCount() - 1;
    RebuildServiceDropdown();
    LoadServiceIntoUi();
    SaveProfiles();
}

void TaskTrackTunnelManager::DuplicateService()
{
    if(runtime_.IsStarted()) {
        Exclamation("Stop the current tunnel before changing services.");
        return;
    }
    McpTunnelProfile *profile = CurrentProfile();
    const McpTunnelService *source = CurrentService();
    if(!profile || !source)
        return;

    SaveServiceFromUi();
    source = CurrentService();
    McpTunnelService service;
    service.id = NewServiceId(source->id + "-copy");
    service.name = source->name + " copy";
    service.channel = NewServiceChannel(source->channel + "-copy");
    service.command = source->command;
    service.enabled = false;
    profile->services.Add(pick(service));
    selected_service_ = profile->services.GetCount() - 1;
    RebuildServiceDropdown();
    LoadServiceIntoUi();
    SaveProfiles();
}

void TaskTrackTunnelManager::DeleteService()
{
    if(runtime_.IsStarted()) {
        Exclamation("Stop the current tunnel before changing services.");
        return;
    }
    McpTunnelProfile *profile = CurrentProfile();
    if(!profile || selected_service_ < 0)
        return;
    if(profile->services.GetCount() <= 1) {
        Exclamation("At least one MCP service must remain.");
        return;
    }
    if(!PromptYesNo("Delete the selected MCP service binding?"))
        return;

    bool was_main = profile->services[selected_service_].channel == "main";
    profile->services.Remove(selected_service_);
    selected_service_ = min(selected_service_, profile->services.GetCount() - 1);
    if(was_main && selected_service_ >= 0) {
        profile->services[selected_service_].channel = "main";
        profile->services[selected_service_].enabled = true;
    }
    RebuildServiceDropdown();
    LoadServiceIntoUi();
    SaveProfiles();
    RefreshProjection();
}

void TaskTrackTunnelManager::BrowseRuntime()
{
    FileSel selector;
    selector.Type("Executable", "*.exe");
    String current = runtime_path_edit_.GetTextUtf8();
    if(!current.IsEmpty())
        selector.Set(current);
    if(selector.ExecuteOpen("Choose OpenAI tunnel runtime")) {
        runtime_path_edit_.SetTextUtf8(~selector);
        SaveProfileFromUi();
    }
}

void TaskTrackTunnelManager::BrowseServiceCommand()
{
    FileSel selector;
    selector.Type("Executable", "*.exe");
    String current = TrimBoth(service_command_edit_.GetTextUtf8());
    if(current.GetCount() >= 2 && current[0] == '"' && current[current.GetCount() - 1] == '"')
        current = current.Mid(1, current.GetCount() - 2);
    if(!current.IsEmpty())
        selector.Set(current);
    if(selector.ExecuteOpen("Choose MCP server executable")) {
        service_command_edit_.SetTextUtf8(McpTunnelCommandForExecutable(~selector));
        SaveServiceFromUi();
    }
}

bool TaskTrackTunnelManager::CredentialAvailable(String& error) const
{
    error.Clear();
    const McpTunnelProfile *profile = CurrentProfile();
    if(!profile) {
        error = "No machine profile is selected.";
        return false;
    }

    if(profile->credential_source == MCP_TUNNEL_CREDENTIAL_ENVIRONMENT) {
        if(GetEnv("CONTROL_PLANE_API_KEY").IsEmpty()) {
            error = "CONTROL_PLANE_API_KEY is not set.";
            return false;
        }
        return true;
    }

    if(!session_credentials_.Contains(*profile)) {
        error = "No session key is bound to this profile, tunnel and runtime.";
        return false;
    }
    return true;
}

bool TaskTrackTunnelManager::ReadCredential(String& secret, String& error) const
{
    secret.Clear();
    if(!CredentialAvailable(error))
        return false;

    const McpTunnelProfile *profile = CurrentProfile();
    if(profile->credential_source == MCP_TUNNEL_CREDENTIAL_ENVIRONMENT)
        secret = GetEnv("CONTROL_PLANE_API_KEY");
    else
        secret = session_credentials_.Read(*profile);

    if(secret.IsEmpty()) {
        error = "Tunnel credential is empty.";
        return false;
    }
    return true;
}

void TaskTrackTunnelManager::RefreshCredentialProjection()
{
    const McpTunnelProfile *profile = CurrentProfile();
    if(!profile)
        return;

    loading_profile_ = true;
    credential_source_dropdown_.SelectByData(McpTunnelCredentialSourceId(profile->credential_source));
    loading_profile_ = false;

    String error;
    bool available = CredentialAvailable(error);
    credential_status_.ClearSpans().EnableRich(true)
                      .AddBulletSpan(available ? OkColor() : DangerColor(), DPI(7))
                      .AddTextSpan(available ? "  Available" : "  Not set",
                                   available ? OkColor() : DangerColor(), true);

    bool session_source = profile->credential_source == MCP_TUNNEL_CREDENTIAL_SESSION;
    credential_set_button_.Enable(session_source && !runtime_.IsStarted());
    credential_clear_button_.Enable(session_source && available && !runtime_.IsStarted());
    credential_note_.SetText(session_source
        ? "Session key is memory-only and disappears when this manager closes. It is sufficient for tunnel validation, not the final security design."
        : "Testing/automation mode: CONTROL_PLANE_API_KEY is read at launch. Durable cross-platform authentication remains deliberately undecided.");
}

void TaskTrackTunnelManager::SetCredential()
{
    McpTunnelProfile *profile = CurrentProfile();
    if(!profile || profile->credential_source != MCP_TUNNEL_CREDENTIAL_SESSION)
        return;

    ApiKeyDialog dialog;
    if(dialog.Run() != IDOK)
        return;

    if(!session_credentials_.Set(*profile, dialog.GetSecret())) {
        Exclamation("Set the machine ID, tunnel ID and runtime path before setting its key.");
        return;
    }
    SaveProfiles();
    RefreshCredentialProjection();
    RefreshProjection();
}

void TaskTrackTunnelManager::ClearCredential()
{
    McpTunnelProfile *profile = CurrentProfile();
    if(!profile || profile->credential_source != MCP_TUNNEL_CREDENTIAL_SESSION)
        return;

    session_credentials_.Clear(*profile);
    SaveProfiles();
    RefreshCredentialProjection();
    RefreshProjection();
}

void TaskTrackTunnelManager::ConnectRuntime()
{
    SaveServiceFromUi();
    SaveProfileFromUi();
    const McpTunnelProfile *profile = CurrentProfile();
    if(!profile)
        return;

    if(runtime_.IsStarted()) {
        RefreshRuntimeStatus(true);
        return;
    }

    String validation_error;
    if(!McpTunnelValidateProfile(*profile, validation_error)) {
        Exclamation(validation_error);
        RefreshProjection();
        return;
    }

    const McpTunnelService *tasktrack = TaskTrackService();
    if(tasktrack && tasktrack->enabled && !tasktrack->command.IsEmpty()
       && tasktrack->command.Find(' ') < 0 && !FileExists(tasktrack->command)) {
        Exclamation("TaskTrackMcp.exe was not found at the configured service command.");
        return;
    }

    RefreshMcpBinaryIdentity(true);
    if(tasktrack && tasktrack->enabled &&
       (mcp_binary_identity_ == MCP_BINARY_DIFFERENT ||
        mcp_binary_identity_ == MCP_BINARY_UNVERIFIED)) {
        String warning = mcp_binary_identity_ == MCP_BINARY_DIFFERENT
            ? String("The configured TaskTrack MCP does not match this verified staged bundle.")
            : String("The configured TaskTrack MCP could not be verified against a staged bundle manifest.");
        warning << "\n\n" << McpBinaryIdentityText()
                << "\n\nYou can continue, but this connection should not count as current-version acceptance.";
        if(!PromptYesNo(warning + "\n\nContinue anyway?"))
            return;
    }

    String secret, credential_error;
    if(!ReadCredential(secret, credential_error)) {
        Exclamation(credential_error);
        RefreshCredentialProjection();
        return;
    }

    String activity_error;
    TaskTrackTunnelResetActivity(activity_error);

    bool started = runtime_.Start(*profile, secret);
    secret.Clear();
    RefreshProjection();

    if(!started) {
        String message = runtime_.GetLastError();
        String diagnostics = runtime_.GetDiagnostics();
        if(!diagnostics.IsEmpty())
            message << "\n\n" << diagnostics;
        Exclamation(message);
    }
}

void TaskTrackTunnelManager::RefreshRuntimeStatus(bool show_dialog)
{
    runtime_.Refresh();
    RefreshProjection();
    if(show_dialog) {
        String message = BuildDiagnostics();
        String diagnostics = runtime_.GetDiagnostics();
        if(!diagnostics.IsEmpty())
            message << "\n\nRuntime output:\n" << diagnostics;
        PromptOK(message);
    }
}

void TaskTrackTunnelManager::StopRuntime()
{
    runtime_.Stop();
    RefreshProjection();
}

void TaskTrackTunnelManager::OpenHealth()
{
    if(runtime_.GetHealthUrl().IsEmpty())
        RefreshRuntimeStatus(false);
    if(runtime_.GetHealthUrl().IsEmpty()) {
        Exclamation("The tunnel runtime has not reported its health URL yet.");
        return;
    }
    LaunchWebBrowser(runtime_.GetHealthUrl() + "/readyz");
}

const McpTunnelService* TaskTrackTunnelManager::TaskTrackService() const
{
    const McpTunnelProfile *profile = CurrentProfile();
    if(!profile)
        return nullptr;
    for(const McpTunnelService& service : profile->services)
        if(service.id == "tasktrack")
            return &service;
    return nullptr;
}

void TaskTrackTunnelManager::RefreshMcpBinaryIdentity(bool force)
{
    const McpTunnelService *service = TaskTrackService();
    String command = service ? service->command : String();
    if(!force && command == mcp_binary_command_ && mcp_binary_identity_ != MCP_BINARY_UNKNOWN)
        return;

    mcp_binary_command_ = command;
    mcp_binary_path_ = CommandExecutablePath(command);
    mcp_binary_actual_hash_.Clear();
    mcp_binary_expected_hash_.Clear();
    mcp_binary_bundle_build_.Clear();
    mcp_binary_source_commit_.Clear();

    if(mcp_binary_path_.IsEmpty() || !FileExists(mcp_binary_path_)) {
        mcp_binary_identity_ = MCP_BINARY_MISSING;
        return;
    }

    String image = LoadFile(mcp_binary_path_);
    if(IsNull(image)) {
        mcp_binary_identity_ = MCP_BINARY_UNVERIFIED;
        return;
    }
    mcp_binary_actual_hash_ = SHA256String(image);

    String manifest_json = LoadFile(GetExeDirFile("manifest.json"));
    if(IsNull(manifest_json) || manifest_json.IsEmpty()) {
        mcp_binary_identity_ = MCP_BINARY_UNVERIFIED;
        return;
    }

    try {
        Value root = ParseJSON(manifest_json);
        if(!root.Is<ValueMap>()) {
            mcp_binary_identity_ = MCP_BINARY_UNVERIFIED;
            return;
        }

        mcp_binary_bundle_build_ = AsString(root["tasktrack_build"]);
        mcp_binary_source_commit_ = AsString(root["source_commit"]);
        Value files_value = root["files"];
        if(files_value.Is<ValueArray>()) {
            ValueArray files = files_value;
            for(int i = 0; i < files.GetCount(); ++i) {
                Value item = files[i];
                if(item.Is<ValueMap>() && AsString(item["name"]) == "TaskTrackMcp.exe") {
                    mcp_binary_expected_hash_ = ToLower(AsString(item["sha256"]));
                    break;
                }
            }
        }
    }
    catch(CParser::Error) {
        mcp_binary_identity_ = MCP_BINARY_UNVERIFIED;
        return;
    }

    if(mcp_binary_expected_hash_.IsEmpty()) {
        mcp_binary_identity_ = MCP_BINARY_UNVERIFIED;
        return;
    }

    bool hash_matches = ToLower(mcp_binary_actual_hash_) == mcp_binary_expected_hash_;
    bool build_matches = mcp_binary_bundle_build_.IsEmpty() ||
                         mcp_binary_bundle_build_ == TaskTrackBuildVersion();
    mcp_binary_identity_ = hash_matches && build_matches
        ? MCP_BINARY_CURRENT : MCP_BINARY_DIFFERENT;
}

String TaskTrackTunnelManager::McpBinaryIdentityText() const
{
    switch(mcp_binary_identity_) {
    case MCP_BINARY_CURRENT:
        return "TaskTrack MCP is the verified staged binary (" + mcp_binary_bundle_build_ + ").";
    case MCP_BINARY_DIFFERENT:
        return "TaskTrack MCP differs from the verified staged binary.";
    case MCP_BINARY_UNVERIFIED:
        return "TaskTrack MCP is present but no matching staged identity is available.";
    case MCP_BINARY_MISSING:
        return "TaskTrack MCP executable is missing.";
    default:
        return "TaskTrack MCP identity has not been checked.";
    }
}

void TaskTrackTunnelManager::RefreshProjection()
{
    McpTunnelRuntime::State state = runtime_.GetState();
    const McpTunnelProfile *profile = CurrentProfile();
    RefreshMcpBinaryIdentity(false);

    Color state_color = StoppedColor();
    Color beacon_face = SubtleColor();
    String state_title = "STOPPED";
    String state_subtitle = "Tunnel is not connected";
    String primary_text = "Connect";

    if(state == McpTunnelRuntime::READY) {
        state_color = OkColor();
        beacon_face = dark_theme_ ? Color(32,55,47) : Color(237,249,244);
        state_title = "READY";
        state_subtitle = "Machine tunnel connected";
        primary_text = "Stop";
    }
    else if(state == McpTunnelRuntime::CONNECTING) {
        state_color = ActivityColor();
        beacon_face = dark_theme_ ? Color(58,48,32) : Color(253,246,233);
        state_title = "CONNECTING...";
        state_subtitle = "Opening secure tunnel";
        primary_text = "Cancel";
    }
    else if(state == McpTunnelRuntime::FAULT) {
        state_color = DangerColor();
        beacon_face = dark_theme_ ? Color(60,39,43) : Color(255,241,242);
        state_title = "ERROR";
        state_subtitle = runtime_.GetLastError().IsEmpty()
            ? String("Tunnel could not be established") : runtime_.GetLastError();
        primary_text = runtime_.IsStarted() ? "Stop" : "Retry";
    }

    beacon_.SetCustomStyle(MakePanelStyle(beacon_face, 11, Blend(state_color, LineColor(), 130)));
    beacon_core_.ClearSpans().EnableRich(true).AddBulletSpan(state_color, DPI(14));
    beacon_core_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);

    state_title_.SetText(state_title);
    state_title_.SetCustomStyle(MakeLabelStyle(state == McpTunnelRuntime::READY ? TextColor() : state_color, 27, true));
    state_subtitle_.SetText(state_subtitle);
    primary_button_.SetText(primary_text);

    if(profile) {
        profile_value_.SetText(profile->machine_id.IsEmpty() ? profile->name : profile->machine_id);
        tunnel_value_.SetText(profile->tunnel_id.IsEmpty() ? String("Not configured") : EllipsizeMiddle(profile->tunnel_id, 10));
    }
    else {
        profile_value_.SetText("No machine");
        tunnel_value_.SetText("Not configured");
    }

    TaskTrackTunnelActivity activity;
    String activity_error;
    bool has_activity = TaskTrackTunnelLoadActivity(activity, activity_error);
    sync_value_.SetText(has_activity && !activity.updated_at.IsEmpty() ? activity.updated_at : String("—"));

    const McpTunnelService *main_service = nullptr;
    if(profile)
        for(const McpTunnelService& service : profile->services)
            if(service.enabled && service.channel == "main") {
                main_service = &service;
                break;
            }

    bool main_configured = main_service && !main_service->command.IsEmpty();
    Color main_color = main_configured ? OkColor() : DangerColor();
    String main_text = main_configured ? main_service->name : String("Missing");
    const McpTunnelService *tasktrack_service = TaskTrackService();
    if(main_configured && tasktrack_service == main_service) {
        if(mcp_binary_identity_ == MCP_BINARY_CURRENT)
            main_text << "  • current";
        else if(mcp_binary_identity_ == MCP_BINARY_DIFFERENT) {
            main_text << "  • differs";
            main_color = ActivityColor();
        }
        else if(mcp_binary_identity_ == MCP_BINARY_UNVERIFIED) {
            main_text << "  • unverified";
            main_color = ActivityColor();
        }
        else if(mcp_binary_identity_ == MCP_BINARY_MISSING) {
            main_text << "  • missing";
            main_color = DangerColor();
        }
    }
    status_value_[0].ClearSpans().EnableRich(true)
                    .AddBulletSpan(main_color, DPI(7))
                    .AddTextSpan("  " + main_text, TextColor(), true);

    Color tunnel_color = state == McpTunnelRuntime::READY ? OkColor()
                       : state == McpTunnelRuntime::CONNECTING ? ActivityColor()
                       : state == McpTunnelRuntime::FAULT ? DangerColor()
                                                         : StoppedColor();
    String tunnel_text = state == McpTunnelRuntime::READY ? "Healthy"
                       : state == McpTunnelRuntime::CONNECTING ? "Connecting"
                       : state == McpTunnelRuntime::FAULT ? "Fault"
                                                         : "Stopped";
    status_value_[1].ClearSpans().EnableRich(true)
                    .AddBulletSpan(tunnel_color, DPI(7))
                    .AddTextSpan("  " + tunnel_text, TextColor(), true);

    int64 received = has_activity ? activity.received : 0;
    int64 sent = has_activity ? activity.sent : 0;
    status_value_[2].ClearSpans().EnableRich(true)
                    .AddBulletSpan((received || sent) ? ActivityColor() : StoppedColor(), DPI(7))
                    .AddTextSpan(Format("  %lld in / %lld out", (long long)received, (long long)sent), TextColor(), true);

    int enabled_services = profile ? EnabledServiceCount(*profile) : 0;
    status_value_[3].ClearSpans().EnableRich(true)
                    .AddBulletSpan(enabled_services ? OkColor() : DangerColor(), DPI(7))
                    .AddTextSpan(Format("  %d enabled", enabled_services), TextColor(), true);

    activity_live_.Show(runtime_.IsStarted());
    activity_footer_note_.SetText(state == McpTunnelRuntime::READY ? "TaskTrack traffic visible when its service is used"
                                : state == McpTunnelRuntime::CONNECTING ? "Connecting to control plane"
                                : state == McpTunnelRuntime::FAULT ? "Last connection attempt failed"
                                                                  : "No TaskTrack remote traffic while stopped");

    bool can_edit = !runtime_.IsStarted();
    profile_dropdown_.Enable(can_edit);
    new_profile_button_.Enable(can_edit);
    duplicate_profile_button_.Enable(can_edit);
    delete_profile_button_.Enable(can_edit && profiles_.GetCount() > 1);
    profile_name_edit_.Enable(can_edit);
    machine_id_edit_.Enable(can_edit);
    tunnel_id_edit_.Enable(can_edit);
    credential_source_dropdown_.Enable(can_edit);
    runtime_path_edit_.Enable(can_edit);
    runtime_browse_button_.Enable(can_edit);
    auto_connect_toggle_.Enable(can_edit);
    remember_toggle_.Enable(can_edit);

    service_dropdown_.Enable(can_edit);
    new_service_button_.Enable(can_edit);
    duplicate_service_button_.Enable(can_edit && CurrentService());
    delete_service_button_.Enable(can_edit && profile && profile->services.GetCount() > 1);
    service_name_edit_.Enable(can_edit && CurrentService());
    service_id_edit_.Enable(can_edit && CurrentService());
    service_channel_edit_.Enable(can_edit && CurrentService());
    service_command_edit_.Enable(can_edit && CurrentService());
    service_browse_button_.Enable(can_edit && CurrentService());
    service_enabled_toggle_.Enable(can_edit && CurrentService());

    health_button_.Enable(runtime_.IsStarted() && !runtime_.GetHealthUrl().IsEmpty());
    RefreshCredentialProjection();
    RefreshActivity();
}

void TaskTrackTunnelManager::RefreshActivity()
{
    TaskTrackTunnelActivity activity;
    String error;
    if(!TaskTrackTunnelLoadActivity(activity, error))
        activity = TaskTrackTunnelActivity();

    RefreshActivityTable(activity);
    activity_count_.SetText(Format("Last %d communications", activity.recent.GetCount()));
}

void TaskTrackTunnelManager::RefreshActivityTable(const TaskTrackTunnelActivity& activity)
{
    int rows = max(1, activity.recent.GetCount());
    activity_model_.SetSize(rows, 5);

    if(activity.recent.IsEmpty()) {
        const char *values[] = { "—", "·", "activity", "No recent communications", "Waiting for remote traffic" };
        for(int c = 0; c < 5; ++c) {
            UiTableCell cell;
            cell.value = values[c];
            cell.edit_value = cell.value;
            cell.editable = false;
            cell.use_custom_ink = true;
            cell.ink = c >= 2 ? MutedColor() : SoftColor();
            activity_model_.SetCell(0, c, cell);
        }
        return;
    }

    for(int r = 0; r < activity.recent.GetCount(); ++r) {
        const TaskTrackTunnelActivityEvent& event = activity.recent[r];
        String values[5] = {
            event.time,
            event.direction == "in" ? String("←") : String("→"),
            event.kind,
            event.action,
            event.result
        };

        for(int c = 0; c < 5; ++c) {
            UiTableCell cell;
            cell.value = values[c];
            cell.edit_value = cell.value;
            cell.editable = false;
            cell.use_custom_ink = true;
            if(c == 0)
                cell.ink = SoftColor();
            else if(c == 1)
                cell.ink = event.direction == "in" ? ActivityColor() : AccentColor();
            else if(c == 2 || c == 4)
                cell.ink = MutedColor();
            else
                cell.ink = TextColor();
            if(c == 3) {
                cell.use_custom_font = true;
                cell.font = StdFont().Bold();
            }
            activity_model_.SetCell(r, c, cell);
        }
    }
}

void TaskTrackTunnelManager::SendProbe()
{
    TaskTrackTunnelProbe probe;
    String error;
    if(!TaskTrackTunnelLoadProbe(probe, error))
        probe = TaskTrackTunnelProbe();

    probe.sequence++;
    probe.updated_at = AsString(GetSysTime());
    probe.source = "TaskTrackTunnelGui";
    const McpTunnelProfile *profile = CurrentProfile();
    probe.tunnel_id = profile ? profile->tunnel_id : String();
    probe.message = Format("TaskTrack local probe #%d", probe.sequence);

    if(!TaskTrackTunnelSaveProbe(probe, error)) {
        Exclamation("Unable to save local tunnel probe.\n\n" + error);
        return;
    }

    PromptOK(Format("Probe #%d is ready.\n\nAsk browser ChatGPT to call tunnel_probe.", probe.sequence));
}

void TaskTrackTunnelManager::ClearActivity()
{
    String error;
    if(!TaskTrackTunnelResetActivity(error)) {
        Exclamation(error);
        return;
    }
    RefreshActivity();
    RefreshProjection();
}

String TaskTrackTunnelManager::BuildDiagnostics() const
{
    const McpTunnelProfile *profile = CurrentProfile();
    TaskTrackTunnelActivity activity;
    String activity_error;
    bool has_activity = TaskTrackTunnelLoadActivity(activity, activity_error);
    String credential_error;
    bool credential_available = profile && CredentialAvailable(credential_error);

    String out;
    out << "MCP Tunnel diagnostics\n"
        << "TaskTrack build: " << TaskTrackBuildVersion() << "\n"
        << "Profile: " << (profile ? profile->name : String("None")) << "\n"
        << "Machine: " << (profile ? profile->machine_id : String()) << "\n"
        << "Tunnel: " << (profile ? profile->tunnel_id : String()) << "\n"
        << "State: ";

    switch(runtime_.GetState()) {
    case McpTunnelRuntime::READY: out << "ready"; break;
    case McpTunnelRuntime::CONNECTING: out << "connecting"; break;
    case McpTunnelRuntime::FAULT: out << "error"; break;
    default: out << "stopped"; break;
    }

    out << "\nRuntime process: " << BoolText(runtime_.IsStarted()) << "\n"
        << "Healthy: " << BoolText(runtime_.IsHealthy()) << "\n"
        << "Ready: " << BoolText(runtime_.IsReady()) << "\n"
        << "Credential source: " << (profile ? McpTunnelCredentialSourceId(profile->credential_source) : String()) << "\n"
        << "Credential available: " << BoolText(credential_available) << "\n"
        << "Secret value: [not exposed]\n"
        << "Runtime executable: " << (profile ? profile->runtime_path : String()) << "\n";

    const_cast<TaskTrackTunnelManager *>(this)->RefreshMcpBinaryIdentity(true);
    out << "TaskTrack MCP identity: " << McpBinaryIdentityText() << "\n"
        << "TaskTrack MCP path: " << mcp_binary_path_ << "\n"
        << "TaskTrack MCP SHA256: " << mcp_binary_actual_hash_ << "\n"
        << "Expected MCP SHA256: " << mcp_binary_expected_hash_ << "\n"
        << "Bundle build: " << mcp_binary_bundle_build_ << "\n"
        << "Bundle source commit: " << mcp_binary_source_commit_ << "\n";

    if(profile) {
        out << "Enabled services: " << EnabledServiceCount(*profile) << "\n";
        for(const McpTunnelService& service : profile->services)
            out << "Service: " << service.id
                << " | channel=" << service.channel
                << " | enabled=" << BoolText(service.enabled)
                << " | command_configured=" << BoolText(!service.command.IsEmpty()) << "\n";
    }

    out << "TaskTrack remote activity: "
        << (has_activity ? AsString(activity.received) : String("0")) << " in / "
        << (has_activity ? AsString(activity.sent) : String("0")) << " out\n";

    if(has_activity && (!activity.last_method.IsEmpty() || !activity.last_tool.IsEmpty()))
        out << "Last TaskTrack remote call: " << activity.last_method
            << (activity.last_tool.IsEmpty() ? String() : " / " + activity.last_tool) << "\n";
    if(!runtime_.GetLastError().IsEmpty())
        out << "Last runtime error: " << runtime_.GetLastError() << "\n";
    return out;
}

void TaskTrackTunnelManager::CopyDiagnostics()
{
    WriteClipboardText(BuildDiagnostics());
}

void TaskTrackTunnelManager::ShowHelp()
{
    PromptOK(
        "MCP Tunnel Manager\n\n"
        "One machine profile owns one OpenAI Secure MCP Tunnel runtime. The Services page binds one or more separate local MCP servers to named tunnel channels.\n\n"
        "TaskTrack remains its own MCP/domain service. Additional services do not share TaskTrack task or dashboard state.\n\n"
        "For RC validation, use a session-only key or CONTROL_PLANE_API_KEY. The secret is never written to the machine profile.\n\n"
        "The intended durable cross-platform direction is a U++ encrypted vault using Core/SSL AES-256-GCM. OAuth remains a separate MCP-auth concern and does not currently replace the tunnel runtime key.\n\n"
        "Exactly one enabled service must use the main channel. Stop the active tunnel before changing the machine profile or service bindings.");
}

void TaskTrackTunnelManager::Tick()
{
    if(runtime_.IsStarted())
        RefreshRuntimeStatus(false);
    else
        RefreshProjection();
}

void TaskTrackTunnelManager::Layout()
{
    Rect client = GetSize();
    const int header_h = DPI(76);
    const int nav_h = DPI(45);
    const int footer_h = DPI(30);

    header_.SetRect(0, 0, client.GetWidth(), header_h);
    nav_.SetRect(0, header_h, client.GetWidth(), nav_h);
    pages_.SetRect(0, header_h + nav_h, client.GetWidth(),
                   max(0, client.GetHeight() - header_h - nav_h - footer_h));
    footer_.SetRect(0, max(0, client.GetHeight() - footer_h), client.GetWidth(), footer_h);

    overview_button_.SetRect(DPI(15), DPI(8), DPI(92), DPI(34));
    setup_button_.SetRect(DPI(111), DPI(8), DPI(76), DPI(34));
    services_button_.SetRect(DPI(191), DPI(8), DPI(88), DPI(34));
    nav_note_.SetRect(max(DPI(300), client.GetWidth() - DPI(190)), DPI(8), DPI(175), DPI(30));

    Rect page = overview_page_.GetSize();
    const int pad = DPI(17);
    const int gap = DPI(12);
    int width = max(0, page.GetWidth() - pad * 2);

    hero_.SetRect(pad, DPI(16), width, DPI(144));
    status_strip_.SetRect(pad, DPI(16) + DPI(144) + gap, width, DPI(74));
    int activity_y = DPI(16) + DPI(144) + gap + DPI(74) + gap;
    activity_panel_.SetRect(pad, activity_y, width, max(DPI(150), page.GetHeight() - activity_y - DPI(15)));

    Rect hr = hero_.GetSize();
    beacon_.SetRect(DPI(18), DPI(48), DPI(42), DPI(42));
    state_eyebrow_.SetRect(DPI(75), DPI(29), DPI(220), DPI(18));
    state_title_.SetRect(DPI(75), DPI(47), DPI(280), DPI(38));
    state_subtitle_.SetRect(DPI(75), DPI(87), DPI(300), DPI(22));

    int meta_x = max(DPI(400), hr.GetWidth() - DPI(390));
    profile_caption_.SetRect(meta_x, DPI(28), DPI(60), DPI(20));
    profile_value_.SetRect(meta_x + DPI(68), DPI(28), DPI(180), DPI(20));
    tunnel_caption_.SetRect(meta_x, DPI(58), DPI(60), DPI(20));
    tunnel_value_.SetRect(meta_x + DPI(68), DPI(58), DPI(180), DPI(20));
    sync_caption_.SetRect(meta_x, DPI(88), DPI(60), DPI(20));
    sync_value_.SetRect(meta_x + DPI(68), DPI(88), DPI(180), DPI(20));

    primary_button_.SetRect(max(0, hr.GetWidth() - DPI(132)), DPI(38), DPI(114), DPI(31));
    health_button_.SetRect(max(0, hr.GetWidth() - DPI(132)), DPI(77), DPI(114), DPI(31));

    Rect sr = status_strip_.GetSize();
    int cell_w = sr.GetWidth() / 4;
    for(int i = 0; i < 4; ++i) {
        int x = i * cell_w;
        int cx = i == 3 ? sr.GetWidth() - x : cell_w;
        status_cell_[i].SetRect(x, 0, cx, sr.GetHeight());
        status_caption_[i].SetRect(DPI(14), DPI(12), max(0, cx - DPI(28)), DPI(18));
        status_value_[i].SetRect(DPI(14), DPI(34), max(0, cx - DPI(28)), DPI(24));
    }

    Rect ar = activity_panel_.GetSize();
    activity_title_.SetRect(DPI(13), DPI(8), DPI(140), DPI(26));
    activity_live_.SetRect(DPI(155), DPI(8), DPI(70), DPI(26));
    activity_count_.SetRect(max(0, ar.GetWidth() - DPI(190)), DPI(8), DPI(175), DPI(26));
    activity_table_.SetRect(DPI(13), DPI(42), max(0, ar.GetWidth() - DPI(26)), max(0, ar.GetHeight() - DPI(84)));
    send_probe_button_.SetRect(DPI(10), max(0, ar.GetHeight() - DPI(36)), DPI(90), DPI(27));
    copy_diagnostics_button_.SetRect(DPI(107), max(0, ar.GetHeight() - DPI(36)), DPI(116), DPI(27));
    clear_activity_button_.SetRect(DPI(230), max(0, ar.GetHeight() - DPI(36)), DPI(105), DPI(27));
    activity_footer_note_.SetRect(max(DPI(345), ar.GetWidth() - DPI(270)), max(0, ar.GetHeight() - DPI(36)), DPI(255), DPI(27));

    int table_w = max(DPI(300), ar.GetWidth() - DPI(18));
    activity_table_.SetColumnWidth(0, DPI(78));
    activity_table_.SetColumnWidth(1, DPI(32));
    activity_table_.SetColumnWidth(2, DPI(105));
    int action_w = min(DPI(180), table_w / 4);
    activity_table_.SetColumnWidth(3, action_w);
    activity_table_.SetColumnWidth(4, max(DPI(140), table_w - DPI(78 + 32 + 105) - action_w));

    Rect sp = setup_page_.GetSize();
    int setup_width = max(0, sp.GetWidth() - pad * 2);
    profile_bar_.SetRect(pad, DPI(16), setup_width, DPI(75));
    setup_form_.SetRect(pad, DPI(16) + DPI(75) + gap, setup_width,
                        max(DPI(350), sp.GetHeight() - DPI(16) - DPI(75) - gap - DPI(15)));

    Rect pr = profile_bar_.GetSize();
    profile_select_caption_.SetRect(DPI(13), DPI(8), DPI(180), DPI(18));
    profile_dropdown_.SetRect(DPI(13), DPI(30), min(DPI(430), max(DPI(220), pr.GetWidth() - DPI(350))), DPI(32));
    delete_profile_button_.SetRect(max(0, pr.GetWidth() - DPI(83)), DPI(30), DPI(70), DPI(31));
    duplicate_profile_button_.SetRect(max(0, pr.GetWidth() - DPI(174)), DPI(30), DPI(84), DPI(31));
    new_profile_button_.SetRect(max(0, pr.GetWidth() - DPI(248)), DPI(30), DPI(67), DPI(31));

    Rect fr = setup_form_.GetSize();
    const int label_x = DPI(14), field_x = DPI(155);
    const int field_w = max(DPI(300), fr.GetWidth() - field_x - DPI(14));
    int y = DPI(10);

    section_profile_.SetRect(label_x, y, field_w, DPI(16)); y += DPI(22);
    profile_name_label_.SetRect(label_x, y + DPI(4), DPI(125), DPI(20));
    profile_name_edit_.SetRect(field_x, y, field_w, DPI(28)); y += DPI(34);
    machine_id_label_.SetRect(label_x, y + DPI(4), DPI(125), DPI(20));
    machine_id_edit_.SetRect(field_x, y, field_w, DPI(28)); y += DPI(34);
    tunnel_id_label_.SetRect(label_x, y + DPI(4), DPI(125), DPI(20));
    tunnel_id_edit_.SetRect(field_x, y, field_w, DPI(28)); y += DPI(34);

    credential_label_.SetRect(label_x, y + DPI(4), DPI(125), DPI(20));
    int source_w = max(DPI(200), field_w - DPI(120));
    credential_source_dropdown_.SetRect(field_x, y, source_w, DPI(28));
    credential_status_.SetRect(field_x + source_w + DPI(5), y + DPI(2), DPI(110), DPI(24)); y += DPI(34);
    credential_set_button_.SetRect(field_x, y, DPI(78), DPI(28));
    credential_clear_button_.SetRect(field_x + DPI(85), y, DPI(72), DPI(28));
    credential_note_.SetRect(field_x + DPI(168), y + DPI(1), max(0, field_w - DPI(168)), DPI(28)); y += DPI(38);

    section_runtime_.SetRect(label_x, y, field_w, DPI(16)); y += DPI(22);
    runtime_path_label_.SetRect(label_x, y + DPI(4), DPI(125), DPI(20));
    runtime_path_edit_.SetRect(field_x, y, max(DPI(180), field_w - DPI(82)), DPI(28));
    runtime_browse_button_.SetRect(field_x + max(DPI(180), field_w - DPI(75)), y, DPI(75), DPI(28)); y += DPI(38);

    section_launch_.SetRect(label_x, y, field_w, DPI(16)); y += DPI(22);
    auto_connect_label_.SetRect(label_x, y + DPI(3), DPI(125), DPI(20));
    auto_connect_toggle_.SetRect(field_x, y, DPI(38), DPI(22));
    auto_connect_title_.SetRect(field_x + DPI(50), y - DPI(2), field_w - DPI(50), DPI(18));
    auto_connect_note_.SetRect(field_x + DPI(50), y + DPI(15), field_w - DPI(50), DPI(16)); y += DPI(34);
    remember_label_.SetRect(label_x, y + DPI(3), DPI(125), DPI(20));
    remember_toggle_.SetRect(field_x, y, DPI(38), DPI(22));
    remember_title_.SetRect(field_x + DPI(50), y - DPI(2), field_w - DPI(50), DPI(18));
    remember_note_.SetRect(field_x + DPI(50), y + DPI(15), field_w - DPI(50), DPI(16));

    Rect svp = services_page_.GetSize();
    int services_width = max(0, svp.GetWidth() - pad * 2);
    service_bar_.SetRect(pad, DPI(16), services_width, DPI(75));
    service_form_.SetRect(pad, DPI(16) + DPI(75) + gap, services_width,
                          max(DPI(260), svp.GetHeight() - DPI(16) - DPI(75) - gap - DPI(15)));

    Rect sbr = service_bar_.GetSize();
    service_select_caption_.SetRect(DPI(13), DPI(8), DPI(180), DPI(18));
    service_dropdown_.SetRect(DPI(13), DPI(30), min(DPI(430), max(DPI(220), sbr.GetWidth() - DPI(350))), DPI(32));
    delete_service_button_.SetRect(max(0, sbr.GetWidth() - DPI(83)), DPI(30), DPI(70), DPI(31));
    duplicate_service_button_.SetRect(max(0, sbr.GetWidth() - DPI(174)), DPI(30), DPI(84), DPI(31));
    new_service_button_.SetRect(max(0, sbr.GetWidth() - DPI(248)), DPI(30), DPI(67), DPI(31));

    Rect sfr = service_form_.GetSize();
    const int slabel_x = DPI(14), sfield_x = DPI(155);
    const int sfield_w = max(DPI(300), sfr.GetWidth() - sfield_x - DPI(14));
    int sy = DPI(10);
    section_service_.SetRect(slabel_x, sy, sfield_w, DPI(16)); sy += DPI(22);
    service_name_label_.SetRect(slabel_x, sy + DPI(4), DPI(125), DPI(20));
    service_name_edit_.SetRect(sfield_x, sy, sfield_w, DPI(28)); sy += DPI(34);
    service_id_label_.SetRect(slabel_x, sy + DPI(4), DPI(125), DPI(20));
    service_id_edit_.SetRect(sfield_x, sy, sfield_w, DPI(28)); sy += DPI(34);
    service_channel_label_.SetRect(slabel_x, sy + DPI(4), DPI(125), DPI(20));
    service_channel_edit_.SetRect(sfield_x, sy, sfield_w, DPI(28)); sy += DPI(34);
    service_command_label_.SetRect(slabel_x, sy + DPI(4), DPI(125), DPI(20));
    service_command_edit_.SetRect(sfield_x, sy, max(DPI(180), sfield_w - DPI(82)), DPI(28));
    service_browse_button_.SetRect(sfield_x + max(DPI(180), sfield_w - DPI(75)), sy, DPI(75), DPI(28)); sy += DPI(38);
    service_enabled_label_.SetRect(slabel_x, sy + DPI(3), DPI(125), DPI(20));
    service_enabled_toggle_.SetRect(sfield_x, sy, DPI(38), DPI(22));
    service_enabled_title_.SetRect(sfield_x + DPI(50), sy - DPI(2), sfield_w - DPI(50), DPI(18));
    service_enabled_note_.SetRect(sfield_x + DPI(50), sy + DPI(15), sfield_w - DPI(50), DPI(32));

    Rect fo = footer_.GetSize();
    footer_build_.SetRect(DPI(11), DPI(4), DPI(145), DPI(22));
    footer_mcp_.SetRect(DPI(165), DPI(4), DPI(100), DPI(22));
    footer_dashboard_.SetRect(DPI(275), DPI(4), DPI(130), DPI(22));
    footer_copy_.SetRect(max(0, fo.GetWidth() - DPI(118)), DPI(2), DPI(108), DPI(26));
    footer_help_.SetRect(max(0, fo.GetWidth() - DPI(170)), DPI(2), DPI(46), DPI(26));
}

}
