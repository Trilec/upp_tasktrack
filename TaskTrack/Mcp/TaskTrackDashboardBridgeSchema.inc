Value DSchema(const String& type, const String& description = String())
{
    ValueMap s;
    if(!type.IsEmpty())
        s.Add("type", type);
    if(!description.IsEmpty())
        s.Add("description", description);
    return Value(s);
}

Value DStringSchema(const String& description = String())
{
    return DSchema("string", description);
}

Value DBoolSchema(const String& description = String())
{
    return DSchema("boolean", description);
}

Value DNumberSchema(const String& description = String())
{
    return DSchema("number", description);
}

Value DIntSchema(int minimum, int maximum, const String& description = String())
{
    ValueMap s;
    s.Add("type", "integer");
    s.Add("minimum", minimum);
    s.Add("maximum", maximum);
    if(!description.IsEmpty())
        s.Add("description", description);
    return Value(s);
}

Value DEnumStringSchema(const char **values, int count, const String& description)
{
    ValueArray allowed;
    for(int i = 0; i < count; ++i)
        allowed.Add(values[i]);
    ValueMap s;
    s.Add("type", "string");
    s.Add("enum", allowed);
    if(!description.IsEmpty())
        s.Add("description", description);
    return Value(s);
}

Value DToolAnnotations(bool read_only, bool destructive, bool idempotent)
{
    ValueMap a;
    a.Add("readOnlyHint", read_only);
    a.Add("destructiveHint", destructive);
    a.Add("idempotentHint", idempotent);
    return Value(a);
}

Value DToolSpec(const String& name, const String& description, const Value& input,
                bool read_only, bool destructive, bool idempotent)
{
    ValueMap tool;
    tool.Add("name", name);
    tool.Add("description", description);
    tool.Add("inputSchema", input);
    tool.Add("annotations", DToolAnnotations(read_only, destructive, idempotent));
    return Value(tool);
}

Value DashboardEntrySchema()
{
    ValueMap p;
    p.Add("id", DStringSchema("Globally stable entry id."));
    p.Add("title", DStringSchema("Short human-readable entry title."));
    p.Add("subtitle", DStringSchema("Optional supporting line."));
    p.Add("status", DStringSchema("Semantic state such as complete, active, next, pending or blocked; not display styling."));
    p.Add("detail", DStringSchema());
    p.Add("category", DStringSchema("Navigation/grouping label, not layout instructions."));
    p.Add("progress", DNumberSchema("Optional measurable 0..100 completion only."));
    p.Add("weight", DNumberSchema("Positive progress weight."));
    p.Add("attention", DBoolSchema("Unresolved awareness, decision or intervention signal."));
    p.Add("archived", DBoolSchema("Entry is no longer current; excluded from derived progress."));
    p.Add("timestamp", DStringSchema());
    p.Add("task_id", DStringSchema("Optional link to a separate human TaskTrack decision; never human evidence itself."));
    p.Add("evidence", DStringSchema("Evidence supporting this current entry."));
    p.Add("data", ValueMap());

    ValueArray required;
    required.Add("id");
    required.Add("title");

    ValueMap s;
    s.Add("type", "object");
    s.Add("required", required);
    s.Add("properties", p);
    s.Add("additionalProperties", false);
    return Value(s);
}

Value DashboardPanelSchema()
{
    const char *types[] = {
        "project_state", "timeline", "progress_list", "action_list",
        "attention", "verification", "changes", "records"
    };
    const char *densities[] = { "summary", "standard", "full" };

    ValueMap p;
    p.Add("id", DStringSchema());
    p.Add("type", DEnumStringSchema(types, 8, "Semantic dashboard panel renderer."));
    p.Add("title", DStringSchema());
    p.Add("subtitle", DStringSchema());
    p.Add("category", DStringSchema("Navigation/grouping label, not layout."));
    p.Add("density", DEnumStringSchema(densities, 3, "Presentation hint, not geometry."));
    p.Add("preview_limit", DIntSchema(1, 100));
    p.Add("contributes_to_progress", DBoolSchema("Whether measurable entries contribute to derived overall progress."));

    ValueMap entries;
    entries.Add("type", "array");
    entries.Add("maxItems", TASKTRACK_DASHBOARD_MAX_ENTRIES_PER_PANEL);
    entries.Add("items", DashboardEntrySchema());
    p.Add("entries", entries);

    ValueArray required;
    required.Add("id");
    required.Add("type");
    required.Add("title");
    required.Add("entries");

    ValueMap s;
    s.Add("type", "object");
    s.Add("required", required);
    s.Add("properties", p);
    s.Add("additionalProperties", false);
    return Value(s);
}

