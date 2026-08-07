}

bool TaskTrackRegisterTask(const String& task_id, const String& path, String& error)
{
    if(!IsSafeTaskId(task_id)) {
        error = "Refusing to register unsafe task id.";
        return false;
    }
    String registry_root = TaskTrackDefaultRegistryRoot();
    if(!RealizeDirectory(registry_root)) {
        error = "Unable to create TaskTrack registry folder: " + registry_root;
        return false;
    }
    String locator = AppendFileName(registry_root, task_id + ".path");
    return AtomicSaveText(locator, NormalizePath(path) + "\n", error);
}

bool TaskTrackResolveTaskPath(const String& task_id, const String& store_root,
                              String& path, String& error)
{
    error.Clear();
    if(!IsSafeTaskId(task_id)) {
        error = "Invalid TaskTrack task_id.";
        return false;
    }

    if(!TrimBoth(store_root).IsEmpty()) {
        String candidate = TaskTrackMakeTaskPath(store_root, task_id);
        if(FileExists(candidate) || FileExists(candidate + ".bak")) {
            path = candidate;
            return true;
        }
    }

    String locator = AppendFileName(TaskTrackDefaultRegistryRoot(), task_id + ".path");
    String registered = TrimBoth(LoadFile(locator));
    if(!IsNull(registered) && !registered.IsEmpty()) {
        if(FileExists(registered) || FileExists(registered + ".bak")) {
            path = NormalizePath(registered);
            return true;
        }
        FileDelete(locator);
        FileDelete(locator + ".bak");
    }

    String candidate = TaskTrackMakeTaskPath(TaskTrackDefaultStoreRoot(), task_id);
    if(FileExists(candidate) || FileExists(candidate + ".bak")) {
        path = candidate;
        return true;
    }

    error = "TaskTrack task not found: " + task_id;
    return false;
}

bool TaskTrackCreateFromArguments(const Value& args, TaskTrackDocument& doc,
                                  String& path, String& error)
{
    error.Clear();
    if(!args.Is<ValueMap>()) {
        error = "TaskTrack create_task arguments must be an object.";
        return false;
    }

    const char *string_fields[] = { "task_id", "project", "title", "subtitle", "actor", "store_root" };
    for(const char *key : string_fields)
        if(!ExpectOptionalString(args, key, error))
            return false;
    const char *bool_fields[] = { "remind_while_paused", "nudge_on_agent_poll" };
    for(const char *key : bool_fields)
        if(!ExpectOptionalBool(args, key, error))
            return false;
    const char *number_fields[] = { "reminder_minutes", "history_limit" };
    for(const char *key : number_fields)
        if(!ExpectOptionalNumber(args, key, error))
            return false;

    TaskTrackDocument created;
    created.schema_version = 2;
    created.task_id = TrimBoth(ReadString(args, "task_id"));
    if(created.task_id.IsEmpty())
        created.task_id = TaskTrackMakeTaskId();
    if(!IsSafeTaskId(created.task_id)) {
        error = "task_id must begin with task- and contain only letters, digits, '-' or '_'.";
        return false;
    }

    created.project = TrimBoth(ReadString(args, "project"));
    created.title = TrimBoth(ReadString(args, "title"));
    created.subtitle = ReadString(args, "subtitle");
    created.actor = ReadString(args, "actor", "agent");
    created.reminder_minutes = min(max(ReadInt(args, "reminder_minutes", 60), 0), 24 * 60);
    created.remind_while_paused = ReadBool(args, "remind_while_paused", false);
    created.nudge_on_agent_poll = ReadBool(args, "nudge_on_agent_poll", false);
    created.history_limit = min(max(ReadInt(args, "history_limit", 20), 5), 200);
    created.state = TaskTrackState::AwaitingHuman;
    created.created_at = TaskTrackNowIso();
    created.updated_at = created.created_at;
    created.last_human_activity_at = created.created_at;
    created.last_human_activity_epoch = GetSysTime().Get();

    if(created.title.IsEmpty()) {
        error = "create_task requires a non-empty title.";
        return false;
    }

    Value raw_items = args["items"];
    if(!raw_items.Is<ValueArray>()) {
        error = "create_task requires an items array.";
        return false;
