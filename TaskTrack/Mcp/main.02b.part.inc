    return Value(s);
}

Value BuildToolsList(bool modern)
{
    ValueArray tools;
    tools.Add(ToolSpec("version", "Return the TaskTrack application/protocol version.", EmptyObjectSchema(), true, false, true));
    tools.Add(ToolSpec("create_task",
        "Ask a human for structured input that automation cannot reliably infer or verify. Use the 18 semantic question types; do not encode U++ widget names. The task is durably saved before its id is returned, so the human may answer minutes, hours, or days later.",
        CreateTaskSchema(), false, false, false));
    tools.Add(ToolSpec("get_task", "Return durable TaskTrack state and structured human answers. Polling may emit a reminder signal but never changes evidence or closes the task.", TaskLocatorSchema(true), true, false, true));
    tools.Add(ToolSpec("open_task", "Launch the TaskTrack GUI for an existing task_id.", TaskLocatorSchema(false), false, false, false));

    ValueMap list_props;
    list_props.Add("store_root", StringSchema());
    list_props.Add("limit", IntSchema(1, 200));
    ValueMap list_schema; list_schema.Add("type", "object"); list_schema.Add("properties", list_props); list_schema.Add("additionalProperties", false);
    tools.Add(ToolSpec("list_tasks", "List recent tasks in a TaskTrack store.", list_schema, true, false, true));
    tools.Add(ToolSpec("close_task", "Explicitly close unfinished work. TaskTrack never closes a task merely because it is old or paused.", TaskLocatorSchema(false), false, true, false));

    ValueMap result;
    if(modern) result.Add("resultType", "complete");
    result.Add("tools", tools);
    if(modern) { result.Add("ttlMs", 300000); result.Add("cacheScope", "public"); AddModernMeta(result); }
    return Value(result);
}

Value BuildDiscoverResult()
{
    ValueArray versions; versions.Add(CURRENT_PROTOCOL);
    ValueMap extensions; extensions.Add(TASKS_EXTENSION, ValueMap());
    ValueMap capabilities; capabilities.Add("tools", ValueMap()); capabilities.Add("extensions", extensions);
    ValueMap result;
    result.Add("resultType", "complete");
    result.Add("supportedVersions", versions);
    result.Add("capabilities", capabilities);
    result.Add("instructions", "Use create_task when a human decision, observation, visual judgement, or interactive check is required. Choose semantic response types; poll by task_id. TaskTrack never auto-closes human work.");
    result.Add("ttlMs", 300000);
    result.Add("cacheScope", "public");
    AddModernMeta(result);
    return Value(result);
}

Value BuildLegacyInitialize(const Value& params)
{
    String requested = ReadString(params, "protocolVersion", "2024-11-05");
    if(requested != "2025-11-25" && requested != "2025-06-18" && requested != "2024-11-05")
        requested = "2025-11-25";
    ValueMap tools; tools.Add("listChanged", false);
    ValueMap caps; caps.Add("tools", tools);
    ValueMap result;
    result.Add("protocolVersion", requested);
    result.Add("capabilities", caps);
    result.Add("serverInfo", ServerInfoValue());
    result.Add("instructions", "TaskTrack creates durable structured human-input tasks. Use get_task to poll without holding a request open.");
    return Value(result);
}

bool LaunchTaskGui(const String& task_path, String& error)
{
    String folder = GetFileFolder(GetExeFilePath());
#ifdef PLATFORM_WIN32
    String gui = AppendFileName(folder, "TaskTrack.exe");
#else
    String gui = AppendFileName(folder, "TaskTrack");
#endif
    if(!FileExists(gui)) { error = "TaskTrack GUI executable not found beside MCP server: " + gui; return false; }
    Vector<String> args; args.Add("--task"); args.Add(task_path);
    LocalProcess process;
#ifndef PLATFORM_WIN32
    process.DoubleFork();
#endif
    if(!process.Start(~gui, args)) { error = "Unable to start TaskTrack GUI."; return false; }
    process.Detach();
    return true;
}

bool ResolveAndLoad(const Value& args, TaskTrackDocument& doc, String& path, String& error)
{
    if(!args.Is<ValueMap>()) { error = "arguments must be an object."; return false; }
    Value id = args["task_id"];
    Value store = args["store_root"];
    if(IsNull(id) || !id.Is<String>()) { error = "task_id is required and must be a string."; return false; }
    if(!IsNull(store) && !store.Is<String>()) { error = "store_root must be a string."; return false; }
    if(!TaskTrackResolveTaskPath(AsString(id), IsNull(store) ? String() : AsString(store), path, error)) return false;
    return TaskTrackLoad(path, doc, error);
}

Value TaskHandleValue(const TaskTrackDocument& doc, const String& status_message)
{
    ValueMap result;
    result.Add("resultType", "task"); result.Add("taskId", doc.task_id); result.Add("status", "working");
    result.Add("statusMessage", status_message); result.Add("createdAt", doc.created_at); result.Add("lastUpdatedAt", doc.updated_at);
    result.Add("ttlMs", Value()); result.Add("pollIntervalMs", 30000); AddModernMeta(result);
    return Value(result);
}

Value DetailedTaskValue(const TaskTrackDocument& doc, const String& path)
{
    ValueMap result;
    result.Add("resultType", "complete"); result.Add("taskId", doc.task_id); result.Add("createdAt", doc.created_at);
    result.Add("lastUpdatedAt", doc.updated_at); result.Add("ttlMs", Value()); result.Add("pollIntervalMs", 30000);
    if(doc.state == TaskTrackState::Completed) {
        result.Add("status", "completed"); result.Add("statusMessage", "Human input completed.");
        result.Add("result", BuildCallToolResult(TaskTrackStatusValue(doc, path, true), false, true));
    }
    else if(doc.state == TaskTrackState::Closed) {
        result.Add("status", "cancelled"); result.Add("statusMessage", "Task was explicitly closed without completion.");
    }
    else {
        result.Add("status", "working"); result.Add("statusMessage", "TaskTrack state: " + TaskTrackStateName(doc.state));
    }
    AddModernMeta(result);
    return Value(result);
}

Value ExecuteTool(const String& name, const Value& args, bool modern, bool task_capability)
{
    if(name == "version") {
        ValueMap v; v.Add("ok", true); v.Add("version", TaskTrackVersion()); v.Add("schema_version", 2);
        v.Add("question_types", 18); v.Add("current_protocol", CURRENT_PROTOCOL); v.Add("tasks_extension", TASKS_EXTENSION);
        return BuildCallToolResult(v, false, modern);
    }
    if(name == "create_task") {
