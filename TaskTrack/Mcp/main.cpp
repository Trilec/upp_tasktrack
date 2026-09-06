/*
    TaskTrack MCP
    =============

    Unified stdio MCP frontend for TaskTrack human decisions and project
    dashboards. The accepted human-decision implementation is retained in
    TaskTrackHumanMcp.inc; this file owns the single public server boundary and
    adds the dashboard tool family without duplicating MCP registration.

    Copyright (c) 2026 Curtis Edwards
    Licensed under the GNU General Public License, version 3. See LICENSE.
*/

#include <Core/Core.h>
#include <TaskTrack/Core/TaskTrackCore.h>
#include <TaskTrack/Core/TaskTrackBuild.h>
#include <TaskTrack/DashboardCore/TaskTrackDashboardCore.h>
#include <TaskTrack/TunnelCore/TaskTrackTunnelCore.h>
#include "TaskTrackDashboardBridge.h"

using namespace Upp;

namespace {

Value UnifiedHandleRequest(const Value& request, bool& has_response);
bool UnifiedProcessMessage(const String& message, String& response, bool& has_response);
int UnifiedRunServer();
int UnifiedRunOneShot(const String& file);
int UnifiedRunSelfTest();
String UnifiedMcpHelpText();

} // namespace

CONSOLE_APP_MAIN
{
    const Vector<String>& cmd = CommandLine();
    switch(TaskTrackClassifyMcpCommand(cmd)) {
    case TaskTrackMcpCommand::Server:
        SetExitCode(UnifiedRunServer());
        return;
    case TaskTrackMcpCommand::OneShot:
        SetExitCode(UnifiedRunOneShot(cmd[1]));
        return;
    case TaskTrackMcpCommand::SelfTest:
        SetExitCode(UnifiedRunSelfTest());
        return;
    case TaskTrackMcpCommand::Help:
        Cout() << UnifiedMcpHelpText() << "\n";
        SetExitCode(0);
        return;
    case TaskTrackMcpCommand::Version:
        Cout() << "TaskTrack MCP\n"
               << "version " << TaskTrackBuildVersion() << "\n"
               << "task core version " << TaskTrackVersion() << "\n"
               << "task schema version 2\n"
               << "dashboard schema version " << TASKTRACK_DASHBOARD_SCHEMA_VERSION << "\n"
               << "MCP protocol 2026-07-28\n";
        SetExitCode(0);
        return;
    default:
        Cerr() << "Unknown TaskTrack MCP arguments.\n" << UnifiedMcpHelpText() << "\n";
        SetExitCode(2);
        return;
    }
}

// Preserve the accepted human-decision MCP implementation as one source slice.
// Only its request handler, self-test entry and obsolete console entry are
// renamed; all human lifecycle helpers and behavior remain unchanged.
#define HandleRequest TaskTrackHumanHandleRequest
#define RunSelfTest TaskTrackHumanRunSelfTest
#define McpHelpText TaskTrackHumanMcpHelpText
#undef CONSOLE_APP_MAIN
#define CONSOLE_APP_MAIN void TaskTrackHumanLegacyMain()
#include "TaskTrackHumanMcp.inc"
#undef CONSOLE_APP_MAIN
#undef McpHelpText
#undef RunSelfTest
#undef HandleRequest

