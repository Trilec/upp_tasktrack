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
    parsed.reminder_minutes = min(max(ReadInt(value, "reminder_minutes", 60), 0), 24 * 60);
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
