#include "TaskTrackCore.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>

namespace Upp {

namespace {

bool IsSafeTaskId(const String& id)
{
    if(id.GetCount() < 8 || id.GetCount() > 120 || !id.StartsWith("task-"))
        return false;
    for(int i = 0; i < id.GetCount(); ++i) {
        int c = (byte)id[i];
        if(!(IsAlNum(c) || c == '-' || c == '_'))
            return false;
    }
    return true;
}

String ReadString(const Value& map, const char *key, const String& def = String())
{
    Value v = map[key];
    return IsNull(v) ? def : AsString(v);
}

bool ReadBool(const Value& map, const char *key, bool def)
{
    Value v = map[key];
    if(IsNull(v))
        return def;
    if(v.Is<bool>())
        return (bool)v;
    return def;
}

int ReadInt(const Value& map, const char *key, int def)
{
    Value v = map[key];
    if(IsNull(v))
        return def;
    if(v.Is<int>())
        return (int)v;
    if(v.Is<int64>())
        return (int)(int64)v;
    return def;
}

int64 ReadInt64(const Value& map, const char *key, int64 def)
{
    Value v = map[key];
    if(IsNull(v))
        return def;
    if(v.Is<int64>())
        return (int64)v;
    if(v.Is<int>())
        return (int)v;
    return def;
}

bool ExpectOptionalString(const Value& map, const char *key, String& error)
{
    Value v = map[key];
    if(IsNull(v) || v.Is<String>())
        return true;
    error = String(key) + " must be a string.";
    return false;
}

bool ExpectOptionalBool(const Value& map, const char *key, String& error)
{
    Value v = map[key];
    if(IsNull(v) || v.Is<bool>())
        return true;
    error = String(key) + " must be a boolean.";
    return false;
}

bool ExpectOptionalInt(const Value& map, const char *key, String& error)
{
    Value v = map[key];
    if(IsNull(v) || v.Is<int>() || v.Is<int64>())
        return true;
    error = String(key) + " must be an integer.";
    return false;
}

Value AnswerToValue(const TaskTrackAnswer& answer)
{
    ValueMap m;
    m.Add("answered", answer.answered);
    m.Add("status", answer.status);
    m.Add("value", answer.value);
    m.Add("note", answer.note);
    m.Add("answered_at", answer.answered_at);
    return Value(m);
}

TaskTrackAnswer AnswerFromValue(const Value& value)
{
    TaskTrackAnswer answer;
    if(!value.Is<ValueMap>())
        return answer;
    answer.answered = ReadBool(value, "answered", false);
    answer.status = ReadString(value, "status");
    answer.value = ReadString(value, "value");
    answer.note = ReadString(value, "note");
    answer.answered_at = ReadString(value, "answered_at");
    return answer;
}

bool ValidateExpectedColor(const String& color)
{
    if(color.IsEmpty())
        return true;
    String s = color;
    if(s.StartsWith("#"))
        s = s.Mid(1);
    if(s.GetCount() != 6 && s.GetCount() != 8)
        return false;
    for(int i = 0; i < s.GetCount(); ++i)
        if(!isxdigit((unsigned char)s[i]))
            return false;
    return true;
}

String EnsureStoreRoot(const String& requested)
{
    String root = TrimBoth(requested);
    if(root.IsEmpty())
        root = TaskTrackDefaultStoreRoot();
    return NormalizePath(root);
}

bool AtomicSaveText(const String& path, const String& text, String& error)
{
    String folder = GetFileFolder(path);
    if(!RealizeDirectory(folder)) {
        error = "Unable to create TaskTrack storage folder: " + folder;
        return false;
    }

    String temp = path + ".tmp";
    String backup = path + ".bak";

    if(!SaveFile(temp, text)) {
        error = "Unable to write temporary TaskTrack file: " + temp;
        return false;
    }

    String verify = LoadFile(temp);
    if(IsNull(verify) || verify != text) {
        FileDelete(temp);
        error = "TaskTrack temporary-file verification failed: " + temp;
        return false;
    }

    if(FileExists(path)) {
        FileDelete(backup);
        if(!FileCopy(path, backup)) {
            FileDelete(temp);
            error = "Unable to create TaskTrack recovery backup: " + backup;
            return false;
        }
    }

    if(FileExists(path) && !FileDelete(path)) {
        FileDelete(temp);
        error = "Unable to replace TaskTrack file: " + path;
        return false;
    }

    if(!FileMove(temp, path)) {
        if(FileExists(backup))
            FileCopy(backup, path);
        FileDelete(temp);
        error = "Unable to install TaskTrack file: " + path;
        return false;
    }

    return true;
}

bool ParseTaskText(const String& text, TaskTrackDocument& doc, String& error)
{
    if(IsNull(text)) {
        error = "TaskTrack task file is unreadable.";
        return false;
    }
    Value parsed;
    try {
        parsed = ParseJSON(text);
    }
    catch(CParser::Error) {
        error = "Invalid TaskTrack JSON.";
        return false;
    }
    return TaskTrackFromValue(parsed, doc, error);
}

String MarkdownEscape(String text)
{
    text.Replace("\r", "");
    text.Replace("\n", " ");
    return text;
}

} // namespace

String TaskTrackVersion()
{
    return "0.1.0";
}

String TaskTrackNowIso()
{
    time_t now = time(nullptr);
    struct tm utc = {};
#ifdef PLATFORM_WIN32
    gmtime_s(&utc, &now);
#else
    gmtime_r(&now, &utc);
#endif
    return Format("%04d-%02d-%02dT%02d:%02d:%02dZ",
                  utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday,
                  utc.tm_hour, utc.tm_min, utc.tm_sec);
}

String TaskTrackStateName(TaskTrackState state)
{
    switch(state) {
    case TaskTrackState::AwaitingHuman: return "awaiting_human";
    case TaskTrackState::InProgress:    return "in_progress";
    case TaskTrackState::Paused:        return "paused";
    case TaskTrackState::Completed:     return "completed";
    case TaskTrackState::Closed:        return "closed";
    }
    return "awaiting_human";
}

bool TaskTrackParseState(const String& text, TaskTrackState& state)
{
    String s = ToLower(TrimBoth(text));
    if(s == "awaiting_human") { state = TaskTrackState::AwaitingHuman; return true; }
    if(s == "in_progress")    { state = TaskTrackState::InProgress; return true; }
    if(s == "paused")         { state = TaskTrackState::Paused; return true; }
    if(s == "completed")      { state = TaskTrackState::Completed; return true; }
    if(s == "closed")         { state = TaskTrackState::Closed; return true; }
    return false;
}

String TaskTrackItemTypeName(TaskTrackItemType type)
{
    switch(type) {
    case TaskTrackItemType::Check:         return "check";
    case TaskTrackItemType::PassFail:      return "pass_fail";
    case TaskTrackItemType::Choice:        return "choice";
    case TaskTrackItemType::Text:          return "text";
    case TaskTrackItemType::Multiline:     return "multiline";
    case TaskTrackItemType::Number:        return "number";
    case TaskTrackItemType::Color:         return "color";
    case TaskTrackItemType::File:          return "file";
    case TaskTrackItemType::Interaction:   return "interaction";
    case TaskTrackItemType::VisualCompare: return "visual_compare";
    }
    return "check";
}

bool TaskTrackParseItemType(const String& text, TaskTrackItemType& type)
{
    String s = ToLower(TrimBoth(text));
    if(s == "check")          { type = TaskTrackItemType::Check; return true; }
    if(s == "pass_fail")      { type = TaskTrackItemType::PassFail; return true; }
    if(s == "choice")         { type = TaskTrackItemType::Choice; return true; }
    if(s == "text")           { type = TaskTrackItemType::Text; return true; }
    if(s == "multiline")      { type = TaskTrackItemType::Multiline; return true; }
    if(s == "number")         { type = TaskTrackItemType::Number; return true; }
    if(s == "color")          { type = TaskTrackItemType::Color; return true; }
    if(s == "file")           { type = TaskTrackItemType::File; return true; }
    if(s == "interaction")    { type = TaskTrackItemType::Interaction; return true; }
    if(s == "visual_compare") { type = TaskTrackItemType::VisualCompare; return true; }
    return false;
}

String TaskTrackMakeTaskId()
{
    Time t = GetSysTime();
    uint64 a = Random64();
    uint64 b = Random64();
    return Format("task-%04d%02d%02dT%02d%02d%02d-%016llx%016llx",
                  (int)t.year, (int)t.month, (int)t.day,
                  (int)t.hour, (int)t.minute, (int)t.second,
                  (unsigned long long)a, (unsigned long long)b);
}

String TaskTrackDefaultStoreRoot()
{
    return NormalizePath(AppendFileName(GetFileFolder(GetExeFilePath()), "tasktrack_data"));
}

String TaskTrackMakeTaskPath(const String& store_root, const String& task_id)
{
    return AppendFileName(EnsureStoreRoot(store_root), task_id + ".tasktrack.json");
}

String TaskTrackDefaultRegistryRoot()
{
    return AppendFileName(TaskTrackDefaultStoreRoot(), "registry");
}

Value TaskTrackToValue(const TaskTrackDocument& doc)
{
    ValueMap root;
    root.Add("schema_version", doc.schema_version);
    root.Add("tasktrack_version", TaskTrackVersion());
    root.Add("task_id", doc.task_id);
    root.Add("project", doc.project);
    root.Add("title", doc.title);
    root.Add("subtitle", doc.subtitle);
    root.Add("actor", doc.actor);
    root.Add("created_at", doc.created_at);
    root.Add("updated_at", doc.updated_at);
    root.Add("last_human_activity_at", doc.last_human_activity_at);
    root.Add("last_human_activity_epoch", doc.last_human_activity_epoch);
    root.Add("state", TaskTrackStateName(doc.state));
    root.Add("reminder_minutes", doc.reminder_minutes);
    root.Add("remind_while_paused", doc.remind_while_paused);
    root.Add("nudge_on_agent_poll", doc.nudge_on_agent_poll);
    root.Add("reminder_count", doc.reminder_count);
    root.Add("history_limit", doc.history_limit);

    ValueArray items;
    for(const TaskTrackItem& item : doc.items) {
        ValueMap m;
        m.Add("id", item.id);
        m.Add("category", item.category);
        m.Add("type", TaskTrackItemTypeName(item.type));
        m.Add("title", item.title);
        m.Add("instruction", item.instruction);
        m.Add("required", item.required);
        ValueArray choices;
        for(const String& choice : item.choices)
            choices.Add(choice);
        m.Add("choices", choices);
        m.Add("expected_color", item.expected_color);
        m.Add("expected_value", item.expected_value);
        m.Add("answer", AnswerToValue(item.answer));
        items.Add(Value(m));
    }
    root.Add("items", items);
    return Value(root);
}

bool TaskTrackFromValue(const Value& value, TaskTrackDocument& doc, String& error)
{
    error.Clear();
    if(!value.Is<ValueMap>()) {
        error = "TaskTrack document must be a JSON object.";
        return false;
    }

    const char *document_string_fields[] = {
        "task_id", "project", "title", "subtitle", "actor",
        "created_at", "updated_at", "last_human_activity_at", "state"
    };
    for(const char *key : document_string_fields)
        if(!ExpectOptionalString(value, key, error))
            return false;
    const char *document_bool_fields[] = { "remind_while_paused", "nudge_on_agent_poll" };
    for(const char *key : document_bool_fields)
        if(!ExpectOptionalBool(value, key, error))
            return false;
    const char *document_int_fields[] = {
        "schema_version", "last_human_activity_epoch", "reminder_minutes",
        "reminder_count", "history_limit"
    };
    for(const char *key : document_int_fields)
        if(!ExpectOptionalInt(value, key, error))
            return false;

    TaskTrackDocument parsed;
    parsed.schema_version = ReadInt(value, "schema_version", 1);
    if(parsed.schema_version != 1) {
        error = Format("Unsupported TaskTrack schema version %d.", parsed.schema_version);
        return false;
    }

    parsed.task_id = ReadString(value, "task_id");
    if(!IsSafeTaskId(parsed.task_id)) {
        error = "TaskTrack task_id is missing or unsafe.";
        return false;
    }

    parsed.project = ReadString(value, "project");
    parsed.title = TrimBoth(ReadString(value, "title"));
    parsed.subtitle = ReadString(value, "subtitle");
    parsed.actor = ReadString(value, "actor");
    parsed.created_at = ReadString(value, "created_at");
    parsed.updated_at = ReadString(value, "updated_at");
    parsed.last_human_activity_at = ReadString(value, "last_human_activity_at");
    parsed.last_human_activity_epoch = ReadInt64(value, "last_human_activity_epoch", 0);
    parsed.reminder_minutes = min(max(ReadInt(value, "reminder_minutes", 60), 0), 24 * 60);
    parsed.remind_while_paused = ReadBool(value, "remind_while_paused", false);
    parsed.nudge_on_agent_poll = ReadBool(value, "nudge_on_agent_poll", false);
    parsed.reminder_count = max(0, ReadInt(value, "reminder_count", 0));
    parsed.history_limit = min(max(ReadInt(value, "history_limit", 20), 5), 200);

    if(parsed.title.IsEmpty()) {
        error = "TaskTrack title is required.";
        return false;
    }

    TaskTrackState state;
    if(!TaskTrackParseState(ReadString(value, "state", "awaiting_human"), state)) {
        error = "TaskTrack state is invalid.";
        return false;
    }
    parsed.state = state;

    Value raw_items = value["items"];
    if(!raw_items.Is<ValueArray>()) {
        error = "TaskTrack items must be an array.";
        return false;
    }

    Index<String> ids;
    const ValueArray& items = raw_items;
    for(int i = 0; i < items.GetCount(); ++i) {
        const Value& raw = items[i];
        if(!raw.Is<ValueMap>()) {
            error = Format("TaskTrack item %d must be an object.", i);
            return false;
        }

        const char *item_string_fields[] = {
            "id", "category", "type", "title", "instruction", "expected_color", "expected_value"
        };
        for(const char *key : item_string_fields) {
            String field_error;
            if(!ExpectOptionalString(raw, key, field_error)) {
                error = Format("TaskTrack item %d: %s", i, field_error);
                return false;
            }
        }
        {
            String field_error;
            if(!ExpectOptionalBool(raw, "required", field_error)) {
                error = Format("TaskTrack item %d: %s", i, field_error);
                return false;
            }
        }

        Value raw_choices = raw["choices"];
        if(!IsNull(raw_choices) && !raw_choices.Is<ValueArray>()) {
            error = Format("TaskTrack item %d choices must be an array.", i);
            return false;
        }
        Value raw_answer = raw["answer"];
        if(!IsNull(raw_answer) && !raw_answer.Is<ValueMap>()) {
            error = Format("TaskTrack item %d answer must be an object.", i);
            return false;
        }
        if(raw_answer.Is<ValueMap>()) {
            String field_error;
            if(!ExpectOptionalBool(raw_answer, "answered", field_error)) {
                error = Format("TaskTrack item %d answer: %s", i, field_error);
                return false;
            }
            const char *answer_string_fields[] = { "status", "value", "note", "answered_at" };
            for(const char *key : answer_string_fields) {
                if(!ExpectOptionalString(raw_answer, key, field_error)) {
                    error = Format("TaskTrack item %d answer: %s", i, field_error);
                    return false;
                }
            }
        }

        TaskTrackItem item;
        item.id = TrimBoth(ReadString(raw, "id"));
        if(item.id.IsEmpty())
            item.id = Format("item-%d", i + 1);
        if(ids.Find(item.id) >= 0) {
            error = "Duplicate TaskTrack item id: " + item.id;
            return false;
        }
        ids.Add(item.id);

        item.category = TrimBoth(ReadString(raw, "category", "General"));
        if(item.category.IsEmpty())
            item.category = "General";
        if(!TaskTrackParseItemType(ReadString(raw, "type", "check"), item.type)) {
            error = "Unknown TaskTrack item type for " + item.id + ".";
            return false;
        }

        item.title = TrimBoth(ReadString(raw, "title"));
        item.instruction = ReadString(raw, "instruction");
        item.required = ReadBool(raw, "required", true);
        item.expected_color = TrimBoth(ReadString(raw, "expected_color"));
        item.expected_value = ReadString(raw, "expected_value");
        if(item.title.IsEmpty()) {
            error = "TaskTrack item title is required for " + item.id + ".";
            return false;
        }
        if(!ValidateExpectedColor(item.expected_color)) {
            error = "Invalid expected_color for " + item.id + ". Use #RRGGBB or #RRGGBBAA.";
            return false;
        }

        if(raw_choices.Is<ValueArray>()) {
            const ValueArray& choices = raw_choices;
            for(int j = 0; j < choices.GetCount(); ++j) {
                if(!choices[j].Is<String>()) {
                    error = Format("TaskTrack item %d choice %d must be a string.", i, j);
                    return false;
                }
                String choice = TrimBoth(AsString(choices[j]));
                if(!choice.IsEmpty())
                    item.choices.Add(choice);
            }
        }
        if(item.type == TaskTrackItemType::Choice && item.choices.IsEmpty()) {
            error = "Choice item " + item.id + " requires at least one choice.";
            return false;
        }

        item.answer = AnswerFromValue(raw["answer"]);
        parsed.items.Add(pick(item));
    }

    if(parsed.items.IsEmpty()) {
        error = "TaskTrack requires at least one verification item.";
        return false;
    }

    doc = pick(parsed);
    return true;
}

String TaskTrackToJson(const TaskTrackDocument& doc, bool pretty)
{
    return AsJSON(TaskTrackToValue(doc), pretty);
}

bool TaskTrackSave(const String& path, const TaskTrackDocument& doc, String& error)
{
    if(path.IsEmpty()) {
        error = "TaskTrack save path is empty.";
        return false;
    }
    TaskTrackDocument verify;
    String parse_error;
    String json = TaskTrackToJson(doc, true);
    if(!ParseTaskText(json, verify, parse_error)) {
        error = "TaskTrack refused to save invalid state: " + parse_error;
        return false;
    }
    return AtomicSaveText(NormalizePath(path), json, error);
}

bool TaskTrackLoad(const String& path, TaskTrackDocument& doc, String& error)
{
    String normalized = NormalizePath(path);
    String text = LoadFile(normalized);
    if(ParseTaskText(text, doc, error))
        return true;

    String primary_error = error;
    String backup = normalized + ".bak";
    if(FileExists(backup)) {
        String backup_text = LoadFile(backup);
        String backup_error;
        if(ParseTaskText(backup_text, doc, backup_error)) {
            error = "Primary task file was invalid; loaded recovery backup.";
            return true;
        }
    }
    error = primary_error;
    return false;
}

bool TaskTrackRegisterTask(const String& task_id, const String& path, String& error)
{
    if(!IsSafeTaskId(task_id)) {
        error = "Refusing to register unsafe task id.";
        return false;
    }
    String registry_root = TaskTrackDefaultRegistryRoot();
    if(!RealizeDirectory(registry_root)) {
        error = "Unable to create TaskTrack registry folder: " + registry_root;
        return false;
    }
    String locator = AppendFileName(registry_root, task_id + ".path");
    return AtomicSaveText(locator, NormalizePath(path) + "\n", error);
}

bool TaskTrackResolveTaskPath(const String& task_id, const String& store_root,
                              String& path, String& error)
{
    error.Clear();
    if(!IsSafeTaskId(task_id)) {
        error = "Invalid TaskTrack task_id.";
        return false;
    }

    if(!TrimBoth(store_root).IsEmpty()) {
        String candidate = TaskTrackMakeTaskPath(store_root, task_id);
        if(FileExists(candidate) || FileExists(candidate + ".bak")) {
            path = candidate;
            return true;
        }
    }

    String locator = AppendFileName(TaskTrackDefaultRegistryRoot(), task_id + ".path");
    String registered = TrimBoth(LoadFile(locator));
    if(!IsNull(registered) && !registered.IsEmpty()) {
        if(FileExists(registered) || FileExists(registered + ".bak")) {
            path = NormalizePath(registered);
            return true;
        }
        FileDelete(locator);
        FileDelete(locator + ".bak");
    }

    String candidate = TaskTrackMakeTaskPath(TaskTrackDefaultStoreRoot(), task_id);
    if(FileExists(candidate) || FileExists(candidate + ".bak")) {
        path = candidate;
        return true;
    }

    error = "TaskTrack task not found: " + task_id;
    return false;
}

bool TaskTrackCreateFromArguments(const Value& args, TaskTrackDocument& doc,
                                  String& path, String& error)
{
    error.Clear();
    if(!args.Is<ValueMap>()) {
        error = "TaskTrack create_task arguments must be an object.";
        return false;
    }

    const char *string_fields[] = { "task_id", "project", "title", "subtitle", "actor", "store_root" };
    for(const char *key : string_fields)
        if(!ExpectOptionalString(args, key, error))
            return false;
    const char *bool_fields[] = { "remind_while_paused", "nudge_on_agent_poll" };
    for(const char *key : bool_fields)
        if(!ExpectOptionalBool(args, key, error))
            return false;
    const char *int_fields[] = { "reminder_minutes", "history_limit" };
    for(const char *key : int_fields)
        if(!ExpectOptionalInt(args, key, error))
            return false;

    TaskTrackDocument created;
    created.task_id = TrimBoth(ReadString(args, "task_id"));
    if(created.task_id.IsEmpty())
        created.task_id = TaskTrackMakeTaskId();
    if(!IsSafeTaskId(created.task_id)) {
        error = "task_id must begin with task- and contain only letters, digits, '-' or '_'.";
        return false;
    }

    created.project = TrimBoth(ReadString(args, "project"));
    created.title = TrimBoth(ReadString(args, "title"));
    created.subtitle = ReadString(args, "subtitle");
    created.actor = ReadString(args, "actor", "agent");
    created.reminder_minutes = min(max(ReadInt(args, "reminder_minutes", 60), 0), 24 * 60);
    created.remind_while_paused = ReadBool(args, "remind_while_paused", false);
    created.nudge_on_agent_poll = ReadBool(args, "nudge_on_agent_poll", false);
    created.history_limit = min(max(ReadInt(args, "history_limit", 20), 5), 200);
    created.state = TaskTrackState::AwaitingHuman;
    created.created_at = TaskTrackNowIso();
    created.updated_at = created.created_at;
    created.last_human_activity_at = created.created_at;
    created.last_human_activity_epoch = GetSysTime().Get();

    if(created.title.IsEmpty()) {
        error = "create_task requires a non-empty title.";
        return false;
    }

    Value raw_items = args["items"];
    if(!raw_items.Is<ValueArray>()) {
        error = "create_task requires an items array.";
        return false;
    }

    Index<String> ids;
    const ValueArray& array = raw_items;
    for(int i = 0; i < array.GetCount(); ++i) {
        const Value& raw = array[i];
        if(!raw.Is<ValueMap>()) {
            error = Format("items[%d] must be an object.", i);
            return false;
        }

        const char *item_string_fields[] = {
            "id", "category", "type", "title", "instruction", "expected_color", "expected_value"
        };
        for(const char *key : item_string_fields) {
            String field_error;
            if(!ExpectOptionalString(raw, key, field_error)) {
                error = Format("items[%d].%s", i, field_error);
                return false;
            }
        }
        {
            String field_error;
            if(!ExpectOptionalBool(raw, "required", field_error)) {
                error = Format("items[%d].%s", i, field_error);
                return false;
            }
        }

        TaskTrackItem item;
        item.id = TrimBoth(ReadString(raw, "id"));
        if(item.id.IsEmpty())
            item.id = Format("item-%d", i + 1);
        if(ids.Find(item.id) >= 0) {
            error = "Duplicate item id: " + item.id;
            return false;
        }
        ids.Add(item.id);

        item.category = TrimBoth(ReadString(raw, "category", "General"));
        if(item.category.IsEmpty())
            item.category = "General";
        item.title = TrimBoth(ReadString(raw, "title"));
        item.instruction = ReadString(raw, "instruction");
        item.required = ReadBool(raw, "required", true);
        item.expected_color = TrimBoth(ReadString(raw, "expected_color"));
        item.expected_value = ReadString(raw, "expected_value");

        if(!TaskTrackParseItemType(ReadString(raw, "type", "check"), item.type)) {
            error = Format("items[%d].type is not supported.", i);
            return false;
        }
        if(item.title.IsEmpty()) {
            error = Format("items[%d].title is required.", i);
            return false;
        }
        if(!ValidateExpectedColor(item.expected_color)) {
            error = Format("items[%d].expected_color must be #RRGGBB or #RRGGBBAA.", i);
            return false;
        }

        Value raw_choices = raw["choices"];
        if(!IsNull(raw_choices) && !raw_choices.Is<ValueArray>()) {
            error = Format("items[%d].choices must be an array.", i);
            return false;
        }
        if(raw_choices.Is<ValueArray>()) {
            const ValueArray& choices = raw_choices;
            for(int j = 0; j < choices.GetCount(); ++j) {
                if(!choices[j].Is<String>()) {
                    error = Format("items[%d].choices[%d] must be a string.", i, j);
                    return false;
                }
                String choice = TrimBoth(AsString(choices[j]));
                if(!choice.IsEmpty())
                    item.choices.Add(choice);
            }
        }
        if(item.type == TaskTrackItemType::Choice && item.choices.IsEmpty()) {
            error = Format("items[%d] is a choice item but has no choices.", i);
            return false;
        }
        created.items.Add(pick(item));
    }

    if(created.items.IsEmpty()) {
        error = "create_task requires at least one item.";
        return false;
    }

    String root = EnsureStoreRoot(ReadString(args, "store_root"));
    path = TaskTrackMakeTaskPath(root, created.task_id);
    if(FileExists(path) || FileExists(path + ".bak")) {
        error = "TaskTrack task already exists: " + created.task_id;
        return false;
    }

    if(!TaskTrackSave(path, created, error))
        return false;

    String registry_error;
    if(!TaskTrackRegisterTask(created.task_id, path, registry_error)) {
        FileDelete(path);
        error = "Task created but registry update failed; rolled back task file. " + registry_error;
        return false;
    }

    TaskTrackPruneHistory(root, created.history_limit);
    doc = pick(created);
    return true;
}

Vector<String> TaskTrackCategories(const TaskTrackDocument& doc)
{
    Index<String> seen;
    Vector<String> out;
    for(const TaskTrackItem& item : doc.items) {
        String category = item.category.IsEmpty() ? String("General") : item.category;
        if(seen.Find(category) < 0) {
            seen.Add(category);
            out.Add(category);
        }
    }
    return out;
}

int TaskTrackAnsweredCount(const TaskTrackDocument& doc)
{
    int count = 0;
    for(const TaskTrackItem& item : doc.items)
        if(item.answer.answered)
            ++count;
    return count;
}

int TaskTrackRequiredCount(const TaskTrackDocument& doc)
{
    int count = 0;
    for(const TaskTrackItem& item : doc.items)
        if(item.required)
            ++count;
    return count;
}

int TaskTrackRequiredAnsweredCount(const TaskTrackDocument& doc)
{
    int count = 0;
    for(const TaskTrackItem& item : doc.items)
        if(item.required && item.answer.answered)
            ++count;
    return count;
}

bool TaskTrackCanComplete(const TaskTrackDocument& doc, Vector<String>* missing_ids)
{
    if(missing_ids)
        missing_ids->Clear();
    bool ok = true;
    for(const TaskTrackItem& item : doc.items) {
        if(item.required && !item.answer.answered) {
            ok = false;
            if(missing_ids)
                missing_ids->Add(item.id);
        }
    }
    return ok;
}

String TaskTrackExportMarkdown(const TaskTrackDocument& doc)
{
    String out;
    out << "# " << doc.title << "\n\n";
    if(!doc.subtitle.IsEmpty())
        out << doc.subtitle << "\n\n";
    out << "- **Task:** `" << doc.task_id << "`\n";
    if(!doc.project.IsEmpty())
        out << "- **Project:** " << doc.project << "\n";
    out << "- **State:** " << TaskTrackStateName(doc.state) << "\n";
    out << "- **Updated:** " << doc.updated_at << "\n";
    out << "- **Progress:** " << TaskTrackAnsweredCount(doc) << "/" << doc.items.GetCount() << "\n\n";

    String last_category;
    for(const TaskTrackItem& item : doc.items) {
        if(item.category != last_category) {
            last_category = item.category;
            out << "## " << last_category << "\n\n";
        }
        String marker = item.answer.answered ? "x" : " ";
        out << "- [" << marker << "] **" << MarkdownEscape(item.title) << "**";
        if(item.required)
            out << " *(required)*";
        out << "\n";
        if(!item.instruction.IsEmpty())
            out << "  - Check: " << MarkdownEscape(item.instruction) << "\n";
        out << "  - Type: `" << TaskTrackItemTypeName(item.type) << "`\n";
        if(!item.expected_color.IsEmpty())
            out << "  - Expected colour: `" << item.expected_color << "`\n";
        if(!item.expected_value.IsEmpty())
            out << "  - Expected: " << MarkdownEscape(item.expected_value) << "\n";
        if(item.answer.answered) {
            if(!item.answer.status.IsEmpty())
                out << "  - Status: **" << MarkdownEscape(item.answer.status) << "**\n";
            if(!item.answer.value.IsEmpty())
                out << "  - Value: " << MarkdownEscape(item.answer.value) << "\n";
            if(!item.answer.note.IsEmpty())
                out << "  - Note: " << MarkdownEscape(item.answer.note) << "\n";
            if(!item.answer.answered_at.IsEmpty())
                out << "  - Answered: " << item.answer.answered_at << "\n";
        }
    }
    return out;
}

Value TaskTrackStatusValue(const TaskTrackDocument& doc, const String& path, bool include_items)
{
    ValueMap root;
    root.Add("ok", true);
    root.Add("task_id", doc.task_id);
    root.Add("task_path", NormalizePath(path));
    root.Add("project", doc.project);
    root.Add("title", doc.title);
    root.Add("subtitle", doc.subtitle);
    root.Add("state", TaskTrackStateName(doc.state));
    root.Add("created_at", doc.created_at);
    root.Add("updated_at", doc.updated_at);
    root.Add("reminder_minutes", doc.reminder_minutes);
    root.Add("remind_while_paused", doc.remind_while_paused);
    root.Add("nudge_on_agent_poll", doc.nudge_on_agent_poll);
    root.Add("reminder_count", doc.reminder_count);
    root.Add("answered", TaskTrackAnsweredCount(doc));
    root.Add("total", doc.items.GetCount());
    root.Add("required_answered", TaskTrackRequiredAnsweredCount(doc));
    root.Add("required_total", TaskTrackRequiredCount(doc));
    root.Add("ready_to_complete", TaskTrackCanComplete(doc));

    if(include_items) {
        ValueArray items;
        for(const TaskTrackItem& item : doc.items) {
            ValueMap m;
            m.Add("id", item.id);
            m.Add("category", item.category);
            m.Add("type", TaskTrackItemTypeName(item.type));
            m.Add("title", item.title);
            m.Add("instruction", item.instruction);
            m.Add("required", item.required);
            m.Add("expected_color", item.expected_color);
            m.Add("expected_value", item.expected_value);
            m.Add("answer", AnswerToValue(item.answer));
            items.Add(Value(m));
        }
        root.Add("items", items);
    }
    return Value(root);
}

bool TaskTrackList(const String& store_root, int limit, ValueArray& result, String& error)
{
    error.Clear();
    result.Clear();
    String root = EnsureStoreRoot(store_root);
    if(limit <= 0)
        limit = 20;
    limit = min(limit, 200);

    Vector<String> paths;
    FindFile ff(AppendFileName(root, "*.tasktrack.json"));
    while(ff) {
        if(ff.IsFile())
            paths.Add(AppendFileName(root, ff.GetName()));
        ff.Next();
    }
    Sort(paths);

    for(int i = paths.GetCount() - 1; i >= 0 && result.GetCount() < limit; --i) {
        TaskTrackDocument doc;
        String load_error;
        if(!TaskTrackLoad(paths[i], doc, load_error))
            continue;
        result.Add(TaskTrackStatusValue(doc, paths[i], false));
    }
    return true;
}

void TaskTrackPruneHistory(const String& store_root, int history_limit)
{
    if(history_limit < 5)
        history_limit = 5;
    String root = EnsureStoreRoot(store_root);
    Vector<String> paths;
    FindFile ff(AppendFileName(root, "*.tasktrack.json"));
    while(ff) {
        if(ff.IsFile())
            paths.Add(AppendFileName(root, ff.GetName()));
        ff.Next();
    }
    if(paths.GetCount() <= history_limit)
        return;

    Sort(paths);
    int excess = paths.GetCount() - history_limit;
    for(int i = 0; i < paths.GetCount() && excess > 0; ++i) {
        TaskTrackDocument doc;
        String error;
        if(!TaskTrackLoad(paths[i], doc, error))
            continue;
        if(doc.state != TaskTrackState::Completed && doc.state != TaskTrackState::Closed)
            continue;
        FileDelete(paths[i] + ".bak");
        if(FileDelete(paths[i])) {
            String locator = AppendFileName(TaskTrackDefaultRegistryRoot(), doc.task_id + ".path");
            FileDelete(locator);
            FileDelete(locator + ".bak");
            --excess;
        }
    }
}

bool TaskTrackSetState(const String& path, TaskTrackState state, String& error)
{
    TaskTrackDocument doc;
    if(!TaskTrackLoad(path, doc, error))
        return false;
    doc.state = state;
    doc.updated_at = TaskTrackNowIso();
    doc.last_human_activity_at = doc.updated_at;
    doc.last_human_activity_epoch = GetSysTime().Get();
    return TaskTrackSave(path, doc, error);
}

String TaskTrackPollMarkerPath(const String& task_path)
{
    return NormalizePath(task_path) + ".poll";
}

bool TaskTrackTouchAgentPoll(const String& task_path)
{
    String marker = TaskTrackPollMarkerPath(task_path);
    String folder = GetFileFolder(marker);
    if(!RealizeDirectory(folder))
        return false;
    return SaveFile(marker, AsString((int64)GetSysTime().Get()));
}

int64 TaskTrackReadAgentPollEpoch(const String& task_path)
{
    String text = TrimBoth(LoadFile(TaskTrackPollMarkerPath(task_path)));
    if(IsNull(text) || text.IsEmpty())
        return 0;
    const char *begin = ~text;
    char *end = nullptr;
    long long value = strtoll(begin, &end, 10);
    if(end == begin)
        return 0;
    return (int64)value;
}

} // namespace Upp
