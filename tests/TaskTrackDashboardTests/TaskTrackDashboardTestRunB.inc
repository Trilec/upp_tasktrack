        held.Close();
    }

    TaskTrackDashboardDocument orphan;
    t.Check(TaskTrackDashboardFromValue(TaskTrackDashboardToValue(saved), orphan, error),
            "could not clone revision for orphan fixture: " + error);
    orphan.revision = 2;
    orphan.updated_at = TaskTrackNowIso();
    String orphan_path = TaskTrackDashboardRevisionPath(root, id, 2);
    RealizeDirectory(GetFileFolder(orphan_path));
    t.Check(SaveFile(orphan_path, TaskTrackDashboardToJson(orphan, true)),
            "could not create orphan revision fixture");

    ValueMap no_base;
    no_base.Add("store_root", root);
    no_base.Add("dashboard", BuildDashboard(id, 70));
    t.Check(!TaskTrackDashboardUpsertFromArguments(no_base, saved, path, error, code) && code == "REVISION_REQUIRED",
            "existing dashboard update without base_revision was accepted");

    ValueMap stale;
    stale.Add("store_root", root);
    stale.Add("base_revision", 0);
    stale.Add("dashboard", BuildDashboard(id, 70));
    t.Check(!TaskTrackDashboardUpsertFromArguments(stale, saved, path, error, code) && code == "REVISION_CONFLICT",
            "stale dashboard update was accepted");

    ValueMap update;
    update.Add("store_root", root);
    update.Add("base_revision", 1);
    update.Add("dashboard", BuildDashboard(id, 90));
    t.Check(TaskTrackDashboardUpsertFromArguments(update, saved, path, error, code),
            "revision 2 update failed: " + code + " " + error);
    t.Check(saved.revision == 2, "updated dashboard revision should be 2");
    t.Check(fabs(TaskTrackDashboardDerivedProgress(saved) - 70.0) < 0.01,
            "revision 2 derived progress mismatch");

    TaskTrackDashboardDocument recovered_orphan;
    String recovered_orphan_path;
    t.Check(TaskTrackDashboardLoadRevision(id, root, 2, recovered_orphan, recovered_orphan_path, error) &&
            fabs(TaskTrackDashboardDerivedProgress(recovered_orphan) - 70.0) < 0.01,
            "orphan revision 2 was not replaced by accepted revision 2 state");

    ValueArray revisions;
    t.Check(TaskTrackDashboardListRevisions(id, root, 20, revisions, error) && revisions.GetCount() == 2,
            "revision history did not return two accepted snapshots");

    ValueArray dashboards;
    t.Check(TaskTrackDashboardList(root, 20, dashboards, error) && dashboards.GetCount() >= 1,
            "dashboard listing did not find current dashboard");

    TaskTrackDashboardDocument current;
    t.Check(TaskTrackDashboardLoad(path, current, error) && current.revision == 2,
            "current dashboard load failed");

    String primary = LoadFile(path);
    t.Check(!IsNull(primary), "could not read current dashboard for recovery fixture");
    SaveFile(path + ".bak", primary);
    SaveFile(path, "{broken json");
    TaskTrackDashboardDocument recovered;
    t.Check(TaskTrackDashboardLoad(path, recovered, error) && recovered.revision == 2,
            "current .bak recovery failed: " + error);

    Cleanup(root, id);
    DeleteFolderDeep(root);

    Cout() << "TaskTrackDashboardTests: " << t.passed << " passed, " << t.failed.GetCount() << " failed\n";
    for(const String& failure : t.failed)
        Cout() << "FAIL: " << failure << "\n";
    SetExitCode(t.failed.IsEmpty() ? 0 : 1);
