bool TaskTrackDashboardSaveCurrent(const String& path, const TaskTrackDashboardDocument& doc, String& error)
{
    if(path.IsEmpty()) { error = "TaskTrack dashboard save path is empty."; return false; }
    if(doc.revision <= 0) { error = "TaskTrack dashboard cannot save without a positive revision."; return false; }
    String json = TaskTrackDashboardToJson(doc, true);
    TaskTrackDashboardDocument verify;
    if(!ParseDashboardText(json, verify, error)) {
        error = "TaskTrack dashboard refused to save invalid state: " + error;
        return false;
    }
    return AtomicSaveText(NormalizePath(path), json, error);
}

bool TaskTrackDashboardLoad(const String& path, TaskTrackDashboardDocument& doc, String& error)
{
    String normalized = NormalizePath(path);
    String text = LoadFile(normalized);
    if(ParseDashboardText(text, doc, error) && doc.revision > 0)
        return true;
    String primary_error = error;
    String backup = normalized + ".bak";
    if(FileExists(backup)) {
        TaskTrackDashboardDocument recovered;
        String backup_error;
        if(ParseDashboardText(LoadFile(backup), recovered, backup_error) && recovered.revision > 0) {
            doc = pick(recovered);
            error = "Primary dashboard was invalid; loaded recovery backup.";
            return true;
        }
    }
    error = primary_error.IsEmpty() ? String("Dashboard revision is invalid.") : primary_error;
    return false;
}

bool TaskTrackDashboardResolvePath(const String& dashboard_id, const String& store_root,
                                   String& path, String& error)
{
    if(!IsSafeDashboardId(dashboard_id)) { error = "Invalid TaskTrack dashboard_id."; return false; }
    path = TaskTrackDashboardMakePath(store_root, dashboard_id);
    if(FileExists(path) || FileExists(path + ".bak")) return true;
    error = "TaskTrack dashboard not found: " + dashboard_id;
    path.Clear();
    return false;
}
