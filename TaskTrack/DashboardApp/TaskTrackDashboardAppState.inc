bool TaskTrackDashboardWindow::LoadDashboard(const String& path, String& error)
{
    TaskTrackDashboardDocument loaded;
    if(!TaskTrackDashboardLoad(path, loaded, error))
        return false;

    dashboard_path_ = NormalizePath(path);
    store_root_ = GetFileFolder(dashboard_path_);
    display_ = pick(loaded);
    current_revision_ = display_.revision;
    viewing_revision_ = 0;
    loaded_ = true;
    selected_category_ = "All";
    expanded_panels_.Clear();

    category_.Enable(true);
    revision_.Enable(true);
    refresh_button_.Enable(true);
    current_button_.Enable(false);

    RefreshHeader();
    RebuildCategoryChoices();
    RebuildRevisionChoices(0);
    RebuildPanels();

    KillTimeCallback(TASKTRACK_DASHBOARD_REFRESH_TIMER_ID);
    SetTimeCallback(-2000, [=] { CheckForUpdate(); }, TASKTRACK_DASHBOARD_REFRESH_TIMER_ID);

    if(IsOpen())
        SetForeground();
    return true;
}

void TaskTrackDashboardWindow::ShowEmptyState()
{
    KillTimeCallback(TASKTRACK_DASHBOARD_REFRESH_TIMER_ID);
    loaded_ = false;
    dashboard_path_.Clear();
    store_root_.Clear();
    display_ = TaskTrackDashboardDocument();
    current_revision_ = 0;
    viewing_revision_ = 0;
    selected_category_ = "All";
    expanded_panels_.Clear();

    header_.SetTitle("TaskTrack Dashboard");
    header_.SetSubTitle("Native read-only view of AI-maintained project state");
    overall_ring_.Set(0, 100).NoPercent().SetText("—");
    title_label_.SetText("No dashboard loaded");
    phase_label_.SetText("Choose a dashboard to inspect current project state");
    meta_label_.SetText("Use Load dashboard, or open a dashboard from TaskTrack MCP.");
    attention_label_.SetText("No project state is currently displayed.");
    attention_label_.SetCustomStyle(LabelStyle(UiRole::Subtle, 8));

    syncing_controls_ = true;
    category_.UseInternalModel().Clear();
    revision_.UseInternalModel().Clear();
    syncing_controls_ = false;
    category_.Enable(false);
    revision_.Enable(false);
    refresh_button_.Enable(false);
    current_button_.Enable(false);

    String message = "No dashboard loaded.\n\nUse Load dashboard to choose a dashboard file.";
    ValueArray recent;
    String error;
    if(TaskTrackDashboardList(String(), 5, recent, error) && !recent.IsEmpty()) {
        message << "\n\nRecent dashboards:";
        for(int i = 0; i < recent.GetCount(); ++i) {
            Value row = recent[i];
            String title = AsString(row["title"]);
            if(title.IsEmpty())
                title = AsString(row["dashboard_id"]);
            message << "\n  • " << title;
            String updated = AsString(row["updated_at"]);
            if(!updated.IsEmpty())
                message << "  ·  " << updated;
        }
        footer_status_.SetText(Format("Ready · %d recent dashboard%s available",
                                      recent.GetCount(), recent.GetCount() == 1 ? "" : "s"));
    }
    else
        footer_status_.SetText("Ready · no dashboards found in the default store");

    empty_label_.SetText(message).SetAlign(UiAlign::LEFT, UiAlign::TOP);
    panels_box_.ClearItems();
    panel_controls_.Clear();
    panels_box_.Add(empty_label_).Fixed(DPI(190)).AlignSelf(UiCrossAlign::Stretch);
    panels_box_.RefreshLayout();
    SyncScrollContent();
    scroll_.RefreshLayout();
    Refresh();
}

bool TaskTrackDashboardWindow::PromptLoadDashboard(String& error)
{
    error.Clear();
    FileSel selector;
    selector.Type("TaskTrack dashboard", "*.tasktrack-dashboard.json");

    ValueArray recent;
    String recent_error;
    if(TaskTrackDashboardList(String(), 1, recent, recent_error) && !recent.IsEmpty()) {
        String path = AsString(recent[0]["dashboard_path"]);
        if(!path.IsEmpty())
            selector.Set(path);
    }

    if(!selector.ExecuteOpen("Load TaskTrack Dashboard"))
        return false;

    if(!LoadDashboard(~selector, error))
        return false;

    SetForeground();
    return true;
}

