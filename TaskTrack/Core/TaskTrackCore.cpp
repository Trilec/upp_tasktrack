#include "TaskTrackCore.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

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
    return v.Is<bool>() ? (bool)v : def;
}

int ReadInt(const Value& map, const char *key, int def)
{
    Value v = map[key];
    if(IsNull(v) || !IsNumber(v))
        return def;
    return (int)v;
}

int64 ReadInt64(const Value& map, const char *key, int64 def)
{
    Value v = map[key];
    if(IsNull(v) || !IsNumber(v))
        return def;
    return (int64)v;
}

double ReadDouble(const Value& map, const char *key, double def)
{
    Value v = map[key];
    if(IsNull(v) || !IsNumber(v))
        return def;
    return (double)v;
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

bool ExpectOptionalNumber(const Value& map, const char *key, String& error)
{
    Value v = map[key];
    if(IsNull(v) || IsNumber(v))
        return true;
    error = String(key) + " must be numeric.";
    return false;
}

bool ExpectOptionalArray(const Value& map, const char *key, String& error)
{
    Value v = map[key];
    if(IsNull(v) || v.Is<ValueArray>())
        return true;
    error = String(key) + " must be an array.";
    return false;
}

bool ValidateColorText(const String& color)
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

String DataToDisplay(const Value& data)
{
    if(IsNull(data))
        return String();
    if(data.Is<ValueArray>() || data.Is<ValueMap>())
        return AsJSON(data, false);
    return AsString(data);
}

Value AnswerToValue(const TaskTrackAnswer& answer)
{
    ValueMap m;
    m.Add("answered", answer.answered);
    m.Add("status", answer.status);
    m.Add("value", answer.value);
    m.Add("data", answer.data);
    m.Add("note", answer.note);
    m.Add("answered_at", answer.answered_at);
    return Value(m);
}

bool ParseAnswer(const Value& raw, TaskTrackAnswer& answer, const String& prefix, String& error)
{
    if(IsNull(raw))
        return true;
    if(!raw.Is<ValueMap>()) {

        error = prefix + "answer must be an object.";
        return false;
    }
    String field_error;
    if(!ExpectOptionalBool(raw, "answered", field_error)) {
        error = prefix + "answer." + field_error;
        return false;
    }
    const char *strings[] = { "status", "value", "note", "answered_at" };
    for(const char *key : strings) {
        if(!ExpectOptionalString(raw, key, field_error)) {
            error = prefix + "answer." + field_error;
            return false;
        }
    }
    answer.answered = ReadBool(raw, "answered", false);
    answer.status = ReadString(raw, "status");
    answer.value = ReadString(raw, "value");
    answer.data = raw["data"];
    if(IsNull(answer.data) && answer.answered && !answer.value.IsEmpty())
        answer.data = answer.value;
    answer.note = ReadString(raw, "note");
    answer.answered_at = ReadString(raw, "answered_at");
    return true;
}

bool ParseStringArray(const Value& raw, const char *field, Vector<String>& out,
                      const String& prefix, String& error)
{
    out.Clear();
    Value value = raw[field];
    if(IsNull(value))
        return true;
    if(!value.Is<ValueArray>()) {
        error = prefix + field + " must be an array.";
        return false;
    }
    const ValueArray& array = value;
    for(int i = 0; i < array.GetCount(); ++i) {
        if(!array[i].Is<String>()) {
            error = Format("%s%s[%d] must be a string.", prefix, field, i);
            return false;
        }
        String text = TrimBoth(AsString(array[i]));
        if(!text.IsEmpty())
            out.Add(text);
    }
    return true;
}

bool ParseGradients(const Value& raw, Vector<TaskTrackGradientOption>& out,
                    const String& prefix, String& error)
{
    out.Clear();
    Value value = raw["gradients"];
    if(IsNull(value))
        return true;
    if(!value.Is<ValueArray>()) {
        error = prefix + "gradients must be an array.";
        return false;
    }
    const ValueArray& array = value;
    Index<String> ids;
    for(int i = 0; i < array.GetCount(); ++i) {
        const Value& entry = array[i];
        if(!entry.Is<ValueMap>()) {
            error = Format("%sgradients[%d] must be an object.", prefix, i);
            return false;
        }
        const char *fields[] = { "id", "label", "from", "to" };
        for(const char *key : fields) {
            String field_error;
            if(!ExpectOptionalString(entry, key, field_error)) {
                error = Format("%sgradients[%d].%s", prefix, i, field_error);
                return false;
            }
        }
        TaskTrackGradientOption option;
        option.id = TrimBoth(ReadString(entry, "id"));
        option.label = TrimBoth(ReadString(entry, "label", option.id));
        option.from_color = TrimBoth(ReadString(entry, "from"));
        option.to_color = TrimBoth(ReadString(entry, "to"));
        if(option.id.IsEmpty() || ids.Find(option.id) >= 0) {
            error = Format("%sgradients[%d] requires a unique non-empty id.", prefix, i);
            return false;
        }
        if(!ValidateColorText(option.from_color) || option.from_color.IsEmpty() ||
           !ValidateColorText(option.to_color) || option.to_color.IsEmpty()) {
            error = Format("%sgradients[%d] requires valid #RRGGBB colours.", prefix, i);
            return false;
        }
        ids.Add(option.id);
        out.Add(pick(option));
    }
    return true;
}

bool ValidateHierarchy(const Vector<TaskTrackHierarchyNode>& nodes, const String& prefix, String& error)
{
    Index<String> ids;
    for(const TaskTrackHierarchyNode& node : nodes) {
        if(node.id.IsEmpty() || ids.Find(node.id) >= 0) {
            error = prefix + "hierarchy node ids must be unique and non-empty.";
            return false;
        }
        ids.Add(node.id);
    }
    for(int i = 0; i < nodes.GetCount(); ++i) {
        const TaskTrackHierarchyNode& node = nodes[i];
        if(!node.parent_id.IsEmpty() && ids.Find(node.parent_id) < 0) {
            error = prefix + "hierarchy node " + node.id + " references missing parent " + node.parent_id + ".";
            return false;
        }
        String parent = node.parent_id;
        Index<String> chain;
        chain.Add(node.id);
        for(int guard = 0; !parent.IsEmpty() && guard <= nodes.GetCount(); ++guard) {
            if(chain.Find(parent) >= 0) {
                error = prefix + "hierarchy contains a cycle at " + node.id + ".";
                return false;
            }
            chain.Add(parent);
            int p = ids.Find(parent);
            if(p < 0)
                break;
            parent = nodes[p].parent_id;
        }
    }
    return true;
}

bool ParseHierarchy(const Value& raw, Vector<TaskTrackHierarchyNode>& out,
                    const String& prefix, String& error)
{
    out.Clear();
    Value value = raw["hierarchy"];
    if(IsNull(value))
        return true;
    if(!value.Is<ValueArray>()) {
        error = prefix + "hierarchy must be an array.";
        return false;
    }
    const ValueArray& array = value;
    for(int i = 0; i < array.GetCount(); ++i) {
        const Value& entry = array[i];
        if(!entry.Is<ValueMap>()) {
            error = Format("%shierarchy[%d] must be an object.", prefix, i);
            return false;
        }
        const char *fields[] = { "id", "parent_id", "label" };
        for(const char *key : fields) {
            String field_error;
            if(!ExpectOptionalString(entry, key, field_error)) {
                error = Format("%shierarchy[%d].%s", prefix, i, field_error);
                return false;
            }
        }
        TaskTrackHierarchyNode node;
        node.id = TrimBoth(ReadString(entry, "id"));
        node.parent_id = TrimBoth(ReadString(entry, "parent_id"));
        node.label = TrimBoth(ReadString(entry, "label", node.id));
        if(node.label.IsEmpty())
            node.label = node.id;
        out.Add(pick(node));
    }
    return ValidateHierarchy(out, prefix, error);
}


void NormalizeLegacyAnswer(const String& raw_type, TaskTrackItem& item)
{
    if(!item.answer.answered)
        return;
    String value = ToLower(TrimBoth(item.answer.value));
    String normalized;
    if(raw_type == "pass_fail") {
        if(value == "pass") normalized = "Pass";
        else if(value == "fail") normalized = "Fail";
        else if(value == "blocked") normalized = "Blocked";
        else if(value == "not_applicable") normalized = "Not applicable";
    }
    else if(raw_type == "file") {
        if(value == "found") normalized = "Found";
        else if(value == "missing") normalized = "Missing";
        else if(value == "wrong_output") normalized = "Wrong output";
        else if(value == "unsure") normalized = "Unsure";
    }
    else if(raw_type == "interaction") {
        if(value == "pass") normalized = "Pass";
        else if(value == "fail") normalized = "Fail";
        else if(value == "partial") normalized = "Partial";
        else if(value == "blocked") normalized = "Blocked";
    }
    else if(raw_type == "visual_compare" || raw_type == "legacy_color_witness") {
        if(value == "match") normalized = "Match";
        else if(value == "different") normalized = "Different";
        else if(value == "unsure") normalized = "Unsure";
    }
    if(!normalized.IsEmpty()) {
        item.answer.value = normalized;
        item.answer.data = normalized;
    }
}

void AddLegacyChoices(const String& raw_type, TaskTrackItem& item)
{
    if(!item.choices.IsEmpty())
        return;
    if(raw_type == "pass_fail") {
        item.choices.Add("Pass"); item.choices.Add("Fail"); item.choices.Add("Blocked"); item.choices.Add("Not applicable");
    }
    else if(raw_type == "file") {
        item.choices.Add("Found"); item.choices.Add("Missing"); item.choices.Add("Wrong output"); item.choices.Add("Unsure");
    }
    else if(raw_type == "interaction") {
        item.choices.Add("Pass"); item.choices.Add("Fail"); item.choices.Add("Partial"); item.choices.Add("Blocked");
    }
    else if(raw_type == "visual_compare" || raw_type == "legacy_color_witness") {
        item.choices.Add("Match"); item.choices.Add("Different"); item.choices.Add("Unsure");
    }

}


bool HasString(const Vector<String>& values, const String& value)
{
    for(const String& candidate : values)
        if(candidate == value)
            return true;
    return false;
}

bool UniqueStrings(const Vector<String>& values)
{
    Index<String> seen;
    for(const String& value : values) {
        if(seen.Find(value) >= 0)
            return false;
        seen.Add(value);
    }
    return true;
}

bool IsPositionToken(const String& value)
{
    static const char *values[] = {
        "top_left", "top", "top_right", "left", "center", "right",
        "bottom_left", "bottom", "bottom_right"
    };
    for(const char *candidate : values)
        if(value == candidate) return true;
    return false;
}

bool IsDirectionToken(const String& value)
{
    static const char *values[] = {
        "north_west", "north", "north_east", "west", "east",
        "south_west", "south", "south_east"
    };
    for(const char *candidate : values)
        if(value == candidate) return true;
    return false;
}

bool ValidateNumericDefault(TaskTrackItem& item, const String& prefix, String& error)
{
    if(IsNull(item.default_value))
        return true;
    if(!IsNumber(item.default_value)) {
        error = prefix + "default must be numeric.";
        return false;
    }
    double value = (double)item.default_value;
    if(item.has_min && value < item.min_value) {
        error = prefix + "default is below min.";
        return false;
    }
    if(item.has_max && value > item.max_value) {
        error = prefix + "default is above max.";
        return false;
    }
    return true;
}

bool ValidateItem(TaskTrackItem& item, const String& raw_type, const String& prefix, String& error)
{
    AddLegacyChoices(raw_type, item);

    if(item.title.IsEmpty()) {
        error = prefix + "title is required.";
        return false;
    }
    if(item.step_value <= 0.0) {
        error = prefix + "step must be greater than zero.";
        return false;
    }
    if(item.has_min && item.has_max && item.min_value > item.max_value) {
        error = prefix + "min must not exceed max.";
        return false;
    }
    if(!UniqueStrings(item.choices)) {
        error = prefix + "choices must be unique.";
        return false;
    }

    switch(item.type) {
    case TaskTrackItemType::Confirm:
        if(!item.choices.IsEmpty() && item.choices.GetCount() != 2) {
            error = prefix + "confirm accepts either no choices or exactly two display labels.";
            return false;
        }
        break;
    case TaskTrackItemType::SingleChoice:
    case TaskTrackItemType::MultiChoice:
    case TaskTrackItemType::Select:
    case TaskTrackItemType::ListSelect:
    case TaskTrackItemType::RankOrder:
        if(item.choices.IsEmpty()) {
            error = prefix + TaskTrackItemTypeName(item.type) + " requires choices.";
            return false;
        }
        break;
    case TaskTrackItemType::Number:
    case TaskTrackItemType::Amount:
        if(!ValidateNumericDefault(item, prefix, error))
            return false;
        break;
    case TaskTrackItemType::Range:
        if(!IsNull(item.default_value)) {
            if(!item.default_value.Is<ValueMap>()) {
                error = prefix + "range default must be {low,high}.";
                return false;
            }
            ValueMap def = item.default_value;
            if(!IsNumber(def["low"]) || !IsNumber(def["high"])) {
                error = prefix + "range default low/high must be numeric.";
                return false;
            }
            double low = (double)def["low"];
            double high = (double)def["high"];
            if(low > high || (item.has_min && low < item.min_value) ||
               (item.has_max && high > item.max_value)) {
                error = prefix + "range default must be ordered and inside min/max.";
                return false;
            }
        }
        break;
    case TaskTrackItemType::Rating:
        if(!item.has_min) { item.has_min = true; item.min_value = 1.0; }
        if(!item.has_max) { item.has_max = true; item.max_value = 5.0; }
        item.step_value = 1.0;
        if(item.max_value - item.min_value > 9.0 ||
           fabs(item.min_value - floor(item.min_value + 0.5)) > 0.000001 ||
           fabs(item.max_value - floor(item.max_value + 0.5)) > 0.000001) {
            error = prefix + "rating requires integer bounds spanning at most ten values.";
            return false;
        }
        break;
    case TaskTrackItemType::Color:
        if(!item.recommended.IsEmpty() && !ValidateColorText(item.recommended)) {
            error = prefix + "recommended color must be #RRGGBB or #RRGGBBAA.";
            return false;
        }
        break;
    case TaskTrackItemType::Gradient:
        if(item.gradients.IsEmpty()) {
            error = prefix + "gradient requires gradients.";
            return false;
        }
        if(!item.recommended.IsEmpty()) {
            bool found = false;
            for(const TaskTrackGradientOption& gradient : item.gradients)
                if(gradient.id == item.recommended) { found = true; break; }
            if(!found) {
                error = prefix + "recommended must match a gradient id.";
                return false;
            }
        }
        break;
    case TaskTrackItemType::Position:
        if(!IsNull(item.default_value) &&
           (!item.default_value.Is<String>() || !IsPositionToken(AsString(item.default_value)))) {
            error = prefix + "position default is invalid.";
            return false;
        }
        break;
    case TaskTrackItemType::Direction:
        if(!IsNull(item.default_value) &&
           (!item.default_value.Is<String>() || !IsDirectionToken(AsString(item.default_value)))) {
            error = prefix + "direction default is invalid.";
            return false;
        }
        break;
    case TaskTrackItemType::HierarchySelect:
        if(item.hierarchy.IsEmpty()) {
            error = prefix + "hierarchy_select requires hierarchy nodes.";
            return false;
        }
        break;
    case TaskTrackItemType::Curve:
        if(!IsNull(item.default_value)) {
            if(!item.default_value.Is<ValueArray>()) {
                error = prefix + "curve default must be [x1,y1,x2,y2].";
                return false;
            }
            ValueArray points = item.default_value;
            if(points.GetCount() != 4) {
                error = prefix + "curve default must contain four values.";
                return false;
            }
            for(int i = 0; i < 4; ++i) {
                if(!IsNumber(points[i]) || (double)points[i] < 0.0 || (double)points[i] > 1.0) {
                    error = prefix + "curve default values must be numbers from 0 to 1.";
                    return false;
                }
            }
        }
        break;
    default:
        break;
    }

    if(!item.recommended.IsEmpty()) {
        bool discrete = item.type == TaskTrackItemType::SingleChoice ||
                        item.type == TaskTrackItemType::Select ||
                        item.type == TaskTrackItemType::ListSelect;
        if(discrete && !HasString(item.choices, item.recommended)) {
            error = prefix + "recommended must match one of choices.";
            return false;
        }
    }

    return true;
}

bool ParseItem(const Value& raw, TaskTrackItem& item, const String& prefix, String& error)
{
    if(!raw.Is<ValueMap>()) {
        error = prefix + "must be an object.";
        return false;

    }

    const char *string_fields[] = {
        "id", "category", "type", "title", "instruction", "recommended",
        "unit", "expected_color", "expected_value"
    };
    for(const char *key : string_fields) {
        String field_error;
        if(!ExpectOptionalString(raw, key, field_error)) {
            error = prefix + field_error;
            return false;
        }
    }
    const char *bool_fields[] = { "required", "allow_multiple", "has_min", "has_max" };
    for(const char *key : bool_fields) {
        String field_error;
        if(!ExpectOptionalBool(raw, key, field_error)) {
            error = prefix + field_error;
            return false;
        }
    }
    const char *number_fields[] = { "min", "max", "step" };
    for(const char *key : number_fields) {
        String field_error;
        if(!ExpectOptionalNumber(raw, key, field_error)) {
            error = prefix + field_error;
            return false;
        }
    }

    item.id = TrimBoth(ReadString(raw, "id"));
    item.category = TrimBoth(ReadString(raw, "category", "General"));
    if(item.category.IsEmpty())
        item.category = "General";
    item.title = TrimBoth(ReadString(raw, "title"));
    item.instruction = ReadString(raw, "instruction");
    item.required = ReadBool(raw, "required", true);
    item.allow_multiple = ReadBool(raw, "allow_multiple", false);
    item.recommended = TrimBoth(ReadString(raw, "recommended"));
    item.unit = TrimBoth(ReadString(raw, "unit"));
    item.expected_color = TrimBoth(ReadString(raw, "expected_color"));
    item.expected_value = ReadString(raw, "expected_value");

    String raw_type = ToLower(TrimBoth(ReadString(raw, "type", "confirm")));
    if(!TaskTrackParseItemType(raw_type, item.type)) {
        error = prefix + "type is not supported: " + raw_type;
        return false;
    }

    item.has_min = ReadBool(raw, "has_min", !IsNull(raw["min"]));
    item.has_max = ReadBool(raw, "has_max", !IsNull(raw["max"]));
    item.min_value = ReadDouble(raw, "min", 0.0);
    item.max_value = ReadDouble(raw, "max", 100.0);
    item.step_value = ReadDouble(raw, "step", 1.0);
    item.default_value = raw["default"];

    if(!ValidateColorText(item.expected_color)) {
        error = prefix + "expected_color must be #RRGGBB or #RRGGBBAA.";
        return false;
    }

    if(!ParseStringArray(raw, "choices", item.choices, prefix, error))
        return false;
    if(!ParseStringArray(raw, "colors", item.colors, prefix, error))
        return false;
    for(int i = 0; i < item.colors.GetCount(); ++i) {
        if(!ValidateColorText(item.colors[i]) || item.colors[i].IsEmpty()) {
            error = Format("%scolors[%d] must be #RRGGBB or #RRGGBBAA.", prefix, i);
            return false;
        }
    }
    if(!ParseGradients(raw, item.gradients, prefix, error))
        return false;
    if(!ParseHierarchy(raw, item.hierarchy, prefix, error))
        return false;
    if(!ParseAnswer(raw["answer"], item.answer, prefix, error))
        return false;

    // V0.1 color meant an expected-colour witness (Match/Different/Unsure),
    // not a colour chooser. Preserve that meaning when the old expected_color
    // field is present without V2 colour choices.
    if(raw_type == "color" && item.colors.IsEmpty() && !item.expected_color.IsEmpty()) {
        raw_type = "legacy_color_witness";
        item.type = TaskTrackItemType::SingleChoice;
        String expected = "Expected colour: " + item.expected_color;
        item.instruction << (item.instruction.IsEmpty() ? "" : "  ") << expected;
    }

    if(!ValidateItem(item, raw_type, prefix, error))
        return false;
    NormalizeLegacyAnswer(raw_type, item);
    return true;
}

Value GradientToValue(const TaskTrackGradientOption& option)
{
    ValueMap m;
    m.Add("id", option.id);
    m.Add("label", option.label);
    m.Add("from", option.from_color);
    m.Add("to", option.to_color);
    return Value(m);
}

Value HierarchyToValue(const TaskTrackHierarchyNode& node)
{
    ValueMap m;
    m.Add("id", node.id);
    m.Add("parent_id", node.parent_id);
    m.Add("label", node.label);
    return Value(m);
}

} // namespace

