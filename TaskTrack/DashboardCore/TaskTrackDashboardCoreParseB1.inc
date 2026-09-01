bool ParseEntry(const Value& value, TaskTrackDashboardEntry& entry,
                const String& prefix, String& error)
{
    if(!value.Is<ValueMap>()) {
        error = prefix + "must be an object.";
        return false;
    }

    const char *string_fields[] = {
        "id", "title", "subtitle", "status", "detail", "category",
        "timestamp", "task_id", "evidence"
    };
    for(const char *key : string_fields) {
        String field_error;
        if(!ExpectOptionalString(value, key, field_error)) {
            error = prefix + field_error;
            return false;
        }
    }
    const char *number_fields[] = { "progress", "weight" };
    for(const char *key : number_fields) {
        String field_error;
        if(!ExpectOptionalNumber(value, key, field_error)) {
            error = prefix + field_error;
            return false;
        }
    }
    const char *bool_fields[] = { "attention", "archived" };
    for(const char *key : bool_fields) {
        String field_error;
        if(!ExpectOptionalBool(value, key, field_error)) {
            error = prefix + field_error;
            return false;
        }
    }

    entry.id = TrimBoth(ReadString(value, "id"));
    entry.title = TrimBoth(ReadString(value, "title"));
    entry.subtitle = ReadString(value, "subtitle");
    entry.status = TrimBoth(ReadString(value, "status"));
    entry.detail = ReadString(value, "detail");
    entry.category = TrimBoth(ReadString(value, "category"));
    entry.progress = ReadDouble(value, "progress", -1.0);
    entry.weight = ReadDouble(value, "weight", 1.0);
    entry.attention = ReadBool(value, "attention", false);
    entry.archived = ReadBool(value, "archived", false);
    entry.timestamp = ReadString(value, "timestamp");
    entry.task_id = TrimBoth(ReadString(value, "task_id"));
    entry.evidence = ReadString(value, "evidence");
    entry.data = value["data"];

    if(entry.id.IsEmpty()) {
        error = prefix + "id is required.";
        return false;
    }
    if(entry.title.IsEmpty()) {
        error = prefix + "title is required.";
        return false;
    }
    if(entry.progress < -1.0 || entry.progress > 100.0) {
        error = prefix + "progress must be 0..100 when supplied.";
        return false;
    }
    if(entry.weight <= 0.0 || entry.weight > 1000000.0) {
        error = prefix + "weight must be greater than zero and bounded.";
        return false;
    }
    return true;
}
