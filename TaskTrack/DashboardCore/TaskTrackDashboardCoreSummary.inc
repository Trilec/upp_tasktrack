double TaskTrackDashboardDerivedProgress(const TaskTrackDashboardDocument& doc)
{
    double weighted = 0.0, weight_total = 0.0;
    for(const TaskTrackDashboardPanel& panel : doc.panels) {
        if(!panel.contributes_to_progress) continue;
        for(const TaskTrackDashboardEntry& entry : panel.entries) {
            if(entry.archived || entry.progress < 0.0) continue;
            weighted += entry.progress * entry.weight;
            weight_total += entry.weight;
        }
    }
    return weight_total > 0.0 ? weighted / weight_total : -1.0;
}

double TaskTrackDashboardEffectiveProgress(const TaskTrackDashboardDocument& doc)
{
    return doc.overall_progress >= 0.0 ? doc.overall_progress : TaskTrackDashboardDerivedProgress(doc);
}

int TaskTrackDashboardAttentionCount(const TaskTrackDashboardDocument& doc)
{
    int count = 0;
    for(const TaskTrackDashboardPanel& panel : doc.panels)
        for(const TaskTrackDashboardEntry& entry : panel.entries)
            if(!entry.archived && (entry.attention || StatusImpliesAttention(entry.status))) ++count;
    return count;
}

Vector<String> TaskTrackDashboardCategories(const TaskTrackDashboardDocument& doc)
{
    Index<String> categories;
    for(const TaskTrackDashboardPanel& panel : doc.panels) {
        if(!panel.category.IsEmpty()) categories.FindAdd(panel.category);
        for(const TaskTrackDashboardEntry& entry : panel.entries)
            if(!entry.category.IsEmpty()) categories.FindAdd(entry.category);
    }
    Vector<String> out;
    for(int i = 0; i < categories.GetCount(); ++i) out.Add(categories[i]);
    Sort(out);
    return out;
}

Value TaskTrackDashboardStatusValue(const TaskTrackDashboardDocument& doc,
                                    const String& path, bool include_panels)
{
    ValueMap root;
    root.Add("ok", true);
    root.Add("schema_version", TASKTRACK_DASHBOARD_SCHEMA_VERSION);
    root.Add("dashboard_version", TaskTrackDashboardVersion());
    root.Add("dashboard_id", doc.dashboard_id);
    root.Add("dashboard_path", NormalizePath(path));
    root.Add("project", doc.project);
    root.Add("title", doc.title);
    root.Add("subtitle", doc.subtitle);
    root.Add("phase", doc.phase);
    root.Add("revision", doc.revision);
    root.Add("created_at", doc.created_at);
    root.Add("updated_at", doc.updated_at);
    root.Add("updated_by", doc.updated_by);
    root.Add("source_head", doc.source_head);
    root.Add("effective_progress", EmptyProgressValue(TaskTrackDashboardEffectiveProgress(doc)));
    root.Add("derived_progress", EmptyProgressValue(TaskTrackDashboardDerivedProgress(doc)));
    root.Add("explicit_progress", EmptyProgressValue(doc.overall_progress));
    root.Add("confidence", EmptyProgressValue(doc.confidence));
    root.Add("attention_count", TaskTrackDashboardAttentionCount(doc));
    root.Add("panel_count", doc.panels.GetCount());
    if(include_panels) root.Add("dashboard", TaskTrackDashboardToValue(doc));
    return Value(root);
}
