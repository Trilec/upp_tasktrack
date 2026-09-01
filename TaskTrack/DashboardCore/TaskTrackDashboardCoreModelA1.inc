String TaskTrackDashboardVersion()
{
    return "0.1.0";
}

String TaskTrackDashboardPanelTypeName(TaskTrackDashboardPanelType type)
{
    switch(type) {
    case TaskTrackDashboardPanelType::ProjectState: return "project_state";
    case TaskTrackDashboardPanelType::Timeline: return "timeline";
    case TaskTrackDashboardPanelType::ProgressList: return "progress_list";
    case TaskTrackDashboardPanelType::ActionList: return "action_list";
    case TaskTrackDashboardPanelType::Attention: return "attention";
    case TaskTrackDashboardPanelType::Verification: return "verification";
    case TaskTrackDashboardPanelType::Changes: return "changes";
    case TaskTrackDashboardPanelType::Records: return "records";
    }
    return "records";
}

bool TaskTrackDashboardParsePanelType(const String& text, TaskTrackDashboardPanelType& type)
{
    String s = ToLower(TrimBoth(text));
    if(s == "project_state" || s == "state") { type = TaskTrackDashboardPanelType::ProjectState; return true; }
    if(s == "timeline" || s == "milestones") { type = TaskTrackDashboardPanelType::Timeline; return true; }
    if(s == "progress_list" || s == "progress" || s == "objectives") { type = TaskTrackDashboardPanelType::ProgressList; return true; }
    if(s == "action_list" || s == "actions" || s == "next_steps") { type = TaskTrackDashboardPanelType::ActionList; return true; }
    if(s == "attention" || s == "risks" || s == "blockers") { type = TaskTrackDashboardPanelType::Attention; return true; }
    if(s == "verification" || s == "audit") { type = TaskTrackDashboardPanelType::Verification; return true; }
    if(s == "changes" || s == "history") { type = TaskTrackDashboardPanelType::Changes; return true; }
    if(s == "records" || s == "table") { type = TaskTrackDashboardPanelType::Records; return true; }
    return false;
}

String TaskTrackDashboardDensityName(TaskTrackDashboardDensity density)
{
    switch(density) {
    case TaskTrackDashboardDensity::Summary: return "summary";
    case TaskTrackDashboardDensity::Standard: return "standard";
    case TaskTrackDashboardDensity::Full: return "full";
    }
    return "standard";
}

bool TaskTrackDashboardParseDensity(const String& text, TaskTrackDashboardDensity& density)
{
    String s = ToLower(TrimBoth(text));
    if(s == "summary" || s == "compact") { density = TaskTrackDashboardDensity::Summary; return true; }
    if(s == "standard" || s.IsEmpty()) { density = TaskTrackDashboardDensity::Standard; return true; }
    if(s == "full" || s == "detail" || s == "detailed") { density = TaskTrackDashboardDensity::Full; return true; }
    return false;
}

String TaskTrackDashboardMakeId()
{
    Time t = GetSysTime();
    uint64 random = Random64();
    return Format("dashboard-%04d%02d%02d-%02d%02d%02d-%016llx",
                  (int)t.year, (int)t.month, (int)t.day,
                  (int)t.hour, (int)t.minute, (int)t.second, (int64)random);
}

String TaskTrackDashboardDefaultStoreRoot()
{
    return NormalizePath(AppendFileName(GetFileFolder(GetExeFilePath()), "tasktrack_dashboard_data"));
}
