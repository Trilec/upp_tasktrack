#include "TaskTrackTunnelCore.h"

namespace Upp {

String TaskTrackTunnelProbePath()
{
    return GetExeDirFile("tasktrack-tunnel-probe.json");
}

String TaskTrackTunnelActivityPath()
{
    return GetExeDirFile("tasktrack-tunnel-activity.json");
}

static ValueMap ProbeToValue(const TaskTrackTunnelProbe& probe)
{
    ValueMap out;
    out.Add("schema_version", probe.schema_version);
    out.Add("sequence", probe.sequence);
    out.Add("message", probe.message);
    out.Add("updated_at", probe.updated_at);
    out.Add("source", probe.source);
    out.Add("tunnel_id", probe.tunnel_id);
    return out;
}

static bool ProbeFromValue(const Value& value, TaskTrackTunnelProbe& probe, String& error)
{
    if(!value.Is<ValueMap>()) {
        error = "Tunnel probe state must be a JSON object.";
        return false;
    }

    Value schema = value["schema_version"];
    Value sequence = value["sequence"];
    if(!IsNull(schema) && (!IsNumber(schema) || (int)schema != 1)) {
        error = "Unsupported tunnel probe schema version.";
        return false;
    }
    if(!IsNull(sequence) && !IsNumber(sequence)) {
        error = "Tunnel probe sequence must be numeric.";
        return false;
    }

    probe.schema_version = IsNull(schema) ? 1 : (int)schema;
    probe.sequence = IsNull(sequence) ? 0 : (int)sequence;
    probe.message = IsNull(value["message"]) ? String() : AsString(value["message"]);
    probe.updated_at = IsNull(value["updated_at"]) ? String() : AsString(value["updated_at"]);
    probe.source = IsNull(value["source"]) ? String() : AsString(value["source"]);
    probe.tunnel_id = IsNull(value["tunnel_id"]) ? String() : AsString(value["tunnel_id"]);
    return true;
}

bool TaskTrackTunnelLoadProbe(TaskTrackTunnelProbe& probe, String& error)
{
    error.Clear();
    String path = TaskTrackTunnelProbePath();
    if(!FileExists(path)) {
        error = "No local tunnel probe has been sent yet.";
        return false;
    }

    String json = LoadFile(path);
    if(IsNull(json)) {
        error = "Unable to read local tunnel probe state.";
        return false;
    }

    try {
        return ProbeFromValue(ParseJSON(json), probe, error);
    }
    catch(CParser::Error) {
        error = "Local tunnel probe state is malformed JSON.";
        return false;
    }
}

bool TaskTrackTunnelSaveProbe(const TaskTrackTunnelProbe& probe, String& error)
{
    error.Clear();
    String json = AsJSON(ProbeToValue(probe), true);
    if(!SaveFile(TaskTrackTunnelProbePath(), json)) {
        error = "Unable to write local tunnel probe state beside the TaskTrack executables.";
        return false;
    }
    return true;
}

static ValueMap ActivityToValue(const TaskTrackTunnelActivity& activity)
{
    ValueMap out;
    out.Add("schema_version", activity.schema_version);
    out.Add("received", activity.received);
    out.Add("sent", activity.sent);
    out.Add("last_method", activity.last_method);
    out.Add("last_tool", activity.last_tool);
    out.Add("updated_at", activity.updated_at);
    return out;
}

static bool ActivityFromValue(const Value& value, TaskTrackTunnelActivity& activity, String& error)
{
    if(!value.Is<ValueMap>()) {
        error = "Tunnel activity state must be a JSON object.";
        return false;
    }

    Value schema = value["schema_version"];
    Value received = value["received"];
    Value sent = value["sent"];
    if(!IsNull(schema) && (!IsNumber(schema) || (int)schema != 1)) {
        error = "Unsupported tunnel activity schema version.";
        return false;
    }
    if(!IsNull(received) && !IsNumber(received)) {
        error = "Tunnel received count must be numeric.";
        return false;
    }
    if(!IsNull(sent) && !IsNumber(sent)) {
        error = "Tunnel sent count must be numeric.";
        return false;
    }

    activity.schema_version = IsNull(schema) ? 1 : (int)schema;
    activity.received = IsNull(received) ? 0 : (int64)received;
    activity.sent = IsNull(sent) ? 0 : (int64)sent;
    activity.last_method = IsNull(value["last_method"]) ? String() : AsString(value["last_method"]);
    activity.last_tool = IsNull(value["last_tool"]) ? String() : AsString(value["last_tool"]);
    activity.updated_at = IsNull(value["updated_at"]) ? String() : AsString(value["updated_at"]);
    return true;
}

bool TaskTrackTunnelLoadActivity(TaskTrackTunnelActivity& activity, String& error)
{
    error.Clear();
    String path = TaskTrackTunnelActivityPath();
    if(!FileExists(path)) {
        error = "No remote tunnel activity has been recorded yet.";
        return false;
    }

    String json = LoadFile(path);
    if(IsNull(json)) {
        error = "Unable to read remote tunnel activity state.";
        return false;
    }

    try {
        return ActivityFromValue(ParseJSON(json), activity, error);
    }
    catch(CParser::Error) {
        error = "Remote tunnel activity state is malformed JSON.";
        return false;
    }
}

bool TaskTrackTunnelSaveActivity(const TaskTrackTunnelActivity& activity, String& error)
{
    error.Clear();
    String json = AsJSON(ActivityToValue(activity), true);
    if(!SaveFile(TaskTrackTunnelActivityPath(), json)) {
        error = "Unable to write remote tunnel activity state beside the TaskTrack executables.";
        return false;
    }
    return true;
}

bool TaskTrackTunnelResetActivity(String& error)
{
    return TaskTrackTunnelSaveActivity(TaskTrackTunnelActivity(), error);
}

bool TaskTrackTunnelRecordReceived(const String& method, const String& tool, String& error)
{
    TaskTrackTunnelActivity activity;
    if(!TaskTrackTunnelLoadActivity(activity, error))
        activity = TaskTrackTunnelActivity();
    activity.received++;
    activity.last_method = method;
    activity.last_tool = tool;
    activity.updated_at = AsString(GetSysTime());
    return TaskTrackTunnelSaveActivity(activity, error);
}

bool TaskTrackTunnelRecordSent(String& error)
{
    TaskTrackTunnelActivity activity;
    if(!TaskTrackTunnelLoadActivity(activity, error))
        activity = TaskTrackTunnelActivity();
    activity.sent++;
    activity.updated_at = AsString(GetSysTime());
    return TaskTrackTunnelSaveActivity(activity, error);
}

bool TaskTrackTunnelIsRemoteSession()
{
    return GetEnv("TASKTRACK_TUNNEL_REMOTE") == "1";
}

ValueMap TaskTrackTunnelProbeStatusValue()
{
    ValueMap out;
    TaskTrackTunnelProbe probe;
    String error;
    bool present = TaskTrackTunnelLoadProbe(probe, error);

    out.Add("ok", true);
    out.Add("read_only", true);
    out.Add("probe_present", present);
    if(present) {
        out.Add("probe_sequence", probe.sequence);
        out.Add("probe_message", probe.message);
        out.Add("probe_updated_at", probe.updated_at);
        out.Add("probe_source", probe.source);
        out.Add("tunnel_id", probe.tunnel_id);
    }
    else {
        out.Add("probe_sequence", 0);
        out.Add("probe_message", "No local probe has been sent yet.");
    }
    return out;
}

}