Value DashboardDocumentSchema()
{
    ValueMap p;
    p.Add("schema_version", DIntSchema(1, 1));
    p.Add("dashboard_version", DStringSchema("Server-produced marker; safe to round-trip."));
    p.Add("dashboard_id", DStringSchema("Stable id beginning dashboard-."));
    p.Add("project", DStringSchema());
    p.Add("title", DStringSchema());
    p.Add("subtitle", DStringSchema());
    p.Add("phase", DStringSchema());
    p.Add("overall_progress", DNumberSchema("Optional explicit management estimate overriding derived display."));
    p.Add("confidence", DNumberSchema("Separate 0..100 confidence in the status, not progress."));
    p.Add("revision", DIntSchema(0, INT_MAX,
                                 "Server-produced revision; upsert allocates next revision."));
    p.Add("created_at", DStringSchema("Server-produced; safe to round-trip."));
    p.Add("updated_at", DStringSchema("Server-produced; replaced on accepted write."));
    p.Add("updated_by", DStringSchema());
    p.Add("source_head", DStringSchema("Repository or source revision supporting this dashboard state."));

    ValueMap panels;
    panels.Add("type", "array");
    panels.Add("minItems", 1);
    panels.Add("maxItems", TASKTRACK_DASHBOARD_MAX_PANELS);
    panels.Add("items", DashboardPanelSchema());
    p.Add("panels", panels);

    ValueArray required;
    required.Add("title");
    required.Add("panels");

    ValueMap s;
    s.Add("type", "object");
    s.Add("required", required);
    s.Add("properties", p);
    s.Add("additionalProperties", false);
    return Value(s);
}

Value DashboardUpsertSchema()
{
    ValueMap p;
    p.Add("store_root", DStringSchema());
    p.Add("base_revision", DIntSchema(0, INT_MAX,
                                     "Required for an existing dashboard; must equal current revision."));
    p.Add("dashboard", DashboardDocumentSchema());

    ValueArray required;
    required.Add("dashboard");

    ValueMap s;
    s.Add("type", "object");
    s.Add("required", required);
    s.Add("properties", p);
    s.Add("additionalProperties", false);
    return Value(s);
}

Value DashboardLocatorSchema(bool include_panels = false)
{
    ValueMap p;
    p.Add("dashboard_id", DStringSchema());
    p.Add("store_root", DStringSchema());
    if(include_panels)
        p.Add("include_panels", DBoolSchema());

    ValueArray required;
    required.Add("dashboard_id");

    ValueMap s;
    s.Add("type", "object");
    s.Add("required", required);
    s.Add("properties", p);
    s.Add("additionalProperties", false);
    return Value(s);
}

Value DashboardRevisionSchema(bool with_limit, bool with_revision)
{
    ValueMap p;
    p.Add("dashboard_id", DStringSchema());
    p.Add("store_root", DStringSchema());
    if(with_limit)
        p.Add("limit", DIntSchema(1, 500));
    if(with_revision)
        p.Add("revision", DIntSchema(1, INT_MAX));

    ValueArray required;
    required.Add("dashboard_id");
    if(with_revision)
        required.Add("revision");

    ValueMap s;
    s.Add("type", "object");
    s.Add("required", required);
    s.Add("properties", p);
    s.Add("additionalProperties", false);
    return Value(s);
}

Value DashboardListSchema()
{
    ValueMap p;
    p.Add("store_root", DStringSchema());
    p.Add("limit", DIntSchema(1, 200));
    ValueMap s;
    s.Add("type", "object");
    s.Add("properties", p);
    s.Add("additionalProperties", false);
    return Value(s);
}

TaskTrackDashboardToolResult DResult(const Value& structured, bool is_error = false)
{
    TaskTrackDashboardToolResult out;
    out.structured = structured;
    out.is_error = is_error;
    return out;
}

Value DFailure(const String& code, const String& message)
{
    ValueMap out;
    out.Add("ok", false);
    out.Add("code", code);
    out.Add("message", message);
    return Value(out);
}