void TaskTrackDashboardWindow::RefreshHeader()
{
    if(!loaded_)
        return;
    double progress = TaskTrackDashboardEffectiveProgress(display_);
    if(progress >= 0)
        overall_ring_.ClearText().Percent(true).Set((int)(progress + 0.5), 100);
    else
        overall_ring_.Set(0, 100).NoPercent().SetText("—");

    header_.SetTitle(display_.project.IsEmpty() ? String("Project dashboard") : display_.project);
    header_.SetSubTitle(display_.subtitle);
    title_label_.SetText(display_.title);
    phase_label_.SetText(display_.phase.IsEmpty() ? String("Current state") : display_.phase);

    String meta;
    meta << "revision " << display_.revision;
    if(viewing_revision_ > 0)
        meta << " (historical)";
    if(!display_.updated_at.IsEmpty())
        meta << " · updated " << display_.updated_at;
    if(!display_.updated_by.IsEmpty())
        meta << " by " << display_.updated_by;
    if(!display_.source_head.IsEmpty())
        meta << " · source " << display_.source_head;
    meta_label_.SetText(meta);

    int attention = TaskTrackDashboardAttentionCount(display_);
    String at = Format("%d attention item%s", attention, attention == 1 ? "" : "s");
    if(display_.confidence >= 0)
        at << Format(" · confidence %.0f%%", display_.confidence);
    if(progress >= 0)
        at << Format(" · progress %.0f%%", progress);
    attention_label_.SetText(at);
    attention_label_.SetCustomStyle(LabelStyle(attention > 0 ? UiRole::Alert : UiRole::Subtle,
                                                8, attention > 0));
    current_button_.Enable(viewing_revision_ != 0);
    footer_status_.SetText(viewing_revision_ == 0
        ? Format("Current revision %d · auto-refresh every 2 seconds · double-click a panel to inspect all records",
                 current_revision_)
        : Format("Viewing immutable revision %d · current revision is %d",
                 viewing_revision_, current_revision_));
}

void TaskTrackDashboardWindow::RebuildCategoryChoices()
{
    syncing_controls_ = true;
    category_.UseInternalModel().Clear();
    category_.Add("All", "All");
    for(const String& c : TaskTrackDashboardCategories(display_))
        category_.Add(c, c);
    if(!category_.SelectByData(selected_category_).HasSelection()) {
        selected_category_ = "All";
        category_.SelectByData("All");
    }
    syncing_controls_ = false;
}

void TaskTrackDashboardWindow::RebuildRevisionChoices(int select_revision)
{
    if(!loaded_)
        return;
    syncing_controls_ = true;
    revision_.UseInternalModel().Clear();
    revision_.Add(Format("Current · r%d", current_revision_), 0);
    ValueArray revisions;
    String error;
    if(TaskTrackDashboardListRevisions(display_.dashboard_id, store_root_, 200, revisions, error))
        for(int i = 0; i < revisions.GetCount(); ++i) {
            Value row = revisions[i];
            int r = IsNumber(row["revision"]) ? (int)row["revision"] : 0;
            if(r > 0)
                revision_.Add(Format("r%d · %s", r, AsString(row["updated_at"])), r);
        }
    revision_.SelectByData(max(0, select_revision));
    syncing_controls_ = false;
}

bool TaskTrackDashboardWindow::PanelMatchesCategory(const TaskTrackDashboardPanel& p) const
{
    if(selected_category_.IsEmpty() || selected_category_ == "All")
        return true;
    if(p.category == selected_category_)
        return true;
    for(const auto& e : p.entries)
        if(e.category == selected_category_)
            return true;
    return false;
}

void TaskTrackDashboardWindow::RebuildPanels()
{
    if(!loaded_)
        return;
    panels_box_.ClearItems();
    panel_controls_.Clear();
    int shown = 0;
    double progress = TaskTrackDashboardEffectiveProgress(display_);
    for(const auto& p : display_.panels) {
        if(!PanelMatchesCategory(p))
            continue;
        auto& ctrl = panel_controls_.Add();
        bool expanded = expanded_panels_.Find(p.id) >= 0;
        ctrl.Bind(p, progress, expanded);
        ctrl.WhenExpansionChanged = [=](const String& id) { ToggleExpanded(id); };
        panels_box_.Add(ctrl).Fit().MinMain(ctrl.GetMinSize().cy).AlignSelf(UiCrossAlign::Stretch);
        ++shown;
    }
    if(!shown) {
        empty_label_.SetText("No dashboard panels match this category.")
                    .SetAlign(UiAlign::CENTER, UiAlign::CENTER);
        panels_box_.Add(empty_label_).Fixed(DPI(54)).AlignSelf(UiCrossAlign::Stretch);
    }
    panels_box_.RefreshLayout();
    SyncScrollContent();
    scroll_.RefreshLayout();
    Refresh();
}
