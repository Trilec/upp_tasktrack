#include "TaskTrackTunnelManager.h"

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

ValueMap ProfileToValue(const TaskTrackTunnelProfile& profile)
{
    ValueMap out;
    out.Add("id", profile.id);
    out.Add("name", profile.name);
    out.Add("tunnel_id", profile.tunnel_id);
    out.Add("runtime_path", profile.runtime_path);
    out.Add("mcp_path", profile.mcp_path);
    out.Add("auto_connect", profile.auto_connect);
    out.Add("remember_profile", profile.remember_profile);
    return out;
}

TaskTrackTunnelProfile ProfileFromValue(const Value& value)
{
    TaskTrackTunnelProfile profile;
    if(!value.Is<ValueMap>())
        return profile;
    profile.id = AsString(value["id"]);
    profile.name = AsString(value["name"]);
    profile.tunnel_id = AsString(value["tunnel_id"]);
    profile.runtime_path = AsString(value["runtime_path"]);
    profile.mcp_path = AsString(value["mcp_path"]);
    profile.auto_connect = !IsNull(value["auto_connect"]) && (bool)value["auto_connect"];
    profile.remember_profile = IsNull(value["remember_profile"]) || (bool)value["remember_profile"];
    return profile;
}

}

TaskTrackTunnelManager::TaskTrackTunnelManager(const TaskTrackTunnelManagerOptions& options)
    : options_(options)
{
    Title("TaskTrack Tunnel");
    Sizeable().Zoomable();
    SetRect(0, 0, DPI(920), DPI(610));
    SetMinSize(Size(DPI(780), DPI(610)));

    LoadProfiles();
    EnsureDefaultProfile();

    if(TaskTrackTunnelProfile *profile = CurrentProfile()) {
        if(!options_.tunnel_id.IsEmpty())
            profile->tunnel_id = options_.tunnel_id;
        if(!options_.runtime_path.IsEmpty())
            profile->runtime_path = options_.runtime_path;
        if(profile->mcp_path.IsEmpty())
            profile->mcp_path = GetExeDirFile("TaskTrackMcp.exe");
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

    const TaskTrackTunnelProfile *profile = CurrentProfile();
    if(profile && profile->auto_connect && !profile->tunnel_id.IsEmpty()
       && !GetEnv("CONTROL_PLANE_API_KEY").IsEmpty())
        PostCallback([=] { ConnectRuntime(); });
}

TaskTrackTunnelManager::~TaskTrackTunnelManager()
{
    KillTimeCallback(TIMER_REFRESH);
    SaveProfileFromUi();
    SaveProfiles();
    if(runtime_started_)
        runtime_process_.Kill();
}

void TaskTrackTunnelManager::BuildUi()
{
    root_.SetCustomStyle(MakePanelStyle(SurfaceColor(), 0, SurfaceColor()));
    Add(root_.SizePos());

    root_.Add(header_);
    root_.Add(nav_);
    root_.Add(pages_);
    root_.Add(footer_);

    header_.SetTitle("TaskTrack Tunnel")
           .SetSubTitle("Secure ChatGPT ↔ local TaskTrack connection")
           .SetMedia(ICON_DESIGN_WIDGETS_48())
           .SetMediaSide(UiAlign::LEFT)
           .SetMediaAlign(UiAlign::CENTER, UiAlign::CENTER)
           .SetMediaReserve(DPI(44))
           .SetMediaMin(DPI(26))
           .SetMediaAutoFit(true)
           .ShowTitleLine(false)
           .SetContentInset(DPI(10))
           .SetContentCell(header_actions_);

    header_actions_.SetGap(DPI(5)).SetInset(0).SetAlignItems(UiCrossAlign::Center);
    header_actions_.AddSpacer(1).Expand(1);
    theme_button_.SetIcon(ICON_ACTION_DARK_MODE_48()).SetIconSize(DPI(16), DPI(16)).Tip("Toggle light/dark theme");
    help_button_.SetIcon(ICON_DESIGN_HELP_48()).SetIconSize(DPI(16), DPI(16)).Tip("TaskTrack Tunnel help");
    exit_button_.SetIcon(ICON_DESIGN_MODE_OFF_ON_48()).SetIconSize(DPI(16), DPI(16)).Tip("Close");
    header_actions_.Add(help_button_).Fixed(DPI(32));
    header_actions_.Add(theme_button_).Fixed(DPI(32));
    header_actions_.Add(exit_button_).Fixed(DPI(32));

    nav_.Add(overview_button_);
    nav_.Add(setup_button_);
    nav_.Add(nav_note_);
    overview_button_.SetText("Overview").SetCheckable();
    setup_button_.SetText("Setup").SetCheckable();
    nav_note_.SetText("Local tunnel control").SetAlign(UiAlign::RIGHT, UiAlign::CENTER);

    pages_.Add(overview_page_, "overview");
    pages_.Add(setup_page_, "setup");

    BuildOverview();
    BuildSetup();

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
    profile_caption_.SetText("Profile");
    tunnel_caption_.SetText("Tunnel");
    sync_caption_.SetText("Last sync");
    primary_button_.SetText("Connect");
    health_button_.SetText("Open health");

    for(int i = 0; i < 4; ++i) {
        status_strip_.Add(status_cell_[i]);
        status_cell_[i].Add(status_caption_[i]);
        status_cell_[i].Add(status_value_[i]);
    }
    status_caption_[0].SetText("TASKTRACK MCP");
    status_caption_[1].SetText("OPENAI TUNNEL");
    status_caption_[2].SetText("REMOTE ACTIVITY");
    status_caption_[3].SetText("BUILD");

    activity_panel_.Add(activity_title_);
    activity_panel_.Add(activity_live_);
    activity_panel_.Add(activity_count_);
    activity_panel_.Add(activity_table_);
    activity_panel_.Add(copy_diagnostics_button_);
    activity_panel_.Add(clear_activity_button_);
    activity_panel_.Add(activity_footer_note_);

    activity_title_.SetText("Recent activity");
    activity_live_.EnableRich(true).ClearSpans().AddBulletSpan(OkColor(), DPI(6)).AddTextSpan("  live");
    activity_count_.SetText("Last 6 communications").SetAlign(UiAlign::RIGHT, UiAlign::CENTER);
    copy_diagnostics_button_.SetText("Copy diagnostics");
    clear_activity_button_.SetText("Clear activity");
    activity_footer_note_.SetText("No remote traffic yet").SetAlign(UiAlign::RIGHT, UiAlign::CENTER);

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

    profile_select_caption_.SetText("Tunnel profile");
    new_profile_button_.SetText("+ New");
    duplicate_profile_button_.SetText("Duplicate");
    delete_profile_button_.SetText("Delete");

    Ctrl *controls[] = {
        &section_profile_, &section_runtime_, &section_launch_,
        &profile_name_label_, &profile_name_edit_,
        &tunnel_id_label_, &tunnel_id_edit_,
        &credential_label_, &credential_edit_, &credential_status_, &credential_note_,
        &runtime_path_label_, &runtime_path_edit_, &runtime_browse_button_,
        &mcp_path_label_, &mcp_path_edit_, &mcp_browse_button_,
        &auto_connect_label_, &auto_connect_toggle_, &auto_connect_title_, &auto_connect_note_,
        &remember_label_, &remember_toggle_, &remember_title_, &remember_note_
    };
    for(Ctrl *ctrl : controls)
        setup_form_.Add(*ctrl);

    section_profile_.SetText("PROFILE");
    section_runtime_.SetText("RUNTIME");
    section_launch_.SetText("LAUNCH BEHAVIOUR");

    profile_name_label_.SetText("Profile name");
    tunnel_id_label_.SetText("Tunnel ID");
    credential_label_.SetText("Credential source");
    credential_edit_.SetTextUtf8("CONTROL_PLANE_API_KEY");
    credential_edit_.SetReadOnly();
    credential_note_.SetText("The API key is read from the credential source and is never shown or stored in the profile.");

    runtime_path_label_.SetText("Runtime executable");
    mcp_path_label_.SetText("TaskTrack MCP");
    runtime_browse_button_.SetText("Browse");
    mcp_browse_button_.SetText("Browse");

    auto_connect_label_.SetText("Auto-connect");
    auto_connect_title_.SetText("Auto-connect on launch");
    auto_connect_note_.SetText("Start the selected tunnel profile when the app opens.");

    remember_label_.SetText("Remember profile");
    remember_title_.SetText("Remember tunnel profile");
    remember_note_.SetText("Reopen with the last selected named profile.");
}

void TaskTrackTunnelManager::Wire()
{
    overview_button_.WhenAction = [=] { SelectPage(PAGE_OVERVIEW); };
    setup_button_.WhenAction = [=] { SelectPage(PAGE_SETUP); };
    theme_button_.WhenAction = [=] { ToggleTheme(); };
    help_button_.WhenAction = [=] { ShowHelp(); };
    exit_button_.WhenAction = [=] { Close(); };
    footer_help_.WhenAction = [=] { ShowHelp(); };
    footer_copy_.WhenAction = [=] { CopyDiagnostics(); };

    primary_button_.WhenAction = [=] {
        RuntimeState state = GetRuntimeState();
        if(state == STATE_READY || state == STATE_CONNECTING)
            StopRuntime();
        else
            ConnectRuntime();
    };
    health_button_.WhenAction = [=] { OpenHealth(); };
    copy_diagnostics_button_.WhenAction = [=] { CopyDiagnostics(); };
    clear_activity_button_.WhenAction = [=] { ClearActivity(); };

    profile_dropdown_.WhenSelectData = [=](const Value& value) {
        if(loading_profile_)
            return;
        SaveProfileFromUi();
        SelectProfileById(AsString(value));
    };
    new_profile_button_.WhenAction = [=] { NewProfile(); };
    duplicate_profile_button_.WhenAction = [=] { DuplicateProfile(); };
    delete_profile_button_.WhenAction = [=] { DeleteProfile(); };

    profile_name_edit_.WhenChange = [=] { if(!loading_profile_) SaveProfileFromUi(); };
    tunnel_id_edit_.WhenChange = [=] { if(!loading_profile_) SaveProfileFromUi(); };
    runtime_path_edit_.WhenChange = [=] { if(!loading_profile_) SaveProfileFromUi(); };
    mcp_path_edit_.WhenChange = [=] { if(!loading_profile_) SaveProfileFromUi(); };
    auto_connect_toggle_.WhenAction = [=] { if(!loading_profile_) SaveProfileFromUi(); };
    remember_toggle_.WhenAction = [=] { if(!loading_profile_) SaveProfileFromUi(); };
    runtime_browse_button_.WhenAction = [=] { BrowseRuntime(); };
    mcp_browse_button_.WhenAction = [=] { BrowseMcp(); };
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
    header_style.media_tint_mono = true;
    header_.SetCustomStyle(header_style);

    ConfigureNavButton(overview_button_);
    ConfigureNavButton(setup_button_);
    nav_note_.SetCustomStyle(MakeLabelStyle(SoftColor(), 10));

    hero_.SetCustomStyle(MakePanelStyle(dark_theme_ ? Color(34,40,48) : White(), 10));
    status_strip_.SetCustomStyle(MakePanelStyle(dark_theme_ ? Color(34,40,48) : White(), 10));
    activity_panel_.SetCustomStyle(MakePanelStyle(dark_theme_ ? Color(34,40,48) : White(), 10));
    profile_bar_.SetCustomStyle(MakePanelStyle(dark_theme_ ? Color(34,40,48) : White(), 10));
    setup_form_.SetCustomStyle(MakePanelStyle(dark_theme_ ? Color(34,40,48) : White(), 10));

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
        status_cell_[i].SetCustomStyle(MakePanelStyle(dark_theme_ ? Color(34,40,48) : White(), 0,
                                                      i == 3 ? (dark_theme_ ? Color(34,40,48) : White()) : LineColor()));
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
    table_style.table_bg = dark_theme_ ? Color(34,40,48) : White();
    table_style.alternate_row_bg = table_style.table_bg;
    table_style.hover_bg = dark_theme_ ? Color(43,51,61) : Color(246,248,250);
    table_style.cell_ink = TextColor();
    table_style.muted_ink = MutedColor();
    table_style.grid_color = LineColor();
    activity_table_.SetCustomStyle(table_style);

    UiButton::Style small_button = MakeButtonStyle(UiButtonRole::Subtle);
    copy_diagnostics_button_.SetCustomStyle(small_button);
    clear_activity_button_.SetCustomStyle(small_button);
    footer_help_.SetCustomStyle(small_button);
    footer_copy_.SetCustomStyle(small_button);
    new_profile_button_.SetCustomStyle(small_button);
    duplicate_profile_button_.SetCustomStyle(small_button);
    delete_profile_button_.SetCustomStyle(MakeButtonStyle(UiButtonRole::Danger));
    runtime_browse_button_.SetCustomStyle(small_button);
    mcp_browse_button_.SetCustomStyle(small_button);

    section_profile_.SetCustomStyle(MakeLabelStyle(SoftColor(), 9, true));
    section_runtime_.SetCustomStyle(MakeLabelStyle(SoftColor(), 9, true));
    section_launch_.SetCustomStyle(MakeLabelStyle(SoftColor(), 9, true));

    UiLabel *form_labels[] = {
        &profile_select_caption_, &profile_name_label_, &tunnel_id_label_, &credential_label_,
        &runtime_path_label_, &mcp_path_label_, &auto_connect_label_, &remember_label_
    };
    for(UiLabel *label : form_labels)
        label->SetCustomStyle(MakeLabelStyle(MutedColor(), 10));

    auto_connect_title_.SetCustomStyle(MakeLabelStyle(TextColor(), 10, true));
    remember_title_.SetCustomStyle(MakeLabelStyle(TextColor(), 10, true));
    auto_connect_note_.SetCustomStyle(MakeLabelStyle(SoftColor(), 9));
    remember_note_.SetCustomStyle(MakeLabelStyle(SoftColor(), 9));
    credential_note_.SetCustomStyle(MakeLabelStyle(SoftColor(), 9));

    footer_build_.SetCustomStyle(MakeLabelStyle(SoftColor(), 9));
    footer_mcp_.SetCustomStyle(MakeLabelStyle(SoftColor(), 9));
    footer_dashboard_.SetCustomStyle(MakeLabelStyle(SoftColor(), 9));

    UiToolButton::Style utility_style = UiTheme::ResolveToolButton(UiToolButtonRole::Standard);
    utility_style.metrics.radius = DPI(7);
    theme_button_.SetCustomStyle(utility_style);
    help_button_.SetCustomStyle(utility_style);
    exit_button_.SetCustomStyle(UiTheme::ResolveToolButton(UiRole::Alert));

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
    style.underline = true;
    style.underline_width = DPI(2);
    style.underline_offset = 0;
    button.SetCustomStyle(style);
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
    page = minmax(page, (int)PAGE_OVERVIEW, (int)PAGE_SETUP);
    pages_.SetActivePage(page);
    overview_button_.SetChecked(page == PAGE_OVERVIEW);
    setup_button_.SetChecked(page == PAGE_SETUP);
}

String TaskTrackTunnelManager::ProfileStorePath() const
{
    return ConfigFile("tasktrack-tunnel-profiles.json");
}

void TaskTrackTunnelManager::LoadProfiles()
{
    profiles_.Clear();
    selected_profile_ = -1;

    String json = LoadFile(ProfileStorePath());
    if(IsNull(json) || json.IsEmpty())
        return;

    try {
        Value root = ParseJSON(json);
        if(!root.Is<ValueMap>())
            return;

        dark_theme_ = !IsNull(root["dark_theme"]) && (bool)root["dark_theme"];
        String selected_id = AsString(root["selected_profile"]);
        Value list_value = root["profiles"];
        if(list_value.Is<ValueArray>()) {
            ValueArray list = list_value;
            for(int i = 0; i < list.GetCount(); ++i) {
                TaskTrackTunnelProfile profile = ProfileFromValue(list[i]);
                if(profile.id.IsEmpty() || profile.name.IsEmpty())
                    continue;
                if(profile.runtime_path.IsEmpty())
                    profile.runtime_path = GetExeDirFile("tunnel-client.exe");
                if(profile.mcp_path.IsEmpty())
                    profile.mcp_path = GetExeDirFile("TaskTrackMcp.exe");
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
    }
}

void TaskTrackTunnelManager::SaveProfiles()
{
    if(profiles_.IsEmpty())
        return;

    ValueMap root;
    root.Add("schema_version", 1);
    root.Add("dark_theme", dark_theme_);

    String selected_id;
    const TaskTrackTunnelProfile *profile = CurrentProfile();
    if(profile && profile->remember_profile)
        selected_id = profile->id;
    root.Add("selected_profile", selected_id);

    ValueArray list;
    for(const TaskTrackTunnelProfile& item : profiles_)
        list.Add(ProfileToValue(item));
    root.Add("profiles", list);
    SaveFile(ProfileStorePath(), AsJSON(root, true));
}

void TaskTrackTunnelManager::EnsureDefaultProfile()
{
    if(profiles_.IsEmpty()) {
        TaskTrackTunnelProfile profile;
        profile.id = "local-tasktrack";
        profile.name = "Local TaskTrack";
        profile.runtime_path = options_.runtime_path.IsEmpty()
            ? GetExeDirFile("tunnel-client.exe") : options_.runtime_path;
        profile.mcp_path = GetExeDirFile("TaskTrackMcp.exe");
        profile.tunnel_id = options_.tunnel_id;
        profiles_.Add(pick(profile));
        selected_profile_ = 0;
    }
    if(selected_profile_ < 0 || selected_profile_ >= profiles_.GetCount())
        selected_profile_ = 0;
}

TaskTrackTunnelProfile* TaskTrackTunnelManager::CurrentProfile()
{
    return selected_profile_ >= 0 && selected_profile_ < profiles_.GetCount()
        ? &profiles_[selected_profile_] : nullptr;
}

const TaskTrackTunnelProfile* TaskTrackTunnelManager::CurrentProfile() const
{
    return selected_profile_ >= 0 && selected_profile_ < profiles_.GetCount()
        ? &profiles_[selected_profile_] : nullptr;
}

String TaskTrackTunnelManager::NewProfileId() const
{
    for(int n = 1;; ++n) {
        String id = Format("profile-%d", n);
        bool used = false;
        for(const TaskTrackTunnelProfile& profile : profiles_)
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
    for(const TaskTrackTunnelProfile& profile : profiles_)
        model.Add(profile.name, profile.id);
    if(const TaskTrackTunnelProfile *profile = CurrentProfile())
        profile_dropdown_.SelectByData(profile->id);
    loading_profile_ = false;
}

void TaskTrackTunnelManager::LoadProfileIntoUi()
{
    const TaskTrackTunnelProfile *profile = CurrentProfile();
    if(!profile)
        return;

    loading_profile_ = true;
    profile_name_edit_.SetTextUtf8(profile->name);
    tunnel_id_edit_.SetTextUtf8(profile->tunnel_id);
    runtime_path_edit_.SetTextUtf8(profile->runtime_path);
    mcp_path_edit_.SetTextUtf8(profile->mcp_path);
    auto_connect_toggle_.SetOn(profile->auto_connect);
    remember_toggle_.SetOn(profile->remember_profile);
    loading_profile_ = false;
    RefreshProjection();
}

void TaskTrackTunnelManager::SaveProfileFromUi()
{
    if(loading_profile_)
        return;
    TaskTrackTunnelProfile *profile = CurrentProfile();
    if(!profile)
        return;

    profile->name = TrimBoth(profile_name_edit_.GetTextUtf8());
    if(profile->name.IsEmpty())
        profile->name = "Unnamed profile";
    profile->tunnel_id = TrimBoth(tunnel_id_edit_.GetTextUtf8());
    profile->runtime_path = TrimBoth(runtime_path_edit_.GetTextUtf8());
    profile->mcp_path = TrimBoth(mcp_path_edit_.GetTextUtf8());
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
            LoadProfileIntoUi();
            SaveProfiles();
            return;
        }
}

void TaskTrackTunnelManager::NewProfile()
{
    if(runtime_started_) {
        Exclamation("Stop the current tunnel before changing profiles.");
        return;
    }

    SaveProfileFromUi();
    TaskTrackTunnelProfile profile;
    profile.id = NewProfileId();
    profile.name = "New profile";
    profile.runtime_path = GetExeDirFile("tunnel-client.exe");
    profile.mcp_path = GetExeDirFile("TaskTrackMcp.exe");
    profiles_.Add(pick(profile));
    selected_profile_ = profiles_.GetCount() - 1;
    RebuildProfileDropdown();
    LoadProfileIntoUi();
    SaveProfiles();
}

void TaskTrackTunnelManager::DuplicateProfile()
{
    if(runtime_started_) {
        Exclamation("Stop the current tunnel before changing profiles.");
        return;
    }

    SaveProfileFromUi();
    const TaskTrackTunnelProfile *source = CurrentProfile();
    if(!source)
        return;

    TaskTrackTunnelProfile profile;
    profile.id = NewProfileId();
    profile.name = source->name + " copy";
    profile.runtime_path = source->runtime_path;
    profile.mcp_path = source->mcp_path;
    profile.auto_connect = false;
    profile.remember_profile = source->remember_profile;
    profiles_.Add(pick(profile));
    selected_profile_ = profiles_.GetCount() - 1;
    RebuildProfileDropdown();
    LoadProfileIntoUi();
    SaveProfiles();
}

void TaskTrackTunnelManager::DeleteProfile()
{
    if(runtime_started_) {
        Exclamation("Stop the current tunnel before changing profiles.");
        return;
    }
    if(profiles_.GetCount() <= 1) {
        Exclamation("At least one tunnel profile must remain.");
        return;
    }
    if(!PromptYesNo("Delete the selected tunnel profile?"))
        return;

    profiles_.Remove(selected_profile_);
    selected_profile_ = min(selected_profile_, profiles_.GetCount() - 1);
    RebuildProfileDropdown();
    LoadProfileIntoUi();
    SaveProfiles();
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

void TaskTrackTunnelManager::BrowseMcp()
{
    FileSel selector;
    selector.Type("Executable", "*.exe");
    String current = mcp_path_edit_.GetTextUtf8();
    if(!current.IsEmpty())
        selector.Set(current);
    if(selector.ExecuteOpen("Choose TaskTrack MCP")) {
        mcp_path_edit_.SetTextUtf8(~selector);
        SaveProfileFromUi();
    }
}

String TaskTrackTunnelManager::RuntimeMcpCommand() const
{
    const TaskTrackTunnelProfile *profile = CurrentProfile();
    String command = profile ? profile->mcp_path : String();
    command.Replace("\\", "/");
    if(command.Find(' ') >= 0 || command.Find('\t') >= 0)
        command = "\"" + command + "\"";
    return command;
}

bool TaskTrackTunnelManager::LoadHealthUrl()
{
    if(health_url_file_.IsEmpty() || !FileExists(health_url_file_))
        return false;

    String url = TrimBoth(LoadFile(health_url_file_));
    if(url.IsEmpty())
        return false;
    while(url.EndsWith("/"))
        url = url.Left(url.GetCount() - 1);
    health_url_ = url;
    return true;
}

void TaskTrackTunnelManager::DrainRuntimeOutput()
{
    if(!runtime_started_)
        return;
    for(int i = 0; i < 8; ++i) {
        String out, err;
        runtime_process_.Read2(out, err);
        if(out.IsEmpty() && err.IsEmpty())
            break;
        runtime_output_ << out << err;
        if(runtime_output_.GetCount() > 6000)
            runtime_output_ = runtime_output_.Right(6000);
    }
}

String TaskTrackTunnelManager::RuntimeDiagnostics()
{
    String out = runtime_output_;
    String log = runtime_log_file_.IsEmpty() ? String() : LoadFile(runtime_log_file_);
    if(!IsNull(log) && !log.IsEmpty()) {
        if(log.GetCount() > 3000)
            log = log.Right(3000);
        if(!out.IsEmpty())
            out << "\n";
        out << log;
    }
    return out;
}

bool TaskTrackTunnelManager::ProbeHealth(const String& suffix, int& status, String& error)
{
    status = 0;
    error.Clear();
    if(health_url_.IsEmpty() && !LoadHealthUrl()) {
        error = "Health URL is not available yet.";
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

void TaskTrackTunnelManager::ConnectRuntime()
{
    SaveProfileFromUi();
    const TaskTrackTunnelProfile *profile = CurrentProfile();
    if(!profile)
        return;

    last_error_.Clear();

    if(profile->tunnel_id.IsEmpty()) {
        last_error_ = "Tunnel ID is not set.";
        RefreshProjection();
        return;
    }
    if(!FileExists(profile->runtime_path)) {
        last_error_ = "The OpenAI tunnel runtime executable was not found.";
        RefreshProjection();
        return;
    }
    if(!FileExists(profile->mcp_path)) {
        last_error_ = "TaskTrackMcp.exe was not found.";
        RefreshProjection();
        return;
    }
    if(GetEnv("CONTROL_PLANE_API_KEY").IsEmpty()) {
        last_error_ = "CONTROL_PLANE_API_KEY is not set.";
        RefreshProjection();
        return;
    }
    if(runtime_started_ && runtime_process_.IsRunning()) {
        RefreshRuntimeStatus(true);
        return;
    }

    runtime_process_.Kill();
    runtime_started_ = false;
    runtime_healthy_ = false;
    runtime_ready_ = false;
    health_url_.Clear();
    runtime_output_.Clear();

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
    args.Add(profile->tunnel_id);
    args.Add("--mcp.command");
    args.Add(RuntimeMcpCommand());
    args.Add("--health.listen-addr");
    args.Add("127.0.0.1:0");
    args.Add("--health.url-file");
    args.Add(health_url_file_);
    args.Add("--log.file");
    args.Add(runtime_log_file_);

    bool started = runtime_process_.Start(~profile->runtime_path, args);
    SetEnv("TASKTRACK_TUNNEL_REMOTE", old_remote);

    if(!started) {
        last_error_ = "Unable to start the OpenAI tunnel runtime.";
        RefreshProjection();
        return;
    }

    runtime_started_ = true;
    RefreshProjection();

    for(int i = 0; i < 40; ++i) {
        DrainRuntimeOutput();
        if(LoadHealthUrl() || !runtime_process_.IsRunning())
            break;
        Sleep(100);
    }

    RefreshRuntimeStatus(false);
}

void TaskTrackTunnelManager::RefreshRuntimeStatus(bool show_dialog)
{
    DrainRuntimeOutput();

    bool running = runtime_started_ && runtime_process_.IsRunning();
    if(!running) {
        if(runtime_started_) {
            String output;
            int code = runtime_process_.Finish(output);
            runtime_output_ << output;
            last_error_ = Format("Tunnel runtime exited with code %d.", code);
            runtime_process_.Kill();
        }
        runtime_started_ = false;
        runtime_healthy_ = false;
        runtime_ready_ = false;
        RefreshProjection();

        if(show_dialog && !last_error_.IsEmpty()) {
            String message = last_error_;
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

    if(runtime_ready_)
        last_error_.Clear();
    else if(!runtime_healthy_ && !health_error.IsEmpty())
        last_error_ = health_error;

    RefreshProjection();

    if(show_dialog) {
        String message = BuildDiagnostics();
        if(!ready_error.IsEmpty() && !runtime_ready_)
            message << "\nreadyz: " << ready_error;
        PromptOK(message);
    }
}

void TaskTrackTunnelManager::StopRuntime()
{
    if(runtime_started_)
        runtime_process_.Kill();
    runtime_started_ = false;
    runtime_healthy_ = false;
    runtime_ready_ = false;
    health_url_.Clear();
    last_error_.Clear();
    RefreshProjection();
}

void TaskTrackTunnelManager::OpenHealth()
{
    if(health_url_.IsEmpty())
        RefreshRuntimeStatus(false);
    if(health_url_.IsEmpty()) {
        Exclamation("The tunnel runtime has not reported its health URL yet.");
        return;
    }
    LaunchWebBrowser(health_url_ + "/readyz");
}

TaskTrackTunnelManager::RuntimeState TaskTrackTunnelManager::GetRuntimeState() const
{
    if(!last_error_.IsEmpty() && !runtime_ready_)
        return STATE_ERROR;
    if(runtime_ready_)
        return STATE_READY;
    if(runtime_started_)
        return STATE_CONNECTING;
    return STATE_STOPPED;
}

void TaskTrackTunnelManager::RefreshProjection()
{
    RuntimeState state = GetRuntimeState();
    const TaskTrackTunnelProfile *profile = CurrentProfile();

    Color state_color = StoppedColor();
    Color beacon_face = SubtleColor();
    String state_title = "STOPPED";
    String state_subtitle = "Tunnel is not connected";
    String primary_text = "Connect";

    if(state == STATE_READY) {
        state_color = OkColor();
        beacon_face = dark_theme_ ? Color(32,55,47) : Color(237,249,244);
        state_title = "READY";
        state_subtitle = "Secure tunnel connected";
        primary_text = "Stop";
    }
    else if(state == STATE_CONNECTING) {
        state_color = ActivityColor();
        beacon_face = dark_theme_ ? Color(58,48,32) : Color(253,246,233);
        state_title = "CONNECTING...";
        state_subtitle = "Opening secure tunnel";
        primary_text = "Cancel";
    }
    else if(state == STATE_ERROR) {
        state_color = DangerColor();
        beacon_face = dark_theme_ ? Color(60,39,43) : Color(255,241,242);
        state_title = "ERROR";
        state_subtitle = last_error_.IsEmpty() ? String("Tunnel could not be established") : last_error_;
        primary_text = "Retry";
    }

    beacon_.SetCustomStyle(MakePanelStyle(beacon_face, 11, Blend(state_color, LineColor(), 130)));
    beacon_core_.ClearSpans().EnableRich(true).AddBulletSpan(state_color, DPI(14));
    beacon_core_.SetAlign(UiAlign::CENTER, UiAlign::CENTER);

    state_title_.SetText(state_title);
    state_title_.SetCustomStyle(MakeLabelStyle(state == STATE_READY ? TextColor() : state_color, 27, true));
    state_subtitle_.SetText(state_subtitle);
    primary_button_.SetText(primary_text);

    if(profile) {
        profile_value_.SetText(profile->name);
        tunnel_value_.SetText(profile->tunnel_id.IsEmpty() ? String("Not configured") : EllipsizeMiddle(profile->tunnel_id, 10));
    }
    else {
        profile_value_.SetText("No profile");
        tunnel_value_.SetText("Not configured");
    }

    TaskTrackTunnelActivity activity;
    String activity_error;
    bool has_activity = TaskTrackTunnelLoadActivity(activity, activity_error);
    sync_value_.SetText(has_activity && !activity.updated_at.IsEmpty() ? activity.updated_at : String("—"));

    Color mcp_color = profile && FileExists(profile->mcp_path) ? OkColor() : DangerColor();
    status_value_[0].ClearSpans().EnableRich(true)
                    .AddBulletSpan(mcp_color, DPI(7))
                    .AddTextSpan(profile && FileExists(profile->mcp_path) ? "  Online" : "  Missing", TextColor(), true);

    Color tunnel_color = state == STATE_READY ? OkColor()
                       : state == STATE_CONNECTING ? ActivityColor()
                       : state == STATE_ERROR ? DangerColor()
                                              : StoppedColor();
    String tunnel_text = state == STATE_READY ? "Healthy"
                       : state == STATE_CONNECTING ? "Connecting"
                       : state == STATE_ERROR ? "Fault"
                                              : "Stopped";
    status_value_[1].ClearSpans().EnableRich(true)
                    .AddBulletSpan(tunnel_color, DPI(7))
                    .AddTextSpan("  " + tunnel_text, TextColor(), true);

    int64 received = has_activity ? activity.received : 0;
    int64 sent = has_activity ? activity.sent : 0;
    status_value_[2].ClearSpans().EnableRich(true)
                    .AddBulletSpan((received || sent) ? ActivityColor() : StoppedColor(), DPI(7))
                    .AddTextSpan(Format("  %lld in / %lld out", (long long)received, (long long)sent), TextColor(), true);
    status_value_[3].SetText(TaskTrackBuildVersion());

    activity_live_.Show(runtime_started_);
    activity_footer_note_.SetText(state == STATE_READY ? "Traffic flowing normally"
                                : state == STATE_CONNECTING ? "Connecting to control plane"
                                : state == STATE_ERROR ? "Last connection attempt failed"
                                                       : "No remote traffic while stopped");

    bool can_edit_profile = !runtime_started_;
    profile_dropdown_.Enable(can_edit_profile);
    new_profile_button_.Enable(can_edit_profile);
    duplicate_profile_button_.Enable(can_edit_profile);
    delete_profile_button_.Enable(can_edit_profile && profiles_.GetCount() > 1);
    profile_name_edit_.Enable(can_edit_profile);
    tunnel_id_edit_.Enable(can_edit_profile);
    runtime_path_edit_.Enable(can_edit_profile);
    mcp_path_edit_.Enable(can_edit_profile);
    runtime_browse_button_.Enable(can_edit_profile);
    mcp_browse_button_.Enable(can_edit_profile);
    auto_connect_toggle_.Enable(can_edit_profile);
    remember_toggle_.Enable(can_edit_profile);
    health_button_.Enable(runtime_started_ && !health_url_.IsEmpty());

    bool key_available = !GetEnv("CONTROL_PLANE_API_KEY").IsEmpty();
    credential_status_.ClearSpans().EnableRich(true)
                      .AddBulletSpan(key_available ? OkColor() : DangerColor(), DPI(7))
                      .AddTextSpan(key_available ? "  Available" : "  Not set",
                                   key_available ? OkColor() : DangerColor(), true);

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
    const TaskTrackTunnelProfile *profile = CurrentProfile();
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
    const TaskTrackTunnelProfile *profile = CurrentProfile();
    TaskTrackTunnelActivity activity;
    String error;
    bool has_activity = TaskTrackTunnelLoadActivity(activity, error);

    String out;
    out << "TaskTrack Tunnel diagnostics\n"
        << "Build: " << TaskTrackBuildVersion() << "\n"
        << "Profile: " << (profile ? profile->name : String("None")) << "\n"
        << "Tunnel: " << (profile ? profile->tunnel_id : String()) << "\n"
        << "State: ";

    switch(GetRuntimeState()) {
    case STATE_READY: out << "ready"; break;
    case STATE_CONNECTING: out << "connecting"; break;
    case STATE_ERROR: out << "error"; break;
    default: out << "stopped"; break;
    }

    out << "\nRuntime process: " << BoolText(runtime_started_) << "\n"
        << "Healthy: " << BoolText(runtime_healthy_) << "\n"
        << "Ready: " << BoolText(runtime_ready_) << "\n"
        << "Credential source: CONTROL_PLANE_API_KEY\n"
        << "Credential available: " << BoolText(!GetEnv("CONTROL_PLANE_API_KEY").IsEmpty()) << "\n"
        << "Secret value: [not exposed]\n"
        << "Runtime executable: " << (profile ? profile->runtime_path : String()) << "\n"
        << "TaskTrack MCP: " << (profile ? profile->mcp_path : String()) << "\n"
        << "Remote activity: " << (has_activity ? AsString(activity.received) : String("0"))
        << " in / " << (has_activity ? AsString(activity.sent) : String("0")) << " out\n";

    if(has_activity && (!activity.last_method.IsEmpty() || !activity.last_tool.IsEmpty()))
        out << "Last remote call: " << activity.last_method
            << (activity.last_tool.IsEmpty() ? String() : " / " + activity.last_tool) << "\n";
    if(!last_error_.IsEmpty())
        out << "Last error: " << last_error_ << "\n";
    return out;
}

void TaskTrackTunnelManager::CopyDiagnostics()
{
    WriteClipboardText(BuildDiagnostics());
}

void TaskTrackTunnelManager::ShowHelp()
{
    PromptOK(
        "TaskTrack Tunnel Manager\n\n"
        "Overview shows whether the local TaskTrack MCP is reachable through the OpenAI Secure MCP Tunnel and displays recent remote MCP traffic.\n\n"
        "Setup manages named, non-secret tunnel profiles. Tunnel IDs and executable paths may be stored locally. The CONTROL_PLANE_API_KEY secret is never stored or displayed by TaskTrack.\n\n"
        "Use one tunnel/profile per local machine. Stop the active tunnel before switching profiles.");
}

void TaskTrackTunnelManager::Tick()
{
    if(runtime_started_)
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
    nav_note_.SetRect(max(DPI(200), client.GetWidth() - DPI(180)), DPI(8), DPI(165), DPI(30));

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
    activity_title_.SetRect(DPI(13), DPI(8), DPI(120), DPI(26));
    activity_live_.SetRect(DPI(135), DPI(8), DPI(70), DPI(26));
    activity_count_.SetRect(max(0, ar.GetWidth() - DPI(190)), DPI(8), DPI(175), DPI(26));
    activity_table_.SetRect(DPI(1), DPI(42), max(0, ar.GetWidth() - DPI(2)), max(0, ar.GetHeight() - DPI(84)));
    copy_diagnostics_button_.SetRect(DPI(10), max(0, ar.GetHeight() - DPI(36)), DPI(116), DPI(27));
    clear_activity_button_.SetRect(DPI(133), max(0, ar.GetHeight() - DPI(36)), DPI(105), DPI(27));
    activity_footer_note_.SetRect(max(DPI(245), ar.GetWidth() - DPI(220)), max(0, ar.GetHeight() - DPI(36)), DPI(205), DPI(27));

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
                        max(DPI(360), sp.GetHeight() - DPI(16) - DPI(75) - gap - DPI(15)));

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
    tunnel_id_label_.SetRect(label_x, y + DPI(4), DPI(125), DPI(20));
    tunnel_id_edit_.SetRect(field_x, y, field_w, DPI(28)); y += DPI(34);
    credential_label_.SetRect(label_x, y + DPI(4), DPI(125), DPI(20));
    int credential_w = max(DPI(180), field_w - DPI(120));
    credential_edit_.SetRect(field_x, y, credential_w, DPI(28));
    credential_status_.SetRect(field_x + credential_w + DPI(5), y + DPI(2), DPI(110), DPI(24)); y += DPI(30);
    credential_note_.SetRect(field_x, y, field_w, DPI(18)); y += DPI(24);

    section_runtime_.SetRect(label_x, y, field_w, DPI(16)); y += DPI(22);
    runtime_path_label_.SetRect(label_x, y + DPI(4), DPI(125), DPI(20));
    runtime_path_edit_.SetRect(field_x, y, max(DPI(180), field_w - DPI(82)), DPI(28));
    runtime_browse_button_.SetRect(field_x + max(DPI(180), field_w - DPI(75)), y, DPI(75), DPI(28)); y += DPI(34);
    mcp_path_label_.SetRect(label_x, y + DPI(4), DPI(125), DPI(20));
    mcp_path_edit_.SetRect(field_x, y, max(DPI(180), field_w - DPI(82)), DPI(28));
    mcp_browse_button_.SetRect(field_x + max(DPI(180), field_w - DPI(75)), y, DPI(75), DPI(28)); y += DPI(36);

    section_launch_.SetRect(label_x, y, field_w, DPI(16)); y += DPI(22);
    auto_connect_label_.SetRect(label_x, y + DPI(3), DPI(125), DPI(20));
    auto_connect_toggle_.SetRect(field_x, y, DPI(38), DPI(22));
    auto_connect_title_.SetRect(field_x + DPI(50), y - DPI(2), field_w - DPI(50), DPI(18));
    auto_connect_note_.SetRect(field_x + DPI(50), y + DPI(15), field_w - DPI(50), DPI(16)); y += DPI(34);
    remember_label_.SetRect(label_x, y + DPI(3), DPI(125), DPI(20));
    remember_toggle_.SetRect(field_x, y, DPI(38), DPI(22));
    remember_title_.SetRect(field_x + DPI(50), y - DPI(2), field_w - DPI(50), DPI(18));
    remember_note_.SetRect(field_x + DPI(50), y + DPI(15), field_w - DPI(50), DPI(16));

    Rect fo = footer_.GetSize();
    footer_build_.SetRect(DPI(11), DPI(4), DPI(145), DPI(22));
    footer_mcp_.SetRect(DPI(165), DPI(4), DPI(100), DPI(22));
    footer_dashboard_.SetRect(DPI(275), DPI(4), DPI(130), DPI(22));
    footer_copy_.SetRect(max(0, fo.GetWidth() - DPI(118)), DPI(2), DPI(108), DPI(26));
    footer_help_.SetRect(max(0, fo.GetWidth() - DPI(170)), DPI(2), DPI(46), DPI(26));
}

}
