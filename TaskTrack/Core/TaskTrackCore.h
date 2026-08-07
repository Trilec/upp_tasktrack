#ifndef _TaskTrack_Core_TaskTrackCore_h_
#define _TaskTrack_Core_TaskTrackCore_h_

/*
    TaskTrack Core
    ==============

    Durable human-in-the-loop verification model shared by the TaskTrack GUI,
    MCP bridge, examples, and tests.

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

enum class TaskTrackItemType : byte {
    Check = 0,
    PassFail,
    Choice,
    Text,
    Multiline,
    Number,
    Color,
    File,
    Interaction,
    VisualCompare,
};

struct TaskTrackAnswer : Moveable<TaskTrackAnswer> {
    bool   answered = false;
    String status;
    String value;
    String note;
    String answered_at;
};

struct TaskTrackItem : Moveable<TaskTrackItem> {
    String id;
    String category = "General";
    TaskTrackItemType type = TaskTrackItemType::Check;
    String title;
    String instruction;
    bool required = true;
    Vector<String> choices;
    String expected_color;
    String expected_value;
    TaskTrackAnswer answer;
};

struct TaskTrackDocument : Moveable<TaskTrackDocument> {
    int schema_version = 1;
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
