        bool launch = true; String error;
        if(!ReadBool(args, "launch", true, launch, error)) return BuildCallToolResult(BuildFailure("BAD_REQUEST", error), true, modern);
        TaskTrackDocument doc; String path;
        if(!TaskTrackCreateFromArguments(args, doc, path, error)) return BuildCallToolResult(BuildFailure("CREATE_FAILED", error), true, modern);
        bool launched = false; String launch_error;
        if(launch) launched = LaunchTaskGui(path, launch_error);
        if(modern && task_capability)
            return TaskHandleValue(doc, launched ? "Awaiting human input; TaskTrack GUI launched."
                                                 : "Awaiting human input; retrieve/open the durable task by taskId.");
        ValueMap status = TaskTrackStatusValue(doc, path, true);
        status.Add("launched", launched); if(launch && !launched) status.Add("launch_error", launch_error);
        return BuildCallToolResult(status, false, modern);
    }
    if(name == "get_task") {
        TaskTrackDocument doc; String path, error;
        if(!ResolveAndLoad(args, doc, path, error)) return BuildCallToolResult(BuildFailure("TASK_NOT_FOUND", error), true, modern);
        bool include_items = true;
        if(!ReadBool(args, "include_items", true, include_items, error)) return BuildCallToolResult(BuildFailure("BAD_REQUEST", error), true, modern);
        TaskTrackTouchAgentPoll(path);
        return BuildCallToolResult(TaskTrackStatusValue(doc, path, include_items), false, modern);
    }
    if(name == "open_task") {
        TaskTrackDocument doc; String path, error;
        if(!ResolveAndLoad(args, doc, path, error)) return BuildCallToolResult(BuildFailure("TASK_NOT_FOUND", error), true, modern);
        bool launched = LaunchTaskGui(path, error);
        ValueMap status = TaskTrackStatusValue(doc, path, false); status.Add("launched", launched); if(!launched) status.Add("launch_error", error);
        return BuildCallToolResult(status, !launched, modern);
    }
    if(name == "list_tasks") {
        if(!args.Is<ValueMap>()) return BuildCallToolResult(BuildFailure("BAD_REQUEST", "arguments must be an object."), true, modern);
        Value store = args["store_root"];
        if(!IsNull(store) && !store.Is<String>()) return BuildCallToolResult(BuildFailure("BAD_REQUEST", "store_root must be a string."), true, modern);
        int limit = 20; String error;
        if(!ReadInt(args, "limit", 20, limit, error)) return BuildCallToolResult(BuildFailure("BAD_REQUEST", error), true, modern);
        ValueArray tasks;
        if(!TaskTrackList(IsNull(store) ? String() : AsString(store), limit, tasks, error)) return BuildCallToolResult(BuildFailure("LIST_FAILED", error), true, modern);
        ValueMap result; result.Add("ok", true); result.Add("tasks", tasks);
        return BuildCallToolResult(result, false, modern);
    }
    if(name == "close_task") {
        TaskTrackDocument doc; String path, error;
        if(!ResolveAndLoad(args, doc, path, error)) return BuildCallToolResult(BuildFailure("TASK_NOT_FOUND", error), true, modern);
        if(doc.state == TaskTrackState::Completed) return BuildCallToolResult(BuildFailure("ALREADY_COMPLETED", "Completed tasks cannot be closed retroactively."), true, modern);
        if(!TaskTrackSetState(path, TaskTrackState::Closed, error) || !TaskTrackLoad(path, doc, error))
            return BuildCallToolResult(BuildFailure("CLOSE_FAILED", error), true, modern);
        return BuildCallToolResult(TaskTrackStatusValue(doc, path, true), false, modern);
    }
    return BuildCallToolResult(BuildFailure("UNKNOWN_TOOL", "Unknown TaskTrack tool: " + name), true, modern);
}

Value HandleTaskExtension(const Value& request, const String& method)
{
    Value id = request["id"];
    if(!IsModern(request)) return JsonRpcError(id, -32601, "Method not found");
    if(!ClientSupportsTasks(request))
        return JsonRpcError(id, MISSING_REQUIRED_CLIENT_CAPABILITY, "Missing required client capability", RequiredTasksCapabilityData());
    Value params = request["params"];
    if(!params.Is<ValueMap>()) return JsonRpcError(id, -32602, "Invalid params");
    Value task_id = params["taskId"];
    if(IsNull(task_id) || !task_id.Is<String>()) return JsonRpcError(id, -32602, "taskId is required");
    String path, error;
    if(!TaskTrackResolveTaskPath(AsString(task_id), String(), path, error)) return JsonRpcError(id, -32602, error);
    if(method == "tasks/get") {
        TaskTrackDocument doc;
        if(!TaskTrackLoad(path, doc, error)) return JsonRpcError(id, -32603, error);
        TaskTrackTouchAgentPoll(path);
        return JsonRpcResult(id, DetailedTaskValue(doc, path));
    }
    if(method == "tasks/cancel") {
        TaskTrackDocument doc;
        if(!TaskTrackLoad(path, doc, error)) return JsonRpcError(id, -32603, error);
        if(doc.state != TaskTrackState::Completed && doc.state != TaskTrackState::Closed)
            if(!TaskTrackSetState(path, TaskTrackState::Closed, error)) return JsonRpcError(id, -32603, error);
        ValueMap result; result.Add("resultType", "complete"); AddModernMeta(result);
        return JsonRpcResult(id, result);
    }
    if(method == "tasks/update")
        return JsonRpcError(id, -32602, "TaskTrack has no outstanding protocol inputRequests; use the GUI and poll tasks/get.");
    return JsonRpcError(id, -32601, "Method not found");
}

Value HandleRequest(const Value& request, bool& has_response)
{
    has_response = false;
    if(!request.Is<ValueMap>()) { has_response = true; return JsonRpcError(Value(), -32600, "Invalid Request"); }
    Value method_value = request["method"];
    if(IsNull(method_value) || !method_value.Is<String>()) { has_response = true; return JsonRpcError(request["id"], -32600, "Invalid Request"); }
    String method = AsString(method_value); Value id = request["id"]; bool has_id = !IsNull(id);
    if(method == "notifications/initialized") return Value();

    String protocol = RequestProtocol(request);
    if(!protocol.IsEmpty() && protocol != CURRENT_PROTOCOL) {
        if(!has_id) return Value();
        ValueMap data; ValueArray supported; supported.Add(CURRENT_PROTOCOL); data.Add("supported", supported); data.Add("requested", protocol);
        has_response = true; return JsonRpcError(id, UNSUPPORTED_PROTOCOL_VERSION, "Unsupported protocol version", data);
    }
    if(method == "server/discover") { if(has_id) { has_response = true; return JsonRpcResult(id, BuildDiscoverResult()); } return Value(); }
    if(method == "initialize") {
        if(has_id) { Value params = request["params"]; if(!params.Is<ValueMap>()) params = Value(ValueMap()); has_response = true; return JsonRpcResult(id, BuildLegacyInitialize(params)); }
        return Value();
    }
    if(method == "ping") {
        if(has_id) { ValueMap result; if(IsModern(request)) { result.Add("resultType", "complete"); AddModernMeta(result); } has_response = true; return JsonRpcResult(id, result); }
