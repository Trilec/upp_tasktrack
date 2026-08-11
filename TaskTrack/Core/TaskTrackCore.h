#ifndef _TaskTrack_Core_TaskTrackCore_h_
#define _TaskTrack_Core_TaskTrackCore_h_

/*
    TaskTrack Core
    ==============

    Durable, GUI-independent human-in-the-loop task model shared by the
    TaskTrack desktop application, MCP bridge, examples, and tests.

    Copyright (c) 2026 Curtis Edwards
    Licensed under the Apache License, Version 2.0. See LICENSE.
*/

#include <Core/Core.h>

namespace Upp {

enum class TaskTrackState : byte {
    AwaitingHuman = 0,
    InProgress,
    Paused,
    Completed,
    Closed,
};

// Canonical agent-facing semantic question vocabulary.
// U++ control names deliberately do not appear in the wire contract.
enum class TaskTrackItemType : byte {
    Confirm = 0,
    SingleChoice,
    MultiChoice,
    Select,
    ListSelect,
    Text,
    Notes,
    Number,
    Amount,
    Range,
    Rating,
    Color,
    Gradient,
    Position,
    Direction,
    RankOrder,
    HierarchySelect,
    Curve,
};

struct TaskTrackGradientOption : Moveable<TaskTrackGradientOption> {
    String id;
    String label;
    String from_color;
    String to_color;
};

struct TaskTrackHierarchyNode : Moveable<TaskTrackHierarchyNode> {
    String id;
    String parent_id;
    String label;
};

struct TaskTrackAnswer : Moveable<TaskTrackAnswer> {
    bool   answered = false;
    String status;
    String value;       // Compact human-readable representation.
    Value  data;        // Canonical structured response returned to the agent.
    String note;
    String answered_at;
};

struct TaskTrackItem : Moveable<TaskTrackItem> {
    String id;
    String category = "General";
    TaskTrackItemType type = TaskTrackItemType::Confirm;
    String title;
    String instruction;
    bool required = true;

    // Discrete response metadata.
    Vector<String> choices;
    bool allow_multiple = false;
    String recommended;

    // Numeric response metadata.
    bool has_min = false;
    bool has_max = false;
    double min_value = 0.0;
    double max_value = 100.0;
    double step_value = 1.0;
    String unit;

    // Visual response metadata.
    Vector<String> colors;
    Vector<TaskTrackGradientOption> gradients;
    Vector<TaskTrackHierarchyNode> hierarchy;
    Value default_value;

    // V0.1 compatibility fields. They remain readable so previously-created
    // tasks migrate cleanly; new agents should prefer the semantic fields.
    String expected_color;
    String expected_value;

