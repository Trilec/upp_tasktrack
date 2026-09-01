bool TaskTrackDashboardLoadRevision(const String& dashboard_id, const String& store_root,
                                    int revision, TaskTrackDashboardDocument& doc,
                                    String& path, String& error)
{
    if(!IsSafeDashboardId(dashboard_id)) { error = "Invalid TaskTrack dashboard_id."; return false; }
    if(revision <= 0) { error = "revision must be greater than zero."; return false; }
    path = TaskTrackDashboardRevisionPath(store_root, dashboard_id, revision);
    if(!FileExists(path)) { error = Format("Dashboard %s revision %d not found.", dashboard_id, revision); return false; }
    if(!TaskTrackDashboardLoad(path, doc, error)) return false;
    if(doc.dashboard_id != dashboard_id || doc.revision != revision) {
        error = "Dashboard revision file identity does not match its requested revision.";
        return false;
    }
    return true;
}

bool TaskTrackDashboardList(const String& store_root, int limit, ValueArray& result, String& error)
{
    error.Clear(); result.Clear();
    String root = EnsureStoreRoot(store_root);
    if(limit <= 0) limit = 20;
    limit = min(limit, 200);
    Vector<String> paths;
    FindFile ff(AppendFileName(root, "*.tasktrack-dashboard.json"));
    while(ff) { if(ff.IsFile()) paths.Add(AppendFileName(root, ff.GetName())); ff.Next(); }
    Sort(paths);
    for(int i = paths.GetCount() - 1; i >= 0 && result.GetCount() < limit; --i) {
        TaskTrackDashboardDocument item; String load_error;
        if(TaskTrackDashboardLoad(paths[i], item, load_error))
            result.Add(TaskTrackDashboardStatusValue(item, paths[i], false));
    }
    return true;
}

bool TaskTrackDashboardListRevisions(const String& dashboard_id, const String& store_root,
                                     int limit, ValueArray& result, String& error)
{
    error.Clear(); result.Clear();
    if(!IsSafeDashboardId(dashboard_id)) { error = "Invalid TaskTrack dashboard_id."; return false; }
    if(limit <= 0) limit = 50;
    limit = min(limit, 500);
    String folder = TaskTrackDashboardRevisionFolder(store_root, dashboard_id);
    if(!DirectoryExists(folder)) return true;
    Vector<String> paths;
    FindFile ff(AppendFileName(folder, "*.json"));
    while(ff) { if(ff.IsFile()) paths.Add(AppendFileName(folder, ff.GetName())); ff.Next(); }
    Sort(paths);
    for(int i = paths.GetCount() - 1; i >= 0 && result.GetCount() < limit; --i) {
        TaskTrackDashboardDocument item; String load_error;
        if(!TaskTrackDashboardLoad(paths[i], item, load_error)) continue;
        ValueMap row;
        row.Add("dashboard_id", item.dashboard_id);
        row.Add("revision", item.revision);
        row.Add("updated_at", item.updated_at);
        row.Add("updated_by", item.updated_by);
        row.Add("source_head", item.source_head);
        row.Add("effective_progress", EmptyProgressValue(TaskTrackDashboardEffectiveProgress(item)));
        row.Add("attention_count", TaskTrackDashboardAttentionCount(item));
        row.Add("revision_path", NormalizePath(paths[i]));
        result.Add(row);
    }
    return true;
}
