bool TaskTrackDashboardValidate(const TaskTrackDashboardDocument& doc, String& error)
{
    error.Clear();
    if(doc.schema_version != TASKTRACK_DASHBOARD_SCHEMA_VERSION) {
        error = "TaskTrack dashboard schema_version is invalid.";
        return false;
    }
    if(!IsSafeDashboardId(doc.dashboard_id)) {
        error = "dashboard_id must begin with dashboard- and contain only letters, digits, '-' or '_'.";
        return false;
    }
    if(doc.title.IsEmpty()) {
        error = "TaskTrack dashboard title is required.";
        return false;
    }
    if(doc.overall_progress < -1.0 || doc.overall_progress > 100.0) {
        error = "overall_progress must be 0..100 when supplied.";
        return false;
    }
    if(doc.confidence < -1.0 || doc.confidence > 100.0) {
        error = "confidence must be 0..100 when supplied.";
        return false;
    }
    if(doc.revision < 0) {
        error = "revision cannot be negative.";
        return false;
    }
    if(doc.panels.IsEmpty()) {
        error = "TaskTrack dashboard requires at least one panel.";
        return false;
    }
    if(doc.panels.GetCount() > TASKTRACK_DASHBOARD_MAX_PANELS) {
        error = "TaskTrack dashboard has too many panels.";
        return false;
    }

    Index<String> panel_ids;
    Index<String> entry_ids;
    for(const TaskTrackDashboardPanel& panel : doc.panels) {
        if(panel.id.IsEmpty() || panel_ids.Find(panel.id) >= 0) {
            error = "Dashboard panel ids must be unique and non-empty.";
            return false;
        }
        panel_ids.Add(panel.id);
        if(panel.title.IsEmpty()) {
            error = "Dashboard panel titles must be non-empty.";
            return false;
        }
        if((int)panel.type < (int)TaskTrackDashboardPanelType::ProjectState ||
           (int)panel.type > (int)TaskTrackDashboardPanelType::Records) {
            error = "Dashboard panel type is invalid.";
            return false;
        }
        if((int)panel.density < (int)TaskTrackDashboardDensity::Summary ||
           (int)panel.density > (int)TaskTrackDashboardDensity::Full) {
            error = "Dashboard panel density is invalid.";
            return false;
        }
        if(panel.preview_limit < 1 || panel.preview_limit > 100) {
            error = "Dashboard panel preview_limit must be 1..100.";
            return false;
        }
        if(panel.entries.GetCount() > TASKTRACK_DASHBOARD_MAX_ENTRIES_PER_PANEL) {
            error = "Dashboard panel contains too many entries.";
            return false;
        }
        for(const TaskTrackDashboardEntry& entry : panel.entries) {
            if(entry.id.IsEmpty() || entry_ids.Find(entry.id) >= 0) {
                error = "Dashboard entry ids must be globally unique and non-empty.";
                return false;
            }
            entry_ids.Add(entry.id);
            if(entry.title.IsEmpty()) {
                error = "Dashboard entry titles must be non-empty.";
                return false;
            }
            if(entry.progress < -1.0 || entry.progress > 100.0) {
                error = "Dashboard entry progress must be 0..100 when supplied.";
                return false;
            }
            if(entry.weight <= 0.0 || entry.weight > 1000000.0) {
                error = "Dashboard entry weight must be greater than zero and bounded.";
                return false;
            }
        }
    }
    return true;
}
