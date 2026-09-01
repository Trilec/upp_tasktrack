Value BuildDashboard(const String& id, double active_progress = 50.0)
{
    ValueArray panels;
    ValueArray a;

    a.Clear();
    a.Add(Entry("state-now", "Current implementation", "active", -1));
    a.Add(Entry("state-next", "Native validation", "next", -1));
    panels.Add(Panel("state", "project_state", "Project state", a));

    a.Clear();
    a.Add(Entry("m1", "Architecture", "complete", 100, 1));
    a.Add(Entry("m2", "Implementation", "active", active_progress, 2));
    a.Add(Entry("m3", "Acceptance", "next", 0, 1));
    panels.Add(Panel("timeline", "timeline", "Milestones", a, true));

    a.Clear();
    a.Add(Entry("obj1", "Persistence", "complete", 100, 1));
    a.Add(Entry("obj2", "Native dashboard", "active", active_progress, 1));
    panels.Add(Panel("objectives", "progress_list", "Objectives", a, false));

    a.Clear(); a.Add(Entry("next1", "Run Windows validation", "next", -1));
    panels.Add(Panel("next", "action_list", "Next steps", a));

    a.Clear(); a.Add(Entry("attention1", "Visual acceptance pending", "attention", -1, 1, true));
    panels.Add(Panel("attention", "attention", "Needs attention", a));

    a.Clear(); a.Add(Entry("verify1", "Static source gate", "complete", 100));
    panels.Add(Panel("verification", "verification", "Verification", a));

    a.Clear(); a.Add(Entry("change1", "Dashboard subsystem added", "complete", -1));
    panels.Add(Panel("changes", "changes", "Changes", a));

    a.Clear(); a.Add(Entry("record1", "Source head", "info", -1));
    panels.Add(Panel("records", "records", "Project data", a));

    ValueMap d;
    d.Add("schema_version", 1);
    d.Add("dashboard_id", id);
    d.Add("project", "TaskTrackDashboardTests");
    d.Add("title", "Dashboard acceptance");
    d.Add("phase", "Implementation");
    d.Add("confidence", 90);
    d.Add("updated_by", "test");
    d.Add("source_head", "test-head");
    d.Add("panels", panels);
    return d;
}

void Cleanup(const String& root, const String& id)
{
    String path = TaskTrackDashboardMakePath(root, id);
    FileDelete(path);
    FileDelete(path + ".bak");
    FileDelete(path + ".tmp");
    FileDelete(AppendFileName(root, id + ".tasktrack-dashboard.lock"));
    DeleteFolderDeep(TaskTrackDashboardRevisionFolder(root, id));
}

} // namespace