namespace {

static const char *UNIFIED_PROTOCOL = "2026-07-28";

String UnifiedDescription()
{
    return "Durable human decisions and semantic project dashboards for AI-assisted workflows.";
}

void PatchServerInfo(ValueMap& result)
{
    Value meta_value = result["_meta"];
    if(meta_value.Is<ValueMap>()) {
        ValueMap meta = meta_value;
        Value info_value = meta["io.modelcontextprotocol/serverInfo"];
        if(info_value.Is<ValueMap>()) {
            ValueMap info = info_value;
            info.Set("name", "tasktrack_mcp");
            info.Set("version", TaskTrackBuildVersion());
            info.Set("buildVersion", TaskTrackBuildVersion());
            info.Set("description", UnifiedDescription());
            meta.Set("io.modelcontextprotocol/serverInfo", info);
            result.Set("_meta", meta);
        }
    }

    Value server_value = result["serverInfo"];
    if(server_value.Is<ValueMap>()) {
        ValueMap info = server_value;
        info.Set("name", "tasktrack_mcp");
        info.Set("version", TaskTrackBuildVersion());
        info.Set("buildVersion", TaskTrackBuildVersion());
        info.Set("description", UnifiedDescription());
        result.Set("serverInfo", info);
    }
}

void PatchUnifiedResponse(ValueMap& response)
{
    Value result_value = response["result"];
    if(!result_value.Is<ValueMap>())
        return;
    ValueMap result = result_value;
    PatchServerInfo(result);
    response.Set("result", result);
}

Value UnifiedVersionResult(bool modern)
{
    ValueMap out;
    out.Add("ok", true);
    out.Add("version", TaskTrackBuildVersion());
    out.Add("build_version", TaskTrackBuildVersion());
    out.Add("task_core_version", TaskTrackVersion());
    out.Add("schema_version", 2); // compatibility with the existing TaskTrack version response
    out.Add("task_schema_version", 2);
    out.Add("dashboard_schema_version", TASKTRACK_DASHBOARD_SCHEMA_VERSION);
    out.Add("question_types", 18);
    out.Add("dashboard_panel_types", 8);
    out.Add("dashboard_tools", true);
    out.Add("tunnel_probe", true);
    out.Add("current_protocol", UNIFIED_PROTOCOL);
    out.Add("tasks_extension", "io.modelcontextprotocol/tasks");
    out.Add("live_create_task", true);
    out.Add("mrtr_assistance", true);
    return BuildCallToolResult(out, false, modern);
}

Value UnifiedTunnelProbeToolSpec()
{
    ValueMap input;
    input.Add("type", "object");
    input.Add("additionalProperties", false);

    ValueMap annotations;
    annotations.Add("readOnlyHint", true);
    annotations.Add("destructiveHint", false);
    annotations.Add("idempotentHint", true);

    ValueMap tool;
    tool.Add("name", "tunnel_probe");
    tool.Add("description",
             "Use this when verifying that browser or remote tunnel access reaches this exact local TaskTrack runtime. It reads only the latest probe written by TaskTrackTunnelGui and never modifies tasks, dashboards, files, or repository state.");
    tool.Add("inputSchema", input);
    tool.Add("annotations", annotations);
    return Value(tool);
}

Value UnifiedTunnelProbeResult(bool modern)
{
    ValueMap out = TaskTrackTunnelProbeStatusValue();
    out.Set("build_version", TaskTrackBuildVersion());
    out.Set("task_schema_version", 2);
    out.Set("dashboard_schema_version", TASKTRACK_DASHBOARD_SCHEMA_VERSION);
    out.Set("transport", "stdio");
    return BuildCallToolResult(out, false, modern);
}

String ToolName(const Value& request)
{
    Value params = request["params"];
    if(!params.Is<ValueMap>())
        return String();
    Value name = params["name"];
    return name.Is<String>() ? AsString(name) : String();
}

Value ToolArguments(const Value& request)
{
    Value params = request["params"];
    if(!params.Is<ValueMap>())
        return Value(ValueMap());
    Value args = params["arguments"];
    return IsNull(args) ? Value(ValueMap()) : args;
}

Value UnifiedHandleRequest(const Value& request, bool& has_response)
{
    // Let the accepted human MCP validate JSON-RPC shape, protocol revision,
    // capabilities and common tool-call parameters first. Dashboard calls are
    // then substituted only after that gate succeeds.
    Value response_value = TaskTrackHumanHandleRequest(request, has_response);
    if(!has_response || !response_value.Is<ValueMap>())
        return response_value;

    ValueMap response = response_value;
    PatchUnifiedResponse(response);

    String method = request["method"].Is<String>() ? AsString(request["method"]) : String();

    if(method == "server/discover") {
        Value result_value = response["result"];
        if(result_value.Is<ValueMap>()) {
            ValueMap result = result_value;
            result.Set("instructions",
                       "TaskTrack is one MCP service for two separate authority domains: durable human evidence and AI-maintained project dashboards. Normal launched create_task stays active until the human completes/cancels. Dashboard tools maintain current project state and immutable revisions; dashboard state never becomes human answer.data.");
            response.Set("result", result);
        }
        return Value(response);
    }

    if(method == "initialize") {
        Value result_value = response["result"];
        if(result_value.Is<ValueMap>()) {
            ValueMap result = result_value;
            result.Set("instructions",
                       "TaskTrack provides durable human decisions and semantic project dashboards through one MCP server. Human answer.data remains authoritative human evidence; dashboard state is agent-authored project presentation state.");
            PatchServerInfo(result);
            response.Set("result", result);
        }
        return Value(response);
    }

    if(method == "tools/list") {
        Value result_value = response["result"];
        if(result_value.Is<ValueMap>()) {
            ValueMap result = result_value;
            Value tools_value = result["tools"];
            if(tools_value.Is<ValueArray>()) {
                ValueArray tools = tools_value;
                TaskTrackAppendDashboardToolSpecs(tools);
                tools.Add(UnifiedTunnelProbeToolSpec());
                result.Set("tools", tools);
            }
            PatchServerInfo(result);
            response.Set("result", result);
        }
        return Value(response);
    }

    if(method != "tools/call")
        return Value(response);

    // A top-level JSON-RPC error means the human MCP common validation failed;
    // preserve it exactly rather than attempting dashboard dispatch.
    if(!IsNull(response["error"]))
        return Value(response);

    String name = ToolName(request);
    if(name == "version") {
        response.Set("result", UnifiedVersionResult(IsModern(request)));
        PatchUnifiedResponse(response);
        return Value(response);
    }

    if(name == "tunnel_probe") {
        response.Set("result", UnifiedTunnelProbeResult(IsModern(request)));
        PatchUnifiedResponse(response);
        return Value(response);
    }

    if(TaskTrackIsDashboardTool(name)) {
        Value args = ToolArguments(request);
        if(!args.Is<ValueMap>())
            return Value(response); // human MCP already produced BAD_REQUEST for this shape
        TaskTrackDashboardToolResult dashboard = TaskTrackExecuteDashboardTool(name, args);
        response.Set("result", BuildCallToolResult(dashboard.structured,
                                                   dashboard.is_error,
                                                   IsModern(request)));
        PatchUnifiedResponse(response);
    }
    return Value(response);
}

bool UnifiedProcessMessage(const String& message, String& response, bool& has_response)
{
    response.Clear();
    has_response = false;
    Value request;
    try {
        request = ParseJSON(message);
    }
    catch(CParser::Error) {
        has_response = true;
        response = AsJSON(JsonRpcError(Value(), -32700, "Parse error"), false);
        return false;
    }

    if(TaskTrackTunnelIsRemoteSession()) {
        String method = request["method"].Is<String>() ? AsString(request["method"]) : String();
        String tool = method == "tools/call" ? ToolName(request) : String();
        String activity_error;
        TaskTrackTunnelRecordReceived(method, tool, activity_error);
    }

    Value result = UnifiedHandleRequest(request, has_response);
    if(has_response)
        response = AsJSON(result, false);
    return true;
}

int UnifiedRunServer()
{
    for(;;) {
        String message, error;
        bool eof = false;
        if(!ReadMessage(message, eof, error)) {
            if(eof)
                return 0;
            WriteMessage(AsJSON(JsonRpcError(Value(), -32700, error), false));
            return 1;
        }
        String response;
        bool has_response = false;
        UnifiedProcessMessage(message, response, has_response);
        if(has_response) {
            WriteMessage(response);
            if(TaskTrackTunnelIsRemoteSession()) {
                bool response_error = false;
                try {
                    Value envelope = ParseJSON(response);
                    if(envelope.Is<ValueMap>()) {
                        response_error = !IsNull(envelope["error"]);
                        Value result = envelope["result"];
                        if(!response_error && result.Is<ValueMap>()
                           && !IsNull(result["isError"]))
                            response_error = (bool)result["isError"];
                    }
                }
                catch(CParser::Error) {
                    response_error = true;
                }
                String activity_error;
                TaskTrackTunnelRecordSent(response.GetCount(), response_error, activity_error);
            }
        }
    }
}

int UnifiedRunOneShot(const String& file)
{
    String request = LoadFile(file);
    if(IsNull(request)) {
        Cerr() << "TaskTrackMcp: unable to read " << file << "\n";
        return 1;
    }
    String response;
    bool has_response = false;
    UnifiedProcessMessage(request, response, has_response);
    if(!has_response)
        return 1;
    WriteMessage(response);
    return 0;
}

Value UnifiedModernMeta()
{
    ValueMap extensions;
    extensions.Add("io.modelcontextprotocol/tasks", ValueMap());
    ValueMap capabilities;
    capabilities.Add("extensions", extensions);
    capabilities.Add("sampling", ValueMap());
    ValueMap meta;
    meta.Add("io.modelcontextprotocol/protocolVersion", UNIFIED_PROTOCOL);
    meta.Add("io.modelcontextprotocol/clientCapabilities", capabilities);
    return Value(meta);
}

Value UnifiedRequest(const String& method, int id, const Value& params)
{
    ValueMap request;
    request.Add("jsonrpc", "2.0");
    request.Add("id", id);
    request.Add("method", method);
    request.Add("params", params);
    return Value(request);
}

int UnifiedRunSelfTest()
{
    if(TaskTrackHumanRunSelfTest() != 0)
        return 1;

    Vector<String> failures;
    int dashboard_checks = TaskTrackDashboardBridgeSelfTest(failures);
    if(dashboard_checks <= 0)
        failures.Add("dashboard MCP bridge self-test did not execute");

    bool has_response = false;
    ValueMap list_params;
    list_params.Add("_meta", UnifiedModernMeta());
    String list_json = AsJSON(UnifiedHandleRequest(
        UnifiedRequest("tools/list", 201, list_params), has_response), false);
    if(!has_response || list_json.Find("create_task") < 0 ||
       list_json.Find("upsert_dashboard") < 0 || list_json.Find("open_dashboard") < 0 ||
       list_json.Find("tunnel_probe") < 0)
        failures.Add("unified tools/list does not expose human, dashboard and tunnel-probe tools");
    if(list_json.Find("validate a candidate semantic dashboard") < 0 ||
       list_json.Find("exact base_revision") < 0 ||
       list_json.Find("human needs to inspect") < 0 ||
       list_json.Find("accepted dashboard history") < 0)
        failures.Add("dashboard tool descriptions do not expose the operational workflow");
    if(list_json.Find("verifying that browser or remote tunnel access") < 0 ||
       list_json.Find("never modifies tasks, dashboards, files, or repository state") < 0)
        failures.Add("tunnel_probe description does not expose its read-only diagnostic boundary");

    ValueMap probe_params;
    probe_params.Add("name", "tunnel_probe");
    probe_params.Add("arguments", ValueMap());
    probe_params.Add("_meta", UnifiedModernMeta());
    String probe_json = AsJSON(UnifiedHandleRequest(
        UnifiedRequest("tools/call", 202, probe_params), has_response), false);
    if(!has_response || probe_json.Find("read_only") < 0 ||
       probe_json.Find(TaskTrackBuildVersion()) < 0)
        failures.Add("tunnel_probe tool is missing read-only/build identity");

    ValueMap call_params;
    call_params.Add("name", "version");
    call_params.Add("arguments", ValueMap());
    call_params.Add("_meta", UnifiedModernMeta());
    String version_json = AsJSON(UnifiedHandleRequest(
        UnifiedRequest("tools/call", 203, call_params), has_response), false);
    if(!has_response || version_json.Find(TaskTrackBuildVersion()) < 0 ||
       version_json.Find("dashboard_schema_version") < 0 ||
       version_json.Find(Format(":%d", TASKTRACK_DASHBOARD_SCHEMA_VERSION)) < 0)
        failures.Add("unified version tool is missing build/dashboard identity");

    if(failures.IsEmpty()) {
        Cout() << "tasktrack-unified-mcp-selftest: ok (dashboard "
               << dashboard_checks << " checks)\n";
        return 0;
    }

    Cout() << "tasktrack-unified-mcp-selftest: failed\n";
    for(const String& failure : failures)
        Cout() << " - " << failure << "\n";
    return 1;
}

String UnifiedMcpHelpText()
{
    return
        "TaskTrack MCP\n"
        "One stdio MCP service for durable human decisions and semantic project dashboards.\n"
        "\n"
        "Usage:\n"
        "  TaskTrackMcp.exe\n"
        "      Run the stdio MCP server.\n"
        "  TaskTrackMcp.exe --help\n"
        "      Show this help.\n"
        "  TaskTrackMcp.exe --version\n"
        "      Show unified build, task schema and dashboard schema identity.\n"
        "  TaskTrackMcp.exe --selftest\n"
        "      Run human-decision and dashboard MCP self-tests.\n"
        "  TaskTrackMcp.exe --oneshot <request.json>\n"
        "      Process one MCP JSON-RPC request file and exit.\n"
        "\n"
        "Runtime:\n"
        "  TaskTrackGui.exe and TaskTrackDashboardGui.exe must be beside TaskTrackMcp.exe.\n"
        "  TaskTrackTunnelGui.exe is an optional supervisor for the official Secure MCP Tunnel client.\n"
        "  open_task/create_task launch TaskTrackGui.exe.\n"
        "  open_dashboard launches TaskTrackDashboardGui.exe.\n"
        "\n"
        "Authority:\n"
        "  Human items[].answer.data is durable human evidence.\n"
        "  Dashboard state is agent-authored project state and never substitutes for human evidence.\n"
        "\n"
        "Transport:\n"
        "  stdio MCP; register TaskTrackMcp.exe once with Codex/OpenCode.\n"
        "  Secure MCP Tunnel may forward the same stdio server through the official tunnel-client.";
}

} // namespace
