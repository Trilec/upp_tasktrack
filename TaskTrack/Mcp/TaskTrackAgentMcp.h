#ifndef _TaskTrack_Mcp_TaskTrackAgentMcp_h_
#define _TaskTrack_Mcp_TaskTrackAgentMcp_h_

/*
    TaskTrack MCP agent-assistance bridge
    =====================================

    Compact wire contract:
      propose_answer            -> recommended required
      clarify                   -> clarification required; recommended optional
      continue_with_judgement   -> no response payload required

    Request lifecycle: pending -> answered -> (cancelled).
    Assistance lifecycle: awaiting_human -> awaiting_agent -> awaiting_human.
    Agent responses never write TaskTrackAnswer; accepted suggestions and
    explicit human answers remain separate from agent assistance.

    Copyright (c) 2026 Curtis Edwards
    Licensed under the Apache License, Version 2.0. See LICENSE.
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

inline bool TaskTrackMcpHasAnsweredDelegation(const TaskTrackAgentChannel& channel,
                                              const String& item_id)
{
    for(int i = channel.requests.GetCount() - 1; i >= 0; --i) {
        const TaskTrackAgentRequest& request = channel.requests[i];
        if(request.item_id == item_id && request.action == "continue_with_judgement" &&
           request.status == "answered")
            return true;
    }
    return false;
}

inline ValueArray TaskTrackMcpResolvedDelegatedItemIds(const TaskTrackDocument& doc,
                                                       const TaskTrackAgentChannel& channel)
{
    ValueArray out;
    for(const TaskTrackItem& item : doc.items) {
        if(!item.required || item.answer.answered)
            continue;
        if(!TaskTrackMcpHasAnsweredDelegation(channel, item.id)) {
            out.Clear();
            return out;
        }
        out.Add(item.id);
    }
    return out;
}

inline Value TaskTrackMcpAugmentAgentStatus(const Value& status, const String& task_path,
                                            const String& task_id)
{
    ValueMap out = status;
    out.Add("build_version", TaskTrackBuildVersion());
    String task_state = TrimBoth(AsString(out["state"]));
    bool terminal = task_state == "completed" || task_state == "closed";
    if(terminal) {
        bool delegated_to_agent = false;
        ValueArray delegated_items;
        if(task_state == "closed") {
            TaskTrackDocument terminal_doc;
            TaskTrackAgentChannel terminal_channel;
            String terminal_error;
            if(TaskTrackLoad(task_path, terminal_doc, terminal_error) &&
               TaskTrackLoadAgentChannel(task_path, task_id, terminal_channel, terminal_error)) {
                delegated_items = TaskTrackMcpResolvedDelegatedItemIds(terminal_doc, terminal_channel);
                delegated_to_agent = !delegated_items.IsEmpty();
            }
        }

        out.Add("interaction_state", task_state);
        out.Add("task_terminal", true);
        out.Add("agent_action_required", false);
        out.Add("agent_must_continue", true);
        out.Add("agent_response_required", true);
        out.Add("human_followup_required", false);
        out.Add("pending_requests", ValueArray());
        if(task_state == "closed") {
            out.Add("delegated_to_agent", delegated_to_agent);
            out.Add("closure_reason", delegated_to_agent ? "agent_judgement" : "human_cancelled");
            if(delegated_to_agent)
                out.Add("delegated_item_ids", delegated_items);
        }
        out.Add("next_action", task_state == "completed"
                ? Value("Continue the originating workflow now and send a concise user-visible acknowledgement that TaskTrack completed, summarizing the returned human evidence. Do not end the turn with only the tool result and do not ask the human to wake you.")
                : delegated_to_agent
                    ? Value("The human delegated the listed item(s) to agent judgement. Continue the originating workflow now using your own judgement for those items. Do not fabricate human answer.data. Send a concise user-visible acknowledgement of the delegated decision and do not ask the human to close TaskTrack or wake you.")
                    : Value("Continue the originating workflow now and send a concise user-visible acknowledgement that TaskTrack was closed/cancelled. Use any already-recorded human evidence, but do not invent missing answers. Do not end the turn with only the tool result and do not ask the human to wake you."));
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

    bool delegation_request = request->action == "continue_with_judgement";

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
    if(delegation_request)
        result.Add("delegation_acknowledged", true);
    result.Add("next_action", terminal
               ? Value("TaskTrack task is already terminal. The late advisory response was settled only; continue from the terminal human result and do not request more human input.")
               : delegation_request && pending_count == 0
                   ? Value("Delegation acknowledged. TaskTrack GUI will close automatically when no other required human input remains; call get_task(task_id, include_items=true, wait_ms=300000) and continue under your own judgement when delegated_to_agent=true is returned.")
                   : Value("If pending_count>0, resolve remaining requests immediately. Otherwise the interaction is awaiting_human: call get_task(task_id, include_items=true, wait_ms=300000) and remain in the TaskTrack workflow until completed/closed or another assistance request appears."));
    ok = true;
    error_code.Clear();
    return Value(result);
}

} // namespace Upp

#endif
