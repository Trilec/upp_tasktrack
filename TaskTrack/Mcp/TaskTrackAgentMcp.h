#ifndef _TaskTrack_Mcp_TaskTrackAgentMcp_h_
#define _TaskTrack_Mcp_TaskTrackAgentMcp_h_

/*
    TaskTrack MCP agent-assistance bridge.

    Compact wire contract:
      propose_answer            -> recommended required
      clarify                   -> clarification required; recommended optional
      continue_with_judgement   -> no response payload required

    Request lifecycle: pending -> answered -> (cancelled).
    Assistance lifecycle: awaiting_human -> awaiting_agent -> awaiting_human.
    Agent responses remain advisory. They never write TaskTrackAnswer.
*/

#include <TaskTrack/Core/TaskTrackAgent.h>
#include <TaskTrack/Core/TaskTrackBuild.h>

namespace Upp {

inline Value TaskTrackMcpAgentStringSchema(const String& description)
{
    ValueMap s;
    s.Add("type", "string");
    s.Add("description", description);
    return Value(s);
}

inline Value TaskTrackMcpRespondRequestSchema()
{
    ValueMap props;
    props.Add("task_id", TaskTrackMcpAgentStringSchema("Task id."));
    props.Add("request_id", TaskTrackMcpAgentStringSchema("Pending request id from get_task.pending_requests."));
    props.Add("store_root", TaskTrackMcpAgentStringSchema("Optional non-default task store."));
    props.Add("recommended", TaskTrackMcpAgentStringSchema("Required for propose_answer; optional for clarify. Not used for continue_with_judgement. Canonical advisory recommendation for the referenced item; it is not human evidence until explicitly accepted by the human."));
    props.Add("clarification", TaskTrackMcpAgentStringSchema("Required for clarify. Concise plain-language restatement for the human."));

    ValueArray required;
    required.Add("task_id");
    required.Add("request_id");

    ValueMap schema;
    schema.Add("type", "object");
    schema.Add("required", required);
    schema.Add("properties", props);
    schema.Add("additionalProperties", false);
    return Value(schema);
}

inline const TaskTrackAgentRequest* TaskTrackMcpFindAgentRequest(const TaskTrackAgentChannel& channel,
                                                                const String& request_id)
{
    for(const TaskTrackAgentRequest& request : channel.requests)
        if(request.id == request_id)
            return &request;
    return nullptr;
}

inline TaskTrackItem* TaskTrackMcpFindItem(TaskTrackDocument& doc, const String& item_id)
{
    for(TaskTrackItem& item : doc.items)
        if(item.id == item_id)
            return &item;
    return nullptr;
}

inline bool TaskTrackMcpRecommendationValid(TaskTrackItem& item, const String& recommendation)
{
    String old_recommended = item.recommended;
    TaskTrackAnswer old_answer = item.answer;
    item.recommended = TrimBoth(recommendation);
    bool valid = TaskTrackApplyRecommendation(item, true);
    item.answer = old_answer;
    item.recommended = old_recommended;
    return valid;
}

inline Value TaskTrackMcpAugmentAgentStatus(const Value& status, const String& task_path,
                                            const String& task_id)
{
    ValueMap out = status;
    out.Add("build_version", TaskTrackBuildVersion());
    String task_state = TrimBoth(AsString(out["state"]));
    bool terminal = task_state == "completed" || task_state == "closed";
    if(terminal) {
        out.Add("interaction_state", task_state);
        out.Add("task_terminal", true);
        out.Add("agent_action_required", false);
        out.Add("agent_must_continue", true);
        out.Add("agent_response_required", true);
        out.Add("human_followup_required", false);
        out.Add("pending_requests", ValueArray());
        out.Add("next_action", task_state == "completed"
                ? Value("Continue the originating workflow now and send a concise user-visible acknowledgement that TaskTrack completed, summarizing the returned human evidence. Do not end the turn with only the tool result and do not ask the human to wake you.")
                : Value("Continue the originating workflow now and send a concise user-visible acknowledgement that TaskTrack was closed/cancelled with no additional human evidence. Do not end the turn with only the tool result and do not ask the human to wake you."));
        return Value(out);
    }

    TaskTrackAgentChannel channel;
    String error;
    if(!TaskTrackLoadAgentChannel(task_path, task_id, channel, error)) {
        out.Add("agent_action_required", false);
        out.Add("interaction_state", "awaiting_human");
        out.Add("task_terminal", false);
        out.Add("agent_channel_error", error);
        return Value(out);
    }

    ValueArray pending = TaskTrackPendingAgentRequestsValue(channel);
    bool action_required = !pending.IsEmpty();

    out.Add("interaction_state", TaskTrackAgentEffectiveInteractionState(channel));
    out.Add("task_terminal", false);
    out.Add("agent_action_required", action_required);
    out.Add("pending_requests", pending);
    if(action_required) {
        out.Add("agent_must_continue", true);
        out.Add("human_followup_required", false);
        out.Add("compatibility_action",
                "TaskTrack is awaiting_agent and the human task is NOT terminal. Resolve every pending_requests entry immediately with respond_to_request, then call get_task(task_id, include_items=true, wait_ms=300000). Remain in this TaskTrack workflow until completed/closed or another agent request appears. Do not ask the human to send a follow-up chat message.");
    }
    return Value(out);
}

inline int TaskTrackMcpPendingAgentRequestCount(const String& task_path, const String& task_id)
{
    TaskTrackAgentChannel channel;
    String error;
    if(!TaskTrackLoadAgentChannel(task_path, task_id, channel, error))
        return 0;
    return TaskTrackPendingAgentRequestCount(channel);
}

inline Value TaskTrackMcpRespondAgentRequest(const Value& args, bool& ok, String& error_code)
{
    ok = false;
    error_code = "BAD_REQUEST";
    if(!args.Is<ValueMap>()) {
        ValueMap out; out.Add("ok", false); out.Add("message", "arguments must be an object.");
        return Value(out);
    }

    Value raw_task = args["task_id"];
    Value raw_request = args["request_id"];
    Value raw_store = args["store_root"];
    Value raw_recommended = args["recommended"];
    Value raw_clarification = args["clarification"];
    if(IsNull(raw_task) || !raw_task.Is<String>() || IsNull(raw_request) || !raw_request.Is<String>() ||
       (!IsNull(raw_store) && !raw_store.Is<String>()) ||
       (!IsNull(raw_recommended) && !raw_recommended.Is<String>()) ||
       (!IsNull(raw_clarification) && !raw_clarification.Is<String>())) {
        ValueMap out; out.Add("ok", false); out.Add("message", "task_id/request_id must be strings; optional response fields must be strings.");
        return Value(out);
    }

    String task_id = AsString(raw_task);
    String request_id = AsString(raw_request);
    String store_root = IsNull(raw_store) ? String() : AsString(raw_store);
    String recommended = IsNull(raw_recommended) ? String() : TrimBoth(AsString(raw_recommended));
    String clarification = IsNull(raw_clarification) ? String() : TrimBoth(AsString(raw_clarification));

    String task_path, error;
    if(!TaskTrackResolveTaskPath(task_id, store_root, task_path, error)) {
        error_code = "TASK_NOT_FOUND";
        ValueMap out; out.Add("ok", false); out.Add("message", error);
        return Value(out);
    }

    TaskTrackDocument doc;
    if(!TaskTrackLoad(task_path, doc, error)) {
        error_code = "TASK_LOAD_FAILED";
        ValueMap out; out.Add("ok", false); out.Add("message", error);
        return Value(out);
    }
    bool terminal = doc.state == TaskTrackState::Completed || doc.state == TaskTrackState::Closed;

    TaskTrackAgentChannel channel;
    if(!TaskTrackLoadAgentChannel(task_path, task_id, channel, error)) {
        error_code = "AGENT_CHANNEL_FAILED";
        ValueMap out; out.Add("ok", false); out.Add("message", error);
        return Value(out);
    }

    const TaskTrackAgentRequest* request = TaskTrackMcpFindAgentRequest(channel, request_id);
    if(!request || request->status != "pending") {
        error_code = "REQUEST_NOT_PENDING";
        ValueMap out; out.Add("ok", false); out.Add("message", "request is missing or not pending.");
        return Value(out);
    }

    TaskTrackItem* item = TaskTrackMcpFindItem(doc, request->item_id);
    if(!item) {
        error_code = "ITEM_NOT_FOUND";
        ValueMap out; out.Add("ok", false); out.Add("message", "request item no longer exists.");
        return Value(out);
    }

    if(request->action == "propose_answer" && recommended.IsEmpty()) {
        ValueMap out; out.Add("ok", false); out.Add("message", "propose_answer requires recommended.");
        return Value(out);
    }
    if(request->action == "clarify" && clarification.IsEmpty()) {
        ValueMap out; out.Add("ok", false); out.Add("message", "clarify requires clarification.");
        return Value(out);
    }
    if(!recommended.IsEmpty() && !TaskTrackMcpRecommendationValid(*item, recommended)) {
        error_code = "INVALID_RECOMMENDATION";
        ValueMap out; out.Add("ok", false); out.Add("message", "recommended is invalid for the referenced item.");
        return Value(out);
    }

    // A response that was already in flight may arrive just after the human
    // independently completes/closes the task. Settle that sidecar request so
    // it cannot remain pending forever, but never alter TaskTrackAnswer or
    // resurrect agent_action_required on the terminal task.
    if(!TaskTrackResolveAgentRequest(task_path, task_id, request_id, recommended, clarification, error)) {
        error_code = "REQUEST_UPDATE_FAILED";
        ValueMap out; out.Add("ok", false); out.Add("message", error);
        return Value(out);
    }

    TaskTrackAgentChannel updated;
    TaskTrackLoadAgentChannel(task_path, task_id, updated, error);
    int pending_count = TaskTrackPendingAgentRequestCount(updated);
    ValueMap result;
    result.Add("ok", true);
    result.Add("build_version", TaskTrackBuildVersion());
    result.Add("task_id", task_id);
    result.Add("request_id", request_id);
    result.Add("status", "answered");
    result.Add("interaction_state", terminal ? TaskTrackStateName(doc.state)
                                             : TaskTrackAgentEffectiveInteractionState(updated));
    result.Add("task_terminal", terminal);
    result.Add("agent_action_required", terminal ? false : pending_count > 0);
    result.Add("pending_count", terminal ? 0 : pending_count);
    result.Add("human_followup_required", false);
    result.Add("next_action", terminal
               ? Value("TaskTrack task is already terminal. The late advisory response was settled only; continue from the terminal human result and do not request more human input.")
               : Value("If pending_count>0, resolve remaining requests immediately. Otherwise the interaction is awaiting_human: call get_task(task_id, include_items=true, wait_ms=300000) and remain in the TaskTrack workflow until completed/closed or another assistance request appears."));
    ok = true;
    error_code.Clear();
    return Value(result);
}

} // namespace Upp

#endif