bool DReadBool(const Value& map, const char *key, bool def, bool& out, String& error)
{
    Value v = map[key];
    if(IsNull(v)) {
        out = def;
        return true;
    }
    if(!v.Is<bool>()) {
        error = String(key) + " must be a boolean.";
        return false;
    }
    out = (bool)v;
    return true;
}

bool DReadInt(const Value& map, const char *key, int def, int& out, String& error)
{
    Value v = map[key];
    if(IsNull(v)) {
        out = def;
        return true;
    }
    if(!IsNumber(v)) {
        error = String(key) + " must be numeric.";
        return false;
    }
    out = (int)v;
    return true;
}

bool LaunchDashboardGui(const String& dashboard_path, String& error)
{
#ifdef PLATFORM_WIN32
    const char *exe_name = "TaskTrackDashboardGui.exe";
#else
    const char *exe_name = "TaskTrackDashboardGui";
#endif
    String gui = AppendFileName(GetFileFolder(GetExeFilePath()), exe_name);
    if(!FileExists(gui)) {
        error = "TaskTrack dashboard GUI executable not found beside MCP server: " + gui;
        return false;
    }

    Vector<String> args;
    args.Add("--dashboard");
    args.Add(dashboard_path);
    LocalProcess process;
#ifndef PLATFORM_WIN32
    process.DoubleFork();
#endif
    if(!process.Start(~gui, args)) {
        error = "Unable to start TaskTrack dashboard GUI.";
        return false;
    }
    process.Detach();
    return true;
}

bool ResolveAndLoadDashboard(const Value& args, TaskTrackDashboardDocument& doc,
                             String& path, String& error)
{
    if(!args.Is<ValueMap>()) {
        error = "arguments must be an object.";
        return false;
    }
    Value id = args["dashboard_id"];
    Value store = args["store_root"];
    if(IsNull(id) || !id.Is<String>()) {
        error = "dashboard_id is required and must be a string.";
        return false;
    }
    if(!IsNull(store) && !store.Is<String>()) {
        error = "store_root must be a string.";
        return false;
    }
    if(!TaskTrackDashboardResolvePath(AsString(id),
                                      IsNull(store) ? String() : AsString(store),
                                      path, error))
        return false;
    return TaskTrackDashboardLoad(path, doc, error);
}

Value SampleDashboard(const String& id, int first_progress, int second_progress)
{
    ValueMap first;
    first.Add("id", "milestone-a");
    first.Add("title", "Foundation");
    first.Add("status", "complete");
    first.Add("progress", first_progress);

    ValueMap second;
    second.Add("id", "milestone-b");
    second.Add("title", "Integration");
    second.Add("status", "blocked");
    second.Add("progress", second_progress);
    second.Add("attention", true);

    ValueArray entries;
    entries.Add(first);
    entries.Add(second);

    ValueMap panel;
    panel.Add("id", "milestones");
    panel.Add("type", "timeline");
    panel.Add("title", "Milestones");
    panel.Add("contributes_to_progress", true);
    panel.Add("entries", entries);

    ValueArray panels;
    panels.Add(panel);

    ValueMap dashboard;
    dashboard.Add("dashboard_id", id);
    dashboard.Add("project", "DashboardSelfTest");
    dashboard.Add("title", "Unified TaskTrack MCP dashboard acceptance");
    dashboard.Add("phase", "Validation");
    dashboard.Add("updated_by", "selftest");
    dashboard.Add("panels", panels);
    return Value(dashboard);
}

void CleanupDashboardSelfTest(const String& root, const String& id)
{
    String current = TaskTrackDashboardMakePath(root, id);
    FileDelete(current);
    FileDelete(current + ".bak");
    FileDelete(current + ".tmp");
    FileDelete(AppendFileName(root, id + ".tasktrack-dashboard.lock"));

    String revisions = TaskTrackDashboardRevisionFolder(root, id);
    FindFile ff(AppendFileName(revisions, "*"));
    while(ff) {
        if(ff.IsFile())
            FileDelete(AppendFileName(revisions, ff.GetName()));
        ff.Next();
    }
    DirectoryDelete(revisions);
    DirectoryDelete(AppendFileName(root, "revisions"));
    DirectoryDelete(root);
}
