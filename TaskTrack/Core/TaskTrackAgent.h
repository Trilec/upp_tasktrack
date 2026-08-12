#ifndef _TaskTrack_Core_TaskTrackAgent_h_
#define _TaskTrack_Core_TaskTrackAgent_h_

/*
    TaskTrack Agent Channel
    =======================

    Durable human->agent request sidecar for an open TaskTrack task.

    The main task JSON remains authoritative human evidence. Agent-assistance
    traffic lives in a separate .agent.json sidecar so an MCP response cannot
    race with GUI autosave or silently become a human answer.

    Wire actions are intentionally terse and imperative:
      propose_answer              -> recommended required
      clarify + mode=simplify     -> clarification required, recommended optional

    Copyright (c) 2026 Curtis Edwards
    Licensed under the Apache License, Version 2.0. See LICENSE.
*/

#include "TaskTrackCore.h"

namespace Upp {

struct TaskTrackAgentRequest : Moveable<TaskTrackAgentRequest> {
    String id;
    String item_id;
    String action;
    String mode;
    String status = "pending";
    String recommended;
    String clarification;
    String created_at;
    String resolved_at;
};

struct TaskTrackAgentChannel : Moveable<TaskTrackAgentChannel> {
    int version = 1;
    String task_id;
    String updated_at;
    Vector<TaskTrackAgentRequest> requests;
};

inline String TaskTrackAgentChannelPath(const String& task_path)
{
    return NormalizePath(task_path) + ".agent.json";
}

inline String TaskTrackMakeAgentRequestId()
{
    return Format("req-%016llx", (int64)Random64());
}

inline Value TaskTrackAgentRequestValue(const TaskTrackAgentRequest& request)
{
    ValueMap m;
    m.Add("id", request.id);
    m.Add("item_id", request.item_id);
    m.Add("action", request.action);
    if(!request.mode.IsEmpty())
        m.Add("mode", request.mode);
    m.Add("status", request.status);
    if(!request.recommended.IsEmpty())
        m.Add("recommended", request.recommended);
    if(!request.clarification.IsEmpty())
        m.Add("clarification", request.clarification);
    m.Add("created_at", request.created_at);
    if(!request.resolved_at.IsEmpty())
        m.Add("resolved_at", request.resolved_at);
    return Value(m);
}

inline Value TaskTrackAgentChannelValue(const TaskTrackAgentChannel& channel)
{
    ValueMap root;
    root.Add("version", channel.version);
    root.Add("task_id", channel.task_id);
    root.Add("updated_at", channel.updated_at);
    ValueArray requests;
    for(const TaskTrackAgentRequest& request : channel.requests)
        requests.Add(TaskTrackAgentRequestValue(request));
    root.Add("requests", requests);
    return Value(root);
}

inline bool TaskTrackParseAgentRequest(const Value& raw, TaskTrackAgentRequest& request,
                                       String& error)
{
    if(!raw.Is<ValueMap>()) {
        error = "agent request must be an object.";
        return false;
    }
    const char *fields[] = {
        "id", "item_id", "action", "mode", "status", "recommended",
        "clarification", "created_at", "resolved_at"
    };
    for(const char *field : fields) {
        Value v = raw[field];
        if(!IsNull(v) && !v.Is<String>()) {
            error = String("agent request ") + field + " must be a string.";
            return false;
        }
    }

    request.id = TrimBoth(AsString(raw["id"]));
    request.item_id = TrimBoth(AsString(raw["item_id"]));
    request.action = TrimBoth(AsString(raw["action"]));
    request.mode = TrimBoth(AsString(raw["mode"]));
    request.status = TrimBoth(AsString(raw["status"]));
    request.recommended = TrimBoth(AsString(raw["recommended"]));
    request.clarification = TrimBoth(AsString(raw["clarification"]));
    request.created_at = TrimBoth(AsString(raw["created_at"]));
    request.resolved_at = TrimBoth(AsString(raw["resolved_at"]));

    if(request.id.IsEmpty() || request.item_id.IsEmpty()) {
        error = "agent request requires id and item_id.";
        return false;
    }
    if(request.action != "propose_answer" && request.action != "clarify") {
        error = "agent request action must be propose_answer or clarify.";
        return false;
    }
    if(request.action == "clarify" && request.mode.IsEmpty())
        request.mode = "simplify";
    if(request.status.IsEmpty())
        request.status = "pending";
    if(request.status != "pending" && request.status != "resolved" && request.status != "cancelled") {
        error = "agent request status is invalid.";
        return false;
    }
    return true;
}

inline bool TaskTrackLoadAgentChannel(const String& task_path, const String& task_id,
                                      TaskTrackAgentChannel& channel, String& error)
{
    error.Clear();
    channel = TaskTrackAgentChannel();
    channel.task_id = task_id;

    String path = TaskTrackAgentChannelPath(task_path);
    if(!FileExists(path))
        return true;

    String text = LoadFile(path);
    if(IsNull(text)) {
        error = "TaskTrack agent channel is unreadable.";
        return false;
    }

    Value raw;
    try {
        raw = ParseJSON(text);
    }
    catch(CParser::Error) {
        error = "Invalid TaskTrack agent-channel JSON.";
        return false;
    }
    if(!raw.Is<ValueMap>()) {
        error = "TaskTrack agent channel must be an object.";
        return false;
    }

    Value version = raw["version"];
    if(!IsNull(version) && !IsNumber(version)) {
        error = "TaskTrack agent channel version must be numeric.";
        return false;
    }
    channel.version = IsNull(version) ? 1 : (int)version;
    if(channel.version != 1) {
        error = "Unsupported TaskTrack agent channel version.";
        return false;
    }

    Value raw_task = raw["task_id"];
    if(!IsNull(raw_task) && !raw_task.Is<String>()) {
        error = "TaskTrack agent channel task_id must be a string.";
        return false;
    }
    channel.task_id = IsNull(raw_task) ? task_id : AsString(raw_task);
    if(channel.task_id != task_id) {
        error = "TaskTrack agent channel belongs to another task.";
        return false;
    }

    Value updated = raw["updated_at"];
    if(!IsNull(updated) && !updated.Is<String>()) {
        error = "TaskTrack agent channel updated_at must be a string.";
        return false;
    }
    channel.updated_at = IsNull(updated) ? String() : AsString(updated);

    Value requests = raw["requests"];
    if(IsNull(requests))
        return true;
    if(!requests.Is<ValueArray>()) {
        error = "TaskTrack agent channel requests must be an array.";
        return false;
    }

    Index<String> ids;
    ValueArray array = requests;
    for(int i = 0; i < array.GetCount(); ++i) {
        TaskTrackAgentRequest request;
        if(!TaskTrackParseAgentRequest(array[i], request, error))
            return false;
        if(ids.Find(request.id) >= 0) {
            error = "Duplicate TaskTrack agent request id: " + request.id;
            return false;
        }
        ids.Add(request.id);
        channel.requests.Add(pick(request));
    }
    return true;
}

inline bool TaskTrackSaveAgentChannel(const String& task_path, TaskTrackAgentChannel& channel,
                                      String& error)
{
    error.Clear();
    channel.updated_at = TaskTrackNowIso();
    String path = TaskTrackAgentChannelPath(task_path);
    String folder = GetFileFolder(path);
    if(!RealizeDirectory(folder)) {
        error = "Unable to create TaskTrack agent-channel folder.";
        return false;
    }

    String text = AsJSON(TaskTrackAgentChannelValue(channel), true);
    String temp = path + ".tmp";
    if(!SaveFile(temp, text)) {
        error = "Unable to write TaskTrack agent-channel temporary file.";
        return false;
    }
    String verify = LoadFile(temp);
    if(IsNull(verify) || verify != text) {
        FileDelete(temp);
        error = "TaskTrack agent-channel temporary-file verification failed.";
        return false;
    }

    String backup = path + ".bak";
    if(FileExists(path)) {
        FileDelete(backup);
        FileCopy(path, backup);
        if(!FileDelete(path)) {
            FileDelete(temp);
            error = "Unable to replace TaskTrack agent-channel file.";
            return false;
        }
    }
    if(!FileMove(temp, path)) {
        if(FileExists(backup))
            FileCopy(backup, path);
        FileDelete(temp);
        error = "Unable to install TaskTrack agent-channel file.";
        return false;
    }
    return true;
}

inline bool TaskTrackQueueAgentRequest(const String& task_path, const String& task_id,
                                       const String& item_id, const String& action,
                                       const String& mode, String& request_id, String& error)
{
    error.Clear();
    request_id.Clear();
    if(item_id.IsEmpty()) {
        error = "agent request requires item_id.";
        return false;
    }
    if(action != "propose_answer" && action != "clarify") {
        error = "agent request action must be propose_answer or clarify.";
        return false;
    }

    TaskTrackAgentChannel channel;
    if(!TaskTrackLoadAgentChannel(task_path, task_id, channel, error))
        return false;

    for(const TaskTrackAgentRequest& request : channel.requests) {
        if(request.item_id == item_id && request.action == action && request.status == "pending") {
            request_id = request.id;
            return true;
        }
    }

    TaskTrackAgentRequest request;
    request.id = TaskTrackMakeAgentRequestId();
    request.item_id = item_id;
    request.action = action;
    request.mode = action == "clarify" ? (mode.IsEmpty() ? String("simplify") : mode) : String();
    request.status = "pending";
    request.created_at = TaskTrackNowIso();
    request_id = request.id;
    channel.requests.Add(pick(request));
    return TaskTrackSaveAgentChannel(task_path, channel, error);
}

inline bool TaskTrackResolveAgentRequest(const String& task_path, const String& task_id,
                                         const String& request_id, const String& recommended,
                                         const String& clarification, String& error)
{
    error.Clear();
    TaskTrackAgentChannel channel;
    if(!TaskTrackLoadAgentChannel(task_path, task_id, channel, error))
        return false;

    for(TaskTrackAgentRequest& request : channel.requests) {
        if(request.id != request_id)
            continue;
        if(request.status != "pending") {
            error = "agent request is not pending.";
            return false;
        }
        if(request.action == "propose_answer" && TrimBoth(recommended).IsEmpty()) {
            error = "propose_answer requires recommended.";
            return false;
        }
        if(request.action == "clarify" && TrimBoth(clarification).IsEmpty()) {
            error = "clarify requires clarification.";
            return false;
        }
        request.recommended = TrimBoth(recommended);
        request.clarification = TrimBoth(clarification);
        request.status = "resolved";
        request.resolved_at = TaskTrackNowIso();
        return TaskTrackSaveAgentChannel(task_path, channel, error);
    }

    error = "agent request not found: " + request_id;
    return false;
}

inline bool TaskTrackAgentHasPending(const TaskTrackAgentChannel& channel,
                                     const String& item_id, const String& action)
{
    for(const TaskTrackAgentRequest& request : channel.requests)
        if(request.item_id == item_id && request.action == action && request.status == "pending")
            return true;
    return false;
}

inline String TaskTrackAgentLatestClarification(const TaskTrackAgentChannel& channel,
                                                const String& item_id)
{
    for(int i = channel.requests.GetCount() - 1; i >= 0; --i) {
        const TaskTrackAgentRequest& request = channel.requests[i];
        if(request.item_id == item_id && request.status == "resolved" &&
           request.action == "clarify" && !request.clarification.IsEmpty())
            return request.clarification;
    }
    return String();
}

inline String TaskTrackAgentLatestRecommendation(const TaskTrackAgentChannel& channel,
                                                 const String& item_id)
{
    for(int i = channel.requests.GetCount() - 1; i >= 0; --i) {
        const TaskTrackAgentRequest& request = channel.requests[i];
        if(request.item_id == item_id && request.status == "resolved" &&
           !request.recommended.IsEmpty())
            return request.recommended;
    }
    return String();
}

inline ValueArray TaskTrackPendingAgentRequestsValue(const TaskTrackAgentChannel& channel)
{
    ValueArray out;
    for(const TaskTrackAgentRequest& request : channel.requests) {
        if(request.status != "pending")
            continue;
        ValueMap m;
        m.Add("id", request.id);
        m.Add("item_id", request.item_id);
        m.Add("action", request.action);
        if(!request.mode.IsEmpty())
            m.Add("mode", request.mode);
        out.Add(m);
    }
    return out;
}

inline int TaskTrackPendingAgentRequestCount(const TaskTrackAgentChannel& channel)
{
    int count = 0;
    for(const TaskTrackAgentRequest& request : channel.requests)
        if(request.status == "pending")
            ++count;
    return count;
}

} // namespace Upp

#endif
