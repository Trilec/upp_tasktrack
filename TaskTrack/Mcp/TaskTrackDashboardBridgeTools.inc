void TaskTrackAppendDashboardToolSpecs(ValueArray& tools)
{
    tools.Add(DToolSpec("validate_dashboard",
                        "Use this to validate a candidate semantic dashboard before installing it; it does not persist state.",
                        DashboardDocumentSchema(), true, false, true));
    tools.Add(DToolSpec("upsert_dashboard",
                        "Use this to create a dashboard or install reconciled current state after reading it. Existing dashboards require the exact base_revision; on REVISION_CONFLICT re-read and merge, and retry WRITE_BUSY without bypassing the writer lock.",
                        DashboardUpsertSchema(), false, false, false));
    tools.Add(DToolSpec("get_dashboard",
                        "Use this to read current state before modifying an existing dashboard or whenever current visual/project state is required; it includes progress and attention summaries.",
                        DashboardLocatorSchema(true), true, false, true));
    tools.Add(DToolSpec("open_dashboard",
                        "Use this when a human needs to inspect the native read-only visual dashboard.",
                        DashboardLocatorSchema(false), false, false, false));
    tools.Add(DToolSpec("list_dashboards",
                        "Use this to discover current project dashboards before selecting one to read or open.",
                        DashboardListSchema(), true, false, true));
    tools.Add(DToolSpec("list_dashboard_revisions",
                        "Use this to inspect accepted dashboard history rather than current working state.",
                        DashboardRevisionSchema(true, false), true, false, true));
    tools.Add(DToolSpec("get_dashboard_revision",
                        "Use this to read one previous accepted dashboard revision, not the current working source.",
                        DashboardRevisionSchema(false, true), true, false, true));
}

bool TaskTrackIsDashboardTool(const String& name)
{
    return name == "validate_dashboard" || name == "upsert_dashboard" ||
           name == "get_dashboard" || name == "open_dashboard" ||
           name == "list_dashboards" || name == "list_dashboard_revisions" ||
           name == "get_dashboard_revision";
}

