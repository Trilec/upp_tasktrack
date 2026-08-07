        return Value();
    }
    if(method == "tools/list") { if(has_id) { has_response = true; return JsonRpcResult(id, BuildToolsList(IsModern(request))); } return Value(); }
    if(method == "tools/call") {
        if(!has_id) return Value();
        Value params = request["params"];
        if(!params.Is<ValueMap>()) { has_response = true; return JsonRpcError(id, -32602, "Invalid params"); }
        Value name = params["name"];
        if(IsNull(name) || !name.Is<String>()) { has_response = true; return JsonRpcError(id, -32602, "Tool call is missing string name"); }
        Value args = params["arguments"]; if(IsNull(args)) args = Value(ValueMap());
        if(!args.Is<ValueMap>()) { has_response = true; return JsonRpcError(id, -32602, "Tool arguments must be an object"); }
        has_response = true; return JsonRpcResult(id, ExecuteTool(AsString(name), args, IsModern(request), ClientSupportsTasks(request)));
    }
    if(method == "tasks/get" || method == "tasks/update" || method == "tasks/cancel") {
        if(has_id) { has_response = true; return HandleTaskExtension(request, method); }
        return Value();
    }
    if(has_id) { has_response = true; return JsonRpcError(id, -32601, "Method not found"); }
    return Value();
}

bool ProcessMessage(const String& message, String& response, bool& has_response)
{
    response.Clear(); has_response = false; Value request;
    try { request = ParseJSON(message); }
    catch(CParser::Error) { has_response = true; response = AsJSON(JsonRpcError(Value(), -32700, "Parse error"), false); return false; }
    Value result = HandleRequest(request, has_response);
    if(has_response) response = AsJSON(result, false);
    return true;
}

bool ReadMessage(String& message, bool& eof, String& error)
{
    message.Clear(); eof = false; error.Clear();
    for(;;) {
        int ch = fgetc(stdin);
        if(ch == EOF) { if(message.IsEmpty()) { eof = true; return false; } error = "Unexpected EOF while reading MCP message."; return false; }
        if(ch == '\n') return true;
        if(ch != '\r') message.Cat((char)ch);
        if(message.GetCount() > 16 * 1024 * 1024) { error = "MCP message exceeded 16 MB."; return false; }
    }
}

void WriteMessage(const String& message)
{
    fwrite(~message, 1, message.GetLength(), stdout); fputc('\n', stdout); fflush(stdout);
}

int RunServer()
{
    for(;;) {
        String message, error; bool eof = false;
        if(!ReadMessage(message, eof, error)) { if(eof) return 0; WriteMessage(AsJSON(JsonRpcError(Value(), -32700, error), false)); return 1; }
        String response; bool has_response = false; ProcessMessage(message, response, has_response); if(has_response) WriteMessage(response);
    }
}

int RunOneShot(const String& file)
{
    String request = LoadFile(file);
    if(IsNull(request)) { Cerr() << "TaskTrackMcp: unable to read " << file << "\n"; return 1; }
    String response; bool has_response = false; ProcessMessage(request, response, has_response);
    if(!has_response) return 1; WriteMessage(response); return 0;
}

struct SelfTest {
    Vector<String> failures;
    void Check(bool ok, const String& message) { if(!ok) failures.Add(message); }
};

Value ModernMeta(bool tasks)
{
    ValueMap extensions; if(tasks) extensions.Add(TASKS_EXTENSION, ValueMap());
    ValueMap caps; caps.Add("extensions", extensions);
    ValueMap meta; meta.Add("io.modelcontextprotocol/protocolVersion", CURRENT_PROTOCOL); meta.Add("io.modelcontextprotocol/clientCapabilities", caps);
    ValueMap info; info.Add("name", "tasktrack-selftest"); info.Add("version", "1"); meta.Add("io.modelcontextprotocol/clientInfo", info);
    return Value(meta);
}

Value Request(const String& method, int id, const Value& params)
{
    ValueMap r; r.Add("jsonrpc", "2.0"); r.Add("id", id); r.Add("method", method); r.Add("params", params); return Value(r);
}

int RunSelfTest()
{
    SelfTest st; bool has = false;
    ValueMap discover_params; discover_params.Add("_meta", ModernMeta(true));
    String discover_json = AsJSON(HandleRequest(Request("server/discover", 1, discover_params), has), false);
    st.Check(has && discover_json.Find(CURRENT_PROTOCOL) >= 0, "modern discovery failed");
    st.Check(discover_json.Find(TASKS_EXTENSION) >= 0, "discovery missing Tasks extension");
    st.Check(discover_json.Find("\"resultType\":\"complete\"") >= 0, "discovery missing modern resultType");

    Value legacy = ParseJSON("{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\"}}");
    String legacy_json = AsJSON(HandleRequest(legacy, has), false);
    st.Check(legacy_json.Find("tasktrack_mcp") >= 0, "legacy initialize missing server identity");
    st.Check(legacy_json.Find("resultType") < 0, "legacy initialize leaked modern resultType");

    ValueMap list_params; list_params.Add("_meta", ModernMeta(true));
    String list_json = AsJSON(HandleRequest(Request("tools/list", 3, list_params), has), false);
    st.Check(list_json.Find("create_task") >= 0, "tools/list missing create_task");
    const char *semantic_types[] = { "confirm", "single_choice", "multi_choice", "select", "list_select", "text", "notes", "number", "amount", "range", "rating", "color", "gradient", "position", "direction", "rank_order", "hierarchy_select", "curve" };
    for(const char *type : semantic_types)
        st.Check(list_json.Find(type) >= 0, String("tools/list missing semantic type ") + type);
    st.Check(list_json.Find("pass_fail") < 0, "canonical MCP schema still advertises legacy pass_fail");
    st.Check(list_json.Find("cacheScope") >= 0 && list_json.Find("ttlMs") >= 0, "modern tools/list is not cacheable");

    String root = AppendFileName(GetFileFolder(GetExeFilePath()), "_tasktrack_mcp_selftest"); RealizeDirectory(root);
    String task_id = TaskTrackMakeTaskId();
    ValueMap item; item.Add("id", "visual"); item.Add("category", "Visual"); item.Add("type", "single_choice");
    item.Add("title", "Window renders correctly"); ValueArray choices; choices.Add("Pass"); choices.Add("Fail"); item.Add("choices", choices);
    ValueArray items; items.Add(item);
    ValueMap args; args.Add("task_id", task_id); args.Add("title", "MCP self-test"); args.Add("store_root", root); args.Add("launch", false); args.Add("items", items);
    ValueMap call_params; call_params.Add("name", "create_task"); call_params.Add("arguments", args); call_params.Add("_meta", ModernMeta(true));
    String create_json = AsJSON(HandleRequest(Request("tools/call", 4, call_params), has), false);
    st.Check(create_json.Find("\"resultType\":\"task\"") >= 0, "modern create_task did not return task handle");
    st.Check(create_json.Find(task_id) >= 0, "task handle missing task id");