    TaskTrackAnswer answer;
};

struct TaskTrackDocument : Moveable<TaskTrackDocument> {
    int schema_version = 2;
    String task_id;
    String project;
    String title;
    String subtitle;
    String actor;
    String created_at;
    String updated_at;
    String last_human_activity_at;
    int64  last_human_activity_epoch = 0;
    TaskTrackState state = TaskTrackState::AwaitingHuman;
    int reminder_minutes = 10;
    bool remind_while_paused = false;
    bool nudge_on_agent_poll = false;
    int reminder_count = 0;
    int history_limit = 20;
    Vector<TaskTrackItem> items;
};

String TaskTrackVersion();

String TaskTrackNowIso();
String TaskTrackStateName(TaskTrackState state);
bool   TaskTrackParseState(const String& text, TaskTrackState& state);
String TaskTrackItemTypeName(TaskTrackItemType type);
bool   TaskTrackParseItemType(const String& text, TaskTrackItemType& type);

String TaskTrackMakeTaskId();
String TaskTrackDefaultStoreRoot();
String TaskTrackMakeTaskPath(const String& store_root, const String& task_id);
String TaskTrackDefaultRegistryRoot();

Value  TaskTrackToValue(const TaskTrackDocument& doc);
bool   TaskTrackFromValue(const Value& value, TaskTrackDocument& doc, String& error);
String TaskTrackToJson(const TaskTrackDocument& doc, bool pretty = true);

bool TaskTrackSave(const String& path, const TaskTrackDocument& doc, String& error);
bool TaskTrackLoad(const String& path, TaskTrackDocument& doc, String& error);

bool TaskTrackRegisterTask(const String& task_id, const String& path, String& error);
bool TaskTrackResolveTaskPath(const String& task_id, const String& store_root,
                              String& path, String& error);

bool TaskTrackCreateFromArguments(const Value& args, TaskTrackDocument& doc,
                                  String& path, String& error);

Vector<String> TaskTrackCategories(const TaskTrackDocument& doc);
int TaskTrackAnsweredCount(const TaskTrackDocument& doc);
int TaskTrackRequiredCount(const TaskTrackDocument& doc);
int TaskTrackRequiredAnsweredCount(const TaskTrackDocument& doc);
bool TaskTrackCanComplete(const TaskTrackDocument& doc, Vector<String>* missing_ids = nullptr);

String TaskTrackExportMarkdown(const TaskTrackDocument& doc);
Value  TaskTrackStatusValue(const TaskTrackDocument& doc, const String& path,
                            bool include_items = true);

bool TaskTrackList(const String& store_root, int limit, ValueArray& result, String& error);
void TaskTrackPruneHistory(const String& store_root, int history_limit);

bool TaskTrackSetState(const String& path, TaskTrackState state, String& error);

String TaskTrackPollMarkerPath(const String& task_path);
bool  TaskTrackTouchAgentPoll(const String& task_path);
int64 TaskTrackReadAgentPollEpoch(const String& task_path);

// Recommendation helpers -----------------------------------------------------
//
// `recommended` remains advisory metadata. These helpers are the one shared
// acceptance path used by the GUI: they never treat default_value as evidence
// and, unless explicitly requested, never overwrite an answer already supplied
// by the human.
inline Vector<String> TaskTrackRecommendationValues(const String& text)
{
    Vector<String> out;
    String source = TrimBoth(text);
    if(source.IsEmpty())
        return out;

    if(source.StartsWith("[")) {
        try {
            Value parsed = ParseJSON(source);
            if(parsed.Is<ValueArray>()) {
                ValueArray array = parsed;
                for(int i = 0; i < array.GetCount(); ++i) {
                    String value = TrimBoth(AsString(array[i]));
                    if(!value.IsEmpty())
                        out.Add(value);
                }
                return out;
            }
        }
        catch(CParser::Error) {
        }
    }

    Vector<String> parts = Split(source, ",", false);
    for(const String& part : parts) {
        String value = TrimBoth(part);
        if(!value.IsEmpty())
            out.Add(value);
    }
    return out;
}

inline bool TaskTrackRecommendationContains(const TaskTrackItem& item, const String& value)
{
    if(item.recommended.IsEmpty())
        return false;
    if(TrimBoth(item.recommended) == value)
        return true;
    Vector<String> values = TaskTrackRecommendationValues(item.recommended);
    for(const String& candidate : values)
        if(candidate == value)
            return true;
    return false;
}

inline bool TaskTrackRecommendationChoiceExists(const TaskTrackItem& item, const String& value)
{
    for(const String& choice : item.choices)
        if(choice == value)
            return true;
    return false;
}

inline bool TaskTrackRecommendationHierarchyExists(const TaskTrackItem& item, const String& value)
{
    for(const TaskTrackHierarchyNode& node : item.hierarchy)
        if(node.id == value)
            return true;
    return false;
}

inline bool TaskTrackRecommendationColorValid(const String& value)
{
    if(value.GetCount() != 7 && value.GetCount() != 9)
        return false;
    if(value[0] != '#')
        return false;
    for(int i = 1; i < value.GetCount(); ++i)
        if(!IsXDigit(value[i]))
            return false;
    return true;
}

inline bool TaskTrackRecommendationPositionValid(const String& value)
{
    static const char *values[] = {
        "top_left", "top", "top_right", "left", "center", "right",
        "bottom_left", "bottom", "bottom_right"
    };
    for(const char *candidate : values)
        if(value == candidate)
            return true;
    return false;
}

inline bool TaskTrackRecommendationDirectionValid(const String& value)
{
    static const char *values[] = {
        "north_west", "north", "north_east", "west", "east",
        "south_west", "south", "south_east"
    };
    for(const char *candidate : values)
        if(value == candidate)
            return true;
    return false;
}

inline bool TaskTrackParseRecommendedRange(const String& text, double& low, double& high)
{
    String source = TrimBoth(text);
    if(source.IsEmpty())
        return false;

    try {
        if(source.StartsWith("{")) {
            Value parsed = ParseJSON(source);
            if(parsed.Is<ValueMap>()) {
                ValueMap map = parsed;
                if(IsNumber(map["low"]) && IsNumber(map["high"])) {
                    low = (double)map["low"];
                    high = (double)map["high"];
                    return low <= high;
                }
            }
        }
        if(source.StartsWith("[")) {
            Value parsed = ParseJSON(source);
            if(parsed.Is<ValueArray>()) {
                ValueArray array = parsed;
                if(array.GetCount() == 2 && IsNumber(array[0]) && IsNumber(array[1])) {
                    low = (double)array[0];
                    high = (double)array[1];
                    return low <= high;
                }
            }
        }
    }
    catch(CParser::Error) {
        return false;
    }

    Vector<String> parts = Split(source, ",", false);
    if(parts.GetCount() != 2)
        return false;
    try {
        Value lo = ParseJSON(TrimBoth(parts[0]));
        Value hi = ParseJSON(TrimBoth(parts[1]));
        if(!IsNumber(lo) || !IsNumber(hi))
            return false;
        low = (double)lo;
        high = (double)hi;
        return low <= high;
    }
    catch(CParser::Error) {
        return false;
    }
}

inline String TaskTrackRecommendationSummary(const TaskTrackItem& item)
{
    String rec = TrimBoth(item.recommended);
    if(rec.IsEmpty())
        return String();

    if(item.type == TaskTrackItemType::Range) {
        double low = 0.0, high = 0.0;
        if(TaskTrackParseRecommendedRange(rec, low, high))
            return AsString(low) + "–" + AsString(high) + item.unit;
    }
    if(item.type == TaskTrackItemType::RankOrder)
        return "Suggested order";
    if(item.type == TaskTrackItemType::Curve)
        return "Suggested curve";
    if(item.type == TaskTrackItemType::MultiChoice || item.type == TaskTrackItemType::ListSelect) {
        Vector<String> values = TaskTrackRecommendationValues(rec);
        if(values.GetCount() > 1) {
            String joined = Join(values, " + ");
            return joined.GetCount() > 30 ? String("Suggested selection") : joined;
        }
    }
    if((item.type == TaskTrackItemType::Text || item.type == TaskTrackItemType::Notes) && rec.GetCount() > 24)
        return "Suggested text";
    return rec.GetCount() > 32 ? rec.Left(29) + "..." : rec;
}

inline bool TaskTrackApplyRecommendation(TaskTrackItem& item, bool overwrite_answer = false)
{
    String rec = TrimBoth(item.recommended);
    if(rec.IsEmpty() || (item.answer.answered && !overwrite_answer))
        return false;

    TaskTrackAnswer answer;
    answer.answered = true;
    answer.status = "accepted";
    answer.answered_at = TaskTrackNowIso();

    switch(item.type) {
    case TaskTrackItemType::Confirm: {
        String lower = ToLower(rec);
        bool yes = lower == "yes" || lower == "true" || lower == "1" || lower == "accept" || lower == "approved";
        bool no = lower == "no" || lower == "false" || lower == "0" || lower == "decline" || lower == "reject";
        if(item.choices.GetCount() == 2) {
            if(rec == item.choices[0]) yes = true;
            if(rec == item.choices[1]) no = true;
        }
        if(!yes && !no)
            return false;
        answer.data = Value(yes);
        answer.value = yes ? "yes" : "no";
        break;
    }
    case TaskTrackItemType::SingleChoice:
    case TaskTrackItemType::Select:
        if(!TaskTrackRecommendationChoiceExists(item, rec))
            return false;
        answer.data = Value(rec);
        answer.value = rec;
        break;
    case TaskTrackItemType::Color:
        if(!TaskTrackRecommendationColorValid(rec))
            return false;
        answer.data = Value(rec);
        answer.value = rec;
        break;
    case TaskTrackItemType::Gradient: {
        bool found = false;
        for(const TaskTrackGradientOption& gradient : item.gradients)
            if(gradient.id == rec) { found = true; break; }
        if(!found)
            return false;
        answer.data = Value(rec);
        answer.value = rec;
        break;
    }
    case TaskTrackItemType::Position:
        if(!TaskTrackRecommendationPositionValid(rec))
            return false;
        answer.data = Value(rec);
        answer.value = rec;
        break;
    case TaskTrackItemType::Direction:
        if(!TaskTrackRecommendationDirectionValid(rec))
            return false;
        answer.data = Value(rec);
        answer.value = rec;
        break;
    case TaskTrackItemType::MultiChoice:
    case TaskTrackItemType::ListSelect: {
        Vector<String> values = TaskTrackRecommendationValues(rec);
        if(values.IsEmpty() || (item.type == TaskTrackItemType::ListSelect && !item.allow_multiple && values.GetCount() != 1))
            return false;
        Index<String> seen;
        for(const String& value : values) {
            if(!TaskTrackRecommendationChoiceExists(item, value) || seen.Find(value) >= 0)
                return false;
            seen.Add(value);
        }
        ValueArray data;
        for(const String& value : values)
            data.Add(value);
        if(item.type == TaskTrackItemType::ListSelect && !item.allow_multiple) {
            answer.data = data[0];
            answer.value = AsString(data[0]);
        }
        else {
            answer.data = data;
            answer.value = AsJSON(data, false);
        }
        break;
    }
    case TaskTrackItemType::Text:
    case TaskTrackItemType::Notes:
        answer.data = Value(rec);
        answer.value = rec;
        break;
    case TaskTrackItemType::Number:
    case TaskTrackItemType::Amount:
    case TaskTrackItemType::Rating: {
        try {
            Value parsed = ParseJSON(rec);
            if(!IsNumber(parsed))
                return false;
            double value = (double)parsed;
            if((item.has_min && value < item.min_value) || (item.has_max && value > item.max_value))
                return false;
            if(item.type == TaskTrackItemType::Rating && fabs(value - floor(value + 0.5)) > 0.000001)
                return false;
            answer.data = Value(value);
            answer.value = AsString(value);
        }
        catch(CParser::Error) {
            return false;
        }
        break;
    }
    case TaskTrackItemType::Range: {
        double low = 0.0, high = 0.0;
        if(!TaskTrackParseRecommendedRange(rec, low, high))
            return false;
        if((item.has_min && low < item.min_value) || (item.has_max && high > item.max_value))
            return false;
        ValueMap data;
        data.Add("low", low);
        data.Add("high", high);
        answer.data = Value(data);
        answer.value = AsString(low) + "–" + AsString(high) + item.unit;
        break;
    }
    case TaskTrackItemType::RankOrder: {
        Vector<String> values = TaskTrackRecommendationValues(rec);
        if(values.GetCount() != item.choices.GetCount())
            return false;
        Index<String> seen;
        for(const String& value : values) {
            if(!TaskTrackRecommendationChoiceExists(item, value) || seen.Find(value) >= 0)
                return false;
            seen.Add(value);
        }
        ValueArray data;
        for(const String& value : values)
            data.Add(value);
        answer.data = data;
        answer.value = AsJSON(data, false);
        break;
    }
    case TaskTrackItemType::HierarchySelect: {
        Vector<String> values = TaskTrackRecommendationValues(rec);
        if(values.IsEmpty() || (!item.allow_multiple && values.GetCount() != 1))
            return false;
        Index<String> seen;
        for(const String& value : values) {
            if(!TaskTrackRecommendationHierarchyExists(item, value) || seen.Find(value) >= 0)
                return false;
            seen.Add(value);
        }
        if(item.allow_multiple) {
            ValueArray data;
            for(const String& value : values)
                data.Add(value);
            answer.data = data;
            answer.value = AsJSON(data, false);
        }
        else {
            answer.data = Value(values[0]);
            answer.value = values[0];
        }
        break;
    }
    case TaskTrackItemType::Curve: {
        try {
            Value parsed = ParseJSON(rec);
            if(!parsed.Is<ValueArray>())
                return false;
            ValueArray points = parsed;
            if(points.GetCount() != 4)
                return false;
            for(int i = 0; i < 4; ++i)
                if(!IsNumber(points[i]) || (double)points[i] < 0.0 || (double)points[i] > 1.0)
                    return false;
            answer.data = parsed;
            answer.value = AsJSON(parsed, false);
        }
        catch(CParser::Error) {
            return false;
        }
        break;
    }
    }

    item.answer = pick(answer);
    return true;
}

} // namespace Upp

#endif