TaskTrackDashboardToolResult TaskTrackExecuteDashboardTool(const String& name,
                                                         const Value& args)
{
    if(name == "validate_dashboard") {
        if(!args.Is<ValueMap>())
            return DResult(DFailure("BAD_REQUEST", "dashboard must be an object."), true);
        ValueMap normalized = args;
        if(IsNull(normalized["dashboard_id"]))
            normalized.Set("dashboard_id", TaskTrackDashboardMakeId());
        if(IsNull(normalized["schema_version"]))
            normalized.Set("schema_version", TASKTRACK_DASHBOARD_SCHEMA_VERSION);

        TaskTrackDashboardDocument doc;
        String error;
        if(!TaskTrackDashboardFromValue(normalized, doc, error))
            return DResult(DFailure("INVALID_DASHBOARD", error), true);

        ValueMap out;
        out.Add("ok", true);
        out.Add("valid", true);
        out.Add("effective_progress", TaskTrackDashboardEffectiveProgress(doc));
        out.Add("derived_progress", TaskTrackDashboardDerivedProgress(doc));
        out.Add("attention_count", TaskTrackDashboardAttentionCount(doc));
        out.Add("panel_count", doc.panels.GetCount());
        return DResult(out);
    }

    if(name == "upsert_dashboard") {
        TaskTrackDashboardDocument doc;
        String path, error, code;
        if(!TaskTrackDashboardUpsertFromArguments(args, doc, path, error, code))
            return DResult(DFailure(code.IsEmpty() ? String("UPSERT_FAILED") : code, error), true);
        ValueMap out = TaskTrackDashboardStatusValue(doc, path, true);
        out.Add("message", doc.revision == 1
                ? "Dashboard created."
                : "Dashboard updated; immutable revision appended.");
        out.Add("next_base_revision", doc.revision);
        return DResult(out);
    }

    if(name == "get_dashboard") {
        TaskTrackDashboardDocument doc;
        String path, error;
        if(!ResolveAndLoadDashboard(args, doc, path, error))
            return DResult(DFailure("DASHBOARD_NOT_FOUND", error), true);
        bool include_panels = true;
        if(!DReadBool(args, "include_panels", true, include_panels, error))
            return DResult(DFailure("BAD_REQUEST", error), true);
        return DResult(TaskTrackDashboardStatusValue(doc, path, include_panels));
    }

    if(name == "open_dashboard") {
        TaskTrackDashboardDocument doc;
        String path, error;
        if(!ResolveAndLoadDashboard(args, doc, path, error))
            return DResult(DFailure("DASHBOARD_NOT_FOUND", error), true);
        bool launched = LaunchDashboardGui(path, error);
        ValueMap out = TaskTrackDashboardStatusValue(doc, path, false);
        out.Add("launched", launched);
        if(!launched)
            out.Add("launch_error", error);
        return DResult(out, !launched);
    }

    if(name == "list_dashboards") {
        if(!args.Is<ValueMap>())
            return DResult(DFailure("BAD_REQUEST", "arguments must be an object."), true);
        Value store = args["store_root"];
        if(!IsNull(store) && !store.Is<String>())
            return DResult(DFailure("BAD_REQUEST", "store_root must be a string."), true);
        int limit = 20;
        String error;
        if(!DReadInt(args, "limit", 20, limit, error) || limit < 1 || limit > 200)
            return DResult(DFailure("BAD_REQUEST",
                                    error.IsEmpty() ? String("limit must be 1..200.") : error), true);
        ValueArray dashboards;
        if(!TaskTrackDashboardList(IsNull(store) ? String() : AsString(store),
                                   limit, dashboards, error))
            return DResult(DFailure("LIST_FAILED", error), true);
        ValueMap out;
        out.Add("ok", true);
        out.Add("dashboards", dashboards);
        return DResult(out);
    }

    if(name == "list_dashboard_revisions") {
        if(!args.Is<ValueMap>())
            return DResult(DFailure("BAD_REQUEST", "arguments must be an object."), true);
        Value id = args["dashboard_id"];
        Value store = args["store_root"];
        if(IsNull(id) || !id.Is<String>() || (!IsNull(store) && !store.Is<String>()))
            return DResult(DFailure("BAD_REQUEST", "dashboard_id is required; store_root must be a string."), true);
        int limit = 50;
        String error;
        if(!DReadInt(args, "limit", 50, limit, error) || limit < 1 || limit > 500)
            return DResult(DFailure("BAD_REQUEST",
                                    error.IsEmpty() ? String("limit must be 1..500.") : error), true);
        ValueArray revisions;
        if(!TaskTrackDashboardListRevisions(AsString(id),
                                            IsNull(store) ? String() : AsString(store),
                                            limit, revisions, error))
            return DResult(DFailure("LIST_FAILED", error), true);
        ValueMap out;
        out.Add("ok", true);
        out.Add("dashboard_id", AsString(id));
        out.Add("revisions", revisions);
        return DResult(out);
    }

    if(name == "get_dashboard_revision") {
        if(!args.Is<ValueMap>())
            return DResult(DFailure("BAD_REQUEST", "arguments must be an object."), true);
        Value id = args["dashboard_id"];
        Value store = args["store_root"];
        int revision = 0;
        String error;
        if(IsNull(id) || !id.Is<String>() || (!IsNull(store) && !store.Is<String>()) ||
           !DReadInt(args, "revision", 0, revision, error) || revision <= 0)
            return DResult(DFailure("BAD_REQUEST",
                                    "dashboard_id and a positive revision are required."), true);
        TaskTrackDashboardDocument doc;
        String path;
        if(!TaskTrackDashboardLoadRevision(AsString(id),
                                           IsNull(store) ? String() : AsString(store),
                                           revision, doc, path, error))
            return DResult(DFailure("REVISION_NOT_FOUND", error), true);
        return DResult(TaskTrackDashboardStatusValue(doc, path, true));
    }

    return DResult(DFailure("UNKNOWN_TOOL", "Unknown TaskTrack dashboard tool: " + name), true);
}
