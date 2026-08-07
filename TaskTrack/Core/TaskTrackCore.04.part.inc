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
    return "0.2.0";
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

