/*
    TaskTrack MCP
    =============

    Thin stdio MCP frontend over TaskTrack/Core. The public contract exposes
    semantic human question types; U++ control names remain a renderer detail.

    Copyright (c) 2026 Curtis Edwards
    Licensed under the Apache License, Version 2.0. See LICENSE.
*/

#include <Core/Core.h>
#include <TaskTrack/Core/TaskTrackCore.h>

#include <stdio.h>

using namespace Upp;

namespace {

static const char *CURRENT_PROTOCOL = "2026-07-28";
static const char *TASKS_EXTENSION = "io.modelcontextprotocol/tasks";
static const int UNSUPPORTED_PROTOCOL_VERSION = -32022;
static const int MISSING_REQUIRED_CLIENT_CAPABILITY = -32021;

Value ServerInfoValue()
{
    ValueMap info;
    info.Add("name", "tasktrack_mcp");
    info.Add("version", TaskTrackVersion());
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

bool ClientSupportsTasks(const Value& request)
{
    Value params = request["params"];
    if(!params.Is<ValueMap>()) return false;
    Value meta = params["_meta"];
    if(!meta.Is<ValueMap>()) return false;
    Value caps = meta["io.modelcontextprotocol/clientCapabilities"];
    if(!caps.Is<ValueMap>()) return false;
    Value extensions = caps["extensions"];
    if(!extensions.Is<ValueMap>()) return false;
    return extensions[TASKS_EXTENSION].Is<ValueMap>();
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
