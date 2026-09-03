#include "TaskTrackTunnelCore.h"

namespace Upp {

String TaskTrackTunnelProbePath()
{
    return GetExeDirFile("tasktrack-tunnel-probe.json");
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
