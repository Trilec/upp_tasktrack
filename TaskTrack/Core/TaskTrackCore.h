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
    int reminder_minutes = 60;
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

} // namespace Upp

#endif
