/*
    TaskTrack MCP
    =============

    Thin stdio MCP frontend over TaskTrack/Core. The public contract exposes
    semantic human question types; U++ control names remain a renderer detail.

    Copyright (c) 2026 Curtis Edwards
    Licensed under the GNU General Public License, version 3. See LICENSE.
*/

#include <Core/Core.h>
#include <TaskTrack/Core/TaskTrackCore.h>
#include "TaskTrackAgentMcp.h"

#include <stdio.h>

using namespace Upp;

namespace {

static const char *CURRENT_PROTOCOL = "2026-07-28";
static const char *TASKS_EXTENSION = "io.modelcontextprotocol/tasks";
static const char *INTERACTIVE_STATE_PREFIX = "tasktrack-live:";
static const int UNSUPPORTED_PROTOCOL_VERSION = -32022;
static const int MISSING_REQUIRED_CLIENT_CAPABILITY = -32021;
static const int DEFAULT_WAIT_MS = 120000;
static const int MAX_WAIT_MS = 300000;
static const int INTERACTIVE_WAIT_MS = 300000;
static const int WAIT_SLICE_MS = 250;

Value ServerInfoValue()
{
    ValueMap info;
    info.Add("name", "tasktrack_mcp");
    info.Add("version", TaskTrackVersion());
    info.Add("buildVersion", TaskTrackBuildVersion());
    info.Add("description", "Durable structured human input for AI-assisted workflows.");
    return Value(info);
}

void AddModernMeta(ValueMap& result)
{
    ValueMap meta;
    meta.Add("io.modelcontextprotocol/serverInfo", ServerInfoValue());
    result.Add("_meta", meta);
}

Value JsonRpcResult(const Value& id, const Value& result)
{
    ValueMap response;
    response.Add("jsonrpc", "2.0");
    response.Add("id", id);
    response.Add("result", result);
    return Value(response);
}

Value JsonRpcError(const Value& id, int code, const String& message, const Value& data = Value())
{
    ValueMap e;
    e.Add("code", code);
    e.Add("message", message);
    if(!IsNull(data)) e.Add("data", data);
    ValueMap response;
    response.Add("jsonrpc", "2.0");
    response.Add("id", id);
    response.Add("error", e);
    return Value(response);
}

Value ToolText(const String& text)
{
    ValueMap c;
    c.Add("type", "text");
    c.Add("text", text);
    return Value(c);
}

Value BuildCallToolResult(const Value& structured, bool is_error, bool modern)
{
    ValueMap result;
    if(modern) result.Add("resultType", "complete");
    ValueArray content;
    content.Add(ToolText(AsJSON(structured, true)));
    result.Add("content", content);
    result.Add("structuredContent", structured);
    if(is_error) result.Add("isError", true);
    if(modern) AddModernMeta(result);
    return Value(result);
}

Value BuildFailure(const String& code, const String& message)
{
    ValueMap m;
    m.Add("ok", false);
    m.Add("code", code);
    m.Add("message", message);
    return Value(m);
}

String ReadString(const Value& map, const char *key, const String& def = String())
{
    Value v = map[key];
    return IsNull(v) ? def : AsString(v);
}

bool ReadBool(const Value& map, const char *key, bool def, bool& value, String& error)
{
    Value v = map[key];
    if(IsNull(v)) { value = def; return true; }
    if(!v.Is<bool>()) { error = String(key) + " must be a boolean."; return false; }
    value = (bool)v;
    return true;
}

bool ReadInt(const Value& map, const char *key, int def, int& value, String& error)
{
    Value v = map[key];
    if(IsNull(v)) { value = def; return true; }
    if(!IsNumber(v)) { error = String(key) + " must be numeric."; return false; }
    value = (int)v;
    return true;
}

String RequestProtocol(const Value& request)
{
    Value params = request["params"];
    if(!params.Is<ValueMap>()) return String();
    Value meta = params["_meta"];
    if(!meta.Is<ValueMap>()) return String();
    return ReadString(meta, "io.modelcontextprotocol/protocolVersion");
}

bool IsModern(const Value& request)
{
    return RequestProtocol(request) == CURRENT_PROTOCOL;
}

Value ClientCapabilities(const Value& request)
{
    Value params = request["params"];
    if(!params.Is<ValueMap>()) return Value();
    Value meta = params["_meta"];
    if(!meta.Is<ValueMap>()) return Value();
    Value caps = meta["io.modelcontextprotocol/clientCapabilities"];
    return caps.Is<ValueMap>() ? caps : Value();
}

bool ClientSupportsTasks(const Value& request)
{
    Value caps = ClientCapabilities(request);
    if(!caps.Is<ValueMap>()) return false;
    Value extensions = caps["extensions"];
    if(!extensions.Is<ValueMap>()) return false;
    return extensions[TASKS_EXTENSION].Is<ValueMap>();
}

bool ClientSupportsSampling(const Value& request)
{
    Value caps = ClientCapabilities(request);
    return caps.Is<ValueMap>() && caps["sampling"].Is<ValueMap>();
}

Value RequiredTasksCapabilityData()
{
    ValueMap extensions;
    extensions.Add(TASKS_EXTENSION, ValueMap());
    ValueMap required;
    required.Add("extensions", extensions);
    ValueMap data;
    data.Add("requiredCapabilities", required);
    return Value(data);
}

Value Schema(const String& type, const String& description = String())
{
    ValueMap s;
    if(!type.IsEmpty()) s.Add("type", type);
    if(!description.IsEmpty()) s.Add("description", description);
    return Value(s);
}

Value StringSchema(const String& description = String()) { return Schema("string", description); }
Value BoolSchema(const String& description = String()) { return Schema("boolean", description); }

Value NumberSchema(const String& description = String())
{
    return Schema("number", description);
}

Value IntSchema(int minimum = INT_MIN, int maximum = INT_MAX, const String& description = String())
{
    ValueMap s;
    s.Add("type", "integer");
    if(!description.IsEmpty()) s.Add("description", description);
    if(minimum != INT_MIN) s.Add("minimum", minimum);
    if(maximum != INT_MAX) s.Add("maximum", maximum);
    return Value(s);
}

Value StringArraySchema(const String& description)
{
    ValueMap s;
    s.Add("type", "array");
    s.Add("items", StringSchema());
    s.Add("description", description);
    return Value(s);
}

Value ToolAnnotations(bool read_only, bool destructive, bool idempotent)
{
    ValueMap a;
    a.Add("readOnlyHint", read_only);
    a.Add("destructiveHint", destructive);
    a.Add("idempotentHint", idempotent);
    return Value(a);
}

Value ToolSpec(const String& name, const String& description, const Value& schema,
               bool read_only, bool destructive, bool idempotent)
{
    ValueMap tool;
    tool.Add("name", name);
    tool.Add("description", description);
    tool.Add("inputSchema", schema);
    tool.Add("annotations", ToolAnnotations(read_only, destructive, idempotent));
    return Value(tool);
}

Value EmptyObjectSchema()
{
    ValueMap s;
    s.Add("type", "object");
    s.Add("properties", ValueMap());
    s.Add("additionalProperties", false);
    return Value(s);
}

Value TaskLocatorSchema(bool include_items = false)
{
    ValueMap props;
    props.Add("task_id", StringSchema("Stable TaskTrack task id returned by create_task."));
    props.Add("store_root", StringSchema("Optional store used when the task was created outside the default store."));
    if(include_items) {
        props.Add("include_items", BoolSchema("Include question definitions and structured answers."));
        props.Add("wait_ms", IntSchema(0, MAX_WAIT_MS,
            "Recovery/detached wait only. Normal launched create_task keeps its own MCP call active until the human finishes."));
    }
    ValueArray required; required.Add("task_id");
    ValueMap s;
    s.Add("type", "object"); s.Add("required", required); s.Add("properties", props); s.Add("additionalProperties", false);
    return Value(s);
}

Value GradientSchema()
{
    ValueMap props;
    props.Add("id", StringSchema("Stable option value returned to the agent."));
    props.Add("label", StringSchema("Short human label."));
    props.Add("from", StringSchema("Start colour as #RRGGBB."));
    props.Add("to", StringSchema("End colour as #RRGGBB."));
    ValueArray required; required.Add("id"); required.Add("from"); required.Add("to");
    ValueMap item; item.Add("type", "object"); item.Add("required", required); item.Add("properties", props); item.Add("additionalProperties", false);
    ValueMap array; array.Add("type", "array"); array.Add("items", item);
    return Value(array);
}

Value HierarchySchema()
{
    ValueMap props;
    props.Add("id", StringSchema("Stable node value returned to the agent."));
    props.Add("parent_id", StringSchema("Parent id; empty for a root-level node."));
    props.Add("label", StringSchema("Human-facing node label."));
    ValueArray required; required.Add("id"); required.Add("label");
    ValueMap item; item.Add("type", "object"); item.Add("required", required); item.Add("properties", props); item.Add("additionalProperties", false);
    ValueMap array; array.Add("type", "array"); array.Add("items", item);
    return Value(array);
}

Value CreateTaskSchema()
{
    ValueArray type_enum;
    const char *types[] = {
        "confirm", "single_choice", "multi_choice", "select", "list_select",
        "text", "notes", "number", "amount", "range", "rating", "color",
        "gradient", "position", "direction", "rank_order", "hierarchy_select", "curve"
    };
    for(const char *type : types) type_enum.Add(type);

    ValueMap type_schema;
    type_schema.Add("type", "string");
    type_schema.Add("enum", type_enum);
    type_schema.Add("description",
        "Semantic human response type. Choose by meaning, never by GUI widget: confirm=binary verification (ordinary human verification prefers choices=[\"Pass\",\"Fail\"]); single_choice=one of choices; multi_choice=several choices; select=compact populated selection; list_select=larger visible list; text=short exact text; notes=qualification/free text; number=exact numeric value; amount=magnitude on a scale; range=low/high bounds; rating=small ordinal score; color=colour choice; gradient=visual gradient choice; position=3x3 placement; direction=8-way direction; rank_order=dragged priority order; hierarchy_select=tree node; curve=easing/falloff curve." );

    ValueMap item_props;
    item_props.Add("id", StringSchema("Stable item id used in results and missing-required reports."));
    item_props.Add("category", StringSchema("Short grouping label. Use categories only when they help scanning."));
    item_props.Add("type", type_schema);
    item_props.Add("title", StringSchema("Concise human question, normally one line."));
    item_props.Add("instruction", StringSchema("Short concrete instruction or decision criterion. Avoid repeating the title."));
    item_props.Add("required", BoolSchema("True only when the agent cannot continue without an answer."));
    item_props.Add("choices", StringArraySchema("Discrete options for single_choice, multi_choice, select, list_select, rank_order; optionally two labels for confirm."));
    item_props.Add("allow_multiple", BoolSchema("Allow multiple selections for list_select or hierarchy_select."));
    item_props.Add("recommended", StringSchema(
        "Default expectation: provide the canonical recommendation. Omit only when no responsible proposal is possible; required+missing recommendation means the human must decide. Advisory only; default is neutral and neither becomes human evidence without explicit human action."));
    item_props.Add("min", NumberSchema("Lower bound for number/amount/range/rating."));
    item_props.Add("max", NumberSchema("Upper bound for number/amount/range/rating."));
    item_props.Add("step", NumberSchema("Positive numeric increment."));
    item_props.Add("unit", StringSchema("Short suffix such as px, %, ms, or variants."));
    item_props.Add("colors", StringArraySchema("Preset #RRGGBB colours for color."));
    item_props.Add("gradients", GradientSchema());
    item_props.Add("hierarchy", HierarchySchema());
    item_props.Add("default", ValueMap());

    ValueArray item_required; item_required.Add("type"); item_required.Add("title");
    ValueMap item_schema;
    item_schema.Add("type", "object"); item_schema.Add("required", item_required); item_schema.Add("properties", item_props); item_schema.Add("additionalProperties", false);

    ValueMap items;
    items.Add("type", "array"); items.Add("minItems", 1); items.Add("items", item_schema);
    items.Add("description", "Minimum durable human evidence only. One independent decision per item. Prefer structured types. Ordinary verification: confirm + [Pass,Fail].");

    ValueMap props;
    props.Add("task_id", StringSchema("Optional caller-supplied id beginning task-. Normally omit."));
    props.Add("project", StringSchema("Short project/product name."));
    props.Add("title", StringSchema("Overall objective the human is helping to resolve."));
    props.Add("subtitle", StringSchema("Optional one-line context."));
    props.Add("actor", StringSchema("Calling agent/session label."));
    props.Add("store_root", StringSchema("Optional durable store directory."));
    props.Add("reminder_minutes", IntSchema(0, 1440, "Inactivity reminder interval; 0 disables reminders."));
    props.Add("remind_while_paused", BoolSchema("Whether inactivity reminders may also appear for explicitly paused tasks."));
    props.Add("nudge_on_agent_poll", BoolSchema("Let a later recovery poll gently remind an inactive human without changing task state."));
    props.Add("history_limit", IntSchema(5, 200, "Bound retained completed/closed task history. Active work is never pruned for this limit."));
    props.Add("launch", BoolSchema("Launch the TaskTrack GUI. Defaults true. Normal launch keeps create_task active until the human completes; launch=false is the explicit detached/recovery path."));
    props.Add("items", items);

    ValueArray required; required.Add("title"); required.Add("items");
    ValueMap s;
    s.Add("type", "object"); s.Add("required", required); s.Add("properties", props); s.Add("additionalProperties", false);
    return Value(s);
}

Value BuildToolsList(bool modern)
{
    ValueArray tools;
    tools.Add(ToolSpec("version", "Use this to inspect TaskTrack version and protocol identity.", EmptyObjectSchema(), true, false, true));
    tools.Add(ToolSpec("create_task",
        "Create durable human evidence and, by default, keep this tool call active while the TaskTrack GUI is open. When the human completes or cancels, the final structured result returns directly from this create_task call so the agent continues without manual polling. If the human requests Suggest/Clarify, modern clients receive an in-call MCP input_required sampling round and then the same create_task call resumes. Use launch=false only for explicitly detached/recovery workflows. Do not use TaskTrack for ordinary conversation/explanation/opinion/information.",
        CreateTaskSchema(), false, false, false));
    tools.Add(ToolSpec("get_task",
        "Use this for recovery/detached lookup or to continue a compatibility interaction. Normal launched create_task waits for and returns the final human result directly; closed with no answer means no human evidence.",
        TaskLocatorSchema(true), true, false, true));
    tools.Add(ToolSpec("respond_to_request",
        "Use this only for a pending compatibility human-to-agent request: propose_answer uses recommended, clarify uses clarification, and continue_with_judgement has no payload. Modern create_task handles this in-call; proposals are advisory and never write human answers.",
        TaskTrackMcpRespondRequestSchema(), false, false, false));
    tools.Add(ToolSpec("open_task", "Use this to reopen the native human-decision GUI for an existing task_id.", TaskLocatorSchema(false), false, false, false));

    ValueMap list_props;
    list_props.Add("store_root", StringSchema());
    list_props.Add("limit", IntSchema(1, 200));
    ValueMap list_schema; list_schema.Add("type", "object"); list_schema.Add("properties", list_props); list_schema.Add("additionalProperties", false);
    tools.Add(ToolSpec("list_tasks", "Use this to find recent human-decision tasks or recover a task_id.", list_schema, true, false, true));
    tools.Add(ToolSpec("close_task", "Use this to end an unfinished task without fabricating human evidence.", TaskLocatorSchema(false), false, true, false));

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
    result.Add("instructions",
        "TaskTrack=durable human evidence, not ordinary chat. Normal launched create_task stays active until the human completes/cancels and returns the result directly; do not manually poll after a normal create_task. Human Suggest/Clarify is fulfilled through modern input_required when supported. get_task/respond_to_request are recovery/compatibility fallbacks. closed+no answer=no human evidence.");
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
    result.Add("instructions",
        "TaskTrack=durable human evidence, not ordinary chat. Normal launched create_task waits for terminal human state. Legacy clients without modern input_required use get_task/respond_to_request only as assistance fallback. closed+no answer=no human evidence.");
    return Value(result);
}

bool LaunchTaskGui(const String& task_path, String& error)
{
    String folder = GetFileFolder(GetExeFilePath());
    String gui = AppendFileName(folder, TaskTrackGuiExecutableName());
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

bool TaskHasMeaningfulAgentState(const TaskTrackDocument& doc, const String& path)
{
    return doc.state == TaskTrackState::Completed || doc.state == TaskTrackState::Closed ||
           TaskTrackMcpPendingAgentRequestCount(path, doc.task_id) > 0;
}

bool WaitAndReloadTask(const Value& args, TaskTrackDocument& doc, const String& path,
                       int wait_ms, String& error)
{
    if(wait_ms <= 0 || TaskHasMeaningfulAgentState(doc, path))
        return true;

    int elapsed = 0;
    while(elapsed < wait_ms) {
        int slice = min(WAIT_SLICE_MS, wait_ms - elapsed);
        Sleep(slice);
        elapsed += slice;
        TaskTrackDocument latest;
        if(!TaskTrackLoad(path, latest, error))
            return false;
        doc = pick(latest);
        if(TaskHasMeaningfulAgentState(doc, path))
            return true;
    }
    return true;
}

const TaskTrackItem* FindTaskItem(const TaskTrackDocument& doc, const String& item_id)
{
    for(const TaskTrackItem& item : doc.items)
        if(item.id == item_id)
            return &item;
    return nullptr;
}

String AgentSamplingPrompt(const TaskTrackDocument& doc, const TaskTrackItem& item,
                           const TaskTrackAgentRequest& request)
{
    String out;
    out << "TaskTrack human->agent assistance request.\n"
        << "Task: " << doc.title << "\n"
        << "Question: " << item.title << "\n";
    if(!item.instruction.IsEmpty())
        out << "Instruction: " << item.instruction << "\n";
    out << "Type: " << TaskTrackItemTypeName(item.type) << "\n";
    if(!item.choices.IsEmpty()) {
        out << "Allowed choices:";
        for(const String& choice : item.choices)
            out << " [" << choice << "]";
        out << "\n";
    }
    if(item.has_min || item.has_max) {
        out << "Numeric bounds:";
        if(item.has_min) out << " min=" << item.min_value;
        if(item.has_max) out << " max=" << item.max_value;
        if(!item.unit.IsEmpty()) out << " unit=" << item.unit;
        out << "\n";
    }

    if(request.action == "propose_answer")
        out << "Return one responsible recommendation compatible with the question. Return only the canonical answer/value, with no explanation or markdown.";
    else if(request.action == "clarify")
        out << "Restate the question or decision criterion more simply for the human. Return only a concise clarification, with no greeting or markdown.";
    else
        out << "The human explicitly authorized the agent to continue using its own judgement. Return only ACK.";
    return out;
}

Value AgentSamplingRequest(const TaskTrackDocument& doc, const TaskTrackItem& item,
                           const TaskTrackAgentRequest& request)
{
    ValueMap text;
    text.Add("type", "text");
    text.Add("text", AgentSamplingPrompt(doc, item, request));
    ValueMap message;
    message.Add("role", "user");
    message.Add("content", text);
    ValueArray messages;
    messages.Add(message);

    ValueMap params;
    params.Add("messages", messages);
    params.Add("systemPrompt", "Fulfil the TaskTrack assistance request exactly. Be terse and operational. Do not converse with the human.");
    params.Add("includeContext", "none");
    params.Add("temperature", 0.1);
    params.Add("maxTokens", request.action == "clarify" ? 160 : 80);

    ValueMap input;
    input.Add("method", "sampling/createMessage");
    input.Add("params", params);
    return Value(input);
}

ValueMap PendingTaskInputRequests(const TaskTrackDocument& doc, const String& path, String& error)
{
    ValueMap inputs;
    TaskTrackAgentChannel channel;
    if(!TaskTrackLoadAgentChannel(path, doc.task_id, channel, error))
        return inputs;

    for(const TaskTrackAgentRequest& request : channel.requests) {
        if(request.status != "pending")
            continue;
        const TaskTrackItem* item = FindTaskItem(doc, request.item_id);
        if(item)
            inputs.Add(request.id, AgentSamplingRequest(doc, *item, request));
    }
    return inputs;
}

String SamplingText(const Value& response)
{
    if(!response.Is<ValueMap>())
        return String();
    Value content = response["content"];
    if(content.Is<ValueMap>()) {
        if(ReadString(content, "type") == "text")
            return TrimBoth(ReadString(content, "text"));
        return String();
    }
    if(content.Is<ValueArray>()) {
        ValueArray blocks = content;
        for(int i = 0; i < blocks.GetCount(); ++i)
            if(blocks[i].Is<ValueMap>() && ReadString(blocks[i], "type") == "text")
                return TrimBoth(ReadString(blocks[i], "text"));
    }
    return String();
}

String NormalizeSampleRecommendation(const TaskTrackItem& item, String text)
{
    text = TrimBoth(text);
    if(text.GetCount() >= 2 && ((text[0] == '"' && text[text.GetCount() - 1] == '"') ||
                               (text[0] == '`' && text[text.GetCount() - 1] == '`')))
        text = TrimBoth(text.Mid(1, text.GetCount() - 2));

    if(!item.choices.IsEmpty()) {
        String lower = ToLower(text);
        for(const String& choice : item.choices)
            if(ToLower(TrimBoth(choice)) == lower)
                return choice;
    }
    return text;
}

bool ApplyTaskInputResponses(const String& task_id, const String& path, const Value& raw_responses,
                             String& error)
{
    if(!raw_responses.Is<ValueMap>()) {
        error = "inputResponses must be an object.";
        return false;
    }

    TaskTrackDocument doc;
    if(!TaskTrackLoad(path, doc, error))
        return false;
    TaskTrackAgentChannel channel;
    if(!TaskTrackLoadAgentChannel(path, task_id, channel, error))
        return false;

    ValueMap responses = raw_responses;
    for(const TaskTrackAgentRequest& request : channel.requests) {
        if(request.status != "pending")
            continue;
        Value response = responses[request.id];
        if(IsNull(response))
            continue;

        TaskTrackItem* item = TaskTrackMcpFindItem(doc, request.item_id);
        if(!item)
            continue;

        String text = SamplingText(response);
        ValueMap args;
        args.Add("task_id", task_id);
        args.Add("request_id", request.id);
        if(request.action == "propose_answer") {
            if(text.IsEmpty()) {
                error = "sampling response for propose_answer contained no text.";
                return false;
            }
            args.Add("recommended", NormalizeSampleRecommendation(*item, text));
        }
        else if(request.action == "clarify") {
            if(text.IsEmpty()) {
                error = "sampling response for clarify contained no text.";
                return false;
            }
            args.Add("clarification", text);
        }

        bool ok = false;
        String code;
        Value result = TaskTrackMcpRespondAgentRequest(args, ok, code);
        if(!ok) {
            error = code + ": " + ReadString(result, "message", "unable to apply task input response");
            return false;
        }
    }
    return true;
}

String InteractiveState(const String& task_id)
{
    return String(INTERACTIVE_STATE_PREFIX) + task_id;
}

bool ParseInteractiveState(const String& state, String& task_id, String& error)
{
    task_id.Clear();
    String prefix = INTERACTIVE_STATE_PREFIX;
    if(!state.StartsWith(prefix)) {
        error = "TaskTrack requestState is invalid.";
        return false;
    }
    task_id = state.Mid(prefix.GetCount());
    if(!task_id.StartsWith("task-") || task_id.GetCount() <= 5) {
        error = "TaskTrack requestState contains an invalid task id.";
        task_id.Clear();
        return false;
    }
    return true;
}

bool InteractiveArgsMatchTask(const Value& args, const TaskTrackDocument& doc, String& error)
{
    if(!args.Is<ValueMap>()) {
        error = "interactive TaskTrack retry arguments must be an object.";
        return false;
    }
    if(TrimBoth(ReadString(args, "title")) != doc.title) {
        error = "TaskTrack requestState does not match the retried task title.";
        return false;
    }
    String supplied_id = TrimBoth(ReadString(args, "task_id"));
    if(!supplied_id.IsEmpty() && supplied_id != doc.task_id) {
        error = "TaskTrack requestState does not match the supplied task_id.";
        return false;
    }
    Value raw_items = args["items"];
    if(!raw_items.Is<ValueArray>()) {
        error = "TaskTrack retry is missing items.";
        return false;
    }
    ValueArray items = raw_items;
    if(items.GetCount() != doc.items.GetCount()) {
        error = "TaskTrack requestState does not match the retried item count.";
        return false;
    }
    for(int i = 0; i < items.GetCount(); ++i) {
        if(!items[i].Is<ValueMap>()) {
            error = "TaskTrack retry item is invalid.";
            return false;
        }
        String title = TrimBoth(ReadString(items[i], "title"));
        TaskTrackItemType type;
        if(title != doc.items[i].title || !TaskTrackParseItemType(ReadString(items[i], "type"), type) || type != doc.items[i].type) {
            error = Format("TaskTrack requestState does not match retried item %d.", i);
            return false;
        }
        String id = TrimBoth(ReadString(items[i], "id"));
        if(!id.IsEmpty() && id != doc.items[i].id) {
            error = Format("TaskTrack requestState does not match retried item id %d.", i);
            return false;
        }
    }
    return true;
}

Value InteractiveInputRequired(const TaskTrackDocument& doc, const String& path, String& error)
{
    ValueMap inputs = PendingTaskInputRequests(doc, path, error);
    if(inputs.IsEmpty())
        return Value();

    ValueMap result;
    result.Add("resultType", "input_required");
    result.Add("inputRequests", inputs);
    result.Add("requestState", InteractiveState(doc.task_id));
    AddModernMeta(result);
    return Value(result);
}

Value TaskHandleValue(const TaskTrackDocument& doc, const String& status_message)
{
    ValueMap result;
    result.Add("resultType", "task"); result.Add("taskId", doc.task_id); result.Add("status", "working");
    result.Add("statusMessage", status_message); result.Add("createdAt", doc.created_at); result.Add("lastUpdatedAt", doc.updated_at);
    result.Add("ttlMs", Value()); result.Add("pollIntervalMs", 2000); AddModernMeta(result);
    return Value(result);
}

Value DetailedTaskValue(const TaskTrackDocument& doc, const String& path, bool sampling_capability)
{
    ValueMap result;
    result.Add("resultType", "complete"); result.Add("taskId", doc.task_id); result.Add("createdAt", doc.created_at);
    result.Add("lastUpdatedAt", doc.updated_at); result.Add("ttlMs", Value()); result.Add("pollIntervalMs", 2000);
    if(doc.state == TaskTrackState::Completed) {
        result.Add("status", "completed"); result.Add("statusMessage", "Human input completed.");
        result.Add("result", BuildCallToolResult(TaskTrackMcpAugmentAgentStatus(TaskTrackStatusValue(doc, path, true), path, doc.task_id), false, true));
    }
    else if(doc.state == TaskTrackState::Closed) {
        result.Add("status", "cancelled"); result.Add("statusMessage", "Task closed; no additional human evidence.");
    }
    else {
        int pending = TaskTrackMcpPendingAgentRequestCount(path, doc.task_id);
        if(pending > 0 && sampling_capability) {
            String input_error;
            ValueMap inputs = PendingTaskInputRequests(doc, path, input_error);
            if(!inputs.IsEmpty()) {
                result.Add("status", "input_required");
                result.Add("statusMessage", "Human requested agent assistance.");
                result.Add("inputRequests", inputs);
            }
            else {
                result.Add("status", "working");
                result.Add("statusMessage", input_error.IsEmpty()
                           ? "Human requested agent assistance; recovery get_task/respond_to_request is available."
                           : "Human requested agent assistance; unable to build task input request: " + input_error);
            }
        }
        else {
            result.Add("status", "working");
            result.Add("statusMessage", pending > 0
                       ? Format("agent_action_required:%d; use get_task/respond_to_request recovery fallback.", pending)
                       : "TaskTrack state: " + TaskTrackStateName(doc.state));
        }
    }
    AddModernMeta(result);
    return Value(result);
}

Value FinalInteractiveResult(const TaskTrackDocument& doc, const String& path, bool modern)
{
    ValueMap status = TaskTrackMcpAugmentAgentStatus(TaskTrackStatusValue(doc, path, true), path, doc.task_id);
    status.Add("delivery", "direct_create_task_result");
    status.Add("agent_poll_required", false);
    if(doc.state == TaskTrackState::Completed)
        status.Add("message", "Human input completed; continue using items[].answer.data as authority.");
    else if(doc.state == TaskTrackState::Closed)
        status.Add("message", "Task closed; no additional human evidence was supplied.");
    return BuildCallToolResult(status, false, modern);
}

Value WaitForInteractiveResult(TaskTrackDocument& doc, const String& path, const Value& request,
                               bool modern, bool task_capability)
{
    int elapsed = 0;
    String error;
    for(;;) {
        if(elapsed > 0) {
            TaskTrackDocument latest;
            if(!TaskTrackLoad(path, latest, error))
                return BuildCallToolResult(BuildFailure("TASK_LOAD_FAILED", error), true, modern);
            doc = pick(latest);
        }

        if(doc.state == TaskTrackState::Completed || doc.state == TaskTrackState::Closed)
            return FinalInteractiveResult(doc, path, modern);

        if(TaskTrackMcpPendingAgentRequestCount(path, doc.task_id) > 0) {
            if(modern && ClientSupportsSampling(request)) {
                Value input = InteractiveInputRequired(doc, path, error);
                if(!IsNull(input))
                    return input;
            }

            // Older/non-sampling clients cannot service MRTR automatically.
            // Return to the agent with an explicit fallback action instead of
            // leaving the human staring at Waiting forever.
            ValueMap status = TaskTrackMcpAugmentAgentStatus(TaskTrackStatusValue(doc, path, true), path, doc.task_id);
            status.Add("delivery", "compatibility_fallback");
            status.Add("interaction_blocked", true);
            status.Add("next_action", "Call respond_to_request for each pending request, then get_task(wait_ms) until the human completes.");
            return BuildCallToolResult(status, false, modern);
        }

        if(elapsed >= INTERACTIVE_WAIT_MS) {
            if(modern && task_capability)
                return TaskHandleValue(doc, "Live human wait exceeded five minutes; durable task remains open. Poll tasks/get as a long-running recovery task.");
            ValueMap status = TaskTrackStatusValue(doc, path, true);
            status.Add("delivery", "wait_timeout");
            status.Add("wait_timed_out", true);
            status.Add("next_action", "The durable GUI task remains open; use get_task for recovery.");
            return BuildCallToolResult(status, false, modern);
        }

        Sleep(WAIT_SLICE_MS);
        elapsed += WAIT_SLICE_MS;
    }
}

Value ExecuteTool(const String& name, const Value& args, const Value& request)
{
    bool modern = IsModern(request);
    bool task_capability = ClientSupportsTasks(request);

    if(name == "version") {
        ValueMap v; v.Add("ok", true); v.Add("version", TaskTrackVersion()); v.Add("build_version", TaskTrackBuildVersion()); v.Add("schema_version", 2);
        v.Add("question_types", 18); v.Add("current_protocol", CURRENT_PROTOCOL); v.Add("tasks_extension", TASKS_EXTENSION);
        v.Add("live_create_task", true); v.Add("mrtr_assistance", true);
        return BuildCallToolResult(v, false, modern);
    }
    if(name == "create_task") {
        bool launch = true;
        String error;
        if(!ReadBool(args, "launch", true, launch, error))
            return BuildCallToolResult(BuildFailure("BAD_REQUEST", error), true, modern);

        Value params = request["params"];
        String request_state;
        Value input_responses;
        if(params.Is<ValueMap>()) {
            Value raw_state = params["requestState"];
            if(!IsNull(raw_state)) {
                if(!raw_state.Is<String>())
                    return BuildCallToolResult(BuildFailure("BAD_REQUEST", "requestState must be a string."), true, modern);
                request_state = AsString(raw_state);
            }
            input_responses = params["inputResponses"];
        }

        TaskTrackDocument doc;
        String path;
        bool reentry = !request_state.IsEmpty();
        if(reentry) {
            if(!modern)
                return BuildCallToolResult(BuildFailure("BAD_REQUEST", "requestState is only valid for the modern MCP multi-round-trip flow."), true, modern);
            String task_id;
            if(!ParseInteractiveState(request_state, task_id, error))
                return BuildCallToolResult(BuildFailure("BAD_REQUEST", error), true, modern);
            if(!TaskTrackResolveTaskPath(task_id, ReadString(args, "store_root"), path, error) || !TaskTrackLoad(path, doc, error))
                return BuildCallToolResult(BuildFailure("TASK_NOT_FOUND", error), true, modern);
            if(!InteractiveArgsMatchTask(args, doc, error))
                return BuildCallToolResult(BuildFailure("BAD_REQUEST", error), true, modern);
            if(!IsNull(input_responses)) {
                if(!ApplyTaskInputResponses(doc.task_id, path, input_responses, error))
                    return BuildCallToolResult(BuildFailure("INPUT_RESPONSE_FAILED", error), true, modern);
                if(!TaskTrackLoad(path, doc, error))
                    return BuildCallToolResult(BuildFailure("TASK_LOAD_FAILED", error), true, modern);
            }
        }
        else {
            if(!IsNull(input_responses))
                return BuildCallToolResult(BuildFailure("BAD_REQUEST", "inputResponses require TaskTrack requestState."), true, modern);
            if(!TaskTrackCreateFromArguments(args, doc, path, error))
                return BuildCallToolResult(BuildFailure("CREATE_FAILED", error), true, modern);

            bool launched = false;
            String launch_error;
            if(launch)
                launched = LaunchTaskGui(path, launch_error);

            if(!launch) {
                if(modern && task_capability)
                    return TaskHandleValue(doc, "Durable detached TaskTrack task created; GUI launch disabled.");
                ValueMap status = TaskTrackStatusValue(doc, path, true);
                status.Add("launched", false);
                status.Add("detached", true);
                status.Add("next_action", "Open or poll this task only because launch=false explicitly requested detached operation.");
                return BuildCallToolResult(status, false, modern);
            }

            if(!launched) {
                ValueMap status = TaskTrackStatusValue(doc, path, true);
                status.Add("launched", false);
                status.Add("launch_error", launch_error);
                status.Add("delivery", "launch_failed");
                return BuildCallToolResult(status, true, modern);
            }
        }

        // This is the primary TaskTrack path. The initiating tool call stays
        // owned until the GUI reaches a terminal state. A human Suggest/Clarify
        // returns input_required; the client retries this same call with
        // inputResponses/requestState, after which we resume waiting.
        return WaitForInteractiveResult(doc, path, request, modern, task_capability);
    }
    if(name == "get_task") {
        TaskTrackDocument doc; String path, error;
        if(!ResolveAndLoad(args, doc, path, error)) return BuildCallToolResult(BuildFailure("TASK_NOT_FOUND", error), true, modern);
        bool include_items = true;
        if(!ReadBool(args, "include_items", true, include_items, error)) return BuildCallToolResult(BuildFailure("BAD_REQUEST", error), true, modern);
        int wait_ms = 0;
        if(!ReadInt(args, "wait_ms", 0, wait_ms, error)) return BuildCallToolResult(BuildFailure("BAD_REQUEST", error), true, modern);
        if(wait_ms < 0 || wait_ms > MAX_WAIT_MS) return BuildCallToolResult(BuildFailure("BAD_REQUEST", "wait_ms must be 0..300000."), true, modern);
        TaskTrackTouchAgentPoll(path);
        if(!WaitAndReloadTask(args, doc, path, wait_ms, error)) return BuildCallToolResult(BuildFailure("TASK_LOAD_FAILED", error), true, modern);
        ValueMap status = TaskTrackMcpAugmentAgentStatus(TaskTrackStatusValue(doc, path, include_items), path, doc.task_id);
        if(doc.state != TaskTrackState::Completed && doc.state != TaskTrackState::Closed && !status["agent_action_required"].Is<bool>())
            status.Add("agent_action_required", false);
        if(doc.state != TaskTrackState::Completed && doc.state != TaskTrackState::Closed) {
            status.Add("next_action", (bool)status["agent_action_required"]
                       ? Value("respond_to_request, then call get_task with wait_ms again")
                       : Value("call get_task with wait_ms again"));
            status.Add("suggested_wait_ms", DEFAULT_WAIT_MS);
        }
        return BuildCallToolResult(status, false, modern);
    }
    if(name == "respond_to_request") {
        bool ok = false; String code;
        Value result = TaskTrackMcpRespondAgentRequest(args, ok, code);
        if(!ok)
            return BuildCallToolResult(BuildFailure(code, ReadString(result, "message", "request failed")), true, modern);
        ValueMap out = result;
        out.Add("next_action", "call get_task with wait_ms again only on the compatibility/recovery path");
        out.Add("suggested_wait_ms", DEFAULT_WAIT_MS);
        return BuildCallToolResult(out, false, modern);
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
        return JsonRpcResult(id, DetailedTaskValue(doc, path, ClientSupportsSampling(request)));
    }
    if(method == "tasks/update") {
        Value input_responses = params["inputResponses"];
        if(!input_responses.Is<ValueMap>())
            return JsonRpcError(id, -32602, "tasks/update requires inputResponses object");
        if(!ApplyTaskInputResponses(AsString(task_id), path, input_responses, error))
            return JsonRpcError(id, -32602, error);
        ValueMap result; result.Add("resultType", "complete"); AddModernMeta(result);
        return JsonRpcResult(id, result);
    }
    if(method == "tasks/cancel") {
        TaskTrackDocument doc;
        if(!TaskTrackLoad(path, doc, error)) return JsonRpcError(id, -32603, error);
        if(doc.state != TaskTrackState::Completed && doc.state != TaskTrackState::Closed)
            if(!TaskTrackSetState(path, TaskTrackState::Closed, error)) return JsonRpcError(id, -32603, error);
        ValueMap result; result.Add("resultType", "complete"); AddModernMeta(result);
        return JsonRpcResult(id, result);
    }
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
        has_response = true; return JsonRpcResult(id, ExecuteTool(AsString(name), args, request));
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

Value ModernMeta(bool tasks, bool sampling = true)
{
    ValueMap extensions; if(tasks) extensions.Add(TASKS_EXTENSION, ValueMap());
    ValueMap caps; caps.Add("extensions", extensions); if(sampling) caps.Add("sampling", ValueMap());
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
    Cout() << "tasktrack-mcp-selftest build " << TaskTrackBuildVersion() << "\n";
    SelfTest st; bool has = false;
    ValueMap discover_params; discover_params.Add("_meta", ModernMeta(true));
    String discover_json = AsJSON(HandleRequest(Request("server/discover", 1, discover_params), has), false);
    st.Check(has && discover_json.Find(CURRENT_PROTOCOL) >= 0, "modern discovery failed");
    st.Check(discover_json.Find(TASKS_EXTENSION) >= 0, "discovery missing Tasks extension");
    st.Check(discover_json.Find("\"resultType\":\"complete\"") >= 0, "discovery missing modern resultType");
    st.Check(discover_json.Find("Normal launched create_task") >= 0 && discover_json.Find("closed+no answer") >= 0,
             "modern discovery missing direct live lifecycle rule");

    Value legacy = ParseJSON("{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2025-11-25\"}}");
    String legacy_json = AsJSON(HandleRequest(legacy, has), false);
    st.Check(legacy_json.Find("tasktrack_mcp") >= 0, "legacy initialize missing server identity");
    st.Check(legacy_json.Find("resultType") < 0, "legacy initialize leaked modern resultType");

    ValueMap list_params; list_params.Add("_meta", ModernMeta(true));
    String list_json = AsJSON(HandleRequest(Request("tools/list", 3, list_params), has), false);
    st.Check(list_json.Find("create_task") >= 0, "tools/list missing create_task");
    st.Check(list_json.Find("respond_to_request") >= 0, "tools/list missing respond_to_request");
    st.Check(list_json.Find("direct") >= 0 && list_json.Find("input_required") >= 0,
             "create_task description missing direct-return/MRTR contract");
    st.Check(list_json.Find("propose_answer") >= 0 && list_json.Find("clarify") >= 0 &&
             list_json.Find("continue_with_judgement") >= 0,
             "respond_to_request schema missing compact actions");
    const char *semantic_types[] = { "confirm", "single_choice", "multi_choice", "select", "list_select", "text", "notes", "number", "amount", "range", "rating", "color", "gradient", "position", "direction", "rank_order", "hierarchy_select", "curve" };
    for(const char *type : semantic_types)
        st.Check(list_json.Find(type) >= 0, String("tools/list missing semantic type ") + type);
    st.Check(list_json.Find("pass_fail") < 0, "canonical MCP schema still advertises legacy pass_fail");
    st.Check(list_json.Find("advisory") >= 0 && list_json.Find("human evidence") >= 0,
             "create_task schema missing recommendation/evidence boundary");
    st.Check(list_json.Find("cacheScope") >= 0 && list_json.Find("ttlMs") >= 0, "modern tools/list is not cacheable");

    String root = AppendFileName(GetFileFolder(GetExeFilePath()), "_tasktrack_mcp_selftest"); RealizeDirectory(root);
    String task_id = TaskTrackMakeTaskId();
    ValueMap item; item.Add("id", "visual"); item.Add("category", "Visual"); item.Add("type", "single_choice");
    item.Add("title", "Window renders correctly"); ValueArray choices; choices.Add("Pass"); choices.Add("Fail"); item.Add("choices", choices);
    ValueArray items; items.Add(item);
    ValueMap args; args.Add("task_id", task_id); args.Add("title", "MCP self-test"); args.Add("store_root", root); args.Add("launch", false); args.Add("items", items);
    ValueMap call_params; call_params.Add("name", "create_task"); call_params.Add("arguments", args); call_params.Add("_meta", ModernMeta(true));
    String create_json = AsJSON(HandleRequest(Request("tools/call", 4, call_params), has), false);
    st.Check(create_json.Find("\"resultType\":\"task\"") >= 0, "detached modern create_task did not return task handle");
    st.Check(create_json.Find(task_id) >= 0, "task handle missing task id");
    st.Check(create_json.Find("Durable detached") >= 0, "detached task handle missing explicit lifecycle message");

    String task_path = TaskTrackMakeTaskPath(root, task_id);
    st.Check(FileExists(task_path), "create_task returned before durable task existed");

    String error, request_id;
    st.Check(TaskTrackQueueAgentRequest(task_path, task_id, "visual", "propose_answer", String(), request_id, error),
             "unable to queue self-test agent request: " + error);

    ValueMap get_args; get_args.Add("task_id", task_id); get_args.Add("store_root", root); get_args.Add("include_items", true); get_args.Add("wait_ms", 1000);
    ValueMap get_call; get_call.Add("name", "get_task"); get_call.Add("arguments", get_args); get_call.Add("_meta", ModernMeta(true));
    String agent_get_json = AsJSON(HandleRequest(Request("tools/call", 5, get_call), has), false);
    st.Check(agent_get_json.Find("\"agent_action_required\":true") >= 0 && agent_get_json.Find(request_id) >= 0,
             "get_task did not surface pending human agent request");

    ValueMap respond_args; respond_args.Add("task_id", task_id); respond_args.Add("store_root", root);
    respond_args.Add("request_id", request_id); respond_args.Add("recommended", "Pass");
    ValueMap respond_call; respond_call.Add("name", "respond_to_request"); respond_call.Add("arguments", respond_args); respond_call.Add("_meta", ModernMeta(true));
    String respond_json = AsJSON(HandleRequest(Request("tools/call", 6, respond_call), has), false);
    st.Check(respond_json.Find("\"status\":\"answered\"") >= 0,
             "respond_to_request did not answer propose_answer");

    // Direct MRTR assistance path. Keep the phases in lifecycle order:
    // input_required -> agent response settles sidecar -> awaiting_human ->
    // human completion -> terminal create_task result.
    String live_task_id = TaskTrackMakeTaskId();
    ValueMap live_args = clone(args);
    live_args.Set("task_id", live_task_id);
    live_args.Set("title", "MCP live self-test");
    ValueMap live_create; live_create.Add("name", "create_task"); live_create.Add("arguments", live_args); live_create.Add("_meta", ModernMeta(true));
    String live_create_json = AsJSON(HandleRequest(Request("tools/call", 70, live_create), has), false);
    st.Check(live_create_json.Find(live_task_id) >= 0, "unable to create live self-test backing task");
    String live_path = TaskTrackMakeTaskPath(root, live_task_id);
    String live_request_id;
    st.Check(TaskTrackQueueAgentRequest(live_path, live_task_id, "visual", "propose_answer", String(), live_request_id, error),
             "unable to queue live MRTR proposal: " + error);

    ValueMap live_retry = live_create;
    live_retry.Add("requestState", InteractiveState(live_task_id));
    String mrtr_json = AsJSON(HandleRequest(Request("tools/call", 71, live_retry), has), false);
    st.Check(mrtr_json.Find("\"resultType\":\"input_required\"") >= 0,
             "live create_task did not surface Suggest as input_required");
    st.Check(mrtr_json.Find("sampling/createMessage") >= 0 && mrtr_json.Find(live_request_id) >= 0,
             "live create_task input_required missing sampling request");
    st.Check(mrtr_json.Find(InteractiveState(live_task_id)) >= 0,
             "live create_task input_required missing requestState");

    ValueMap sample_text; sample_text.Add("type", "text"); sample_text.Add("text", "Pass");
    ValueMap sample_result; sample_result.Add("role", "assistant"); sample_result.Add("content", sample_text); sample_result.Add("model", "selftest");
    ValueMap live_responses; live_responses.Add(live_request_id, sample_result);

    String live_apply_error;
    st.Check(ApplyTaskInputResponses(live_task_id, live_path, live_responses, live_apply_error),
             "live create_task sampling response application failed: " + live_apply_error);
    TaskTrackAgentChannel live_channel;
    st.Check(TaskTrackLoadAgentChannel(live_path, live_task_id, live_channel, error) &&
             !TaskTrackAgentHasPending(live_channel, "visual", "propose_answer") &&
             TaskTrackAgentLatestRecommendation(live_channel, "visual") == "Pass" &&
             TaskTrackAgentEffectiveInteractionState(live_channel) == "awaiting_human",
             "live create_task sampling response did not return assistance to awaiting_human");

    TaskTrackDocument live_doc;
    if(TaskTrackLoad(live_path, live_doc, error)) {
        live_doc.items[0].answer.answered = true;
        live_doc.items[0].answer.status = "accepted";
        live_doc.items[0].answer.value = "Pass";
        live_doc.items[0].answer.data = "Pass";
        live_doc.items[0].answer.answered_at = TaskTrackNowIso();
        live_doc.state = TaskTrackState::Completed;
        live_doc.updated_at = TaskTrackNowIso();
        st.Check(TaskTrackSave(live_path, live_doc, error), "unable to complete live self-test task");
    }
    else st.Check(false, "unable to load live self-test task: " + error);

    ValueMap live_final = live_create;
    live_final.Add("requestState", InteractiveState(live_task_id));
    String live_final_json = AsJSON(HandleRequest(Request("tools/call", 72, live_final), has), false);
    st.Check(live_final_json.Find("direct_create_task_result") >= 0 && live_final_json.Find("\"data\":\"Pass\"") >= 0,
             "live create_task did not return terminal human evidence directly");

    // Keep Tasks extension behavior as recovery compatibility.
    String task_input_id;
    st.Check(TaskTrackQueueAgentRequest(task_path, task_id, "visual", "propose_answer", String(), task_input_id, error),
             "unable to queue Tasks input request: " + error);
    ValueMap task_params; task_params.Add("taskId", task_id); task_params.Add("_meta", ModernMeta(true, true));
    String input_json = AsJSON(HandleRequest(Request("tasks/get", 61, task_params), has), false);
    st.Check(input_json.Find("\"status\":\"input_required\"") >= 0,
             "tasks/get did not expose pending assistance as input_required");
    st.Check(input_json.Find("sampling/createMessage") >= 0 && input_json.Find(task_input_id) >= 0,
             "tasks/get input_required missing sampling request");

    ValueMap task_responses; task_responses.Add(task_input_id, sample_result);
    ValueMap update_params; update_params.Add("taskId", task_id); update_params.Add("inputResponses", task_responses); update_params.Add("_meta", ModernMeta(true, true));
    String update_json = AsJSON(HandleRequest(Request("tasks/update", 62, update_params), has), false);
    st.Check(update_json.Find("\"resultType\":\"complete\"") >= 0,
             "tasks/update did not acknowledge sampling response");

    String cwj_request_id, cwj_error;
    st.Check(TaskTrackQueueAgentRequest(task_path, task_id, "visual", "continue_with_judgement", String(), cwj_request_id, cwj_error),
             "unable to queue continue_with_judgement: " + cwj_error);
    String cwj_dup_id, cwj_dup_error;
    st.Check(TaskTrackQueueAgentRequest(task_path, task_id, "visual", "continue_with_judgement", String(), cwj_dup_id, cwj_dup_error) && cwj_dup_id == cwj_request_id,
             "duplicate continue_with_judgement request was not reused");

    ValueMap cwj_get_args; cwj_get_args.Add("task_id", task_id); cwj_get_args.Add("store_root", root); cwj_get_args.Add("include_items", true); cwj_get_args.Add("wait_ms", 1000);
    ValueMap cwj_get_call; cwj_get_call.Add("name", "get_task"); cwj_get_call.Add("arguments", cwj_get_args); cwj_get_call.Add("_meta", ModernMeta(true));
    String cwj_get_json = AsJSON(HandleRequest(Request("tools/call", 51, cwj_get_call), has), false);
    st.Check(cwj_get_json.Find("\"agent_action_required\":true") >= 0 && cwj_get_json.Find(cwj_request_id) >= 0,
             "get_task did not surface pending continue_with_judgement");

    ValueMap cwj_respond_args; cwj_respond_args.Add("task_id", task_id); cwj_respond_args.Add("store_root", root); cwj_respond_args.Add("request_id", cwj_request_id);
    ValueMap cwj_respond_call; cwj_respond_call.Add("name", "respond_to_request"); cwj_respond_call.Add("arguments", cwj_respond_args); cwj_respond_call.Add("_meta", ModernMeta(true));
    String cwj_respond_json = AsJSON(HandleRequest(Request("tools/call", 52, cwj_respond_call), has), false);
    st.Check(cwj_respond_json.Find("\"status\":\"answered\"") >= 0,
             "continue_with_judgement did not reach answered");

    String get_json = AsJSON(HandleRequest(Request("tasks/get", 7, task_params), has), false);
    st.Check(get_json.Find("\"status\":\"working\"") >= 0, "tasks/get did not report working");

    TaskTrackDocument doc;
    if(TaskTrackLoad(task_path, doc, error)) {
        doc.items[0].answer.answered = true; doc.items[0].answer.status = "selected"; doc.items[0].answer.value = "Pass";
        doc.items[0].answer.data = "Pass"; doc.items[0].answer.answered_at = TaskTrackNowIso(); doc.state = TaskTrackState::Completed; doc.updated_at = TaskTrackNowIso();
        st.Check(TaskTrackSave(task_path, doc, error), "unable to complete self-test task");
    }
    else st.Check(false, "unable to load self-test task: " + error);

    get_json = AsJSON(HandleRequest(Request("tasks/get", 8, task_params), has), false);
    st.Check(get_json.Find("\"status\":\"completed\"") >= 0, "tasks/get did not report completed");
    st.Check(get_json.Find("structuredContent") >= 0 && get_json.Find("\"data\":\"Pass\"") >= 0,
             "completed task missing structured human result");

    ValueMap bad_meta; ValueMap meta; meta.Add("io.modelcontextprotocol/protocolVersion", "2026-01-01"); meta.Add("io.modelcontextprotocol/clientCapabilities", ValueMap()); bad_meta.Add("_meta", meta);
    String bad_json = AsJSON(HandleRequest(Request("tools/list", 9, bad_meta), has), false);
    st.Check(bad_json.Find("-32022") >= 0, "unsupported protocol revision was not rejected");

    FileDelete(task_path); FileDelete(task_path + ".bak"); FileDelete(TaskTrackPollMarkerPath(task_path));
    FileDelete(TaskTrackAgentChannelPath(task_path)); FileDelete(TaskTrackAgentChannelPath(task_path) + ".bak");
    String locator = AppendFileName(TaskTrackDefaultRegistryRoot(), task_id + ".path"); FileDelete(locator); FileDelete(locator + ".bak");

    FileDelete(live_path); FileDelete(live_path + ".bak"); FileDelete(TaskTrackPollMarkerPath(live_path));
    FileDelete(TaskTrackAgentChannelPath(live_path)); FileDelete(TaskTrackAgentChannelPath(live_path) + ".bak");
    String live_locator = AppendFileName(TaskTrackDefaultRegistryRoot(), live_task_id + ".path"); FileDelete(live_locator); FileDelete(live_locator + ".bak");
    DirectoryDelete(root);

    if(st.failures.IsEmpty()) { Cout() << "tasktrack-mcp-selftest: ok\n"; return 0; }
    Cout() << "tasktrack-mcp-selftest: failed\n";
    for(const String& failure : st.failures) Cout() << " - " << failure << "\n";
    return 1;
}

} // namespace

static String McpHelpText()
{
    String gui = TaskTrackGuiExecutableName();
    return
        "TaskTrack MCP\n"
        "Human-in-the-loop decision and verification server for AI agents.\n"
        "\n"
        "Usage:\n"
        "  TaskTrackMcp.exe\n"
        "      Run the stdio MCP server.\n"
        "  TaskTrackMcp.exe --help\n"
        "      Show this help.\n"
        "  TaskTrackMcp.exe --version\n"
        "      Show version information.\n"
        "  TaskTrackMcp.exe --selftest\n"
        "      Run deterministic MCP self-test.\n"
        "  TaskTrackMcp.exe --oneshot <request.json>\n"
        "      Process one MCP JSON-RPC request file and exit.\n"
        "      Diagnostic/test utility; not required for normal host registration.\n"
        "\n"
        "Runtime:\n"
        "  " + gui + " must be in the same directory.\n"
        "  Human tasks are opened with:\n"
        "      " + gui + " --task <task-file>\n"
        "\n"
        "Live interaction:\n"
        "  Normal launched create_task remains active until the human completes.\n"
        "  Final human evidence returns directly as the create_task result.\n"
        "  Suggest/Clarify uses MCP multi-round-trip input_required when supported.\n"
        "\n"
        "Transport:\n"
        "  stdio MCP\n"
        "  No arguments are required when registered with OpenCode or Codex.";
}

CONSOLE_APP_MAIN
{
    const Vector<String>& cmd = CommandLine();
    switch(TaskTrackClassifyMcpCommand(cmd)) {
    case TaskTrackMcpCommand::Server:
        SetExitCode(RunServer());
        return;
    case TaskTrackMcpCommand::OneShot:
        SetExitCode(RunOneShot(cmd[1]));
        return;
    case TaskTrackMcpCommand::SelfTest:
        SetExitCode(RunSelfTest());
        return;
    case TaskTrackMcpCommand::Help:
        Cout() << McpHelpText() << "\n";
        SetExitCode(0);
        return;
    case TaskTrackMcpCommand::Version:
        Cout() << "TaskTrack MCP\n"
               << "TaskTrack version " << TaskTrackVersion() << "\n"
               << "build version " << TaskTrackBuildVersion() << "\n"
               << "schema version " << 2 << "\n"
               << "MCP protocol " << CURRENT_PROTOCOL << "\n";
        SetExitCode(0);
        return;
    default:
        Cerr() << "Unknown TaskTrack MCP arguments.\n" << McpHelpText() << "\n";
        SetExitCode(2);
        return;
    }
}
