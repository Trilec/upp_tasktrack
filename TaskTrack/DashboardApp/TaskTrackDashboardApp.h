#ifndef _TaskTrack_DashboardApp_TaskTrackDashboardApp_h_
#define _TaskTrack_DashboardApp_TaskTrackDashboardApp_h_

#include <Ui/Ui.h>
#include <TaskTrack/DashboardCore/TaskTrackDashboardCore.h>
#include <TaskTrack/DashboardWidgets/TaskTrackDashboardWidgets.h>

namespace Upp {
class TaskTrackDashboardWindow : public TopWindow {
public:
    typedef TaskTrackDashboardWindow CLASSNAME;
    TaskTrackDashboardWindow();
    ~TaskTrackDashboardWindow() override;
    bool LoadDashboard(const String& path, String& error);
    virtual void Layout() override;
private:
    void BuildUi(); void Wire(); void RefreshHeader(); void RebuildCategoryChoices();
    void RebuildRevisionChoices(int select_revision=0); void RebuildPanels();
    void SyncScrollContent(); void RefreshCurrent(bool force_view); void LoadRevision(int revision);
    void CheckForUpdate(); bool PanelMatchesCategory(const TaskTrackDashboardPanel& panel) const;
    void ToggleExpanded(const String& panel_id);
    String dashboard_path_, store_root_; TaskTrackDashboardDocument display_;
    int current_revision_=0, viewing_revision_=0; String selected_category_="All";
    bool loaded_=false, syncing_controls_=false, laying_out_=false; Index<String> expanded_panels_;
    UiPanel surface_; UiBoxLayout root_{UiDirection::V}; UiGroupPanel header_; UiBoxLayout header_box_{UiDirection::H};
    UiProgressRing overall_ring_; UiBoxLayout summary_box_{UiDirection::V}; UiLabel title_label_,phase_label_,meta_label_,attention_label_;
    UiBoxLayout controls_{UiDirection::H}; UiLabel category_label_; UiDropdown category_; UiLabel revision_label_; UiDropdown revision_;
    UiButton refresh_button_,current_button_,close_button_; UiScrollPanel scroll_; UiBoxLayout panels_box_{UiDirection::V};
    Array<TaskTrackDashboardPanelCtrl> panel_controls_; UiLabel empty_label_; UiBoxLayout footer_{UiDirection::H}; UiLabel footer_status_;
};
}
#endif
