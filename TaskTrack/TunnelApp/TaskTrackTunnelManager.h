#ifndef _TaskTrack_TunnelApp_TaskTrackTunnelManager_h_
#define _TaskTrack_TunnelApp_TaskTrackTunnelManager_h_

#include <Core/Core.h>
#include <Ui/Ui.h>
#include <McpTunnelRuntime/McpTunnelRuntime.h>
#include <TaskTrack/Core/TaskTrackBuild.h>
#include <TaskTrack/TunnelCore/TaskTrackTunnelCore.h>

namespace Upp {

struct TaskTrackTunnelManagerOptions {
    String tunnel_id;
    String runtime_path;
    String alias = "tasktrack-browser";
};

class TaskTrackTunnelManager : public TopWindow {
public:
    typedef TaskTrackTunnelManager CLASSNAME;

    TaskTrackTunnelManager(const TaskTrackTunnelManagerOptions& options);
    ~TaskTrackTunnelManager();

    virtual void Layout() override;

private:
    enum Page {
        PAGE_OVERVIEW = 0,
        PAGE_SETUP = 1,
        PAGE_SERVICES = 2,
    };

    TaskTrackTunnelManagerOptions options_;
    Vector<McpTunnelProfile> profiles_;
    int selected_profile_ = -1;
    int selected_service_ = -1;
    bool dark_theme_ = false;
    bool loading_profile_ = false;
    bool loading_service_ = false;

    McpTunnelRuntime runtime_;
    McpTunnelSessionCredentials session_credentials_;

    UiPanel root_;
    UiTitleCard header_;
    UiBoxLayout header_actions_{UiDirection::H};
    UiToolButton theme_button_, help_button_, exit_button_;

    UiPanel nav_;
    UiToolButton overview_button_, setup_button_, services_button_;
    UiLabel nav_note_;

    UiStack pages_;
    UiPanel overview_page_, setup_page_, services_page_;

    UiPanel hero_;
    UiPanel beacon_;
    UiLabel beacon_core_;
    UiLabel state_eyebrow_, state_title_, state_subtitle_;
    UiLabel profile_caption_, profile_value_;
    UiLabel tunnel_caption_, tunnel_value_;
    UiLabel sync_caption_, sync_value_;
    UiButton primary_button_, health_button_;

    UiPanel status_strip_;
    UiPanel status_cell_[4];
    UiLabel status_caption_[4];
    UiLabel status_value_[4];

    UiPanel activity_panel_;
    UiLabel activity_title_, activity_live_, activity_count_;
    UiTable activity_table_;
    UiTableModel activity_model_;
    UiButton send_probe_button_, copy_diagnostics_button_, clear_activity_button_;
    UiLabel activity_footer_note_;

    UiPanel profile_bar_;
    UiLabel profile_select_caption_;
    UiDropdown profile_dropdown_;
    UiButton new_profile_button_, duplicate_profile_button_, delete_profile_button_;

    UiPanel setup_form_;
    UiLabel section_profile_, section_runtime_, section_launch_;
    UiLabel profile_name_label_, machine_id_label_, tunnel_id_label_, credential_label_;
    UiLineEdit profile_name_edit_, machine_id_edit_, tunnel_id_edit_;
    UiDropdown credential_source_dropdown_;
    UiLabel credential_status_;
    UiButton credential_set_button_, credential_clear_button_;
    UiLabel credential_note_;
    UiLabel runtime_path_label_;
    UiLineEdit runtime_path_edit_;
    UiButton runtime_browse_button_;
    UiLabel auto_connect_label_, auto_connect_title_, auto_connect_note_;
    UiToggle auto_connect_toggle_;
    UiLabel remember_label_, remember_title_, remember_note_;
    UiToggle remember_toggle_;

    UiPanel service_bar_, service_form_;
    UiLabel service_select_caption_, section_service_;
    UiDropdown service_dropdown_;
    UiButton new_service_button_, duplicate_service_button_, delete_service_button_;
    UiLabel service_name_label_, service_id_label_, service_channel_label_, service_command_label_;
    UiLineEdit service_name_edit_, service_id_edit_, service_channel_edit_, service_command_edit_;
    UiButton service_browse_button_;
    UiLabel service_enabled_label_, service_enabled_title_, service_enabled_note_;
    UiToggle service_enabled_toggle_;

    UiPanel footer_;
    UiLabel footer_build_, footer_mcp_, footer_dashboard_;
    UiButton footer_help_, footer_copy_;

    void BuildUi();
    void BuildOverview();
    void BuildSetup();
    void BuildServices();
    void Wire();

    void ApplyTheme();
    void ToggleTheme();
    void ConfigureNavButton(UiToolButton& button);
    void SelectPage(int page);

    Color SurfaceColor() const;
    Color SubtleColor() const;
    Color LineColor() const;
    Color TextColor() const;
    Color MutedColor() const;
    Color SoftColor() const;
    Color AccentColor() const;
    Color AccentStrongColor() const;
    Color OkColor() const;
    Color ActivityColor() const;
    Color DangerColor() const;
    Color StoppedColor() const;

    UiPanel::Style MakePanelStyle(Color face, int radius = 10, Color frame = Null) const;
    UiLabel::Style MakeLabelStyle(Color ink, int height = 11, bool bold = false) const;
    UiButton::Style MakeButtonStyle(UiButtonRole role = UiButtonRole::Standard) const;

    void LoadProfiles();
    void SaveProfiles();
    void EnsureDefaultProfile();
    McpTunnelProfile* CurrentProfile();
    const McpTunnelProfile* CurrentProfile() const;
    String NewProfileId() const;
    void RebuildProfileDropdown();
    void LoadProfileIntoUi();
    void SaveProfileFromUi();
    void SelectProfileById(const String& id);
    void NewProfile();
    void DuplicateProfile();
    void DeleteProfile();

    McpTunnelService* CurrentService();
    const McpTunnelService* CurrentService() const;
    String NewServiceId(const String& base = "service") const;
    String NewServiceChannel(const String& base = "service") const;
    void RebuildServiceDropdown();
    void LoadServiceIntoUi();
    void SaveServiceFromUi();
    void SelectServiceById(const String& id);
    void NewService();
    void DuplicateService();
    void DeleteService();

    void BrowseRuntime();
    void BrowseServiceCommand();

    bool CredentialAvailable(String& error) const;
    bool ReadCredential(String& secret, String& error) const;
    void RefreshCredentialProjection();
    void SetCredential();
    void ClearCredential();

    void ConnectRuntime();
    void RefreshRuntimeStatus(bool show_dialog = false);
    void StopRuntime();
    void OpenHealth();

    void RefreshProjection();
    void RefreshActivity();
    void RefreshActivityTable(const TaskTrackTunnelActivity& activity);
    void SendProbe();
    void ClearActivity();
    void CopyDiagnostics();
    String BuildDiagnostics() const;

    void ShowHelp();
    void Tick();

    String ProfileStorePath() const;
    const McpTunnelService* TaskTrackService() const;
};

}

#endif