String TaskTrackVersion()
{
    return "0.2.1";
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
    case TaskTrackItemType::Confirm:         return "confirm";
    case TaskTrackItemType::SingleChoice:    return "single_choice";
    case TaskTrackItemType::MultiChoice:     return "multi_choice";
    case TaskTrackItemType::Select:          return "select";
    case TaskTrackItemType::ListSelect:      return "list_select";
    case TaskTrackItemType::Text:            return "text";
    case TaskTrackItemType::Notes:           return "notes";
    case TaskTrackItemType::Number:          return "number";
    case TaskTrackItemType::Amount:          return "amount";
    case TaskTrackItemType::Range:           return "range";
    case TaskTrackItemType::Rating:          return "rating";
    case TaskTrackItemType::Color:           return "color";
    case TaskTrackItemType::Gradient:        return "gradient";
    case TaskTrackItemType::Position:        return "position";
    case TaskTrackItemType::Direction:       return "direction";
    case TaskTrackItemType::RankOrder:       return "rank_order";
    case TaskTrackItemType::HierarchySelect: return "hierarchy_select";
    case TaskTrackItemType::Curve:           return "curve";
    }
    return "confirm";
}

bool TaskTrackParseItemType(const String& text, TaskTrackItemType& type)
{
    String s = ToLower(TrimBoth(text));
    if(s == "confirm" || s == "check") { type = TaskTrackItemType::Confirm; return true; }
    if(s == "single_choice" || s == "choice" || s == "pass_fail" ||
       s == "file" || s == "interaction" || s == "visual_compare") {
        type = TaskTrackItemType::SingleChoice; return true;
    }
    if(s == "multi_choice")     { type = TaskTrackItemType::MultiChoice; return true; }
    if(s == "select")           { type = TaskTrackItemType::Select; return true; }
    if(s == "list_select")      { type = TaskTrackItemType::ListSelect; return true; }
    if(s == "text")             { type = TaskTrackItemType::Text; return true; }
    if(s == "notes" || s == "multiline") { type = TaskTrackItemType::Notes; return true; }
    if(s == "number")           { type = TaskTrackItemType::Number; return true; }
    if(s == "amount")           { type = TaskTrackItemType::Amount; return true; }
    if(s == "range")            { type = TaskTrackItemType::Range; return true; }
    if(s == "rating")           { type = TaskTrackItemType::Rating; return true; }
    if(s == "color" || s == "colour") { type = TaskTrackItemType::Color; return true; }
    if(s == "gradient")         { type = TaskTrackItemType::Gradient; return true; }
    if(s == "position")         { type = TaskTrackItemType::Position; return true; }
    if(s == "direction")        { type = TaskTrackItemType::Direction; return true; }
    if(s == "rank_order" || s == "rank") { type = TaskTrackItemType::RankOrder; return true; }
    if(s == "hierarchy_select") { type = TaskTrackItemType::HierarchySelect; return true; }
    if(s == "curve")            { type = TaskTrackItemType::Curve; return true; }
    return false;
}

