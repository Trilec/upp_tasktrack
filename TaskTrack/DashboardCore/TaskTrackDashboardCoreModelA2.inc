String TaskTrackDashboardMakePath(const String& store_root, const String& dashboard_id)
{
    return AppendFileName(EnsureStoreRoot(store_root), dashboard_id + ".tasktrack-dashboard.json");
}

String TaskTrackDashboardRevisionFolder(const String& store_root, const String& dashboard_id)
{
    return AppendFileName(AppendFileName(EnsureStoreRoot(store_root), "revisions"), dashboard_id);
}

String TaskTrackDashboardRevisionPath(const String& store_root, const String& dashboard_id, int revision)
{
    return AppendFileName(TaskTrackDashboardRevisionFolder(store_root, dashboard_id),
                          Format("%08d.json", max(0, revision)));
}

Value TaskTrackDashboardEntryToValue(const TaskTrackDashboardEntry& entry)
{
    ValueMap m;
    m.Add("id", entry.id);
    m.Add("title", entry.title);
    if(!entry.subtitle.IsEmpty()) m.Add("subtitle", entry.subtitle);
    if(!entry.status.IsEmpty()) m.Add("status", entry.status);
    if(!entry.detail.IsEmpty()) m.Add("detail", entry.detail);
    if(!entry.category.IsEmpty()) m.Add("category", entry.category);
    if(entry.progress >= 0.0) m.Add("progress", entry.progress);
    if(entry.weight != 1.0) m.Add("weight", entry.weight);
    if(entry.attention) m.Add("attention", true);
    if(entry.archived) m.Add("archived", true);
    if(!entry.timestamp.IsEmpty()) m.Add("timestamp", entry.timestamp);
    if(!entry.task_id.IsEmpty()) m.Add("task_id", entry.task_id);
    if(!entry.evidence.IsEmpty()) m.Add("evidence", entry.evidence);
    if(!IsNull(entry.data)) m.Add("data", entry.data);
    return Value(m);
}

Value TaskTrackDashboardPanelToValue(const TaskTrackDashboardPanel& panel)
{
    ValueMap m;
    m.Add("id", panel.id);
    m.Add("type", TaskTrackDashboardPanelTypeName(panel.type));
    m.Add("title", panel.title);
    if(!panel.subtitle.IsEmpty()) m.Add("subtitle", panel.subtitle);
    if(!panel.category.IsEmpty()) m.Add("category", panel.category);
    m.Add("density", TaskTrackDashboardDensityName(panel.density));
    m.Add("preview_limit", panel.preview_limit);
    m.Add("contributes_to_progress", panel.contributes_to_progress);
    ValueArray entries;
    for(const TaskTrackDashboardEntry& entry : panel.entries)
        entries.Add(TaskTrackDashboardEntryToValue(entry));
    m.Add("entries", entries);
    return Value(m);
}

Value TaskTrackDashboardToValue(const TaskTrackDashboardDocument& doc)
{
    ValueMap root;
    root.Add("schema_version", TASKTRACK_DASHBOARD_SCHEMA_VERSION);
    root.Add("dashboard_version", TaskTrackDashboardVersion());
    root.Add("dashboard_id", doc.dashboard_id);
    root.Add("project", doc.project);
    root.Add("title", doc.title);
    if(!doc.subtitle.IsEmpty()) root.Add("subtitle", doc.subtitle);
    if(!doc.phase.IsEmpty()) root.Add("phase", doc.phase);
    if(doc.overall_progress >= 0.0) root.Add("overall_progress", doc.overall_progress);
    if(doc.confidence >= 0.0) root.Add("confidence", doc.confidence);
    root.Add("revision", doc.revision);
    root.Add("created_at", doc.created_at);
    root.Add("updated_at", doc.updated_at);
    root.Add("updated_by", doc.updated_by);
    if(!doc.source_head.IsEmpty()) root.Add("source_head", doc.source_head);
    ValueArray panels;
    for(const TaskTrackDashboardPanel& panel : doc.panels)
        panels.Add(TaskTrackDashboardPanelToValue(panel));
    root.Add("panels", panels);
    return Value(root);
}
