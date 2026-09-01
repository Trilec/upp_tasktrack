int TaskTrackDashboardBridgeSelfTest(Vector<String>& failures)
{
    int passed = 0;
    auto Check = [&](bool condition, const String& message) {
        if(condition)
            ++passed;
        else
            failures.Add(message);
    };

    String root = AppendFileName(GetFileFolder(GetExeFilePath()),
                                 "_tasktrack_dashboard_mcp_selftest");
    String id = "dashboard-mcp-selftest";
    CleanupDashboardSelfTest(root, id);
    RealizeDirectory(root);

    TaskTrackDashboardToolResult validate =
        TaskTrackExecuteDashboardTool("validate_dashboard", SampleDashboard(id, 100, 0));
    String validate_json = AsJSON(validate.structured, false);
    Check(!validate.is_error && validate_json.Find("\"valid\":true") >= 0,
          "dashboard validate tool failed");

    ValueMap create;
    create.Add("store_root", root);
    create.Add("base_revision", 0);
    create.Add("dashboard", SampleDashboard(id, 100, 0));
    TaskTrackDashboardToolResult created =
        TaskTrackExecuteDashboardTool("upsert_dashboard", create);
    Check(!created.is_error && AsJSON(created.structured, false).Find("\"revision\":1") >= 0,
          "dashboard create revision 1 failed");

    ValueMap get;
    get.Add("dashboard_id", id);
    get.Add("store_root", root);
    get.Add("include_panels", true);
    TaskTrackDashboardToolResult current =
        TaskTrackExecuteDashboardTool("get_dashboard", get);
    String current_json = AsJSON(current.structured, false);
    Check(!current.is_error && current_json.Find("dashboard_version") >= 0 &&
          current_json.Find(TaskTrackBuildVersion()) >= 0,
          "dashboard get did not return unified build identity");

    ValueMap stale;
    stale.Add("store_root", root);
    stale.Add("base_revision", 0);
    stale.Add("dashboard", SampleDashboard(id, 100, 50));
    TaskTrackDashboardToolResult conflict =
        TaskTrackExecuteDashboardTool("upsert_dashboard", stale);
    Check(conflict.is_error && AsJSON(conflict.structured, false).Find("REVISION_CONFLICT") >= 0,
          "dashboard stale revision was not rejected");

    ValueMap update;
    update.Add("store_root", root);
    update.Add("base_revision", 1);
    update.Add("dashboard", SampleDashboard(id, 100, 50));
    TaskTrackDashboardToolResult updated =
        TaskTrackExecuteDashboardTool("upsert_dashboard", update);
    Check(!updated.is_error && AsJSON(updated.structured, false).Find("\"revision\":2") >= 0,
          "dashboard update revision 2 failed");

    TaskTrackDashboardDocument doc;
    String error;
    Check(TaskTrackDashboardLoad(TaskTrackDashboardMakePath(root, id), doc, error) && doc.revision == 2,
          "dashboard current revision is not 2");
    Check(fabs(TaskTrackDashboardDerivedProgress(doc) - 75.0) < 0.001,
          "dashboard derived progress mismatch");

    ValueMap list_revisions;
    list_revisions.Add("dashboard_id", id);
    list_revisions.Add("store_root", root);
    list_revisions.Add("limit", 10);
    TaskTrackDashboardToolResult revisions =
        TaskTrackExecuteDashboardTool("list_dashboard_revisions", list_revisions);
    Check(!revisions.is_error && AsJSON(revisions.structured, false).Find("\"revision\":2") >= 0,
          "dashboard revision listing failed");

    ValueMap get_revision;
    get_revision.Add("dashboard_id", id);
    get_revision.Add("store_root", root);
    get_revision.Add("revision", 1);
    TaskTrackDashboardToolResult revision =
        TaskTrackExecuteDashboardTool("get_dashboard_revision", get_revision);
    Check(!revision.is_error && AsJSON(revision.structured, false).Find("Foundation") >= 0,
          "dashboard revision retrieval failed");

    ValueMap list;
    list.Add("store_root", root);
    list.Add("limit", 10);
    TaskTrackDashboardToolResult dashboards =
        TaskTrackExecuteDashboardTool("list_dashboards", list);
    Check(!dashboards.is_error && AsJSON(dashboards.structured, false).Find(id) >= 0,
          "dashboard listing failed");

    CleanupDashboardSelfTest(root, id);
    return passed;
}