String TaskTrackMakeTaskId()
{
    Time t = GetSysTime();
    uint64 a = Random64();
    uint64 b = Random64();
    return Format("task-%04d%02d%02d`T%02d%02d%02d-%016llx%016llx",
                  (int)t.year, (int)t.month, (int)t.day,
                  (int)t.hour, (int)t.minute, (int)t.second,
                  (int64)a, (int64)b);
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
    root.Add("schema_version", 2);
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
        m.Add("allow_multiple", item.allow_multiple);
        m.Add("recommended", item.recommended);
        m.Add("unit", item.unit);
        m.Add("has_min", item.has_min);
        m.Add("has_max", item.has_max);
        if(item.has_min) m.Add("min", item.min_value);
        if(item.has_max) m.Add("max", item.max_value);
        m.Add("step", item.step_value);
        m.Add("default", item.default_value);
        m.Add("expected_color", item.expected_color);
        m.Add("expected_value", item.expected_value);

        ValueArray choices;
        for(const String& choice : item.choices)
            choices.Add(choice);
        m.Add("choices", choices);

        ValueArray colors;
        for(const String& color : item.colors)
            colors.Add(color);
        m.Add("colors", colors);

        ValueArray gradients;
        for(const TaskTrackGradientOption& gradient : item.gradients)
            gradients.Add(GradientToValue(gradient));
        m.Add("gradients", gradients);

        ValueArray hierarchy;
        for(const TaskTrackHierarchyNode& node : item.hierarchy)
            hierarchy.Add(HierarchyToValue(node));
        m.Add("hierarchy", hierarchy);

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
    const char *document_number_fields[] = {
        "schema_version", "last_human_activity_epoch", "reminder_minutes",
        "reminder_count", "history_limit"
    };
    for(const char *key : document_number_fields)
        if(!ExpectOptionalNumber(value, key, error))
            return false;

    int source_schema = ReadInt(value, "schema_version", 1);
    if(source_schema != 1 && source_schema != 2) {
        error = Format("Unsupported TaskTrack schema version %d.", source_schema);
        return false;
    }

    TaskTrackDocument parsed;
    parsed.schema_version = 2;
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
    parsed.reminder_minutes = min(max(ReadInt(value, "reminder_minutes", 10), 0), 24 * 60);
    parsed.remind_while_paused = ReadBool(value, "remind_while_paused", false);
    parsed.nudge_on_agent_poll = ReadBool(value, "nudge_on_agent_poll", false);
    parsed.reminder_count = max(0, ReadInt(value, "reminder_count", 0));
    parsed.history_limit = min(max(ReadInt(value, "history_limit", 20), 5), 200);

    if(parsed.title.IsEmpty()) {
        error = "TaskTrack title is required.";
        return false;
    }

    if(!TaskTrackParseState(ReadString(value, "state", "awaiting_human"), parsed.state)) {
        error = "TaskTrack state is invalid.";
        return false;
    }

    Value raw_items = value["items"];
    if(!raw_items.Is<ValueArray>()) {
        error = "TaskTrack items must be an array.";
        return false;
    }

    Index<String> ids;
    const ValueArray& items = raw_items;
    for(int i = 0; i < items.GetCount(); ++i) {
        TaskTrackItem item;
        String prefix = Format("TaskTrack item %d: ", i);
        if(!ParseItem(items[i], item, prefix, error))
            return false;
        if(item.id.IsEmpty())
            item.id = Format("item-%d", i + 1);
        if(ids.Find(item.id) >= 0) {
            error = "Duplicate TaskTrack item id: " + item.id;
            return false;
        }
        ids.Add(item.id);
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
    const char *number_fields[] = { "reminder_minutes", "history_limit" };
    for(const char *key : number_fields)
        if(!ExpectOptionalNumber(args, key, error))
            return false;

    TaskTrackDocument created;
    created.schema_version = 2;
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
    created.reminder_minutes = min(max(ReadInt(args, "reminder_minutes", 10), 0), 24 * 60);
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
        TaskTrackItem item;
        String prefix = Format("items[%d].", i);
        if(!ParseItem(array[i], item, prefix, error))
            return false;
        if(item.id.IsEmpty())
            item.id = Format("item-%d", i + 1);
        if(ids.Find(item.id) >= 0) {
            error = "Duplicate item id: " + item.id;
            return false;
        }
        ids.Add(item.id);
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
        out << "- [" << (item.answer.answered ? "x" : " ") << "] **"
            << MarkdownEscape(item.title) << "**";
        if(item.required)
            out << " *(required)*";
        out << "\n";
        if(!item.instruction.IsEmpty())
            out << "  - Question: " << MarkdownEscape(item.instruction) << "\n";
        out << "  - Type: `" << TaskTrackItemTypeName(item.type) << "`\n";
        if(!item.recommended.IsEmpty())
            out << "  - Agent recommendation: " << MarkdownEscape(item.recommended) << "\n";
        if(item.answer.answered) {
            if(!item.answer.status.IsEmpty())
                out << "  - Status: **" << MarkdownEscape(item.answer.status) << "**\n";
            String data = DataToDisplay(item.answer.data);
            if(!data.IsEmpty())
                out << "  - Answer: " << MarkdownEscape(data) << "\n";
            else if(!item.answer.value.IsEmpty())
                out << "  - Answer: " << MarkdownEscape(item.answer.value) << "\n";
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
    root.Add("schema_version", 2);
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
        Value serialized = TaskTrackToValue(doc);
        root.Add("items", serialized["items"]);
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
        TaskTrackDocument item;
        String load_error;
        if(!TaskTrackLoad(paths[i], item, load_error))
            continue;
        result.Add(TaskTrackStatusValue(item, paths[i], false));
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
        TaskTrackDocument item;
        String error;
        if(!TaskTrackLoad(paths[i], item, error))
            continue;
        if(item.state != TaskTrackState::Completed && item.state != TaskTrackState::Closed)
            continue;
        FileDelete(paths[i] + ".bak");
        if(FileDelete(paths[i])) {
            String locator = AppendFileName(TaskTrackDefaultRegistryRoot(), item.task_id + ".path");
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
