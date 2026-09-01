bool TaskTrackDashboardUpsertFromArguments(const Value& args,
                                           TaskTrackDashboardDocument& doc,
                                           String& path,
                                           String& error,
                                           String& error_code)
{
    error.Clear();
    error_code = "BAD_REQUEST";
    if(!args.Is<ValueMap>()) { error = "arguments must be an object."; return false; }
    if(!ExpectOptionalString(args, "store_root", error) || !ExpectOptionalNumber(args, "base_revision", error))
        return false;
    Value raw_dashboard = args["dashboard"];
    if(!raw_dashboard.Is<ValueMap>()) { error = "upsert_dashboard requires a dashboard object."; return false; }

    ValueMap normalized = raw_dashboard;
    String dashboard_id = TrimBoth(ReadString(normalized, "dashboard_id"));
    if(dashboard_id.IsEmpty()) {
        dashboard_id = TaskTrackDashboardMakeId();
        normalized.Set("dashboard_id", dashboard_id);
    }
    if(IsNull(normalized["schema_version"]))
        normalized.Set("schema_version", TASKTRACK_DASHBOARD_SCHEMA_VERSION);

    TaskTrackDashboardDocument incoming;
    if(!TaskTrackDashboardFromValue(Value(normalized), incoming, error))
        return false;

    String store_root = ReadString(args, "store_root");
    DashboardWriteLock write_lock;
    if(!write_lock.Acquire(store_root, incoming.dashboard_id, error)) {
        error_code = "WRITE_BUSY";
        return false;
    }

    path = TaskTrackDashboardMakePath(store_root, incoming.dashboard_id);
    bool exists = FileExists(path) || FileExists(path + ".bak");
    TaskTrackDashboardDocument current;
    if(exists) {
        if(!TaskTrackDashboardLoad(path, current, error)) {
            error_code = "DASHBOARD_LOAD_FAILED";
            return false;
        }
        Value base = args["base_revision"];
        if(IsNull(base)) {
            error_code = "REVISION_REQUIRED";
            error = Format("Dashboard %s is at revision %d; updates require base_revision.", incoming.dashboard_id, current.revision);
            return false;
        }
        int requested = (int)base;
        if(requested != current.revision) {
            error_code = "REVISION_CONFLICT";
            error = Format("Dashboard %s revision conflict: base_revision=%d, current=%d.", incoming.dashboard_id, requested, current.revision);
            return false;
        }
    }
    else {
        Value base = args["base_revision"];
        if(!IsNull(base) && (int)base != 0) {
            error_code = "REVISION_CONFLICT";
            error = "New dashboards require base_revision 0 when base_revision is supplied.";
            return false;
        }
    }

    String now = TaskTrackNowIso();
    incoming.revision = exists ? current.revision + 1 : 1;
    incoming.created_at = exists ? current.created_at : now;
    incoming.updated_at = now;
    if(incoming.updated_by.IsEmpty()) incoming.updated_by = "agent";
    if(!TaskTrackDashboardValidate(incoming, error)) return false;

    String revision_folder = TaskTrackDashboardRevisionFolder(store_root, incoming.dashboard_id);
    if(!RealizeDirectory(revision_folder)) {
        error_code = "SAVE_FAILED";
        error = "Unable to create dashboard revision folder: " + revision_folder;
        return false;
    }
    String revision_path = TaskTrackDashboardRevisionPath(store_root, incoming.dashboard_id, incoming.revision);
    if(FileExists(revision_path)) {
        if(exists && incoming.revision <= current.revision) {
            error_code = "REVISION_EXISTS";
            error = "Refusing to overwrite accepted immutable dashboard revision: " + revision_path;
            return false;
        }
        FileDelete(revision_path);
        FileDelete(revision_path + ".bak");
        FileDelete(revision_path + ".tmp");
        if(FileExists(revision_path)) {
            error_code = "REVISION_RECOVERY_FAILED";
            error = "Unable to remove incomplete dashboard revision snapshot: " + revision_path;
            return false;
        }
    }

    String json = TaskTrackDashboardToJson(incoming, true);
    if(!AtomicSaveText(revision_path, json, error)) {
        error_code = "SAVE_FAILED";
        return false;
    }
    if(!TaskTrackDashboardSaveCurrent(path, incoming, error)) {
        FileDelete(revision_path);
        FileDelete(revision_path + ".bak");
        error_code = "SAVE_FAILED";
        return false;
    }

    doc = pick(incoming);
    error_code.Clear();
    return true;
}
