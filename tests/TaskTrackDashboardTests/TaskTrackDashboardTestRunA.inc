    TestState t;
    String root = NormalizePath(AppendFileName(GetFileFolder(GetExeFilePath()), "_tasktrack_dashboard_tests"));
    RealizeDirectory(root);
    String id = "dashboard-core-regression";
    Cleanup(root, id);

    String error, code, path;
    TaskTrackDashboardDocument parsed;
    Value dashboard = BuildDashboard(id, 50);
    t.Check(TaskTrackDashboardFromValue(dashboard, parsed, error), "all panel types did not parse: " + error);
    t.Check(parsed.panels.GetCount() == 8, "expected eight semantic panel types");
    t.Check(TaskTrackDashboardAttentionCount(parsed) == 1, "attention count mismatch");
    t.Check(fabs(TaskTrackDashboardDerivedProgress(parsed) - 50.0) < 0.01,
            "weighted derived progress mismatch");
    t.Check(fabs(TaskTrackDashboardEffectiveProgress(parsed) - 50.0) < 0.01,
            "effective progress should use derived progress when explicit value is absent");

    TaskTrackDashboardDocument explicit_progress;
    ValueMap explicit_value = dashboard;
    explicit_value.Add("overall_progress", 68);
    t.Check(TaskTrackDashboardFromValue(explicit_value, explicit_progress, error) &&
            fabs(TaskTrackDashboardEffectiveProgress(explicit_progress) - 68.0) < 0.01,
            "explicit overall progress was not authoritative");

    ValueMap duplicate = dashboard;
    ValueArray dup_panels = duplicate["panels"];
    ValueMap second = dup_panels[1];
    ValueArray second_entries = second["entries"];
    ValueMap duplicate_entry = second_entries[0];
    duplicate_entry.Set("id", "state-now");
    second_entries.Set(0, duplicate_entry);
    second.Set("entries", second_entries);
    dup_panels.Set(1, second);
    duplicate.Set("panels", dup_panels);
    TaskTrackDashboardDocument invalid_doc;
    t.Check(!TaskTrackDashboardFromValue(duplicate, invalid_doc, error),
            "global duplicate entry id was accepted");

    ValueMap bad_progress = dashboard;
    ValueArray bad_panels = bad_progress["panels"];
    ValueMap bad_panel = bad_panels[1];
    ValueArray bad_entries = bad_panel["entries"];
    ValueMap bad_entry = bad_entries[1];
    bad_entry.Set("progress", 101);
    bad_entries.Set(1, bad_entry);
    bad_panel.Set("entries", bad_entries);
    bad_panels.Set(1, bad_panel);
    bad_progress.Set("panels", bad_panels);
    t.Check(!TaskTrackDashboardFromValue(bad_progress, invalid_doc, error),
            "out-of-range progress was accepted");

    ValueMap create;
    create.Add("store_root", root);
    create.Add("dashboard", dashboard);
    TaskTrackDashboardDocument saved;
    t.Check(TaskTrackDashboardUpsertFromArguments(create, saved, path, error, code),
            "revision 1 create failed: " + code + " " + error);
    t.Check(saved.revision == 1, "created dashboard revision should be 1");
    t.Check(FileExists(path), "current dashboard file missing after create");
    t.Check(FileExists(TaskTrackDashboardRevisionPath(root, id, 1)), "revision 1 snapshot missing");

    {
        String lock_path = AppendFileName(root, id + ".tasktrack-dashboard.lock");
        FileStream held;
        t.Check(held.Open(~lock_path, FileStream::READWRITE | FileStream::NOWRITESHARE),
                "could not acquire dashboard test write lock");
        ValueMap busy;
        busy.Add("store_root", root);
        busy.Add("base_revision", 1);
        busy.Add("dashboard", BuildDashboard(id, 55));
        TaskTrackDashboardDocument ignored;
        t.Check(!TaskTrackDashboardUpsertFromArguments(busy, ignored, path, error, code) && code == "WRITE_BUSY",
                "concurrent dashboard writer was not rejected while lock was held");
